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

   Source File Name = inMemBitMap.h

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

#ifndef VESSEL_IN_MEM_BIT_MAP_H_
#define VESSEL_IN_MEM_BIT_MAP_H_

#include "ossLatch.hpp"
#include "utilPooledObject.hpp"
#include "ossMemPool.hpp"

#include <map>

namespace engine
{
namespace vessel
{
   class requestContext;
   struct spaceManagementPageHead;

   const UINT32 IMBM_FLAG_KEEP_NONFREE_PAGE = 0x01;

   class inMemBitMap : public SDBObject
   {
      private:
         class _inMemBitPage : public _utilPooledObject
         {
            public:
               _inMemBitPage():
               _pageID(-1),
               _free(0), 
               _firstFreeBits(-1),
               _buf(NULL)
               {}

               ~_inMemBitPage();

            public:
               INT32 init(INT32 pageID, UINT32 capacity, BOOLEAN noFree = FALSE, UINT32 occupied = 0);
               INT32 initFromAlignedBuf(INT32 pageID, UINT32 capacity, const CHAR *buf);
               INT32 fini();

               INT32 allocate(UINT32 alignedBitsCount,
                              UINT32 count,
                              UINT32 *buf,
                              UINT32 *stillFreeCount = NULL);
               void free(UINT32 alignedBitsCount, UINT32 count, const UINT32 *buf);

               OSS_INLINE INT32 getPageID()const
               {
                  return _pageID;
               }
               OSS_INLINE UINT32 getFree()const
               {
                  return _free;
               }

            private:
               void updateFirstFree(UINT32 bitsCount, UINT32 beginBits);

            private:
               INT32 _pageID;
               UINT32 _free; /// free <= _size
               INT32 _firstFreeBits;
               UINT64 *_buf;

         };// class _inMemBitPage

      public:
         inMemBitMap();
         ~inMemBitMap();

      public:
         /// _pageCount will never become smaller.no latch protected.
         OSS_INLINE UINT32 getPageCount()const
         {
            return _pageCount;
         }
         OSS_INLINE BOOLEAN isInitialized()const
         {
            return 0 < _bitCountInPage;
         }
      public:
         /// bitCountInPage:
         /// you'd better make bitCountInPage equal to corresponding smp's(or id map page) capacity.
         /// inMemBitMap will always do allocating from only one page.
         /// it may cause a waste of resource, but can save the io times.
         /// freeBound:
         /// freeBound will be used to quickly skip bit pages with insufficient free count.
         INT32 init(UINT32 bitCountInPage, UINT32 freeBound = 0);
         INT32 fini();

         INT32 allocateNewBitPage(UINT32 occupied=0);

         INT32 allocateBits(UINT32 count, UINT32 *buf);

         void releaseBits(UINT32 count, const UINT32 *buf);

      public:
         INT32 mapNewBitPage(UINT32 bitPageID, const spaceManagementPageHead *head);

      private:
         INT32 allocateBitsFromHFC(UINT32 count, UINT32 *buf);
         INT32 allocateBitsFromLFC(UINT32 count, UINT32 *buf);

         void releaseBitFromHFC(UINT32 count,
                                 const UINT32 *buf,
                                 _inMemBitPage *page);
         void releaseBitFromLFC(UINT32 count,
                                 const UINT32 *buf,
                                 _inMemBitPage *page);
         void releaseAtDestroyedPage(INT32 pageID, UINT32 count, const UINT32 *buf);

      private:
         ossSpinXLatch _mutex;
         UINT32 _bitCountInPage;
         UINT32 _alignedBitsCount;
         UINT32 _freeBound;

         /// we will not keep bit pages in mem when it's free count is zero.
         /// _pageCount means total count of page we ever allocated.
         UINT32 _pageCount;
         typedef ossPoolMap<INT32, _inMemBitPage*> _PAGE_MAP;
         _PAGE_MAP _pagesWithLowFreeCount;
         _PAGE_MAP _pagesWithHighFreeCount;
   };//class inMemBitMap
} /// end of namespace vessel
} /// end of namespace engine
#endif // VESSEL_IN_MEM_BIT_MAP_H_