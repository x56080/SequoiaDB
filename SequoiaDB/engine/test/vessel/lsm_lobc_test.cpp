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

   Source File Name = lsm_lobc_test.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          04/20/2020  LYC  Initial Draft

   Last Changed =

*******************************************************************************/
#include "test_def.h"
#include "vessel/lextentDescriptor.h"
#include "vessel/lsm/lsmLobChunkValue.h"
#include <gtest/gtest.h>

#include <boost/filesystem.hpp>
namespace fs = boost::filesystem;
namespace v = engine::vessel;

class lsm_lobc_test : public testing::Test
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

TEST_F(lsm_lobc_test, base_lobc_test1)
{
   INT32 rc = SDB_OK;
   UINT16 mbid = 1;
   UINT64 lsn = 1;
   UINT32 extCount = 5;
   ossPoolVector<v::lextentDescriptor> exts;

   for (UINT32 i = 0; i < extCount; ++i)
   {
      v::lextentDescriptor desc;
      desc.pcnt = 1;
      desc.pid = ossRand() % 100;
      desc.psv = 1;
      desc.size = 1000;
      exts.push_back(desc);
   }
   bson::BSONObj obj = v::buildLsmLobChunkValue(mbid, lsn, exts);

   UINT16 mbidRes = 0;
   UINT64 lsnRes = 0;
   ossPoolVector<v::lextentDescriptor> extsRes;
   rc = v::extractLsmLobChunkValue(obj, mbidRes, lsnRes, extsRes);
   ASSERT_EQ(SDB_OK, rc);
   ASSERT_EQ(mbid, mbidRes);
   ASSERT_EQ(lsn, lsnRes);
   ASSERT_EQ(extCount, extsRes.size());
   for (UINT32 i = 0; i < extCount; ++i)
   {
      ASSERT_EQ(exts[i].pcnt, extsRes[i].pcnt);
      ASSERT_EQ(exts[i].pid, extsRes[i].pid);
      ASSERT_EQ(exts[i].psv, extsRes[i].psv);
      ASSERT_EQ(exts[i].size, extsRes[i].size);
   }

}