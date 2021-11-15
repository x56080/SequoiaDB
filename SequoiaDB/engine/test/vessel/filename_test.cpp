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

   Source File Name = filename_test.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#include "vessel/vesselFileName.h"
#include <gtest/gtest.h>

using namespace engine::vessel;

// TEST(filename_test, extract)
// extract normal filename
TEST(filename_test, extract_test1)
{
   vesselFileName fn;
   ASSERT_TRUE(fn.extract(strSlice("_cs_0.0.idmap.data")));
   ASSERT_EQ(fn.getSpaceID(), 0);
   ASSERT_EQ(fn.getSequence(), 0);
   ASSERT_EQ(fn.getFileType(), FILE_TYPE_ID_MAP);
   ASSERT_EQ(fn.getSpaceType(), SPACE_TYPE_MAIN_DATA);

   ASSERT_TRUE(fn.extract(strSlice("_cs_0.0.idmap.idx")));
   ASSERT_EQ(fn.getSpaceID(), 0);
   ASSERT_EQ(fn.getSequence(), 0);
   ASSERT_EQ(fn.getFileType(), FILE_TYPE_ID_MAP);
   ASSERT_EQ(fn.getSpaceType(), SPACE_TYPE_IDX);

   ASSERT_TRUE(fn.extract(strSlice("_cs_0.0.idmap.lob")));
   ASSERT_EQ(fn.getSpaceID(), 0);
   ASSERT_EQ(fn.getSequence(), 0);
   ASSERT_EQ(fn.getFileType(), FILE_TYPE_ID_MAP);
   ASSERT_EQ(fn.getSpaceType(), SPACE_TYPE_LOB);

   ASSERT_TRUE(fn.extract(strSlice("_cs_1.1.ds.data")));
   ASSERT_EQ(fn.getSpaceID(), 1);
   ASSERT_EQ(fn.getSequence(), 1);
   ASSERT_EQ(fn.getFileType(), FILE_TYPE_DATA_STORAGE);
   ASSERT_EQ(fn.getSpaceType(), SPACE_TYPE_MAIN_DATA);

   ASSERT_TRUE(fn.extract(strSlice("_cs_1.1.ds.idx")));
   ASSERT_EQ(fn.getSpaceID(), 1);
   ASSERT_EQ(fn.getSequence(), 1);
   ASSERT_EQ(fn.getFileType(), FILE_TYPE_DATA_STORAGE);
   ASSERT_EQ(fn.getSpaceType(), SPACE_TYPE_IDX);

   ASSERT_TRUE(fn.extract(strSlice("_cs_1.1.ds.lob")));
   ASSERT_EQ(fn.getSpaceID(), 1);
   ASSERT_EQ(fn.getSequence(), 1);
   ASSERT_EQ(fn.getFileType(), FILE_TYPE_DATA_STORAGE);
   ASSERT_EQ(fn.getSpaceType(), SPACE_TYPE_LOB);

   ASSERT_TRUE(fn.extract(strSlice("_cs_1.1.ds.lob._tmp"), TRUE));
   ASSERT_EQ(fn.getSpaceID(), 1);
   ASSERT_EQ(fn.getSequence(), 1);
   ASSERT_EQ(fn.getFileType(), FILE_TYPE_DATA_STORAGE);
   ASSERT_EQ(fn.getSpaceType(), SPACE_TYPE_LOB);
   ASSERT_TRUE(fn.hasShadowSuffix());

   ASSERT_TRUE(fn.extract(strSlice("_cs_0.1.idmap.data.")));
   ASSERT_EQ(0, ossStrcmp(fn.getFileName(), "_cs_0.1.idmap.data"));

   ASSERT_TRUE(fn.extract(strSlice("_cs_0.1.idmap.data."), TRUE));
   ASSERT_EQ(0, ossStrcmp(fn.getFileName(), "_cs_0.1.idmap.data"));
}

// extract abnormal filename
TEST(filename_test, extract_test2)
{
   vesselFileName fn;
   ASSERT_FALSE(fn.extract(strSlice("_cs_16384.0.idmap.data")));
   ASSERT_FALSE(fn.extract(strSlice("_cs_0.0.idmap1.data")));
   ASSERT_FALSE(fn.extract(strSlice("_cs_0.0.idmap.data1")));
   ASSERT_FALSE(fn.extract(strSlice("_cs_0.1.idmap.data._tmp")));

   // wrong
   ASSERT_FALSE(fn.extract(strSlice("_cs_*.1.idmap.data._tmp")));
   ASSERT_FALSE(fn.extract(strSlice("_cs_0.*.idmap.data._tmp")));
   ASSERT_FALSE(fn.extract(strSlice("_cs_0.1.*.data._tmp")));
   ASSERT_FALSE(fn.extract(strSlice("_cs_0.1.idmap.*._tmp")));
   ASSERT_FALSE(fn.extract(strSlice("_cs_0.1.idmap.data.*")));

   // missing
   ASSERT_FALSE(fn.extract(strSlice("")));
   ASSERT_FALSE(fn.extract(strSlice("_cs_.1.idmap.data")));
   ASSERT_FALSE(fn.extract(strSlice("_cs_0..idmap.data")));
   ASSERT_FALSE(fn.extract(strSlice("_cs_0.1..data")));
   ASSERT_FALSE(fn.extract(strSlice("_cs_0.1.idmap..")));
   ASSERT_FALSE(fn.extract(strSlice("_cs_0.csname")));
}

// TEST(filename_test, build)
// build normal filename
TEST(filename_test, build_test1)
{
   vesselFileName fn;
   ASSERT_TRUE(fn.build(0, FILE_TYPE_SYS, SPACE_TYPE_MAIN_DATA, 0));
   ASSERT_EQ(0, ossStrcmp(fn.getFileName(), "_cs_0.0.sys.data"));
   ASSERT_EQ(fn.getSpaceID(), 0);
   ASSERT_EQ(fn.getSequence(), 0);
   ASSERT_EQ(fn.getFileType(), FILE_TYPE_SYS);
   ASSERT_EQ(fn.getSpaceType(), SPACE_TYPE_MAIN_DATA);

   ASSERT_TRUE(fn.build(100, FILE_TYPE_ID_MAP, SPACE_TYPE_IDX, 101));
   ASSERT_EQ(0, ossStrcmp(fn.getFileName(), "_cs_100.101.idmap.idx"));
   ASSERT_EQ(fn.getSpaceID(), 100);
   ASSERT_EQ(fn.getSequence(), 101);
   ASSERT_EQ(fn.getFileType(), FILE_TYPE_ID_MAP);
   ASSERT_EQ(fn.getSpaceType(), SPACE_TYPE_IDX);

   ASSERT_TRUE(fn.build(298, FILE_TYPE_DATA_STORAGE, SPACE_TYPE_LOB, 361, FILE_SHADOW_SUFFIX_TMP));
   ASSERT_EQ(0, ossStrcmp(fn.getFileName(), "_cs_298.361.ds.lob._tmp"));
   ASSERT_EQ(fn.getSpaceID(), 298);
   ASSERT_EQ(fn.getSequence(), 361);
   ASSERT_EQ(fn.getFileType(), FILE_TYPE_DATA_STORAGE);
   ASSERT_EQ(fn.getSpaceType(), SPACE_TYPE_LOB);
   ASSERT_EQ(fn.getShadowSuffix(), FILE_SHADOW_SUFFIX_TMP);
}

// build abnormal filename  
TEST(filename_test, build_test2)
{
   vesselFileName fn;
   ASSERT_FALSE(fn.build(INVALID_SPACE_ID, FILE_TYPE_ID_MAP, SPACE_TYPE_MAIN_DATA, 0));
   ASSERT_FALSE(fn.build(0, INVALID_FILE_TYPE, SPACE_TYPE_MAIN_DATA, 0));
   ASSERT_FALSE(fn.build(0, FILE_TYPE_ID_MAP, INVALID_SPACE_TYPE, 0));
}

// build border filename
TEST(filename_test, build_test3)
{
   vesselFileName fn;
   ASSERT_TRUE(fn.build(16383, FILE_TYPE_DATA_STORAGE, SPACE_TYPE_MAIN_DATA, 1000000000));
   ASSERT_EQ(0, ossStrcmp(fn.getFileName(), "_cs_16383.1000000000.ds.data"));
   ASSERT_EQ(fn.getSpaceID(), 16383);
   ASSERT_EQ(fn.getSequence(), 1000000000);
   ASSERT_EQ(fn.getFileType(), FILE_TYPE_DATA_STORAGE);
   ASSERT_EQ(fn.getSpaceType(), SPACE_TYPE_MAIN_DATA);

   ASSERT_FALSE(fn.build(MAX_SPACE_ID + 1, FILE_TYPE_DATA_STORAGE, SPACE_TYPE_MAIN_DATA, 1000000000));
}

// rebuild filename without suffix
TEST(filename_test, build_test4)
{
   vesselFileName fn;
   ASSERT_TRUE(fn.build(0, FILE_TYPE_ID_MAP, SPACE_TYPE_IDX, 1, FILE_SHADOW_SUFFIX_TMP));
   ASSERT_TRUE(fn.hasShadowSuffix());

   fn.rebuildWithOutShadowSuffix();
   ASSERT_FALSE(fn.hasShadowSuffix());
}

// build normal simplename
TEST(filename_test, build_test5)
{
   vesselFileName fn;
   CHAR buf[MAX_FILE_NAME_LEN] = {'\0'};
   ASSERT_TRUE(fn.buildSimpleName(0, strSlice("cs"), MAX_FILE_NAME_LEN, buf));
   ASSERT_EQ(0, ossStrcmp(buf, "_cs_0.cs"));
   ASSERT_TRUE(fn.buildSimpleName(16383, strSlice("cs"), MAX_FILE_NAME_LEN, buf));
   ASSERT_EQ(0, ossStrcmp(buf, "_cs_16383.cs"));
   ASSERT_TRUE(fn.buildSimpleName(0, strSlice("cs"), 13, buf));
   ASSERT_EQ(0, ossStrcmp(buf, "_cs_0.cs"));
}

// build abnormal simplename
TEST(filename_test, build_test6)
{
   vesselFileName fn;
   CHAR buf[MAX_FILE_NAME_LEN] = {'\0'};
   ASSERT_FALSE(fn.buildSimpleName(0, strSlice(""), MAX_FILE_NAME_LEN, buf));
   ASSERT_FALSE(fn.buildSimpleName(16384, strSlice("cs"), MAX_FILE_NAME_LEN, buf));
   ASSERT_FALSE(fn.buildSimpleName(INVALID_SPACE_ID, strSlice("cs"), MAX_FILE_NAME_LEN, buf));
   ASSERT_FALSE(fn.buildSimpleName(0, strSlice("cs"), 12, buf));
}

// TEST(filename_test, dir_test)
// buildDirName
TEST(filename_test, dir_test1)
{
   vesselFileName fn;
   CHAR buf[MAX_SPACE_DIR_LEN + 1] = {'\0'};
   ASSERT_TRUE(fn.buildDirName(0, MAX_SPACE_DIR_LEN + 1, buf));
   ASSERT_EQ(0, ossStrcmp(buf, "_cs_0"));
}

TEST(filename_test, dir_test2)
{
   vesselFileName fn;
   CHAR buf[MAX_SPACE_DIR_LEN + 1] = {'\0'};
   ASSERT_FALSE(fn.buildDirName(1, MAX_SPACE_DIR_LEN, buf));
}

// parseDirName
TEST(filename_test, dir_test3)
{
   vesselFileName fn;
   SPACE_ID *sid = NULL;
   ASSERT_TRUE(fn.parseDirName(strSlice("_cs_0"), sid));
   ASSERT_TRUE(fn.parseDirName(strSlice("_cs_16383"), sid));
}

TEST(filename_test, dir_test4)
{
   vesselFileName fn;
   SPACE_ID *sid = NULL;
   ASSERT_FALSE(fn.parseDirName(strSlice("_cs_"), sid));
   ASSERT_FALSE(fn.parseDirName(strSlice("_cs_16384"), sid));
   ASSERT_FALSE(fn.parseDirName(strSlice("_cs_abc"), sid));
   ASSERT_FALSE(fn.parseDirName(strSlice("_cs*"), sid));
   ASSERT_FALSE(fn.parseDirName(strSlice(""), sid));
   ASSERT_FALSE(fn.parseDirName(strSlice("xxx"), sid));
}
   