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

TEST(filename_test, test1)
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
}

TEST(filename_test, test2)
{
   vesselFileName fn;
   ASSERT_TRUE(fn.build(0, FILE_TYPE_DATA_STORAGE, SPACE_TYPE_MAIN_DATA, 0));
   ASSERT_EQ(0, ossStrcmp(fn.getFileName(), "_cs_0.0.ds.data"));

   ASSERT_TRUE(fn.build(100, FILE_TYPE_ID_MAP, SPACE_TYPE_IDX, 101));
   ASSERT_EQ(0, ossStrcmp(fn.getFileName(), "_cs_100.101.idmap.idx"));

   ASSERT_TRUE(fn.build(16383, FILE_TYPE_ID_MAP, SPACE_TYPE_LOB, 1000000000, FILE_SHADOW_SUFFIX_TMP));
   ASSERT_EQ(0, ossStrcmp(fn.getFileName(), "_cs_16383.1000000000.idmap.lob._tmp"));
}

TEST(filename_test, test3)
{
   vesselFileName fn;
   ASSERT_FALSE(fn.build(MAX_SPACE_ID + 1, FILE_TYPE_DATA_STORAGE, SPACE_TYPE_MAIN_DATA, 0));
   ASSERT_FALSE(fn.build(0, INVALID_FILE_TYPE, SPACE_TYPE_MAIN_DATA, 0));

   ASSERT_FALSE(fn.extract(strSlice("_cs_16384.0.idmap.data")));
   ASSERT_FALSE(fn.extract(strSlice("_cs_0.0.idmap1.data")));
   ASSERT_FALSE(fn.extract(strSlice("_cs_0.0.idmap.data1")));
   ASSERT_FALSE(fn.extract(strSlice("_cs_0.1.idmap.data._tmpa")));
}