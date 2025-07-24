/*******************************************************************************

   Copyright (C) 2011-Present SequoiaDB Ltd.

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

      http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.

   Source File Name = benchmark_test.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          11/20/2021  LYC  Initial Draft

   Last Changed =

*******************************************************************************/
#include "test_def.h"
#include "vessel/vesselImpl.h"
#include "vessel/requestContext.h"
#include "ossUtil.hpp"
#include "dpsLogRecord.hpp"
#include "vessel/listCollectionSpaceDef.h"
#include "vessel/listCollectionsDef.h"
#include "dpsLogRecord.hpp"
#include "../bson/bson.hpp"
#include "pd.hpp"
#include "interface/IDataCollection.h"

#include <boost/filesystem.hpp>
namespace fs = boost::filesystem;


constexpr CHAR *CSNAME = "foo";
constexpr CHAR *CLNAME = "bar";
constexpr CHAR *FULLNAME = "foo.bar";

std::atomic<UINT32> quitThreadCount(0);

void printCountPerSecond(UINT32 recordcount, 
                         UINT32 threadcount,
                         std::atomic<UINT32> insertCounts[])
{
   UINT32 sum = 0;
   UINT32 total = 0;
   for (;;)
   {
      ossSleep(1000);
      for (UINT32 i = 0; i < threadcount; ++i)
      {
         sum += insertCounts[i].exchange(0);
      }
      cout << "Record counts in one second:" << sum << endl;
      total += sum;
      sum = 0;
      if (total >= recordcount || 0 == quitThreadCount)
      {
         break;
      }
   }
}

INT32 normal_insert(vesselImpl *db,
                    const CHAR *fullName,
                    UINT32 count, std::atomic<UINT32> insertCounts[], UINT32 x)
{
   INT32 rc = SDB_OK;
   test_executor session;
   CHAR pad[1024] = {0};

   bson::BSONObjBuilder builder;
   builder.append("a", 1);
   builder.append("b", 2);
   builder.append("c", pad, 1024);
   bson::BSONObj obj = builder.obj();

   DATA_COLLECTION_PTR handler;
   rc = db->openCL(&session, fullName, dmsOpenCLOptions(), handler);
   if (SDB_OK != rc)
   {
      goto error;
   }

   for (UINT32 i = 0; i < count; ++i)
   {
      utilInsertResult res;
      rc = handler->insertRecord(&session, obj, dmsInsertRecordOptions(), &res);
      if (SDB_OK != rc)
      {
         goto error;
      }
      ++insertCounts[x];
   }
   handler->close();

done:
   --quitThreadCount;
   return rc;
error:
   goto done;

}

// multi thread insert 
void insertTest(CHAR *datapath, CHAR *lsmpath, CHAR *logpath, 
                UINT32 recordcount, UINT32 threadcount)
{
   INT32 rc = SDB_OK;
   vesselImpl db;
   outerResource resource = test_outer_resource::getResource();
   sdbEnablePD(logpath, 1, 1000);
   setPDLevel(PDDEBUG);
   test_executor session;
   openDBOptions options;
   options.path.dataPath = datapath;
   options.path.lsmPath = lsmpath;
   utilInsertResult res;
   CHAR pad[1024] = {0};

   bson::BSONObjBuilder builder;
   builder.append("a", 1);
   builder.append("b", 2);
   builder.append("c", pad, 1024);
   bson::BSONObj obj = builder.obj();

   bson::BSONObj adjunct;

   std::thread threads[threadcount];
   UINT32 countPerThread = recordcount / threadcount;
   UINT32 mod = 0;
   if (0 != recordcount % threadcount)
   {
      mod = recordcount % threadcount;
   }
   std::atomic<UINT32> *insertCounts = new std::atomic<UINT32>[threadcount];
   for (UINT32 i = 0; i < threadcount; ++i)
   {
      insertCounts[i] = 0;
   }
   quitThreadCount = threadcount;

   rc = db.open(&session, &resource, options);
   if (SDB_OK != rc)
   {
      goto error;
   }
   rc = db.createCS(&session, CSNAME, 1, dmsCreateCSOptions(), adjunct);
   if (SDB_OK != rc)
   {
      goto error;
   }
   rc = db.createCL(&session, FULLNAME, 1, dmsCreateCLOptions(), adjunct);
   if (SDB_OK != rc)
   {
      goto error;
   }

   for (UINT32 i = 0; i < threadcount; ++i)
   {
      if (mod != 0 && i == threadcount - 1)
      {
         countPerThread += mod;
      }
      threads[i] = std::move(std::thread(normal_insert, &db, 
                                         FULLNAME, countPerThread, 
                                         insertCounts, i));
   }
   printCountPerSecond(recordcount, threadcount, insertCounts);

   for (UINT32 i = 0; i < threadcount; ++i)
   {
      threads[i].join();
   }

done:   
   db.close(&session, closeDBOptions());
   delete []insertCounts;
   return;
error:
   goto done;
}

INT32 index_insert(vesselImpl *db,
                  const CHAR *csName, const CHAR *clName,
                  UINT32 count,
                  std::atomic<UINT32> insertCounts[], UINT32 x)
{
   INT32 rc = SDB_OK ;
   test_executor session;
   CHAR pad[1024] = {0};
   CHAR randStr[32] = {0};
   ossMemset(randStr, 'a', sizeof(randStr) - 1);
   randStr[31] = '\0';
   bson::BSONObjBuilder builder;
   DATA_COLLECTION_PTR handler;
   dmsInsertRecordOptions options;

   rc = db->openCL(&session, FULLNAME, dmsOpenCLOptions(), handler);
   if (SDB_OK != rc)
   {
      goto error;
   }

   for (UINT32 i = 0; i < count; ++i)
   {
      for (UINT32 i = 0; i < 31; ++i)
      {
         randStr[i] = 'a' + ossRand() % 26;
      }
      randStr[31] = '\0';
      builder.reset();
      builder.append("a", randStr, 32);
      builder.append("b", 2);
      builder.append("c", pad, 1024);
      bson::BSONObj obj = builder.done();
      utilInsertResult res;
      rc = handler->insertRecord(&session, obj,
                                 options, &res);
      if (SDB_OK != rc)
      {
         goto error;
      }
      ++insertCounts[x];
   }

   handler->close();

done:   
   
   --quitThreadCount;
   return rc;
error:
   goto done;
}

// multi thread insert with index
void insertTestwithIndex(CHAR *datapath, CHAR *lsmpath, CHAR *logpath, 
                         UINT32 recordcount, UINT32 threadcount)
{
   INT32 rc = SDB_OK;
   vesselImpl db;
   outerResource resource = test_outer_resource::getResource();
   sdbEnablePD(logpath, 1, 1000);
   setPDLevel(PDDEBUG);
   test_executor session;
   openDBOptions options;
   options.path.dataPath = datapath;
   options.path.lsmPath = lsmpath;
   DATA_COLLECTION_PTR handler;
   std::thread threads[threadcount];
   UINT32 countPerThread = recordcount / threadcount;
   UINT32 mod = 0;
   if (0 != recordcount % threadcount)
   {
      mod = recordcount % threadcount;
   }

   bson::BSONObj indexDef;
   bson::BSONObj adjunct;

   std::atomic<UINT32> *insertCounts = new std::atomic<UINT32>[threadcount];
   for (UINT32 i = 0; i < threadcount; ++i)
   {
      insertCounts[i] = 0;
   }
   quitThreadCount = threadcount;

   closeDBOptions co;
   co.closeMode = closeDBOptions::CLOSE_MODE_IMMDIETE;

   dmsBuildIndexOptions buildOptions;

   rc = db.open(&session, &resource, options);
   if (SDB_OK != rc)
   {
      goto error;
   }
   rc = db.createCS(&session, CSNAME, 1, dmsCreateCSOptions(), adjunct);
   if (SDB_OK != rc)
   {
      goto error;
   }
   rc = db.createCL(&session, FULLNAME, 1, dmsCreateCLOptions(), adjunct);
   if (SDB_OK != rc)
   {
      goto error;
   }
   rc = db.openCL(&session, FULLNAME, dmsOpenCLOptions(), handler);
   if (SDB_OK != rc)
   {
      goto error;
   }

   indexDef = indexTestUtil::createIndexObj(INDEX_TYPE_BTREE,
                                            "index1",
                                            FALSE, BSON("a" << 1));
   rc = handler->createIndex(&session, buildOptions, indexDef);
   if (SDB_OK != rc)
   {
      goto error;
   }

   for (UINT32 i = 0; i < threadcount; ++i)
   {
      if (mod != 0 && i == threadcount - 1)
      {
         countPerThread += mod;
      }
      threads[i] = std::move(std::thread(index_insert,
                                         &db, CSNAME, CLNAME,
                                         countPerThread, insertCounts, i));
   }
   printCountPerSecond(recordcount, threadcount, insertCounts);

   for (UINT32 i = 0; i < threadcount; ++i)
   {
      threads[i].join();
   }

done:
   db.close(&session, co);
   delete []insertCounts;
   return;
error:
   goto done;
}

INT32 main(INT32 argc, CHAR** argv)
{
   INT32 arg = 0;
   CHAR datapath[OSS_MAX_PATHSIZE] = "/opt/unit_test/vessel";
   CHAR lsmpath[OSS_MAX_PATHSIZE] = "/opt/unit_test/lsm";
   CHAR logpath[OSS_MAX_PATHSIZE] = "/opt/unit_test/log";
   UINT32 threadcount = 1;
   UINT32 recordcount = 100000000;

   while ((arg = getopt(argc, argv, "d:s:l:c:j:")) != -1)
   {
      switch (arg)
      {
      case 'd':
         ossStrcpy(datapath, optarg);
         break;
      case 's':
         ossStrcpy(lsmpath, optarg);
         break;
      case 'l':
         ossStrcpy(logpath, optarg);
         break;
      case 'c':
         recordcount = ossAtoi(optarg);
         break;
      case 'j':
         threadcount = ossAtoi(optarg);
         break;
      default:
         break;
      }
   }
   {
      fs::path testPath(datapath);
      fs::remove_all(testPath);
      fs::create_directory(testPath);
   }
   {
      fs::path testPath(lsmpath);
      fs::remove_all(testPath);
      fs::create_directory(testPath);
   }
   {
      fs::path testPath(logpath);
      fs::remove_all(testPath);
      fs::create_directory(testPath);
   }
   cout << "Insert with Index:" << endl;
   insertTestwithIndex(datapath, lsmpath, logpath, recordcount, threadcount);
   // cout << "Insert without Index:" << endl;
   // insertTest(datapath, lsmpath, logpath, recordcount, threadcount);
   {
      fs::path testPath(datapath);
      fs::remove_all(testPath);
   }
   {
      fs::path testPath(lsmpath);
      fs::remove_all(testPath);
   }
   {
      fs::path testPath(logpath);
      fs::remove_all(testPath);
   }
   return 0;

}