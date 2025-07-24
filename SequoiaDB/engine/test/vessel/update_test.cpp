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

   Source File Name = update_test.cpp

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
#include "../bson/bson.hpp"
#include "pd.hpp"
#include "vessel/builtinRecordUpdater.h"
#include "mthMatchTree.hpp"
#include "interface/IDataStorageEngine.h"

#include <boost/filesystem.hpp>
namespace fs = boost::filesystem;


class update_test : public testing::Test
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

TEST_F(update_test, base_update_test1)
{
   vesselImpl db;
   outerResource resource = test_outer_resource::getResource();
   test_executor executor;
   openDBOptions options;
   options.path.dataPath = DATA_PATH;
   options.path.lsmPath = LSM_PATH;
   DATA_COLLECTION_PTR handler;
   INT32 rc = SDB_OK;
   bson::BSONObjBuilder builder;
   UINT32 count = 10000;
   ossPoolVector<dmsRecordID> rids;
   bson::BSONObj pattern = BSON("$inc" << BSON("a" << 1));
   bsonRecordUpdater updater;
   DATA_CURSOR_PTR cursor;

   mthModifier modifier;
   rc = modifier.loadPattern(pattern);
   ASSERT_EQ(SDB_OK, rc);

   updater.setModifier(&modifier);

   rc = db.open(&executor, &resource, options);
   ASSERT_EQ(SDB_OK, rc);

   rc = db.createCS(&executor, "foo", 1, dmsCreateCSOptions(), bson::BSONObj());
   ASSERT_EQ(SDB_OK, rc);

   rc = db.createCL(&executor, "foo.bar", 1, dmsCreateCLOptions(), bson::BSONObj());
   ASSERT_EQ(SDB_OK, rc);

   rc = db.openCL(&executor, "foo.bar", dmsOpenCLOptions(), handler);
   ASSERT_EQ(SDB_OK, rc);

   for (UINT32 i = 0; i < count; ++i)
   {
      utilInsertResult insertRes;
      /// insert a as even number.
      builder.append("a", i * 2);
      builder.append("b", i);
      bson::BSONObj obj = builder.done();
      insertRes.enableReturnIDInfo();
      rc = handler->insertRecord(&executor, obj, dmsInsertRecordOptions(), &insertRes);
      ASSERT_EQ(SDB_OK, rc);

      INT32 page, slot;
      insertRes.getInsertLoc(page, slot);
      dmsRecordID rid(page, slot);
      rids.push_back(rid);
      builder.reset();
   }

   for (UINT32 i = 0; i < count; ++i)
   {
      /// update a to odd number.
      utilUpdateResult updateRes;
      rc = handler->updateRecord(&executor, rids[i], &updater,
                                 dmsUpdateRecordOptions(), &updateRes);
      ASSERT_EQ(SDB_OK, rc);
   }

   rc = handler->scan(&executor, dmsScanOptions(), cursor);
   ASSERT_EQ(SDB_OK, rc);

   
   for (UINT32 i = 0; i < count; ++i)
   {
      rc = cursor->fetchNext(&executor);
      ASSERT_EQ(SDB_OK, rc);
      bson::BSONObj r = cursor->getBsonRecord();
      ASSERT_EQ(r.getIntField("a"), r.getIntField("b") * 2 + 1);
   }

   
   db.close(&executor, closeDBOptions());

   rc = db.open(&executor, &resource, options);
   ASSERT_EQ(SDB_OK, rc);

   rc = db.openCL(&executor, "foo.bar", dmsOpenCLOptions(), handler);
   ASSERT_EQ(SDB_OK, rc);

   rc = handler->scan(&executor, dmsScanOptions(), cursor);
   ASSERT_EQ(SDB_OK, rc);

   
   for (UINT32 i = 0; i < count; ++i)
   {
      rc = cursor->fetchNext(&executor);
      ASSERT_EQ(SDB_OK, rc);
      bson::BSONObj r = cursor->getBsonRecord();
      ASSERT_EQ(r.getIntField("a"), r.getIntField("b") * 2 + 1);
   }

   db.close(&executor, closeDBOptions());
}

/// update with index
TEST_F(update_test, base_update_test2)
{
   vesselImpl db;
   outerResource resource = test_outer_resource::getResource();
   test_executor executor;
   openDBOptions options;
   options.path.dataPath = DATA_PATH;
   options.path.lsmPath = LSM_PATH;
   DATA_COLLECTION_PTR handler;
   INT32 rc = SDB_OK;
   bson::BSONObjBuilder builder;
   UINT32 count = 10000;
   ossPoolVector<dmsRecordID> rids;
   bson::BSONObj pattern = BSON("$inc" << BSON("a" << 1));
   bsonRecordUpdater updater;
   DATA_CURSOR_PTR cursor;
   bson::BSONObj indexDef = indexTestUtil::createIndexObj(INDEX_TYPE_HYBRID_TREE, "index", TRUE, BSON("a" << 1));

   mthModifier modifier;
   rc = modifier.loadPattern(pattern);
   ASSERT_EQ(SDB_OK, rc);

   updater.setModifier(&modifier);

   rc = db.open(&executor, &resource, options);
   ASSERT_EQ(SDB_OK, rc);

   rc = db.createCS(&executor, "foo", 1, dmsCreateCSOptions(), bson::BSONObj());
   ASSERT_EQ(SDB_OK, rc);

   rc = db.createCL(&executor, "foo.bar", 1, dmsCreateCLOptions(), bson::BSONObj());
   ASSERT_EQ(SDB_OK, rc);

   rc = db.openCL(&executor, "foo.bar", dmsOpenCLOptions(), handler);
   ASSERT_EQ(SDB_OK, rc);

   rc = handler->createIndex(&executor, dmsBuildIndexOptions(), indexDef);
   ASSERT_EQ(SDB_OK, rc);

   for (UINT32 i = 0; i < count; ++i)
   {
      utilInsertResult insertRes;
      /// insert a as even number.
      builder.append("a", i * 2);
      builder.append("b", i);
      bson::BSONObj obj = builder.done();
      insertRes.enableReturnIDInfo();
      rc = handler->insertRecord(&executor, obj, dmsInsertRecordOptions(), &insertRes);
      ASSERT_EQ(SDB_OK, rc);

      INT32 page, slot;
      insertRes.getInsertLoc(page, slot);
      dmsRecordID rid(page, slot);
      rids.push_back(rid);
      builder.reset();
   }

   for (UINT32 i = 0; i < count; ++i)
   {
      /// update a to odd number.
      utilUpdateResult updateRes;
      rc = handler->updateRecord(&executor, rids[i],
                                 &updater, dmsUpdateRecordOptions(), &updateRes);
      ASSERT_EQ(SDB_OK, rc);
   }

   rc = handler->scan(&executor, dmsScanOptions(), cursor);
   ASSERT_EQ(SDB_OK, rc);

   
   for (UINT32 i = 0; i < count; ++i)
   {
      rc = cursor->fetchNext(&executor);
      ASSERT_EQ(SDB_OK, rc);
      bson::BSONObj r = cursor->getBsonRecord();
      ASSERT_EQ(r.getIntField("a"), r.getIntField("b") * 2 + 1);
   }

   
   handler->close();
   db.close(&executor, closeDBOptions());

   rc = db.open(&executor, &resource, options);
   ASSERT_EQ(SDB_OK, rc);

   rc = db.openCL(&executor, "foo.bar", dmsOpenCLOptions(), handler);
   ASSERT_EQ(SDB_OK, rc);

   rc = handler->scan(&executor, dmsScanOptions(), cursor);
   ASSERT_EQ(SDB_OK, rc);

   
   for (UINT32 i = 0; i < count; ++i)
   {
      rc = cursor->fetchNext(&executor);
      ASSERT_EQ(SDB_OK, rc);
      bson::BSONObj r = cursor->getBsonRecord();
      ASSERT_EQ(r.getIntField("a"), r.getIntField("b") * 2 + 1);
   }

   
   handler->close();
   db.close(&executor, closeDBOptions());
}


/*
Name: base_update_test3
Description: 
   记录页内重新存储
   1. 插入一条记录
   2. 将该记录更新为超出原记录reserved长度的记录
   3. 更新触发页内重新存储
   4. 更新后验证记录内容正确性
Expected Result: 
   记录更新正确
   
*/
TEST_F(update_test, base_update_test3)
{
   vesselImpl db;
   outerResource resource = test_outer_resource::getResource();
   test_executor executor;
   openDBOptions options;
   options.path.dataPath = DATA_PATH;
   options.path.lsmPath = LSM_PATH;
   DATA_COLLECTION_PTR handler;
   INT32 rc = SDB_OK;
   bson::BSONObjBuilder builder;

   rc = db.open(&executor, &resource, options);
   ASSERT_EQ(SDB_OK, rc);

   rc = db.createCS(&executor, "foo", 1, dmsCreateCSOptions(), bson::BSONObj());
   ASSERT_EQ(SDB_OK, rc);

   rc = db.createCL(&executor, "foo.bar", 1, dmsCreateCLOptions(), bson::BSONObj());
   ASSERT_EQ(SDB_OK, rc);

   rc = db.openCL(&executor, "foo.bar", dmsOpenCLOptions(), handler);
   ASSERT_EQ(SDB_OK, rc);


   utilInsertResult insertRes;
   builder.append("a", "foo");
   bson::BSONObj obj = builder.done();
   insertRes.enableReturnIDInfo();
   rc = handler->insertRecord(&executor, obj, dmsInsertRecordOptions(), &insertRes);
   ASSERT_EQ(SDB_OK, rc);

   INT32 page, slot;
   insertRes.getInsertLoc(page, slot);

   dmsRecordID rid(page, slot);
   utilUpdateResult updateRes;

   string updateStr = "footesttesttesttest";
   bson::BSONObj pattern = BSON("$set" << BSON("a" << updateStr));
   bsonRecordUpdater updater;
   mthModifier modifier;
   rc = modifier.loadPattern(pattern);
   ASSERT_EQ(SDB_OK, rc);

   updater.setModifier(&modifier);

   rc = handler->updateRecord(&executor, rid, &updater,
                              dmsUpdateRecordOptions(), &updateRes);
   ASSERT_EQ(SDB_OK, rc);
   
   DATA_CURSOR_PTR cursor;
   rc = handler->scan(&executor, dmsScanOptions(), cursor);
   ASSERT_EQ(SDB_OK, rc);

   
   rc = cursor->fetchNext(&executor);
   ASSERT_EQ(SDB_OK, rc);
   bson::BSONObj r = cursor->getBsonRecord();
   ASSERT_EQ(r.getStringField("a"), updateStr);

   
   handler->close();
   db.close(&executor, closeDBOptions());
}

/*
Name: base_update_test4
Description: 
   重组记录页
   1. 构造长度为2k的记录(保证reserved固定,保证记录长度固定)
   2. 多次插入记录填满整个页
   3. 更新插入的第一条记录,触发该记录所在页的重组
   4. 更新后验证第一条记录及其他记录的正确性
Expected Result: 
   记录更新正确
*/
TEST_F(update_test, base_update_test4)
{
   vesselImpl db;
   outerResource resource = test_outer_resource::getResource();
   test_executor executor;
   openDBOptions options;
   options.path.dataPath = DATA_PATH;
   options.path.lsmPath = LSM_PATH;
   DATA_COLLECTION_PTR handler;
   INT32 rc = SDB_OK;
   bson::BSONObjBuilder builder;
   UINT32 count = 50;
   ossPoolVector<dmsRecordID> rids;
   bsonRecordUpdater updater;
   DATA_CURSOR_PTR cursor;

   rc = db.open(&executor, &resource, options);
   ASSERT_EQ(SDB_OK, rc);

   rc = db.createCS(&executor, "foo", 1, dmsCreateCSOptions(), bson::BSONObj());
   ASSERT_EQ(SDB_OK, rc);

   rc = db.createCL(&executor, "foo.bar", 1, dmsCreateCLOptions(), bson::BSONObj());
   ASSERT_EQ(SDB_OK, rc);

   rc = db.openCL(&executor, "foo.bar", dmsOpenCLOptions(), handler);
   ASSERT_EQ(SDB_OK, rc);

   // build records
   string insertStr(2000, 'a');
   string updateStr(2400, 'b');
   
   for (UINT32 i = 0; i < count; ++i)
   {
      utilInsertResult insertRes;
      builder.append("a", insertStr);
      bson::BSONObj obj = builder.done();
      insertRes.enableReturnIDInfo();
      rc = handler->insertRecord(&executor, obj, dmsInsertRecordOptions(), &insertRes);
      ASSERT_EQ(SDB_OK, rc);

      INT32 page, slot;
      insertRes.getInsertLoc(page, slot);
      dmsRecordID rid(page, slot);
      rids.push_back(rid);
      builder.reset();
   }

   mthModifier modifier;

   bson::BSONObj pattern = BSON("$set" << BSON("a" << updateStr));
   rc = modifier.loadPattern(pattern);
   ASSERT_EQ(SDB_OK, rc);
   updater.setModifier(&modifier);

   utilUpdateResult updateRes;
   rc = handler->updateRecord(&executor, rids[0], &updater,
                              dmsUpdateRecordOptions(), &updateRes);
   ASSERT_EQ(SDB_OK, rc);
   
   rc = handler->scan(&executor, dmsScanOptions(), cursor);
   ASSERT_EQ(SDB_OK, rc);

   
   rc = cursor->fetchNext(&executor);
   ASSERT_EQ(SDB_OK, rc);
   bson::BSONObj r = cursor->getBsonRecord();
   ASSERT_EQ(r.getStringField("a"), updateStr);
   for (UINT32 i = 0; i < count - 1; ++i)
   {
      rc = cursor->fetchNext(&executor);
      ASSERT_EQ(SDB_OK, rc);
      bson::BSONObj r = cursor->getBsonRecord();
      ASSERT_EQ(r.getStringField("a"), insertStr);
   }

   db.close(&executor, closeDBOptions());
}

/*
Name: base_update_test5
Description: 
   重组带有删除的记录页
   1. 构造长度为2k的记录(保证reserved固定,保证记录长度固定)
   2. 多次插入记录填满整个页
   3. 删除第一条以外的记录
   4. 更新插入的第一条记录,触发记录所在页的重组
   5. 更新后验证记录内容正确性
Expected Result: 
   记录更新正确
*/
TEST_F(update_test, base_update_test5)
{
   vesselImpl db;
   outerResource resource = test_outer_resource::getResource();
   test_executor executor;
   openDBOptions options;
   DATA_COLLECTION_PTR handler;
   options.path.dataPath = DATA_PATH;
   options.path.lsmPath = LSM_PATH;
   dmsCreateCLOptions clOptions;
   clOptions.pageMinFreePercent = 0;

   INT32 rc = SDB_OK;
   bson::BSONObjBuilder builder;
   UINT32 count = 50;
   ossPoolVector<dmsRecordID> rids;
   bsonRecordUpdater updater;

   rc = db.open(&executor, &resource, options);
   ASSERT_EQ(SDB_OK, rc);

   rc = db.createCS(&executor, "foo", 1, dmsCreateCSOptions(), bson::BSONObj());
   ASSERT_EQ(SDB_OK, rc);

   rc = db.createCL(&executor, "foo.bar", 1, clOptions, bson::BSONObj());
   ASSERT_EQ(SDB_OK, rc);

   rc = db.openCL(&executor, "foo.bar", dmsOpenCLOptions(), handler);
   ASSERT_EQ(SDB_OK, rc);

   // build records
   string insertStr(2000, 'a');
   string updateStr(2400, 'b');

   for (UINT32 i = 0; i < count; ++i)
   {
      utilInsertResult insertRes;
      builder.append("a", insertStr);
      bson::BSONObj obj = builder.done();
      insertRes.enableReturnIDInfo();
      rc = handler->insertRecord(&executor, obj, dmsInsertRecordOptions(), &insertRes);
      ASSERT_EQ(SDB_OK, rc);

      INT32 page, slot;
      insertRes.getInsertLoc(page, slot);
      dmsRecordID rid(page, slot);
      rids.push_back(rid);
      builder.reset();
   }

   for (UINT32 i = 1; i < count; ++i)
   {

      utilDeleteResult deleteRes;
      const dmsRecordID &rid = rids[i];
      rc = handler->deleteRecord(&executor, rid, dmsDeleteRecordOptions(), &deleteRes);
      ASSERT_EQ(SDB_OK, rc);
   }

   mthModifier modifier;

   DATA_CURSOR_PTR cursor;
   bson::BSONObj pattern = BSON("$set" << BSON("a" << updateStr));
   rc = modifier.loadPattern(pattern);
   ASSERT_EQ(SDB_OK, rc);
   updater.setModifier(&modifier);

   utilUpdateResult updateRes;
   rc = handler->updateRecord(&executor, rids[0], &updater,
                              dmsUpdateRecordOptions(), &updateRes);
   ASSERT_EQ(SDB_OK, rc);
   
   rc = handler->scan(&executor, dmsScanOptions(), cursor);
   ASSERT_EQ(SDB_OK, rc);

   
   rc = cursor->fetchNext(&executor);
   ASSERT_EQ(SDB_OK, rc);
   bson::BSONObj r = cursor->getBsonRecord();
   ASSERT_EQ(r.getStringField("a"), updateStr);
   rc = cursor->fetchNext(&executor);
   ASSERT_EQ(SDB_DMS_EOC, rc);

   db.close(&executor, closeDBOptions());
}

/*
Name: base_update_test6
Description: 
   普通记录更新为overflowed记录
   1. 插入多条记录
   2. 更新第一条记录为overflowed(更新长度超过该页剩余空闲空间)
   3. 更新后验证该记录及其他记录正确性
Expected Result: 
   记录更新正确
*/
TEST_F(update_test, base_update_test6)
{
   vesselImpl db;
   outerResource resource = test_outer_resource::getResource();
   test_executor executor;
   openDBOptions options;
   DATA_COLLECTION_PTR handler;
   options.path.dataPath = DATA_PATH;
   options.path.lsmPath = LSM_PATH;
   dmsCreateCLOptions clOptions;
   clOptions.pageMinFreePercent = 0;
   INT32 rc = SDB_OK;
   UINT32 count = 100;
   bson::BSONObjBuilder builder;
   ossPoolVector<dmsRecordID> rids;

   rc = db.open(&executor, &resource, options);
   ASSERT_EQ(SDB_OK, rc);

   rc = db.createCS(&executor, "foo", 1, dmsCreateCSOptions(), bson::BSONObj());
   ASSERT_EQ(SDB_OK, rc);

   rc = db.createCL(&executor, "foo.bar", 1, clOptions, bson::BSONObj());
   ASSERT_EQ(SDB_OK, rc);

   rc = db.openCL(&executor, "foo.bar", dmsOpenCLOptions(), handler);
   ASSERT_EQ(SDB_OK, rc);

   // build records
   string insertStr(2000, 'a');
   string updateStr(5000, 'b');

   for (UINT32 i = 0; i < count; ++i)
   {
      utilInsertResult insertRes;
      builder.append("a", insertStr);
      bson::BSONObj obj = builder.done();
      insertRes.enableReturnIDInfo();
      rc = handler->insertRecord(&executor, obj, dmsInsertRecordOptions(), &insertRes);
      ASSERT_EQ(SDB_OK, rc);

      INT32 page, slot;
      insertRes.getInsertLoc(page, slot);
      dmsRecordID rid(page, slot);
      rids.push_back(rid);
      builder.reset();
   }

   bsonRecordUpdater updater;
   mthModifier modifier;
   bson::BSONObj pattern = BSON("$set" << BSON("a" << updateStr));
   rc = modifier.loadPattern(pattern);
   ASSERT_EQ(SDB_OK , rc);
   updater.setModifier(&modifier);

   utilUpdateResult updateRes;
   rc = handler->updateRecord(&executor, rids[0], &updater,
                              dmsUpdateRecordOptions(), &updateRes);
   ASSERT_EQ(SDB_OK, rc);

   DATA_CURSOR_PTR cursor;
   rc = handler->scan(&executor, dmsScanOptions(), cursor);
   ASSERT_EQ(SDB_OK, rc);

   
   rc = cursor->fetchNext(&executor);
   ASSERT_EQ(SDB_OK, rc);
   bson::BSONObj r = cursor->getBsonRecord();
   ASSERT_EQ(r.getStringField("a"), updateStr);
   for (UINT32 i = 0; i < count - 1; ++i)
   {
      rc = cursor->fetchNext(&executor);
      ASSERT_EQ(SDB_OK, rc);
      bson::BSONObj r = cursor->getBsonRecord();
      ASSERT_EQ(r.getStringField("a"), insertStr);
   }
   
   db.close(&executor, closeDBOptions()); 
}

/*
Name: base_update_test7
Description: 
   原地更新overflowed记录
   1. 插入多条记录
   2. 更新第一条记录为overflowed(更新长度超过该页剩余空闲空间)
   3. 再次更新第一条记录(更新长度不超过reserved空间)
   4. 验证记录正确性
Expected Result: 
   记录更新正确
*/
TEST_F(update_test, base_update_test7)
{
   vesselImpl db;
   outerResource resource = test_outer_resource::getResource();
   test_executor executor;
   openDBOptions options;
   DATA_COLLECTION_PTR handler;
   options.path.dataPath = DATA_PATH;
   options.path.lsmPath = LSM_PATH;
   dmsCreateCLOptions clOptions;
   clOptions.pageMinFreePercent = 0;
   INT32 rc = SDB_OK;
   UINT32 count = 10000;
   bson::BSONObjBuilder builder;
   ossPoolVector<dmsRecordID> rids;

   rc = db.open(&executor, &resource, options);
   ASSERT_EQ(SDB_OK, rc);

   rc = db.createCS(&executor, "foo", 1, dmsCreateCSOptions(), bson::BSONObj());
   ASSERT_EQ(SDB_OK, rc);

   rc = db.createCL(&executor, "foo.bar", 1, clOptions, bson::BSONObj());
   ASSERT_EQ(SDB_OK, rc);

   rc = db.openCL(&executor, "foo.bar", dmsOpenCLOptions(), handler);
   ASSERT_EQ(SDB_OK, rc);

   string insertStr(100, 'a');
   string overflowedStr(2100, 'b');
   string updateStr(100, 'c');

   for (UINT32 i = 0; i < count; ++i)
   {
      utilInsertResult insertRes;
      builder.append("a", insertStr);
      bson::BSONObj obj = builder.done();
      insertRes.enableReturnIDInfo();
      rc = handler->insertRecord(&executor, obj, dmsInsertRecordOptions(), &insertRes);
      ASSERT_EQ(SDB_OK, rc);

      INT32 page, slot;
      insertRes.getInsertLoc(page, slot);
      dmsRecordID rid(page, slot);
      rids.push_back(rid);
      builder.reset();
   }

   // set record overflowed
   bsonRecordUpdater updater;
   mthModifier modifier1;
   bson::BSONObj pattern1 = BSON("$set" << BSON("a" << overflowedStr));
   rc = modifier1.loadPattern(pattern1);
   ASSERT_EQ(SDB_OK , rc);
   updater.setModifier(&modifier1);

   utilUpdateResult updateRes;
   rc = handler->updateRecord(&executor, rids[0], &updater,
                              dmsUpdateRecordOptions(), &updateRes);
   ASSERT_EQ(SDB_OK, rc);

   mthModifier modifier2;
   bson::BSONObj pattern2 = BSON("$set" << BSON("a" << updateStr));
   rc = modifier2.loadPattern(pattern2);
   ASSERT_EQ(SDB_OK , rc);
   updater.setModifier(&modifier2);

   updateRes.reset();
   rc = handler->updateRecord(&executor, rids[0], &updater,
                              dmsUpdateRecordOptions(), &updateRes);
   ASSERT_EQ(SDB_OK, rc);


   DATA_CURSOR_PTR cursor;
   rc = handler->scan(&executor, dmsScanOptions(), cursor);
   ASSERT_EQ(SDB_OK, rc);

   
   rc = cursor->fetchNext(&executor);
   ASSERT_EQ(SDB_OK, rc);
   bson::BSONObj r = cursor->getBsonRecord();
   ASSERT_EQ(r.getStringField("a"), updateStr);
   for (UINT32 i = 0; i < count - 1; ++i)
   {
      rc = cursor->fetchNext(&executor);
      ASSERT_EQ(SDB_OK, rc);
      bson::BSONObj r = cursor->getBsonRecord();
      ASSERT_EQ(r.getStringField("a"), insertStr);
   }
   
   db.close(&executor, closeDBOptions());
}

/*
Name: base_update_test8
Description: 
   页内重新存储overflowed记录
   1. 插入多条记录
   2. 更新第一条记录为overflowed(更新长度超过该页剩余空闲空间)
   3. 再次更新第一条记录(更新长度超过reserved,不超过连续剩余空间)
   4. 验证记录正确性
Expected Result: 
   记录更新正确
*/
TEST_F(update_test, base_update_test8)
{
   vesselImpl db;
   outerResource resource = test_outer_resource::getResource();
   test_executor executor;
   openDBOptions options;
   DATA_COLLECTION_PTR handler;
   options.path.dataPath = DATA_PATH;
   options.path.lsmPath = LSM_PATH;
   dmsCreateCLOptions clOptions;
   clOptions.pageMinFreePercent = 0;
   INT32 rc = SDB_OK;
   UINT32 count = 10000;
   bson::BSONObjBuilder builder;
   ossPoolVector<dmsRecordID> rids;

   rc = db.open(&executor, &resource, options);
   ASSERT_EQ(SDB_OK, rc);

   rc = db.createCS(&executor, "foo", 1, dmsCreateCSOptions(), bson::BSONObj());
   ASSERT_EQ(SDB_OK, rc);

   rc = db.createCL(&executor, "foo.bar", 1, clOptions, bson::BSONObj());
   ASSERT_EQ(SDB_OK, rc);

   rc = db.openCL(&executor, "foo.bar", dmsOpenCLOptions(), handler);
   ASSERT_EQ(SDB_OK, rc);

   string insertStr(100, 'a');
   string overflowedStr(2100, 'b');
   string updateStr(2200, 'c');

   for (UINT32 i = 0; i < count; ++i)
   {
      utilInsertResult insertRes;
      builder.append("a", insertStr);
      bson::BSONObj obj = builder.done();
      insertRes.enableReturnIDInfo();
      rc = handler->insertRecord(&executor, obj, dmsInsertRecordOptions(), &insertRes);
      ASSERT_EQ(SDB_OK, rc);

      INT32 page, slot;
      insertRes.getInsertLoc(page, slot);
      dmsRecordID rid(page, slot);
      rids.push_back(rid);
      builder.reset();
   }

   // set record overflowed
   bsonRecordUpdater updater;
   mthModifier modifier1;
   bson::BSONObj pattern1 = BSON("$set" << BSON("a" << overflowedStr));
   rc = modifier1.loadPattern(pattern1);
   ASSERT_EQ(SDB_OK , rc);
   updater.setModifier(&modifier1);

   utilUpdateResult updateRes;
   rc = handler->updateRecord(&executor, rids[0], &updater,
                              dmsUpdateRecordOptions(), &updateRes);
   ASSERT_EQ(SDB_OK, rc);

   mthModifier modifier2;
   bson::BSONObj pattern2 = BSON("$set" << BSON("a" << updateStr));
   rc = modifier2.loadPattern(pattern2);
   ASSERT_EQ(SDB_OK , rc);
   updater.setModifier(&modifier2);

   updateRes.reset();
   rc = handler->updateRecord(&executor, rids[0], &updater,
                              dmsUpdateRecordOptions(), &updateRes);
   ASSERT_EQ(SDB_OK, rc);


   DATA_CURSOR_PTR cursor;
   rc = handler->scan(&executor, dmsScanOptions(), cursor);
   ASSERT_EQ(SDB_OK, rc);

   
   rc = cursor->fetchNext(&executor);
   ASSERT_EQ(SDB_OK, rc);
   bson::BSONObj r = cursor->getBsonRecord();
   ASSERT_EQ(r.getStringField("a"), updateStr);
   for (UINT32 i = 0; i < count - 1; ++i)
   {
      rc = cursor->fetchNext(&executor);
      ASSERT_EQ(SDB_OK, rc);
      bson::BSONObj r = cursor->getBsonRecord();
      ASSERT_EQ(r.getStringField("a"), insertStr);
   }
   
   db.close(&executor, closeDBOptions());
}


/*
Name: base_update_test9
Description: 
   overflowed记录页内重组
   1. 插入多条记录
   2. 更新第一条记录为overflowed(更新长度超过该页剩余空闲空间)
   3. 再次更新第一条记录(更新长度不超过该页剩余空闲空间)
   4. 验证记录正确性
Expected Result: 
   记录更新正确
*/
TEST_F(update_test, base_update_test9)
{
   vesselImpl db;
   outerResource resource = test_outer_resource::getResource();
   test_executor executor;
   openDBOptions options;
   DATA_COLLECTION_PTR handler;
   options.path.dataPath = DATA_PATH;
   options.path.lsmPath = LSM_PATH;
   dmsCreateCLOptions clOptions;
   clOptions.pageMinFreePercent = 0;
   INT32 rc = SDB_OK;
   UINT32 count = 10000;
   bson::BSONObjBuilder builder;
   ossPoolVector<dmsRecordID> rids;

   rc = db.open(&executor, &resource, options);
   ASSERT_EQ(SDB_OK, rc);

   rc = db.createCS(&executor, "foo", 1, dmsCreateCSOptions(), bson::BSONObj());
   ASSERT_EQ(SDB_OK, rc);

   rc = db.createCL(&executor, "foo.bar", 1, clOptions, bson::BSONObj());
   ASSERT_EQ(SDB_OK, rc);

   rc = db.openCL(&executor, "foo.bar", dmsOpenCLOptions(), handler);
   ASSERT_EQ(SDB_OK, rc);

   string insertStr(100, 'a');
   string overflowedStr(2100, 'b');
   string updateStr(10000, 'c');

   for (UINT32 i = 0; i < count; ++i)
   {
      utilInsertResult insertRes;
      builder.append("a", insertStr);
      bson::BSONObj obj = builder.done();
      insertRes.enableReturnIDInfo();
      rc = handler->insertRecord(&executor, obj, dmsInsertRecordOptions(), &insertRes);
      ASSERT_EQ(SDB_OK, rc);

      INT32 page, slot;
      insertRes.getInsertLoc(page, slot);
      dmsRecordID rid(page, slot);
      rids.push_back(rid);
      builder.reset();
   }

   // set record overflowed
   bsonRecordUpdater updater;
   mthModifier modifier1;
   bson::BSONObj pattern1 = BSON("$set" << BSON("a" << overflowedStr));
   rc = modifier1.loadPattern(pattern1);
   ASSERT_EQ(SDB_OK , rc);
   updater.setModifier(&modifier1);

   utilUpdateResult updateRes;
   rc = handler->updateRecord(&executor, rids[0], &updater,
                              dmsUpdateRecordOptions(), &updateRes);
   ASSERT_EQ(SDB_OK, rc);

   mthModifier modifier2;
   bson::BSONObj pattern2 = BSON("$set" << BSON("a" << updateStr));
   rc = modifier2.loadPattern(pattern2);
   ASSERT_EQ(SDB_OK , rc);
   updater.setModifier(&modifier2);

   updateRes.reset();
   rc = handler->updateRecord(&executor, rids[0], &updater,
                              dmsUpdateRecordOptions(), &updateRes);
   ASSERT_EQ(SDB_OK, rc);


   DATA_CURSOR_PTR cursor;
   rc = handler->scan(&executor, dmsScanOptions(), cursor);
   ASSERT_EQ(SDB_OK, rc);

   
   rc = cursor->fetchNext(&executor);
   ASSERT_EQ(SDB_OK, rc);
   bson::BSONObj r = cursor->getBsonRecord();
   ASSERT_EQ(r.getStringField("a"), updateStr);
   for (UINT32 i = 0; i < count - 1; ++i)
   {
      rc = cursor->fetchNext(&executor);
      ASSERT_EQ(SDB_OK, rc);
      bson::BSONObj r = cursor->getBsonRecord();
      ASSERT_EQ(r.getStringField("a"), insertStr);
   }
   
   db.close(&executor, closeDBOptions());
}

/*
Name: base_update_test10
Description: 
   更新overflowed记录为overflowed记录
   1. 插入多条记录
   2. 更新第一条记录为overflowed(更新长度超过该页剩余空闲空间)
   3. 再次更新第一条记录(更新长度超过该页剩余空闲空间)
   4. 验证记录正确性
Expected Result: 
   记录更新正确
*/
TEST_F(update_test, base_update_test10)
{
   vesselImpl db;
   outerResource resource = test_outer_resource::getResource();
   test_executor executor;
   openDBOptions options;
   DATA_COLLECTION_PTR handler;
   options.path.dataPath = DATA_PATH;
   options.path.lsmPath = LSM_PATH;
   dmsCreateCLOptions clOptions;
   clOptions.pageMinFreePercent = 0;
   INT32 rc = SDB_OK;
   UINT32 count = 10000;
   bson::BSONObjBuilder builder;
   ossPoolVector<dmsRecordID> rids;

   rc = db.open(&executor, &resource, options);
   ASSERT_EQ(SDB_OK, rc);

   rc = db.createCS(&executor, "foo", 1, dmsCreateCSOptions(), bson::BSONObj());
   ASSERT_EQ(SDB_OK, rc);

   rc = db.createCL(&executor, "foo.bar", 1, clOptions, bson::BSONObj());
   ASSERT_EQ(SDB_OK, rc);

   rc = db.openCL(&executor, "foo.bar", dmsOpenCLOptions(), handler);
   ASSERT_EQ(SDB_OK, rc);

   string insertStr(100, 'a');
   string overflowedStr(2100, 'b');
   string updateStr(20000, 'c');

   for (UINT32 i = 0; i < count; ++i)
   {
      utilInsertResult insertRes;
      builder.append("a", insertStr);
      bson::BSONObj obj = builder.done();
      insertRes.enableReturnIDInfo();
      rc = handler->insertRecord(&executor, obj, dmsInsertRecordOptions(), &insertRes);
      ASSERT_EQ(SDB_OK, rc);

      INT32 page, slot;
      insertRes.getInsertLoc(page, slot);
      dmsRecordID rid(page, slot);
      rids.push_back(rid);
      builder.reset();
   }

   // set record overflowed
   bsonRecordUpdater updater;
   mthModifier modifier1;
   bson::BSONObj pattern1 = BSON("$set" << BSON("a" << overflowedStr));
   rc = modifier1.loadPattern(pattern1);
   ASSERT_EQ(SDB_OK , rc);
   updater.setModifier(&modifier1);

   utilUpdateResult updateRes;
   rc = handler->updateRecord(&executor, rids[0], &updater,
                              dmsUpdateRecordOptions(), &updateRes);
   ASSERT_EQ(SDB_OK, rc);

   mthModifier modifier2;
   bson::BSONObj pattern2 = BSON("$set" << BSON("a" << updateStr));
   rc = modifier2.loadPattern(pattern2);
   ASSERT_EQ(SDB_OK , rc);
   updater.setModifier(&modifier2);

   updateRes.reset();
   rc = handler->updateRecord(&executor, rids[0], &updater,
                              dmsUpdateRecordOptions(), &updateRes);
   ASSERT_EQ(SDB_OK, rc);


   DATA_CURSOR_PTR cursor;
   rc = handler->scan(&executor, dmsScanOptions(), cursor);
   ASSERT_EQ(SDB_OK, rc);

   
   rc = cursor->fetchNext(&executor);
   ASSERT_EQ(SDB_OK, rc);
   bson::BSONObj r = cursor->getBsonRecord();
   ASSERT_EQ(r.getStringField("a"), updateStr);
   for (UINT32 i = 0; i < count - 1; ++i)
   {
      rc = cursor->fetchNext(&executor);
      ASSERT_EQ(SDB_OK, rc);
      bson::BSONObj r = cursor->getBsonRecord();
      ASSERT_EQ(r.getStringField("a"), insertStr);
   }
   
   db.close(&executor, closeDBOptions());
}

/*
Name: base_update_test11
Description: 
   Update normal record to big record
   1. 插入多条普通记录
   2. 更新普通记录为big record
   3. 验证记录正确性
Expected Result: 
   记录更新正确
*/
TEST_F(update_test, base_update_test11)
{
   vesselImpl db;
   outerResource resource = test_outer_resource::getResource();
   test_executor executor;
   openDBOptions options;
   DATA_COLLECTION_PTR handler;
   options.path.dataPath = DATA_PATH;
   options.path.lsmPath = LSM_PATH;
   dmsCreateCSOptions csOption;
   INT32 rc = SDB_OK;
   UINT32 count = 10000;
   bson::BSONObjBuilder builder;
   ossPoolVector<dmsRecordID> rids;

   rc = db.open(&executor, &resource, options);
   ASSERT_EQ(SDB_OK, rc);

   rc = db.createCS(&executor, "foo", 1, csOption, bson::BSONObj());
   ASSERT_EQ(SDB_OK, rc);

   rc = db.createCL(&executor, "foo.bar", 1, dmsCreateCLOptions(), bson::BSONObj());
   ASSERT_EQ(SDB_OK, rc);

   rc = db.openCL(&executor, "foo.bar", dmsOpenCLOptions(), handler);
   ASSERT_EQ(SDB_OK, rc);

   string insertStr(100, 'a');
   string updateStr(csOption.dataPageSize + 1000, 'x');

   for (UINT32 i = 0; i < count; ++i)
   {
      utilInsertResult insertRes;
      builder.append("a", insertStr);
      bson::BSONObj obj = builder.done();
      insertRes.enableReturnIDInfo();
      rc = handler->insertRecord(&executor, obj, dmsInsertRecordOptions(), &insertRes);
      ASSERT_EQ(SDB_OK, rc);

      INT32 page, slot;
      insertRes.getInsertLoc(page, slot);
      dmsRecordID rid(page, slot);
      rids.push_back(rid);
      builder.reset();
   }

   for (UINT32 i = 0; i < count; ++i)
   {
      bsonRecordUpdater updater;
      mthModifier modifier;
      bson::BSONObj pattern = BSON("$set" << BSON("a" << updateStr));
      rc = modifier.loadPattern(pattern);
      ASSERT_EQ(SDB_OK , rc);
      updater.setModifier(&modifier);

      utilUpdateResult updateRes;
      rc = handler->updateRecord(&executor, rids[i], &updater,
                                 dmsUpdateRecordOptions(), &updateRes);
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
      ASSERT_EQ(r.getStringField("a"), updateStr);
   }
   
   db.close(&executor, closeDBOptions());
}

/*
Name: base_update_test12
Description: 
   Update big record to big record
   1. 插入多条big record
   2. 更新记录为新的big record
   3. 验证记录正确性
Expected Result: 
   记录更新正确
*/
TEST_F(update_test, base_update_test12)
{
   vesselImpl db;
   outerResource resource = test_outer_resource::getResource();
   test_executor executor;
   openDBOptions options;
   DATA_COLLECTION_PTR handler;
   options.path.dataPath = DATA_PATH;
   options.path.lsmPath = LSM_PATH;
   dmsCreateCSOptions csOption;
   INT32 rc = SDB_OK;
   UINT32 count = 10000;
   bson::BSONObjBuilder builder;
   ossPoolVector<dmsRecordID> rids;

   rc = db.open(&executor, &resource, options);
   ASSERT_EQ(SDB_OK, rc);

   rc = db.createCS(&executor, "foo", 1, csOption, bson::BSONObj());
   ASSERT_EQ(SDB_OK, rc);

   rc = db.createCL(&executor, "foo.bar", 1, dmsCreateCLOptions(), bson::BSONObj());
   ASSERT_EQ(SDB_OK, rc);

   rc = db.openCL(&executor, "foo.bar", dmsOpenCLOptions(), handler);
   ASSERT_EQ(SDB_OK, rc);

   string insertStr(csOption.dataPageSize + 1000, 'a');
   string updateStr(csOption.dataPageSize + 1000, 'b');

   for (UINT32 i = 0; i < count; ++i)
   {
      utilInsertResult insertRes;
      builder.append("a", insertStr);
      bson::BSONObj obj = builder.done();
      insertRes.enableReturnIDInfo();
      rc = handler->insertRecord(&executor, obj, dmsInsertRecordOptions(), &insertRes);
      ASSERT_EQ(SDB_OK, rc);

      INT32 page, slot;
      insertRes.getInsertLoc(page, slot);
      dmsRecordID rid(page, slot);
      rids.push_back(rid);
      builder.reset();
   }

   for (UINT32 i = 0; i < count; ++i)
   {
      bsonRecordUpdater updater;
      mthModifier modifier;
      bson::BSONObj pattern = BSON("$set" << BSON("a" << updateStr));
      rc = modifier.loadPattern(pattern);
      ASSERT_EQ(SDB_OK , rc);
      updater.setModifier(&modifier);

      utilUpdateResult updateRes;
      rc = handler->updateRecord(&executor, rids[i], &updater,
                                 dmsUpdateRecordOptions(), &updateRes);
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
      ASSERT_EQ(r.getStringField("a"), updateStr);
   }
   
   db.close(&executor, closeDBOptions());
}

/*
Name: base_update_test13
Description: 
   Update big record to normal record
   1. 插入多条big record
   2. 更新big record为普通记录
   3. 验证记录正确性
Expected Result: 
   记录更新正确
*/
TEST_F(update_test, base_update_test13)
{
   vesselImpl db;
   outerResource resource = test_outer_resource::getResource();
   test_executor executor;
   openDBOptions options;
   DATA_COLLECTION_PTR handler;
   options.path.dataPath = DATA_PATH;
   options.path.lsmPath = LSM_PATH;
   dmsCreateCSOptions csOption;
   INT32 rc = SDB_OK;
   UINT32 count = 10000;
   bson::BSONObjBuilder builder;
   ossPoolVector<dmsRecordID> rids;

   rc = db.open(&executor, &resource, options);
   ASSERT_EQ(SDB_OK, rc);

   rc = db.createCS(&executor, "foo", 1, csOption, bson::BSONObj());
   ASSERT_EQ(SDB_OK, rc);

   rc = db.createCL(&executor, "foo.bar", 1, dmsCreateCLOptions(), bson::BSONObj());
   ASSERT_EQ(SDB_OK, rc);

   rc = db.openCL(&executor, "foo.bar", dmsOpenCLOptions(), handler);
   ASSERT_EQ(SDB_OK, rc);

   string insertStr(csOption.dataPageSize + 1000, 'a');
   string updateStr(100, 'b');

   for (UINT32 i = 0; i < count; ++i)
   {
      utilInsertResult insertRes;
      builder.append("a", insertStr);
      bson::BSONObj obj = builder.done();
      insertRes.enableReturnIDInfo();
      rc = handler->insertRecord(&executor, obj, dmsInsertRecordOptions(), &insertRes);
      ASSERT_EQ(SDB_OK, rc);

      INT32 page, slot;
      insertRes.getInsertLoc(page, slot);
      dmsRecordID rid(page, slot);
      rids.push_back(rid);
      builder.reset();
   }

   for (UINT32 i = 0; i < count; ++i)
   {
      bsonRecordUpdater updater;
      mthModifier modifier;
      bson::BSONObj pattern = BSON("$set" << BSON("a" << updateStr));
      rc = modifier.loadPattern(pattern);
      ASSERT_EQ(SDB_OK , rc);
      updater.setModifier(&modifier);

      utilUpdateResult updateRes;
      rc = handler->updateRecord(&executor, rids[i], &updater,
                                 dmsUpdateRecordOptions(), &updateRes);
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
      ASSERT_EQ(r.getStringField("a"), updateStr);
   }
   
   db.close(&executor, closeDBOptions());
}

/*
Name: base_update_test14
Description: 
   Update overflowed normal record to big record
   1. 插入多条普通记录
   2. 更新第一条记录为overflowed record
   3. 再次更新overflowed记录为big record
   4. 验证记录更新正确性
Expected Result: 
   记录更新正确
*/
TEST_F(update_test, base_update_test14)
{
   vesselImpl db;
   outerResource resource = test_outer_resource::getResource();
   test_executor executor;
   openDBOptions options;
   DATA_COLLECTION_PTR handler;
   options.path.dataPath = DATA_PATH;
   options.path.lsmPath = LSM_PATH;
   dmsCreateCSOptions csOptions;
   dmsCreateCLOptions clOptions;
   clOptions.pageMinFreePercent = 0;
   INT32 rc = SDB_OK;
   UINT32 count = 10000;
   bson::BSONObjBuilder builder;
   ossPoolVector<dmsRecordID> rids;

   rc = db.open(&executor, &resource, options);
   ASSERT_EQ(SDB_OK, rc);

   rc = db.createCS(&executor, "foo", 1, csOptions, bson::BSONObj());
   ASSERT_EQ(SDB_OK, rc);

   rc = db.createCL(&executor, "foo.bar", 1, clOptions, bson::BSONObj());
   ASSERT_EQ(SDB_OK, rc);

   rc = db.openCL(&executor, "foo.bar", dmsOpenCLOptions(), handler);
   ASSERT_EQ(SDB_OK, rc);

   string insertStr(100, 'a');
   string overflowedStr(2100, 'b');
   string updateStr(csOptions.dataPageSize + 1000, 'c');

   for (UINT32 i = 0; i < count; ++i)
   {
      utilInsertResult insertRes;
      builder.append("a", insertStr);
      bson::BSONObj obj = builder.done();
      insertRes.enableReturnIDInfo();
      rc = handler->insertRecord(&executor, obj, dmsInsertRecordOptions(), &insertRes);
      ASSERT_EQ(SDB_OK, rc);

      INT32 page, slot;
      insertRes.getInsertLoc(page, slot);
      dmsRecordID rid(page, slot);
      rids.push_back(rid);
      builder.reset();
   }

   // set record overflowed
   bsonRecordUpdater updater;
   mthModifier modifier1;
   bson::BSONObj pattern1 = BSON("$set" << BSON("a" << overflowedStr));
   rc = modifier1.loadPattern(pattern1);
   ASSERT_EQ(SDB_OK , rc);
   updater.setModifier(&modifier1);

   utilUpdateResult updateRes;
   rc = handler->updateRecord(&executor, rids[0], &updater,
                              dmsUpdateRecordOptions(), &updateRes);
   ASSERT_EQ(SDB_OK, rc);

   mthModifier modifier2;
   bson::BSONObj pattern2 = BSON("$set" << BSON("a" << updateStr));
   rc = modifier2.loadPattern(pattern2);
   ASSERT_EQ(SDB_OK , rc);
   updater.setModifier(&modifier2);

   updateRes.reset();
   rc = handler->updateRecord(&executor, rids[0], &updater,
                              dmsUpdateRecordOptions(), &updateRes);
   ASSERT_EQ(SDB_OK, rc);


   DATA_CURSOR_PTR cursor;
   rc = handler->scan(&executor, dmsScanOptions(), cursor);
   ASSERT_EQ(SDB_OK, rc);

   
   rc = cursor->fetchNext(&executor);
   ASSERT_EQ(SDB_OK, rc);
   bson::BSONObj r = cursor->getBsonRecord();
   ASSERT_EQ(r.getStringField("a"), updateStr);
   for (UINT32 i = 0; i < count - 1; ++i)
   {
      rc = cursor->fetchNext(&executor);
      ASSERT_EQ(SDB_OK, rc);
      bson::BSONObj r = cursor->getBsonRecord();
      ASSERT_EQ(r.getStringField("a"), insertStr);
   }
   
   db.close(&executor, closeDBOptions());
}