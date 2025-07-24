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

   Source File Name = bitmap_misc_test.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          11/08/2021  WY  Initial Draft

   Last Changed =

*******************************************************************************/
#include "test_def.h"
#include "vessel/unitedBitmap.hpp"
#include <gtest/gtest.h>
#include "vessel/bitsetTree.hpp"
#include "vessel/blockBasedMemPool.h"
#include "vessel/sparseBitmap32.h"
#include <random>

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
   o.minFreeReused = _SIZE * 0.1f;
   bitmap.init(o);

   UINT32 unitCount = 128;
   INT32 rc = bitmap.extendUnitNum(unitCount, FALSE);
   ASSERT_EQ(SDB_OK, rc);

   for (UINT32 i = 0; i < (o.minFreeReused - 1); ++i)
   {
      bitmap.set(i);
      ASSERT_EQ(-1, bitmap.pop());
      ASSERT_TRUE(bitmap.none());
   }

   bitmap.set(o.minFreeReused - 1);
   ASSERT_FALSE(bitmap.none());

   for (UINT32 i = 0; i < o.minFreeReused; ++i)
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

TEST(bitmapMiscTest, base_sparse_bitmap32_test1)
{
   constexpr UINT32 count = 100000000;
   sparseBitmap32 bitmap;
   for (UINT32 i = 0; i < count; i+=2)
   {
      BOOLEAN old = FALSE;
      INT32 rc = bitmap.set(i, &old);
      ASSERT_EQ(SDB_OK, rc);
      ASSERT_FALSE(old);
   }

   for (UINT32 i = 0; i < count; ++i)
   {
      BOOLEAN r = bitmap.test(i);
      if (0 == (i & 0x01))
      {
         ASSERT_TRUE(r);
      }
      else
      {
         ASSERT_FALSE(r);
      }
   }

   sparseBitmap32::iterator itr;
   for (UINT32 i = 0; i < count; i+=2)
   {
      BOOLEAN r = bitmap.next(itr);
      ASSERT_TRUE(r);
      ASSERT_EQ(i, itr.get());
   }
   ASSERT_FALSE(bitmap.next(itr));

   for (UINT32 i = 0; i < count; i+=2)
   {
      bitmap.reset(i);
   }

   for (UINT32 i = 0; i < count; ++i)
   {
      ASSERT_FALSE(bitmap.test(i));
   }
}

TEST(bitmapMiscTest, base_sparse_bitmap32_test2)
{
   constexpr UINT32 count = 10000;
   sparseBitmap32 bitmap;
   vector<UINT32> values;
   values.reserve(count);
   std::default_random_engine generator;
   std::uniform_int_distribution<UINT32> distribution(0, 1000000);

   for (UINT32 i = 0; i < count; ++i)
   {
      values.emplace_back(distribution(generator));
   }

   for (UINT32 i = 0; i < values.size(); ++i)
   {
      bitmap.set(values[i]);
   }

   std::sort(values.begin(), values.end());
   auto last = std::unique(values.begin(), values.end());
   values.resize(std::distance(values.begin(), last));

   for (UINT32 i = 0; i < values.size(); ++i)
   {
      ASSERT_TRUE(bitmap.test(values[i]));
   }

   sparseBitmap32::iterator itr;
   for (UINT32 i = 0; i < values.size(); ++i)
   {
      BOOLEAN r = bitmap.next(itr);
      ASSERT_TRUE(r);
      ASSERT_EQ(values[i], itr.get());
   }
   ASSERT_FALSE(bitmap.next(itr));
}
