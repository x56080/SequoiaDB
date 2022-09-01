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

   Source File Name = hit_index_test.cpp

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
#include "dpsLogRecord.hpp"
#include "../bson/bson.hpp"
#include "pd.hpp"
#include "vessel/collectionOptions.h"
#include "mthMatchTree.hpp"
#include <iostream>

#include <boost/filesystem.hpp>
#include <random>
namespace fs = boost::filesystem;

class hit_index_test : public testing::Test
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

TEST_F(hit_index_test, base_test_1)
{
   INT32 rc = SDB_OK;
   vesselImpl db;
   outerResource resource = test_outer_resource::getResource();
   test_executor session;
   openDBOptions options;
   options.path.dataPath = DATA_PATH;
   options.path.lsmPath = LSM_PATH;
   options.lsmOptions.hitCfMemtableSize = 1 << 20;

   UINT32 count = 1000000;
   DATA_COLLECTION_PTR handler;
   bson::BSONObj pattern = BSON("a" << 1);
   const CHAR *indexName = "index";

   bson::BSONObj indexDef = indexTestUtil::createIndexObj(INDEX_TYPE_HYBRID_TREE, indexName, 
                                                          FALSE, pattern);    

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

   rc = db.close(&session, closeDBOptions());
   ASSERT_EQ(SDB_OK, rc);

   rc = db.open(&session, &resource, options);
   ASSERT_EQ(SDB_OK, rc);

   rc = db.openCL(&session, "foo.bar", dmsOpenCLOptions(), handler);
   ASSERT_EQ(SDB_OK, rc);

   mthMatchTree mt;
   dmsIndexScanOptions o;

   builder.reset();
   bson::BSONObjBuilder subBuilder(builder.subobjStart("a"));
   subBuilder.append("$gte", 0);
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

   for (UINT32 i = 0; i < count; ++i)
   {
      rc = cursor->fetchNext(&session);
      ASSERT_EQ(SDB_OK, rc);
      ASSERT_EQ(i, cursor->getBsonRecord().getIntField("a"));
   }

   rc = cursor->fetchNext(&session);
   ASSERT_EQ(SDB_DMS_EOC, rc);

   rc = db.close(&session, closeDBOptions());
   ASSERT_EQ(SDB_OK, rc);
}

TEST_F(hit_index_test, base_test_2)
{
   INT32 rc = SDB_OK;
   vesselImpl db;
   outerResource resource = test_outer_resource::getResource();
   test_executor session;
   openDBOptions options;
   options.path.dataPath = DATA_PATH;
   options.path.lsmPath = LSM_PATH;
   options.lsmOptions.hitCfMemtableSize = 1 << 20;

   UINT32 count = 1000000;
   DATA_COLLECTION_PTR handler;
   bson::BSONObj pattern = BSON("a" << 1 << "b" << 1);

   const CHAR *indexName = "index";

   bson::BSONObj indexDef = indexTestUtil::createIndexObj(INDEX_TYPE_HYBRID_TREE, indexName, 
                                                          FALSE, pattern, FALSE);    

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

   std::default_random_engine generator;
   std::uniform_int_distribution<INT32> distribution(0, count);
   std::vector<INT32> bValues;
   bson::BSONObjBuilder builder;
   ossPoolString s = "fixed_prefix";

   for (UINT32 i = 0; i < count; ++i)
   {
      utilInsertResult res;
      builder.reset();
      builder.appendStrWithNoTerminating("a", s.data(), s.size());
      INT32 randomInt32 = distribution(generator);
      builder.appendIntOrLL("b", randomInt32);
      bValues.push_back(randomInt32);
      bson::BSONObj obj = builder.done();

      rc = handler->insertRecord(&session, obj, dmsInsertRecordOptions(), &res);
      ASSERT_EQ(SDB_OK, rc);
   }
   // rc = handler->createIndex(&session, dmsBuildIndexOptions(), indexDef);
   // ASSERT_EQ(SDB_OK, rc);

   // rc = db.close(&session, closeDBOptions());
   // ASSERT_EQ(SDB_OK, rc);

   // rc = db.open(&session, &resource, options);
   // ASSERT_EQ(SDB_OK, rc);

   // rc = db.openCL(&session, "foo.bar", dmsOpenCLOptions(), handler);
   // ASSERT_EQ(SDB_OK, rc);

   mthMatchTree mt;
   dmsIndexScanOptions o;

   builder.reset();
   bson::BSONObjBuilder subBuilder(builder.subobjStart("a"));
   subBuilder.append("$gte", s.data(), s.size());
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
   std::sort(bValues.begin(), bValues.end());
   for (UINT32 i = 0; i < count; ++i)
   {
      rc = cursor->fetchNext(&session);
      if (SDB_OK != rc)
      {
         cout << i << endl;
      }
      ASSERT_EQ(SDB_OK, rc);
      BSONObj obj = cursor->getBsonRecord();
      ASSERT_STREQ(s.data(), obj.getStringField("a"));
      INT32 v = obj.getIntField("b");
      if (v != bValues[i])
      {
         ASSERT_EQ(bValues[i], obj.getIntField("b"));
      }
      //ASSERT_EQ(bValues[i], obj.getIntField("b"));
   }

   rc = cursor->fetchNext(&session);
   ASSERT_EQ(SDB_DMS_EOC, rc);

   rc = db.close(&session, closeDBOptions());
   ASSERT_EQ(SDB_OK, rc);
}

TEST_F(hit_index_test, base_compress_test_1)
{
   INT32 rc = SDB_OK;
   vesselImpl db;
   outerResource resource = test_outer_resource::getResource();
   test_executor session;
   openDBOptions options;
   options.path.dataPath = DATA_PATH;
   options.path.lsmPath = LSM_PATH;
   options.lsmOptions.hitCfMemtableSize = 1 << 20;

   UINT32 count = 1000000;
   DATA_COLLECTION_PTR handler;
   bson::BSONObj pattern = BSON("a" << 1 << "b" << 1);
   const CHAR *indexName = "index";

   bson::BSONObj indexDef = indexTestUtil::createIndexObj(INDEX_TYPE_HYBRID_TREE, indexName, 
                                                          FALSE, pattern, TRUE);    

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

   std::default_random_engine generator;
   std::uniform_int_distribution<INT32> distribution(0, count);
   std::vector<INT32> bValues;
   INT32 div = 50;
   bson::BSONObjBuilder builder;
   ossPoolString s = "fixed_prefix";
   for (UINT32 i = 0; i < count; ++i)
   {
      utilInsertResult res;
      builder.reset();
      builder.appendStrWithNoTerminating("a", s.data(), s.size());
      INT32 randomInt32 = distribution(generator);
      builder.appendIntOrLL("b", randomInt32);
      bValues.push_back(randomInt32);
      bson::BSONObj obj = builder.done();

      rc = handler->insertRecord(&session, obj, dmsInsertRecordOptions(), &res);
      ASSERT_EQ(SDB_OK, rc);
   }

   mthMatchTree mt;
   dmsIndexScanOptions o;

   builder.reset();
   bson::BSONObjBuilder subBuilder(builder.subobjStart("a"));
   subBuilder.append("$gte", s.data(), s.size());
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
   std::sort(bValues.begin(), bValues.end());
   for (UINT32 i = 0; i < count; ++i)
   {
      rc = cursor->fetchNext(&session);
      ASSERT_EQ(SDB_OK, rc);
      BSONObj obj = cursor->getBsonRecord();
      ASSERT_STREQ(s.data(), obj.getStringField("a"));
      ASSERT_EQ(bValues[i], obj.getIntField("b"));
   }

   rc = cursor->fetchNext(&session);
   ASSERT_EQ(SDB_DMS_EOC, rc);

   rc = db.close(&session, closeDBOptions());
   ASSERT_EQ(SDB_OK, rc);
}