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

   Source File Name = lc_freelist_test.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          11/08/2021  LYC  Initial Draft

   Last Changed =

******************************************************************************/
#include "test_def.h"
#include "vessel/lcFreeList.h"
#include <gtest/gtest.h>
#include "ossUtil.h"

using namespace engine::vessel;

void allocate_release_test1(lcFreeList *fl)
{
   freeListPage page1;
   freeListPage page2;
   freeListPage page3;

   ASSERT_TRUE(fl->fastCheckIfHasFreePage());

   ASSERT_EQ(fl->allocate(page1), 0);
   ASSERT_EQ(fl->allocate(page2), 0);
   fl->releasePage(page1);
   ASSERT_EQ(fl->allocate(page3), 0);
   ASSERT_EQ(fl->allocate(page1), 0);
   fl->releasePage(page2);
   fl->releasePage(page3);
   fl->releasePage(page1);
}

void allocate_release_test2(lcFreeList *fl, UINT32 count)
{
   freeListPage *pages = SDB_OSS_NEW freeListPage[count];
   UINT64 start = ossGetCurrentMilliseconds();
   for (INT32 i = 0; i < count; i++)
   {
      ASSERT_EQ(fl->allocate(pages[i]), 0);
   }

   fl->releasePages(count, pages);
   UINT64 end = ossGetCurrentMilliseconds();
   std::cout << "sub thread time:" << end - start << std::endl;
   SDB_OSS_DEL []pages;
}


TEST(freeListTest, normalTest1)
{
   lcFreeList fl;
   freeListPage page1;
   freeListPage page2;
   freeListPage page3;
   liteCacheOptions::freeListOptions options;
   fl.init(DMS_PAGE_SIZE32K, options);

   ASSERT_TRUE(fl.fastCheckIfHasFreePage());

   ASSERT_EQ(fl.allocate(page1), 0);
   ASSERT_EQ(fl.allocate(page2), 0);
   ASSERT_EQ(fl.allocate(page3), 0);
   fl.releasePage(page1);
   fl.releasePage(page2);
   fl.releasePage(page3);

   ASSERT_EQ(fl.allocate(page1), 0);
   fl.releasePage(page1);
   ASSERT_EQ(fl.allocate(page2), 0);
   ASSERT_EQ(fl.allocate(page3), 0);
   fl.releasePage(page2);
   fl.releasePage(page3);

   ASSERT_EQ(fl.allocate(page1), 0);
   ASSERT_EQ(fl.allocate(page2), 0);
   fl.releasePage(page1);
   ASSERT_EQ(fl.allocate(page3), 0);
   ASSERT_EQ(fl.allocate(page1), 0);
   fl.releasePage(page2);
   fl.releasePage(page3);
   fl.releasePage(page1);

   fl.fini();
}

TEST(freeListTest, normalTest2)
{
   lcFreeList fl;
   liteCacheOptions::freeListOptions options;
   UINT32 maxPageNum = options.maxChunkCount * options.pageCountInChunk;
   freeListPage *pages = SDB_OSS_NEW freeListPage[maxPageNum + 1];
   fl.init(DMS_PAGE_SIZE32K, options);

   ASSERT_TRUE(fl.fastCheckIfHasFreePage());
   for (UINT32 i = 0; i < maxPageNum; ++i)
   {
      ASSERT_EQ(fl.allocate(pages[i]), 0);
   }
   ASSERT_FALSE(fl.fastCheckIfHasFreePage());
   ASSERT_EQ(fl.allocate(pages[maxPageNum]),  SDB_VESSEL_LC_NOT_ENOUGH_PAGES_IN_FL);

   fl.releasePages(options.maxChunkCount * options.pageCountInChunk + 1, pages);

   SDB_OSS_DEL []pages;
   fl.fini();
}

TEST(freeListTest, normalTest3)
{
   lcFreeList fl;
   liteCacheOptions::freeListOptions options;
   fl.init(DMS_PAGE_SIZE32K, options);

   static const UINT32 THREAD_COUNT = 5;
   std::thread threads[THREAD_COUNT];
   for (UINT32 i = 0; i < THREAD_COUNT; ++i)
   {
      threads[i] = std::move(std::thread(allocate_release_test1, &fl));
   }
   
   for (UINT32 i = 0; i < THREAD_COUNT; ++i)
   {
      threads[i].join();
   }
   fl.fini();
}

TEST(freeListTest, normalTest4)
{
   lcFreeList fl;
   freeListPage page1;
   freeListPage page2;
   liteCacheOptions::freeListOptions options;
   options.maxChunkCount = 0;
   ASSERT_EQ(fl.init(DMS_PAGE_SIZE32K, options), SDB_INVALIDARG);
   options.maxChunkCount = 128;
   options.pageCountInChunk = 0;
   ASSERT_EQ(fl.init(DMS_PAGE_SIZE32K, options), SDB_INVALIDARG);
   options.pageCountInChunk = 33;
   ASSERT_EQ(fl.init(DMS_PAGE_SIZE32K, options), SDB_INVALIDARG);
   options.maxChunkCount = 128;
   options.pageCountInChunk = 1024;
   ASSERT_EQ(fl.init(DMS_PAGE_SIZE512K, options), SDB_INVALIDARG);

   options.pageCountInChunk = 1;
   options.maxChunkCount = 1;
   ASSERT_EQ(fl.init(DMS_PAGE_SIZE32K, options), SDB_OK);
   ASSERT_EQ(fl.allocate(page1), SDB_OK);
   ASSERT_EQ(fl.allocate(page2), SDB_VESSEL_LC_NOT_ENOUGH_PAGES_IN_FL);
   fl.releasePage(page1);
   ASSERT_EQ(fl.allocate(page2), SDB_OK);
}

TEST(freeListTest, performanceTest1)
{
   lcFreeList fl;
   UINT32 pageNum = 1000000;
   freeListPage *page = SDB_OSS_NEW freeListPage[pageNum];
   liteCacheOptions::freeListOptions options;
   options.maxChunkCount = 1024;

   fl.init(DMS_PAGE_SIZE64K, options);
   UINT64 start = ossGetCurrentMilliseconds();
   for (INT32 i = 0; i < pageNum; i++)
   {
      ASSERT_EQ(fl.allocate(page[i]), 0);
   }
   for (INT32 i = 0; i < pageNum; i++)
   {
      fl.releasePage(page[i]);
   }
   for (INT32 i = 0; i < pageNum; i++)
   {
      ASSERT_EQ(fl.allocate(page[i]), 0);
   }
   for (INT32 i = 0; i < pageNum; i++)
   {
      fl.releasePage(page[i]);
   }
   UINT64 end = ossGetCurrentMilliseconds();
   std::cout << "total time:" << end - start << std::endl;
   SDB_OSS_DEL []page;
   fl.fini();
}

TEST(freeListTest, performanceTest2)
{
   lcFreeList fl;
   UINT32 pageNum = 1000000;
   liteCacheOptions::freeListOptions options;
   options.maxChunkCount = 1024;
   static const UINT32 THREAD_COUNT = 5;
   std::thread threads[THREAD_COUNT];

   fl.init(DMS_PAGE_SIZE32K, options);
   UINT64 start = ossGetCurrentMilliseconds();
   for (UINT32 i = 0; i < THREAD_COUNT; ++i)
   {
      threads[i] = std::move(std::thread(allocate_release_test2, &fl, 
                                         pageNum/THREAD_COUNT));
   }
   
   for (UINT32 i = 0; i < THREAD_COUNT; ++i)
   {
      threads[i].join();
   }
   UINT64 end = ossGetCurrentMilliseconds();
   std::cout << "total time:" << end - start << std::endl;
   fl.fini();
}

TEST(freeListTest, chunkTest)
{
   lcCacheChunk chunk;
   freeListPage page;
   
   ASSERT_EQ(chunk.setup(1, 1024, DMS_PAGE_SIZE32K), SDB_OK);
   ASSERT_EQ(chunk.getId(), 1);
   ASSERT_EQ(chunk.getPageNum(), 1024);
   ASSERT_EQ(chunk.getFreePageCount(), 1024);
   ASSERT_TRUE(chunk.hasFreePage());
   ASSERT_TRUE(chunk.allocatePage(page));
   ASSERT_EQ(chunk.getFreePageCount(), 1023);
   chunk.releasePage(page);
   ASSERT_EQ(chunk.getFreePageCount(), 1024);
   chunk.teardown();

   ASSERT_EQ(chunk.setup(1, 1024, 0), SDB_INVALIDARG);
   ASSERT_EQ(chunk.setup(1, 0, DMS_PAGE_SIZE32K), SDB_INVALIDARG);
   
}
