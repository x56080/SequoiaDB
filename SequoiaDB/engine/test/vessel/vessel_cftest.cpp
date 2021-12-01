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
#include "ossFile.hpp"
#include "utilStr.hpp"

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
      sdbEnablePD("/tmp/sdb.log", 1, 1000);
      setPDLevel(PDDEBUG);
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
      virtual BOOLEAN getFileName(UINT32 i,
                                  std::string &name) const
      {
         std::stringstream ss;
         ss << "unit_test." << i;
         name = ss.str();
         return TRUE;
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
   v::strSlice dirSlice(TEST_PATH); 
   testControlFile file(1);
   rc = file.open(dirSlice);
   ASSERT_EQ(SDB_OK, rc);
   dummyContent content;
   UINT64 version = 0;

   rc = file.readOldestVersion(version, sizeof(content), &content);
   ASSERT_EQ(SDB_VESSEL_CF_VERSION_NOT_AVAILABLE, rc);

   rc = file.readLatestVersion(version, sizeof(content), &content);
   ASSERT_EQ(SDB_VESSEL_CF_VERSION_NOT_AVAILABLE, rc);

   for (UINT32 i = 0; i < 32; ++i)
   {
      dummyContent commit;
      commit.a = i;
      rc = file.commit(sizeof(commit), &commit);
      ASSERT_EQ(SDB_OK, rc);
      rc = file.readLatestVersion(version, sizeof(dummyContent), &content);
      ASSERT_EQ(SDB_OK, rc);
      ASSERT_EQ(version, i);
      ASSERT_EQ(content.a, i);

      rc = file.readOldestVersion(version, sizeof(dummyContent), &content);
      ASSERT_EQ(SDB_OK, rc);
      ASSERT_EQ(version, i);
      ASSERT_EQ(content.a, i);
   }
   
   file.close();
}

/// loop write and read with multi versions
TEST_F(cftest, test1)
{
   INT32 rc = SDB_OK;
   v::strSlice dirSlice(TEST_PATH); 
   testControlFile f(16);
   rc = f.open(dirSlice);
   ASSERT_EQ(SDB_OK, rc);
   dummyContent content;
   UINT64 version = 0;

   rc = f.readOldestVersion(version, sizeof(content), &content);
   ASSERT_EQ(SDB_VESSEL_CF_VERSION_NOT_AVAILABLE, rc);

   rc = f.readLatestVersion(version, sizeof(content), &content);
   ASSERT_EQ(SDB_VESSEL_CF_VERSION_NOT_AVAILABLE, rc);

   for (UINT32 i = 0; i < 32; ++i)
   {
      dummyContent commit;
      commit.a = i;
      rc = f.commit(sizeof(commit), &commit);
      ASSERT_EQ(SDB_OK, rc);
      rc = f.readLatestVersion(version, sizeof(dummyContent), &content);
      ASSERT_EQ(SDB_OK, rc);
      ASSERT_EQ(version, i);
      ASSERT_EQ(content.a, i);

      rc = f.readOldestVersion(version, sizeof(dummyContent), &content);
      ASSERT_EQ(SDB_OK, rc);
      if (i < 16)
      {
         ASSERT_EQ(version, 0);
         ASSERT_EQ(content.a, 0);
      }
      else
      {
         ASSERT_EQ(version, i - 16 + 1);
         ASSERT_EQ(content.a, i - 16 + 1);
      }
   }
   
   f.close();
}

/// write, close and reopen
TEST_F(cftest, test2)
{
   INT32 rc = SDB_OK;
   v::strSlice dirSlice(TEST_PATH); 
   testControlFile f(16);
   rc = f.open(dirSlice);
   ASSERT_EQ(SDB_OK, rc);
   UINT64 version = 0;
   dummyContent content;

   for (UINT32 i = 0; i < 32; ++i)
   {
      dummyContent commit;
      commit.a = i;
      rc = f.commit(sizeof(commit), &commit);
      ASSERT_EQ(SDB_OK, rc);
   }

   f.close();
   rc = f.open(dirSlice);
   ASSERT_EQ(SDB_OK, rc);
   rc = f.readLatestVersion(version, sizeof(dummyContent), &content);
   ASSERT_EQ(SDB_OK, rc);
   ASSERT_EQ(version, 31);
   ASSERT_EQ(content.a, 31);

   rc = f.readOldestVersion(version, sizeof(dummyContent), &content);
   ASSERT_EQ(SDB_OK, rc);
   ASSERT_EQ(version, 16);
   ASSERT_EQ(content.a, 16);

   for (UINT32 i = 0; i < 16 ; ++i)
   {
      rc = f.readPreVersion(i, version, sizeof(dummyContent), &content);
      ASSERT_EQ(SDB_OK, rc);
      ASSERT_EQ(version, 31 - i);
      ASSERT_EQ(content.a, 31 - i);
   }

   rc = f.readPreVersion(16, version, sizeof(dummyContent), &content);
   ASSERT_EQ(SDB_VESSEL_CF_VERSION_NOT_AVAILABLE, rc);

   f.close();
}

// file crashed with only one version
TEST_F(cftest, test3)
{
   INT32 rc = SDB_OK;
   v::strSlice dirSlice(TEST_PATH); 
   testControlFile f(1);
   rc = f.open(dirSlice);
   ASSERT_EQ(SDB_OK, rc);
   dummyContent content;
   UINT64 version = 0;

   
   rc = f.readOldestVersion(version, sizeof(content), &content);
   ASSERT_EQ(SDB_VESSEL_CF_VERSION_NOT_AVAILABLE, rc);

   rc = f.readLatestVersion(version, sizeof(content), &content);
   ASSERT_EQ(SDB_VESSEL_CF_VERSION_NOT_AVAILABLE, rc);

   dummyContent commit;
   rc = f.commit(sizeof(commit), &commit);
   ASSERT_EQ(SDB_OK, rc);
   rc = f.readLatestVersion(version, sizeof(dummyContent), &content);
   ASSERT_EQ(SDB_OK, rc);
   f.close();

   CHAR full[OSS_MAX_PATHSIZE + 1] = {'\0'};
   _OSS_FILE crashFile;
   SINT64 written = 0;
   const CHAR tmpBuf[5] = {'C','R','A','S','H'};
   string filename;
   f.getFileName(0, filename);
   rc = engine::utilBuildFullPath(TEST_PATH, filename.c_str(), 
                                  OSS_MAX_PATHSIZE + 1, full);
   ASSERT_EQ(SDB_OK, rc);
   rc = ossOpen(full, OSS_WRITEONLY, OSS_DEFAULTFILE, crashFile);
   ASSERT_EQ(SDB_OK, rc);
   rc = ossSeekAndWrite(&crashFile, 0, tmpBuf, sizeof(tmpBuf), &written);
   ASSERT_EQ(SDB_OK, rc);
   rc = ossClose(crashFile);
   ASSERT_EQ(SDB_OK, rc);

   rc = f.open(dirSlice);
   ASSERT_EQ(SDB_OK, rc);
   rc = f.readLatestVersion(version, sizeof(dummyContent), &content);
   ASSERT_EQ(SDB_VESSEL_CF_VERSION_NOT_AVAILABLE, rc);

   f.close();

}

// file crashed with multi versions
TEST_F(cftest, test4)
{
   INT32 rc = SDB_OK;
   v::strSlice dirSlice(TEST_PATH);
   testControlFile f(16);
   dummyContent content;
   UINT64 version = 0;

   rc = f.open(dirSlice);
   ASSERT_EQ(SDB_OK, rc);
   for (UINT32 i = 0; i < 16; ++i)
   {
      dummyContent commit;
      commit.a = i;
      rc = f.commit(sizeof(commit), &commit);
      ASSERT_EQ(SDB_OK, rc);
   }
   f.close();

   CHAR full[OSS_MAX_PATHSIZE + 1] = {'\0'};
   string filename;
   _OSS_FILE crashFile;
   SINT64 written = 0;
   const CHAR tmpBuf[5] = {'C','R','A','S','H'};
   for (UINT32 i = 0; i < 8; ++i)
   {
      f.getFileName(i, filename);
      rc = engine::utilBuildFullPath(TEST_PATH, filename.c_str(), 
                                     OSS_MAX_PATHSIZE + 1, full);
      ASSERT_EQ(SDB_OK, rc);
      rc = ossOpen(full, OSS_WRITEONLY, OSS_DEFAULTFILE, crashFile);
      ASSERT_EQ(SDB_OK, rc);
      rc = ossSeekAndWrite(&crashFile, 5,  tmpBuf, 5, &written);
      ASSERT_EQ(SDB_OK, rc);
      rc = ossClose(crashFile);
      ASSERT_EQ(SDB_OK, rc);
   }

   rc = f.open(dirSlice);
   ASSERT_EQ(SDB_OK, rc);
   rc = f.readLatestVersion(version, sizeof(dummyContent), &content);
   ASSERT_EQ(SDB_OK, rc);
   ASSERT_EQ(version, 15);
   ASSERT_EQ(content.a, 15);
   rc = f.readOldestVersion(version, sizeof(dummyContent), &content);
   ASSERT_EQ(SDB_OK, rc);

   // test the last 8 files
   for (UINT32 i = 0; i < 8; ++i)
   {
      rc = f.readPreVersion(i, version, sizeof(dummyContent), &content);
      ASSERT_EQ(SDB_OK, rc);
   }
   
   // test the first 8 crashed files
   for (UINT32 i = 8; i < 16; ++i)
   {
      rc = f.readPreVersion(i, version, sizeof(dummyContent), &content);
      ASSERT_EQ(SDB_VESSEL_CF_VERSION_NOT_AVAILABLE, rc);
   }
   
   f.close();
}

// remove files
TEST_F(cftest, test5)
{
   INT32 rc = SDB_OK;
   v::strSlice dirSlice(TEST_PATH); 
   testControlFile f(16);
   rc = f.open(dirSlice);
   ASSERT_EQ(SDB_OK, rc);
   dummyContent content;
   UINT64 version = 0;

   rc = f.readOldestVersion(version, sizeof(content), &content);
   ASSERT_EQ(SDB_VESSEL_CF_VERSION_NOT_AVAILABLE, rc);

   rc = f.readLatestVersion(version, sizeof(content), &content);
   ASSERT_EQ(SDB_VESSEL_CF_VERSION_NOT_AVAILABLE, rc);

   for (UINT32 i = 0; i < 16; ++i)
   {
      dummyContent commit;
      commit.a = i;
      rc = f.commit(sizeof(commit), &commit);
      ASSERT_EQ(SDB_OK, rc);
   }
   f.close();


   string filename;
   CHAR full[OSS_MAX_PATHSIZE + 1] = {'\0'};
   for (UINT32 i = 0; i < 16; i += 2)
   {
      f.getFileName(i, filename);
      rc = engine::utilBuildFullPath(TEST_PATH, filename.c_str(), 
                                     OSS_MAX_PATHSIZE + 1, full);
      ASSERT_EQ(SDB_OK,rc);
      fs::path rmFile(full);
      fs::remove(rmFile);
   }

   f.open(dirSlice);
   rc = f.readOldestVersion(version, sizeof(content), &content);
   ASSERT_EQ(SDB_OK, rc);
   rc = f.readLatestVersion(version, sizeof(content), &content);
   ASSERT_EQ(SDB_OK, rc);
}
