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

   Source File Name = vessel_sbm_test.cpp

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

#include "vessel/inMemBitMap.h"
#include <gtest/gtest.h>

TEST(sbmtest, test1)
{
   engine::vessel::inMemBitMap bitmap;
   INT32 rc = SDB_OK;
   UINT32 bitCount = 1234;
   UINT32 freeBound = 0;
   UINT32 offset = 0;

   rc = bitmap.init(bitCount, freeBound);
   ASSERT_EQ(SDB_OK, rc);
   rc = bitmap.allocateBits(1, &offset);
   ASSERT_EQ(SDB_VESSEL_SMP_NO_FREE, rc);


   rc = bitmap.allocateNewBitPage();
   ASSERT_EQ(SDB_OK, rc);

   for (UINT32 i = 0; i < bitCount; ++i)
   {
      rc = bitmap.allocateBits(1, &offset);
      ASSERT_EQ(SDB_OK, rc);
      ASSERT_EQ(i, offset);
   }
   rc = bitmap.allocateBits(1, &offset);
   ASSERT_EQ(SDB_VESSEL_SMP_NO_FREE, rc);

   rc = bitmap.fini();
   ASSERT_EQ(SDB_OK, rc);
}

TEST(sbmtest, test2)
{
   engine::vessel::inMemBitMap bitmap;
   INT32 rc = SDB_OK;
   UINT32 bitCount = 4078;
   UINT32 offset = 0;
   const UINT32 freeBound = 8;
   UINT32 buf[freeBound];
   UINT32 loop = bitCount / freeBound;

   rc = bitmap.init(bitCount, freeBound);
   ASSERT_EQ(SDB_OK, rc);

   rc = bitmap.allocateNewBitPage();
   ASSERT_EQ(SDB_OK, rc);
   for (UINT32 i = 0; i < loop; ++i)
   {
      rc = bitmap.allocateBits(freeBound, buf);
      ASSERT_EQ(SDB_OK, rc);
      for (UINT32 j = 0; j < freeBound; ++j)
      {
         ASSERT_EQ(i*8 + j, buf[j]);
      }
   }

   rc = bitmap.allocateBits(freeBound, buf);
   ASSERT_EQ(SDB_VESSEL_SMP_NO_FREE, rc);

   for (UINT32 i = 0; i < (freeBound % 8); ++i)
   {
      rc = bitmap.allocateBits(1, buf);
      ASSERT_EQ(SDB_OK, rc);
      ASSERT_EQ(loop * 8 + i, buf[i]);
   }

   bitmap.fini();
}

TEST(sbmtest, test3)
{
   engine::vessel::inMemBitMap bitmap;
   INT32 rc = SDB_OK;
   UINT32 bitCount = 16384;
   UINT32 offset = 0;
   const UINT32 freeBound = 0;

   rc = bitmap.init(bitCount, freeBound);
   ASSERT_EQ(SDB_OK, rc);


   for (UINT32 loop = 0; loop < 32; ++loop)
   {
      rc = bitmap.allocateNewBitPage();
      ASSERT_EQ(SDB_OK, rc);

      for (UINT32 i = 0; i < bitCount; ++i)
      {
         rc = bitmap.allocateBits(1, &offset);
         ASSERT_EQ(SDB_OK, rc);
         ASSERT_EQ(loop * bitCount + i, offset);
      }
      rc = bitmap.allocateBits(1, &offset);
      ASSERT_EQ(SDB_VESSEL_SMP_NO_FREE, rc);
   }

   bitmap.fini();
}
