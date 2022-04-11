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

#include "vessel/controlFile.h"
#include "vessel/strSlice.h"
#include <gtest/gtest.h>

#include <boost/filesystem.hpp>
namespace fs = boost::filesystem;
namespace v = engine::vessel;

constexpr CHAR *FILE_PATH = "/opt/unit_test/cfTest";

class vessel_cftest : public testing::Test
{
   public:
   static void SetUpTestCase()
   {
      fs::path testPath(FILE_PATH);
      fs::remove_all(testPath);
   }

   static void TearDownTestCase()
   {
      fs::path testPath(FILE_PATH);
      fs::remove_all(testPath);
   }

   virtual void SetUp()
   {
      fs::path testPath(FILE_PATH);
      fs::remove_all(testPath);
      sdbEnablePD("/tmp/sdb.log", 1, 1000);
      setPDLevel(PDDEBUG);
   }
};

/*
Name: base_create_test
Description: 
   control file文件的创建
   1. 创建controfile文件并写入content
   2. 重复创建同名的文件
   3. 以替换方式创建同名文件
Expected Result: 
   文件创建结果正确
*/
TEST_F(vessel_cftest, base_create_test)
{
   INT32 rc = SDB_OK;
   const CHAR *content = "test";
   v::strSlice fullPath(FILE_PATH);

   rc = v::controlFile::create(fullPath, content, ossStrlen(content));
   ASSERT_EQ(SDB_OK, rc);

   rc = v::controlFile::create(fullPath, content, ossStrlen(content));
   ASSERT_EQ(SDB_FE, rc);

   rc = v::controlFile::create(fullPath, content, ossStrlen(content), TRUE);
   ASSERT_EQ(SDB_OK ,rc);

}

/*
Name: base_read_test
Description: 
   control file读取测试
   1. 直接打开文件，校验文件不存在
   2. 创建文件并写入content
   3. 打开文件并读取content
   4. 校验读取内容正确性
   5. 重新创建该文件并写入新的content
   6. 打开文件并读取content
   7. 校验读取内容正确性
Expected Result: 
   返回结果正确，文件内容读取正确
*/
TEST_F(vessel_cftest, base_read_test)
{
   INT32 rc = SDB_OK;
   const CHAR *content = "test";
   v::strSlice fullPath(FILE_PATH);

   v::controlFile file;
   rc = file.openToRead(fullPath);
   ASSERT_EQ(SDB_FNE, rc);

   rc = v::controlFile::create(fullPath, content, ossStrlen(content), TRUE);
   ASSERT_EQ(SDB_OK ,rc);

   rc = file.openToRead(fullPath);
   ASSERT_EQ(SDB_OK, rc);
   
   UINT32 readSize = file.getContentLen();
   ASSERT_EQ(readSize, ossStrlen(content));

   CHAR res1[readSize];
   rc = file.read(res1, readSize);
   ASSERT_EQ(SDB_OK, rc);
   ASSERT_EQ(string(res1, readSize), content);

   file.close();

   const CHAR *newContent = "newtest";
   rc = v::controlFile::create(fullPath, newContent, ossStrlen(newContent), TRUE);
   ASSERT_EQ(SDB_OK, rc);

   rc = file.openToRead(fullPath);
   ASSERT_EQ(SDB_OK, rc);

   readSize = file.getContentLen();
   ASSERT_EQ(readSize, ossStrlen(newContent));

   CHAR res2[readSize];
   rc = file.read(res2, readSize);
   ASSERT_EQ(string(res2, readSize), newContent);

}

/*
Name: base_crash_test1
Description: 
   control file崩溃测试
   1. 创建control file并写入content
   2. 关闭文件
   3. 覆盖文件头
   4. control file再次打开文件
Expected Result: 
   文件被修改，无法通过control file校验
*/
TEST_F(vessel_cftest, base_crash_test1)
{
   INT32 rc = SDB_OK;
   const CHAR *content = "test";
   v::strSlice fullPath(FILE_PATH);

   v::controlFile file;
   rc = file.create(fullPath, content, ossStrlen(content), TRUE);
   ASSERT_EQ(SDB_OK, rc);

   OSSFILE fileDesc;
   rc = ossOpen(FILE_PATH, OSS_READWRITE, OSS_DEFAULTFILE, fileDesc);
   ASSERT_EQ(SDB_OK, rc);

   rc = ossWriteN(&fileDesc, content, ossStrlen(content));
   ASSERT_EQ(SDB_OK ,rc);

   rc = ossClose(fileDesc);
   ASSERT_EQ(SDB_OK ,rc);

   rc = file.openToRead(fullPath);
   ASSERT_EQ(SDB_VESSEL_FILE_HEAD_CRASHED, rc);

}

/*
Name: base_crash_test2
Description: 
   control file崩溃测试
   1. 创建control file并写入content
   2. 关闭文件
   3. 增加文件长度
   4. control file再次打开文件
Expected Result: 
   文件被修改，无法通过control file校验
*/
TEST_F(vessel_cftest, base_crash_test2)
{
   INT32 rc = SDB_OK;
   const CHAR *content = "test";
   v::strSlice fullPath(FILE_PATH);

   v::controlFile file;
   rc = file.create(fullPath, content, ossStrlen(content), TRUE);
   ASSERT_EQ(SDB_OK, rc);

   OSSFILE fileDesc;
   rc = ossOpen(FILE_PATH, OSS_READWRITE, OSS_DEFAULTFILE, fileDesc);
   ASSERT_EQ(SDB_OK, rc);

   SINT64 written = 0;
   rc = ossSeekAndWriteN(&fileDesc, 100, content, ossStrlen(content), written);
   ASSERT_EQ(SDB_OK ,rc);

   rc = ossClose(fileDesc);
   ASSERT_EQ(SDB_OK ,rc);

   rc = file.openToRead(fullPath);
   ASSERT_EQ(SDB_VESSEL_INVALID_VESSEL_FILE, rc);

}

/*
Name: base_crash_test3
Description: 
   control file崩溃测试
   1. 创建control file并写入content
   2. 关闭文件
   3. 修改写入的content
   4. control file再次打开文件
Expected Result: 
   文件被修改，无法通过control file校验
*/
TEST_F(vessel_cftest, base_crash_test3)
{
   INT32 rc = SDB_OK;
   const CHAR *content = "test";
   v::strSlice fullPath(FILE_PATH);

   v::controlFile file;
   rc = file.create(fullPath, content, ossStrlen(content), TRUE);
   ASSERT_EQ(SDB_OK, rc);

   OSSFILE fileDesc;
   rc = ossOpen(FILE_PATH, OSS_READWRITE, OSS_DEFAULTFILE, fileDesc);
   ASSERT_EQ(SDB_OK, rc);

   SINT64 written = 0;
   const CHAR *newContent = "abcd";
   // '24' is the length of control file head
   rc = ossSeekAndWriteN(&fileDesc, 24, newContent, ossStrlen(newContent), written);
   ASSERT_EQ(SDB_OK ,rc);

   rc = ossClose(fileDesc);
   ASSERT_EQ(SDB_OK ,rc);

   rc = file.openToRead(fullPath);
   ASSERT_EQ(SDB_VESSEL_INVALID_VESSEL_FILE, rc);

}

