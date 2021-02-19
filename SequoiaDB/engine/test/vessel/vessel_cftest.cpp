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

   When/how to use: this program may be used on binary and text-formatted
   versions of PMD component. This file contains functions for agent processing.

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
      virtual const CHAR *getName() const
      {
         return "unit_test";
      }

      virtual UINT16 getUserType() const
      {
         return 100;
      }

      virtual UINT32 getSeqWindow()const
      {
         return _cnt;
      }

      virtual std::string toString(const CHAR *body)const
      {
         return std::string("");
      }

   private:
      UINT32 _cnt;
};

/// loop write and readlatest
TEST_F(cftest, test1)
{
   INT32 rc = SDB_OK;
   testControlFile f(10);
   rc = f.open(TEST_PATH);
   ASSERT_EQ(SDB_OK, rc);

   rc = f.readOldestVersion(NULL, NULL);
   ASSERT_EQ(SDB_VESSEL_CF_INVALID_SEQUENCE, rc);

   rc = f.readLatestVersion(NULL, NULL);
   ASSERT_EQ(SDB_VESSEL_CF_INVALID_SEQUENCE, rc);

   for (UINT32 i = 0; i < 20; ++i)
   {
      dummyContent dc1, dc2;
      engine::vessel::controlFile::head h;
      dc1.a = i;
      rc = f.commit(sizeof(dc1), &dc1);
      ASSERT_EQ(SDB_OK, rc);
      rc = f.readLatestVersion(&h, &dc2);
      ASSERT_EQ(SDB_OK, rc);
      ASSERT_EQ(h.userType, 100);
      ASSERT_EQ(h.sequence, i+1);
      ASSERT_EQ(h.contentLen, sizeof(dummyContent));
      ASSERT_TRUE(h.valid());
      ASSERT_EQ(dc2.a, i);
   }
   
   f.close();
}

/// write, close and reopen
TEST_F(cftest, test2)
{
   INT32 rc = SDB_OK;
   testControlFile f(10);
   rc = f.open(TEST_PATH);
   ASSERT_EQ(SDB_OK, rc);

   dummyContent dc;
   engine::vessel::controlFile::head h;
   dc.a = 1000;
   rc = f.commit(sizeof(dc), &dc);
   ASSERT_EQ(SDB_OK, rc);
   f.close();

   dc.a = 0;
   rc = f.open(TEST_PATH);
   ASSERT_EQ(SDB_OK, rc);

   rc = f.readLatestVersion(&h, &dc);
   ASSERT_EQ(SDB_OK, rc);
   ASSERT_EQ(h.userType, 100);
   ASSERT_EQ(h.sequence, 1);
   ASSERT_EQ(h.contentLen, sizeof(dummyContent));
   ASSERT_TRUE(h.valid());
   ASSERT_EQ(dc.a, 1000);

   dc.a = 0;
   h.contentLen = 0;
   rc = f.readLatestVersion(&h, &dc);
   ASSERT_EQ(SDB_OK, rc);
   ASSERT_EQ(h.userType, 100);
   ASSERT_EQ(h.sequence, 1);
   ASSERT_EQ(h.contentLen, sizeof(dummyContent));
   ASSERT_TRUE(h.valid());
   ASSERT_EQ(dc.a, 1000);
   f.close();

}

/// loop write and read oldest
TEST_F(cftest, test3)
{
   INT32 rc = SDB_OK;
   testControlFile f(10);
   rc = f.open(TEST_PATH);
   ASSERT_EQ(SDB_OK, rc);

   for (UINT32 i = 0; i < 10; ++i)
   {
      dummyContent dc1;
      engine::vessel::controlFile::head h;
      dc1.a = i;
      rc = f.commit(sizeof(dc1), &dc1);
      ASSERT_EQ(SDB_OK, rc);
   }

   for (UINT32 i = 10; i < 20; ++i)
   {
      dummyContent dc1, dc2;
      engine::vessel::controlFile::head h;
      dc1.a = i;
      rc = f.commit(sizeof(dc1), &dc1);
      ASSERT_EQ(SDB_OK, rc);
      rc = f.readOldestVersion(&h, &dc2);
      ASSERT_EQ(SDB_OK, rc);
      ASSERT_EQ(h.userType, 100);
      ASSERT_EQ(h.sequence, i-8);
      ASSERT_EQ(h.contentLen, sizeof(dummyContent));
      ASSERT_TRUE(h.valid());
      ASSERT_EQ(dc2.a, i - 9);
   }
   
   f.close();
}

/// loop write and reopen
TEST_F(cftest, test4)
{
   INT32 rc = SDB_OK;
   testControlFile f(10);
   rc = f.open(TEST_PATH);
   ASSERT_EQ(SDB_OK, rc);

   for (UINT32 i = 0; i < 50; ++i)
   {
      dummyContent dc1;
      engine::vessel::controlFile::head h;
      dc1.a = i;
      rc = f.commit(sizeof(dc1), &dc1);
      ASSERT_EQ(SDB_OK, rc);
   }

   f.close();
   rc = f.open(TEST_PATH);
   ASSERT_EQ(SDB_OK, rc);

   {
   dummyContent dc;
   engine::vessel::controlFile::head h;
   rc = f.readLatestVersion(&h, &dc);
   ASSERT_EQ(SDB_OK, rc);
   ASSERT_EQ(h.userType, 100);
   ASSERT_EQ(h.sequence, 50);
   ASSERT_EQ(h.contentLen, sizeof(dummyContent));
   ASSERT_TRUE(h.valid());
   ASSERT_EQ(dc.a, 49);
   }

   {
   dummyContent dc;
   engine::vessel::controlFile::head h;
   rc = f.readOldestVersion(&h, &dc);
   ASSERT_EQ(SDB_OK, rc);
   ASSERT_EQ(h.userType, 100);
   ASSERT_EQ(h.sequence, 41);
   ASSERT_EQ(h.contentLen, sizeof(dummyContent));
   ASSERT_TRUE(h.valid());
   ASSERT_EQ(dc.a, 40);
   }

   for (UINT32 i = 40; i < 50; ++i)
   {
      dummyContent dc;
      engine::vessel::controlFile::head h;
      rc = f.read(i+1, &h, &dc);
      ASSERT_EQ(SDB_OK, rc);
      ASSERT_EQ(h.userType, 100);
      ASSERT_EQ(h.sequence, i+1);
      ASSERT_EQ(h.contentLen, sizeof(dummyContent));
      ASSERT_TRUE(h.valid());
      ASSERT_EQ(dc.a, i);
   }
   
   f.close();
}

/// loop write and readlatest, only one file
TEST_F(cftest, test5)
{
   INT32 rc = SDB_OK;
   testControlFile f(1);
   rc = f.open(TEST_PATH);
   ASSERT_EQ(SDB_OK, rc);

   rc = f.readOldestVersion(NULL, NULL);
   ASSERT_EQ(SDB_VESSEL_CF_INVALID_SEQUENCE, rc);

   rc = f.readLatestVersion(NULL, NULL);
   ASSERT_EQ(SDB_VESSEL_CF_INVALID_SEQUENCE, rc);

   for (UINT32 i = 0; i < 20; ++i)
   {
      dummyContent dc1, dc2;
      engine::vessel::controlFile::head h;
      dc1.a = i;
      rc = f.commit(sizeof(dc1), &dc1);
      ASSERT_EQ(SDB_OK, rc);
      rc = f.readLatestVersion(&h, &dc2);
      ASSERT_EQ(SDB_OK, rc);
      ASSERT_EQ(h.userType, 100);
      ASSERT_EQ(h.sequence, i+1);
      ASSERT_EQ(h.contentLen, sizeof(dummyContent));
      ASSERT_TRUE(h.valid());
      ASSERT_EQ(dc2.a, i);
   }
   
   f.close();

   {
   dummyContent dc;
   engine::vessel::controlFile::head h;
   rc = f.open(TEST_PATH);
   ASSERT_EQ(SDB_OK, rc);

   rc = f.readOldestVersion(&h, &dc);
   ASSERT_EQ(SDB_OK, rc);
   ASSERT_EQ(h.userType, 100);
   ASSERT_EQ(h.sequence, 20);
   ASSERT_EQ(h.contentLen, sizeof(dummyContent));
   ASSERT_TRUE(h.valid());
   ASSERT_EQ(dc.a, 19);

   rc = f.readLatestVersion(NULL, NULL);
   ASSERT_EQ(SDB_OK, rc);
   ASSERT_EQ(h.userType, 100);
   ASSERT_EQ(h.sequence, 20);
   ASSERT_EQ(h.contentLen, sizeof(dummyContent));
   ASSERT_TRUE(h.valid());
   ASSERT_EQ(dc.a, 19);
   }
}