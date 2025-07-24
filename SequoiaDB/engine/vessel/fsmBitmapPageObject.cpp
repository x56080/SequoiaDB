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

   Source File Name = fsmBitmapPageObject.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

*******************************************************************************/
#include "vessel/fsmBitmapPageObject.h"
#include "pdTrace.hpp"
#include "ossLikely.hpp"
#include "vessel/bitmapUtils.h"

namespace engine
{
namespace vessel
{
   fsmBitmapPageObject::fsmBitmapPageObject()
   {
      
   }

   fsmBitmapPageObject::~fsmBitmapPageObject()
   {
      
   }

   void fsmBitmapPageObject::reset()
   {
      _pageNo = 0;
      _pid = INVALID_PAGE_ID;
      _page = NULL;
      _size = 0;
      for (UINT32 i = 0; i < FSM_SPACE_LVL_COUNT; ++i)
      {
         _stats[i] = 0;
      }
      return;
   }

   INT32 fsmBitmapPageObject::initWhenCreate(UINT32 pageNo,
                                             PAGE_ID pid,
                                             fsmBitmapPage *page)
   {
      INT32 rc = SDB_OK;

      reset();

      if (OSS_UNLIKELY(NULL == page ||
                       INVALID_PAGE_ID == pid))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      _pageNo = pageNo;
      _pid = pid;
      _page = page;
      _size = 0;

   done:
      return rc;
   error:
      reset();
      goto done;
   }

   INT32 fsmBitmapPageObject::initWhenOpen(UINT32 pageNo,
                                           PAGE_ID pid,
                                           UINT32 dataPageCount,
                                           fsmBitmapPage *page,
                                           UINT32 &abnormalCount)
   {
      INT32 rc = SDB_OK;
      UINT32 bitsCount = 0;
      SDB_ASSERT(dataPageCount <= FSM_BITMAP_PAGE_CAPACITY, "impossible");
      SDB_ASSERT(ossIsAligned64(FSM_BITMAP_PAGE_CAPACITY), "must be aligned");
      abnormalCount = 0;

      reset();

      if (OSS_UNLIKELY(NULL == page ||
                       INVALID_PAGE_ID == pid ||
                       FSM_BITMAP_PAGE_CAPACITY < dataPageCount))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      _pageNo = pageNo;
      _pid = pid;
      _page = page;
      _size = dataPageCount;

      bitsCount = FSM_BITMAP_PAGE_CAPACITY / 64;

      if (dataPageCount < FSM_BITMAP_PAGE_CAPACITY)
      {
         for (UINT32 i = 0; i < FSM_SPACE_LVL_COUNT; ++i)
         {
            UINT32 offset = 0;
            if (lowerBoundFirstNonzeroBit(bitsCount,
                                          dataPageCount,
                                          _page->lvlBitmaps[i],
                                          offset))
            {
               clearFromOffsetToTheEnd(bitsCount, dataPageCount, _page->lvlBitmaps[i]);
            }
         }
      }

      if (0 == dataPageCount)
      {
         goto done;
      }
      
      /// A page may exist in mutiple lvls here,
      /// which caused by system crashing.
      /// It may be put into more than one buckets.
      for (UINT32 i = 0; i < FSM_SPACE_LVL_COUNT; ++i)
      {
         const UINT64 *bits = _page->lvlBitmaps[i];
         for (UINT32 j = 0; j < bitsCount; ++j)
         {
            UINT32 cnt = ossGetNonZeroBitCount64(bits[j]);
            if (0 < cnt)
            {
               _stats[i] += cnt;
            }
         }
      }
   done:
      return rc;
   error:
      reset();
      goto done;
   }

   void fsmBitmapPageObject::resetSizeIfHigher(UINT32 size)
   {
      SDB_ASSERT(isValid(), "can not be invalid");
      SDB_ASSERT(size <= FSM_BITMAP_PAGE_CAPACITY, "out of bound");
      if (_size < size)
      {
         _size = size;
      }

      return;
   }

   INT32 fsmBitmapPageObject::findAndClear(INT32 targetLvl,
                                           BOOLEAN &found,
                                           UINT32 &seq,
                                           INT32 &realLvl)
   {
      INT32 rc = SDB_OK;

      if (OSS_UNLIKELY(!isValid()))
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }
      else if (OSS_UNLIKELY(!isValidFsmLvL(targetLvl)))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      found = FALSE;
      if (0 == _size)
      {
         goto done;
      }

      for (INT32 i = targetLvl; i < (INT32)FSM_SPACE_LVL_COUNT; ++i)
      {
         UINT32 offset = 0;
         if (atomicFindAndClear(i, offset))
         {
            found = TRUE;
            seq = _pageNo * FSM_BITMAP_PAGE_CAPACITY + offset;
            realLvl = i;
            break;
         }
         /// If not found in target lvl, find in upper lvl.
      }

   done:
      return rc;
   error:
      goto done;
   }

   INT32 fsmBitmapPageObject::getStats(INT32 lvl)const
   {
      SDB_ASSERT(isValidFsmLvL(lvl), "can not be invalid");
      return _stats[lvl];
   }

   BOOLEAN fsmBitmapPageObject::atomicFindAndClear(INT32 lvl,
                                                   UINT32 &offset)
   {
      BOOLEAN r = FALSE;
      SDB_ASSERT(NULL != _page, "can not be null");
      SDB_ASSERT(isValidFsmLvL(lvl), "can not be invalid");
      SDB_ASSERT(0 < _size, "can not be zero");
      UINT32 count = 0;

      if (_stats[lvl] <= 0)
      {
         goto done;
      }

      count = ossAlign64(_size) >> 6;

      r = atomicFindAndClearFirstNonzeroBit(count, 0,
                                            _page->lvlBitmaps[lvl],
                                            offset);
      if (r)
      {
         ossFetchAndDecrement32(_stats + lvl);
      }
      
   done:
      return r;
   }

   INT32 fsmBitmapPageObject::upgradePageSpaceLvl(UINT32 seq,
                                                  INT32 lvl,
                                                  BOOLEAN &upgraded)
   {
      INT32 rc = SDB_OK;
      UINT32 offset = 0;
      UINT32 bitsCount = 0;
      upgraded = FALSE;

      if (OSS_UNLIKELY(!isValid()))
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }
      else if (!isValidFsmLvL(lvl))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      offset = seq % FSM_BITMAP_PAGE_CAPACITY;
      if (_size <= offset)
      {
         rc = SDB_OUT_OF_BOUND;
         goto error;
      }
      
      bitsCount = (offset >> 6) + 1;
      for (INT32 i = 0; i < (INT32)FSM_SPACE_LVL_COUNT; ++i)
      {
         if (i == lvl)
         {
            continue;
         }

         if (testBitIsNonzero(bitsCount, _page->lvlBitmaps[i], offset))
         {
            atomicUnsetLvl(bitsCount, offset, i);
         }
      }

      upgraded = atomicSetLvl(bitsCount, offset, lvl);
   done:
      return rc;
   error:
      goto done;
   }

   INT32 fsmBitmapPageObject::downgradePageSpaceLvl(UINT32 seq,
                                                    INT32 lvl)
   {
      INT32 rc = SDB_OK;
      UINT32 offset = 0;
      UINT32 bitsCount = 0;
      UINT32 notZeroLvlCount = 0;

      if (OSS_UNLIKELY(!isValid()))
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }
      else if (!isValidFsmLvL(lvl))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      offset = seq % FSM_BITMAP_PAGE_CAPACITY;
      if (_size <= offset)
      {
         rc = SDB_OUT_OF_BOUND;
         goto error;
      }
      
      bitsCount = (offset >> 6) + 1;

      for (INT32 i = 0; i < (INT32)FSM_SPACE_LVL_COUNT; ++i)
      {
         if (i == lvl)
         {
            continue;
         }

         if (testBitIsNonzero(bitsCount, _page->lvlBitmaps[i], offset))
         {
            if (atomicUnsetLvl(bitsCount, offset, i))
            {
               ++notZeroLvlCount;
            }  
         }
      }

      if (0 == notZeroLvlCount)
      {
         /// If the page is not at any other lvls:
         /// 1. The page is already at target, do nothing;
         /// 2. The page is not at target lvl too:
         ///    2.1. It may be in bucket now, do nothing;
         ///    2.2. For some reason lvl info lost. It will corrected at next upgrading.
         goto done;
      }

      atomicUnsetLvl(bitsCount, offset, lvl);
   done:
      return rc;
   error:
      goto done;
   }

   BOOLEAN fsmBitmapPageObject::atomicSetLvl(UINT32 bitsCount,
                                             UINT32 offset,
                                             INT32 lvl)
   {
      SDB_ASSERT(isValid(), "must be valid");
      SDB_ASSERT(offset < _size, "can not beo out of range");
      SDB_ASSERT(isValidFsmLvL(lvl), "can not be invalid");
      BOOLEAN r = FALSE;
      atomicSetBitAtOffset(bitsCount, _page->lvlBitmaps[lvl],
                           offset, &r);
      if (r)
      {
         ossFetchAndIncrement32(_stats + lvl);
      }

   done:
      return r;
   }

   BOOLEAN fsmBitmapPageObject::atomicUnsetLvl(UINT32 bitsCount,
                                               UINT32 offset,
                                               INT32 lvl)
   {
      SDB_ASSERT(isValid(), "must be valid");
      SDB_ASSERT(offset < _size, "can not beo out of range");
      SDB_ASSERT(isValidFsmLvL(lvl), "can not be invalid");
      BOOLEAN r = FALSE;

      atomicClearBitAtOffset(bitsCount, _page->lvlBitmaps[lvl],
                             offset, &r);
      if (r)
      {
         ossFetchAndDecrement32(_stats + lvl);
      }

   done:
      return r;
   }

}//namespace vessel
}//namespace engine