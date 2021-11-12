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

#include "vessel/inMemBitmap.h"
#include <gtest/gtest.h>

TEST(sbmtest, test1)
{
   engine::vessel::inMemBitmap bitmap;
   engine::vessel::inMemBitmap::options o;
   INT32 rc = SDB_OK;
   UINT32 capacity = 1280;
   UINT32 offset = 0;

   rc = bitmap.init(capacity, o);
   ASSERT_EQ(SDB_OK, rc);
   rc = bitmap.allocateBits(1, &offset);
   ASSERT_EQ(SDB_VESSEL_NOT_ENOUGH_FREE_RESOURCE, rc);


   rc = bitmap.allocateNewBitmapPages(1);
   ASSERT_EQ(SDB_OK, rc);

   for (UINT32 i = 0; i < capacity; ++i)
   {
      rc = bitmap.allocateBits(1, &offset);
      ASSERT_EQ(SDB_OK, rc);
      ASSERT_EQ(i, offset);
   }
   rc = bitmap.allocateBits(1, &offset);
   ASSERT_EQ(SDB_VESSEL_NOT_ENOUGH_FREE_RESOURCE, rc);

   bitmap.fini();
}

/*
TEST(sbmtest, test2)
{
   engine::vessel::inMemBitmap bitmap;
   engine::vessel::inMemBitmap::options o;
   INT32 rc = SDB_OK;
   UINT32 capacity = 1536;
   UINT32 offset = 0;
   static const UINT32 freeBound = 7;
   o.freeBound = freeBound;
   UINT32 buf[freeBound];
   UINT32 loop = capacity / freeBound;
   UINT32 mod = capacity % freeBound;

   rc = bitmap.init(capacity, o);
   ASSERT_EQ(SDB_OK, rc);

   rc = bitmap.allocateNewBitmapPage();
   ASSERT_EQ(SDB_OK, rc);
   for (UINT32 i = 0; i < loop; ++i)
   {
      rc = bitmap.allocateBits(freeBound, buf);
      ASSERT_EQ(SDB_OK, rc);
      for (UINT32 j = 0; j < freeBound; ++j)
      {
         ASSERT_EQ(i*freeBound + j, buf[j]);
      }
   }

   rc = bitmap.allocateBits(freeBound, buf);
   ASSERT_EQ(SDB_VESSEL_NOT_ENOUGH_FREE_RESOURCE, rc);

   for (UINT32 i = 0; i < mod; ++i)
   {
      rc = bitmap.allocateBits(1, buf);
      ASSERT_EQ(SDB_OK, rc);
      ASSERT_EQ(loop * freeBound + i, buf[0]);
   }

   bitmap.fini();
}

*/

TEST(sbmtest, test3)
{
   engine::vessel::inMemBitmap bitmap;
   engine::vessel::inMemBitmap::options o;
   INT32 rc = SDB_OK;
   UINT32 capacity = 16384;
   UINT32 offset = 0;
   o.bitmapBeginPage = 8;

   rc = bitmap.init(capacity, o);
   ASSERT_EQ(SDB_OK, rc);

   rc = bitmap.allocateNewBitmapPages(1);
   ASSERT_EQ(SDB_OK, rc);

   for (UINT32 i = 0; i < capacity; ++i)
   {
      rc = bitmap.allocateBits(1, &offset);
      ASSERT_EQ(SDB_OK, rc);
      ASSERT_EQ(8 * capacity + i, offset);
   }

   bitmap.fini();
}

TEST(sbmtest, test4)
{
   engine::vessel::inMemBitmap bitmap;
   engine::vessel::inMemBitmap::options o;
   INT32 rc = SDB_OK;
   UINT32 capacity = 4096;
   UINT32 offset = 0;
   o.maxBitmapPageCount = 8;

   rc = bitmap.init(capacity, o);
   ASSERT_EQ(SDB_OK, rc);


   for (UINT32 i = 0; i < (capacity * o.maxBitmapPageCount); ++i)
   {
      rc = bitmap.allocateBits(1, &offset, 1);
      ASSERT_EQ(SDB_OK, rc);
      ASSERT_EQ(i, offset);
   }

   rc = bitmap.allocateBits(1, &offset, 1);
   ASSERT_EQ(SDB_VESSEL_OUT_OF_RESOURCE, rc);
   bitmap.fini();
}
