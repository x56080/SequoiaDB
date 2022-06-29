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

   Source File Name = lsm_compaction_filter_test.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          11/08/2021  LYC  Initial Draft

   Last Changed =

******************************************************************************/
#include "test_def.h"
#include <gtest/gtest.h>
#include "rocksdb/db.h"
#include "rocksdb/options.h"
#include "rocksdb/compaction_filter.h"
#include "vessel/vesselImpl.h"
#include "vessel/lsm/lsmCompactionFilter.hpp"
#include "vessel/lsm/lsmDB.h"
#include <boost/filesystem.hpp>
#include "vessel/lsm/lsmIndexKeyPacker.h"

using namespace std;
using namespace rocksdb;

namespace fs = boost::filesystem;

class lsm_compaction_filter_test : public testing::Test
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
   void SetUp()
   {
      {
      fs::path testPath(LSM_PATH);
      fs::remove_all(testPath);
      fs::create_directory(testPath);
      }
   }
};

/*
Name: base_compaction_filter_test1
Description: 
   相同key和rid的单条记录compaction
   1. 插入一条value类型为Insert的记录(老版本)
   2. 插入一条value类型为Delete的记录(删除)
   3. 验证记录数量
   4. 再次插入value类型为Insert的记录(最新版本)
   5. 手动触发Compaction
   6. 验证记录是否正确
Expected Result: 
   Compation完成后无法查到插入记录
*/
TEST_F(lsm_compaction_filter_test, base_compaction_filter_test1)
{
   INT32 rc = SDB_OK;
   lsmDB ldb;
   rocksdb::Options opt;
   opt.create_if_missing = TRUE;
   opt.create_missing_column_families = TRUE;

   CHAR fullKey[100] = {};
   CHAR indexValue[LSM_VALUE_SIZE] = {};

   INT32 count = 0;
   CHAR keyData = 1;

   lsmIndexValue vl;
   lsmIdxFixedKey fixedKey;
   fixedKey.ordering = orderingWrapper(1, 1);
   fixedKey.indexid.reset(0, 0, 0);
   fixedKey.rid = recordID(0, 0);

   dummyDataJournal dummyJournal;
   ldb.setJournal(&dummyJournal);
   rc = ldb.open(LSM_PATH, &opt);
   ASSERT_EQ(SDB_OK, rc);
   
   // insert
   vl.reset(LSM_IDX_VALUE_TYPE_INSERT);

   fixedKey.lsn = 1;
   rc = lsmIndexKeyPacker().pack(fixedKey, ixmKey(&keyData),
                                 sizeof(fullKey), fullKey);
   ASSERT_EQ(SDB_OK, rc);

   ossMemcpy(indexValue, &vl, LSM_VALUE_SIZE);
   rc = ldb.getIdxColumnFamily().put(Slice(fullKey,sizeof(fullKey)), 
                                     Slice(indexValue, LSM_VALUE_SIZE));
   ASSERT_EQ(SDB_OK, rc);

   // delete
   fixedKey.lsn = 2;
   rc = lsmIndexKeyPacker().pack(fixedKey, ixmKey(&keyData),
                                 sizeof(fullKey), fullKey);
   ASSERT_EQ(SDB_OK, rc);
   vl.reset(LSM_IDX_VALUE_TYPE_DELETE);
   ossMemcpy(indexValue, &vl, LSM_VALUE_SIZE);
   ldb.getIdxColumnFamily().put(Slice(fullKey, sizeof(fullKey)), 
                                Slice(indexValue, LSM_VALUE_SIZE));

   // re-insert
   vl.reset(LSM_IDX_VALUE_TYPE_INSERT);
   fixedKey.lsn = 3;
   rc = lsmIndexKeyPacker().pack(fixedKey, ixmKey(&keyData),
                                 sizeof(fullKey), fullKey);
   ASSERT_EQ(SDB_OK, rc);

   ossMemcpy(indexValue, &vl, LSM_VALUE_SIZE);
   rc = ldb.getIdxColumnFamily().put(Slice(fullKey,sizeof(fullKey)), 
                                     Slice(indexValue, LSM_VALUE_SIZE));
   ASSERT_EQ(SDB_OK, rc);

   Iterator *itr = ldb.getIdxColumnFamily().newIterator(rocksdb::ReadOptions());
   ASSERT_NE(nullptr, itr);
   for(itr->SeekToFirst(); itr->Valid(); itr->Next())
   {
      ++count;
   }
   delete itr;
   ASSERT_EQ(3, count);
   

   rc = ldb.getIdxColumnFamily().compact(nullptr, nullptr);
   ASSERT_EQ(SDB_OK, rc);

   itr = ldb.getIdxColumnFamily().newIterator(rocksdb::ReadOptions());
   ASSERT_NE(nullptr, itr);
   count = 0;
   for(itr->SeekToFirst(); itr->Valid(); itr->Next())
   {
      const lsmIndexValue *val = (const lsmIndexValue *)itr->value().data();
      ASSERT_FALSE(val->isDeleted());
      ++count;
   }
   delete itr;
   ASSERT_EQ(1, count);

   ldb.close();
}


/*
Name: base_compaction_filter_test2
Description: 
   不同rid多条记录Compaction
   1. 插入多条rid不同,value类型为Insert的记录(老版本)
   2. 插入多条rid不同,value类型为Delete的记录(删除前半部分记录)
   3. 验证记录数量
   4. 手动触发Compaction
   5. 验证记录是否正确
Expected Result: 
   Compation完成后无法查到被删除的记录
*/
TEST_F(lsm_compaction_filter_test, base_compaction_filter_test2)
{
   INT32 rc = SDB_OK;

   lsmDB ldb;
   rocksdb::Options opt;
   opt.create_if_missing = TRUE;
   opt.create_missing_column_families = TRUE;
   dummyDataJournal dummyJournal;
   ldb.setJournal(&dummyJournal);
   rc = ldb.open(LSM_PATH, &opt);
   ASSERT_EQ(SDB_OK, rc);

   CHAR fullKey[100] = {};
   CHAR indexValue[LSM_VALUE_SIZE] = {};
   globalIndexID indexId;

   UINT32 count = 0;
   UINT32 insertCount = 50;
   CHAR keyData = 1;
   lsmIdxFixedKey fixedKey;
   fixedKey.ordering = orderingWrapper(1, 1);
   fixedKey.indexid.reset(0, 0, 0);
   fixedKey.rid = recordID(0, 0);

   lsmIndexValue vl;


   // insert
   vl.reset(LSM_IDX_VALUE_TYPE_INSERT);
   for (UINT32 i = 0; i < insertCount; ++i)
   {  
      fixedKey.rid.setPid(i+1);
      fixedKey.rid.setPos((INT16)i+1);
      fixedKey.lsn = 1;
      rc = lsmIndexKeyPacker().pack(fixedKey, ixmKey(&keyData),
                                 sizeof(fullKey), fullKey);
      ASSERT_EQ(SDB_OK, rc);

      ossMemcpy(indexValue, &vl, LSM_VALUE_SIZE);
      rc = ldb.getIdxColumnFamily().put(Slice(fullKey, sizeof(fullKey)), 
                                        Slice(indexValue, LSM_VALUE_SIZE));
      ASSERT_EQ(SDB_OK, rc);
   }

   // delete
   vl.reset(LSM_IDX_VALUE_TYPE_DELETE);
   for (UINT32 i = 0; i < insertCount / 2; ++i)
   {
      fixedKey.rid.setPid(i+1);
      fixedKey.rid.setPos((INT16)i+1);
      fixedKey.lsn = 2;
      rc = lsmIndexKeyPacker().pack(fixedKey, ixmKey(&keyData),
                                 sizeof(fullKey), fullKey);
      ASSERT_EQ(SDB_OK, rc);

      ossMemcpy(indexValue, &vl, LSM_VALUE_SIZE);
      rc = ldb.getIdxColumnFamily().put(Slice(fullKey,sizeof(fullKey)), 
                                        Slice(indexValue, LSM_VALUE_SIZE));
      ASSERT_EQ(SDB_OK, rc);
   }
   

   Iterator* itr = ldb.getIdxColumnFamily().newIterator(rocksdb::ReadOptions());
   for (itr->SeekToFirst(); itr->Valid(); itr->Next())
   {
      ++count;
   }
   delete itr;
   ASSERT_EQ(insertCount + insertCount/2, count);

   ldb.getIdxColumnFamily().compact(nullptr, nullptr);

   itr =  ldb.getIdxColumnFamily().newIterator(rocksdb::ReadOptions());
   ASSERT_NE(nullptr, itr);
   count = 0;
   UINT32 i = insertCount / 2 + 1;
   lsmPureKeyEntry key;

   for (itr->SeekToFirst(); itr->Valid(); itr->Next())
   {
      rc = key.shallowCopy(itr->key());
      ASSERT_EQ(SDB_OK, rc);
      ASSERT_EQ(i, key.getRid().getPid());
      ASSERT_EQ((INT16)i, key.getRid().getPos());

      const lsmIndexValue *val = (const lsmIndexValue *)itr->value().data();
      ASSERT_FALSE(val->isDeleted());

      ++count;
      ++i;
   }
   delete itr;
   ASSERT_EQ(insertCount/2, count);

   ldb.close();
}

/*
Name: base_compaction_filter_test3
Description: 
   不同ixmKey多条记录compaction
   1. 插入ixmKey不同的两条记录
   2. 插入其中一条的删除记录
   3. 验证记录数量
   4. 手动触发Compaction
   5. 验证记录是否正确
Expected Result: 
   Compation完成后无法查到被删除的记录
*/
TEST_F(lsm_compaction_filter_test, base_compaction_filter_test3)
{
   INT32 rc = SDB_OK;

   lsmDB ldb;
   rocksdb::Options opt;
   opt.create_if_missing = TRUE;
   opt.create_missing_column_families = TRUE;
   dummyDataJournal dummyJournal;
   ldb.setJournal(&dummyJournal);
   rc = ldb.open(LSM_PATH, &opt);
   ASSERT_EQ(SDB_OK, rc);

   CHAR fullKey[100] = {};
   CHAR indexValue[LSM_VALUE_SIZE] = {};

   UINT32 count = 0;
   CHAR keyData1 = 1;
   CHAR keyData2 = 2;
   lsmIdxFixedKey fixedKey;
   fixedKey.ordering = orderingWrapper(1, 1);
   fixedKey.indexid.reset(0, 0, 0);
   fixedKey.rid = recordID(0, 0);

   lsmIndexValue vl;
   // insert
   vl.reset(LSM_IDX_VALUE_TYPE_INSERT);
   fixedKey.lsn = 1;
   rc = lsmIndexKeyPacker().pack(fixedKey, ixmKey(&keyData1),
                                 sizeof(fullKey), fullKey);
   ASSERT_EQ(SDB_OK, rc);

   ossMemcpy(indexValue, &vl, LSM_VALUE_SIZE);
   rc = ldb.getIdxColumnFamily().put(Slice(fullKey, sizeof(fullKey)), 
                                     Slice(indexValue, LSM_VALUE_SIZE));
   ASSERT_EQ(SDB_OK, rc);

   rc = lsmIndexKeyPacker().pack(fixedKey, ixmKey(&keyData2),
                                 sizeof(fullKey), fullKey);
   ASSERT_EQ(SDB_OK, rc);
   rc = ldb.getIdxColumnFamily().put(Slice(fullKey, sizeof(fullKey)), 
                                     Slice(indexValue, LSM_VALUE_SIZE));
   ASSERT_EQ(SDB_OK, rc);

   //delete
   vl.reset(LSM_IDX_VALUE_TYPE_DELETE);
   fixedKey.lsn = 2;
   rc = lsmIndexKeyPacker().pack(fixedKey, ixmKey(&keyData2),
                                 sizeof(fullKey), fullKey);
   ASSERT_EQ(SDB_OK, rc);

   ossMemcpy(indexValue, &vl, LSM_VALUE_SIZE);
   rc = ldb.getIdxColumnFamily().put(Slice(fullKey, sizeof(fullKey)), 
                                     Slice(indexValue, LSM_VALUE_SIZE));
   ASSERT_EQ(SDB_OK, rc);

   Iterator* itr = ldb.getIdxColumnFamily().newIterator(rocksdb::ReadOptions());
   for (itr->SeekToFirst(); itr->Valid(); itr->Next())
   {
      ++count;
   }
   delete itr;
   ASSERT_EQ((UINT32)3, count);


   rc = ldb.getIdxColumnFamily().compact(nullptr, nullptr);
   ASSERT_EQ(SDB_OK, rc);

   itr = ldb.getIdxColumnFamily().newIterator(rocksdb::ReadOptions());
   ASSERT_NE(nullptr, itr);
   count = 0;
   lsmPureKeyEntry key;
   for (itr->SeekToFirst(); itr->Valid(); itr->Next())
   {
      rc = key.shallowCopy(itr->key());
      ASSERT_EQ(SDB_OK, rc);
      ASSERT_TRUE(key.getKey().woEqual(ixmKey(&keyData1)));

      const lsmIndexValue *val = (const lsmIndexValue *)itr->value().data();
      ASSERT_FALSE(val->isDeleted());
      ++count;
   }
   delete itr;
   ASSERT_EQ((UINT32)1, count);

   ldb.close();

}

/*
Name: base_compaction_filter_test4
Description: 
   相同key和rid,不同索引Compaction
   1. 插入仅有indexID不同的两条记录
   2. 验证记录数量
   3. 手动触发Compaction
   4. 验证记录是否正确
Expected Result: 
   Compation完成后仍能查到记录
*/
TEST_F(lsm_compaction_filter_test, base_compaction_filter_test4)
{
   INT32 rc = SDB_OK;
   lsmDB ldb;
   rocksdb::Options opt;
   opt.create_if_missing = TRUE;
   opt.create_missing_column_families = TRUE;
   dummyDataJournal dummyJournal;
   ldb.setJournal(&dummyJournal);
   rc = ldb.open(LSM_PATH, &opt);
   ASSERT_EQ(SDB_OK, rc);
   CHAR fullKey[100] = {};
   CHAR indexValue[LSM_VALUE_SIZE] = {};

   CHAR keyData = 1;
   ixmKey key;
   lsmIdxFixedKey fixedKey;
   fixedKey.ordering = orderingWrapper(1, 1);
   fixedKey.indexid.reset(0, 0, 0);
   fixedKey.rid = recordID(0, 0);

   UINT32 count = 0;

   lsmIndexValue vl;

   // insert
   vl.reset(LSM_IDX_VALUE_TYPE_INSERT);
   fixedKey.indexid.reset(1, 2, 3);
   fixedKey.lsn = 1;
   rc = lsmIndexKeyPacker().pack(fixedKey, ixmKey(&keyData),
                                 sizeof(fullKey), fullKey);
   ASSERT_EQ(SDB_OK, rc);
   ossMemcpy(indexValue, &vl, LSM_VALUE_SIZE);
   rc = ldb.getIdxColumnFamily().put(Slice(fullKey, sizeof(fullKey)), 
                                     Slice(indexValue, LSM_VALUE_SIZE));
   ASSERT_EQ(SDB_OK, rc);


   // insert
   fixedKey.indexid.reset(111, 222, 333);
   rc = lsmIndexKeyPacker().pack(fixedKey, ixmKey(&keyData),
                                 sizeof(fullKey), fullKey);
   ASSERT_EQ(SDB_OK, rc);
   ossMemcpy(indexValue, &vl, LSM_VALUE_SIZE);
   rc = ldb.getIdxColumnFamily().put(Slice(fullKey, sizeof(fullKey)), 
                                     Slice(indexValue, LSM_VALUE_SIZE));
   ASSERT_EQ(SDB_OK, rc);

   Iterator* itr = ldb.getIdxColumnFamily().newIterator(rocksdb::ReadOptions());
   ASSERT_NE(nullptr, itr);
   for (itr->SeekToFirst(); itr->Valid(); itr->Next())
   {
      ++count;
   }
   delete itr;
   ASSERT_EQ((UINT32)2, count);

   ldb.getIdxColumnFamily().compact(nullptr, nullptr);

   itr = ldb.getIdxColumnFamily().newIterator(rocksdb::ReadOptions());
   ASSERT_NE(nullptr, itr);
   count = 0;
   for (itr->SeekToFirst(); itr->Valid(); itr->Next())
   {
      ++count;
   }
   delete itr;
   ASSERT_EQ(2, count);
   ldb.close();
}
