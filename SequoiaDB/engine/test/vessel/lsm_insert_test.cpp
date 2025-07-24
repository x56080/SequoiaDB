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

   Source File Name = lsm_insert_test.cpp

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
#include "rocksdb/db.h"
#include <gtest/gtest.h>

#include <boost/filesystem.hpp>
namespace fs = boost::filesystem;

class lsm_insert_test : public testing::Test
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

TEST_F(lsm_insert_test, base_insert_without_compression)
{
    rocksdb::DB* db;
    rocksdb::Options options;
    options.create_if_missing = true;
    options.compression = rocksdb::kNoCompression;
    options.db_log_dir = LSM_PATH;
    rocksdb::Status s = rocksdb::DB::Open(options, LSM_PATH, &db);
    ASSERT_TRUE(s.ok());
    std::string returnVal;
    std::vector<string> keys={"name","age","tel","height","email"};
    std::vector<string> values={"John","18","13399997777","180","John@example.com"};
    for(UINT32 i = 0; i < keys.size(); ++i)
    {
       s = db->Put(rocksdb::WriteOptions(),keys[i] ,values[i]);
       ASSERT_TRUE(s.ok());
    }
    s=db->Close();
    ASSERT_TRUE(s.ok());
    delete db;
}

TEST_F(lsm_insert_test, base_insert_with_zstd)
{
    rocksdb::DB* db;
    rocksdb::Options options;
    options.create_if_missing = true;
    options.compression = rocksdb::kZSTD;
    options.db_log_dir = LSM_PATH;
    rocksdb::Status s;
    s = rocksdb::DB::Open(options, LSM_PATH, &db);
    ASSERT_TRUE(s.ok());
    std::string returnVal;
    std::vector<string> keys={"name","age","tel","height","email"};
    std::vector<string> values={"John","18","13399997777","180","John@example.com"};
    for(UINT32 i = 0; i < keys.size(); ++i)
    {
       s = db->Put(rocksdb::WriteOptions(),keys[i] ,values[i]);
       ASSERT_TRUE(s.ok());
    }
    s=db->Close();
    ASSERT_TRUE(s.ok());
    delete db;
}

TEST_F(lsm_insert_test, base_insert_with_snappy)
{
    rocksdb::DB* db;
    rocksdb::Options options;
    options.create_if_missing = true;
    options.compression = rocksdb::kSnappyCompression;
    options.db_log_dir = LSM_PATH;
    rocksdb::Status s;
    s = rocksdb::DB::Open(options, LSM_PATH, &db);
    ASSERT_TRUE(s.ok());
    std::string returnVal;
    std::vector<string> keys={"name","age","tel","height","email"};
    std::vector<string> values={"John","18","13399997777","180","John@example.com"};
    for(UINT32 i = 0; i < keys.size(); ++i)
    {
       s = db->Put(rocksdb::WriteOptions(),keys[i] ,values[i]);
       ASSERT_TRUE(s.ok());
    }
    s=db->Close();
    ASSERT_TRUE(s.ok());
    delete db;
}


TEST_F(lsm_insert_test, base_insert_with_lz4)
{
    rocksdb::DB* db;
    rocksdb::Options options;
    options.create_if_missing = true;
    options.compression = rocksdb::kLZ4Compression;
    options.db_log_dir = LSM_PATH;
    rocksdb::Status s;
    s = rocksdb::DB::Open(options, LSM_PATH, &db);
    ASSERT_TRUE(s.ok());
    std::string returnVal;
    std::vector<string> keys={"name","age","tel","height","email"};
    std::vector<string> values={"John","18","13399997777","180","John@example.com"};
    for(UINT32 i = 0; i < keys.size(); ++i)
    {
       s = db->Put(rocksdb::WriteOptions(),keys[i] ,values[i]);
       ASSERT_TRUE(s.ok());
    }
    s=db->Close();
    ASSERT_TRUE(s.ok());
    delete db;
}