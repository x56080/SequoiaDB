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

#include "vessel/globalPageID.h"
#include "ossUtil.h"
#include "dpsDef.hpp"
#include "vessel/freeListPage.h"
#include "ossSpinLatch.hpp"
#include "ossLatch.hpp"
#include "utilPooledObject.hpp"
#include "vessel/lcBucketInnerIndex.h"

namespace engine
{
namespace vessel
{
   const UINT16 LC_TAG_STATUS_INVALID = 0;
   const UINT16 LC_TAG_STATUS_NORMAL = 1;
   const UINT16 LC_TAG_STATUS_TO_BE_REMOVED = 2;

   const UINT16 LC_TAG_FAST_FLAG_IO_PENDING_WRITE = 0x01;
   //const UINT16 LC_TAG_FAST_FLAG_DIRTY = 0x02;

   const UINT32 LC_TAG_LRU_FLAG_COLD = 0x01;

   const UINT32 LC_TAG_FLAG_IN_BUCKET = 0x01;
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
                                   ossValuePtr ptr)
         {
            _ts.status = LC_TAG_STATUS_NORMAL;
            _id = id;
            _diskPagePtr = ptr;
            return;
         }

         OSS_INLINE ossSharedLatch &getAccessingLatch()
         {
            return _accessingLatch;
         }
         OSS_INLINE const GLOBAL_PAGE_ID &id()const
         {
            return _id;
         }

      public:
         OSS_INLINE void setStatusAsNormal()
         {
            ossSpinGuard guard(&_pinLatch);
            _ts.status = LC_TAG_STATUS_NORMAL;
            return;
         }
         OSS_INLINE BOOLEAN testFastFlags(UINT16 flags)
         {
            _pinLatch.lock();
            BOOLEAN r = OSS_BIT_TEST(_ts.flags, flags);
            _pinLatch.unlock();
            return r;
         }

         OSS_INLINE void setFastFlags(UINT16 flags)
         {
            _pinLatch.lock();
            OSS_BIT_SET(_ts.flags, flags);
            _pinLatch.unlock();
            return;
         }

      public:
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

         OSS_INLINE void setMinAndMaxDirtyLSN(UINT64 lsn)
         {
            _minDirtyLSN = lsn;
            _maxMemDirtyLSN = lsn;
         }

         OSS_INLINE UINT64 getMinDirtyLSN()const
         {
            return _minDirtyLSN;
         }

         OSS_INLINE void setMaxMemDirtyLSN(UINT64 lsn)
         {
            _maxMemDirtyLSN = lsn;
         }

         OSS_INLINE UINT64 getMaxMemDirtyLSN()const
         {
            return _maxMemDirtyLSN;
         }

         OSS_INLINE BOOLEAN isNotInAnyList()const
         {
            return !isInLruList() && !isInDirtyList();
         }

         OSS_INLINE BOOLEAN isMemPageDirty()const
         {
            return DPS_INVALID_LSN_OFFSET != _maxMemDirtyLSN;
         }

      public:
         OSS_INLINE void insertIntoBucket(const LC_BUCKET_INNER_INDEX_ITERATOR &itr)
         {
            OSS_BIT_SET(_flags, LC_TAG_FLAG_IN_BUCKET);
            _bucketItr = itr;
         }

         OSS_INLINE const LC_BUCKET_INNER_INDEX_ITERATOR &getBucketIterator()const
         {
            return _bucketItr;
         }

         OSS_INLINE void removedFromBucket()
         {
            OSS_BIT_CLEAR(_flags, LC_TAG_FLAG_IN_BUCKET);
            _bucketItr = LC_BUCKET_INNER_INDEX_ITERATOR();
         }

         OSS_INLINE BOOLEAN isInBucket()const
         {
            return 0 != OSS_BIT_TEST(_flags, LC_TAG_FLAG_IN_BUCKET);
         }

      public:
         OSS_INLINE BOOLEAN isInLruList()const
         {
            /// Do not user lru pre/next ptr to check if in lru list.
            /// Ptrs may modified with out holding accessing latch by lru.
            /// Or, if this is only one tuple in list, both pre and next
            /// are null.
            return 0 != OSS_BIT_TEST(_flags, LC_TAG_FLAG_IN_LRU_LIST);
         }
         /// under lru lock and w lock
         OSS_INLINE void insertIntoLru(liteCachePageTag *pre,
                                       liteCachePageTag *next,
                                       BOOLEAN isCold)
         {
            OSS_BIT_SET(_flags, LC_TAG_FLAG_IN_LRU_LIST);
            _lruPre = pre;
            _lruNext = next;
            _lruTouchCnt = 0;
            if (isCold)
            {
               OSS_BIT_SET(_lruFlags, LC_TAG_LRU_FLAG_COLD);
            }
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

         OSS_INLINE void setLruTouchCnt(UINT32 n)
         {
            _lruTouchCnt = n;
         }

         OSS_INLINE void incLruTouchCnt()
         {
            ++_lruTouchCnt;
         }

         OSS_INLINE void incLruTouchCntWithAtom()
         {
            ossFetchAndIncrement32(&_lruTouchCnt);
         }

         OSS_INLINE UINT32 fetchLruTouchCnt()
         {
            return ossAtomicFetch32(&_lruTouchCnt);
         }

         OSS_INLINE UINT32 getLruTouchCnt()const
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
         OSS_INLINE BOOLEAN isInDirtyList()const
         {
            /// Do not user dirty pre/next ptr to check if in dirty list.
            return 0 != OSS_BIT_TEST(_flags, LC_TAG_FLAG_IN_DIRTY_LIST);
         }
         /// under dirty list lock and w lock
         OSS_INLINE void insertIntoDirtyList(liteCachePageTag *pre,
                                             liteCachePageTag *next)
         {
            OSS_BIT_SET(_flags, LC_TAG_FLAG_IN_DIRTY_LIST);
            _dirtyPre = pre;
            _dirtyNext = next;
         }
         OSS_INLINE void removeFromDirtyList()
         {
            _dirtyPre = NULL;
            _dirtyNext = NULL;
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
         /// pin latch is not necessary when performing precheck.
         /// it just affects performance and accuracy.
         /// WANRING: we can not recycle the tag already set as removed.
         OSS_INLINE BOOLEAN fastTestIfCanBeRecycled(BOOLEAN lock=TRUE)
         {
            BOOLEAN r = FALSE;
            ossSpinLatch *latch = lock ? &_pinLatch : NULL;
            ossSpinGuard guard(latch);
            r = isNotInAnyList() && isNormalAndUnpinned();
            return r;
         }

         /// under rw latch.
         OSS_INLINE BOOLEAN tryToSetRemoved();

         /// under rw latch
         /// Used to rollback tag just allocated.
         /// The tag must not be in any list.
         /// Will decrease usage count and 
         /// try to set removed. So do not decrease
         /// usage count out side again.
         OSS_INLINE BOOLEAN tryToRollbackNewTag();

         OSS_INLINE BOOLEAN isRemoved(BOOLEAN lock=TRUE)
         {
            ossSpinLatch *latch = lock ? &_pinLatch : NULL;
            ossSpinGuard guard(latch);
            return (LC_TAG_STATUS_TO_BE_REMOVED == _ts.status);
         }
         /** remove tag in bucket end **/

         /** evict tag in lru begin **/
         /// return true if "may" be evicted.
         OSS_INLINE BOOLEAN fastTestIfCanBeEvictedFromLru(BOOLEAN lock=TRUE)
         {
            ossSpinLatch *latch = lock ? &_pinLatch : NULL;
            ossSpinGuard guard(latch);
            return isNormalAndUnpinned() && !isMemPageDirty();
         }
         
         /** evict tag in lru end **/

         OSS_INLINE BOOLEAN isPendingWrite(BOOLEAN lock=TRUE)
         {
            ossSpinLatch *latch = lock ? &_pinLatch : NULL;
            ossSpinGuard guard(latch);
            return OSS_BIT_TEST(_ts.flags, LC_TAG_FAST_FLAG_IO_PENDING_WRITE);
         }

         /// under dirty list w lock
         OSS_INLINE BOOLEAN setPendingWrite()
         {
            BOOLEAN r = FALSE;
            ossSpinGuard guard(&_pinLatch);
            if (isNoPending())
            {
               OSS_BIT_SET(_ts.flags, LC_TAG_FAST_FLAG_IO_PENDING_WRITE);
               r = TRUE;
            }
            return r;
         }

         OSS_INLINE void setUnPendingWrite()
         {
            ossSpinGuard guard(&_pinLatch);
            OSS_BIT_CLEAR(_ts.flags, LC_TAG_FAST_FLAG_IO_PENDING_WRITE);
            return ;
         }

      private:
         OSS_INLINE BOOLEAN isPinned()const
         {
            return 0 < _ts.usageCnt || 0 != _ts.flags;
         }

         OSS_INLINE BOOLEAN isNormalAndUnpinned() const
         {
            return LC_TAG_STATUS_NORMAL == _ts.status &&
                   !isPinned();
         }

         OSS_INLINE BOOLEAN isNoPending()const
         {
            return 0 == OSS_BIT_TEST(_ts.flags, LC_TAG_FAST_FLAG_IO_PENDING_WRITE);
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
         ossSpinLatch _pinLatch;
         ossSharedLatch _accessingLatch;

         /// readonly after firstInitTag
         GLOBAL_PAGE_ID _id;
         ossValuePtr _diskPagePtr = 0;
         //UINT32 _pageSize = 0;

         /// protected by pin latch.
         _TagState _ts;

         /// protected by rw latch.
         UINT32 _flags = 0;
         freeListPage _memPage;
         UINT64 _minDirtyLSN = DPS_INVALID_LSN_OFFSET;
         UINT64 _maxMemDirtyLSN = DPS_INVALID_LSN_OFFSET;

         /// bucket, protected by bucket latch and accessing latch
         LC_BUCKET_INNER_INDEX_ITERATOR _bucketItr;
         
         /// lru list, protected by lru latch and accessing latch(except _lruTouchCnt)
         UINT32 _lruTouchCnt = 0;
         UINT32 _lruFlags = 0;
         liteCachePageTag *_lruPre = NULL;
         liteCachePageTag *_lruNext = NULL;

         /// dirty list, protected by dirty list latch and accessing latch
         liteCachePageTag *_dirtyPre = NULL;
         liteCachePageTag *_dirtyNext = NULL;
   }; /// end of class liteCachePageTag

   OSS_INLINE BOOLEAN liteCachePageTag::incUsageCnt(BOOLEAN lock)
   {
      BOOLEAN r = FALSE;
      ossSpinLatch *latch = lock ? &_pinLatch : NULL;
      ossSpinGuard guard(latch);
      if (LC_TAG_STATUS_NORMAL == _ts.status)
      {
         ++_ts.usageCnt;
         r = TRUE;
      }
      return r;
   }

   OSS_INLINE void liteCachePageTag::decUsageCnt()
   {
      ossSpinGuard guard(&_pinLatch);
      --_ts.usageCnt;
      return;
   }

   /// under rw lock
   /// WARNING: validate other status under rw lock first.
   OSS_INLINE BOOLEAN liteCachePageTag::tryToSetRemoved()
   {
      BOOLEAN r = FALSE;
      ossSpinGuard guard(&_pinLatch);
      if (isNormalAndUnpinned())
      {
         _ts.status = LC_TAG_STATUS_TO_BE_REMOVED;
         r = TRUE;
      }
      return r;
   }

   OSS_INLINE BOOLEAN liteCachePageTag::tryToRollbackNewTag()
   {
      BOOLEAN r = FALSE;
      ossSpinGuard guard(&_pinLatch);
      --_ts.usageCnt;
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
