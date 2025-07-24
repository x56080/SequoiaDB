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

*******************************************************************************/
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
