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

   Source File Name = vessel_cftest.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#include "vessel/controlFile.h"
#include <gtest/gtest.h>
#include "ossUtil.hpp"
#include <boost/filesystem.hpp>

namespace fs = boost::filesystem;
namespace v = engine::vessel;

const CHAR *TEST_PATH = "/tmp/vessel_cftest";

struct dummyContent
{
   UINT32 a;
   CHAR pad[100];
   dummyContent()
   {
      a = 0;
      ossMemset(pad, 0, sizeof(pad));
   }
};

class cftest : public testing::Test
{
   public:
   static void SetUpTestCase()
   {
      fs::path testPath(TEST_PATH);
      fs::remove_all(testPath);
      fs::create_directory(testPath);
   }

   static void TearDownTestCase()
   {
      fs::path testPath(TEST_PATH);
      fs::remove_all(testPath);
   }

   virtual void SetUp()
   {
      fs::path testPath(TEST_PATH);
      fs::remove_all(testPath);
      fs::create_directory(testPath);
   }
};

class testControlFile : public engine::vessel::controlFile
{
   public:
      testControlFile(UINT32 cnt):_cnt(cnt){}
   public:
      testControlFile(){}
      virtual ~testControlFile(){}

   public:
      virtual const CHAR *getFileNamePrefix() const
      {
         return "unit_test";
      }

      virtual UINT32 getMaxAliveVersionCount()const
      {
         return _cnt;
      }

   private:
      UINT32 _cnt;
};

/// loop write and read with only one version
TEST_F(cftest, test0)
{
   INT32 rc = SDB_OK;
   testControlFile f(1);
   rc = f.open(TEST_PATH, TRUE);
   ASSERT_EQ(SDB_OK, rc);
   v::controlFile::head h;
   dummyContent content;

   rc = f.readOldestVersion(h, sizeof(content), &content);
   ASSERT_EQ(SDB_VESSEL_CF_VERSION_NOT_AVAILABLE, rc);

   rc = f.readLatestVersion(h, sizeof(content), &content);
   ASSERT_EQ(SDB_VESSEL_CF_VERSION_NOT_AVAILABLE, rc);

   for (UINT32 i = 0; i < 32; ++i)
   {
      dummyContent commit;
      commit.a = i;
      rc = f.commit(sizeof(commit), &commit);
      ASSERT_EQ(SDB_OK, rc);
      rc = f.readLatestVersion(h, sizeof(dummyContent), &content);
      ASSERT_EQ(SDB_OK, rc);
      ASSERT_TRUE(h.isValid());
      ASSERT_FALSE(h.isUnused());
      ASSERT_EQ(h.commitVersion, i);
      ASSERT_EQ(h.contentLen, sizeof(dummyContent));
      ASSERT_EQ(content.a, i);

      rc = f.readOldestVersion(h, sizeof(dummyContent), &content);
      ASSERT_EQ(SDB_OK, rc);
      ASSERT_TRUE(h.isValid());
      ASSERT_FALSE(h.isUnused());
      ASSERT_EQ(h.contentLen, sizeof(dummyContent));
      ASSERT_EQ(h.commitVersion, i);
      ASSERT_EQ(content.a, i);
   }
   
   f.close();
}

/// loop write and read with multi versions
TEST_F(cftest, test1)
{
   INT32 rc = SDB_OK;
   testControlFile f(16);
   rc = f.open(TEST_PATH, TRUE);
   ASSERT_EQ(SDB_OK, rc);
   v::controlFile::head h;
   dummyContent content;

   rc = f.readOldestVersion(h, sizeof(content), &content);
   ASSERT_EQ(SDB_VESSEL_CF_VERSION_NOT_AVAILABLE, rc);

   rc = f.readLatestVersion(h, sizeof(content), &content);
   ASSERT_EQ(SDB_VESSEL_CF_VERSION_NOT_AVAILABLE, rc);

   for (UINT32 i = 0; i < 32; ++i)
   {
      dummyContent commit;
      commit.a = i;
      rc = f.commit(sizeof(commit), &commit);
      ASSERT_EQ(SDB_OK, rc);
      rc = f.readLatestVersion(h, sizeof(dummyContent), &content);
      ASSERT_EQ(SDB_OK, rc);
      ASSERT_TRUE(h.isValid());
      ASSERT_FALSE(h.isUnused());
      ASSERT_EQ(h.commitVersion, i);
      ASSERT_EQ(h.contentLen, sizeof(dummyContent));
      ASSERT_EQ(content.a, i);

      rc = f.readOldestVersion(h, sizeof(dummyContent), &content);
      ASSERT_EQ(SDB_OK, rc);
      ASSERT_TRUE(h.isValid());
      ASSERT_FALSE(h.isUnused());
      ASSERT_EQ(h.contentLen, sizeof(dummyContent));
      if (i < 16)
      {
         ASSERT_EQ(h.commitVersion, 0);
         ASSERT_EQ(content.a, 0);
      }
      else
      {
         ASSERT_EQ(h.commitVersion, i - 16 + 1);
         ASSERT_EQ(content.a, i - 16 + 1);
      }
   }
   
   f.close();
}

/// write, close and reopen
TEST_F(cftest, test2)
{
   INT32 rc = SDB_OK;
   testControlFile f(16);
   rc = f.open(TEST_PATH, TRUE);
   ASSERT_EQ(SDB_OK, rc);
   v::controlFile::head h;
   dummyContent content;

   for (UINT32 i = 0; i < 32; ++i)
   {
      dummyContent commit;
      commit.a = i;
      rc = f.commit(sizeof(commit), &commit);
      ASSERT_EQ(SDB_OK, rc);
   }

   f.close();
   rc = f.open(TEST_PATH, TRUE);
   ASSERT_EQ(SDB_OK, rc);
   rc = f.readLatestVersion(h, sizeof(dummyContent), &content);
   ASSERT_EQ(SDB_OK, rc);
   ASSERT_TRUE(h.isValid());
   ASSERT_FALSE(h.isUnused());
   ASSERT_EQ(h.commitVersion, 31);
   ASSERT_EQ(h.contentLen, sizeof(dummyContent));
   ASSERT_EQ(content.a, 31);

   rc = f.readOldestVersion(h, sizeof(dummyContent), &content);
   ASSERT_EQ(SDB_OK, rc);
   ASSERT_TRUE(h.isValid());
   ASSERT_FALSE(h.isUnused());
   ASSERT_EQ(h.commitVersion, 16);
   ASSERT_EQ(h.contentLen, sizeof(dummyContent));
   ASSERT_EQ(content.a, 16);

   for (UINT32 i = 0; i < 16 ; ++i)
   {
      rc = f.readPreVersion(i, h, sizeof(dummyContent), &content);
      ASSERT_EQ(SDB_OK, rc);
      ASSERT_TRUE(h.isValid());
      ASSERT_FALSE(h.isUnused());
      ASSERT_EQ(h.commitVersion, 31 - i);
      ASSERT_EQ(h.contentLen, sizeof(dummyContent));
      ASSERT_EQ(content.a, 31 - i);
   }

   rc = f.readPreVersion(16, h, sizeof(dummyContent), &content);
   ASSERT_EQ(SDB_VESSEL_CF_VERSION_NOT_AVAILABLE, rc);

   f.close();
}

/// write, close, rm latest file, reopen
TEST_F(cftest, test3)
{
   INT32 rc = SDB_OK;
   testControlFile f(16);
   rc = f.open(TEST_PATH, TRUE);
   ASSERT_EQ(SDB_OK, rc);
   v::controlFile::head h;
   dummyContent content;
   std::string deletePath = std::string(TEST_PATH) + "/unit_test.control.15";

   for (UINT32 i = 0; i < 16; ++i)
   {
      dummyContent commit;
      commit.a = i;
      rc = f.commit(sizeof(commit), &commit);
      ASSERT_EQ(SDB_OK, rc);
   }

   f.close();

   ossDelete(deletePath.c_str());

   rc = f.open(TEST_PATH, TRUE);
   ASSERT_EQ(SDB_OK, rc);
   rc = f.readLatestVersion(h, sizeof(dummyContent), &content);
   ASSERT_EQ(SDB_OK, rc);
   ASSERT_TRUE(h.isValid());
   ASSERT_FALSE(h.isUnused());
   ASSERT_EQ(h.commitVersion, 14);
   ASSERT_EQ(h.contentLen, sizeof(dummyContent));
   ASSERT_EQ(content.a, 14);

   ASSERT_EQ(f.getAliveVersionCount(), 15);
   dummyContent commit;
   commit.a = 15;
   rc = f.commit(sizeof(commit), &commit);
   ASSERT_EQ(SDB_OK, rc);
   ASSERT_EQ(f.getAliveVersionCount(), 16);

   rc = f.readLatestVersion(h, sizeof(dummyContent), &content);
   ASSERT_EQ(SDB_OK, rc);
   ASSERT_TRUE(h.isValid());
   ASSERT_FALSE(h.isUnused());
   ASSERT_EQ(h.commitVersion, 15);
   ASSERT_EQ(h.contentLen, sizeof(dummyContent));
   ASSERT_EQ(content.a, 15);

   f.close();
}

/// write, close, rm non-latest file, reopen
TEST_F(cftest, test4)
{
   INT32 rc = SDB_OK;
   testControlFile f(16);
   rc = f.open(TEST_PATH, TRUE);
   ASSERT_EQ(SDB_OK, rc);
   v::controlFile::head h;
   dummyContent content;
   std::string deletePath = std::string(TEST_PATH) + "/unit_test.control.13";

   for (UINT32 i = 0; i < 16; ++i)
   {
      dummyContent commit;
      commit.a = i;
      rc = f.commit(sizeof(commit), &commit);
      ASSERT_EQ(SDB_OK, rc);
   }

   f.close();

   ossDelete(deletePath.c_str());  

   rc = f.open(TEST_PATH, TRUE);
   ASSERT_EQ(SDB_OK, rc);
   rc = f.readLatestVersion(h, sizeof(dummyContent), &content);
   ASSERT_EQ(SDB_OK, rc);
   ASSERT_TRUE(h.isValid());
   ASSERT_FALSE(h.isUnused());
   ASSERT_EQ(h.commitVersion, 15);
   ASSERT_EQ(h.contentLen, sizeof(dummyContent));
   ASSERT_EQ(content.a, 15);
   ASSERT_EQ(f.getAliveVersionCount(), 15);

   dummyContent commit;
   commit.a = 16;
   rc = f.commit(sizeof(commit), &commit);
   ASSERT_EQ(SDB_OK, rc);
   ASSERT_EQ(f.getAliveVersionCount(), 16);

   rc = f.readLatestVersion(h, sizeof(dummyContent), &content);
   ASSERT_EQ(SDB_OK, rc);
   ASSERT_TRUE(h.isValid());
   ASSERT_FALSE(h.isUnused());
   ASSERT_EQ(h.commitVersion, 16);
   ASSERT_EQ(h.contentLen, sizeof(dummyContent));
   ASSERT_EQ(content.a, 16);

   rc = f.readOldestVersion(h, sizeof(dummyContent), &content);
   ASSERT_EQ(SDB_OK, rc);
   ASSERT_TRUE(h.isValid());
   ASSERT_FALSE(h.isUnused());
   ASSERT_EQ(h.commitVersion, 0);
   ASSERT_EQ(h.contentLen, sizeof(dummyContent));
   ASSERT_EQ(content.a, 0);

   f.close();
}