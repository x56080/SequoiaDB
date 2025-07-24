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

   Source File Name = insert_test.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

*******************************************************************************/
#include "test_def.h"
#include "vessel/vesselImpl.h"
#include "vessel/requestContext.h"
#include <gtest/gtest.h>
#include "ossUtil.hpp"
#include "dpsLogRecord.hpp"
#include "vessel/listCollectionSpaceDef.h"
#include "vessel/listCollectionsDef.h"
#include "dpsLogRecord.hpp"
#include "../bson/bson.hpp"
#include "pd.hpp"

#include <boost/filesystem.hpp>
namespace fs = boost::filesystem;


class insert_test : public testing::Test
{
   public:
   static void SetUpTestCase()
   {
      {
      fs::path testPath(DATA_PATH);
      fs::remove_all(testPath);
      fs::create_directory(testPath);
      }
      {
      fs::path testPath(LSM_PATH);
      fs::remove_all(testPath);
      fs::create_directory(testPath);
      }
   }

   static void TearDownTestCase()
   {
      {
      fs::path testPath(DATA_PATH);
      fs::remove_all(testPath);
      }
      {
      fs::path testPath(LSM_PATH);
      fs::remove_all(testPath);
      }
   }

   virtual void SetUp()
   {
      {
      fs::path testPath(DATA_PATH);
      fs::remove_all(testPath);
      fs::create_directory(testPath);
      }
      {
      fs::path testPath(LSM_PATH);
      fs::remove_all(testPath);
      fs::create_directory(testPath);
      }
   }
};

TEST_F(insert_test, test1)
{
   INT32 rc = SDB_OK;

   vesselImpl db;
   outerResource resource = test_outer_resource::getResource();
   test_executor session;
   openDBOptions options;
   options.path.dataPath = DATA_PATH;
   options.path.lsmPath = LSM_PATH;

   utilInsertResult res;
   CHAR pad[1024] = {0};

   bson::BSONObjBuilder builder;
   builder.append("a", 1);
   builder.append("b", 2);
   builder.append("c", pad, 1024);
   bson::BSONObj obj = builder.obj();

   UINT32 count = 100;
   UINT64 recordCount = 0;

   DATA_COLLECTION_PTR handler;

   rc = db.open(&session, &resource, options);
   ASSERT_EQ(SDB_OK, rc);

   rc = db.createCS(&session, "foo", 1, dmsCreateCSOptions(), bson::BSONObj());
   ASSERT_EQ(SDB_OK, rc);

   rc = db.createCL(&session, "foo.bar1", 1, dmsCreateCLOptions(), bson::BSONObj());
   ASSERT_EQ(SDB_OK, rc);

   rc = db.openCL(&session, "foo.bar1", dmsOpenCLOptions(), handler);
   ASSERT_EQ(SDB_OK, rc);

   for (UINT32 i = 0; i < count; ++i)
   {
      rc = handler->insertRecord(&session, obj, dmsInsertRecordOptions(), &res);
      ASSERT_EQ(SDB_OK, rc);
   }

   rc = handler->getRecordCount(&session, recordCount);
   ASSERT_EQ(SDB_OK, rc);
   ASSERT_EQ(count, recordCount);
   
   DATA_CURSOR_PTR cursor;
   
   rc = handler->scan(&session, dmsScanOptions(), cursor);
   ASSERT_EQ(SDB_OK, rc);

   
   for (UINT32 i = 0; i < count; ++i)
   {
      rc = cursor->fetchNext(&session);
      ASSERT_EQ(SDB_OK, rc);
      bson::BSONObj r = cursor->getBsonRecord();
      ASSERT_EQ(1, r.getIntField("a"));
      ASSERT_EQ(2, r.getIntField("b"));
   }
   rc = cursor->fetchNext(&session);
   ASSERT_EQ(SDB_DMS_EOC, rc);

   rc = db.close(&session, closeDBOptions());
   ASSERT_EQ(SDB_OK, rc);
}

TEST_F(insert_test, test2)
{
   INT32 rc = SDB_OK;
   vesselImpl db;
   outerResource resource = test_outer_resource::getResource();
   test_executor session;
   openDBOptions options;
   options.path.dataPath = DATA_PATH;
   options.path.lsmPath = LSM_PATH;

   utilInsertResult res;
   CHAR pad[1024] = {0};
   bson::BSONObjBuilder builder;
   builder.append("a", 1);
   builder.append("b", 2);
   builder.append("c", pad, 1024);
   bson::BSONObj obj = builder.obj();
   UINT32 count = 100;
   UINT64 recordCount = 0;

   DATA_CURSOR_PTR cursor;
   
   DATA_COLLECTION_PTR cl;

   rc = db.open(&session, &resource, options);
   ASSERT_EQ(SDB_OK, rc);

   rc = db.createCS(&session, "foo", 1, dmsCreateCSOptions(), bson::BSONObj());
   ASSERT_EQ(SDB_OK, rc);

   rc = db.createCL(&session, "foo.bar1", 1, dmsCreateCLOptions(), bson::BSONObj());
   ASSERT_EQ(SDB_OK, rc);

   rc = db.openCL(&session, "foo.bar1", dmsOpenCLOptions(), cl);
   ASSERT_EQ(SDB_OK, rc);

   for (UINT32 i = 0; i < count; ++i)
   {
      rc = cl->insertRecord(&session, obj, dmsInsertRecordOptions(), &res);
      ASSERT_EQ(SDB_OK, rc);
   }

   rc = cl->getRecordCount(&session, recordCount);
   ASSERT_EQ(SDB_OK, rc);
   ASSERT_EQ(count, recordCount);
   
   rc = cl->scan(&session, dmsScanOptions(), cursor);
   ASSERT_EQ(SDB_OK, rc);

   
   for (UINT32 i = 0; i < count; ++i)
   {
      rc = cursor->fetchNext(&session);
      bson::BSONObj r = cursor->getBsonRecord();
      ASSERT_EQ(1, r.getIntField("a"));
      ASSERT_EQ(2, r.getIntField("b"));
   }
   rc = cursor->fetchNext(&session);
   ASSERT_EQ(SDB_DMS_EOC, rc);

   rc = db.close(&session, closeDBOptions());
   ASSERT_EQ(SDB_OK, rc);

   rc = db.open(&session, &resource, options);
   ASSERT_EQ(SDB_OK, rc);

   rc = db.openCL(&session, "foo.bar1", dmsOpenCLOptions(), cl);
   ASSERT_EQ(SDB_OK, rc);

   rc = cl->getRecordCount(&session, recordCount);
   ASSERT_EQ(SDB_OK, rc);
   ASSERT_EQ(count, recordCount);
   
   rc = cl->scan(&session, dmsScanOptions(), cursor);
   ASSERT_EQ(SDB_OK, rc);

   
   for (UINT32 i = 0; i < count; ++i)
   {
      rc = cursor->fetchNext(&session);
      bson::BSONObj r = cursor->getBsonRecord();
      ASSERT_EQ(1, r.getIntField("a"));
      ASSERT_EQ(2, r.getIntField("b"));
   }
   rc = cursor->fetchNext(&session);
   ASSERT_EQ(SDB_DMS_EOC, rc);

   rc = db.close(&session, closeDBOptions());
   ASSERT_EQ(SDB_OK, rc);
}

void thread_insert(vesselImpl *db,
                   const CHAR *fullName,
                   UINT32 count)
{
   test_executor session;
   CHAR pad[1024] = {0};
   bson::BSONObjBuilder builder;
   builder.append("a", 1);
   builder.append("b", 2);
   builder.append("c", pad, 1024);
   bson::BSONObj obj = builder.obj();

   DATA_COLLECTION_PTR handler;
   INT32 rc = db->openCL(&session, fullName, dmsOpenCLOptions(), handler);
   ASSERT_EQ(SDB_OK, rc);

   for (UINT32 i = 0; i < count; ++i)
   {
      utilInsertResult res;
      rc = handler->insertRecord(&session, obj, dmsInsertRecordOptions(), &res);
      ASSERT_EQ(SDB_OK, rc);
   }
}

TEST_F(insert_test, test3_1)
{
   INT32 rc = SDB_OK;
   vesselImpl db;
   outerResource resource = test_outer_resource::getResource();
   test_executor session;
   openDBOptions options;

   options.path.dataPath = DATA_PATH;
   options.path.lsmPath = LSM_PATH;
   options.bufferPoolOptions.flushDirtyListThreshold = 0.9f;
   UINT32 count = 6000000;
   static const UINT32 threadCount = 6;
   std::thread threads[threadCount];
   UINT32 countPerThread = count / threadCount;

   closeDBOptions co;

   rc = db.open(&session, &resource, options);
   ASSERT_EQ(SDB_OK, rc);

   rc = db.createCS(&session, "foo", 1, dmsCreateCSOptions(), bson::BSONObj());
   ASSERT_EQ(SDB_OK, rc);

   rc = db.createCL(&session, "foo.bar", 1, dmsCreateCLOptions(), bson::BSONObj());
   ASSERT_EQ(SDB_OK, rc);

   for (UINT32 i = 0; i < threadCount; ++i)
   {
      threads[i] = std::move(std::thread(thread_insert, &db,
                                         "foo.bar", countPerThread));
   }

   for (UINT32 i = 0; i < threadCount; ++i)
   {
      threads[i].join();
   }

   rc = db.close(&session, co);
   ASSERT_EQ(SDB_OK, rc);
}

TEST_F(insert_test, test3_2)
{
   INT32 rc = SDB_OK;
   vesselImpl db;
   outerResource resource = test_outer_resource::getResource();
   test_executor session;
   openDBOptions options;

   options.path.dataPath = DATA_PATH;
   options.path.lsmPath = LSM_PATH;
   options.bufferPoolOptions.maxMemSize = (UINT64)2 << 30;
   UINT32 count = 6000000;
   static const UINT32 threadCount = 6;
   std::thread threads[threadCount];
   UINT32 countPerThread = count / threadCount;

   closeDBOptions co;
   co.closeMode = closeDBOptions::CLOSE_MODE_IMMDIETE;

   rc = db.open(&session, &resource, options);
   ASSERT_EQ(SDB_OK, rc);

   rc = db.createCS(&session, "foo", 1, dmsCreateCSOptions(), bson::BSONObj());
   ASSERT_EQ(SDB_OK, rc);

   rc = db.createCL(&session, "foo.bar", 1, dmsCreateCLOptions(), bson::BSONObj());
   ASSERT_EQ(SDB_OK, rc);

   for (UINT32 i = 0; i < threadCount; ++i)
   {
      threads[i] = std::move(std::thread(thread_insert, &db, 
                                         "foo.bar", countPerThread));
   }

   for (UINT32 i = 0; i < threadCount; ++i)
   {
      threads[i].join();
   }

   rc = db.close(&session, co);
   ASSERT_EQ(SDB_OK, rc);
}


TEST_F(insert_test, test4)
{
   INT32 rc = SDB_OK;
   vesselImpl db;
   outerResource resource = test_outer_resource::getResource();
   test_executor session;
   openDBOptions options;
   options.path.dataPath = DATA_PATH;
   options.path.lsmPath = LSM_PATH;

   options.bufferPoolOptions.maxMemSize = (UINT64)1 << 30;
   UINT32 count = 4000000;
   static const UINT32 threadCount = 4;
   std::thread threads[threadCount];
   UINT32 countPerThread = count / threadCount;

   UINT64 recordCount = 0;
   DATA_COLLECTION_PTR handler;
   DATA_CURSOR_PTR cursor;
   

   rc = db.open(&session, &resource, options);
   ASSERT_EQ(SDB_OK, rc);

   rc = db.createCS(&session, "foo", 1, dmsCreateCSOptions(), bson::BSONObj());
   ASSERT_EQ(SDB_OK, rc);

   rc = db.createCL(&session, "foo.bar", 1, dmsCreateCLOptions(), bson::BSONObj());
   ASSERT_EQ(SDB_OK, rc);

   for (UINT32 i = 0; i < threadCount; ++i)
   {
      threads[i] = std::move(std::thread(thread_insert, &db,
                                         "foo.bar", countPerThread));
   }

   for (UINT32 i = 0; i < threadCount; ++i)
   {
      threads[i].join();
   }

   rc = db.openCL(&session, "foo.bar", dmsOpenCLOptions(), handler);
   ASSERT_EQ(SDB_OK, rc);

   rc = handler->getRecordCount(&session, recordCount);
   ASSERT_EQ(SDB_OK, rc);
   ASSERT_EQ(count, recordCount);
   
   rc = handler->scan(&session, dmsScanOptions(), cursor);
   ASSERT_EQ(SDB_OK, rc);

   
   for (UINT32 i = 0; i < count; ++i)
   {
      rc = cursor->fetchNext(&session);
      ASSERT_EQ(1, cursor->getBsonRecord().getIntField("a"));
      ASSERT_EQ(2, cursor->getBsonRecord().getIntField("b"));
   }
   rc = cursor->fetchNext(&session);
   ASSERT_EQ(SDB_DMS_EOC, rc);

   handler.reset();
   rc = db.close(&session, closeDBOptions());
   ASSERT_EQ(SDB_OK, rc);

   rc = db.open(&session, &resource, options);
   ASSERT_EQ(SDB_OK, rc);

   rc = db.openCL(&session, "foo.bar", dmsOpenCLOptions(), handler);
   ASSERT_EQ(SDB_OK, rc);

   rc = handler->getRecordCount(&session, recordCount);
   ASSERT_EQ(SDB_OK, rc);
   ASSERT_EQ(count, recordCount);
   
   rc = handler->scan(&session, dmsScanOptions(), cursor);
   ASSERT_EQ(SDB_OK, rc);

   
   for (UINT32 i = 0; i < count; ++i)
   {
      rc = cursor->fetchNext(&session);
      ASSERT_EQ(1, cursor->getBsonRecord().getIntField("a"));
      ASSERT_EQ(2, cursor->getBsonRecord().getIntField("b"));
   }
   rc = cursor->fetchNext(&session);
   ASSERT_EQ(SDB_DMS_EOC, rc);

   rc = db.close(&session, closeDBOptions());
   ASSERT_EQ(SDB_OK, rc);
}

void thread_insert_striping(vesselImpl *db,
                            const CHAR *fullName,
                            UINT32 count)
{
   test_executor session;
   CHAR pad[1024] = {0};
   bson::BSONObjBuilder builder;
   builder.append("a", 1);
   builder.append("b", 2);
   builder.append("c", pad, 1024);
   bson::BSONObj obj = builder.obj();

   DATA_COLLECTION_PTR handler;
   INT32 rc = db->openCL(&session, fullName, dmsOpenCLOptions(), handler);
   ASSERT_EQ(SDB_OK, rc);

   for (UINT32 i = 0; i < count; ++i)
   {
      utilInsertResult res;
      dmsInsertRecordOptions o;
      o.stripingId = dmsStripingId(ossRand() % 65535);
      rc = handler->insertRecord(&session, obj, o, &res);
      ASSERT_EQ(SDB_OK, rc);
   }
   handler->close();
}

/*
TEST_F(insert_test, test5)
{
   INT32 rc = SDB_OK;
   vesselImpl db;
   outerResource resource = test_outer_resource::getResource();
   test_executor session;
   openDBOptions options;
   options.path.dataPath = DATA_PATH;
   options.path.lsmPath = LSM_PATH;
   options.cacheOptions.flush.flushDirtyListThreshold = 0.8;
   options.cacheOptions.freelist.maxChunkCount = 1024;

   UINT32 count = 4000000;
   static const UINT32 threadCount = 4;
   std::thread threads[threadCount];
   UINT32 countPerThread = count / threadCount;

   closeDBOptions co;
   co.closeMode = closeDBOptions::CLOSE_MODE_IMMDIETE;

   rc = db.open(&session, &resource, options);
   ASSERT_EQ(SDB_OK, rc);

   rc = db.createCS(&session, "foo", 1, dmsCreateCSOptions(), bson::BSONObj());
   ASSERT_EQ(SDB_OK, rc);

   clOptions.minFreePercent = 0;
   clOptions.minStriping = 0;
   clOptions.maxStriping = 65534;
   
   rc = db.createCollection(&session, "foo", "bar1", 1, clOptions);
   ASSERT_EQ(SDB_OK, rc);

   for (UINT32 i = 0; i < threadCount; ++i)
   {
      threads[i] = std::move(std::thread(thread_insert_striping, &db,
                                         "foo", "bar1", countPerThread));
   }

   for (UINT32 i = 0; i < threadCount; ++i)
   {
      threads[i].join();
   }

   rc = db.close(&session, co);
   ASSERT_EQ(SDB_OK, rc);
}

TEST_F(insert_test, test6)
{
   INT32 rc = SDB_OK;
   vesselImpl db;
   outerResource resource = test_outer_resource::getResource();
   test_executor session;
   openDBOptions options;
   options.path.dataPath = DATA_PATH;
   options.path.lsmPath = LSM_PATH;

   UINT32 count = 4000000;
   static const UINT32 threadCount = 4;
   std::thread threads[threadCount];
   UINT32 countPerThread = count / threadCount;

   UINT64 recordCount = 0;

   createCLOptions clOptions;
   clOptions.minFreePercent = 0;
   clOptions.minStriping = 0;
   clOptions.maxStriping = 65534;

   collectionSpaceId csid;

   rc = db.open(&session, &resource, options);
   ASSERT_EQ(SDB_OK, rc);

   rc = db.createCollectionSpace(&session, "foo", 1, createCSOptions(), csid);
   ASSERT_EQ(SDB_OK, rc);

   rc = db.createCollection(&session, "foo", "bar1", 1, clOptions);
   ASSERT_EQ(SDB_OK, rc);

   for (UINT32 i = 0; i < threadCount; ++i)
   {
      threads[i] = std::move(std::thread(thread_insert_striping, &db,
                                         "foo", "bar1", countPerThread));
   }

   for (UINT32 i = 0; i < threadCount; ++i)
   {
      threads[i].join();
   }

   rc = db.openCollection(&session, "foo", "bar1", openCLOptions(), handler);
   ASSERT_EQ(SDB_OK, rc);

   rc = handler.getTotalRecordCountInPageHead(&session, recordCount);
   ASSERT_EQ(SDB_OK, rc);
   ASSERT_EQ(count, recordCount);
   
   rc = handler.openScanCursor(&session, NULL, collectionScanOptions(), cursor);
   ASSERT_EQ(SDB_OK, rc);

   for (UINT32 i = 0; i < count; ++i)
   {
      recordRow.reset();
      rc = cursor.getNextRow(&session, recordRow);
      bson::BSONObj obj(recordRow.getRecord().data());
      ASSERT_EQ(1, obj.getIntField("a"));
      ASSERT_EQ(2, obj.getIntField("b"));
   }
   rc = cursor.getNextRow(&session, recordRow);
   ASSERT_EQ(SDB_VESSEL_EOC, rc);

   cursor.close();
   handler.close();
   rc = db.close(&session, closeDBOptions());
   ASSERT_EQ(SDB_OK, rc);

   rc = db.open(&session, &resource, options);
   ASSERT_EQ(SDB_OK, rc);

   rc = db.openCollection(&session, "foo", "bar1", openCLOptions(), handler);
   ASSERT_EQ(SDB_OK, rc);

    rc = handler.getTotalRecordCountInPageHead(&session, recordCount);
   ASSERT_EQ(SDB_OK, rc);
   ASSERT_EQ(count, recordCount);

   rc = handler.openScanCursor(&session, NULL, collectionScanOptions(), cursor);
   ASSERT_EQ(SDB_OK, rc);

   handler.close();

   for (UINT32 i = 0; i < count; ++i)
   {
      recordRow.reset();
      rc = cursor.getNextRow(&session, recordRow);
      ASSERT_EQ(SDB_OK, rc);
      bson::BSONObj obj(recordRow.getRecord().data());
      ASSERT_EQ(1, obj.getIntField("a"));
      ASSERT_EQ(2, obj.getIntField("b"));
   }
   rc = cursor.getNextRow(&session, recordRow);
   ASSERT_EQ(SDB_VESSEL_EOC, rc);

   cursor.close();
   rc = db.close(&session, closeDBOptions());
   ASSERT_EQ(SDB_OK, rc);
}
*/

void thread_insert_index(vesselImpl *db,
                         const CHAR *fullName,
                         UINT32 count)
{
   test_executor session;
   CHAR pad[1024] = {0};
   bson::BSONObjBuilder builder;
   
   DATA_COLLECTION_PTR handler;
   INT32 rc = db->openCL(&session, fullName, dmsOpenCLOptions(), handler);
   ASSERT_EQ(SDB_OK, rc);

   for (UINT32 i = 0; i < count; ++i)
   {
      builder.reset();
      builder.append("a", ossRand());   /// just for benchmark.
      builder.append("b", 2);
      builder.append("c", pad, 1024);
      bson::BSONObj obj = builder.done();
      utilInsertResult res;
      rc = handler->insertRecord(&session, obj, dmsInsertRecordOptions(), &res);
      ASSERT_EQ(SDB_OK, rc);
   }
   handler->close();
}

void thread_insert_unique_index(vesselImpl *db,
                                 const CHAR *fullName,
                                 UINT32 begin, UINT32 count)
{
   test_executor session;
   CHAR pad[1024] = {0};
   bson::BSONObjBuilder builder;
   DATA_COLLECTION_PTR handler;
   INT32 rc = db->openCL(&session, fullName, dmsOpenCLOptions(), handler);
   ASSERT_EQ(SDB_OK, rc);

   for (UINT32 i = 0; i < count; ++i)
   {
      builder.reset();
      builder.append("a", i + begin);
      builder.append("b", 2);
      builder.append("c", pad, 1024);
      bson::BSONObj obj = builder.done();
      utilInsertResult res;
      rc = handler->insertRecord(&session, obj, dmsInsertRecordOptions(), &res);
      ASSERT_EQ(SDB_OK, rc);
   }
   handler->close();
}

void insert_test_nonunique_index(INDEX_TYPE type)
{
   INT32 rc = SDB_OK;
   vesselImpl db;
   outerResource resource = test_outer_resource::getResource();
   test_executor session;
   openDBOptions options;

   options.path.dataPath = DATA_PATH;
   options.path.lsmPath = LSM_PATH;

   UINT32 count = 6000000;
   static const UINT32 threadCount = 6;
   std::thread threads[threadCount];
   UINT32 countPerThread = count / threadCount;

   closeDBOptions co;
   co.closeMode = closeDBOptions::CLOSE_MODE_IMMDIETE;

   DATA_COLLECTION_PTR cl;

   rc = db.open(&session, &resource, options);
   ASSERT_EQ(SDB_OK, rc);

   rc = db.createCS(&session, "foo", 1, dmsCreateCSOptions(), bson::BSONObj());
   ASSERT_EQ(SDB_OK, rc);

   rc = db.createCL(&session, "foo.bar", 1, dmsCreateCLOptions(), bson::BSONObj());
   ASSERT_EQ(SDB_OK, rc);

   rc = db.openCL(&session, "foo.bar", dmsOpenCLOptions(), cl);
   ASSERT_EQ(SDB_OK, rc);

   rc = cl->createIndex(&session, dmsBuildIndexOptions(),
                        indexTestUtil::createIndexObj(type, "index", FALSE, BSON("a" << 1)));
   ASSERT_EQ(SDB_OK, rc);                   

   for (UINT32 i = 0; i < threadCount; ++i)
   {
      threads[i] = std::move(std::thread(thread_insert_index, &db,
                                         "foo.bar", countPerThread));
   }

   for (UINT32 i = 0; i < threadCount; ++i)
   {
      threads[i].join();
   }

   rc = db.close(&session, co);
   ASSERT_EQ(SDB_OK, rc);
}

/// insert with nonunique index
TEST_F(insert_test, test8_1_1)
{
   insert_test_nonunique_index(INDEX_TYPE_HYBRID_TREE);
}


void insert_test_unique_index(INDEX_TYPE type)
{
   INT32 rc = SDB_OK;
   vesselImpl db;
   outerResource resource = test_outer_resource::getResource();
   test_executor session;
   openDBOptions options;

   options.path.dataPath = DATA_PATH;
   options.path.lsmPath = LSM_PATH;

   UINT32 count = 6000000;
   static const UINT32 threadCount = 6;
   std::thread threads[threadCount];
   UINT32 countPerThread = count / threadCount;

   closeDBOptions co;
   co.closeMode = closeDBOptions::CLOSE_MODE_IMMDIETE;

   DATA_COLLECTION_PTR clHandler;

   rc = db.open(&session, &resource, options);
   ASSERT_EQ(SDB_OK, rc);

   rc = db.createCS(&session, "foo", 1, dmsCreateCSOptions(), bson::BSONObj());
   ASSERT_EQ(SDB_OK, rc);

   rc = db.createCL(&session, "foo.bar", 1, dmsCreateCLOptions(), bson::BSONObj());
   ASSERT_EQ(SDB_OK, rc);

   rc = db.openCL(&session, "foo.bar", dmsOpenCLOptions(), clHandler);
   ASSERT_EQ(SDB_OK, rc);

   rc = clHandler->createIndex(&session, dmsBuildIndexOptions(),
                        indexTestUtil::createIndexObj(type, "index", TRUE, BSON("a" << 1)));
   ASSERT_EQ(SDB_OK, rc);                   

   for (UINT32 i = 0; i < threadCount; ++i)
   {
      threads[i] = std::move(std::thread(thread_insert_unique_index,
                                         &db,
                                         "foo.bar", i * countPerThread,
                                         countPerThread));
   }

   for (UINT32 i = 0; i < threadCount; ++i)
   {
      threads[i].join();
   }

   rc = db.close(&session, co);
   ASSERT_EQ(SDB_OK, rc);
}

/// insert with unique index
TEST_F(insert_test, test8_2_1)
{
   insert_test_unique_index(INDEX_TYPE_HYBRID_TREE);
}


void death_thread_insert(vesselImpl *db,
                         const CHAR *fullName,
                         UINT32 count,
                         UINT32 i,
                         const slice &pad,
                         atomic_int *counter)
{
   test_executor session;
   session._id = i;
   bson::BSONObjBuilder builder;
   bson::StringBuilder b;

   DATA_COLLECTION_PTR handler;
   INT32 rc = db->openCL(&session, fullName, dmsOpenCLOptions(), handler);
   ASSERT_EQ(SDB_OK, rc);

   for (UINT32 i = 0; i < count; ++i)
   {
      UINT32 r = ossRand();
      b << r << "aaaaaaaaaaaaaaaaaaaaaaaa";
      builder.append("a", b.poolStr());
      builder.append("b", i);
      if (pad.isValid())
      {
         builder.append("c", pad.data(), pad.size());
      }
      bson::BSONObj obj = builder.done();
      utilInsertResult res;
      rc = handler->insertRecord(&session, obj, dmsInsertRecordOptions(), &res);
      ASSERT_EQ(SDB_OK, rc);
      counter->fetch_add(1, std::memory_order_relaxed);
      builder.reset();
      b.reset();
   }
   handler->close();
   cout << "thread quit:" << i << endl;
}


/// insert into single cl by multi threads.
TEST_F(insert_test, DISABLED_death_test_1)
{
   INT32 rc = SDB_OK;
   vesselImpl db;
   outerResource resource = test_outer_resource::getResource();
   test_executor session;
   openDBOptions options;

   options.path.dataPath = DATA_PATH;
   options.path.lsmPath = LSM_PATH;
   DATA_COLLECTION_PTR handler;

   constexpr UINT32 threadCount = 8;
   std::thread threads[threadCount];
   atomic_int counters[threadCount] = {};
   UINT32 countPerThread = 12000000;
   UINT32 count = 0;
   UINT64 readCount = 0;
   CHAR padbuf[1000] = {};
   slice pad(sizeof(padbuf), padbuf);

   closeDBOptions co;

   rc = db.open(&session, &resource, options);
   ASSERT_EQ(SDB_OK, rc);

   rc = db.createCS(&session, "foo", 1, dmsCreateCSOptions(), bson::BSONObj());
   ASSERT_EQ(SDB_OK, rc);

   rc = db.createCL(&session, "foo.bar", 1, dmsCreateCLOptions(), bson::BSONObj());
   ASSERT_EQ(SDB_OK, rc);

   for (UINT32 i = 0; i < threadCount; ++i)
   {
      threads[i] = std::move(std::thread(death_thread_insert, &db,
                                         "foo.bar", countPerThread,
                                         i, pad, counters+i));
   }

   do
   {
      ossSleep(1000);
      UINT32 countPerSecond = 0;
      for (UINT32 i = 0; i < threadCount; ++i)
      {
         countPerSecond += counters[i].exchange(0, std::memory_order_relaxed);
      }

      count += countPerSecond;
      cout << "count per second:" << countPerSecond << ", total count:" << count << endl;
   } while (count < (countPerThread * threadCount));
   

   for (UINT32 i = 0; i < threadCount; ++i)
   {
      threads[i].join();
   }

   rc = db.close(&session, co);
   ASSERT_EQ(SDB_OK, rc);

   rc = db.open(&session, &resource, options);
   ASSERT_EQ(SDB_OK, rc);
   rc = db.openCL(&session, "foo.bar", dmsOpenCLOptions(), handler);
   ASSERT_EQ(SDB_OK, rc);
   rc = handler->getRecordCount(&session, readCount);
   ASSERT_EQ(SDB_OK, rc);
   ASSERT_EQ(count, readCount);

   rc = db.close(&session, co);
   ASSERT_EQ(SDB_OK, rc);
}

/// insert into single cl by multi threads.
TEST_F(insert_test, DISABLED_death_test_2)
{
   INT32 rc = SDB_OK;
   vesselImpl db;
   outerResource resource = test_outer_resource::getResource();
   test_executor session;
   openDBOptions options;

   options.path.dataPath = DATA_PATH;
   options.path.lsmPath = LSM_PATH;
   DATA_COLLECTION_PTR handler;

   constexpr UINT32 threadCount = 8;
   std::thread threads[threadCount];
   atomic_int counters[threadCount] = {};
   UINT32 countPerThread = 10000000;
   UINT32 count = 0;
   slice pad;

   closeDBOptions co;

   rc = db.open(&session, &resource, options);
   ASSERT_EQ(SDB_OK, rc);

   rc = db.createCS(&session, "foo", 1, dmsCreateCSOptions(), bson::BSONObj());
   ASSERT_EQ(SDB_OK, rc);

   rc = db.createCL(&session, "foo.bar", 1, dmsCreateCLOptions(), bson::BSONObj());
   ASSERT_EQ(SDB_OK, rc);

   DATA_COLLECTION_PTR cl;
   rc = db.openCL(&session, "foo.bar", dmsOpenCLOptions(), cl);
   ASSERT_EQ(SDB_OK, rc);

   bson::BSONObj indexDef = indexTestUtil::createIndexObj(INDEX_TYPE_HYBRID_TREE, "index", 
                                                          TRUE, BSON("a" << 1));
   rc = cl->createIndex(&session, dmsBuildIndexOptions(), indexDef);
   ASSERT_EQ(SDB_OK, rc);

   for (UINT32 i = 0; i < threadCount; ++i)
   {
      threads[i] = std::move(std::thread(death_thread_insert, &db,
                                         "foo.bar", countPerThread,
                                         i, pad, counters+i));
   }

   do
   {
      ossSleep(1000);
      UINT32 countPerSecond = 0;
      for (UINT32 i = 0; i < threadCount; ++i)
      {
         countPerSecond += counters[i].exchange(0, std::memory_order_relaxed);
      }

      cout << "total count per second:" << countPerSecond << endl;
      count += countPerSecond;
   } while (count < (countPerThread * threadCount));
   

   for (UINT32 i = 0; i < threadCount; ++i)
   {
      threads[i].join();
   }

   rc = db.close(&session, co);
   ASSERT_EQ(SDB_OK, rc);
}

void death_thread_loop_insert(vesselImpl *db,
                              const CHAR *fullNamePrefix,
                              UINT32 count,
                              UINT32 i,
                              UINT32 clCount,
                              atomic_int *counter)
{
   INT32 rc = SDB_OK;
   test_executor session;
   session._id = i;
   CHAR pad[1024] = {0};
   bson::BSONObjBuilder builder;
   builder.append("a", 1);
   builder.append("b", 2);
   builder.append("c", pad, 1024);
   bson::BSONObj obj = builder.obj();

   vector<DATA_COLLECTION_PTR> cls;
   for (UINT32 i = 0; i < clCount; ++i)
   {
      std::stringstream ss;
      ss << i;
      std::string clName;
      clName.append(fullNamePrefix);
      clName.append(ss.str());
      DATA_COLLECTION_PTR handler;
      rc = db->openCL(&session, clName.c_str(), dmsOpenCLOptions(), handler);
      ASSERT_EQ(SDB_OK, rc);
      cls.push_back(handler);
   }
   
   for (UINT32 i = 0; i < count; ++i)
   {
      utilInsertResult res;
      rc = cls[i % clCount]->insertRecord(&session, obj, dmsInsertRecordOptions(), &res);
      ASSERT_EQ(SDB_OK, rc);
      counter->fetch_add(1, std::memory_order_relaxed);
   }

   cout << "thread quit:" << i << endl;
}

/// insert into multi collections by single thread each
TEST_F(insert_test, DISABLED_death_test_3)
{
   INT32 rc = SDB_OK;
   vesselImpl db;
   outerResource resource = test_outer_resource::getResource();
   test_executor session;
   openDBOptions options;

   options.path.dataPath = DATA_PATH;
   options.path.lsmPath = LSM_PATH;
   DATA_COLLECTION_PTR handler;

   constexpr UINT32 threadCount = 8;
   std::thread threads[threadCount];
   atomic_int counters[threadCount] = {};
   UINT32 countPerThread = 10000000;
   UINT32 count = 0;

   closeDBOptions co;

   rc = db.open(&session, &resource, options);
   ASSERT_EQ(SDB_OK, rc);

   rc = db.createCS(&session, "foo", 1, dmsCreateCSOptions(), bson::BSONObj());
   ASSERT_EQ(SDB_OK, rc);

   for (UINT32 i = 0; i < threadCount; ++i)
   {
      std::stringstream ss;
      ss << i;
      std::string clName;
      clName.append("foo.bar");
      clName.append(ss.str());
      rc = db.createCL(&session, clName.c_str(), i + 1,
                       dmsCreateCLOptions(), bson::BSONObj());
      ASSERT_EQ(SDB_OK, rc);
   }

   for (UINT32 i = 0; i < threadCount; ++i)
   {
      threads[i] = std::move(std::thread(death_thread_loop_insert, &db,
                                         "foo.bar", countPerThread,
                                         i, threadCount, counters+i));
   }

   do
   {
      ossSleep(1000);
      UINT32 countPerSecond = 0;
      for (UINT32 i = 0; i < threadCount; ++i)
      {
         countPerSecond += counters[i].exchange(0, std::memory_order_relaxed);
      }

      cout << "total count per second:" << countPerSecond << endl;
      count += countPerSecond;
   } while (count < (countPerThread * threadCount));
   

   for (UINT32 i = 0; i < threadCount; ++i)
   {
      threads[i].join();
   }

   rc = db.close(&session, co);
   ASSERT_EQ(SDB_OK, rc);
}

/// insert into single cl with multi lsm indexes.
TEST_F(insert_test, DISABLED_death_test_4)
{
   INT32 rc = SDB_OK;
   vesselImpl db;
   outerResource resource = test_outer_resource::getResource();
   test_executor session;
   openDBOptions options;

   options.path.dataPath = DATA_PATH;
   options.path.lsmPath = LSM_PATH;
   DATA_COLLECTION_PTR handler;

   constexpr UINT32 threadCount = 8;
   std::thread threads[threadCount];
   atomic_int counters[threadCount] = {};
   UINT32 countPerThread = 10000000;
   UINT32 count = 0;
   CHAR padbuf[1000] = {};
   slice pad(sizeof(padbuf), padbuf);

   closeDBOptions co;

   rc = db.open(&session, &resource, options);
   ASSERT_EQ(SDB_OK, rc);

   rc = db.createCS(&session, "foo", 1, dmsCreateCSOptions(), bson::BSONObj());
   ASSERT_EQ(SDB_OK, rc);

   rc = db.createCL(&session, "foo.bar", 1, dmsCreateCLOptions(), bson::BSONObj());
   ASSERT_EQ(SDB_OK, rc);

   DATA_COLLECTION_PTR cl;
   rc = db.openCL(&session, "foo.bar", dmsOpenCLOptions(), cl);
   ASSERT_EQ(SDB_OK, rc);

   {
   bson::BSONObj indexDef = indexTestUtil::createIndexObj(INDEX_TYPE_HYBRID_TREE, "index", 
                                                          FALSE, BSON("a" << 1));
   rc = cl->createIndex(&session, dmsBuildIndexOptions(), indexDef);
   ASSERT_EQ(SDB_OK, rc);
   }

   /*
   {
   bson::BSONObj indexDef = indexTestUtil::createIndexObj(INDEX_TYPE_HYBRID_TREE, "index", 
                                                          FALSE, BSON("b" << 1));
   rc = cl->createIndex(&session, dmsBuildIndexOptions(), indexDef);
   ASSERT_EQ(SDB_OK, rc);
   }*/

   for (UINT32 i = 0; i < threadCount; ++i)
   {
      threads[i] = std::move(std::thread(death_thread_insert, &db,
                                         "foo.bar", countPerThread,
                                         i, pad, counters+i));
   }

   do
   {
      ossSleep(1000);
      UINT32 countPerSecond = 0;
      for (UINT32 i = 0; i < threadCount; ++i)
      {
         countPerSecond += counters[i].exchange(0, std::memory_order_relaxed);
      }

      cout << "total count per second:" << countPerSecond << endl;
      count += countPerSecond;
   } while (count < (countPerThread * threadCount));
   

   for (UINT32 i = 0; i < threadCount; ++i)
   {
      threads[i].join();
   }

   rc = db.close(&session, co);
   ASSERT_EQ(SDB_OK, rc);
}

/*
Name: base_insert_test9
Description: 
   插入多条大记录
   1. 插入多条大记录
   2. 验证记录插入正确性
Expected Result: 
   记录成功插入且查询结果正确
*/
TEST_F(insert_test, base_insert_test9)
{
   vesselImpl db;
   outerResource resource = test_outer_resource::getResource();
   test_executor executor;
   openDBOptions options;
   DATA_COLLECTION_PTR handler;
   options.path.dataPath = DATA_PATH;
   options.path.lsmPath = LSM_PATH;
   dmsCreateCSOptions csOptions;
   INT32 rc = SDB_OK;
   bson::BSONObjBuilder builder;
   UINT32 count = 10000;

   rc = db.open(&executor, &resource, options);
   ASSERT_EQ(SDB_OK, rc);

   rc = db.createCS(&executor, "foo", 1, csOptions, bson::BSONObj());
   ASSERT_EQ(SDB_OK, rc);

   rc = db.createCL(&executor, "foo.bar", 1, dmsCreateCLOptions(), bson::BSONObj());
   ASSERT_EQ(SDB_OK, rc);

   rc = db.openCL(&executor, "foo.bar", dmsOpenCLOptions(), handler);
   ASSERT_EQ(SDB_OK, rc);

   string insertStr(csOptions.dataPageSize + 1000, 'x');
   for (UINT32 i = 0; i < count; ++i)
   {
      utilInsertResult insertRes;
      builder.reset();
      builder.append("a", insertStr);
      bson::BSONObj obj = builder.done();
      rc = handler->insertRecord(&executor, obj, dmsInsertRecordOptions(), &insertRes);
      ASSERT_EQ(SDB_OK, rc);
   }

   DATA_CURSOR_PTR cursor;
   rc = handler->scan(&executor, dmsScanOptions(), cursor);
   ASSERT_EQ(SDB_OK, rc);

   
   
   for (UINT32 i = 0; i < count; ++i)
   {
      rc = cursor->fetchNext(&executor);
      ASSERT_EQ(SDB_OK, rc);
      bson::BSONObj r = cursor->getBsonRecord();
      ASSERT_EQ(r.getStringField("a"), insertStr);
   }
   
   db.close(&executor, closeDBOptions());
}

/*
Name: advanced_insert_test1
Description: 
   多线程插入大记录
   1. 插入多条大记录
   2. 验证记录插入正确性
Expected Result: 
   记录成功插入且查询结果正确
*/
void big_record_thread_insert(vesselImpl *db, 
                              const CHAR *fullName, 
                              const string &record, 
                              UINT32 count)
{
   INT32 rc = SDB_OK;
   test_executor executor;
   DATA_COLLECTION_PTR handler;
   bson::BSONObjBuilder builder;

   rc = db->openCL(&executor, fullName, dmsOpenCLOptions(), handler);
   ASSERT_EQ(SDB_OK, rc);

   for (UINT32 i = 0; i < count; ++i)
   {
      utilInsertResult insertRes;
      builder.append("a", record);
      bson::BSONObj obj = builder.done();
      rc = handler->insertRecord(&executor, obj, dmsInsertRecordOptions(), &insertRes);
      ASSERT_EQ(SDB_OK ,rc);
      builder.reset();
   }
}

TEST_F(insert_test, advanced_insert_test1)
{
   vesselImpl db;
   outerResource resource = test_outer_resource::getResource();
   test_executor executor;
   openDBOptions options;
   DATA_COLLECTION_PTR handler;
   options.path.dataPath = DATA_PATH;
   options.path.lsmPath = LSM_PATH;
   dmsCreateCSOptions csOptions;

   INT32 rc = SDB_OK;
   bson::BSONObjBuilder builder;
   UINT32 count = 10000;
   static const UINT32 threadCount = 4;
   UINT32 countPerThread = count / threadCount;
   std::thread threads[threadCount];
   string insertStr(csOptions.dataPageSize + 1000, 'x');

   rc = db.open(&executor, &resource, options);
   ASSERT_EQ(SDB_OK, rc);

   rc = db.createCS(&executor, "foo", 1, csOptions, bson::BSONObj());
   ASSERT_EQ(SDB_OK, rc);

   rc = db.createCL(&executor, "foo.bar", 1, dmsCreateCLOptions(), bson::BSONObj());
   ASSERT_EQ(SDB_OK, rc);

   rc = db.openCL(&executor, "foo.bar", dmsOpenCLOptions(), handler);
   ASSERT_EQ(SDB_OK, rc);

   for (UINT32 i = 0; i < threadCount; ++i)
   {
      threads[i] = std::move(std::thread(big_record_thread_insert, &db,
                                         "foo.bar", insertStr, countPerThread));
   }

   for (UINT32 i = 0; i < threadCount; ++i)
   {
      threads[i].join();
   }

   DATA_CURSOR_PTR cursor;
   rc = handler->scan(&executor, dmsScanOptions(), cursor);
   ASSERT_EQ(SDB_OK, rc);

   
   
   for (UINT32 i = 0; i < count; ++i)
   {
      rc = cursor->fetchNext(&executor);
      ASSERT_EQ(SDB_OK, rc);
      bson::BSONObj r = cursor->getBsonRecord();
      ASSERT_EQ(r.getStringField("a"), insertStr);
   }

   db.close(&executor, closeDBOptions());
}
