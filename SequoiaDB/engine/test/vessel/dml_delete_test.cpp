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

   Source File Name = dml_delete_test.cpp

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
#include "vessel/collectionOptions.h"
#include "vessel/builtinRecordUpdater.h"

#include <boost/filesystem.hpp>
namespace fs = boost::filesystem;


class dml_delete_test : public testing::Test
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
Description: 
   删除普通记录
   1. 插入多条记录并删除
   2. 删除前后验证记录操作正确性
Expected Result: 
   记录插入成功并被正确删除
*/
TEST_F(dml_delete_test, base_delete_test1)
{
   vesselImpl db;
   outerResource resource = test_outer_resource::getResource();
   test_executor executor;
   openDBOptions options;
   options.path.dataPath = DATA_PATH;
   options.path.lsmPath = LSM_PATH;

   INT32 rc = SDB_OK;
   bson::BSONObjBuilder builder;
   UINT32 count = 10000;
   ossPoolVector<dmsRecordID> rids;

   DATA_COLLECTION_PTR cl;

   rc = db.open(&executor, &resource, options);
   ASSERT_EQ(SDB_OK, rc);

   rc = db.createCS(&executor, "foo", 1, dmsCreateCSOptions(), bson::BSONObj());
   ASSERT_EQ(SDB_OK, rc);

   rc = db.createCL(&executor, "foo.bar", 1, dmsCreateCLOptions(), bson::BSONObj());
   ASSERT_EQ(SDB_OK, rc);

   rc = db.openCL(&executor, "foo.bar", dmsOpenCLOptions(), cl);
   ASSERT_EQ(SDB_OK, rc);

   for (UINT32 i = 0; i < count; ++i)
   {
      utilInsertResult insertRes;
      /// insert a as even number.
      builder.append("a", i);
      builder.append("b", i);

      insertRes.enableReturnIDInfo();
      rc = cl->insertRecord(&executor, builder.done(),
                            dmsInsertRecordOptions(), &insertRes);
      ASSERT_EQ(SDB_OK, rc);

      INT32 page, slot;
      insertRes.getInsertLoc(page, slot);
      dmsRecordID rid(page, slot);
      rids.push_back(rid);
      builder.reset();
   }

   for (UINT32 i = 0; i < count; ++i)
   {

      utilDeleteResult deleteRes;
      const dmsRecordID &rid = rids[i];
      rc = cl->deleteRecord(&executor, rid, dmsDeleteRecordOptions(), &deleteRes);
      ASSERT_EQ(SDB_OK, rc);
   }

   UINT64 currentCount = 0;
   rc = cl->getRecordCount(&executor, currentCount);
   ASSERT_EQ(SDB_OK, rc);
   ASSERT_EQ(0, currentCount);

   DATA_CURSOR_PTR cursor;
   rc = cl->scan(&executor, dmsScanOptions(), cursor);
   ASSERT_EQ(SDB_OK, rc);

   rc = cursor->fetchNext(&executor);
   ASSERT_EQ(SDB_DMS_EOC, rc);

   db.close(&executor, closeDBOptions());

   rc = db.open(&executor, &resource, options);
   ASSERT_EQ(SDB_OK, rc);

   rc = db.openCL(&executor, "foo.bar", dmsOpenCLOptions(), cl);
   ASSERT_EQ(SDB_OK, rc);

   rc = cl->getRecordCount(&executor, currentCount);
   ASSERT_EQ(SDB_OK, rc);
   ASSERT_EQ(0, currentCount);

   db.close(&executor, closeDBOptions());
}

/*
Name: base_delete_test2
Description: 
   删除overflowed普通记录
   1. 插入多条普通记录
   2. 更新第一条记录为overflowed记录
   3. 删除该记录
   4. 验证记录操作正确性
Expected Result: 
   记录被删除
*/
TEST_F(dml_delete_test, base_delete_test2)
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
      ASSERT_EQ(cursor->getBsonRecord().getStringField("a"), insertStr);
   }

   utilDeleteResult deleteRes;
   rc = handler->deleteRecord(&executor, rids[0], dmsDeleteRecordOptions(), &deleteRes);
   ASSERT_EQ(SDB_OK, rc);

   UINT64 currentCount = 0;
   rc = handler->getRecordCount(&executor, currentCount);
   ASSERT_EQ(SDB_OK, rc);
   ASSERT_EQ(count - 1, currentCount);

   rc = handler->scan(&executor, dmsScanOptions(), cursor);
   ASSERT_EQ(SDB_OK, rc);

   for (UINT64 i = 0; i < currentCount; ++i)
   {
      rc = cursor->fetchNext(&executor);
      ASSERT_EQ(SDB_OK, rc);
      ASSERT_EQ(cursor->getBsonRecord().getStringField("a"), insertStr);
   }
   rc = cursor->fetchNext(&executor);
   ASSERT_EQ(SDB_DMS_EOC, rc);

   db.close(&executor, closeDBOptions()); 
}

/*
Name: base_delete_test3
Description: 
   big record删除
   1. 插入多条big record
   2. 删除所有big record
   3. 验证记录操作正确性
Expected Result: 
   记录被成功删除
*/
TEST_F(dml_delete_test, base_delete_test3)
{
   vesselImpl db;
   outerResource resource = test_outer_resource::getResource();
   test_executor executor;
   openDBOptions options;
   options.path.dataPath = DATA_PATH;
   options.path.lsmPath = LSM_PATH;
   dmsCreateCSOptions csOption;

   INT32 rc = SDB_OK;
   bson::BSONObjBuilder builder;
   UINT32 count = 10000;
   ossPoolVector<dmsRecordID> rids;

   DATA_COLLECTION_PTR cl;

   rc = db.open(&executor, &resource, options);
   ASSERT_EQ(SDB_OK, rc);

   rc = db.createCS(&executor, "foo", 1, csOption, bson::BSONObj());
   ASSERT_EQ(SDB_OK, rc);

   rc = db.createCL(&executor, "foo.bar", 1, dmsCreateCLOptions(), bson::BSONObj());
   ASSERT_EQ(SDB_OK, rc);

   rc = db.openCL(&executor, "foo.bar", dmsOpenCLOptions(), cl);
   ASSERT_EQ(SDB_OK, rc);

   string insertStr( csOption.dataPageSize + 1000, 'x');
   for (UINT32 i = 0; i < count; ++i)
   {
      utilInsertResult insertRes;
      builder.append("a", insertStr);
      insertRes.enableReturnIDInfo();
      rc = cl->insertRecord(&executor, builder.done(),
                            dmsInsertRecordOptions(), &insertRes);
      ASSERT_EQ(SDB_OK, rc);

      INT32 page, slot;
      insertRes.getInsertLoc(page, slot);
      dmsRecordID rid(page, slot);
      rids.push_back(rid);
      builder.reset();
   }

   for (UINT32 i = 0; i < count; ++i)
   {

      utilDeleteResult deleteRes;
      const dmsRecordID &rid = rids[i];
      rc = cl->deleteRecord(&executor, rid, dmsDeleteRecordOptions(), &deleteRes);
      ASSERT_EQ(SDB_OK, rc);
   }

   UINT64 currentCount = 0;
   rc = cl->getRecordCount(&executor, currentCount);
   ASSERT_EQ(SDB_OK, rc);
   ASSERT_EQ(0, currentCount);

   DATA_CURSOR_PTR cursor;
   rc = cl->scan(&executor, dmsScanOptions(), cursor);
   ASSERT_EQ(SDB_OK, rc);

   rc = cursor->fetchNext(&executor);
   ASSERT_EQ(SDB_DMS_EOC, rc);

   db.close(&executor, closeDBOptions());

   rc = db.open(&executor, &resource, options);
   ASSERT_EQ(SDB_OK, rc);

   rc = db.openCL(&executor, "foo.bar", dmsOpenCLOptions(), cl);
   ASSERT_EQ(SDB_OK, rc);

   rc = cl->getRecordCount(&executor, currentCount);
   ASSERT_EQ(SDB_OK, rc);
   ASSERT_EQ(0, currentCount);

   db.close(&executor, closeDBOptions());
}