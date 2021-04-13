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

   Source File Name = liteCachePageTag.h

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

#ifndef VESSEL_LITE_CACHE_PAGE_TAG_H_
#define VESSEL_LITE_CACHE_PAGE_TAG_H_

#include "vessel/phyExtentID.h"
#include "vessel/latch.h"
#include "ossUtil.h"
#include "dpsDef.hpp"
#include "vessel/freeListPage.h"
#include "ossSpinLatch.hpp"
#include "utilPooledObject.hpp"

namespace engine
{
namespace vessel
{
   const UINT16 LC_TAG_STATUS_INVALID = 0;
   const UINT16 LC_TAG_STATUS_NORMAL = 1;
   const UINT16 LC_TAG_STATUS_TO_BE_REMOVED = 2;

   const UINT16 LC_TAG_FAST_FLAG_IO_PENDING_WRITE = 0x01;
   const UINT16 LC_TAG_FAST_FLAG_DIRTY = 0x02;

   const UINT32 LC_TAG_LRU_FLAG_COLD = 0x01;

   const UINT32 LC_TAG_FLAG_DIRTY = 0x01;
   const UINT32 LC_TAG_FLAG_IN_DIRTY_LIST = 0x02;
   const UINT32 LC_TAG_FLAG_IN_LRU_LIST = 0x04;

   class liteCachePageTag : public _utilPooledObject
   {
      public:
         OSS_INLINE liteCachePageTag(){}
         OSS_INLINE ~liteCachePageTag(){}

         liteCachePageTag(const liteCachePageTag &) = delete;
         liteCachePageTag &operator=(const liteCachePageTag &) = delete;
      public:
         void reset();

         OSS_INLINE void firstInit(const GLOBAL_PAGE_ID &id,
                                   UINT32 pageSize)
         {
            _ts.status = LC_TAG_STATUS_NORMAL;
            _id = id;
            _pageSize = pageSize;
            return;
         }

         OSS_INLINE SHARED_MUTEX &rwMutex()
         {
            return _rwMutex;
         }
         OSS_INLINE const GLOBAL_PAGE_ID &id()const
         {
            return _id;
         }
         OSS_INLINE UINT32 getPageSize()const
         {
            return _pageSize;
         }

      public:
         OSS_INLINE void setStatusAsNormal()
         {
            ossSpinGuard guard(&_spinLatch);
            _ts.status = LC_TAG_STATUS_NORMAL;
            return;
         }
         OSS_INLINE BOOLEAN testFastFlags(UINT16 flags)
         {
            _spinLatch.lock();
            BOOLEAN r = OSS_BIT_TEST(_ts.flags, flags);
            _spinLatch.unlock();
            return r;
         }

         OSS_INLINE void setFastFlags(UINT16 flags)
         {
            _spinLatch.lock();
            OSS_BIT_SET(_ts.flags, flags);
            _spinLatch.unlock();
            return;
         }

         OSS_INLINE void clearFastFlags(UINT16 flags)
         {
            _spinLatch.lock();
            OSS_BIT_CLEAR(_ts.flags, flags);
            _spinLatch.unlock();
            return;
         }

      public:
         OSS_INLINE void setDiskPagePtr(ossValuePtr ptr)
         {
            _diskPagePtr = ptr;
            return;
         }

         OSS_INLINE BOOLEAN hasDiskPage()const
         {
            return 0 != _diskPagePtr;
         }

         OSS_INLINE ossValuePtr getDiskPagePtr()const
         {
            return _diskPagePtr;
         }

         OSS_INLINE BOOLEAN hasMemPage()const
         {
            return _memPage.valid();
         }

         OSS_INLINE void setMemPage(const freeListPage &page)
         {
            _memPage = page;
         }

         OSS_INLINE void releaseMemPage()
         {
            _memPage.reset();
         }
         
         OSS_INLINE const freeListPage &getMemPage()const
         {
            return _memPage;
         }

         OSS_INLINE freeListPage &getMemPage()
         {
            return _memPage;
         }

         OSS_INLINE void setMinAndMaxLSN(UINT64 lsn)
         {
            _minLSN = lsn;
            _maxLSN = lsn;
         }

         OSS_INLINE UINT64 getMinLSN()const
         {
            return _minLSN;
         }

         OSS_INLINE void setMaxLSN(UINT64 lsn)
         {
            _maxLSN = lsn;
         }

         OSS_INLINE UINT64 getMaxLSN()const
         {
            return _maxLSN;
         }

      public:
         OSS_INLINE BOOLEAN testFlags(UINT32 flags)const
         {
            return OSS_BIT_TEST(_flags, flags);
         }
         OSS_INLINE BOOLEAN noFlagsSet()const
         {
            return 0 == _flags;
         }

         OSS_INLINE BOOLEAN isInLruList()const
         {
            return OSS_BIT_TEST(_flags, LC_TAG_FLAG_IN_LRU_LIST);
         }
          OSS_INLINE BOOLEAN isInDirtyList()const
         {
            return OSS_BIT_TEST(_flags, LC_TAG_FLAG_IN_DIRTY_LIST);
         }
         OSS_INLINE BOOLEAN isDirty()const
         {
            return OSS_BIT_TEST(_flags, LC_TAG_FLAG_DIRTY);
         }
         OSS_INLINE void setNonDirty()
         {
            OSS_BIT_CLEAR(_flags, LC_TAG_FLAG_DIRTY);
         }

      public:
         /// under lru lock and w lock
         OSS_INLINE void insertIntoLru(liteCachePageTag *pre,
                                       liteCachePageTag *next,
                                       BOOLEAN isCold)
         {
            _lruPre = pre;
            _lruNext = next;
            _lruTouchCnt = 0;
            if (isCold)
            {
               OSS_BIT_SET(_lruFlags, LC_TAG_LRU_FLAG_COLD);
            }
            OSS_BIT_SET(_flags, LC_TAG_FLAG_IN_LRU_LIST);
         }

         OSS_INLINE void removeFromLru()
         {
            _lruPre = NULL;
            _lruNext = NULL;
            _lruTouchCnt = 0;
            _lruFlags = 0;
            OSS_BIT_CLEAR(_flags, LC_TAG_FLAG_IN_LRU_LIST);
         }

         OSS_INLINE void setLruCold()
         {
            OSS_BIT_SET(_lruFlags, LC_TAG_LRU_FLAG_COLD);
         }

         OSS_INLINE void setLruUncold()
         {
            OSS_BIT_CLEAR(_lruFlags, LC_TAG_LRU_FLAG_COLD);
         }

         OSS_INLINE BOOLEAN isLruCold()const
         {
            return OSS_BIT_TEST(_lruFlags, LC_TAG_LRU_FLAG_COLD);
         }

         OSS_INLINE void incLruTouchCnt()
         {
            ++_lruTouchCnt;
         }

         OSS_INLINE void incLruTouchCntWithCAS()
         {
            ossFetchAndIncrement32(&_lruTouchCnt);
         }

         OSS_INLINE UINT32 getLruTouchCnt()
         {
            return _lruTouchCnt;
         }

         OSS_INLINE void setLruPre(liteCachePageTag *pre)
         {
            _lruPre = pre;
         }

         OSS_INLINE liteCachePageTag *getLruPre()
         {
            return _lruPre;
         }

         OSS_INLINE void setLruNext(liteCachePageTag *next)
         {
            _lruNext = next;
         }

         OSS_INLINE liteCachePageTag *getLruNext()
         {
            return _lruNext;
         }

      public:
         /// under dirty list lock and w lock
         OSS_INLINE void insertIntoDirtyList(liteCachePageTag *pre,
                                             liteCachePageTag *next)
         {
            _dirtyPre = pre;
            _dirtyNext = next;
            OSS_BIT_SET(_flags, LC_TAG_FLAG_IN_DIRTY_LIST|LC_TAG_FLAG_DIRTY);
         }
         OSS_INLINE void removeFromDirtyList()
         {
            _dirtyPre = NULL;
            _dirtyNext = NULL;
            SDB_ASSERT(!OSS_BIT_TEST(_flags, LC_TAG_FLAG_DIRTY), "can not be dirty");
            OSS_BIT_CLEAR(_flags, LC_TAG_FLAG_IN_DIRTY_LIST);
         }
         OSS_INLINE void setDirtyListPre(liteCachePageTag *tag)
         {
            _dirtyPre = tag;
         }
         OSS_INLINE void setDirtyListNext(liteCachePageTag *tag)
         {
            _dirtyNext = tag;
         }
         OSS_INLINE liteCachePageTag *getDirtyListPre()
         {
            return _dirtyPre;
         }
         OSS_INLINE liteCachePageTag *getDirtyListNext()
         {
            return _dirtyNext;
         }

      public:
         /** get tag in bucket begin **/
         /// WARNING: lock can be set as false only when first created.
         OSS_INLINE BOOLEAN incUsageCnt(BOOLEAN lock=TRUE);
         OSS_INLINE void decUsageCnt();
         /** get tag in bucket end **/

         /** remove tag from bucket begin **/
         /// spin latch is not necessary when performing precheck.
         /// it just affects performance and accuracy.
         /// WANRING: we can not recycle the tag already set as removed.
         /// noFlagsSet() will not be protected by spin latch.
         OSS_INLINE BOOLEAN testIfCanBeRecycled(BOOLEAN lock=TRUE)
         {
            BOOLEAN r = FALSE;
            ossSpinLatch *latch = lock ? &_spinLatch : NULL;
            ossSpinGuard guard(latch);
            r = noFlagsSet() && isNormalAndUnpinned();
            return r;
         }

         /// under rw latch.
         OSS_INLINE BOOLEAN tryToSetRemoved();

         OSS_INLINE BOOLEAN isRemoved()
         {
            ossSpinGuard guard(&_spinLatch);
            return (LC_TAG_STATUS_TO_BE_REMOVED == _ts.status);
         }
         /** remove tag in bucket end **/

         /** evict tag in lru begin **/
         /// return true if can be evicted.
         OSS_INLINE BOOLEAN testIfCanBeEvictedFromLru(BOOLEAN lock=TRUE)
         {
            ossSpinLatch *latch = lock ? &_spinLatch : NULL;
            ossSpinGuard guard(latch);
            return !isMemBufferPinned();
         }

         OSS_INLINE BOOLEAN setPendingWriteIfDirty()
         {
            BOOLEAN r = FALSE;
            ossSpinGuard guard(&_spinLatch);
            if (isDirtyAndNoPending())
            {
               OSS_BIT_SET(_ts.flags, LC_TAG_FAST_FLAG_IO_PENDING_WRITE);
               r = TRUE;
            }
            return r;
         }
         /** evict tag in lru end **/

         OSS_INLINE BOOLEAN isPendingWrite(BOOLEAN lock=TRUE)
         {
            ossSpinLatch *latch = lock ? &_spinLatch : NULL;
            ossSpinGuard guard(latch);
            return OSS_BIT_TEST(_ts.flags, LC_TAG_FAST_FLAG_IO_PENDING_WRITE);
         }

         /// under dirty list w lock
         OSS_INLINE BOOLEAN setPendingWrite()
         {
            BOOLEAN r = FALSE;
            ossSpinGuard guard(&_spinLatch);
            if (isNoPending())
            {
               OSS_BIT_SET(_ts.flags, LC_TAG_FAST_FLAG_IO_PENDING_WRITE);
               r = TRUE;
            }
            return r;
         }

         OSS_INLINE void setUnPendingWrite()
         {
            ossSpinGuard guard(&_spinLatch);
            SDB_ASSERT(LC_TAG_STATUS_NORMAL == _ts.status, "must be normal");
            OSS_BIT_CLEAR(_ts.flags, LC_TAG_FAST_FLAG_IO_PENDING_WRITE);
            return ;
         }

      private:
         OSS_INLINE BOOLEAN isPinned()const
         {
            return 0 < _ts.usageCnt || 0 != _ts.flags;
         }

         OSS_INLINE BOOLEAN isMemBufferPinned()const
         {
            return 0 < _ts.usageCnt ||
                   OSS_BIT_TEST(_ts.flags, LC_TAG_FAST_FLAG_DIRTY);
         }

         OSS_INLINE BOOLEAN isNormalAndUnpinned() const
         {
            return LC_TAG_STATUS_NORMAL == _ts.status &&
                   !isPinned();
         }

         OSS_INLINE BOOLEAN isDirtyAndNoPending()const
         {
            SDB_ASSERT(LC_TAG_STATUS_NORMAL == _ts.status, "must be normal");
            return OSS_BIT_TEST(_ts.flags, LC_TAG_FAST_FLAG_DIRTY) &&
                   !OSS_BIT_TEST(_ts.flags, LC_TAG_FAST_FLAG_IO_PENDING_WRITE);
         }

         OSS_INLINE BOOLEAN isNoPending()const
         {
            SDB_ASSERT(LC_TAG_STATUS_NORMAL == _ts.status, "must be normal");
            return !OSS_BIT_TEST(_ts.flags, LC_TAG_FAST_FLAG_IO_PENDING_WRITE);
         }
         
      private:
         struct _TagState
         {
            OSS_INLINE _TagState(){}
            OSS_INLINE ~_TagState(){}

            UINT16 status = LC_TAG_STATUS_INVALID;
            UINT16 flags = 0;
            UINT32 usageCnt = 0;
         };

      private:
         ossSpinLatch _spinLatch;
         SHARED_MUTEX _rwMutex;

         /// readonly after firstInitTag
         GLOBAL_PAGE_ID _id;
         UINT32 _pageSize = 0;

         /// protected by spin latch.
         _TagState _ts;

         /// protected by rw latch.
         ossValuePtr _diskPagePtr = 0;
         UINT32 _flags = 0;
         UINT64 _minLSN = DPS_INVALID_LSN_OFFSET;
         UINT64 _maxLSN = DPS_INVALID_LSN_OFFSET;
         freeListPage _memPage;
         
         /// lru list, protected by lru latch and rw latch(except _lruTouchCnt)
         UINT32 _lruTouchCnt = 0;
         UINT32 _lruFlags = 0;
         liteCachePageTag *_lruPre = NULL;
         liteCachePageTag *_lruNext = NULL;
         /// dirty list, protected by dirty list latch and rw latch
         liteCachePageTag *_dirtyPre = NULL;
         liteCachePageTag *_dirtyNext = NULL;

   }; /// end of class liteCachePageTag

   OSS_INLINE BOOLEAN liteCachePageTag::incUsageCnt(BOOLEAN lock)
   {
      BOOLEAN r = FALSE;
      ossSpinLatch *latch = lock ? &_spinLatch : NULL;
      ossSpinGuard guard(latch);
      SDB_ASSERT(LC_TAG_STATUS_INVALID != _ts.status,
                 "should not inc invalid tag's usage cnt");
      if (LC_TAG_STATUS_NORMAL == _ts.status)
      {
         ++_ts.usageCnt;
         r = TRUE;
      }
      return r;
   }

   OSS_INLINE void liteCachePageTag::decUsageCnt()
   {
      ossSpinGuard guard(&_spinLatch);
      SDB_ASSERT(LC_TAG_STATUS_INVALID != _ts.status,
                 "should not inc invalid tag's usage cnt");
      SDB_ASSERT(0 != _ts.usageCnt, "can not be zero");
      --_ts.usageCnt;
      return;
   }

   /// under rw lock
   /// WARNING: validate flags under rw lock first.
   OSS_INLINE BOOLEAN liteCachePageTag::tryToSetRemoved()
   {
      BOOLEAN r = FALSE;
      ossSpinGuard guard(&_spinLatch);
      if (isNormalAndUnpinned())
      {
         _ts.status = LC_TAG_STATUS_TO_BE_REMOVED;
         r = TRUE;
      }
      return r;
   }
} /// end of namespace vessel
} /// end of namespace engine


#endif /// define VESSEL_LITE_CACHE_PAGE_TAG_H_
