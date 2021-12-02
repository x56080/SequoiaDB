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

   Source File Name = insert_test.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#include "test_def.h"
#include "vessel/vesselImpl.h"
#include "vessel/requestContext.h"
#include <gtest/gtest.h>
#include "ossUtil.hpp"
#include "vessel/IRedoLogger.h"
#include "dpsLogRecord.hpp"
#include "vessel/listCollectionSpaceDef.h"
#include "vessel/listCollectionsDef.h"
#include "vessel/logRecordContext.h"
#include "dpsLogRecord.hpp"
#include "../bson/bson.hpp"
#include "pd.hpp"
#include "vessel/dataScanRow.h"
#include "vessel/collectionOptions.h"

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
   collectionHandler handler;
   utilInsertResult res;
   CHAR pad[1024] = {0};

   bson::BSONObjBuilder builder;
   builder.append("a", 1);
   builder.append("b", 2);
   builder.append("c", pad, 1024);
   bson::BSONObj obj = builder.obj();
   slice record(obj.objsize(), obj.objdata());

   UINT32 count = 100;
   UINT64 recordCount = 0;
   dataScanRow recordRow;

   cursorHandler cursor;

   rc = db.open(&session, &resource, options);
   ASSERT_EQ(SDB_OK, rc);

   rc = db.createCollectionSpace(&session, "foo", 1, createCSOptions());
   ASSERT_EQ(SDB_OK, rc);

   rc = db.createCollection(&session, "foo", "bar1", 1, createCLOptions());
   ASSERT_EQ(SDB_OK, rc);

   rc = db.openCollection(&session, "foo", "bar1", openCLOptions(), handler);
   ASSERT_EQ(SDB_OK, rc);

   for (UINT32 i = 0; i < count; ++i)
   {
      rc = handler.insert(&session, record,
                          INVALID_STRIPING_ID, insertOptions(), &res);
      ASSERT_EQ(SDB_OK, rc);
   }

   rc = handler.getTotalRecordCountInPageHead(&session, recordCount);
   ASSERT_EQ(SDB_OK, rc);
   ASSERT_EQ(count, recordCount);
   
   rc = handler.openScanCursor(&session, NULL, collectionScanOptions(), cursor);
   ASSERT_EQ(SDB_OK, rc);

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
   handler.close();
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
   collectionHandler handler;
   slice record;
   utilInsertResult res;
   CHAR pad[1024] = {0};
   bson::BSONObjBuilder builder;
   builder.append("a", 1);
   builder.append("b", 2);
   builder.append("c", pad, 1024);
   bson::BSONObj obj = builder.obj();
   record.reset(obj.objsize(), obj.objdata());
   UINT32 count = 100;
   UINT64 recordCount = 0;
   dataScanRow recordRow;

   cursorHandler cursor;

   rc = db.open(&session, &resource, options);
   ASSERT_EQ(SDB_OK, rc);

   rc = db.createCollectionSpace(&session, "foo", 1, createCSOptions());
   ASSERT_EQ(SDB_OK, rc);

   rc = db.createCollection(&session, "foo", "bar1", 1, createCLOptions());
   ASSERT_EQ(SDB_OK, rc);

   rc = db.openCollection(&session, "foo", "bar1", openCLOptions(), handler);
   ASSERT_EQ(SDB_OK, rc);

   for (UINT32 i = 0; i < count; ++i)
   {
      rc = handler.insert(&session, record,
                          INVALID_STRIPING_ID,
                          insertOptions(), &res);
      ASSERT_EQ(SDB_OK, rc);
   }

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

void thread_insert(vesselImpl *db,
                   const CHAR *csName, const CHAR *clName,
                   UINT32 count)
{
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
   INT32 rc = db->openCollection(&session, csName, clName, openCLOptions(), handler);
   ASSERT_EQ(SDB_OK, rc);

   for (UINT32 i = 0; i < count; ++i)
   {
      utilInsertResult res;
      rc = handler.insert(&session, record, 
                          INVALID_STRIPING_ID,
                          insertOptions(), &res);
      ASSERT_EQ(SDB_OK, rc);
   }
   handler.close();
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
   options.cacheOptions.flush.flushDirtyListThreshold = 0.8;
   options.cacheOptions.freelist.maxChunkCount = 1024;
   collectionHandler handler;
   UINT32 count = 6000000;
   static const UINT32 threadCount = 6;
   std::thread threads[threadCount];
   UINT32 countPerThread = count / threadCount;

   createCSOptions csOptions;
   csOptions.dataSegSize = STORAGE_FILE_SEGMENT_SIZE_32MB;

   closeDBOptions co;
   co.closeMode = closeDBOptions::CLOSE_MODE_IMMDIETE;

   rc = db.open(&session, &resource, options);
   ASSERT_EQ(SDB_OK, rc);

   rc = db.createCollectionSpace(&session, "foo", 1, csOptions);
   ASSERT_EQ(SDB_OK, rc);

   rc = db.createCollection(&session, "foo", "bar1", 1, createCLOptions());
   ASSERT_EQ(SDB_OK, rc);

   for (UINT32 i = 0; i < threadCount; ++i)
   {
      threads[i] = std::move(std::thread(thread_insert, &db,
                                         "foo", "bar1", countPerThread));
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
   options.cacheOptions.freelist.maxChunkCount = 64;
   collectionHandler handler;
   UINT32 count = 6000000;
   static const UINT32 threadCount = 6;
   std::thread threads[threadCount];
   UINT32 countPerThread = count / threadCount;

   createCSOptions csOptions;
   csOptions.dataSegSize = STORAGE_FILE_SEGMENT_SIZE_32MB;

   closeDBOptions co;
   co.closeMode = closeDBOptions::CLOSE_MODE_IMMDIETE;

   rc = db.open(&session, &resource, options);
   ASSERT_EQ(SDB_OK, rc);

   rc = db.createCollectionSpace(&session, "foo", 1, csOptions);
   ASSERT_EQ(SDB_OK, rc);

   rc = db.createCollection(&session, "foo", "bar1", 1, createCLOptions());
   ASSERT_EQ(SDB_OK, rc);

   for (UINT32 i = 0; i < threadCount; ++i)
   {
      threads[i] = std::move(std::thread(thread_insert, &db, 
                                         "foo", "bar1", countPerThread));
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

   options.cacheOptions.freelist.maxChunkCount = 32;
   collectionHandler handler;
   UINT32 count = 4000000;
   static const UINT32 threadCount = 4;
   std::thread threads[threadCount];
   UINT32 countPerThread = count / threadCount;

   cursorHandler cursor;
   UINT64 recordCount = 0;
   dataScanRow recordRow;

   rc = db.open(&session, &resource, options);
   ASSERT_EQ(SDB_OK, rc);

   rc = db.createCollectionSpace(&session, "foo", 1, createCSOptions());
   ASSERT_EQ(SDB_OK, rc);

   rc = db.createCollection(&session, "foo", "bar1", 1, createCLOptions());
   ASSERT_EQ(SDB_OK, rc);

   for (UINT32 i = 0; i < threadCount; ++i)
   {
      threads[i] = std::move(std::thread(thread_insert, &db,
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

void thread_insert_striping(vesselImpl *db,
                            const CHAR *csName, const CHAR *clName,
                            UINT32 count)
{
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
   INT32 rc = db->openCollection(&session, csName, clName, openCLOptions(), handler);
   ASSERT_EQ(SDB_OK, rc);

   for (UINT32 i = 0; i < count; ++i)
   {
      utilInsertResult res;
      rc = handler.insert(&session, record,
                          ossRand() % 65535,
                          insertOptions(), &res);
      ASSERT_EQ(SDB_OK, rc);
   }
   handler.close();
}

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
   collectionHandler handler;
   UINT32 count = 4000000;
   static const UINT32 threadCount = 4;
   std::thread threads[threadCount];
   UINT32 countPerThread = count / threadCount;

   createCLOptions clOptions;

   closeDBOptions co;
   co.closeMode = closeDBOptions::CLOSE_MODE_IMMDIETE;

   rc = db.open(&session, &resource, options);
   ASSERT_EQ(SDB_OK, rc);

   rc = db.createCollectionSpace(&session, "foo", 1, createCSOptions());
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

   collectionHandler handler;
   UINT32 count = 4000000;
   static const UINT32 threadCount = 4;
   std::thread threads[threadCount];
   UINT32 countPerThread = count / threadCount;

   cursorHandler cursor;
   UINT64 recordCount = 0;
   dataScanRow recordRow;

   createCLOptions clOptions;
   clOptions.minFreePercent = 0;
   clOptions.minStriping = 0;
   clOptions.maxStriping = 65534;

   rc = db.open(&session, &resource, options);
   ASSERT_EQ(SDB_OK, rc);

   rc = db.createCollectionSpace(&session, "foo", 1, createCSOptions());
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

void thread_insert_index(vesselImpl *db,
                         const CHAR *csName, const CHAR *clName,
                         UINT32 count)
{
   test_executor session;
   CHAR pad[1024] = {0};
   bson::BSONObjBuilder builder;


   collectionHandler handler;
   INT32 rc = db->openCollection(&session, csName, clName, openCLOptions(), handler);
   ASSERT_EQ(SDB_OK, rc);

   for (UINT32 i = 0; i < count; ++i)
   {
      builder.reset();
      builder.append("a", ossRand());   /// just for benchmark.
      builder.append("b", 2);
      builder.append("c", pad, 1024);
      bson::BSONObj obj = builder.done();
      slice record;
      record.reset(obj.objsize(), obj.objdata());
      utilInsertResult res;
      rc = handler.insert(&session, record,
                          INVALID_STRIPING_ID,
                          insertOptions(), &res);
      ASSERT_EQ(SDB_OK, rc);
   }
   handler.close();
}

void thread_insert_unique_index(vesselImpl *db,
                         const CHAR *csName, const CHAR *clName,
                         UINT32 begin, UINT32 count)
{
   test_executor session;
   CHAR pad[1024] = {0};
   bson::BSONObjBuilder builder;
   collectionHandler handler;
   INT32 rc = db->openCollection(&session, csName, clName, openCLOptions(), handler);
   ASSERT_EQ(SDB_OK, rc);

   for (UINT32 i = 0; i < count; ++i)
   {
      builder.reset();
      builder.append("a", i + begin);
      builder.append("b", 2);
      builder.append("c", pad, 1024);
      bson::BSONObj obj = builder.done();
      slice record;
      record.reset(obj.objsize(), obj.objdata());
      utilInsertResult res;
      rc = handler.insert(&session, record,
                          INVALID_STRIPING_ID,
                          insertOptions(), &res);
      ASSERT_EQ(SDB_OK, rc);
   }
   handler.close();
}

/// insert with index
TEST_F(insert_test, test7)
{
   INT32 rc = SDB_OK;
   vesselImpl db;
   outerResource resource = test_outer_resource::getResource();
   test_executor session;
   openDBOptions options;

   options.path.dataPath = DATA_PATH;
   options.path.lsmPath = LSM_PATH;
   collectionHandler handler;
   UINT32 count = 10000;
   bson::BSONObjBuilder builder;
   static const UINT32 PAD_SIZE = 1024;
   CHAR pad[PAD_SIZE];

   createCSOptions csOptions;
   csOptions.dataSegSize = STORAGE_FILE_SEGMENT_SIZE_32MB;

   closeDBOptions co;

   indexParameters params;
   params.type = INDEX_TYPE_LSM;

   rc = db.open(&session, &resource, options);
   ASSERT_EQ(SDB_OK, rc);

   rc = db.createCollectionSpace(&session, "foo", 1, csOptions);
   ASSERT_EQ(SDB_OK, rc);

   rc = db.createCollection(&session, "foo", "bar1", 1, createCLOptions());
   ASSERT_EQ(SDB_OK, rc);

   rc = db.openCollection(&session, "foo", "bar1", openCLOptions(), handler);
   ASSERT_EQ(SDB_OK, rc);

   rc = handler.createIndex(&session, strSlice("index1"),
                              BSON("a" << 1), params, createIndexOptions());
   ASSERT_EQ(SDB_OK, rc);                   

   for (UINT32 i = 0; i < count; ++i)
   {
      utilInsertResult res;
      builder.reset();
      builder.append("a", i);
      builder.append("b", pad, PAD_SIZE);
      bson::BSONObj obj = builder.done();
      slice record(obj.objsize(), obj.objdata());
      rc = handler.insert(&session, record,
                          INVALID_STRIPING_ID,
                          insertOptions(), &res);
      ASSERT_EQ(SDB_OK, rc);
   }

   rc = db.close(&session, co);
   ASSERT_EQ(SDB_OK, rc);
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
   options.cacheOptions.flush.flushDirtyListThreshold = 0.8;
   options.cacheOptions.freelist.maxChunkCount = 1024;
   collectionHandler handler;
   UINT32 count = 6000000;
   static const UINT32 threadCount = 6;
   std::thread threads[threadCount];
   UINT32 countPerThread = count / threadCount;

   createCSOptions csOptions;
   csOptions.dataSegSize = STORAGE_FILE_SEGMENT_SIZE_32MB;

   closeDBOptions co;
   co.closeMode = closeDBOptions::CLOSE_MODE_IMMDIETE;

   collectionHandler clHandler;

   indexParameters params;
   params.type = type;

   rc = db.open(&session, &resource, options);
   ASSERT_EQ(SDB_OK, rc);

   rc = db.createCollectionSpace(&session, "foo", 1, csOptions);
   ASSERT_EQ(SDB_OK, rc);

   rc = db.createCollection(&session, "foo", "bar1", 1, createCLOptions());
   ASSERT_EQ(SDB_OK, rc);

   rc = db.openCollection(&session, "foo", "bar1", openCLOptions(), clHandler);
   ASSERT_EQ(SDB_OK, rc);

   rc = clHandler.createIndex(&session, strSlice("index1"),
                              BSON("a" << 1), params, createIndexOptions());
   ASSERT_EQ(SDB_OK, rc);                   

   for (UINT32 i = 0; i < threadCount; ++i)
   {
      threads[i] = std::move(std::thread(thread_insert_index, &db,
                                         "foo", "bar1", countPerThread));
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
   insert_test_nonunique_index(INDEX_TYPE_LSM);
}

TEST_F(insert_test, test8_1_2)
{
   insert_test_nonunique_index(INDEX_TYPE_BTREE);
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
   options.cacheOptions.flush.flushDirtyListThreshold = 0.8;
   options.cacheOptions.freelist.maxChunkCount = 1024;
   collectionHandler handler;
   UINT32 count = 6000000;
   static const UINT32 threadCount = 6;
   std::thread threads[threadCount];
   UINT32 countPerThread = count / threadCount;

   createCSOptions csOptions;
   csOptions.dataSegSize = STORAGE_FILE_SEGMENT_SIZE_32MB;

   closeDBOptions co;
   co.closeMode = closeDBOptions::CLOSE_MODE_IMMDIETE;

   collectionHandler clHandler;

   indexParameters params;
   params.type = type;
   params.isUnique = TRUE;

   rc = db.open(&session, &resource, options);
   ASSERT_EQ(SDB_OK, rc);

   rc = db.createCollectionSpace(&session, "foo", 1, csOptions);
   ASSERT_EQ(SDB_OK, rc);

   rc = db.createCollection(&session, "foo", "bar1", 1, createCLOptions());
   ASSERT_EQ(SDB_OK, rc);

   rc = db.openCollection(&session, "foo", "bar1", openCLOptions(), clHandler);
   ASSERT_EQ(SDB_OK, rc);

   rc = clHandler.createIndex(&session, strSlice("index1"),
                              BSON("a" << 1), params, createIndexOptions());
   ASSERT_EQ(SDB_OK, rc);                   

   for (UINT32 i = 0; i < threadCount; ++i)
   {
      threads[i] = std::move(std::thread(thread_insert_unique_index,
                                         &db,
                                         "foo", "bar1", i * countPerThread,
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
   insert_test_unique_index(INDEX_TYPE_LSM);
}

TEST_F(insert_test, test8_2_2)
{
   insert_test_unique_index(INDEX_TYPE_BTREE);
}