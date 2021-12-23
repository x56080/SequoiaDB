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
#include "ossSharedLatch.hpp"
#include "utilPooledObject.hpp"
#include "vessel/lcBucketInnerIndex.h"

namespace engine
{
namespace vessel
{
   constexpr UINT16 LC_TAG_STATUS_INVALID = 0;
   constexpr UINT16 LC_TAG_STATUS_NORMAL = 1;
   constexpr UINT16 LC_TAG_STATUS_DISCARDED = 2;

   constexpr UINT16 LC_TAG_FAST_FLAG_IO_PENDING_WRITE = 0x01;

   constexpr UINT32 LC_TAG_FLAG_IN_BUCKET = 0x01;
   constexpr UINT32 LC_TAG_FLAG_IN_DIRTY_LIST = 0x02;
   constexpr UINT32 LC_TAG_FLAG_IN_LRU_LIST = 0x04;

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
         OSS_INLINE const LC_BUCKET_INNER_INDEX_ITERATOR &getBucketIterator()const
         {
            return _bucketItr;
         }

         OSS_INLINE void setBucketIndex(const LC_BUCKET_INNER_INDEX_ITERATOR &itr)
         {
            _bucketItr = itr;
         }

         OSS_INLINE liteCachePageTag *getPreInBucket()const
         {
            return _preInBucket;
         }

         OSS_INLINE liteCachePageTag *getNextInBucket()const
         {
            return _nextInBucket;
         }

         OSS_INLINE void setPreInBucket(liteCachePageTag *pre)
         {
            _preInBucket = pre;
         }

         OSS_INLINE void setNextInBucket(liteCachePageTag *next)
         {
            _nextInBucket = next;
         }

         OSS_INLINE void setInBucket()
         {
            OSS_BIT_SET(_flags, LC_TAG_FLAG_IN_BUCKET);
         }

         OSS_INLINE BOOLEAN isInBucket()const
         {
            return 0 != OSS_BIT_TEST(_flags, LC_TAG_FLAG_IN_BUCKET);
         }

         OSS_INLINE void removeFromBucket()
         {
            _preInBucket = NULL;
            _nextInBucket = NULL;
            OSS_BIT_CLEAR(_flags, LC_TAG_FLAG_IN_BUCKET);
            _bucketItr = LC_BUCKET_INNER_INDEX_ITERATOR();
         }

      public:
         OSS_INLINE BOOLEAN isInLruList()const
         {
            /// Do not use lru pre/next ptr to check if in lru list.
            /// Ptrs may modified with out holding accessing latch by lru.
            /// Or, if this is only one tuple in list, both pre and next
            /// are null.
            return 0 != OSS_BIT_TEST(_flags, LC_TAG_FLAG_IN_LRU_LIST);
         }
         /// under lru lock and w lock
         OSS_INLINE void insertIntoLru(liteCachePageTag *pre,
                                       liteCachePageTag *next)
         {
            OSS_BIT_SET(_flags, LC_TAG_FLAG_IN_LRU_LIST);
            _lruPre = pre;
            _lruNext = next;
            _lruTouchCnt = 0;
            _lruTouchCntUpdatedTime = 0;
         }

         OSS_INLINE void removeFromLru()
         {
            _lruPre = NULL;
            _lruNext = NULL;
            _lruTouchCnt = 0;
            _lruTouchCntUpdatedTime = 0;
            OSS_BIT_CLEAR(_flags, LC_TAG_FLAG_IN_LRU_LIST);
         }

         OSS_INLINE void setLruTouchCnt(UINT32 n)
         {
            _lruTouchCnt = n;
         }

         OSS_INLINE void incLruTouchCnt()
         {
            ++_lruTouchCnt;
         }

         OSS_INLINE UINT32 fetchLruTouchCnt()
         {
            return ossAtomicFetch32(&_lruTouchCnt);
         }

         OSS_INLINE UINT32 getLruTouchCnt()const
         {
            return _lruTouchCnt;
         }

         OSS_INLINE UINT64 getLruTouchCntUpdatedTime()const
         {
            return _lruTouchCntUpdatedTime;
         }

         OSS_INLINE void setLruTouchCntUpdatedTime(UINT64 millis)
         {
            _lruTouchCntUpdatedTime = millis;
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
         OSS_INLINE BOOLEAN discardAndIncUsageCnt();
         /** get tag in bucket end **/

         /** remove tag from bucket begin **/
         /// pin latch is not necessary when performing precheck.
         /// it just affects performance and accuracy.
         OSS_INLINE BOOLEAN fastTestIfCanBeRecycled(BOOLEAN lock=TRUE)
         {
            BOOLEAN r = FALSE;
            ossSpinLatch *latch = lock ? &_pinLatch : NULL;
            ossSpinGuard guard(latch);
            r = isNotInAnyList() && !_isPinned();
            return r;
         }
         /** remove tag in bucket end **/

         /** evict tag in lru begin **/
         /// return true if "may" be evicted.
         OSS_INLINE BOOLEAN fastTestIfCanBeEvictedFromLru(BOOLEAN lock=TRUE)
         {
            ossSpinLatch *latch = lock ? &_pinLatch : NULL;
            ossSpinGuard guard(latch);
            return !_isPinned() && !isMemPageDirty();
         }
         
         /** evict tag in lru end **/

         OSS_INLINE BOOLEAN isPinned(BOOLEAN lock=TRUE)
         {
            ossSpinLatch *latch = lock ? &_pinLatch : NULL;
            ossSpinGuard guard(latch);
            return _isPinned();
         }

         OSS_INLINE BOOLEAN isPendingWrite(BOOLEAN lock=TRUE)
         {
            ossSpinLatch *latch = lock ? &_pinLatch : NULL;
            ossSpinGuard guard(latch);
            return _isPendingWrite();
         }

         /// under dirty list w lock
         OSS_INLINE BOOLEAN setPendingWrite()
         {
            BOOLEAN r = FALSE;
            ossSpinGuard guard(&_pinLatch);
            if (_ts.isNormal() && !_isPendingWrite())
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
         OSS_INLINE BOOLEAN _isPinned()const
         {
            return 0 < _ts.usageCnt || 0 != _ts.flags;
         }

         OSS_INLINE BOOLEAN _isPendingWrite()const
         {
            return 0 != OSS_BIT_TEST(_ts.flags, LC_TAG_FAST_FLAG_IO_PENDING_WRITE);
         }
         
      private:
         struct _TagState
         {
            OSS_INLINE _TagState(){}
            OSS_INLINE ~_TagState(){}
            _TagState &operator=(const _TagState &) = delete;

            OSS_INLINE BOOLEAN isNormal()const
            {
               return LC_TAG_STATUS_NORMAL == status;
            }

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
         liteCachePageTag *_preInBucket = NULL;
         liteCachePageTag *_nextInBucket = NULL;
         LC_BUCKET_INNER_INDEX_ITERATOR _bucketItr;
         
         /// lru list, protected by lru latch and accessing latch(except _lruTouchCnt)
         UINT32 _lruTouchCnt = 0;
         UINT64 _lruTouchCntUpdatedTime = 0;
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
      if (_ts.isNormal())
      {
         ++_ts.usageCnt;
         r = TRUE;
      }
      return r;
   }

   OSS_INLINE void liteCachePageTag::decUsageCnt()
   {
      ossSpinGuard guard(&_pinLatch);
      if (OSS_LIKELY(0 < _ts.usageCnt))
      {
         --_ts.usageCnt;
      }
      else
      {
         SDB_ASSERT(FALSE, "can not dec zero usage count");
      }
      return;
   }

   OSS_INLINE BOOLEAN liteCachePageTag::discardAndIncUsageCnt()
   {
      BOOLEAN r = FALSE;
      ossSpinGuard guard(&_pinLatch);
      SDB_ASSERT(0 == _ts.usageCnt, "must ensure no one accessing buffer first");
      /// WARNING: before discarding cache page, user should ensure that
      /// no one is accessing page except io workers.
      /// in fact, user should always hold exclusive sid latch first to
      /// avoid new requests coming.

      if (_ts.isNormal())
      {
         _ts.status = LC_TAG_STATUS_DISCARDED;
         ++_ts.usageCnt;
         r = TRUE;
      }

      return r;
   }
} /// end of namespace vessel
} /// end of namespace engine



#endif /// define VESSEL_LITE_CACHE_PAGE_TAG_H_
