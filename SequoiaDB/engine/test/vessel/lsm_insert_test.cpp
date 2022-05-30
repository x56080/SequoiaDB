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

   Source File Name = lsm_insert_test.cpp

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