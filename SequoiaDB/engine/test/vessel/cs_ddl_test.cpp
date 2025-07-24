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

   Source File Name = cs_ddl_test.cpp

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
#include <atomic>
#include <gtest/gtest.h>
#include "ossUtil.hpp"
#include "dpsLogRecord.hpp"
#include "vessel/listCollectionSpaceDef.h"
#include "vessel/listCollectionsDef.h"
#include "dpsLogRecord.hpp"

#include <boost/filesystem.hpp>
#include <thread>
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
    
   options.path.dataPath = DATA_PATH;
   options.path.indexPath = DATA_PATH;
   options.path.lobmPath = DATA_PATH;
   options.path.lobdPath = DATA_PATH;
   options.path.lsmPath = LSM_PATH;

   utilCSUniqueID uniqueId = UTIL_UNIQUEID_NULL;
   bson::BSONObj adjunct;
   
   DATA_CURSOR_PTR cursor;

   UINT32 count = 0;

   rc = db.open(&session, &resource, options);
   ASSERT_EQ(SDB_OK, rc);

   rc = db.createCS(&session, "foo", 1, dmsCreateCSOptions(), adjunct);
   ASSERT_EQ(SDB_OK, rc);

   // test if the collection space was created successfully
   rc = db.testCS(&session, "foo", uniqueId);
   ASSERT_EQ(SDB_OK, rc);
   ASSERT_EQ(1, uniqueId);

   // test collection space count
   rc = db.getCSCount(&session, count);
   ASSERT_EQ(SDB_OK, rc);
   ASSERT_EQ(1, count);

   rc = db.listCS(&session, cursor);
   ASSERT_EQ(SDB_OK, rc);

   // test the collection space's name and unique_id
   rc = cursor->fetchNext(&session);
   ASSERT_EQ(SDB_OK, rc);
   ASSERT_EQ(0, ossStrcmp("foo", 
                          cursor->getBsonRecord().getStringField(CS_DUMP_RECORD_FIELD_NAME)));
   ASSERT_EQ(1, cursor->getBsonRecord().getIntField(CS_DUMP_RECORD_FIELD_UNIQUE_ID));

   rc = cursor->fetchNext(&session);
   ASSERT_EQ(SDB_DMS_EOC, rc);
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
TEST_F(cs_ddl_test, base_createCS_2)
{
   INT32 rc = SDB_OK;
   outerResource resource = test_outer_resource::getResource();
   test_executor session;
   vesselImpl db;
   openDBOptions options;
    
   options.path.dataPath = DATA_PATH;
   options.path.indexPath = DATA_PATH;
   options.path.lobmPath = DATA_PATH;
   options.path.lobdPath = DATA_PATH;
   options.path.lsmPath = LSM_PATH;

   utilCSUniqueID uniqueId = UTIL_UNIQUEID_NULL;
   bson::BSONObj adjunct;
   
   DATA_CURSOR_PTR cursor;
   UINT32 count = 0;

   rc = db.open(&session, &resource, options);
   ASSERT_EQ(SDB_OK, rc);

   rc = db.createCS(&session, "foo", 1, dmsCreateCSOptions(), adjunct);
   ASSERT_EQ(SDB_OK, rc);

   // reopen db
   db.close(&session, closeDBOptions());
   rc = db.open(&session, &resource, options);
   ASSERT_EQ(SDB_OK, rc);

   // test if the collection space was created successfully
   rc = db.testCS(&session, "foo", uniqueId);
   ASSERT_EQ(SDB_OK, rc);
   ASSERT_EQ(1, uniqueId);

   // test collection space count
   rc = db.getCSCount(&session, count);
   ASSERT_EQ(SDB_OK, rc);
   ASSERT_EQ(1, count);

   rc = db.listCS(&session, cursor);
   ASSERT_EQ(SDB_OK, rc);

   // test the collection space's name and unique_id
   rc = cursor->fetchNext(&session);
   ASSERT_EQ(SDB_OK, rc);
   ASSERT_EQ(0, ossStrcmp("foo", 
                          cursor->getBsonRecord().getStringField(CS_DUMP_RECORD_FIELD_NAME)));
   ASSERT_EQ(1, cursor->getBsonRecord().getIntField(CS_DUMP_RECORD_FIELD_UNIQUE_ID));

   rc = cursor->fetchNext(&session);
   ASSERT_EQ(SDB_DMS_EOC, rc);
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
    
   options.path.dataPath = DATA_PATH;
   options.path.indexPath = DATA_PATH;
   options.path.lobmPath = DATA_PATH;
   options.path.lobdPath = DATA_PATH;
   options.path.lsmPath = LSM_PATH;

   bson::BSONObj adjunct;
   
   DATA_CURSOR_PTR cursor;
   UINT32 count = 0;

   rc = db.open(&session, &resource, options);
   ASSERT_EQ(SDB_OK, rc);

   rc = db.createCS(&session, "foo1", 1, dmsCreateCSOptions(), adjunct);
   ASSERT_EQ(SDB_OK, rc);

   rc = db.createCS(&session, "foo2", 2, dmsCreateCSOptions(), adjunct);
   ASSERT_EQ(SDB_OK, rc);

   rc = db.createCS(&session, "foo3", 3, dmsCreateCSOptions(), adjunct);
   ASSERT_EQ(SDB_OK, rc);

   rc = db.getCSCount(&session, count);
   ASSERT_EQ(SDB_OK, rc);
   ASSERT_EQ(3, count);

   rc = db.listCS(&session, cursor);
   ASSERT_EQ(SDB_OK, rc);

   rc = cursor->fetchNext(&session);
   ASSERT_EQ(SDB_OK, rc);
   ASSERT_EQ(0, ossStrcmp("foo1", 
                          cursor->getBsonRecord().getStringField(CS_DUMP_RECORD_FIELD_NAME)));
   ASSERT_EQ(1, cursor->getBsonRecord().getIntField(CS_DUMP_RECORD_FIELD_UNIQUE_ID));

   rc = cursor->fetchNext(&session);
   ASSERT_EQ(SDB_OK, rc);
   ASSERT_EQ(0, ossStrcmp("foo2", 
                          cursor->getBsonRecord().getStringField(CS_DUMP_RECORD_FIELD_NAME)));
   ASSERT_EQ(2, cursor->getBsonRecord().getIntField(CS_DUMP_RECORD_FIELD_UNIQUE_ID));

   rc = cursor->fetchNext(&session);
   ASSERT_EQ(SDB_OK, rc);
   ASSERT_EQ(0, ossStrcmp("foo3", 
                          cursor->getBsonRecord().getStringField(CS_DUMP_RECORD_FIELD_NAME)));
   ASSERT_EQ(3, cursor->getBsonRecord().getIntField(CS_DUMP_RECORD_FIELD_UNIQUE_ID));

   rc = cursor->fetchNext(&session);
   ASSERT_EQ(SDB_DMS_EOC, rc);

   db.close(&session, closeDBOptions());

   rc = db.open(&session, &resource, options);
   ASSERT_EQ(SDB_OK, rc);
   rc = db.getCSCount(&session, count);
   ASSERT_EQ(SDB_OK, rc);
   ASSERT_EQ(3, count);

   rc = db.listCS(&session, cursor);
   ASSERT_EQ(SDB_OK, rc);

   
   rc = cursor->fetchNext(&session);
   ASSERT_EQ(SDB_OK, rc);
   ASSERT_EQ(0, ossStrcmp("foo1", 
                          cursor->getBsonRecord().getStringField(CS_DUMP_RECORD_FIELD_NAME)));
   ASSERT_EQ(1, cursor->getBsonRecord().getIntField(CS_DUMP_RECORD_FIELD_UNIQUE_ID));

   rc = cursor->fetchNext(&session);
   ASSERT_EQ(SDB_OK, rc);
   ASSERT_EQ(0, ossStrcmp("foo2", 
                          cursor->getBsonRecord().getStringField(CS_DUMP_RECORD_FIELD_NAME)));
   ASSERT_EQ(2, cursor->getBsonRecord().getIntField(CS_DUMP_RECORD_FIELD_UNIQUE_ID));

   rc = cursor->fetchNext(&session);
   ASSERT_EQ(SDB_OK, rc);
   ASSERT_EQ(0, ossStrcmp("foo3", 
                          cursor->getBsonRecord().getStringField(CS_DUMP_RECORD_FIELD_NAME)));
   ASSERT_EQ(3, cursor->getBsonRecord().getIntField(CS_DUMP_RECORD_FIELD_UNIQUE_ID));

   rc = cursor->fetchNext(&session);
   ASSERT_EQ(SDB_DMS_EOC, rc);

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
    
   collectionSpaceId identifier;
   options.path.dataPath = DATA_PATH;
   options.path.indexPath = DATA_PATH;
   options.path.lobmPath = DATA_PATH;
   options.path.lobdPath = DATA_PATH;
   options.path.lsmPath = LSM_PATH;

   utilCSUniqueID uniqueId = UTIL_UNIQUEID_NULL;
   bson::BSONObj adjunct;
   UINT32 count = 0;

   rc = db.open(&session, &resource, options);
   ASSERT_EQ(SDB_OK, rc);

   rc = db.createCS(&session, "foo", 1, dmsCreateCSOptions(), adjunct);
   ASSERT_EQ(SDB_OK, rc);
   rc = db.getCSCount(&session, count);
   ASSERT_EQ(SDB_OK, rc);
   ASSERT_EQ(1, count);

   rc = db.createCS(&session, "foo", 2, dmsCreateCSOptions(), adjunct);
   ASSERT_EQ(SDB_DMS_CS_EXIST, rc);

   rc = db.createCS(&session, "foo1", 1, dmsCreateCSOptions(), adjunct);
   ASSERT_EQ(SDB_DMS_CS_EXIST, rc);

   rc = db.testCS(&session, "foo1", uniqueId);
   ASSERT_EQ(SDB_DMS_CS_NOTEXIST, rc);

   rc = db.createCS(&session, "bar", 2, dmsCreateCSOptions(), adjunct); 
   ASSERT_EQ(SDB_OK, rc);

   rc = db.getCSCount(&session, count);
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
    
   options.path.dataPath = DATA_PATH;
   options.path.indexPath = DATA_PATH;
   options.path.lobmPath = DATA_PATH;
   options.path.lobdPath = DATA_PATH;
   options.path.lsmPath = LSM_PATH;
   
   bson::BSONObj adjunct;
   dmsCreateCSOptions invalidOptions;
   invalidOptions.dataPageSize = DMS_PAGE_SIZE64K + 1;

   rc = db.open(&session, &resource, options);
   ASSERT_EQ(SDB_OK, rc);

   rc = db.createCS(&session, "foo", 1, invalidOptions, adjunct);
   ASSERT_EQ(SDB_INVALIDARG, rc);

   invalidOptions.dataPageSize = DMS_PAGE_SIZE256K;
   rc = db.createCS(&session, "foo", 1, invalidOptions, adjunct);
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
    
   options.path.dataPath = DATA_PATH;
   options.path.indexPath = DATA_PATH;
   options.path.lobmPath = DATA_PATH;
   options.path.lobdPath = DATA_PATH;
   options.path.lsmPath = LSM_PATH;

   DATA_CURSOR_PTR cursor;
   
   bson::BSONObj adjunct;

   rc = db.open(&session, &resource, options);
   ASSERT_EQ(SDB_OK, rc);

   rc = db.listCS(&session, cursor);
   ASSERT_EQ(SDB_OK, rc);

   
   rc = cursor->fetchNext(&session);
   ASSERT_EQ(SDB_DMS_EOC, rc);

   rc = db.createCS(&session, "foo1", 1, dmsCreateCSOptions(), adjunct);
   ASSERT_EQ(SDB_OK, rc);

   rc = db.createCS(&session, "foo2", 2, dmsCreateCSOptions(), adjunct);
   ASSERT_EQ(SDB_OK, rc);

   rc = db.createCS(&session, "foo3", 3, dmsCreateCSOptions(), adjunct);
   ASSERT_EQ(SDB_OK, rc);

   rc = db.listCS(&session, cursor);
   ASSERT_EQ(SDB_OK, rc);

   
   rc = cursor->fetchNext(&session);
   ASSERT_EQ(SDB_OK, rc);
   ASSERT_EQ(0, ossStrcmp("foo1", cursor->getBsonRecord().getStringField(CS_DUMP_RECORD_FIELD_NAME)));
   ASSERT_EQ(1, cursor->getBsonRecord().getIntField(CS_DUMP_RECORD_FIELD_UNIQUE_ID));

   rc = cursor->fetchNext(&session);
   ASSERT_EQ(SDB_OK, rc);
   ASSERT_EQ(0, ossStrcmp("foo2", cursor->getBsonRecord().getStringField(CS_DUMP_RECORD_FIELD_NAME)));
   ASSERT_EQ(2, cursor->getBsonRecord().getIntField(CS_DUMP_RECORD_FIELD_UNIQUE_ID));

   rc = cursor->fetchNext(&session);
   ASSERT_EQ(SDB_OK, rc);
   ASSERT_EQ(0, ossStrcmp("foo3", cursor->getBsonRecord().getStringField(CS_DUMP_RECORD_FIELD_NAME)));
   ASSERT_EQ(3, cursor->getBsonRecord().getIntField(CS_DUMP_RECORD_FIELD_UNIQUE_ID));

   rc = cursor->fetchNext(&session);
   ASSERT_EQ(SDB_DMS_EOC, rc);
   
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
    
   options.path.dataPath = DATA_PATH;
   options.path.indexPath = DATA_PATH;
   options.path.lobmPath = DATA_PATH;
   options.path.lobdPath = DATA_PATH;
   options.path.lsmPath = LSM_PATH;

   UINT32 count = 0;

   rc = db.open(&session, &resource, options);
   ASSERT_EQ(SDB_OK, rc);

   rc = db.getCSCount(&session, count);
   ASSERT_EQ(SDB_OK, rc);
   ASSERT_EQ(0, count);

   rc = db.createCS(&session, "foo", 1, dmsCreateCSOptions(), bson::BSONObj());
   ASSERT_EQ(SDB_OK, rc);

   rc = db.getCSCount(&session, count);
   ASSERT_EQ(SDB_OK, rc);
   ASSERT_EQ(1, count);

   db.close(&session, closeDBOptions());
}

// threads create test entry function
void thread_create(vesselImpl *db, atomic<UINT32> &createdCount, UINT32 sum)
{
   INT32 rc = SDB_OK;
   test_executor session;
     
   collectionSpaceId identifier;

   for (;;)
   {  
      UINT32 tmpCount = createdCount.fetch_add(1);
      if (tmpCount >= sum)
      {
         break;
      }
      CHAR buf[6] = {0};
      ossSnprintf(buf, 6, "%d", tmpCount);
      rc = db->createCS(&session, buf, tmpCount, dmsCreateCSOptions(), bson::BSONObj());
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

void thread_interference_create(vesselImpl *db, UINT32 sum)
{
   INT32 rc = SDB_OK;
   test_executor session;
     
   collectionSpaceId identifier;

   for (UINT32 i = 0; i < sum; ++i)
   {  
      CHAR buf[6] = {0};
      ossSnprintf(buf, 6, "%d", i);
      rc = db->createCS(&session, buf, i, dmsCreateCSOptions(), bson::BSONObj());
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
    
   collectionSpaceId identifier;
   options.path.dataPath = DATA_PATH;
   options.path.indexPath = DATA_PATH;
   options.path.lobmPath = DATA_PATH;
   options.path.lobdPath = DATA_PATH;
   options.path.lsmPath = LSM_PATH;

   atomic<UINT32> createdCount(0);
   UINT32 createNum = 32;
   UINT32 count = 0;

   UINT32 threadCount = 4;
   thread threads[threadCount];


   rc = db.open(&session, &resource, options);
   ASSERT_EQ(SDB_OK, rc);

   for (UINT32 i = 0; i < threadCount; ++i)
   {
      threads[i] = std::move(std::thread(thread_create, 
                                         &db, ref(createdCount), createNum));
   }
   for (UINT32 i = 0; i < threadCount; ++i)
   {
      threads[i].join();
   }

   rc = db.getCSCount(&session, count);
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
    
   collectionSpaceId identifier;
   options.path.dataPath = DATA_PATH;
   options.path.indexPath = DATA_PATH;
   options.path.lobmPath = DATA_PATH;
   options.path.lobdPath = DATA_PATH;
   options.path.lsmPath = LSM_PATH;

   UINT32 createNum = 32;
   UINT32 count = 0;

   UINT32 threadCount = 4;
   thread threads[threadCount];

   rc = db.open(&session, &resource, options);
   ASSERT_EQ(SDB_OK, rc);

   for (UINT32 i = 0; i < threadCount; ++i)
   {
      threads[i] = std::move(std::thread(thread_interference_create, 
                                         &db, createNum));
   }
   for (UINT32 i = 0; i < threadCount; ++i)
   {
      threads[i].join();
   }

   rc = db.getCSCount(&session, count);
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
TEST_F(cs_ddl_test, DISABLED_death_createCS_1)
{
   INT32 rc = SDB_OK;
   outerResource resource = test_outer_resource::getResource();
   test_executor session;
   vesselImpl db;
   openDBOptions options;
    
   options.path.dataPath = DATA_PATH;
   options.path.indexPath = DATA_PATH;
   options.path.lobmPath = DATA_PATH;
   options.path.lobdPath = DATA_PATH;
   options.path.lsmPath = LSM_PATH;

   UINT32 count = 0;
   UINT32 createdCount = 0;

   collectionSpaceId identifier;

   rc = db.open(&session, &resource, options);
   ASSERT_EQ(SDB_OK, rc);

   for (UINT32 i = 0; i < MAX_SU_COUNT; ++i)
   {
      CHAR buf[6] = {0};
      ossSnprintf(buf, 6, "%d", i);

      rc = db.createCS(&session, buf, i, dmsCreateCSOptions(), bson::BSONObj());
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
      rc = db.createCS(&session, buf, MAX_SU_COUNT, dmsCreateCSOptions(), bson::BSONObj());
      ASSERT_EQ(SDB_DMS_SU_OUTRANGE, rc);
   }

   db.close(&session, closeDBOptions());

   rc = db.open(&session, &resource, options);
   ASSERT_EQ(SDB_OK, rc);

   rc = db.getCSCount(&session, count);
   ASSERT_EQ(SDB_OK, rc);
   ASSERT_EQ(count, createdCount);

   db.close(&session, closeDBOptions());
}

/*
Name: death_createCS_2
Description: 
   多线程创建最大数量CS
Input: 无
Output: 无
Expected Result: 
*/
TEST_F(cs_ddl_test, DISABLED_death_createCS_2)
{
   INT32 rc = SDB_OK;
   vesselImpl db;
   outerResource resource = test_outer_resource::getResource();
   test_executor session;
   openDBOptions options;
    
   options.path.dataPath = DATA_PATH;
   options.path.indexPath = DATA_PATH;
   options.path.lobmPath = DATA_PATH;
   options.path.lobdPath = DATA_PATH;
   options.path.lsmPath = LSM_PATH;

   UINT32 createNum = MAX_SU_COUNT;
   UINT32 count = 0;
   atomic<UINT32> createdCount(0);

   UINT32 threadCount = 4;
   thread threads[threadCount];

   collectionSpaceId identifier;
   
   rc = db.open(&session, &resource, options);
   ASSERT_EQ(SDB_OK, rc);
   
   for (UINT32 i = 0; i < threadCount; ++i)
   {
      threads[i] = std::move(std::thread(thread_create, 
                                         &db, ref(createdCount), createNum));
   }
   for (UINT32 i = 0; i < threadCount; ++i)
   {
      threads[i].join();
   }

   if (MAX_SU_COUNT == createdCount)
   {
      CHAR buf[6] = {0};
      ossSnprintf(buf, 6, "%d", MAX_SU_COUNT);
      rc = db.createCS(&session, buf, MAX_SU_COUNT, dmsCreateCSOptions(), bson::BSONObj());
      ASSERT_EQ(SDB_DMS_SU_OUTRANGE, rc);
   }
   db.close(&session, closeDBOptions());

   rc = db.open(&session, &resource, options);
   ASSERT_EQ(SDB_OK, rc);

   rc = db.getCSCount(&session, count);
   ASSERT_EQ(SDB_OK, rc);
   ASSERT_EQ(count, createdCount);

   db.close(&session, closeDBOptions());
}

/*
Name: base_removeCS_1
Description: 
   多个CS的依次删除
   1. 创建3个CS
   2. 校验CS总数个数是否为3
   3. 删除1个CS
   4. 校验CS总数是否为2
   5. 校验剩余CS名称是否符合预期
   6. 删除剩余所有CS，校验CS总数是否为0
Input: 无
Output: 无
Expected Result: 
   成功创建CS后删除CS，且过程中CS符合预期
*/
TEST_F(cs_ddl_test, base_removeCS_1)
{
   INT32 rc = SDB_OK;
   outerResource resource = test_outer_resource::getResource();
   test_executor session;
   vesselImpl db;
   openDBOptions options;

   options.path.dataPath = DATA_PATH;
   options.path.indexPath = DATA_PATH;
   options.path.lobmPath = DATA_PATH;
   options.path.lobdPath = DATA_PATH;
   options.path.lsmPath = LSM_PATH;

   utilCSUniqueID uniqueId = UTIL_UNIQUEID_NULL;
   bson::BSONObj adjunct;
   UINT32 count = 0;
   DATA_CURSOR_PTR cursor;
   

   rc = db.open(&session, &resource, options);
   ASSERT_EQ(SDB_OK, rc);

   std::vector<std::string> csNames{"foo1", "foo2", "foo3"};

   for (UINT32 i = 0; i < csNames.size(); ++i)
   {
      rc = db.createCS(&session, csNames[i].c_str(), i + 1, dmsCreateCSOptions(), adjunct);
      ASSERT_EQ(SDB_OK, rc);
      rc = db.testCS(&session, csNames[i].c_str(), uniqueId);
      ASSERT_EQ(SDB_OK, rc);
      ASSERT_EQ(i + 1, uniqueId);
   }

   rc = db.removeCS(&session, "foo1");
   ASSERT_EQ(SDB_OK, rc);
   rc = db.getCSCount(&session, count);
   ASSERT_EQ(SDB_OK, rc);
   ASSERT_EQ(2, count);

   rc = db.listCS(&session, cursor);
   ASSERT_EQ(SDB_OK, rc);

   
   rc = cursor->fetchNext(&session);
   ASSERT_EQ(SDB_OK, rc);
   ASSERT_EQ(0, ossStrcmp("foo2", cursor->getBsonRecord().getStringField(CS_DUMP_RECORD_FIELD_NAME)));

   rc = cursor->fetchNext(&session);
   ASSERT_EQ(SDB_OK, rc);
   ASSERT_EQ(0, ossStrcmp("foo3", cursor->getBsonRecord().getStringField(CS_DUMP_RECORD_FIELD_NAME)));

   rc = db.removeCS(&session, "foo2");
   ASSERT_EQ(SDB_OK, rc);
   rc = db.removeCS(&session, "foo3");
   ASSERT_EQ(SDB_OK, rc);

   rc = db.getCSCount(&session, count);
   ASSERT_EQ(SDB_OK, rc);
   ASSERT_EQ(0, count);
}

/*
Name: base_removeCS_2
Description:
   创建CS、CL、插入记录后删除cs
   1. 创建单个CS，CS上创建单个CL
   2. 在CL上插入10000条记录
   3. 删除CS
Input: 无
Output: 无
Expected Result: 
   成功创建CS、创建CL、CL中插入记录、删除CS
*/
TEST_F(cs_ddl_test, base_removeCS_2)
{
   INT32 rc = SDB_OK;
   outerResource resource = test_outer_resource::getResource();
   test_executor session;
   vesselImpl db;
   openDBOptions options;

   options.path.dataPath = DATA_PATH;
   options.path.indexPath = DATA_PATH;
   options.path.lobmPath = DATA_PATH;
   options.path.lobdPath = DATA_PATH;
   options.path.lsmPath = LSM_PATH;

   bson::BSONObj adjunct;
   DATA_CURSOR_PTR cursor;
   
   DATA_COLLECTION_PTR handler;
   UINT32 count = 10000;
   bson::BSONObjBuilder builder;
   utilInsertResult res;

   rc = db.open(&session, &resource, options);
   ASSERT_EQ(SDB_OK, rc);

   rc = db.createCS(&session, "foo", 1, dmsCreateCSOptions(), adjunct);
   ASSERT_EQ(SDB_OK, rc);

   rc = db.createCL(&session, "foo.bar", 1, dmsCreateCLOptions(), bson::BSONObj());
   ASSERT_EQ(SDB_OK, rc);

   rc = db.openCL(&session, "foo.bar", dmsOpenCLOptions(), handler);
   ASSERT_EQ(SDB_OK, rc);
   
   for (UINT32 i = 0; i < count; ++i)
   {
      builder.reset();
      builder.append("a", i);
      bson::BSONObj obj = builder.obj();
      rc = handler->insertRecord(&session, obj, dmsInsertRecordOptions(), &res);
      ASSERT_EQ(SDB_OK, rc);
   }

   rc = db.removeCS(&session, "foo");
   ASSERT_EQ(SDB_OK, rc);
}

/*
Name: base_removeCS_3
Description:
   创建CS、CL、插入lob chunk后删除CS
   1. 创建单个CS，CS上创建单个CL
   2. 在CL上插入lob chunk
   3. 删除CS
Input: 无
Output: 无
Expected Result: 
   成功创建CS、创建CL、CL中插入lob chunk、删除CS
*/
TEST_F(cs_ddl_test, base_removeCS_3)
{
   INT32 rc = SDB_OK;
   outerResource resource = test_outer_resource::getResource();
   test_executor session;
   vesselImpl db;
   openDBOptions options;

   options.path.dataPath = DATA_PATH;
   options.path.indexPath = DATA_PATH;
   options.path.lobmPath = DATA_PATH;
   options.path.lobdPath = DATA_PATH;
   options.path.lsmPath = LSM_PATH;

   bson::BSONObj adjunct;
   DATA_CURSOR_PTR cursor;
   
   DATA_COLLECTION_PTR handler;
   constexpr UINT32 count = 1024;
   constexpr UINT32 LOBC_SIZE = 1024 * 1024;
   CHAR buf[LOBC_SIZE];

   rc = db.open(&session, &resource, options);
   ASSERT_EQ(SDB_OK, rc);

   rc = db.createCS(&session, "foo", 1, dmsCreateCSOptions(), adjunct);
   ASSERT_EQ(SDB_OK, rc);

   rc = db.createCL(&session, "foo.bar", 1, dmsCreateCLOptions(), bson::BSONObj());
   ASSERT_EQ(SDB_OK, rc);

   rc = db.openCL(&session, "foo.bar", dmsOpenCLOptions(), handler);
   ASSERT_EQ(SDB_OK, rc);
   
   for (UINT32 i = 0; i < count; ++i)
   {
      bson::OID oid;
      oid.init();
      rc = handler->insertLobChunk(&session, oid, 0, 0, LOBC_SIZE, buf);
      ASSERT_EQ(SDB_OK, rc);
   }

   rc = db.removeCS(&session, "foo");
   ASSERT_EQ(SDB_OK, rc);
}

/*
Name: base_removeCS_4
Description:
   创建CS、CL，启动一个线程插入lob chunk，启动另一个线程在中途删除CS
   1. 创建单个CS，CS上创建单个CL
   2. 启动一个线程插入4GB的lob chunk数据
   3. 启动另一个线程在插入2GB时删除CS
Input: 无
Output: 无
Expected Result: 
   成功创建CS、创建CL、CL中成功插入2GB大小的lob chunk，删除CS，无法插入剩余的数据，得到预期报错，-34集合空间不存在
*/
void test4_insert_lob(vesselImpl *db, std::atomic<UINT32> *lobc_count, UINT32 total_number)
{
   INT32 rc = SDB_OK;
   test_executor session;
   DATA_COLLECTION_PTR handler;
   rc = db->openCL(&session, "foo.bar", dmsOpenCLOptions(), handler);
   ASSERT_EQ(SDB_OK, rc);
   constexpr UINT32 LOBC_SIZE = 1024 * 1024;
   unique_ptr<CHAR[]> buf(new CHAR[LOBC_SIZE]);
   while ((*lobc_count)++ < total_number)
   {
      bson::OID oid;
      oid.init();
      rc = handler->insertLobChunk(&session, oid, lobc_count->load(), 0, LOBC_SIZE, buf.get());
      if (SDB_OK == rc)
      {
         continue;
      }
      else
      {
         ASSERT_EQ(SDB_DMS_CS_NOTEXIST, rc);
         break;
      }
   }
}

void test4_remove_cs(vesselImpl *db, std::atomic<UINT32> *lobc_count, UINT32 trigger_number)
{
   INT32 rc = SDB_OK;
   test_executor session;
   while (true)
   {
      if ((*lobc_count) > trigger_number)
      {
         rc = db->removeCS(&session, "foo");
         ASSERT_EQ(SDB_OK, rc);
         return;
      }
      ossSleepmillis(10);
   }
}

TEST_F(cs_ddl_test, base_removeCS_4)
{
   INT32 rc = SDB_OK;
   outerResource resource = test_outer_resource::getResource();
   test_executor session;
   vesselImpl db;
   openDBOptions options;

   options.path.dataPath = DATA_PATH;
   options.path.indexPath = DATA_PATH;
   options.path.lobmPath = DATA_PATH;
   options.path.lobdPath = DATA_PATH;
   options.path.lsmPath = LSM_PATH;

   bson::BSONObj adjunct;
   DATA_CURSOR_PTR cursor;
   
   DATA_COLLECTION_PTR handler;

   rc = db.open(&session, &resource, options);
   ASSERT_EQ(SDB_OK, rc);

   rc = db.createCS(&session, "foo", 1, dmsCreateCSOptions(), adjunct);
   ASSERT_EQ(SDB_OK, rc);

   rc = db.createCL(&session, "foo.bar", 1, dmsCreateCLOptions(), bson::BSONObj());
   ASSERT_EQ(SDB_OK, rc);

   std::atomic<UINT32> lobc_count{0};
   constexpr UINT32 total_number = 4 * 1024;
   constexpr UINT32 trigger_number = 2 * 1024;
   static_assert(trigger_number < total_number, "Trigger number must be less than total number");
   std::thread th1(test4_insert_lob, &db, &lobc_count, total_number);
   std::thread th2(test4_remove_cs, &db, &lobc_count, trigger_number);

   th1.join();
   th2.join();

   db.close(&session, closeDBOptions());
}
