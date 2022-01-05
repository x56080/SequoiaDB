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

   Source File Name = update_test.cpp

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
#include "../bson/bson.hpp"
#include "pd.hpp"
#include "vessel/builtinRecordUpdater.h"
#include "dmsCursorReader.hpp"

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

   dmsBsonCursorReader reader;
   reader.init(cursor, FALSE);
   for (UINT32 i = 0; i < count; ++i)
   {
      rc = reader.fetchNext(&executor);
      ASSERT_EQ(SDB_OK, rc);
      const bson::BSONObj  &r = reader.getRecord();
      ASSERT_EQ(r.getIntField("a"), r.getIntField("b") * 2 + 1);
   }

   reader.fini();
   db.close(&executor, closeDBOptions());

   rc = db.open(&executor, &resource, options);
   ASSERT_EQ(SDB_OK, rc);

   rc = db.openCL(&executor, "foo.bar", dmsOpenCLOptions(), handler);
   ASSERT_EQ(SDB_OK, rc);

   rc = handler->scan(&executor, dmsScanOptions(), cursor);
   ASSERT_EQ(SDB_OK, rc);

   reader.init(cursor, FALSE);
   for (UINT32 i = 0; i < count; ++i)
   {
      rc = reader.fetchNext(&executor);
      ASSERT_EQ(SDB_OK, rc);
      const bson::BSONObj  &r = reader.getRecord();
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
   indexParameters params;
   params.type = INDEX_TYPE_BTREE;
   bson::BSONObj indexDef = indexTestUtil::createIndexObj("index", params, BSON("a" << 1));

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

   dmsBsonCursorReader reader;
   reader.init(cursor, FALSE);
   for (UINT32 i = 0; i < count; ++i)
   {
      rc = reader.fetchNext(&executor);
      ASSERT_EQ(SDB_OK, rc);
      const bson::BSONObj &r = reader.getRecord();
      ASSERT_EQ(r.getIntField("a"), r.getIntField("b") * 2 + 1);
   }

   reader.fini();
   handler->close();
   db.close(&executor, closeDBOptions());

   rc = db.open(&executor, &resource, options);
   ASSERT_EQ(SDB_OK, rc);

   rc = db.openCL(&executor, "foo.bar", dmsOpenCLOptions(), handler);
   ASSERT_EQ(SDB_OK, rc);

   rc = handler->scan(&executor, dmsScanOptions(), cursor);
   ASSERT_EQ(SDB_OK, rc);

   reader.init(cursor, FALSE);
   for (UINT32 i = 0; i < count; ++i)
   {
      rc = reader.fetchNext(&executor);
      ASSERT_EQ(SDB_OK, rc);
      const bson::BSONObj &r = reader.getRecord();
      ASSERT_EQ(r.getIntField("a"), r.getIntField("b") * 2 + 1);
   }

   reader.fini();
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

   dmsBsonCursorReader reader;
   reader.init(cursor, FALSE);
   rc = reader.fetchNext(&executor);
   ASSERT_EQ(SDB_OK, rc);
   const bson::BSONObj  &r = reader.getRecord();
   ASSERT_EQ(r.getStringField("a"), updateStr);

   reader.fini();
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

   dmsBsonCursorReader reader;
   reader.init(cursor, FALSE);
   rc = reader.fetchNext(&executor);
   ASSERT_EQ(SDB_OK, rc);
   const bson::BSONObj &r = reader.getRecord();
   ASSERT_EQ(r.getStringField("a"), updateStr);
   for (UINT32 i = 0; i < count - 1; ++i)
   {
      rc = reader.fetchNext(&executor);
      ASSERT_EQ(SDB_OK, rc);
      const bson::BSONObj &r = reader.getRecord();
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

   dmsBsonCursorReader reader;
   reader.init(cursor, FALSE);
   rc = reader.fetchNext(&executor);
   ASSERT_EQ(SDB_OK, rc);
   const bson::BSONObj &r = reader.getRecord();
   ASSERT_EQ(r.getStringField("a"), updateStr);
   rc = cursor->fetchNext(&executor);
   ASSERT_EQ(SDB_DMS_EOC, rc);

   db.close(&executor, closeDBOptions());
}