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

   Source File Name = index_scan_test.cpp

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
#include "vessel/ISession.h"
#include "vessel/requestContext.h"
#include <gtest/gtest.h>
#include "ossUtil.hpp"
#include "vessel/IRedoLogger.h"
#include "vessel/logRecordContext.h"
#include "dpsLogRecord.hpp"
#include "../bson/bson.hpp"
#include "pd.hpp"
#include "vessel/dataScanRow.h"
#include "vessel/collectionOptions.h"
#include "mthMatchTree.hpp"
#include <iostream>

#include <boost/filesystem.hpp>
namespace fs = boost::filesystem;

class index_scan_test : public testing::Test
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

static void thread_insert(vesselImpl *db, test_logger *logger,
                   const CHAR *csName, const CHAR *clName,
                   UINT32 count)
{
   test_session session(logger);
   CHAR pad[1024] = {0};
   bson::BSONObjBuilder builder;
   collectionHandler handler;
   INT32 rc = db->openCollection(&session, csName, clName, openCLOptions(), handler);
   ASSERT_EQ(SDB_OK, rc);

   for (UINT32 i = 0; i < count; ++i)
   {
      utilInsertResult res;
      builder.reset();
      builder.append("a", i);
      builder.append("b", 2);
      builder.append("c", pad, 1024);
      bson::BSONObj obj = builder.obj();
      slice record;
      record.reset(obj.objsize(), obj.objdata());     
      rc = handler.insert(&session, record, DPS_TRANS_ID(),
                          INVALID_STRIPING_ID,
                          insertOptions(), res);
      ASSERT_EQ(SDB_OK, rc);
   }
   handler.close();
}

void test1(INDEX_TYPE type)
{
   INT32 rc = SDB_OK;
   vesselImpl db;
   outerResource resource= test_outer_resource::getResource();
   resource.logger = test_logger::instance();
   resource.sessionMgr = test_session_mgr::instance(); 
   test_session session(test_logger::instance());
   openDBOptions options;
   options.ioWorkerCount = 4;
   options.path.dataPath = DATA_PATH;
   options.path.lsmPath = LSM_PATH;

   collectionHandler handler;
   UINT32 count = 1000;

   indexParameters params;
   params.type = type;

   bson::BSONObj pattern = BSON("a" << 1);
   strSlice indexName("index1");

   rc = db.open(&session, &resource, options);
   ASSERT_EQ(SDB_OK, rc);

   rc = db.createCollectionSpace(&session, "foo", 1, createCSOptions());
   ASSERT_EQ(SDB_OK, rc);

   rc = db.createCollection(&session, "foo", "bar1", 1, createCLOptions());
   ASSERT_EQ(SDB_OK, rc);

   rc = db.openCollection(&session, "foo", "bar1", openCLOptions(), handler);
   ASSERT_EQ(SDB_OK, rc);

   rc = handler.createIndex(&session, indexName,
                              pattern, params, createIndexOptions());
   ASSERT_EQ(SDB_OK, rc);

   bson::BSONObjBuilder builder;
   for (UINT32 i = 0; i < count; ++i)
   {
      utilInsertResult res;
      builder.reset();
      builder.append("a", i);
      builder.append("b", i+1);
      bson::BSONObj obj = builder.done();
      slice record(obj.objsize(), obj.objdata());
      rc = handler.insert(&session, record, DPS_TRANS_ID(),
                          INVALID_STRIPING_ID,
                          insertOptions(), res);
      ASSERT_EQ(SDB_OK, rc);
   }


   mthMatchTree mt;
   indexScanOptions o;
   o.indexCoverd = FALSE;

   for (UINT32 i = 0; i < count; ++i)
   {
      builder.reset();
      builder.append("a", i);
      rc = mt.loadPattern(builder.done(), FALSE);
      ASSERT_EQ(SDB_OK, rc);
      rtnPredicateSet ps;
      rc = mt.calcPredicate(ps, NULL);
      ASSERT_EQ(SDB_OK, rc);
      rtnPredicateList predicates;
      UINT32 lvl = 0;
      rc = predicates.initialize(ps, pattern, 1, lvl);
      ASSERT_EQ(SDB_OK, rc);
      cursorHandler cursor;
      rc = handler.openIndexScanCursor(&session, indexName,
                                       predicates, o, cursor);
      ASSERT_EQ(SDB_OK, rc);

      dataScanRow row;
      rc = cursor.getNextRow(&session, row);
      ASSERT_EQ(SDB_OK, rc);
      bson::BSONObj recordObj(row.getRecord().data());
      ASSERT_EQ(i, recordObj.getIntField("a"));

      rc = cursor.getNextRow(&session, row);
      ASSERT_EQ(SDB_VESSEL_EOC, rc);
      mt.clear();
   }

   rc = db.close(&session, closeDBOptions());
   ASSERT_EQ(SDB_OK, rc);

   rc = db.open(&session, &resource, options);
   ASSERT_EQ(SDB_OK, rc);

   rc = db.openCollection(&session, "foo", "bar1", openCLOptions(), handler);
   ASSERT_EQ(SDB_OK, rc);

   for (UINT32 i = 0; i < count; ++i)
   {
      builder.reset();
      builder.append("a", i);
      rc = mt.loadPattern(builder.done(), FALSE);
      ASSERT_EQ(SDB_OK, rc);
      rtnPredicateSet ps;
      rc = mt.calcPredicate(ps, NULL);
      ASSERT_EQ(SDB_OK, rc);
      rtnPredicateList predicates;
      UINT32 lvl = 0;
      rc = predicates.initialize(ps, pattern, 1, lvl);
      ASSERT_EQ(SDB_OK, rc);
      cursorHandler cursor;
      rc = handler.openIndexScanCursor(&session, indexName,
                                       predicates, o, cursor);
      ASSERT_EQ(SDB_OK, rc);

      dataScanRow row;
      rc = cursor.getNextRow(&session, row);
      ASSERT_EQ(SDB_OK, rc);
      bson::BSONObj recordObj(row.getRecord().data());
      ASSERT_EQ(i, recordObj.getIntField("a"));

      rc = cursor.getNextRow(&session, row);
      ASSERT_EQ(SDB_VESSEL_EOC, rc);
      mt.clear(); 
   }

   rc = db.close(&session, closeDBOptions());
   ASSERT_EQ(SDB_OK, rc);
}

TEST_F(index_scan_test, test1_1)
{
   test1(INDEX_TYPE_LSM);
}

TEST_F(index_scan_test, test1_2)
{
   test1(INDEX_TYPE_BTREE);
}

void test2(INDEX_TYPE type)
{
   INT32 rc = SDB_OK;
   vesselImpl db;
   outerResource resource= test_outer_resource::getResource();
   resource.logger = test_logger::instance();
   resource.sessionMgr = test_session_mgr::instance(); 
   test_session session(test_logger::instance());
   openDBOptions options;
   options.ioWorkerCount = 4;
   options.path.dataPath = DATA_PATH;
   options.path.lsmPath = LSM_PATH;

   collectionHandler handler;
   UINT32 count = 1000000;

   indexParameters params;
   params.type = type;

   bson::BSONObj pattern = BSON("a" << 1);
   strSlice indexName("index1");

   rc = db.open(&session, &resource, options);
   ASSERT_EQ(SDB_OK, rc);

   rc = db.createCollectionSpace(&session, "foo", 1, createCSOptions());
   ASSERT_EQ(SDB_OK, rc);

   rc = db.createCollection(&session, "foo", "bar1", 1, createCLOptions());
   ASSERT_EQ(SDB_OK, rc);

   rc = db.openCollection(&session, "foo", "bar1", openCLOptions(), handler);
   ASSERT_EQ(SDB_OK, rc);

   bson::BSONObjBuilder builder;
   for (UINT32 i = 0; i < count; ++i)
   {
      utilInsertResult res;
      builder.reset();
      builder.append("a", i);
      builder.append("b", i+1);
      bson::BSONObj obj = builder.done();
      slice record(obj.objsize(), obj.objdata());
      rc = handler.insert(&session, record, DPS_TRANS_ID(),
                          INVALID_STRIPING_ID,
                          insertOptions(), res);
      ASSERT_EQ(SDB_OK, rc);
   }

   rc = handler.createIndex(&session, indexName,
                              pattern, params, createIndexOptions());
   ASSERT_EQ(SDB_OK, rc);


   mthMatchTree mt;
   indexScanOptions o;
   o.indexCoverd = FALSE;

   UINT64 begin = ossGetCurrentMilliseconds();
   for (UINT32 i = 0; i < count; ++i)
   {
      builder.reset();
      builder.append("a", i);
      rc = mt.loadPattern(builder.done(), FALSE);
      ASSERT_EQ(SDB_OK, rc);
      rtnPredicateSet ps;
      rc = mt.calcPredicate(ps, NULL);
      ASSERT_EQ(SDB_OK, rc);
      rtnPredicateList predicates;
      UINT32 lvl = 0;
      rc = predicates.initialize(ps, pattern, 1, lvl);
      ASSERT_EQ(SDB_OK, rc);
      cursorHandler cursor;
      rc = handler.openIndexScanCursor(&session, indexName,
                                       predicates, o, cursor);
      ASSERT_EQ(SDB_OK, rc);

      dataScanRow row;
      rc = cursor.getNextRow(&session, row);
      ASSERT_EQ(SDB_OK, rc);
      bson::BSONObj recordObj(row.getRecord().data());
      ASSERT_EQ(i, recordObj.getIntField("a"));
      rc = cursor.getNextRow(&session, row);
      ASSERT_EQ(SDB_VESSEL_EOC, rc);
      cursor.close();
      mt.clear();
   }
   UINT64 end = ossGetCurrentMilliseconds();
   std::cout << "time cost:" << (end - begin) << std::endl;

   rc = db.close(&session, closeDBOptions());
   ASSERT_EQ(SDB_OK, rc);
}

TEST_F(index_scan_test, test2_1)
{
   test2(INDEX_TYPE_LSM);
}

TEST_F(index_scan_test, test2_2)
{
   test2(INDEX_TYPE_BTREE);
}

void test3(INDEX_TYPE type)
{
   INT32 rc = SDB_OK;
   vesselImpl db;
   outerResource resource= test_outer_resource::getResource();
   resource.logger = test_logger::instance();
   resource.sessionMgr = test_session_mgr::instance(); 
   test_session session(test_logger::instance());
   openDBOptions options;
   options.ioWorkerCount = 4;
   options.path.dataPath = DATA_PATH;
   options.path.lsmPath = LSM_PATH;

   collectionHandler handler;
   UINT32 count = 100000;

   indexParameters params;
   params.type = type;

   bson::BSONObj pattern = BSON("a" << 1);
   strSlice indexName("index1");

   rc = db.open(&session, &resource, options);
   ASSERT_EQ(SDB_OK, rc);

   rc = db.createCollectionSpace(&session, "foo", 1, createCSOptions());
   ASSERT_EQ(SDB_OK, rc);

   rc = db.createCollection(&session, "foo", "bar1", 1, createCLOptions());
   ASSERT_EQ(SDB_OK, rc);

   rc = db.openCollection(&session, "foo", "bar1", openCLOptions(), handler);
   ASSERT_EQ(SDB_OK, rc);

   rc = handler.createIndex(&session, indexName,
                              pattern, params, createIndexOptions());
   ASSERT_EQ(SDB_OK, rc);

   bson::BSONObjBuilder builder;
   for (UINT32 i = 0; i < count; ++i)
   {
      utilInsertResult res;
      builder.reset();
      builder.append("a", i);
      builder.append("b", i+1);
      bson::BSONObj obj = builder.done();
      slice record(obj.objsize(), obj.objdata());
      rc = handler.insert(&session, record, DPS_TRANS_ID(),
                          INVALID_STRIPING_ID,
                          insertOptions(), res);
      ASSERT_EQ(SDB_OK, rc);
   }


   mthMatchTree mt;
   indexScanOptions o;
   o.indexCoverd = FALSE;

   /// {"a":{"$gte":i, "$lt":i + 2}}
   for (UINT32 i = 0; i < count; ++i)
   {
      builder.reset();
      bson::BSONObjBuilder subBuilder(builder.subobjStart("a"));
      subBuilder.append("$gte", i);
      subBuilder.append("$lt", i + 2);
      subBuilder.done();
      bson::BSONObj query = builder.done();

      rc = mt.loadPattern(query, FALSE);
      ASSERT_EQ(SDB_OK, rc);
      rtnPredicateSet ps;
      rc = mt.calcPredicate(ps, NULL);
      ASSERT_EQ(SDB_OK, rc);
      rtnPredicateList predicates;
      UINT32 lvl = 0;
      rc = predicates.initialize(ps, pattern, 1, lvl);
      ASSERT_EQ(SDB_OK, rc);
      cursorHandler cursor;
      rc = handler.openIndexScanCursor(&session, indexName,
                                       predicates, o, cursor);
      ASSERT_EQ(SDB_OK, rc);

      dataScanRow row;
      rc = cursor.getNextRow(&session, row);
      ASSERT_EQ(SDB_OK, rc);
      bson::BSONObj recordObj(row.getRecord().data());
      ASSERT_EQ(i, recordObj.getIntField("a"));

      if ((i + 1) < count)
      {
         rc = cursor.getNextRow(&session, row);
         ASSERT_EQ(SDB_OK, rc);
         recordObj = bson::BSONObj(row.getRecord().data());
         ASSERT_EQ(i + 1, recordObj.getIntField("a"));
      }

      rc = cursor.getNextRow(&session, row);
      ASSERT_EQ(SDB_VESSEL_EOC, rc);
      mt.clear();
   }

   rc = db.close(&session, closeDBOptions());
   ASSERT_EQ(SDB_OK, rc);
}

TEST_F(index_scan_test, test3_1)
{
   test3(INDEX_TYPE_LSM);
}

TEST_F(index_scan_test, test3_2)
{
   test3(INDEX_TYPE_BTREE);
}

void test4(INDEX_TYPE type)
{
   INT32 rc = SDB_OK;
   vesselImpl db;
   outerResource resource= test_outer_resource::getResource();
   resource.logger = test_logger::instance();
   resource.sessionMgr = test_session_mgr::instance(); 
   test_session session(test_logger::instance());
   openDBOptions options;
   options.ioWorkerCount = 4;
   options.path.dataPath = DATA_PATH;
   options.path.lsmPath = LSM_PATH;

   collectionHandler handler;
   UINT32 count = 100000;

   indexParameters params;
   params.type = type;

   bson::BSONObj pattern = BSON("a" << 1);
   strSlice indexName("index1");

   rc = db.open(&session, &resource, options);
   ASSERT_EQ(SDB_OK, rc);

   rc = db.createCollectionSpace(&session, "foo", 1, createCSOptions());
   ASSERT_EQ(SDB_OK, rc);

   rc = db.createCollection(&session, "foo", "bar1", 1, createCLOptions());
   ASSERT_EQ(SDB_OK, rc);

   rc = db.openCollection(&session, "foo", "bar1", openCLOptions(), handler);
   ASSERT_EQ(SDB_OK, rc);

   rc = handler.createIndex(&session, indexName,
                              pattern, params, createIndexOptions());
   ASSERT_EQ(SDB_OK, rc);

   bson::BSONObjBuilder builder;
   for (UINT32 i = 0; i < count; ++i)
   {
      utilInsertResult res;
      builder.reset();
      builder.append("a", i);
      builder.append("b", i+1);
      bson::BSONObj obj = builder.done();
      slice record(obj.objsize(), obj.objdata());
      rc = handler.insert(&session, record, DPS_TRANS_ID(),
                          INVALID_STRIPING_ID,
                          insertOptions(), res);
      ASSERT_EQ(SDB_OK, rc);
   }


   mthMatchTree mt;
   indexScanOptions o;
   o.indexCoverd = FALSE;
   o.forward = FALSE;

   {
      builder.reset();
      bson::BSONObjBuilder subBuilder(builder.subobjStart("a"));
      subBuilder.append("$lt", count);
      subBuilder.done();
      bson::BSONObj query = builder.done();

      rc = mt.loadPattern(query, FALSE);
      ASSERT_EQ(SDB_OK, rc);
      rtnPredicateSet ps;
      rc = mt.calcPredicate(ps, NULL);
      ASSERT_EQ(SDB_OK, rc);
      rtnPredicateList predicates;
      UINT32 lvl = 0;
      rc = predicates.initialize(ps, pattern, -1, lvl);
      ASSERT_EQ(SDB_OK, rc);
      cursorHandler cursor;
      rc = handler.openIndexScanCursor(&session, indexName,
                                       predicates, o, cursor);
      ASSERT_EQ(SDB_OK, rc);

      dataScanRow row;
      for (INT32 i = count - 1; i >= 0; --i)
      {
         dataScanRow row;
         rc = cursor.getNextRow(&session, row);
         ASSERT_EQ(SDB_OK, rc);
         bson::BSONObj recordObj(row.getRecord().data());
         ASSERT_EQ(i, recordObj.getIntField("a"));
      }

      rc = cursor.getNextRow(&session, row);
      ASSERT_EQ(SDB_VESSEL_EOC, rc);
      mt.clear();
   }

   rc = db.close(&session, closeDBOptions());
   ASSERT_EQ(SDB_OK, rc);
}

///backword scan
TEST_F(index_scan_test, test4_1)
{
   test4(INDEX_TYPE_LSM);
}

TEST_F(index_scan_test, test4_2)
{
   test4(INDEX_TYPE_BTREE);
}

void test5(INDEX_TYPE type)
{
   INT32 rc = SDB_OK;
   vesselImpl db;
   outerResource resource= test_outer_resource::getResource();
   resource.logger = test_logger::instance();
   resource.sessionMgr = test_session_mgr::instance(); 
   test_session session(test_logger::instance());
   openDBOptions options;
   options.ioWorkerCount = 4;
   options.path.dataPath = DATA_PATH;
   options.path.lsmPath = LSM_PATH;

   collectionHandler handler;
   UINT32 count = 100000;

   indexParameters params;
   params.type = type;

   bson::BSONObj pattern = BSON("a" << 1);
   strSlice indexName("index1");

   rc = db.open(&session, &resource, options);
   ASSERT_EQ(SDB_OK, rc);

   rc = db.createCollectionSpace(&session, "foo", 1, createCSOptions());
   ASSERT_EQ(SDB_OK, rc);

   rc = db.createCollection(&session, "foo", "bar1", 1, createCLOptions());
   ASSERT_EQ(SDB_OK, rc);

   rc = db.openCollection(&session, "foo", "bar1", openCLOptions(), handler);
   ASSERT_EQ(SDB_OK, rc);

   rc = handler.createIndex(&session, indexName,
                              pattern, params, createIndexOptions());
   ASSERT_EQ(SDB_OK, rc);

   bson::BSONObjBuilder builder;
   for (UINT32 i = 0; i < count; ++i)
   {
      utilInsertResult res;
      builder.reset();
      builder.append("a", i);
      builder.append("b", i+1);
      bson::BSONObj obj = builder.done();
      slice record(obj.objsize(), obj.objdata());
      rc = handler.insert(&session, record, DPS_TRANS_ID(),
                          INVALID_STRIPING_ID,
                          insertOptions(), res);
      ASSERT_EQ(SDB_OK, rc);
   }


   mthMatchTree mt;
   indexScanOptions o;
   o.indexCoverd = TRUE;

   for (UINT32 i = 0; i < count; ++i)
   {
      builder.reset();
      builder.append("a", i);
      rc = mt.loadPattern(builder.done(), FALSE);
      ASSERT_EQ(SDB_OK, rc);
      rtnPredicateSet ps;
      rc = mt.calcPredicate(ps, NULL);
      ASSERT_EQ(SDB_OK, rc);
      rtnPredicateList predicates;
      UINT32 lvl = 0;
      rc = predicates.initialize(ps, pattern, 1, lvl);
      ASSERT_EQ(SDB_OK, rc);
      cursorHandler cursor;
      rc = handler.openIndexScanCursor(&session, indexName,
                                       predicates, o, cursor);
      ASSERT_EQ(SDB_OK, rc);

      dataScanRow row;
      rc = cursor.getNextRow(&session, row);
      ASSERT_EQ(SDB_OK, rc);
      bson::BSONObj recordObj(row.getRecord().data());
      ASSERT_EQ(i, recordObj.getIntField("a"));

      rc = cursor.getNextRow(&session, row);
      ASSERT_EQ(SDB_VESSEL_EOC, rc);
      mt.clear();
   }

   rc = db.close(&session, closeDBOptions());
   ASSERT_EQ(SDB_OK, rc);
}

/// index covered
TEST_F(index_scan_test, test5_1)
{
   test5(INDEX_TYPE_LSM);
}

TEST_F(index_scan_test, test5_2)
{
   test5(INDEX_TYPE_BTREE);
}