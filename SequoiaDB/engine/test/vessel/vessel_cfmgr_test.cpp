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

   Source File Name = vessel_cfmgr_test.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          06/16/2022  ZHY Initial Draft
   Last Changed =

******************************************************************************/

#include "ossMemPool.hpp"
#include "test_def.h"
#include "vessel/multiControlFilesMgr.h"
#include "gtest/gtest.h"
#include "utilStr.hpp"
#include <algorithm>
#include <string>
#include <vector>
#include "vessel/fileNameLoader.h"

namespace fs = boost::filesystem;
const CHAR *controlFilePrefix = "testControlFile";
class vessel_cfmgr_test : public testing::Test
{
public:
   static void SetUpTestCase()
   {
      {
         fs::path testPath(DATA_PATH);
         fs::remove_all(testPath);
         fs::create_directory(testPath);
         sdbEnablePD("/opt/diaglog/sdb.log", 1, 1000);
         setPDLevel(PDDEBUG);
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

BOOLEAN validateAndExtract(const std::string &path, UINT32 &version)
   {
      fs::path filePath(path);
      std::vector<std::string> columns = utilStrSplit(filePath.filename().string(), ".");
      if (filePath.filename().string().size() > CONTROL_FILE_MAX_NAME_LEN)
      {
         return FALSE;
      }
      if (columns.size() == CONTROL_FILE_NAME_FORMAT_COLUMNS)
      {
         return FALSE;
      }
      if (!utilStrIsDigit(columns.at(1).c_str()))
      {
         return FALSE;
      }
      if (columns.at(0).compare(controlFilePrefix))
      {
         return FALSE;
      }
      version = std::stoul(columns.at(1));
      return TRUE;
   }

void controlFileTestCreateThreeFiles()
{
   INT32 rc = SDB_OK;
   multiControlFilesMgr mgr;
   mgr.setMaxValidFilesNum(3);
   mgr.load(controlFilePrefix, DATA_PATH, TRUE);
   const CHAR *buf = "control file content";
   rc = mgr.createFile(buf, strlen(buf));
   ASSERT_EQ(SDB_OK, rc);
   rc = mgr.createFile(buf, strlen(buf));
   ASSERT_EQ(SDB_OK, rc);
   rc = mgr.createFile(buf, strlen(buf));
   ASSERT_EQ(SDB_OK, rc);
}

void loadFilenamesAndCheckExtension(const std::vector<std::string> &versions)
{
   INT32 rc = SDB_OK;
   fileNameLoader loader;
   fileNameList fnl;
   rc = loader.loadFileNameList(DATA_PATH, fnl);
   ASSERT_EQ(SDB_OK, rc);
   ossPoolList<std::string> paths = fnl.getFullFilePaths();
   ASSERT_EQ(versions.size(), paths.size());
   for (auto it = paths.begin(); it != paths.end(); it++)
   {
      UINT32 version = 0;
      fs::path path(*it);
      if (validateAndExtract(*it, version))
      {
         BOOLEAN r = std::binary_search(versions.begin(), versions.end(), path.extension());
         ASSERT_EQ(r, TRUE);
      }
   }
}

/*
Name: base_create
Description: 
   multiControlMgr创建文件测试
   1. 创建control file并写入content
   2. 扫描目录中的文件
Expected Result: 
   目录中存在版本号为0的文件
*/
TEST_F(vessel_cfmgr_test, base_create)
{
   INT32 rc = SDB_OK;
   multiControlFilesMgr mgr;
   mgr.load(controlFilePrefix, DATA_PATH, TRUE);
   const CHAR *buf = "control file content";
   rc = mgr.createFile(buf, strlen(buf));
   ASSERT_EQ(SDB_OK, rc);
   loadFilenamesAndCheckExtension({".000000"});
}

/*
Name: base_create_overnum
Description: 
   multiControlMgr创建文件时，超出文件数量上限测试
   1. 设定文件数量上限为3
   2. 创建4个control file并写入content
   3. 扫描目录中的文件
Expected Result: 
   目录中存在版本号为1,2,3的文件
*/
TEST_F(vessel_cfmgr_test, base_create_overnum)
{
   INT32 rc = SDB_OK;
   multiControlFilesMgr mgr;
   mgr.setMaxValidFilesNum(3);
   rc = mgr.load(controlFilePrefix, DATA_PATH, TRUE);
   ASSERT_EQ(SDB_OK, rc);
   const CHAR *buf = "control file content";
   rc = mgr.createFile(buf, strlen(buf));
   ASSERT_EQ(SDB_OK, rc);
   rc = mgr.createFile(buf, strlen(buf));
   ASSERT_EQ(SDB_OK, rc);
   rc = mgr.createFile(buf, strlen(buf));
   ASSERT_EQ(SDB_OK, rc);
   rc = mgr.createFile(buf, strlen(buf));
   ASSERT_EQ(SDB_OK, rc);
   loadFilenamesAndCheckExtension({".000001", ".000002", ".000003"});
}

/*
Name: base_reload
Description: 
   multiControlMgr重新载入时，已存在文件数超过文件数量上限
   1. 创建3个control file
   2. 用新的mgr重新载入，文件数量上限默认为1
   3. 扫描目录中的文件
Expected Result: 
   目录中仅存在版本号为2的文件
*/
TEST_F(vessel_cfmgr_test, base_reload)
{
   INT32 rc = SDB_OK;
   controlFileTestCreateThreeFiles();
   ASSERT_EQ(SDB_OK, rc);
   multiControlFilesMgr mgr;
   rc = mgr.load(controlFilePrefix, DATA_PATH, TRUE);
   ASSERT_EQ(SDB_OK, rc);
   loadFilenamesAndCheckExtension({".000002"});
}

/*
Name: base_current_file
Description: 
   multiControlMgr载入时获取当前文件的内容(确定存在有效文件)
   1. 创建3个文件
   2. 设定TRUE.currentFile
   3. 使用此TRUE载入
   4. 检查文件内容
Expected Result: 
   文件内容与创建时的内容相同
*/
TEST_F(vessel_cfmgr_test, base_current_file)
{
   INT32 rc = SDB_OK;
   controlFileTestCreateThreeFiles();
   multiControlFilesMgr mgr;
   memoryBlock block;
   mgr.setMaxValidFilesNum(3);
   rc = mgr.load(controlFilePrefix, DATA_PATH, TRUE);
   ASSERT_EQ(SDB_OK, rc);
   loadFilenamesAndCheckExtension({".000000", ".000001", ".000002"});
   rc = mgr.getCurrentVersionFileContent(block);
   ASSERT_EQ(SDB_OK, rc);
   ASSERT_EQ(0, strncmp(block.getBuffer(), "control file content", block.getSize()));
}

/*
Name: base_list
Description: 
   multiControlMgr获取文件元数据列表
   1. 创建3个文件
   2. 获取元数据列表
   3. 检查元数据列表中的版本号
Expected Result: 
   元数据列表中的版本号为0,1,2
*/
TEST_F(vessel_cfmgr_test, base_list)
{
   INT32 rc = SDB_OK;
   multiControlFilesMgr mgr;
   mgr.setMaxValidFilesNum(3);
   rc = mgr.load(controlFilePrefix, DATA_PATH, TRUE);
   const CHAR *buf = "control file content";
   rc = mgr.createFile(buf, strlen(buf));
   ASSERT_EQ(SDB_OK, rc);
   rc = mgr.createFile(buf, strlen(buf));
   ASSERT_EQ(SDB_OK, rc);
   rc = mgr.createFile(buf, strlen(buf));
   ASSERT_EQ(SDB_OK, rc);
   loadFilenamesAndCheckExtension({".000000", ".000001", ".000002"});
   ossPoolList<bson::BSONObj> l;
   mgr.list(l);
   ASSERT_EQ(SDB_OK, rc);
   INT32 versions[3]{0, 1, 2};
   UINT32 i = 0;
   for (auto it = l.begin(); it != l.end(); it++)
   {
      ASSERT_EQ(it->getField(CONTROL_FILE_FILEDNAME_COMMIT_VERSION).numberInt(), versions[i]);
      i++;
   }
}

/*
Name: base_invalid_file
Description: 
   multiControlMgr获取文件元数据列表
   1. 创建三个文件
   2. 修改version为1的文件内容
   3. 设置最大有效文件数为3,重新载入,且deleteInvalidFiles=TRUE
   4. 扫描目录中的文件
Expected Result: 
   目录中仅存在版本号为0和2的文件
*/
TEST_F(vessel_cfmgr_test, base_delete_invalid_file)
{
   INT32 rc = SDB_OK;
   multiControlFilesMgr mgr;
   controlFileTestCreateThreeFiles();
   loadFilenamesAndCheckExtension({".000000", ".000001", ".000002"});
   mgr.setMaxValidFilesNum(3);
   std::string FILE_PATH(DATA_PATH);
   FILE_PATH.append(controlFilePrefix);
   FILE_PATH.append(".000001");
   OSSFILE fileDesc;
   rc = ossOpen(FILE_PATH.c_str(), OSS_READWRITE, OSS_DEFAULTFILE, fileDesc);
   ASSERT_EQ(SDB_OK, rc);
   const CHAR *content = "Unexpected edit";
   rc = ossWriteN(&fileDesc, content, ossStrlen(content));
   ASSERT_EQ(SDB_OK ,rc);
   rc = ossClose(fileDesc);
   ASSERT_EQ(SDB_OK ,rc);
   rc = mgr.load(controlFilePrefix, DATA_PATH, TRUE);
   ASSERT_EQ(SDB_OK, rc);
   loadFilenamesAndCheckExtension({".000000", ".000002"});
}

/*
Name: base_not_delete_invalid_file
Description: 
   multiControlMgr获取文件元数据列表
   1. 创建三个文件
   2. 修改version为1的文件内容
   3. 设置最大有效文件数为3,重新载入,且deleteInvalidFiles=FALSE
   4. 扫描目录中的文件
   5. 获取mgr的当前有效文件数
Expected Result: 
   目录中存在版本号为0,1,2的文件;当前有效文件数为2
*/
TEST_F(vessel_cfmgr_test, base_not_delete_invalid_file)
{
   INT32 rc = SDB_OK;
   multiControlFilesMgr mgr;
   controlFileTestCreateThreeFiles();
   loadFilenamesAndCheckExtension({".000000", ".000001", ".000002"});
   mgr.setMaxValidFilesNum(3);
   std::string FILE_PATH(DATA_PATH);
   FILE_PATH.append(controlFilePrefix);
   FILE_PATH.append(".000001");
   OSSFILE fileDesc;
   rc = ossOpen(FILE_PATH.c_str(), OSS_READWRITE, OSS_DEFAULTFILE, fileDesc);
   ASSERT_EQ(SDB_OK, rc);
   const CHAR *content = "Unexpected edit";
   rc = ossWriteN(&fileDesc, content, ossStrlen(content));
   ASSERT_EQ(SDB_OK ,rc);
   rc = ossClose(fileDesc);
   ASSERT_EQ(SDB_OK ,rc);
   rc = mgr.load(controlFilePrefix, DATA_PATH, FALSE);
   ASSERT_EQ(SDB_OK, rc);
   loadFilenamesAndCheckExtension({".000000", ".000001", ".000002"});
   ASSERT_EQ(mgr.getValidFilesNum(), 2);
}