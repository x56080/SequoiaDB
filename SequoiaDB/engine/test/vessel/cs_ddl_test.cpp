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

   Source File Name = cs_ddl_test.cpp

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

#include <boost/filesystem.hpp>
namespace fs = boost::filesystem;

class cs_ddl_test : public testing::Test
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
Name: base_createCS_1
Description: 
   单个CS的创建
   1. 创建一个CS
   2. 创建后校验CS总个数是否为1
   3. 列举并校验CS名称是否正确
   4. 列举并校验CSunique_id是否正确
Input: 无
Output: 无
Expected Result: 
   成功创建一个CS 且校验通过
*/
TEST_F(cs_ddl_test, base_createCS_1)
{
   INT32 rc = SDB_OK;
   outerResource resource = test_outer_resource::getResource();
   test_executor session;
   vesselImpl db;
   openDBOptions options;
   createCSOptions csOptions;
   options.path.dataPath = DATA_PATH;
   options.path.indexPath = DATA_PATH;
   options.path.lobMetaPath = DATA_PATH;
   options.path.lobPath = DATA_PATH;
   options.path.lsmPath = LSM_PATH;

   collectionSpaceId identifier;

   UINT32 count = 0;

   slice content;
   bson::BSONObj record;
   cursorHandler c;

   rc = db.open(&session, &resource, options);
   ASSERT_EQ(SDB_OK, rc);

   rc = db.testCollectionSpace(&session, "foo", identifier);
   ASSERT_EQ(SDB_DMS_CS_NOTEXIST, rc);

   rc = db.getCollectionSpaceCount(&session, count);
   ASSERT_EQ(SDB_OK, rc);
   ASSERT_EQ(0, count);

   rc = db.listCollectionSpace(&session, NULL, c);
   ASSERT_EQ(SDB_OK, rc);

   rc = c.getNext(&session, content);
   ASSERT_EQ(SDB_VESSEL_EOC, rc);
   c.close();

   rc = db.createCollectionSpace(&session, "foo", 1, csOptions);
   ASSERT_EQ(SDB_OK, rc);

   // test if the collection space was created successfully
   rc = db.testCollectionSpace(&session, "foo", identifier);
   ASSERT_EQ(SDB_OK, rc);

   // test collection space count
   rc = db.getCollectionSpaceCount(&session, count);
   ASSERT_EQ(SDB_OK, rc);
   ASSERT_EQ(1, count);

   rc = db.listCollectionSpace(&session, NULL, c);
   ASSERT_EQ(SDB_OK, rc);

   // test the collection space's name and unique_id
   rc = c.getNext(&session, content);
   ASSERT_EQ(SDB_OK, rc);
   ASSERT_TRUE(content.isValid());
   record = bson::BSONObj(content.data());
   ASSERT_EQ(0, ossStrcmp("foo", 
                          record.getStringField(CS_DUMP_RECORD_FIELD_NAME)));
   ASSERT_EQ(1, record.getIntField(CS_DUMP_RECORD_FIELD_UNIQUE_ID));

   c.close();
   db.close(&session, closeDBOptions());
}

/*
Name: base_createCS_2
Description: 
   单个CS的创建
   1. 创建一个CS
   2. 重启db
   3. 重启后进行CS相关信息校验
Input: 无
Output: 无
Expected Result: 
   单个CS成功创建 且重启db后可通过校验
*/
TEST_F(cs_ddl_test, _base_createCS_2)
{
   INT32 rc = SDB_OK;
   outerResource resource = test_outer_resource::getResource();
   test_executor session;
   vesselImpl db;
   openDBOptions options;
   createCSOptions csOptions;
   options.path.dataPath = DATA_PATH;
   options.path.indexPath = DATA_PATH;
   options.path.lobMetaPath = DATA_PATH;
   options.path.lobPath = DATA_PATH;
   options.path.lsmPath = LSM_PATH;

   collectionSpaceId identifier;

   UINT32 count = 0;

   slice content;
   bson::BSONObj record;
   cursorHandler c;

   rc = db.open(&session, &resource, options);
   ASSERT_EQ(SDB_OK, rc);

   rc = db.testCollectionSpace(&session, "foo", identifier);
   ASSERT_EQ(SDB_DMS_CS_NOTEXIST, rc);

   rc = db.getCollectionSpaceCount(&session, count);
   ASSERT_EQ(SDB_OK, rc);
   ASSERT_EQ(0, count);

   rc = db.listCollectionSpace(&session, NULL, c);
   ASSERT_EQ(SDB_OK, rc);

   rc = c.getNext(&session, content);
   ASSERT_EQ(SDB_VESSEL_EOC, rc);
   c.close();

   rc = db.createCollectionSpace(&session, "foo", 1, csOptions);
   ASSERT_EQ(SDB_OK, rc);

   // reopen db
   db.close(&session, closeDBOptions());
   rc = db.open(&session, &resource, options);
   ASSERT_EQ(SDB_OK, rc);

   // test if the collection space was created successfully
   rc = db.testCollectionSpace(&session, "foo", identifier);
   ASSERT_EQ(SDB_OK, rc);

   // test collection space count
   rc = db.getCollectionSpaceCount(&session, count);
   ASSERT_EQ(SDB_OK, rc);
   ASSERT_EQ(1, count);

   rc = db.listCollectionSpace(&session, NULL, c);
   ASSERT_EQ(SDB_OK, rc);

   // test the collection space's name and unique_id
   rc = c.getNext(&session, content);
   ASSERT_EQ(SDB_OK, rc);
   ASSERT_TRUE(content.isValid());
   record = bson::BSONObj(content.data());
   ASSERT_EQ(0, ossStrcmp("foo", 
                          record.getStringField(CS_DUMP_RECORD_FIELD_NAME)));
   ASSERT_EQ(1, record.getIntField(CS_DUMP_RECORD_FIELD_UNIQUE_ID));

   c.close();
   db.close(&session, closeDBOptions());
}

/*
Name: base_createCS_3
Description: 
   多个CS的创建
   1. 创建多个CS
   2. 列举并获取其内容
   3. 进行CS相关信息校验
Input: 无
Output: 无
Expected Result: 
   多个CS创建成功 且通过校验
*/
TEST_F(cs_ddl_test, base_createCS_3)
{
   INT32 rc = SDB_OK;
   outerResource resource = test_outer_resource::getResource();
   test_executor session;
   vesselImpl db;
   openDBOptions options;
   createCSOptions csOptions;
   options.path.dataPath = DATA_PATH;
   options.path.indexPath = DATA_PATH;
   options.path.lobMetaPath = DATA_PATH;
   options.path.lobPath = DATA_PATH;
   options.path.lsmPath = LSM_PATH;

   UINT32 count = 0;

   slice content;
   bson::BSONObj record;
   cursorHandler c;

   rc = db.open(&session, &resource, options);
   ASSERT_EQ(SDB_OK, rc);

   rc = db.getCollectionSpaceCount(&session, count);
   ASSERT_EQ(SDB_OK, rc);
   ASSERT_EQ(0, count);

   rc = db.listCollectionSpace(&session, NULL, c);
   ASSERT_EQ(SDB_OK, rc);

   rc = c.getNext(&session, content);
   ASSERT_EQ(SDB_VESSEL_EOC, rc);
   c.close();

   rc = db.createCollectionSpace(&session, "foo1", 1, csOptions);
   ASSERT_EQ(SDB_OK, rc);

   rc = db.createCollectionSpace(&session, "foo2", 2, csOptions);
   ASSERT_EQ(SDB_OK, rc);

   rc = db.createCollectionSpace(&session, "foo3", 3, csOptions);
   ASSERT_EQ(SDB_OK, rc);

   rc = db.getCollectionSpaceCount(&session, count);
   ASSERT_EQ(SDB_OK, rc);
   ASSERT_EQ(3, count);

   rc = db.listCollectionSpace(&session, NULL, c);
   ASSERT_EQ(SDB_OK, rc);

   rc = c.getNext(&session, content);
   ASSERT_EQ(SDB_OK, rc);
   ASSERT_TRUE(content.isValid());
   record = bson::BSONObj(content.data());
   ASSERT_EQ(0, ossStrcmp("foo1", record.getStringField(CS_DUMP_RECORD_FIELD_NAME)));
   ASSERT_EQ(1, record.getIntField(CS_DUMP_RECORD_FIELD_UNIQUE_ID));

   rc = c.getNext(&session, content);
   ASSERT_EQ(SDB_OK, rc);
   ASSERT_TRUE(content.isValid());
   record = bson::BSONObj(content.data());
   ASSERT_EQ(0, ossStrcmp("foo2", record.getStringField(CS_DUMP_RECORD_FIELD_NAME)));
   ASSERT_EQ(2, record.getIntField(CS_DUMP_RECORD_FIELD_UNIQUE_ID));

   rc = c.getNext(&session, content);
   ASSERT_EQ(SDB_OK, rc);
   ASSERT_TRUE(content.isValid());
   record = bson::BSONObj(content.data());
   ASSERT_EQ(0, ossStrcmp("foo3", record.getStringField(CS_DUMP_RECORD_FIELD_NAME)));
   ASSERT_EQ(3, record.getIntField(CS_DUMP_RECORD_FIELD_UNIQUE_ID));

   rc = c.getNext(&session, content);
   ASSERT_EQ(SDB_VESSEL_EOC, rc);
   c.close();

   db.close(&session, closeDBOptions());

   rc = db.open(&session, &resource, options);
   ASSERT_EQ(SDB_OK, rc);
   rc = db.getCollectionSpaceCount(&session, count);
   ASSERT_EQ(SDB_OK, rc);
   ASSERT_EQ(3, count);

   rc = db.listCollectionSpace(&session, NULL, c);
   ASSERT_EQ(SDB_OK, rc);

   rc = c.getNext(&session, content);
   ASSERT_EQ(SDB_OK, rc);
   ASSERT_TRUE(content.isValid());
   record = bson::BSONObj(content.data());
   ASSERT_EQ(0, ossStrcmp("foo1", record.getStringField(CS_DUMP_RECORD_FIELD_NAME)));
   ASSERT_EQ(1, record.getIntField(CS_DUMP_RECORD_FIELD_UNIQUE_ID));

   rc = c.getNext(&session, content);
   ASSERT_EQ(SDB_OK, rc);
   ASSERT_TRUE(content.isValid());
   record = bson::BSONObj(content.data());
   ASSERT_EQ(0, ossStrcmp("foo2", record.getStringField(CS_DUMP_RECORD_FIELD_NAME)));
   ASSERT_EQ(2, record.getIntField(CS_DUMP_RECORD_FIELD_UNIQUE_ID));

   rc = c.getNext(&session, content);
   ASSERT_EQ(SDB_OK, rc);
   ASSERT_TRUE(content.isValid());
   record = bson::BSONObj(content.data());
   ASSERT_EQ(0, ossStrcmp("foo3", record.getStringField(CS_DUMP_RECORD_FIELD_NAME)));
   ASSERT_EQ(3, record.getIntField(CS_DUMP_RECORD_FIELD_UNIQUE_ID));

   rc = c.getNext(&session, content);
   ASSERT_EQ(SDB_VESSEL_EOC, rc);
   c.close();

   db.close(&session, closeDBOptions());
}

/*
Name: base_createCS_4
Description: 
   重复字段CS的创建
   1. 创建CS foo
   2. 创建与foo名称相同的CS
   3. 创建与foo unique_id相同的CS
Input: 无
Output: 无
Expected Result: 
   1. 不重复的CS创建成功
   2. 与已创建有重复信息的CS创建失败
*/
TEST_F(cs_ddl_test, base_createCS_4)
{
   INT32 rc = SDB_OK;
   vesselImpl db;
   outerResource resource = test_outer_resource::getResource();
   test_executor session;
   openDBOptions options;
   createCSOptions csOptions;
   collectionSpaceId identifier;
   options.path.dataPath = DATA_PATH;
   options.path.indexPath = DATA_PATH;
   options.path.lobMetaPath = DATA_PATH;
   options.path.lobPath = DATA_PATH;
   options.path.lsmPath = LSM_PATH;

   UINT32 count = 0;

   rc = db.open(&session, &resource, options);
   ASSERT_EQ(SDB_OK, rc);

   rc = db.getCollectionSpaceCount(&session, count);
   ASSERT_EQ(SDB_OK, rc);
   ASSERT_EQ(0, count);

   rc = db.createCollectionSpace(&session, "foo", 1, csOptions);
   ASSERT_EQ(SDB_OK, rc);
   rc = db.getCollectionSpaceCount(&session, count);
   ASSERT_EQ(SDB_OK, rc);
   ASSERT_EQ(1, count);

   rc = db.createCollectionSpace(&session, "foo", 2, csOptions);
   ASSERT_EQ(SDB_DMS_CS_EXIST, rc);

   rc = db.createCollectionSpace(&session, "foo1", 1, csOptions);
   ASSERT_EQ(SDB_DMS_CS_EXIST, rc);

   rc = db.testCollectionSpace(&session, "foo1", identifier);
   ASSERT_EQ(SDB_DMS_CS_NOTEXIST, rc);

   rc = db.createCollectionSpace(&session, "bar", 2, csOptions); 
   ASSERT_EQ(SDB_OK, rc);

   rc = db.getCollectionSpaceCount(&session, count);
   ASSERT_EQ(SDB_OK, rc);
   ASSERT_EQ(2, count);

   db.close(&session, closeDBOptions());
}

/*
Name: base_createCS_5
Description: 
   错误配置CS的创建
   提供错误名称、unique_id等配置信息创建CS
Input: 无
Output: 无
Expected Result: 
   创建CS失败
*/
TEST_F(cs_ddl_test, base_createCS_5)
{
   INT32 rc = SDB_OK;
   vesselImpl db;
   outerResource resource = test_outer_resource::getResource();
   test_executor session;
   openDBOptions options;
   createCSOptions csOptions;
   options.path.dataPath = DATA_PATH;
   options.path.indexPath = DATA_PATH;
   options.path.lobMetaPath = DATA_PATH;
   options.path.lobPath = DATA_PATH;
   options.path.lsmPath = LSM_PATH;

   rc = db.createCollectionSpace(&session, "foo", 1, csOptions);
   ASSERT_EQ(SDB_INVALIDARG, rc);

   rc = db.open(&session, &resource, options);
   ASSERT_EQ(SDB_OK, rc);

   rc = db.createCollectionSpace(&session, "", 1, csOptions);
   ASSERT_EQ(SDB_INVALIDARG, rc);

   csOptions.dataPageSize = DMS_PAGE_SIZE32K + 1;
   rc = db.createCollectionSpace(&session, "foo", 1, csOptions);
   ASSERT_EQ(SDB_INVALIDARG, rc);

   csOptions.dataPageSize = DMS_PAGE_SIZE32K;
   csOptions.dataSegSize = STORAGE_FILE_SEGMENT_SIZE_32MB + 1;
   rc = db.createCollectionSpace(&session, "foo", 1, csOptions);
   ASSERT_EQ(SDB_INVALIDARG, rc);

   csOptions.dataSegSize = STORAGE_FILE_SEGMENT_SIZE_32MB;
   csOptions.idxPageSize = DMS_PAGE_SIZE32K + 1;
   rc = db.createCollectionSpace(&session, "foo", 1, csOptions);
   ASSERT_EQ(SDB_INVALIDARG, rc);

   csOptions.idxPageSize = DMS_PAGE_SIZE32K;
   csOptions.idxSegSize = STORAGE_FILE_SEGMENT_SIZE_32MB + 1;
   rc = db.createCollectionSpace(&session, "foo", 1, csOptions);
   ASSERT_EQ(SDB_INVALIDARG, rc);

   csOptions.idxSegSize = STORAGE_FILE_SEGMENT_SIZE_32MB;
   csOptions.lobPageSize = DMS_PAGE_SIZE256K + 1;
   rc = db.createCollectionSpace(&session, "foo", 1, csOptions);
   ASSERT_EQ(SDB_INVALIDARG, rc);

   csOptions.lobPageSize = DMS_PAGE_SIZE256K;
   csOptions.lobSegSize = STORAGE_FILE_SEGMENT_SIZE_128MB + 1;
   rc = db.createCollectionSpace(&session, "foo", 1, csOptions);
   ASSERT_EQ(SDB_INVALIDARG, rc);

   db.close(&session, closeDBOptions());
}

/*
Name: base_listCS_1
Description: 
   列举所有CS
   1. 创建CS
   2. 列举所有CS并校验内容是否正确
Input: 无
Output: 无
Expected Result: 
   列举出的CS信息均正确
*/
TEST_F(cs_ddl_test, base_listCS_1)
{
   INT32 rc = SDB_OK;
   vesselImpl db;
   outerResource resource = test_outer_resource::getResource();
   test_executor session;
   openDBOptions options;
   createCSOptions csOptions;
   options.path.dataPath = DATA_PATH;
   options.path.indexPath = DATA_PATH;
   options.path.lobMetaPath = DATA_PATH;
   options.path.lobPath = DATA_PATH;
   options.path.lsmPath = LSM_PATH;

   slice content;
   bson::BSONObj record;
   cursorHandler c;

   rc = db.open(&session, &resource, options);
   ASSERT_EQ(SDB_OK, rc);

   rc = db.listCollectionSpace(&session, NULL, c);
   ASSERT_EQ(SDB_OK, rc);

   rc = c.getNext(&session, content);
   ASSERT_EQ(SDB_VESSEL_EOC, rc);
   c.close();

   rc = db.createCollectionSpace(&session, "foo1", 1, csOptions);
   ASSERT_EQ(SDB_OK, rc);

   rc = db.createCollectionSpace(&session, "foo2", 2, csOptions);
   ASSERT_EQ(SDB_OK, rc);

   rc = db.createCollectionSpace(&session, "foo3", 3, csOptions);
   ASSERT_EQ(SDB_OK, rc);

   rc = db.listCollectionSpace(&session, NULL, c);
   ASSERT_EQ(SDB_OK, rc);

   rc = c.getNext(&session, content);
   ASSERT_EQ(SDB_OK, rc);
   ASSERT_TRUE(content.isValid());
   record = bson::BSONObj(content.data());
   ASSERT_EQ(0, ossStrcmp("foo1", record.getStringField(CS_DUMP_RECORD_FIELD_NAME)));
   ASSERT_EQ(1, record.getIntField(CS_DUMP_RECORD_FIELD_UNIQUE_ID));

   rc = c.getNext(&session, content);
   ASSERT_EQ(SDB_OK, rc);
   ASSERT_TRUE(content.isValid());
   record = bson::BSONObj(content.data());
   ASSERT_EQ(0, ossStrcmp("foo2", record.getStringField(CS_DUMP_RECORD_FIELD_NAME)));
   ASSERT_EQ(2, record.getIntField(CS_DUMP_RECORD_FIELD_UNIQUE_ID));

   rc = c.getNext(&session, content);
   ASSERT_EQ(SDB_OK, rc);
   ASSERT_TRUE(content.isValid());
   record = bson::BSONObj(content.data());
   ASSERT_EQ(0, ossStrcmp("foo3", record.getStringField(CS_DUMP_RECORD_FIELD_NAME)));
   ASSERT_EQ(3, record.getIntField(CS_DUMP_RECORD_FIELD_UNIQUE_ID));

   rc = c.getNext(&session, content);
   ASSERT_EQ(SDB_VESSEL_EOC, rc);
   c.close();
   
   db.close(&session, closeDBOptions());
}

/*
Name: base_getCount_1
Description: 
   获取CS个数
   1. 没有CS判断CS数量是否为0
   1. 创建CS
   2. 创建后判断CS数量是否正确
Input: 无
Output: 无
Expected Result: 
   列举出的CS信息均正确
*/
TEST_F(cs_ddl_test, base_getCount_1)
{
   INT32 rc = SDB_OK;
   vesselImpl db;
   outerResource resource = test_outer_resource::getResource();
   test_executor session;
   openDBOptions options;
   createCSOptions csOptions;
   options.path.dataPath = DATA_PATH;
   options.path.indexPath = DATA_PATH;
   options.path.lobMetaPath = DATA_PATH;
   options.path.lobPath = DATA_PATH;
   options.path.lsmPath = LSM_PATH;

   UINT32 count = 0;

   rc = db.getCollectionSpaceCount(&session, count);
   ASSERT_EQ(SDB_INVALIDARG, rc);
   ASSERT_EQ(0, count);

   rc = db.open(&session, &resource, options);
   ASSERT_EQ(SDB_OK, rc);

   rc = db.getCollectionSpaceCount(&session, count);
   ASSERT_EQ(SDB_OK, rc);
   ASSERT_EQ(0, count);

   rc = db.createCollectionSpace(&session, "foo", 1, csOptions);
   ASSERT_EQ(SDB_OK, rc);

   rc = db.getCollectionSpaceCount(&session, count);
   ASSERT_EQ(SDB_OK, rc);
   ASSERT_EQ(1, count);

   db.close(&session, closeDBOptions());
}

// threads create test entry function
void thread_create(vesselImpl *db, atomic<UINT32> &createdCount, UINT32 sum)
{
   INT32 rc = SDB_OK;
   test_executor session;
   createCSOptions options;
   for (;;)
   {  
      UINT32 tmpCount = createdCount.fetch_add(1);
      if (tmpCount >= sum)
      {
         break;
      }
      CHAR buf[6] = {0};
      ossSnprintf(buf, 6, "%d", tmpCount);
      rc = db->createCollectionSpace(&session, buf, tmpCount, options);
      if (SDB_OK == rc)
      {
         continue;
      }
      // When expected error SDB_NOSPC occurred, 
      // stop creating and continue testing. 
      else if (SDB_NOSPC == rc)
      {
         rc = SDB_OK;
         break;
      }
      else
      {
         ASSERT_EQ(SDB_NOSPC, rc);
      }
   }
}

void thread_confilix_create(vesselImpl *db, UINT32 sum)
{
   INT32 rc = SDB_OK;
   test_executor session;
   createCSOptions options;
   for (UINT32 i = 0; i < sum; ++i)
   {  
      CHAR buf[6] = {0};
      ossSnprintf(buf, 6, "%d", i);
      rc = db->createCollectionSpace(&session, buf, i, options);
      if (SDB_OK == rc)
      {
         continue;
      }
      // When expected error SDB_DMS_CS_EXIST occurred, 
      // stop creating and continue testing. 
      else if (SDB_DMS_CS_EXIST == rc)
      {
         rc = SDB_OK;
         break;
      }
      // When expected error SDB_NOSPC occurred, 
      // stop creating and continue testing. 
      else if (SDB_NOSPC == rc)
      {
         rc = SDB_OK;
         break;
      }
      else
      {
         ASSERT_EQ(SDB_NOSPC, rc);
      }
   }
}

/*
Name: advanced_createCS_1
Description: 
   多线程创建CS
   1. 启动多线程创建CS
   2. 结束后校验创建的CS个数是否符合预期
Input: 无
Output: 无
Expected Result: 
   CS创建数量符合预期，不会重复创建
*/
TEST_F(cs_ddl_test, advanced_createCS_1)
{
   INT32 rc = SDB_OK;
   vesselImpl db;
   outerResource resource = test_outer_resource::getResource();
   test_executor session;
   openDBOptions options;
   createCSOptions csOptions;
   collectionSpaceId identifier;
   options.path.dataPath = DATA_PATH;
   options.path.indexPath = DATA_PATH;
   options.path.lobMetaPath = DATA_PATH;
   options.path.lobPath = DATA_PATH;
   options.path.lsmPath = LSM_PATH;

   atomic<UINT32> createdCount(0);
   UINT32 createNum = 32;
   UINT32 count = 0;

   UINT32 threadCount = 4;
   thread threads[threadCount];


   slice content;
   bson::BSONObj record;
   cursorHandler c;

   rc = db.open(&session, &resource, options);
   ASSERT_EQ(SDB_OK, rc);

   rc = db.getCollectionSpaceCount(&session, count);
   ASSERT_EQ(SDB_OK, rc);
   ASSERT_EQ(0, count);

   for (UINT32 i = 0; i < threadCount; ++i)
   {
      threads[i] = std::move(std::thread(thread_create, 
                                         &db, ref(createdCount), createNum));
   }
   for (UINT32 i = 0; i < threadCount; ++i)
   {
      threads[i].join();
   }

   rc = db.getCollectionSpaceCount(&session, count);
   ASSERT_EQ(SDB_OK, rc);
   ASSERT_EQ(createNum, count);

   db.close(&session, closeDBOptions());
}

/*
Name: advanced_createCS_2
Description: 
   多线程冲突创建CS
   1. 启动多线程创建CS
   2. 会出现重复创建问题，CS已存在
Input: 无
Output: 无
Expected Result: 
   CS创建个数符合预期，不会重复创建
*/
TEST_F(cs_ddl_test, advanced_createCS_2)
{
   INT32 rc = SDB_OK;
   vesselImpl db;
   outerResource resource = test_outer_resource::getResource();
   test_executor session;
   openDBOptions options;
   createCSOptions csOptions;
   collectionSpaceId identifier;
   options.path.dataPath = DATA_PATH;
   options.path.indexPath = DATA_PATH;
   options.path.lobMetaPath = DATA_PATH;
   options.path.lobPath = DATA_PATH;
   options.path.lsmPath = LSM_PATH;

   UINT32 createNum = 32;
   UINT32 count = 0;

   UINT32 threadCount = 4;
   thread threads[threadCount];


   slice content;
   bson::BSONObj record;
   cursorHandler c;

   rc = db.open(&session, &resource, options);
   ASSERT_EQ(SDB_OK, rc);

   rc = db.getCollectionSpaceCount(&session, count);
   ASSERT_EQ(SDB_OK, rc);
   ASSERT_EQ(0, count);

   for (UINT32 i = 0; i < threadCount; ++i)
   {
      threads[i] = std::move(std::thread(thread_confilix_create, 
                                         &db, createNum));
   }
   for (UINT32 i = 0; i < threadCount; ++i)
   {
      threads[i].join();
   }

   rc = db.getCollectionSpaceCount(&session, count);
   ASSERT_EQ(SDB_OK, rc);
   ASSERT_EQ(createNum, count);

   db.close(&session, closeDBOptions());
}

/*
Name: death_createCS_1
Description: 
   单线程创建最大数量CS
Input: 无
Output: 无
Expected Result: 
   1. CS创建成功
   2. CS计数正确
*/
TEST_F(cs_ddl_test, death_createCS_1)
{
   INT32 rc = SDB_OK;
   outerResource resource = test_outer_resource::getResource();
   test_executor session;
   vesselImpl db;
   openDBOptions options;
   createCSOptions csOptions;
   options.path.dataPath = DATA_PATH;
   options.path.indexPath = DATA_PATH;
   options.path.lobMetaPath = DATA_PATH;
   options.path.lobPath = DATA_PATH;
   options.path.lsmPath = LSM_PATH;

   UINT32 count = 0;
   UINT32 createdCount = 0;

   rc = db.open(&session, &resource, options);
   ASSERT_EQ(SDB_OK, rc);

   for (UINT32 i = 0; i < MAX_SU_COUNT; ++i)
   {
      CHAR buf[6] = {0};
      ossSnprintf(buf, 6, "%d", i);

      rc = db.createCollectionSpace(&session, buf, i, csOptions);
      if (SDB_OK == rc)
      {
         ++createdCount;
         continue;
      }
      // When expected error SDB_NOSPC occurred, 
      // stop creating and continue testing. 
      else if (SDB_NOSPC == rc) 
      {
         rc = SDB_OK;
         break;
      }
      else
      {
         ASSERT_EQ(rc, SDB_NOSPC);
      }
   }

   if (MAX_SU_COUNT == createdCount)
   {
      CHAR buf[6] = {0};
      ossSnprintf(buf, 6, "%d", MAX_SU_COUNT);
      rc = db.createCollectionSpace(&session, buf, MAX_SU_COUNT, csOptions);
      ASSERT_EQ(SDB_DMS_SU_OUTRANGE, rc);
   }

   db.close(&session, closeDBOptions());

   rc = db.open(&session, &resource, options);
   ASSERT_EQ(SDB_OK, rc);

   rc = db.getCollectionSpaceCount(&session, count);
   ASSERT_EQ(SDB_OK, rc);
   ASSERT_EQ(count, createdCount);

   db.close(&session, closeDBOptions());
}

// /*
// Name: death_createCS_2
// Description: 
//    多线程创建最大数量CS
// Input: 无
// Output: 无
// Expected Result: 
// */
// TEST_F(cs_ddl_test, death_createCS_2)
// {
//    INT32 rc = SDB_OK;
//    vesselImpl db;
//    outerResource resource = test_outer_resource::getResource();
//    test_executor session;
//    openDBOptions options;
//    createCSOptions csOptions;
//    options.path.dataPath = DATA_PATH;
//    options.path.indexPath = DATA_PATH;
//    options.path.lobMetaPath = DATA_PATH;
//    options.path.lobPath = DATA_PATH;
//    options.path.lsmPath = LSM_PATH;

//    UINT32 createNum = MAX_SU_COUNT;
//    UINT32 count = 0;
//    atomic<UINT32> createdCount(0);

//    UINT32 threadCount = 4;
//    thread threads[threadCount];
   
//    rc = db.open(&session, &resource, options);
//    ASSERT_EQ(SDB_OK, rc);
   
//    for (UINT32 i = 0; i < threadCount; ++i)
//    {
//       threads[i] = std::move(std::thread(thread_create, 
//                                          &db, ref(createdCount), createNum));
//    }
//    for (UINT32 i = 0; i < threadCount; ++i)
//    {
//       threads[i].join();
//    }

//    if (MAX_SU_COUNT == createdCount)
//    {
//       CHAR buf[6] = {0};
//       ossSnprintf(buf, 6, "%d", MAX_SU_COUNT);
//       rc = db.createCollectionSpace(&session, buf, MAX_SU_COUNT, csOptions);
//       ASSERT_EQ(SDB_DMS_SU_OUTRANGE, rc);
//    }
//    db.close(&session, closeDBOptions());

//    rc = db.open(&session, &resource, options);
//    ASSERT_EQ(SDB_OK, rc);

//    rc = db.getCollectionSpaceCount(&session, count);
//    ASSERT_EQ(SDB_OK, rc);
//    ASSERT_EQ(count, createdCount);

//    db.close(&session, closeDBOptions());
// }
