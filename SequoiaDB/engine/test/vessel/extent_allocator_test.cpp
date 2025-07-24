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

   Source File Name = extent_allocator_test.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

*******************************************************************************/
#include "test_def.h"
#include <gtest/gtest.h>
#include "vessel/variableExtentAllocator.h"

using namespace engine::vessel;

TEST(extent_allocator_test, base_test1)
{
   variableExtentAllocator allocator;
   
   constexpr UINT32 SEG_PCNT = 32768;
   constexpr UINT32 SEG_COUNT_PERFILE = 128;
   constexpr UINT32 TOTAL_SEG_COUNT = 16384;
   variableExtentAllocator::options o;
   o.maxPageCountPerSegment = SEG_PCNT;
   o.maxSegmentCountPerFile = SEG_COUNT_PERFILE;
   allocator.init(o);

   INT32 rc = SDB_OK;
   UINT32 currentSegCount = 0;

   for (UINT32 i = 0; i < TOTAL_SEG_COUNT; ++i)
   {
      rc = allocator.deposit();
      ASSERT_EQ(SDB_OK, rc);

      for (UINT32 j = 0; j < SEG_PCNT; ++j)
      {
         PAGE_ID pid = INVALID_PAGE_ID;
         rc = allocator.reserveExtent(1, pid);
         ASSERT_EQ(SDB_OK, rc);
         ASSERT_EQ(i * SEG_PCNT + j, pid);
      }

      PAGE_ID pid = INVALID_PAGE_ID;
      rc = allocator.reserveExtent(1, pid);
      ASSERT_EQ(INVALID_PAGE_ID, pid);
   }

   allocator.reset();
}

TEST(extent_allocator_test, base_test2)
{
   variableExtentAllocator allocator;
   constexpr UINT32 SEG_PCNT = 32768;
   constexpr UINT32 SEG_COUNT = 4096;
   variableExtentAllocator::options o;
   o.maxPageCountPerSegment = SEG_PCNT;
   o.maxSegmentCountPerFile = SEG_COUNT;
   allocator.init(o);

   INT32 rc = SDB_OK;

   for (UINT32 i = 0; i < SEG_COUNT; ++i)
   {
      rc = allocator.deposit();
      ASSERT_EQ(SDB_OK, rc);

      for (UINT32 j = 0; j < SEG_PCNT; ++j)
      {
         PAGE_ID pid = INVALID_PAGE_ID;
         rc = allocator.reserveExtent(1, pid);
         ASSERT_EQ(SDB_OK, rc);
         ASSERT_EQ(i * SEG_PCNT + j, pid);
      }

      PAGE_ID pid = INVALID_PAGE_ID;
      rc = allocator.reserveExtent(1, pid);
      ASSERT_EQ(SDB_OK, rc);
      ASSERT_EQ(INVALID_PAGE_ID, pid);
   }

   for (UINT32 i = 0; i < SEG_COUNT; ++i)
   {
      for (UINT32 j = 0; j < SEG_PCNT; ++j)
      {
         allocator.freeExtent(i * SEG_PCNT + j, 1);
      }
   }

   for (UINT32 i = 0; i < SEG_COUNT; ++i)
   {
      for (UINT32 j = 0; j < SEG_PCNT; ++j)
      {
         PAGE_ID pid = INVALID_PAGE_ID;
         rc = allocator.reserveExtent(1, pid);
         ASSERT_EQ(SDB_OK, rc);
         ASSERT_NE(INVALID_PAGE_ID, pid);
      }
   }

   PAGE_ID pid = INVALID_PAGE_ID;
   rc = allocator.reserveExtent(1, pid);
   ASSERT_EQ(SDB_OK, rc);
   ASSERT_EQ(INVALID_PAGE_ID, pid);

   allocator.reset();
}

TEST(extent_allocator_test, base_test3)
{
   variableExtentAllocator allocator;
   constexpr UINT32 SEG_PCNT = 32768;
   constexpr UINT32 SEG_COUNT = 4096;
   variableExtentAllocator::options o;
   o.maxPageCountPerSegment = SEG_PCNT;
   o.maxSegmentCountPerFile = SEG_COUNT;
   allocator.init(o);

   INT32 rc = SDB_OK;

   for (UINT32 i = 0; i < SEG_COUNT; ++i)
   {
      rc = allocator.deposit();
      ASSERT_EQ(SDB_OK, rc);

      for (UINT32 j = 0; j < SEG_PCNT; ++j)
      {
         PAGE_ID pid = INVALID_PAGE_ID;
         rc = allocator.reserveExtent(1, pid);
         ASSERT_EQ(SDB_OK, rc);
         ASSERT_EQ(i * SEG_PCNT + j, pid);
      }

      PAGE_ID pid = INVALID_PAGE_ID;
      rc = allocator.reserveExtent(1, pid);
      ASSERT_EQ(SDB_OK, rc);
      ASSERT_EQ(INVALID_PAGE_ID, pid);
   }

   for (UINT32 i = 0; i < SEG_COUNT; ++i)
   {
      for (UINT32 j = 0; j < SEG_PCNT; ++j)
      {
         allocator.freeExtent(i * SEG_PCNT + j, 1);
      }
   }

   for (UINT32 i = 0; i < SEG_COUNT; ++i)
   {
      for (UINT32 j = 0; j < SEG_PCNT; ++j)
      {
         PAGE_ID pid = INVALID_PAGE_ID;
         rc = allocator.reserveExtent(1, pid);
         ASSERT_EQ(SDB_OK, rc);
         ASSERT_NE(INVALID_PAGE_ID, pid);
      }
   }

   PAGE_ID pid = INVALID_PAGE_ID;
   rc = allocator.reserveExtent(1, pid);
   ASSERT_EQ(SDB_OK, rc);
   ASSERT_EQ(INVALID_PAGE_ID, pid);

   allocator.reset();
}


TEST(extent_allocator_test, base_test4)
{
   variableExtentAllocator allocator;
   constexpr UINT32 SEG_PCNT = 32768;
   constexpr UINT32 SEG_COUNT_PERFILE = 8192;
   constexpr UINT32 TOTAL_SEG_COUNT = 16384;
   constexpr UINT32 EXTENT_SIZE = 1024;
   variableExtentAllocator::options o;
   o.maxPageCountPerSegment = SEG_PCNT;
   o.maxSegmentCountPerFile = SEG_COUNT_PERFILE;
   allocator.init(o);

   INT32 rc = SDB_OK;

   for (UINT32 i = 0; i < TOTAL_SEG_COUNT; ++i)
   {
      rc = allocator.deposit();
      ASSERT_EQ(SDB_OK, rc);

      UINT32 loop = SEG_PCNT / EXTENT_SIZE;
      for (UINT32 j = 0; j < loop; ++j)
      {
         PAGE_ID pid = INVALID_PAGE_ID;
         rc = allocator.reserveExtent(EXTENT_SIZE, pid);
         ASSERT_EQ(SDB_OK, rc);
         ASSERT_EQ(i * SEG_PCNT + (j * EXTENT_SIZE), pid);
      }

      PAGE_ID pid = INVALID_PAGE_ID;
      rc = allocator.reserveExtent(1, pid);
      ASSERT_EQ(SDB_OK, rc);
      ASSERT_EQ(INVALID_PAGE_ID, pid);
   }

   for (UINT32 i = 0; i < TOTAL_SEG_COUNT; ++i)
   {
      UINT32 loop = SEG_PCNT / EXTENT_SIZE;
      for (UINT32 j = 0; j < loop; ++j)
      {
         allocator.freeExtent(i * SEG_PCNT + j * EXTENT_SIZE, EXTENT_SIZE);
      }
   }

   for (UINT32 i = 0; i < TOTAL_SEG_COUNT; ++i)
   {
      UINT32 loop = SEG_PCNT;
      for (UINT32 j = 0; j < loop; ++j)
      {
         PAGE_ID pid = INVALID_PAGE_ID;
         rc = allocator.reserveExtent(1, pid);
         ASSERT_EQ(SDB_OK, rc);
         ASSERT_NE(INVALID_PAGE_ID, pid);
      }
   }

   PAGE_ID pid = INVALID_PAGE_ID;
   rc = allocator.reserveExtent(1, pid);
   ASSERT_EQ(SDB_OK, rc);
   ASSERT_EQ(INVALID_PAGE_ID, pid);
   allocator.reset();
}

/// free pids
TEST(extent_allocator_test, base_test5)
{
   variableExtentAllocator allocator;
   constexpr UINT32 SEG_PCNT = 512;
   constexpr UINT32 SEG_COUNT_PERFILE = 128;
   constexpr UINT32 TOTAL_SEG_COUNT = 256;
   constexpr UINT32 TOTAL_PCNT = SEG_PCNT * TOTAL_SEG_COUNT;
   constexpr UINT32 SME_BUF_SIZE = SEG_PCNT >> 3;

   variableExtentAllocator::options o;
   o.maxPageCountPerSegment = SEG_PCNT;
   o.maxSegmentCountPerFile = SEG_COUNT_PERFILE;
   allocator.init(o);

   vector<std::unique_ptr<CHAR []>> smes;
   vector<PAGE_ID> pids;
   set<PAGE_ID> pidSet;

   INT32 rc = SDB_OK;

   for (UINT32 i = 0; i < TOTAL_SEG_COUNT; ++i)
   {
      std::unique_ptr<CHAR []> sme(new CHAR[SME_BUF_SIZE]);
      ossMemset(sme.get(), 0x0, SME_BUF_SIZE);
      rc = allocator.depositWithSme((UINT64 *)sme.get());
      ASSERT_EQ(SDB_OK, rc);
      smes.emplace_back(std::move(sme));
   }

   for (UINT32 i = 0; i < TOTAL_PCNT; i += 2)
   {
      PAGE_ID pid = ossRand() % TOTAL_PCNT;
      if (0 < pidSet.count(pid))
      {
         continue;
      }
      pids.push_back(pid);
      pidSet.insert(pid);
   }

   allocator.freePids(pids.size(), pids.data());
   for (UINT32 i = 0; i < pids.size(); ++i)
   {
      ASSERT_TRUE(allocator.test(pids[i]));
   }

   for (UINT32 i = 0; i < TOTAL_PCNT; ++i)
   {
      if (0 < pidSet.count(i))
      {
         continue;
      }

      ASSERT_FALSE(allocator.test(i));
   }

   allocator.reset();
   allocator.init(o);
   for (auto itr = smes.begin(); itr != smes.end(); ++itr)
   {
      rc = allocator.depositWithSme((UINT64 *)(itr->get()));
      ASSERT_EQ(SDB_OK, rc);
   }

   for (UINT32 i = 0; i < TOTAL_PCNT; ++i)
   {
      if (0 < pidSet.count(i))
      {
         ASSERT_TRUE(allocator.test(i));
      }
      else
      {
         ASSERT_FALSE(allocator.test(i));
      }
   }
}