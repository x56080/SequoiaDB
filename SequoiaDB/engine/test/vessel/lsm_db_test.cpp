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

   Source File Name = lsm_db_test.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          08/15/2022  LYC  Initial Draft

   Last Changed =

*******************************************************************************/
#include "oss.hpp"
#include "vessel/lsm/lsmDB.h"
#include "vessel/keyStringBuilder.h"
#include "vessel/keyString.h"
#include "vessel/keyStringCoder.h"
#include "vessel/keyStringModifier.h"
#include "vessel/sliceTransfer.h"
#include "vessel/lsm/lsmIndexEntryValue.h"
#include <gtest/gtest.h>
#include <boost/filesystem.hpp>

namespace fs = boost::filesystem;
using namespace engine::vessel;

constexpr CHAR *LSM_PATH = "/opt/unit_test/lsm";

class lsm_db_test : public testing::Test
{
   public:
   static void SetUpTestCase()
   {
      {
      fs::path testPath(LSM_PATH);
      fs::remove_all(testPath);
      fs::create_directory(testPath);
      }
   }

   static void TearDownTestCase()
   {
      {
      fs::path testPath(LSM_PATH);
      fs::remove_all(testPath);
      }
   }

   virtual void SetUp()
   {
      {
      fs::path testPath(LSM_PATH);
      fs::remove_all(testPath);
      fs::create_directory(testPath);
      }
      sdbEnablePD("/opt/diaglog/sdb.log", 1, 1000);
      setPDLevel(PDDEBUG);
   }
};

/*
Name: base_restore_test1
Description: 
   lsmDB崩溃恢复处理
   1. 启动lsmDB，插入多条索引记录，关闭lsmDB
   2. 重新启动lsmDB，校验sst文件数量
   3. 设置恢复lsn为0，调用lsmDB restore方法进行恢复，并校验sst文件数量
      （恢复lsn为0，代表插入的索引记录均为无效记录，需要被删除）
   4. 读取记录内容并校验内容正确性
   5. 关闭lsmDB
Expected Result: 
   restore前，包含多个sst文件；restore后，所有sst文件均被删除。
   且无法读取到任意一条记录。
*/
TEST_F(lsm_db_test, base_restore_test1)
{
   INT32 rc = SDB_OK;
   lsmDB db;

   rc = db.open(LSM_PATH);
   ASSERT_EQ(SDB_OK, rc);

   UINT32 count = 30000;
   STACK_KEY_STRING_BUILDER builder;
   for (UINT32 i = 1; i <= count; ++i)
   {
      bson::BSONObj obj = BSON("a" << i);
      orderingWrapper ordering(0, 1);
      recordID rid(i, 1);
      globalIndexID id(1, 0, i);
      lsmIndexEntryValue vl;
      rc = builder.buildIndexEntryKey(obj, ordering, rid, &id);
      ASSERT_EQ(SDB_OK, rc);
      vl.type = LSM_INDEX_ENTRY_TYPE_INSERT;
      vl.lsn = i;
      keyString ks = builder.getShallowKeyString();
      rocksdb::Slice vs = rocksdb::Slice((const CHAR *)&vl,
                                         LSM_INDEX_ENTRY_VALUE_SIZE);
      rc = db.put(LSM_CF_HYBRID_INDEX,
                  toRocksdbSlice(ks.getRawData()),
                  vs);
      ASSERT_EQ(SDB_OK, rc);
   }

   db.close();

   rc = db.open(LSM_PATH);
   ASSERT_EQ(SDB_OK, rc);

   ossPoolVector<std::string> ssts;
   rc = db.loadSSTs(LSM_CF_HYBRID_INDEX, 0, FALSE, TRUE, ssts);
   ASSERT_TRUE(!ssts.empty());
   ssts.clear();

   rc = db.restore(0, 0);
   ASSERT_EQ(SDB_OK, rc);

   rc = db.loadSSTs(LSM_CF_HYBRID_INDEX, 0, FALSE, TRUE, ssts);
   ASSERT_EQ(SDB_OK, rc);
   ASSERT_TRUE(ssts.empty());

   rocksdb::ReadOptions rOpt;
   rocksdb::Iterator *itr = db.newIterator(LSM_CF_HYBRID_INDEX, rOpt);
   ASSERT_NE(nullptr, itr);
   itr->SeekToFirst();
   ASSERT_FALSE(itr->Valid());
   delete itr;

   db.close();
}

/*
Name: base_restore_test2
Description: 
   lsmDB崩溃恢复处理
   1. 启动lsmDB，插入多条索引记录，关闭lsmDB
   2. 重新启动lsmDB，校验sst文件数量
   3. 设置恢复lsn为插入的索引记录数，调用lsmDB restore方法进行恢复，并校验sst文件数量
      （恢复lsn为索引记录数，代表插入的索引记录均为有效记录，不需要被删除）
   4. 读取记录内容并校验内容正确性
   5. 关闭lsmDB
Expected Result: 
   restore前，包含多个sst文件；restore后，所有sst文件不变。
   且可以读取到任意一条记录。
*/
TEST_F(lsm_db_test, base_restore_test2)
{
   INT32 rc = SDB_OK;
   lsmDB db;

   rc = db.open(LSM_PATH);
   ASSERT_EQ(SDB_OK, rc);

   UINT32 count = 30000;
   STACK_KEY_STRING_BUILDER builder;
   for (UINT32 i = 0; i < count; ++i)
   {
      bson::BSONObj obj = BSON("a" << i);
      orderingWrapper ordering(0, 1);
      recordID rid(i, 1);
      globalIndexID id(1, 0, i);
      lsmIndexEntryValue vl;
      rc = builder.buildIndexEntryKey(obj, ordering, rid, &id);
      ASSERT_EQ(SDB_OK, rc);

      vl.type = LSM_INDEX_ENTRY_TYPE_INSERT;
      vl.lsn = i;
      keyString ks = builder.getShallowKeyString();
      rocksdb::Slice vs = rocksdb::Slice((const CHAR *)&vl,
                                         LSM_INDEX_ENTRY_VALUE_SIZE);
      rc = db.put(LSM_CF_HYBRID_INDEX,
                  toRocksdbSlice(ks.getRawData()),
                  vs);
      ASSERT_EQ(SDB_OK, rc);
   }

   db.close();

   rc = db.open(LSM_PATH);
   ASSERT_EQ(SDB_OK, rc);

   ossPoolVector<std::string> ssts;

   rc = db.loadSSTs(LSM_CF_HYBRID_INDEX, 0, FALSE, TRUE, ssts);
   ASSERT_TRUE(!ssts.empty());
   UINT32 sstCount = ssts.size();
   ssts.clear();

   rc = db.restore(0, count);
   ASSERT_EQ(SDB_OK, rc);

   rc = db.loadSSTs(LSM_CF_HYBRID_INDEX, 0, FALSE, TRUE, ssts);
   ASSERT_TRUE(!ssts.empty());
   ASSERT_EQ(sstCount, ssts.size());

   rocksdb::ReadOptions rOpt;
   rocksdb::Iterator *itr = db.newIterator(LSM_CF_HYBRID_INDEX, rOpt);
   ASSERT_NE(nullptr, itr);

   itr->SeekToFirst();
   ASSERT_TRUE(itr->Valid());
   ASSERT_TRUE(itr->status().ok());
   UINT32 i = 0;
   while(itr->Valid())
   {
      lsmIndexEntryValueRef vl(itr->value());
      ASSERT_TRUE(vl.isValid());
      ASSERT_EQ(i, vl.getValuePtr()->lsn);
      ++i;
      itr->Next();
      ASSERT_TRUE(itr->status().ok());
   }
   delete itr;
   ASSERT_EQ(i, count);

   db.close();
}

/*
Name: base_restore_test3
Description: 
   lsmDB崩溃恢复处理
   1. 启动lsmDB，插入多条索引记录，关闭lsmDB
   2. 重新启动lsmDB，并校验sst文件数量
   3. 设置恢复lsn小于插入的索引记录数，调用lsmDB restore方法进行恢复，并校验sst文件数量
      （说明部分插入记录为无效记录，需要删除）
   4. 读取记录内容并校验内容正确性
   5. 关闭lsmDB
Expected Result: 
   restore前，包含多个sst文件；restore后，所有sst文件均被删除。
   且无法读取到任意一条记录。
*/
TEST_F(lsm_db_test, base_restore_test3)
{
   INT32 rc = SDB_OK;
   lsmDB db;

   rc = db.open(LSM_PATH);
   ASSERT_EQ(SDB_OK, rc);

   UINT32 count = 60000;
   UINT32 restoreCount = 30000;
   ASSERT_GT(count, restoreCount);
   STACK_KEY_STRING_BUILDER builder;
   for (UINT32 i = 0; i < count; ++i)
   {
      bson::BSONObj obj = BSON("a" << i);
      orderingWrapper ordering(0, 1);
      recordID rid(i, 1);
      globalIndexID id(1, 0, i);
      lsmIndexEntryValue vl;
      rc = builder.buildIndexEntryKey(obj, ordering, rid, &id);
      ASSERT_EQ(SDB_OK, rc);

      vl.type = LSM_INDEX_ENTRY_TYPE_INSERT;
      vl.lsn = i;
      keyString ks = builder.getShallowKeyString();
      rocksdb::Slice vs = rocksdb::Slice((const CHAR *)&vl,
                                         LSM_INDEX_ENTRY_VALUE_SIZE);
      rc = db.put(LSM_CF_HYBRID_INDEX,
                  toRocksdbSlice(ks.getRawData()),
                  vs);
      ASSERT_EQ(SDB_OK, rc);
   }

   db.close();

   rc = db.open(LSM_PATH);
   ASSERT_EQ(SDB_OK, rc);

   ossPoolVector<std::string> ssts;
   rc = db.loadSSTs(LSM_CF_HYBRID_INDEX, 0, FALSE, TRUE, ssts);
   ASSERT_TRUE(!ssts.empty());
   ssts.clear();

   rc = db.restore(10, restoreCount);
   ASSERT_EQ(SDB_INVALID_OPERATION, rc);
   rc = db.loadSSTs(LSM_CF_HYBRID_INDEX, 0, FALSE, TRUE, ssts);
   ASSERT_TRUE(!ssts.empty());
   ssts.clear();

   rc = db.restore(0, restoreCount);
   ASSERT_EQ(SDB_OK, rc);

   rc = db.loadSSTs(LSM_CF_HYBRID_INDEX, 0, FALSE, TRUE, ssts);
   ASSERT_TRUE(ssts.empty());

   rocksdb::ReadOptions rOpt;
   rocksdb::Iterator *itr = db.newIterator(LSM_CF_HYBRID_INDEX, rOpt);
   ASSERT_NE(nullptr, itr);
   itr->SeekToFirst();
   ASSERT_FALSE(itr->Valid());
   delete itr;

   db.close();
}

