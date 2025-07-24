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

   Source File Name = filename_test.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

*******************************************************************************/
#include "vessel/storageFileName.h"
#include "vessel/storageUtils.h"
#include <gtest/gtest.h>

using namespace engine::vessel;

TEST(filename_test, base_build_test1)
{
   storageFileName fn;
   ASSERT_TRUE(fn.build(FILE_TYPE_LPM, SPACE_TYPE_MAIN_DATA, 0));
   ASSERT_EQ(0, ossStrcmp(fn.getFileName(), "data.lpm.000000"));
   ASSERT_EQ(fn.getSequence(), 0);
   ASSERT_EQ(fn.getFileType(), FILE_TYPE_LPM);
   ASSERT_EQ(fn.getSpaceType(), SPACE_TYPE_MAIN_DATA);

   ASSERT_TRUE(fn.build(FILE_TYPE_LPM, SPACE_TYPE_MAIN_DATA, 1000000));
   ASSERT_EQ(0, ossStrcmp(fn.getFileName(), "data.lpm.1000000"));
   ASSERT_EQ(fn.getSequence(), 1000000);
   ASSERT_EQ(fn.getFileType(), FILE_TYPE_LPM);
   ASSERT_EQ(fn.getSpaceType(), SPACE_TYPE_MAIN_DATA);

   ASSERT_TRUE(fn.build(FILE_TYPE_LPM, SPACE_TYPE_IDX, 101));
   ASSERT_EQ(0, ossStrcmp(fn.getFileName(), "idx.lpm.000101"));
   ASSERT_EQ(fn.getSequence(), 101);
   ASSERT_EQ(fn.getFileType(), FILE_TYPE_LPM);
   ASSERT_EQ(fn.getSpaceType(), SPACE_TYPE_IDX);

   ASSERT_TRUE(fn.build(FILE_TYPE_DATA_STORAGE, SPACE_TYPE_LOB, 361, FILE_SHADOW_SUFFIX_TMP));
   ASSERT_EQ(0, ossStrcmp(fn.getFileName(), "lob.ds.000361._tmp"));
   ASSERT_EQ(fn.getSequence(), 361);
   ASSERT_EQ(fn.getFileType(), FILE_TYPE_DATA_STORAGE);
   ASSERT_EQ(fn.getSpaceType(), SPACE_TYPE_LOB);
   ASSERT_EQ(fn.getShadowSuffix(), FILE_SHADOW_SUFFIX_TMP);

   ASSERT_FALSE(fn.build(INVALID_FILE_TYPE, SPACE_TYPE_MAIN_DATA, 0));
   ASSERT_FALSE(fn.build(FILE_TYPE_LPM, INVALID_SPACE_TYPE, 0));
}

TEST(filename_test, base_extract_test1)
{
   storageFileName fn;
   ASSERT_TRUE(fn.extract(strSlice("data.lpm.000000")));
   ASSERT_EQ(fn.getSequence(), 0);
   ASSERT_EQ(fn.getFileType(), FILE_TYPE_LPM);
   ASSERT_EQ(fn.getSpaceType(), SPACE_TYPE_MAIN_DATA);

   ASSERT_TRUE(fn.extract(strSlice("idx.lpm.000111")));
   ASSERT_EQ(fn.getSequence(), 111);
   ASSERT_EQ(fn.getFileType(), FILE_TYPE_LPM);
   ASSERT_EQ(fn.getSpaceType(), SPACE_TYPE_IDX);

   ASSERT_TRUE(fn.extract(strSlice("lob.lpm.999999")));
   ASSERT_EQ(fn.getSequence(), 999999);
   ASSERT_EQ(fn.getFileType(), FILE_TYPE_LPM);
   ASSERT_EQ(fn.getSpaceType(), SPACE_TYPE_LOB);

   ASSERT_TRUE(fn.extract(strSlice("data.ds.1000000")));
   ASSERT_EQ(fn.getSequence(), 1000000);
   ASSERT_EQ(fn.getFileType(), FILE_TYPE_DATA_STORAGE);
   ASSERT_EQ(fn.getSpaceType(), SPACE_TYPE_MAIN_DATA);

   ASSERT_TRUE(fn.extract(strSlice("idx.ds.000001")));
   ASSERT_EQ(fn.getSequence(), 1);
   ASSERT_EQ(fn.getFileType(), FILE_TYPE_DATA_STORAGE);
   ASSERT_EQ(fn.getSpaceType(), SPACE_TYPE_IDX);

   ASSERT_TRUE(fn.extract(strSlice("lob.ds.000000")));
   ASSERT_EQ(fn.getSequence(), 0);
   ASSERT_EQ(fn.getFileType(), FILE_TYPE_DATA_STORAGE);
   ASSERT_EQ(fn.getSpaceType(), SPACE_TYPE_LOB);

   ASSERT_TRUE(fn.extract(strSlice("lob.ds.000001._tmp"), TRUE));
   ASSERT_EQ(fn.getSequence(), 1);
   ASSERT_EQ(fn.getFileType(), FILE_TYPE_DATA_STORAGE);
   ASSERT_EQ(fn.getSpaceType(), SPACE_TYPE_LOB);
   ASSERT_TRUE(fn.hasShadowSuffix());

   ASSERT_TRUE(fn.extract(strSlice("data.ds.000001.")));
   ASSERT_EQ(0, ossStrcmp(fn.getFileName(), "data.ds.000001"));

   ASSERT_TRUE(fn.extract(strSlice("data.ds.000001."), TRUE));
   ASSERT_EQ(0, ossStrcmp(fn.getFileName(), "data.ds.000001"));
}

// extract abnormal filename
TEST(filename_test, base_extract_test2)
{
   storageFileName fn;
   // wrong
   ASSERT_FALSE(fn.extract(strSlice("*.ds.000001._tmp")));
   ASSERT_FALSE(fn.extract(strSlice("data.*.ds._tmp")));
   ASSERT_FALSE(fn.extract(strSlice("data.ds.*._tmp")));
   ASSERT_FALSE(fn.extract(strSlice("data.ds.000001.*")));

   // missing
   ASSERT_FALSE(fn.extract(strSlice("")));
   ASSERT_FALSE(fn.extract(strSlice(".ds.000001")));
   ASSERT_FALSE(fn.extract(strSlice("data..000001")));
   ASSERT_FALSE(fn.extract(strSlice("data.ds.")));
}

// TEST(filename_test, dir_test)
// buildDirName
TEST(filename_test, base_dir_test1)
{
   storageFileName fn;
   CHAR buf[MAX_SPACE_DIR_LEN + 1] = {'\0'};
   ASSERT_TRUE(buildSpaceDirName(0, MAX_SPACE_DIR_LEN + 1, buf));
   ASSERT_EQ(0, ossStrcmp(buf, "_cs_00000"));
}

TEST(filename_test, base_dir_test2)
{
   storageFileName fn;
   CHAR buf[MAX_SPACE_DIR_LEN + 1] = {'\0'};
   ASSERT_FALSE(buildSpaceDirName(1, MAX_SPACE_DIR_LEN, buf)); 
}

// parseDirName
TEST(filename_test, base_dir_test3)
{
   storageFileName fn;
   SPACE_ID *sid = NULL;
   ASSERT_TRUE(parseSpaceDirName(strSlice("_cs_00000"), sid));
   ASSERT_TRUE(parseSpaceDirName(strSlice("_cs_16383"), sid));
}

TEST(filename_test, base_dir_test4)
{
   storageFileName fn;
   SPACE_ID *sid = NULL;
   ASSERT_FALSE(parseSpaceDirName(strSlice("_cs_"), sid));
   ASSERT_FALSE(parseSpaceDirName(strSlice("_cs_16384"), sid));
   ASSERT_FALSE(parseSpaceDirName(strSlice("_cs_abc"), sid));
   ASSERT_FALSE(parseSpaceDirName(strSlice("_cs*"), sid));
   ASSERT_FALSE(parseSpaceDirName(strSlice(""), sid));
   ASSERT_FALSE(parseSpaceDirName(strSlice("xxx"), sid));
}