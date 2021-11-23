/*******************************************************************************


   Copyright (C) 2011-2018 SequoiaDB Ltd.

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU Affero General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU Affero General Public License for more details.

   You should have received a copy of the GNU Affero General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.

   Source File Name = InsertMainTest.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          11/20/2021  LYC  Initial Draft

   Last Changed =

******************************************************************************/
#include "test_def.h"
#include "vessel/vesselImpl.h"
#include "vessel/requestContext.h"
#include "ossUtil.hpp"
#include "vessel/IRedoLogger.h"
#include "dpsLogRecord.hpp"
#include "vessel/listCollectionSpaceDef.h"
#include "vessel/listCollectionsDef.h"
#include "vessel/logRecordContext.h"
#include "dpsLogRecord.hpp"
#include "../bson/bson.hpp"
#include "pd.hpp"
#include "vessel/collectionOptions.h"

#include <boost/filesystem.hpp>
namespace fs = boost::filesystem;


static const CHAR *CSNAME = "foo";
static const CHAR *CLNAME = "bar";
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
                    const CHAR *csName, const CHAR *clName,
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
   slice record;
   record.reset(obj.objsize(), obj.objdata());

   collectionHandler handler;
   rc = db->openCollection(&session, csName, clName, openCLOptions(), handler);
   if (SDB_OK != rc)
   {
      goto error;
   }

   for (UINT32 i = 0; i < count; ++i)
   {
      utilInsertResult res;
      rc = handler.insert(&session, record, 
                          INVALID_STRIPING_ID,
                          insertOptions(), &res);
      if (SDB_OK != rc)
      {
         goto error;
      }
      ++insertCounts[x];
   }
   handler.close();

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
   rc = db.createCollectionSpace(&session, CSNAME, 1, createCSOptions());
   if (SDB_OK != rc)
   {
      goto error;
   }
   rc = db.createCollection(&session, CSNAME, CLNAME, 1, createCLOptions());
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
                                         CSNAME, CLNAME, countPerThread, 
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
   collectionHandler handler;

   rc = db->openCollection(&session, csName, clName, openCLOptions(), handler);
   if (SDB_OK != rc)
   {
      goto error;
   }

   for (UINT32 i = 0; i < count; ++i)
   {
      ossItoa(ossRand(), randStr, 32);
      builder.reset();
      builder.append("a", randStr, 32);
      builder.append("b", 2);
      builder.append("c", pad, 1024);
      bson::BSONObj obj = builder.done();
      slice record;
      record.reset(obj.objsize(), obj.objdata());
      utilInsertResult res;
      rc = handler.insert(&session, record,
                          INVALID_STRIPING_ID,
                          insertOptions(), &res);
      if (SDB_OK != rc)
      {
         goto error;
      }
      ++insertCounts[x];
   }

done:   
   handler.close();
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
   collectionHandler handler;
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

   createCSOptions csOptions;
   csOptions.dataSegSize = STORAGE_FILE_SEGMENT_SIZE_32MB;

   closeDBOptions co;
   co.closeMode = closeDBOptions::CLOSE_MODE_IMMDIETE;

   collectionHandler clHandler;

   indexParameters params;
   params.type = INDEX_TYPE_BTREE;
   params.isUnique = TRUE;

   rc = db.open(&session, &resource, options);
   if (SDB_OK != rc)
   {
      goto error;
   }
   rc = db.createCollectionSpace(&session, CSNAME, 1, csOptions);
   if (SDB_OK != rc)
   {
      goto error;
   }
   rc = db.createCollection(&session, CSNAME, CLNAME, 1, createCLOptions());
   if (SDB_OK != rc)
   {
      goto error;
   }
   rc = db.openCollection(&session, CSNAME, CLNAME, openCLOptions(), clHandler);
   if (SDB_OK != rc)
   {
      goto error;
   }
   rc = clHandler.createIndex(&session, strSlice("index1"),
                         BSON("a" << 1), params, createIndexOptions());
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
   insertTestwithIndex(datapath, lsmpath, lsmpath, recordcount, threadcount);
   // cout << "Insert without Index:" << endl;
   // insertTest(datapath, lsmpath, lsmpath, recordcount, threadcount);
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