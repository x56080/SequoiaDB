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
#include "vessel/requestContext.h"
#include <gtest/gtest.h>
#include "ossUtil.hpp"
#include "vessel/IRedoLogger.h"
#include "vessel/logRecordContext.h"
#include "dpsLogRecord.hpp"
#include "../bson/bson.hpp"
#include "pd.hpp"
#include "vessel/collectionOptions.h"
#include "mthMatchTree.hpp"
#include <iostream>
#include "dmsCursorReader.hpp"

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

static void thread_insert(vesselImpl *db,
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
      utilInsertResult res;
      builder.reset();
      builder.append("a", i);
      builder.append("b", 2);
      builder.append("c", pad, 1024);
      bson::BSONObj obj = builder.obj();
    
      rc = handler->insertRecord(&session, obj, dmsInsertRecordOptions(), &res);
      ASSERT_EQ(SDB_OK, rc);
   }
   handler->close();
}

void test1(INDEX_TYPE type)
{
   INT32 rc = SDB_OK;
   vesselImpl db;
   outerResource resource = test_outer_resource::getResource();
   test_executor session;
   openDBOptions options;

   options.path.dataPath = DATA_PATH;
   options.path.lsmPath = LSM_PATH;

   DATA_COLLECTION_PTR handler;
   UINT32 count = 1000;

   indexParameters params;
   params.type = type;

   bson::BSONObj pattern = BSON("a" << 1);
   const CHAR *indexName = "index";

   bson::BSONObj indexDef = indexTestUtil::createIndexObj(indexName, params, pattern);

   rc = db.open(&session, &resource, options);
   ASSERT_EQ(SDB_OK, rc);

   rc = db.createCS(&session, "foo", 1, dmsCreateCSOptions(), bson::BSONObj());
   ASSERT_EQ(SDB_OK, rc);

   rc = db.createCL(&session, "foo.bar", 1, dmsCreateCLOptions(), bson::BSONObj());
   ASSERT_EQ(SDB_OK, rc);

   rc = db.openCL(&session, "foo.bar", dmsOpenCLOptions(), handler);
   ASSERT_EQ(SDB_OK, rc);

   rc = handler->createIndex(&session, dmsBuildIndexOptions(), indexDef);
   ASSERT_EQ(SDB_OK, rc);

   bson::BSONObjBuilder builder;
   for (UINT32 i = 0; i < count; ++i)
   {
      utilInsertResult res;
      builder.reset();
      builder.append("a", i);
      builder.append("b", i+1);
      bson::BSONObj obj = builder.done();

      rc = handler->insertRecord(&session, obj, dmsInsertRecordOptions(), &res);
      ASSERT_EQ(SDB_OK, rc);
   }


   mthMatchTree mt;
   dmsIndexScanOptions o;
   
   for (UINT32 i = 0; i < count; ++i)
   {
      dmsBsonCursorReader reader;
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
      DATA_CURSOR_PTR cursor;
      rc = handler->scanIndex(&session, indexName, predicates, o, cursor);
      ASSERT_EQ(SDB_OK, rc);

      reader.init(cursor, FALSE);
      rc = reader.fetchNext(&session);
      ASSERT_EQ(SDB_OK, rc);
      ASSERT_EQ(i, reader.getRecord().getIntField("a"));

      rc = reader.fetchNext(&session);
      ASSERT_EQ(SDB_DMS_EOC, rc);
      mt.clear();
   }

   rc = db.close(&session, closeDBOptions());
   ASSERT_EQ(SDB_OK, rc);

   rc = db.open(&session, &resource, options);
   ASSERT_EQ(SDB_OK, rc);

   rc = db.openCL(&session, "foo.bar", dmsOpenCLOptions(), handler);
   ASSERT_EQ(SDB_OK, rc);

   for (UINT32 i = 0; i < count; ++i)
   {
      dmsBsonCursorReader reader;
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
      DATA_CURSOR_PTR cursor;
      rc = handler->scanIndex(&session, indexName, predicates, o, cursor);
      ASSERT_EQ(SDB_OK, rc);

      reader.init(cursor, FALSE);
      rc = reader.fetchNext(&session);
      ASSERT_EQ(SDB_OK, rc);
      ASSERT_EQ(i, reader.getRecord().getIntField("a"));

      rc = reader.fetchNext(&session);
      ASSERT_EQ(SDB_DMS_EOC, rc);
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
   outerResource resource = test_outer_resource::getResource();
   test_executor session;
   openDBOptions options;

   options.path.dataPath = DATA_PATH;
   options.path.lsmPath = LSM_PATH;

   DATA_COLLECTION_PTR handler;
   UINT32 count = 1000000;

   indexParameters params;
   params.type = type;

   bson::BSONObj pattern = BSON("a" << 1);
   const CHAR *indexName = "index1";

   bson::BSONObj indexDef = indexTestUtil::createIndexObj(indexName, params, pattern);

   rc = db.open(&session, &resource, options);
   ASSERT_EQ(SDB_OK, rc);

   rc = db.createCS(&session, "foo", 1, dmsCreateCSOptions(), bson::BSONObj());
   ASSERT_EQ(SDB_OK, rc);

   rc = db.createCL(&session, "foo.bar", 1, dmsCreateCLOptions(), bson::BSONObj());
   ASSERT_EQ(SDB_OK, rc);

   rc = db.openCL(&session, "foo.bar", dmsOpenCLOptions(), handler);
   ASSERT_EQ(SDB_OK, rc);

   bson::BSONObjBuilder builder;
   for (UINT32 i = 0; i < count; ++i)
   {
      utilInsertResult res;
      builder.reset();
      builder.append("a", i);
      builder.append("b", i+1);
      bson::BSONObj obj = builder.done();

      rc = handler->insertRecord(&session, obj, dmsInsertRecordOptions(), &res);
      ASSERT_EQ(SDB_OK, rc);
   }

   rc = handler->createIndex(&session, dmsBuildIndexOptions(), indexDef);
   ASSERT_EQ(SDB_OK, rc);


   mthMatchTree mt;
   dmsIndexScanOptions o;

   UINT64 begin = ossGetCurrentMilliseconds();
   for (UINT32 i = 0; i < count; ++i)
   {
      dmsBsonCursorReader reader;
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
      DATA_CURSOR_PTR cursor;
      rc = handler->scanIndex(&session, indexName,
                              predicates, o, cursor);
      ASSERT_EQ(SDB_OK, rc);

      reader.init(cursor, FALSE);
      rc = reader.fetchNext(&session);
      ASSERT_EQ(SDB_OK, rc);
      ASSERT_EQ(i, reader.getRecord().getIntField("a"));
      rc = reader.fetchNext(&session);
      ASSERT_EQ(SDB_DMS_EOC, rc);
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
   outerResource resource = test_outer_resource::getResource();
   test_executor session;
   openDBOptions options;

   options.path.dataPath = DATA_PATH;
   options.path.lsmPath = LSM_PATH;

   DATA_COLLECTION_PTR handler;
   UINT32 count = 100000;

   indexParameters params;
   params.type = type;

   bson::BSONObj pattern = BSON("a" << 1);
   const CHAR *indexName = "index1";
   bson::BSONObj indexDef = indexTestUtil::createIndexObj(indexName, params, pattern);

   rc = db.open(&session, &resource, options);
   ASSERT_EQ(SDB_OK, rc);

   rc = db.createCS(&session, "foo", 1, dmsCreateCSOptions(), bson::BSONObj());
   ASSERT_EQ(SDB_OK, rc);

   rc = db.createCL(&session, "foo.bar", 1, dmsCreateCLOptions(), bson::BSONObj());
   ASSERT_EQ(SDB_OK, rc);

   rc = db.openCL(&session, "foo.bar", dmsOpenCLOptions(), handler);
   ASSERT_EQ(SDB_OK, rc);

   rc = handler->createIndex(&session, dmsBuildIndexOptions(), indexDef);
   ASSERT_EQ(SDB_OK, rc);

   bson::BSONObjBuilder builder;
   for (UINT32 i = 0; i < count; ++i)
   {
      utilInsertResult res;
      builder.reset();
      builder.append("a", i);
      builder.append("b", i+1);
      bson::BSONObj obj = builder.done();
      rc = handler->insertRecord(&session, obj, dmsInsertRecordOptions(), &res);
      ASSERT_EQ(SDB_OK, rc);
   }


   mthMatchTree mt;
   dmsIndexScanOptions o;

   /// {"a":{"$gte":i, "$lt":i + 2}}
   for (UINT32 i = 0; i < count; ++i)
   {
      dmsBsonCursorReader reader;
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
      DATA_CURSOR_PTR cursor;
      rc = handler->scanIndex(&session, indexName,
                              predicates, o, cursor);
      ASSERT_EQ(SDB_OK, rc);

      reader.init(cursor, FALSE);
      rc = reader.fetchNext(&session);
      ASSERT_EQ(SDB_OK, rc);
      ASSERT_EQ(i, reader.getRecord().getIntField("a"));

      if ((i + 1) < count)
      {
         rc = reader.fetchNext(&session);
         ASSERT_EQ(SDB_OK, rc);
         ASSERT_EQ(i + 1, reader.getRecord().getIntField("a"));
      }

      rc = reader.fetchNext(&session);
      ASSERT_EQ(SDB_DMS_EOC, rc);
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
   outerResource resource = test_outer_resource::getResource();
   test_executor session;
   openDBOptions options;

   options.path.dataPath = DATA_PATH;
   options.path.lsmPath = LSM_PATH;

   DATA_COLLECTION_PTR handler;
   UINT32 count = 100000;

   indexParameters params;
   params.type = type;

   bson::BSONObj pattern = BSON("a" << 1);
   const CHAR *indexName = "index1";
   bson::BSONObj indexDef = indexTestUtil::createIndexObj(indexName, params, pattern);

   rc = db.open(&session, &resource, options);
   ASSERT_EQ(SDB_OK, rc);

   rc = db.createCS(&session, "foo", 1, dmsCreateCSOptions(), bson::BSONObj());
   ASSERT_EQ(SDB_OK, rc);

   rc = db.createCL(&session, "foo.bar", 1, dmsCreateCLOptions(), bson::BSONObj());
   ASSERT_EQ(SDB_OK, rc);

   rc = db.openCL(&session, "foo.bar", dmsOpenCLOptions(), handler);
   ASSERT_EQ(SDB_OK, rc);

   rc = handler->createIndex(&session, dmsBuildIndexOptions(), indexDef);
   ASSERT_EQ(SDB_OK, rc);

   bson::BSONObjBuilder builder;
   for (UINT32 i = 0; i < count; ++i)
   {
      utilInsertResult res;
      builder.reset();
      builder.append("a", i);
      builder.append("b", i+1);
      bson::BSONObj obj = builder.done();
      rc = handler->insertRecord(&session, obj, dmsInsertRecordOptions(), &res);
      ASSERT_EQ(SDB_OK, rc);
   }


   mthMatchTree mt;
   dmsIndexScanOptions o;
   o.indexCovered = FALSE;
   o.forward = FALSE;

   {
      dmsBsonCursorReader reader;
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
      DATA_CURSOR_PTR cursor;
      rc = handler->scanIndex(&session, indexName,
                              predicates, o, cursor);
      ASSERT_EQ(SDB_OK, rc);

      reader.init(cursor, FALSE);
      for (INT32 i = count - 1; i >= 0; --i)
      {
         rc = reader.fetchNext(&session);
         ASSERT_EQ(SDB_OK, rc);
         ASSERT_EQ(i, reader.getRecord().getIntField("a"));
      }

      rc = reader.fetchNext(&session);
      ASSERT_EQ(SDB_DMS_EOC, rc);
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
   outerResource resource = test_outer_resource::getResource();
   test_executor session;
   openDBOptions options;

   options.path.dataPath = DATA_PATH;
   options.path.lsmPath = LSM_PATH;

   DATA_COLLECTION_PTR handler;
   UINT32 count = 100000;

   indexParameters params;
   params.type = type;

   bson::BSONObj pattern = BSON("a" << 1);
   const CHAR *indexName = "index1";
   bson::BSONObj indexDef = indexTestUtil::createIndexObj(indexName, params, pattern);

   rc = db.open(&session, &resource, options);
   ASSERT_EQ(SDB_OK, rc);

   rc = db.createCS(&session, "foo", 1, dmsCreateCSOptions(), bson::BSONObj());
   ASSERT_EQ(SDB_OK, rc);

   rc = db.createCL(&session, "foo.bar", 1, dmsCreateCLOptions(), bson::BSONObj());
   ASSERT_EQ(SDB_OK, rc);

   rc = db.openCL(&session, "foo.bar", dmsOpenCLOptions(), handler);
   ASSERT_EQ(SDB_OK, rc);

   rc = handler->createIndex(&session, dmsBuildIndexOptions(), indexDef);
   ASSERT_EQ(SDB_OK, rc);

   bson::BSONObjBuilder builder;
   for (UINT32 i = 0; i < count; ++i)
   {
      utilInsertResult res;
      builder.reset();
      builder.append("a", i);
      builder.append("b", i+1);
      bson::BSONObj obj = builder.done();
      rc = handler->insertRecord(&session, obj, dmsInsertRecordOptions(), &res);
      ASSERT_EQ(SDB_OK, rc);
   }


   mthMatchTree mt;
   dmsIndexScanOptions o;
   o.indexCovered = TRUE;

   for (UINT32 i = 0; i < count; ++i)
   {
      dmsBsonCursorReader reader;
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
      DATA_CURSOR_PTR cursor;

      rc = handler->scanIndex(&session, indexName,
                              predicates, o, cursor);
      ASSERT_EQ(SDB_OK, rc);

      reader.init(cursor, FALSE);
      rc = reader.fetchNext(&session);
      ASSERT_EQ(SDB_OK, rc);
      ASSERT_EQ(i, reader.getRecord().getIntField("a"));

      rc = reader.fetchNext(&session);
      ASSERT_EQ(SDB_DMS_EOC, rc);
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

void multi_index_scan_test(INDEX_TYPE type)
{
   INT32 rc = SDB_OK;
   vesselImpl db;
   outerResource resource = test_outer_resource::getResource();
   test_executor session;
   openDBOptions options;

   options.path.dataPath = DATA_PATH;
   options.path.lsmPath = LSM_PATH;

   DATA_COLLECTION_PTR handler;
   INT32 count = 100;

   constexpr INT32 indexCount = 26;

   indexParameters params;
   params.type = type;

   rc = db.open(&session, &resource, options);
   ASSERT_EQ(SDB_OK, rc);

   rc = db.createCS(&session, "foo", 1, dmsCreateCSOptions(), bson::BSONObj());
   ASSERT_EQ(SDB_OK, rc);

   rc = db.createCL(&session, "foo.bar", 1, dmsCreateCLOptions(), bson::BSONObj());
   ASSERT_EQ(SDB_OK, rc);

   rc = db.openCL(&session, "foo.bar", dmsOpenCLOptions(), handler);
   ASSERT_EQ(SDB_OK, rc);

   for (INT32 i = 0; i < indexCount; ++i)
   {
      CHAR indexName[2] = {};
      indexName[0] = 'a' + i;
      bson::BSONObj pattern = BSON(indexName << 1);
      bson::BSONObj indexDef = indexTestUtil::createIndexObj(indexName, params, pattern);
      rc = handler->createIndex(&session, dmsBuildIndexOptions(), indexDef);
      ASSERT_EQ(SDB_OK, rc);
   }

   bson::BSONObjBuilder builder;
   for (INT32 i = 0; i < count; ++i)
   {
      for (INT32 j = 0; j < indexCount; ++j)
      {
         CHAR fieldName[2] = {};
         fieldName[0] = 'a' + j;
         builder.append(fieldName, i);
      }

      rc = handler->insertRecord(&session, builder.done(), dmsInsertRecordOptions(), NULL);
      ASSERT_EQ(SDB_OK, rc);
      builder.reset();
   }

   for (INT32 i = 0; i < indexCount; ++i)
   {
      CHAR fieldName[2] = {};
      fieldName[0] = 'a' + i;
      bson::BSONObj match = BSON(fieldName << BSON("$gte" << 0));
      mthMatchTree mt;
      rc = mt.loadPattern(match, FALSE);
      ASSERT_EQ(SDB_OK, rc);
      rtnPredicateSet ps;
      rc = mt.calcPredicate(ps, NULL);
      ASSERT_EQ(SDB_OK, rc);
      rtnPredicateList predicates;
      UINT32 lvl = 0;
      bson::BSONObj pattern = BSON(fieldName << 1);
      rc = predicates.initialize(ps, pattern, 1, lvl);
      ASSERT_EQ(SDB_OK, rc);
      DATA_CURSOR_PTR cursor;
      rc = handler->scanIndex(&session, fieldName, predicates,
                              dmsIndexScanOptions(), cursor);
      ASSERT_EQ(SDB_OK, rc);
      dmsBsonCursorReader reader;
      reader.init(cursor, FALSE);
      for (INT32 j = 0; j < count; ++j)
      {
         rc = reader.fetchNext(&session);
         ASSERT_EQ(SDB_OK, rc);
         ASSERT_EQ(j, reader.getRecord().getIntField(fieldName));
      }

      rc = reader.fetchNext(&session);
      ASSERT_EQ(SDB_DMS_EOC, rc);
   }

   handler->close();
   rc = db.close(&session, closeDBOptions());
   ASSERT_EQ(SDB_OK, rc);
}

TEST_F(index_scan_test, test6_1)
{
   multi_index_scan_test(INDEX_TYPE_BTREE);
}

TEST_F(index_scan_test, test6_2)
{
   multi_index_scan_test(INDEX_TYPE_LSM);
}