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

   Source File Name = dpsReqBuilderTest.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#include "dpsWriteReqBuilder.hpp"
#include <gtest/gtest.h>

using namespace engine;

class dpsReqBuilderTest :  public testing::Test
{
   public:
   static void SetUpTestCase(){}
   static void TearDownTestCase(){}
   virtual void SetUp(){}
};

TEST_F(dpsReqBuilderTest, base_test_1)
{
   INT32 rc = SDB_OK;
   dpsWriteReqBuilder builder;
   dpsWriteRequest req;

   constexpr UINT32 flags = 0x03;
   constexpr DPS_LOG_TYPE type = LOG_TYPE_DATA_INSERT;
   constexpr UINT32 numericFields = 10;
   constexpr UINT32 padSize = 4096;
   CHAR pad[padSize];
   ossMemset(pad, 'a', sizeof(pad));

   builder.setFlag(flags);
   builder.setType(type);
   
   /// valid tag start with 1.
   for (UINT32 i = 1; i <= numericFields; ++i)
   {
      rc = builder.appendInt32(i, i);
      ASSERT_EQ(rc, SDB_OK);
   }

   rc = builder.append(numericFields + 1, padSize, pad);
   ASSERT_EQ(rc, SDB_OK);

   req = builder.reap();

   ASSERT_EQ(flags, req.getFlags());
   ASSERT_EQ(type, req.getType());
   ASSERT_EQ(numericFields + 1, req.getElementNum());

   for (UINT32 i = 0; i < numericFields; ++i)
   {
      const utilSlice &e = req.getElement(i);
      ASSERT_EQ(sizeof(dpsRecordEle) + sizeof(UINT32), e.getSize());
      utilSlice data;
      BOOLEAN r = req.seek(i + 1, data);
      ASSERT_TRUE(r);
      ASSERT_EQ(sizeof(UINT32), data.size());
      ASSERT_EQ(i + 1, *(data.castTo<UINT32>()));
   }

   utilSlice padData;
   BOOLEAN r = req.seek(numericFields + 1, padData);
   ASSERT_TRUE(r);
   ASSERT_EQ(padSize, padData.size());
   INT32 cmp = ossMemcmp(pad, padData.data(), padSize);
   ASSERT_EQ(0, cmp);
}