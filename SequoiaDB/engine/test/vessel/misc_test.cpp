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

   Source File Name = misc_test.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          02/11/2022  LYC  Initial Draft

   Last Changed =

******************************************************************************/
#include "test_def.h"
#include "vessel/vesselImpl.h"
#include "dmsCursorReader.hpp"
#include <gtest/gtest.h>
#include <boost/filesystem.hpp>

namespace fs = boost::filesystem;

class misc_test : public testing::Test
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
Name: base_openCL_test1
Description: 
   使用collection名称开启collection
   1. 创建名称为foo的collection space
   2. 创建名称为bar的collection
   3. 使用错误的名称开启collection
   4. 使用正确的名称开启collection
   5. 验证开启collection正确性
Expected Result: 
   错误名称开启失败，正确名称开启成功
*/
TEST_F(misc_test, base_openCL_test1)
{
   INT32 rc = SDB_OK;
   vesselImpl db;
   outerResource resource = test_outer_resource::getResource();
   test_executor session;
   openDBOptions options;
   options.path.dataPath = DATA_PATH;
   options.path.lsmPath = LSM_PATH;

   DATA_COLLECTION_PTR handler;

   rc = db.open(&session, &resource, options);
   ASSERT_EQ(SDB_OK, rc);

   rc = db.createCS(&session, "foo", 1, dmsCreateCSOptions(), bson::BSONObj());
   ASSERT_EQ(SDB_OK, rc);

   rc = db.createCL(&session, "foo.bar", 1, dmsCreateCLOptions(), bson::BSONObj());
   ASSERT_EQ(SDB_OK, rc);

   rc = db.openCL(&session, "foo.bar1", dmsOpenCLOptions(), handler);
   ASSERT_EQ(SDB_DMS_NOTEXIST, rc);

   rc = db.openCL(&session, "foo.bar", dmsOpenCLOptions(), handler);
   ASSERT_EQ(SDB_OK, rc);

   rc = db.close(&session, closeDBOptions());
   ASSERT_EQ(SDB_OK, rc);
}

/*
Name: base_openCL_test2
Description: 
   使用uniqueId开启collection
   1. 创建uniqueId为1的collection space
   2. 创建clInnerId为1的collection
   3. 使用错误的uniqueId开启collection
   4. 使用正确的uniqueId开启collection
   5. 验证开启collection正确性
Expected Result: 
   错误uniqueId开启失败，正确uniqueId开启成功
*/
TEST_F(misc_test, base_openCL_test2)
{
   INT32 rc = SDB_OK;
   vesselImpl db;
   outerResource resource = test_outer_resource::getResource();
   test_executor session;
   openDBOptions options;
   options.path.dataPath = DATA_PATH;
   options.path.lsmPath = LSM_PATH;

   DATA_COLLECTION_PTR handler;
   utilCSUniqueID csUniqueId = 1;
   utilCLUniqueID clUniqueId1 = 0x0000000100000001;
   utilCLUniqueID clUniqueId2 = 0x0000000100000002;

   rc = db.open(&session, &resource, options);
   ASSERT_EQ(SDB_OK, rc);

   rc = db.createCS(&session, "foo", csUniqueId, dmsCreateCSOptions(), bson::BSONObj());
   ASSERT_EQ(SDB_OK, rc);

   rc = db.createCL(&session, "foo.bar", clUniqueId1, dmsCreateCLOptions(), bson::BSONObj());
   ASSERT_EQ(SDB_OK, rc);

   rc = db.openCL(&session, clUniqueId2, dmsOpenCLOptions(), handler);
   ASSERT_EQ(SDB_DMS_NOTEXIST, rc);

   rc = db.openCL(&session, clUniqueId1, dmsOpenCLOptions(), handler);
   ASSERT_EQ(SDB_OK, rc);

   rc = db.close(&session, closeDBOptions());
   ASSERT_EQ(SDB_OK, rc);
}

/*
Name: base_openCL_test3
Description: 
   使用uniqueId开启collection
   1. 创建uniqueId为1的collection space
   2. 创建clInnerId为1的collection
   3. 使用uniqueId开启collection
   4. 插入一条普通记录
   4. 重启vessel
   5. 使用collection名称重启collection
   6. 验证记录插入正确性
Expected Result: 
   collection被正确开启，记录插入正确
*/
TEST_F(misc_test, base_openCL_test3)
{
   INT32 rc = SDB_OK;
   vesselImpl db;
   outerResource resource = test_outer_resource::getResource();
   test_executor session;
   openDBOptions options;
   options.path.dataPath = DATA_PATH;
   options.path.lsmPath = LSM_PATH;

   utilInsertResult res;
   bson::BSONObjBuilder builder;
   builder.append("a", 1);
   bson::BSONObj obj = builder.obj();

   DATA_COLLECTION_PTR handler;
   constexpr utilCSUniqueID csUniqueId = 1;
   constexpr utilCLUniqueID clUniqueId = 0x0000000100000001;

   rc = db.open(&session, &resource, options);
   ASSERT_EQ(SDB_OK, rc);

   rc = db.createCS(&session, "foo", csUniqueId, dmsCreateCSOptions(), bson::BSONObj());
   ASSERT_EQ(SDB_OK, rc);

   rc = db.createCL(&session, "foo.bar", clUniqueId, dmsCreateCLOptions(), bson::BSONObj());
   ASSERT_EQ(SDB_OK, rc);

   rc = db.openCL(&session, clUniqueId, dmsOpenCLOptions(), handler);
   ASSERT_EQ(SDB_OK, rc);

   rc = handler->insertRecord(&session, obj, dmsInsertRecordOptions(), &res);
   ASSERT_EQ(SDB_OK, rc);

   rc = db.close(&session, closeDBOptions());
   ASSERT_EQ(SDB_OK, rc);

   rc = db.open(&session, &resource, options);
   ASSERT_EQ(SDB_OK, rc);

   rc = db.openCL(&session, "foo.bar", dmsOpenCLOptions(), handler);
   ASSERT_EQ(SDB_OK, rc);

   DATA_CURSOR_PTR cursor;
   dmsBsonCursorReader reader;
   rc = handler->scan(&session, dmsScanOptions(), cursor);
   ASSERT_EQ(SDB_OK, rc);

   reader.init(cursor);
   rc = reader.fetchNext(&session);
   ASSERT_EQ(SDB_OK, rc);
   const bson::BSONObj &r = reader.getRecord();
   ASSERT_EQ(1, r.getIntField("a"));
   rc = reader.fetchNext(&session);
   ASSERT_EQ(SDB_DMS_EOC, rc);

   rc = db.close(&session, closeDBOptions());
   ASSERT_EQ(SDB_OK, rc);
}