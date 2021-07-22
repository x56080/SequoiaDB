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

   Source File Name = fsmBitmapPageObject.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

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
      SDB_ASSERT((FSM_BITMAP_PAGE_CAPACITY / 64) < 65535, "we save _where as uint16");
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
         _where[i] = 0;
         _stats[i] = 0;
      }
      return;
   }

   INT32 fsmBitmapPageObject::init(UINT32 pageNo,
                                   PAGE_ID pid,
                                   UINT32 dataPageCount,
                                   fsmBitmapPage *page,
                                   UINT32 &abnormalCount)
   {
      INT32 rc = SDB_OK;
      UINT32 bitsCount = 0;
      SDB_ASSERT(dataPageCount <= FSM_BITMAP_PAGE_CAPACITY, "impossible");
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

      if (0 == dataPageCount)
      {
         goto done;
      }

      bitsCount = ossAlign64(dataPageCount) >> 6;

      /// OS crashing may occur error stats. 
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

   INT32 fsmBitmapPageObject::ensureSize(UINT32 size)
   {
      INT32 rc = SDB_OK;
      if (OSS_UNLIKELY(!isValid()))
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }
      else if (OSS_UNLIKELY(FSM_BITMAP_PAGE_CAPACITY < size))
      {
         PD_LOG(PDERROR, "data page count out of range");
         rc = SDB_VESSEL_OPERATOION_NOT_PERMITTED;
         goto error;
      }
      else if (_size < size)
      {
         _size = size;
      }

   done:
      return rc;
   error:
      goto done;
   }

   void fsmBitmapPageObject::clearBitAtLvLs(INT32 exceptLvl, UINT32 bitOffset)
   {
      SDB_ASSERT(isValid(), "can not be invalid");
      SDB_ASSERT(bitOffset < _size, "can not be out of range");
      UINT32 count = (bitOffset >> 6) + 1;

      for (INT32 i = 0; i < FSM_SPACE_LVL_COUNT; ++i)
      {
         if (i == exceptLvl)
         {
            continue;
         }
         
         if (setNotFreeIfFree64(count, _page->lvlBitmaps[i], bitOffset))
         {
            --_stats[i];
         }
      }
      return;
   }

   void fsmBitmapPageObject::atomicClearBitAtLvls(INT32 exceptLvl, UINT32 bitOffset)
   {
      SDB_ASSERT(isValid(), "can not be invalid");
      SDB_ASSERT(bitOffset < _size, "can not be out of range");
      UINT32 count = (bitOffset >> 6) + 1;

      for (INT32 i = 0; i < FSM_SPACE_LVL_COUNT; ++i)
      {
         if (i == exceptLvl)
         {
            continue;
         }

         if (!testBitIsFree(count, _page->lvlBitmaps[i], bitOffset))
         {
            continue;
         }
         
         atomicUnsetLvl(count, bitOffset, i);
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

      for (INT32 i = targetLvl; i < FSM_SPACE_LVL_COUNT; ++i)
      {
         UINT32 offset = 0;
         if (findAndClear(i, offset))
         {
            found = TRUE;
            seq = _pageNo * FSM_BITMAP_PAGE_CAPACITY + offset;
            realLvl = i;

            /// In case of unexpected error stats, clear it in other lvls.
            clearBitAtLvLs(i, offset);
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

   BOOLEAN fsmBitmapPageObject::findAndClear(INT32 lvl,
                                             UINT32 &offset)
   {
      BOOLEAN r = FALSE;
      SDB_ASSERT(isValidFsmLvL(lvl), "can not be invalid");
      SDB_ASSERT(0 < _size, "can not be zero");

      UINT32 where = _where[lvl];
      UINT32 maxBitsCount = ossAlign64(_size) >> 6;
      SDB_ASSERT(where < maxBitsCount, "impossible");

      do
      {
         if (_stats[lvl] <= 0)
         {
            goto done;
         }

         if (findAndClearFirstFreeBitFromBit64(maxBitsCount,
                                               (INT32)(_where[lvl]),
                                               _page->lvlBitmaps[lvl],
                                               offset))
         {
            --_stats[lvl];
            _where[lvl] = (offset >> 6);
            if (_size <= offset)
            {
               /// We always zeroed bitmap when creating and only
               /// set it as non-zero when upgrading lvl.
               /// In case of unexpected error, we check it again.
               /// If it is out of bound, just continue.
               continue;
            }
            else
            {
               r = TRUE;
               goto done;
            }
         }
         else
         {
            /// searched from 'where' to the end.  
            break;
         }
      } while (TRUE);
      

      SDB_ASSERT(0 < _stats[lvl], "impossible");
      if (0 != where)
      {
         /// search [0, where)
         if (findAndClearFirstFreeBitFromBit64(where, -1,
                                             _page->lvlBitmaps[lvl], offset))
         {
            /// Offset may be out of bound only at the last uint64.
            SDB_ASSERT(offset < _size, "impossible");
            --_stats[lvl];
            _where[lvl] = (offset >> 6);
            r = TRUE;
            goto done;
         }
      }

      /// We have searched all bitmap, but nothing found.
      /// It may caused by error stats or some reason else.
      /// Just set stat as zero
      PD_LOG(PDWARNING, "stat is not zero but nothing found");
      _stats[lvl] = 0;
      _where[lvl] = 0;
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
      else if (INVALID_CL_PAGE_SEQ == seq || 
               !isValidFsmLvL(lvl))
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

      if (!atomicSetLvl(bitsCount, offset, lvl))
      {
         ///At same lvl, do nothing. It may caused by flushing missed.
         goto done;
      }

      upgraded = TRUE;
      atomicClearBitAtLvls(lvl, offset);
   done:
      return rc;
   error:
      goto done;
   }

   INT32 fsmBitmapPageObject::downgradePageSpaceLvl(UINT32 seq,
                                                    INT32 lvl,
                                                    BOOLEAN &downgraded)
   {
      INT32 rc = SDB_OK;
      UINT32 offset = 0;
      UINT32 bitsCount = 0;
      downgraded = FALSE;
      INT32 notZeroLvls[FSM_SPACE_LVL_COUNT] = {0};
      UINT32 notZeroLvlCount = 0;

      if (OSS_UNLIKELY(!isValid()))
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }
      else if (INVALID_CL_PAGE_SEQ == seq || 
               !isValidFsmLvL(lvl))
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

      for (INT32 i = 0; i < FSM_SPACE_LVL_COUNT; ++i)
      {
         if (testBitIsFree(bitsCount, _page->lvlBitmaps[i], offset))
         {
            notZeroLvls[i] = 1;
            ++notZeroLvlCount;
         }
      }

      if (0 == notZeroLvlCount)
      {
         /// Do not downgrade lvl if not exists.
         goto done;
      }

      for (INT32 i = 0; i < FSM_SPACE_LVL_COUNT; ++i)
      {
         if (0 == notZeroLvls[i])
         {
            if (i == lvl)
            {
               atomicSetLvl(bitsCount, offset, lvl);
            }
         }
         else
         {
            if (i != lvl)
            {
               atomicUnsetLvl(bitsCount, offset, lvl);
            }
         }
      }

      downgraded = TRUE;
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
      SDB_ASSERT(offset < FSM_BITMAP_PAGE_CAPACITY, "can not beo out of range");
      SDB_ASSERT(isValidFsmLvL(lvl), "can not be invalid");
      BOOLEAN r = FALSE;

      /// The bit we are accessing may not updating by others.
      if (testBitIsFree(bitsCount, _page->lvlBitmaps[lvl], offset))
      {
         ///At same lvl, do nothing.
         goto done;
      }

      setFreeWithAtomic64(bitsCount, _page->lvlBitmaps[lvl], offset);
      ossFetchAndIncrement32(_stats + lvl);
      r = TRUE;

   done:
      return r;
   }

   BOOLEAN fsmBitmapPageObject::atomicUnsetLvl(UINT32 bitsCount,
                                               UINT32 offset,
                                               INT32 lvl)
   {
      SDB_ASSERT(isValid(), "must be valid");
      SDB_ASSERT(offset < FSM_BITMAP_PAGE_CAPACITY, "can not beo out of range");
      SDB_ASSERT(isValidFsmLvL(lvl), "can not be invalid");
      BOOLEAN r = FALSE;
      if (!testBitIsFree(bitsCount, _page->lvlBitmaps[lvl], offset))
      {
         ///At same lvl, do nothing.
         goto done;
      }

      setNotFreeWithAtomic64(bitsCount, _page->lvlBitmaps[lvl], offset);
      ossFetchAndDecrement32(_stats + lvl);
      r = TRUE;

   done:
      return r;
   }

   void fsmBitmapPageObject::clearFromOffsetToTheEnd(INT32 lvl, UINT32 offset)
   {
      SDB_ASSERT(NULL != _page, "can not be null");
      SDB_ASSERT(isValidFsmLvL(lvl), "can not be invalid");
      SDB_ASSERT(offset < FSM_BITMAP_PAGE_CAPACITY, "can not be out of bound");

      UINT32 bitsCount = FSM_BITMAP_PAGE_CAPACITY >> 6;
      UINT64 *bitmap = _page->lvlBitmaps[lvl];

   }
}//namespace vessel
}//namespace engine