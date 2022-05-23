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

   Source File Name = extent_allocator_test.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#include "test_def.h"
#include <gtest/gtest.h>
#include "vessel/variableExtentAllocator.h"

using namespace engine::vessel;

TEST(extent_allocator_test, base_test1)
{
   variableExtentAllocator allocator;
   
   constexpr UINT32 SEG_PCNT = 32768;
   constexpr UINT32 SEG_COUNT = 16384;
   variableExtentAllocator::options o;
   o.maxPageCountPerSegment = SEG_PCNT;
   o.maxSegmentCountPerFile = SEG_COUNT;
   allocator.init(o);

   INT32 rc = SDB_OK;
   UINT32 currentSegCount = 0;

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
   constexpr UINT32 SEG_COUNT = 16384;
   constexpr UINT32 EXTENT_SIZE = 1024;
   variableExtentAllocator::options o;
   o.maxPageCountPerSegment = SEG_PCNT;
   o.maxSegmentCountPerFile = SEG_COUNT;
   allocator.init(o);

   INT32 rc = SDB_OK;

   for (UINT32 i = 0; i < SEG_COUNT; ++i)
   {
      rc = allocator.deposit();
      ASSERT_EQ(SDB_OK, rc);

      UINT32 loop = SEG_PCNT / EXTENT_SIZE;
      for (UINT32 j = 0; j < loop; ++j)
      {
         PAGE_ID pid = INVALID_PAGE_ID;
         rc = allocator.reserveExtent(1, pid);
         ASSERT_EQ(SDB_OK, rc);
         ASSERT_EQ(i * SEG_PCNT + (j * EXTENT_SIZE), pid);
      }

      PAGE_ID pid = INVALID_PAGE_ID;
      rc = allocator.reserveExtent(1, pid);
      ASSERT_EQ(SDB_OK, rc);
      ASSERT_EQ(INVALID_PAGE_ID, pid);
   }

   for (UINT32 i = 0; i < SEG_COUNT; ++i)
   {
      UINT32 loop = SEG_PCNT / EXTENT_SIZE;
      for (UINT32 j = 0; j < loop; ++j)
      {
         allocator.freeExtent(i * SEG_PCNT + j * EXTENT_SIZE, EXTENT_SIZE);
      }
   }

   for (UINT32 i = 0; i < SEG_COUNT; ++i)
   {
      UINT32 loop = SEG_PCNT / EXTENT_SIZE;
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