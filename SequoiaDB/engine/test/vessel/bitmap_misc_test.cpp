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

   Source File Name = bitmap_misc_test.cpp

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
#include "vessel/unitedBitmap.hpp"
#include <gtest/gtest.h>
#include "vessel/bitsetTree.hpp"
#include "vessel/blockBasedMemPool.h"

using namespace engine::vessel;


TEST(bitmapMiscTest, united_bitmap_test1)
{
   unitedBitmap<512> bitmap;
   UINT32 unitCount = 128;
   INT32 rc = bitmap.extendUnitNum(unitCount);
   ASSERT_EQ(SDB_OK, rc);

   for (UINT32 loop = 0; loop < 100; ++loop)
   {
      for (UINT32 i = 0; i < bitmap.getTotalBitNum(); ++i)
      {
         INT32 bit = bitmap.pop();
         ASSERT_EQ((INT32)i, bit);
      }

      ASSERT_TRUE(bitmap.none());
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

      ASSERT_TRUE(bitmap.none());

      for (UINT32 i = 0; i < bitmap.getTotalBitNum(); ++i)
      {
         BOOLEAN old = FALSE;
         bitmap.set(i, &old);
         ASSERT_FALSE(old);
      }
   }
   
}


TEST(bitmapMiscTest, united_bitmap_test2)
{
   constexpr UINT32 _SIZE = 512;
   unitedBitmap<_SIZE> bitmap;
   unitedBitmap<_SIZE>::options o;
   o.percentFreeReused = 0.1f;
   bitmap.setOptions(o);

   UINT32 unitCount = 128;
   INT32 rc = bitmap.extendUnitNum(unitCount, FALSE);
   ASSERT_EQ(SDB_OK, rc);

   for (UINT32 i = 0; i < (_SIZE * o.percentFreeReused - 1); ++i)
   {
      bitmap.set(i);
      ASSERT_EQ(-1, bitmap.pop());
      ASSERT_TRUE(bitmap.none());
   }

   bitmap.set(_SIZE * o.percentFreeReused);
   ASSERT_FALSE(bitmap.none());

   for (UINT32 i = 0; i < (_SIZE * o.percentFreeReused); ++i)
   {
      ASSERT_EQ(i, bitmap.pop());
   }

   ASSERT_TRUE(bitmap.none());
   INT32 bit = bitmap.pop();
   ASSERT_EQ(-1, bit);
}

TEST(bitmapMiscTest, united_bitmap_test3)
{
   constexpr UINT32 _SIZE = 512;
   unitedBitmap<_SIZE> bitmap;
   UINT32 unitCount = 256;
   INT32 rc = bitmap.extendUnitNum(unitCount, FALSE);
   ASSERT_EQ(SDB_OK, rc);

   for (UINT32 i = 0; i < 100000000; ++i)
   {
      bitmap.set(bitmap.getTotalBitNum() - 1);
      ASSERT_EQ(bitmap.getTotalBitNum() - 1, bitmap.pop());
   }
}

TEST(bitmapMiscTest, bitset_tree_test1)
{
   INT32 rc = SDB_OK;
   _bitsetTree<512, 3> tree;
   UINT32 max = tree.getMaxBitCount(); /// set it as non-aligned number.
   for (UINT32 i = 0; i < max; ++i)
   {
      rc = tree.pushBack(TRUE);
      ASSERT_EQ(SDB_OK, rc);
   }

   rc = tree.pushBack(TRUE);
   ASSERT_EQ(SDB_VESSEL_OUT_OF_RESOURCE, rc);

   for (UINT32 i = 0; i < max; ++i)
   {
      ASSERT_TRUE(tree.test(i));
   }

   for (UINT32 i = 0; i < max; ++i)
   {
      INT32 bit = tree.findFirst();
      ASSERT_EQ(i, bit);
      tree.reset(i);
   }

   ASSERT_TRUE(tree.none());
   INT32 bit = tree.findFirst();
   ASSERT_EQ(-1, bit);

   for (INT32 i = (INT32)max - 1; 0 <= i; --i)
   {
      tree.set(i);
      ASSERT_EQ(i, tree.findFirst());
   }

   tree.resetAll();
   ASSERT_TRUE(tree.none());
   tree.setAll();

   for (UINT32 i = 0; i < max; ++i)
   {
      INT32 bit = tree.findFirst();
      ASSERT_EQ(i, bit);
      tree.reset(i);
   }

   ASSERT_TRUE(tree.none());
   ASSERT_EQ(-1, tree.findFirst());
}

TEST(bitmapMiscTest, bitset_tree_test2)
{
   _bitsetTree<512, 3> tree;
   UINT32 max = tree.getMaxBitCount();
   UINT32 factor = 17; 
   for (UINT32 i = 0; i < (max - factor); ++i)
   {
      ASSERT_EQ(SDB_OK, tree.pushBack(FALSE));
   }
   ASSERT_TRUE(tree.none());

   tree.setAll();
   for (UINT32 i = 0; i < (max - factor); ++i)
   {
      INT32 bit = tree.findFirst();
      ASSERT_EQ(i, bit);
      tree.reset(i);
   }
   ASSERT_TRUE(tree.none());
   ASSERT_EQ(-1, tree.findFirst());
}

TEST(bitmapMiscTest, bitset_tree_test3)
{
   _bitsetTree<512, 3> tree;
   UINT32 max = tree.getMaxBitCount();
   tree.fastRefill(max, TRUE);
   for (UINT32 i = 0; i < max; ++i)
   {
      INT32 bit = tree.findFirst();
      ASSERT_EQ(i, bit);
      tree.reset(i);
   }
   ASSERT_TRUE(tree.none());
   ASSERT_EQ(-1, tree.findFirst());
}

TEST(bitmapMiscTest, mem_pool_test1)
{
   INT32 rc = SDB_OK;
   UINT32 maxChunkSize = 128;
   blockBasedMemPool pool;
   rc = pool.init(maxChunkSize);
   ASSERT_EQ(SDB_OK, rc);

   for (UINT32 i = 0; i < pool.getTotalBlockNum(); ++i)
   {
      blockBasedMemPool::memBlock block;
      rc = pool.allocate(block);
      ASSERT_EQ(SDB_OK, rc);
   }
}