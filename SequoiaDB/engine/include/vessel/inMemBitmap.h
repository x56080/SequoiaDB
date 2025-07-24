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

   Source File Name = inMemBitmap.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef VESSEL_IN_MEM_BITMAP_H_
#define VESSEL_IN_MEM_BITMAP_H_

#include "ossLatch.hpp"
#include "utilPooledObject.hpp"
#include "ossMemPool.hpp"
#include "bitmapScanner.h"

namespace engine
{
namespace vessel
{
   class inMemBitmap : public SDBObject
   {
      private:
         class _inMemBitPage : public _utilPooledObject
         {
            public:
               _inMemBitPage();
               ~_inMemBitPage();
               _inMemBitPage(const _inMemBitPage &) = delete;
               _inMemBitPage &operator=(const _inMemBitPage &) = delete;

            public:
               BOOLEAN isReady()const
               {
                  return 0 <= _pageID && NULL != _buf;
               }

               INT32 init(INT32 pageId,
                          UINT32 capacity,
                          BOOLEAN noFree);

               void fini();

               INT32 allocate(UINT32 count,
                              UINT32 *buf);

               void release( UINT32 count, const UINT32 *buf);

               INT32 test(UINT32 offset,
                          BOOLEAN &isFree)const;

               INT32 occupy(UINT32 offset);

               OSS_INLINE INT32 getPageID()const
               {
                  return _pageID;
               }
               OSS_INLINE UINT32 getFree()const
               {
                  return _scanner.getNonZeroedCount();
               }

            private:
               INT32 _pageID = -1;
               UINT64 *_buf = NULL;
               bitmapScanner _scanner;

            public:
               BOOLEAN isInFreeList()const
               {
                  return _inList;
               }
               _inMemBitPage *_pre = NULL;
               _inMemBitPage *_next = NULL;
               BOOLEAN _inList = FALSE;
         };// class _inMemBitPage

      public:
         inMemBitmap();
         ~inMemBitmap();
         inMemBitmap(const inMemBitmap &) = delete;
         inMemBitmap &operator=(const inMemBitmap &) = delete;

      public:
         OSS_INLINE UINT32 getRealPageCount()const
         {
            return _pages.size();
         }
         OSS_INLINE UINT32 getCustomizedPageCount()const
         {
            return _o.bitmapBeginPage + _pages.size();
         }
         OSS_INLINE UINT32 getPageCapacity()const
         {
            return _pageCapacity;
         }
         OSS_INLINE BOOLEAN isInitialized()const
         {
            return 0 < _pageCapacity;
         }


      public:
         class options : public SDBObject
         {
            public:
               /// capacity * bitmapBeginPage bits will not managed by bitmap.
               UINT32 bitmapBeginPage = 0;

               UINT32 maxBitmapPageCount = UINT32(-1);

               BOOLEAN keepEmptyPageInMem = FALSE;

               FLOAT32 percentFreeReused = 0.0;
         };//class options
      public:       
         /// capacity must be 64 aligned.
         INT32 init(UINT32 pageCapacity,
                    const options &o);

         /// bitmap will not hold any latch in working.
         INT32 initWithNoLatch(UINT32 pageCapacity,
                             const options &o);
         void fini();

         /// All or nothing created.
         INT32 allocateNewBitmapPages(UINT32 count);

         INT32 ensureBitmapPageCount(UINT32 count);

         INT32 allocateBits(UINT32 count,
                            UINT32 *buf,
                            UINT32 autoExtendingCount = 0);

         void release(UINT32 bitOffset);

         void releaseBits(UINT32 count, const UINT32 *buf);

         INT32 occupy(UINT32 count, const UINT32 *buf);

         INT32 occupy(UINT32 offset);

         INT32 test(UINT32 offset, BOOLEAN &isFree)const;

      private:
         INT32 init(UINT32 pageCapacity,
                    const options &o,
                    ossSpinXLatch *latch);

         INT32 _allocateNewBitmapPages(UINT32 count);

      private:
         INT32 _allocateBits(UINT32 count, UINT32 *buf);
         void _releaseBits(UINT32 count, const UINT32 *buf);
         INT32 _occupy(UINT32 count, const UINT32 *buf);

      private:
         OSS_INLINE UINT32 getRealPos(INT32 pageId)const
         {
            SDB_ASSERT((INT32)_o.bitmapBeginPage <= pageId, "out of bound");
            return pageId - _o.bitmapBeginPage;
         }
         OSS_INLINE INT32 getNextPageId()const
         {
            return (INT32)(_pages.size() + _o.bitmapBeginPage);
         }

      private:
         /// WARNING: will not reset pre/next ptr of each item.
         void clearFreeList();

         BOOLEAN isFreeListEmpty()const;

         void pushBackToFL(_inMemBitPage *page);

         void pushFrontToFL(_inMemBitPage *page);

         void popFrontFromFL();

         void eraseFromFL(_inMemBitPage *page);
      private:
         typedef ossPoolVector<_inMemBitPage *> _PAGE_VEC;

      private:
         ossSpinXLatch _innerLatch;
         ossSpinXLatch *_latch = NULL;
         UINT32 _pageCapacity = 0;
         options _o;
         INT64 _totalFreeBitCount = 0;

         _PAGE_VEC _pages;
         
         /// free list
         UINT32 _lsize = 0;
         _inMemBitPage *_head = NULL;
         _inMemBitPage *_tail = NULL;
   };//class inMemBitmap
} /// end of namespace vessel
} /// end of namespace engine
#endif // VESSEL_IN_MEM_BITMAP_H_