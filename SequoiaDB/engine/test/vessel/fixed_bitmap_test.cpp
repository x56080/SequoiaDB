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

   Source File Name = fixed_bitmap_test.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          11/08/2021  WY  Initial Draft

   Last Changed =

******************************************************************************/
#include "test_def.h"
#include "vessel/fixedBitmap.hpp"
#include <gtest/gtest.h>
#include "ossUtil.h"
#include "ossMemPool.hpp"

using namespace engine::vessel;


TEST(fixedBitmapTest, base_test1)
{
   fixedBitmap<> bitmap;
   UINT32 unitCount = 128;
   bitmap.init(unitCount);

   for (UINT32 loop = 0; loop < 100; ++loop)
   {
      for (UINT32 i = 0; i < bitmap.getTotalBitNum(); ++i)
      {
         INT32 bit = bitmap.pop();
         ASSERT_EQ((INT32)i, bit);
      }

      ASSERT_FALSE(bitmap.hasNonzeroBit());
      INT32 bit = bitmap.pop();
      ASSERT_EQ(-1, bit);

      for (UINT32 i = 0; i < bitmap.getTotalBitNum(); ++i)
      {
         BOOLEAN old = FALSE;
         bitmap.set(i, &old);
         ASSERT_FALSE(old);
         INT32 bit = bitmap.pop();
         ASSERT_EQ((INT32)i, bit);
      }

      ASSERT_FALSE(bitmap.hasNonzeroBit());

      for (UINT32 i = 0; i < bitmap.getTotalBitNum(); ++i)
      {
         BOOLEAN old = FALSE;
         bitmap.set(i, &old);
         ASSERT_FALSE(old);
      }

      ASSERT_TRUE(bitmap.allSet());
   }
}

