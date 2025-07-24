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

   Source File Name = index_delete_test.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          12/06/2021  LYC  Initial Draft

   Last Changed =

*******************************************************************************/
#include "test_def.h"
#include "vessel/vesselImpl.h"
#include "vessel/requestContext.h"
#include <gtest/gtest.h>
#include "ossUtil.hpp"
#include "../bson/bson.hpp"
#include "pd.hpp"
#include "vessel/collectionOptions.h"
#include "vessel/builtinRecordUpdater.h"
#include "mthMatchTree.hpp"

#include <boost/filesystem.hpp>

namespace fs = boost::filesystem;

class index_delete_test : public testing::Test
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

/*
Name: base_delete_test1
      base_delete_test2
Description: 
   记录删除测试
   1. 创建集合和LSM索引，并插入记录
   2. 验证记录数量
   3. 删除插入记录
   4. 验证记录数量
Expected Result: 
   插入并删除记录成功且数量正确
*/
void delete_test1(INDEX_TYPE type)
{
   INT32 rc = SDB_OK;
   vesselImpl db;
   outerResource resource = test_outer_resource::getResource();
   test_executor session;

   openDBOptions options;
   options.path.dataPath = DATA_PATH;
   options.path.lsmPath = LSM_PATH;

   ossPoolVector<dmsRecordID> rids;

   DATA_COLLECTION_PTR handler;
   UINT32 count = 10000;

   bson::BSONObj pattern = BSON("a" << 1);
   bson::BSONObj indexDef = indexTestUtil::createIndexObj(type, "index", 
                                                          FALSE, pattern);

   bson::BSONObjBuilder builder;

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

   // insert records
   for (UINT32 i = 0; i < count; ++i)
   {
      utilInsertResult res;
      builder.reset();
      builder.append("a", i);
      builder.append("b", i + 1);
      bson::BSONObj obj = builder.done();

      rc = handler->insertRecord(&session, obj, dmsInsertRecordOptions(), &res);
      ASSERT_EQ(SDB_OK, rc);

      INT32 page, slot;
      res.getInsertLoc(page, slot);
      dmsRecordID rid(page, slot);
      rids.push_back(rid);
      builder.reset();
   }

   UINT64 currentCount = 0;
   rc = handler->getRecordCount(&session, currentCount);
   ASSERT_EQ(SDB_OK, rc);
   ASSERT_EQ((UINT64)count, currentCount);

   // delete records
   for (UINT32 i = 0; i < count; ++i)
   {
      utilDeleteResult deleteRes;
      const dmsRecordID &rid = rids[i];
      rc = handler->deleteRecord(&session, rid, dmsDeleteRecordOptions(), &deleteRes);
      ASSERT_EQ(SDB_OK, rc);
   }

   currentCount = 0;
   rc = handler->getRecordCount(&session, currentCount);
   ASSERT_EQ(SDB_OK, rc);
   ASSERT_EQ((UINT64)0, currentCount);

   rc = db.close(&session, closeDBOptions());
   ASSERT_EQ(SDB_OK, rc);
}

TEST_F(index_delete_test, base_delete_test1)
{
   delete_test1(INDEX_TYPE_HYBRID_TREE);
}


/*
Name: base_delete_test3
      base_delete_test4
Description: 
   记录删除和索引扫描
   1. 创建集合和索引，并插入记录
   2. 扫描验证记录正确性
   3. 删除插入记录
   4. 验证记录数量
   5. 重新插入记录
   6. 扫描验证记录正确性
Expected Result: 
   插入并删除记录成功且数量正确
*/
void delete_test2(INDEX_TYPE type)
{
   INT32 rc = SDB_OK;
   vesselImpl db;
   outerResource resource = test_outer_resource::getResource();
   test_executor session;

   openDBOptions options;
   options.path.dataPath = DATA_PATH;
   options.path.lsmPath = LSM_PATH;

   ossPoolVector<dmsRecordID> rids;

   DATA_COLLECTION_PTR handler;
   INT32 count = 10000;

   bson::BSONObj pattern = BSON("a" << 1);
   bson::BSONObj indexDef = indexTestUtil::createIndexObj(type, "index", 
                                                          FALSE, pattern);
   bson::BSONObjBuilder builder;

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

   // insert records
   for (INT32 i = 0; i < count; ++i)
   {
      utilInsertResult res;
      builder.reset();
      builder.append("a", i);
      builder.append("b", i + 1);
      bson::BSONObj obj = builder.done();
      rc = handler->insertRecord(&session, obj, dmsInsertRecordOptions(), &res);
      ASSERT_EQ(SDB_OK, rc);

      INT32 page, slot;
      res.getInsertLoc(page, slot);
      dmsRecordID rid(page, slot);
      rids.push_back(rid);
      builder.reset();
   }
   UINT64 currentCount = 0;
   rc = handler->getRecordCount(&session, currentCount);
   ASSERT_EQ(SDB_OK, rc);
   ASSERT_EQ(count, currentCount);

   // index scan
   mthMatchTree mt;
   dmsIndexScanOptions o;
   for (INT32 i = 0; i < count; ++i)
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
      DATA_CURSOR_PTR cursor;
      rc = handler->scanIndex(&session, "index", predicates, o, cursor);
      ASSERT_EQ(SDB_OK, rc);

      
      rc = cursor->fetchNext(&session);
      
      ASSERT_EQ(SDB_OK, rc);
      ASSERT_EQ(i, cursor->getBsonRecord().getIntField("a"));
      
      cursor->close();
      mt.clear();
   }

   // delete records
   for (INT32 i = 0; i < count; ++i)
   {
      utilDeleteResult deleteRes;
      const dmsRecordID &rid = rids[i];
      rc = handler->deleteRecord(&session, rid, dmsDeleteRecordOptions(), &deleteRes);
      ASSERT_EQ(SDB_OK, rc);
   }
   rc = handler->getRecordCount(&session, currentCount);
   ASSERT_EQ(SDB_OK, rc);
   ASSERT_EQ((UINT64)0, currentCount);

   //re-insert records
   rids.clear();
   for (INT32 i = 0; i < count; ++i)
   {
      utilInsertResult res;
      builder.reset();
      builder.append("a", i);
      builder.append("b", i + 1);
      bson::BSONObj obj = builder.done();
      rc = handler->insertRecord(&session, obj, dmsInsertRecordOptions(), &res);
      ASSERT_EQ(SDB_OK, rc);

      INT32 page, slot;
      res.getInsertLoc(page, slot);
      dmsRecordID rid(page, slot);
      rids.push_back(rid);
      builder.reset();
   }

   // re-scan 
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
   rc = handler->scanIndex(&session, "index", predicates, o, cursor);
   ASSERT_EQ(SDB_OK, rc);
   
   
   for (INT32 i = 0; i < count; ++i)
   {
      rc = cursor->fetchNext(&session);
      ASSERT_EQ(SDB_OK, rc);
      ASSERT_EQ(i, cursor->getBsonRecord().getIntField("a"));
   }
   cursor->close();
   mt.clear();

   rc = db.close(&session, closeDBOptions());
   ASSERT_EQ(SDB_OK, rc);
}

TEST_F(index_delete_test, base_delete_test3)
{
   delete_test2(INDEX_TYPE_HYBRID_TREE);
}


/*
Name: base_delete_test5
      base_delete_test6
Description: 
   记录删除和索引扫描
   1. 创建集合和索引，并插入记录
   2. 扫描验证记录正确性
   3. 删除部分插入记录
   4. 验证记录数量
   5. 扫描验证正确性
Expected Result: 
   插入并删除记录成功且数量正确
*/
void partial_delete(INDEX_TYPE type)
{
   INT32 rc = SDB_OK;
   vesselImpl db;
   outerResource resource = test_outer_resource::getResource();
   test_executor session;

   openDBOptions options;
   options.path.dataPath = DATA_PATH;
   options.path.lsmPath = LSM_PATH;

   ossPoolVector<dmsRecordID> rids;

   DATA_COLLECTION_PTR handler;
   UINT32 count = 10000;

   bson::BSONObj pattern = BSON("a" << 1);
   const CHAR *indexName = "index";
   bson::BSONObj indexDef = indexTestUtil::createIndexObj(type, "index", 
                                                          FALSE, pattern);

   bson::BSONObjBuilder builder;

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

   // insert records
   for (UINT32 i = 0; i < count; ++i)
   {
      utilInsertResult res;
      builder.reset();
      builder.append("a", i);
      builder.append("b", i + 1);
      bson::BSONObj obj = builder.done();
      rc = handler->insertRecord(&session, obj, dmsInsertRecordOptions(), &res);
      ASSERT_EQ(SDB_OK, rc);

      INT32 page, slot;
      res.getInsertLoc(page, slot);
      dmsRecordID rid(page, slot);
      rids.push_back(rid);
      builder.reset();
   }
   UINT64 currentCount = 0;
   rc = handler->getRecordCount(&session, currentCount);
   ASSERT_EQ(SDB_OK, rc);
   ASSERT_EQ(count, currentCount);

   //delete records
   for (UINT32 i = 0; i < count; i += 2)
   {
      utilDeleteResult deleteRes;

      const dmsRecordID &rid = rids[i];
      rc = handler->deleteRecord(&session, rid, dmsDeleteRecordOptions(), &deleteRes);
      ASSERT_EQ(SDB_OK, rc);
   }
   rc = handler->getRecordCount(&session, currentCount);
   ASSERT_EQ(SDB_OK, rc);
   ASSERT_EQ((UINT64)count / 2, currentCount);

   // index scan
   mthMatchTree mt;
   dmsIndexScanOptions o;

   builder.reset();
   bson::BSONObjBuilder subBuilder(builder.subobjStart("a"));
   subBuilder.append("$gt", 0);
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
   rc = handler->scanIndex(&session, indexName, predicates, o, cursor);
   ASSERT_EQ(SDB_OK, rc);
   
   
   for (UINT32 i = 1; i < count; i+=2)
   {
      rc = cursor->fetchNext(&session);
      ASSERT_EQ(SDB_OK, rc);
      ASSERT_EQ(i, cursor->getBsonRecord().getIntField("a"));
   }
   cursor->close();
   mt.clear();

   rc = db.close(&session, closeDBOptions());
   ASSERT_EQ(SDB_OK, rc);
}

TEST_F(index_delete_test, base_delete_test5)
{
   partial_delete(INDEX_TYPE_HYBRID_TREE);
}


/*
Name: base_backward_delete_test1
      base_backward_delete_test2
Description: 
   记录删除和索引的逆序扫描
   1. 创建集合和索引，并插入记录
   2. 逆序扫描验证记录正确性
   3. 删除部分插入记录
   4. 验证记录数量
   5. 逆序扫描验证正确性
Expected Result: 
   插入并删除记录成功且数量正确
*/
void backward_delete(INDEX_TYPE type)
{
   INT32 rc = SDB_OK;
   vesselImpl db;
   outerResource resource = test_outer_resource::getResource();
   test_executor session;
   openDBOptions options;

   options.path.dataPath = DATA_PATH;
   options.path.lsmPath = LSM_PATH;

   DATA_COLLECTION_PTR handler;
   INT32 count = 10000;

   ossPoolVector<dmsRecordID> rids;

   bson::BSONObjBuilder builder;

   bson::BSONObj pattern = BSON("a" << 1);
   const CHAR *indexName = "index";
   bson::BSONObj indexDef = indexTestUtil::createIndexObj(type, "index", 
                                                          FALSE, pattern);
   

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

   // insert records
   for (INT32 i = 0; i < count; ++i)
   {
      utilInsertResult res;
      builder.reset();
      builder.append("a", i);
      builder.append("b", i + 1);
      bson::BSONObj obj = builder.done();
      rc = handler->insertRecord(&session, obj, dmsInsertRecordOptions(), &res);
      ASSERT_EQ(SDB_OK, rc);

      INT32 page, slot;
      res.getInsertLoc(page, slot);
      dmsRecordID rid(page, slot);
      rids.push_back(rid);
      builder.reset();
   }

   UINT64 currentCount = 0;
   rc = handler->getRecordCount(&session, currentCount);
   ASSERT_EQ(SDB_OK, rc);
   ASSERT_EQ(count, currentCount);

   //delete records
   for (INT32 i = count - 1; i >= count / 2; --i)
   {
      utilDeleteResult deleteRes;
      const dmsRecordID &rid = rids[i];
      rc = handler->deleteRecord(&session, rid, dmsDeleteRecordOptions(), &deleteRes);
      ASSERT_EQ(SDB_OK, rc);
   }
   rc = handler->getRecordCount(&session, currentCount);
   ASSERT_EQ(SDB_OK, rc);
   ASSERT_EQ((UINT64)count / 2, currentCount);

   mthMatchTree mt;
   dmsIndexScanOptions o;
   // scan
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

      DATA_CURSOR_PTR cursor;
      rc = handler->scanIndex(&session, indexName, predicates, o, cursor);
      ASSERT_EQ(SDB_OK, rc);

      
      

      for (INT32 i = count / 2 - 1; i >= 0; --i)
      {
         rc = cursor->fetchNext(&session);
         ASSERT_EQ(SDB_OK, rc);
         ASSERT_EQ(i, cursor->getBsonRecord().getIntField("a"));
      }
      rc = cursor->fetchNext(&session);
      ASSERT_EQ(SDB_DMS_EOC, rc);
   
      mt.clear();
   }

   rc = db.close(&session, closeDBOptions());
   ASSERT_EQ(SDB_OK, rc);
}

TEST_F(index_delete_test, base_backward_delete_test1)
{
   backward_delete(INDEX_TYPE_HYBRID_TREE);
}