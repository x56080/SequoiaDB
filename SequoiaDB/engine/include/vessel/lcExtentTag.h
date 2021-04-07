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

   Source File Name = lcExtentTag.h

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

#ifndef VESSEL_LC_EXTENT_TAG_H_
#define VESSEL_LC_EXTENT_TAG_H_

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
   const UINT16 ET_STATE_INVALID = 0;
   const UINT16 ET_STATE_NORMAL = 1;
   const UINT16 ET_STATE_TO_BE_REMOVED = 2;

   const UINT16 ET_FLAG_NONE = 0;
   const UINT16 ET_FLAG_IO_PENDING_WRITE = 1;
   //const UINT16 ET_FLAG_IO_PENDING_READ = 2;
   const UINT16 ET_FLAG_DIRTY = 4;
   const UINT16 ET_FLAG_IN_DIRTY_LIST = 8;
   const UINT16 ET_FLAG_IN_LRU_LIST = 16;

   const UINT16 ET_LRU_FLAG_NONE = 0;
   const UINT16 ET_LRU_FLAG_COLD = 1;

   const UINT32 TAG_FLAG_NONE = 0;
   const UINT32 TAG_FLAG_DIRTY = 0x01;
   const UINT32 TAG_FLAG_IN_DIRTY_LIST = 0x02;
   const UINT32 TAG_FLAG_IN_LRU_LIST = 0x04;

   class lcExtentTag : public _utilPooledObject
   {
      public:
         lcExtentTag();
         ~lcExtentTag();

         lcExtentTag(const lcExtentTag &) = delete;
         lcExtentTag &operator=(const lcExtentTag &) = delete;
      public:
         void reset();

         OSS_INLINE SHARED_MUTEX &rwMutex()
         {
            return _rwMutex;
         }

         /// do not get spin latch unless you are very clear
         /// how it works 
         OSS_INLINE ossSpinLatch &getSpinMutex()
         {
            return _spinLatch;
         }

         OSS_INLINE void setStateNormal()
         {
            _spinLatch.lock();
            _ts.state = ET_STATE_NORMAL;
            _spinLatch.unlock();
         }
      
         OSS_INLINE const GLOBAL_PAGE_ID &id()const
         {
            return _id;
         }

         OSS_INLINE BOOLEAN testFlags(UINT16 flags)
         {
            _spinLatch.lock();
            BOOLEAN r = OSS_BIT_TEST(_ts.flags, flags);
            _spinLatch.unlock();
            return r;
         }

         OSS_INLINE void setFlags(UINT16 flags)
         {
            _spinLatch.lock();
            OSS_BIT_SET(_ts.flags, flags);
            _spinLatch.unlock();
            return;
         }

         OSS_INLINE void clearFlags(UINT16 flags)
         {
            _spinLatch.lock();
            OSS_BIT_CLEAR(_ts.flags, flags);
            _spinLatch.unlock();
            return;
         }

         OSS_INLINE UINT16 flags()
         {
            UINT16 flags = 0;
            _spinLatch.lock();
            flags = _ts.flags;
            _spinLatch.unlock();
            return flags;
         }

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
         
         OSS_INLINE const freeListPage &getMemPage()const
         {
            return _memPage;
         }

         OSS_INLINE freeListPage &getMemPage()
         {
            return _memPage;
         }

         OSS_INLINE UINT32 getPageSize()const
         {
            return _pageSize;
         }

         OSS_INLINE BOOLEAN inDirtyList()const
         {
            return OSS_BIT_TEST(_flags, TAG_FLAG_IN_DIRTY_LIST);
         }

         OSS_INLINE BOOLEAN isDirty()const
         {
            return OSS_BIT_TEST(_flags, TAG_FLAG_DIRTY);
         }

         OSS_INLINE BOOLEAN inLruList()const
         {
            return OSS_BIT_TEST(_flags, TAG_FLAG_IN_LRU_LIST);
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

         /// under lru lock and w lock
         OSS_INLINE void lruInserted(lcExtentTag *pre,
                                     lcExtentTag *next,
                                     UINT16 touchCnt,
                                     UINT16 flags)
         {
            _lruPre = pre;
            _lruNext = next;
            _lruCnt.init(touchCnt);
            _lruFlags = flags;
            OSS_BIT_SET(_flags, TAG_FLAG_IN_LRU_LIST);
         }

         OSS_INLINE void lruRemoved()
         {
            _lruPre = NULL;
            _lruNext = NULL;
            _lruCnt.init(0);
            _lruFlags = ET_LRU_FLAG_NONE;
            OSS_BIT_CLEAR(_flags, TAG_FLAG_IN_LRU_LIST);
         }

         OSS_INLINE void setLRUCold()
         {
            OSS_BIT_SET(_lruFlags, ET_LRU_FLAG_COLD);
         }

         OSS_INLINE void setLRUUncold()
         {
            OSS_BIT_CLEAR(_lruFlags, ET_LRU_FLAG_COLD);
         }

         OSS_INLINE BOOLEAN isLRUCold()const
         {
            return OSS_BIT_TEST(_lruFlags, ET_LRU_FLAG_COLD);
         }

         OSS_INLINE void incLRUCnt()
         {
            _lruCnt.inc();
         }

         OSS_INLINE void clearLRUCnt()
         {
            _lruCnt.init(0);
         }

         OSS_INLINE UINT32 getLRUCnt()
         {
            return _lruCnt.fetch();
         }

         OSS_INLINE void setLRUPre(lcExtentTag *pre)
         {
            _lruPre = pre;
         }

         OSS_INLINE lcExtentTag *getLRUPre()
         {
            return _lruPre;
         }

         OSS_INLINE void setLRUNext(lcExtentTag *next)
         {
            _lruNext = next;
         }

         OSS_INLINE lcExtentTag *getLRUNext()
         {
            return _lruNext;
         }

         OSS_INLINE void setDirtyListPre(lcExtentTag *tag)
         {
            _dirtyPre = tag;
         }

         OSS_INLINE void setDirtyListNext(lcExtentTag *tag)
         {
            _dirtyNext = tag;
         }

         /// under dirty list lock and w lock
         OSS_INLINE void insertIntoDirtyList(lcExtentTag *pre,
                                             lcExtentTag *next)
         {
            _dirtyPre = pre;
            _dirtyNext = next;
            OSS_BIT_SET(_flags, TAG_FLAG_IN_DIRTY_LIST|TAG_FLAG_DIRTY);
         }

         OSS_INLINE void removeFromDirtyList()
         {
            _dirtyPre = NULL;
            _dirtyNext = NULL;
            OSS_BIT_CLEAR(_flags, TAG_FLAG_IN_DIRTY_LIST);
         }

         OSS_INLINE lcExtentTag *getDirtyListPre()
         {
            return _dirtyPre;
         }

         OSS_INLINE lcExtentTag *getDirtyListNext()
         {
            return _dirtyNext;
         }


      public:
         /** get tag in bucket begin **/
         BOOLEAN incUsageCnt();
         void decUsageCnt();
         /** get tag in bucket end **/

         /** recycle tag in bucket begin **/
         OSS_INLINE BOOLEAN recyclePreCheck()
         {
            BOOLEAN r = FALSE;
            _spinLatch.lock();
            r = readyToBeRecycled();
            _spinLatch.unlock();
            return r;
         }

         /// under w lock
         OSS_INLINE BOOLEAN tryToSetRecycled()
         {
            BOOLEAN r = FALSE;
            _spinLatch.lock();
            if (readyToBeRecycled())
            {
               _ts.state = ET_STATE_TO_BE_REMOVED;
               r = TRUE;
            }
            _spinLatch.unlock();
            return r;
         }

         /// remove tag from bucket after evicting.
         OSS_INLINE BOOLEAN toBeRemoved()
         {
            _spinLatch.lock();
            BOOLEAN r = (ET_STATE_TO_BE_REMOVED == _ts.state);
            _spinLatch.unlock();
            return r;
         }
         /** recycle tag in bucket end **/

         /** evict tag in lru begin **/
         /// unkder no lock
         /// return true if can be evicted(or deprived).
         OSS_INLINE BOOLEAN lruEvictionPrecheck(BOOLEAN *dirtyAndNoPending)
         {
            BOOLEAN r = FALSE;
            _spinLatch.lock();
            r = readyToBeEvictedFromLRU();
            if (r)
            {
               goto done;
            }

            if (NULL != dirtyAndNoPending)
            {
               *dirtyAndNoPending = isDirtyAndNoPending();
            }
            
         done:
            _spinLatch.unlock();
            return r;
         }

         /// under w lock
         OSS_INLINE BOOLEAN setEvictedFromLRUAndRemoved()
         {
            BOOLEAN r = FALSE;
            _spinLatch.lock();
            if (readyToBeEvictedFromLRU() &&
                !OSS_BIT_TEST(_ts.flags, ET_FLAG_IN_DIRTY_LIST))
            {
               _ts.state = ET_STATE_TO_BE_REMOVED;
               _ts.flags = ET_FLAG_NONE;
               r = TRUE;
            }
            _spinLatch.unlock();
            return r;
         }

         OSS_INLINE BOOLEAN setPendingWriteIfDirty()
         {
            BOOLEAN r = FALSE;
            _spinLatch.lock();
            if (isDirtyAndNoPending())
            {
               OSS_BIT_SET(_ts.flags, ET_FLAG_IO_PENDING_WRITE);
               r = TRUE;
            }
            _spinLatch.unlock();
            return r;
         }
         /** evict tag in lru end **/

         OSS_INLINE BOOLEAN pendingWrite()
         {
            _spinLatch.lock();
            BOOLEAN r = OSS_BIT_TEST(_ts.flags, ET_FLAG_IO_PENDING_WRITE);
            _spinLatch.unlock();
            return r;
         }

         /// under dirty list w lock
         OSS_INLINE BOOLEAN setPendingWrite()
         {
            BOOLEAN r = FALSE;
            _spinLatch.lock();
            if (isNoPending())
            {
               OSS_BIT_SET(_ts.flags, ET_FLAG_IO_PENDING_WRITE);
               r = TRUE;
            }
            _spinLatch.unlock();
            return r;
         }

         OSS_INLINE void setUnPendingWrite()
         {
            _spinLatch.lock();
            if (ET_STATE_NORMAL == _ts.state &&
                OSS_BIT_TEST(_ts.flags, ET_FLAG_IO_PENDING_WRITE))
            {
               OSS_BIT_CLEAR(_ts.flags, ET_FLAG_IO_PENDING_WRITE);
            }
            _spinLatch.unlock();
            return ;
         }

      public:
         /// call this func before insert into bucket.
         /// no lock
         OSS_INLINE void firstInit(const GLOBAL_PAGE_ID &id,
                                   UINT32 pageSize)
         {
            _id = id;
            _pageSize = pageSize;
            return;
         }

         /// w lock
         OSS_INLINE void setMemPage(const freeListPage &page)
         {
            _memPage = page;
         }

         /// w lock
         OSS_INLINE void releaseMemPage()
         {
            _memPage.reset();
         }

         /// w lock
         INT32 copyDataToDisk();

      private:
         OSS_INLINE BOOLEAN readyToBeRecycled() const
         {
            return ET_STATE_NORMAL == _ts.state &&
                   0 == _ts.usageCnt &&
                   ET_FLAG_NONE == _ts.flags;
         }

         OSS_INLINE BOOLEAN isDirtyAndNoPending()const
         {
            return ET_STATE_NORMAL == _ts.state &&
                   OSS_BIT_TEST(_ts.flags, ET_FLAG_DIRTY) &&
                   OSS_BIT_TEST(_ts.flags, ET_FLAG_IN_DIRTY_LIST) &&
                   !OSS_BIT_TEST(_ts.flags, ET_FLAG_IO_PENDING_WRITE);
         }

         OSS_INLINE BOOLEAN isNoPending()const
         {
            return ET_STATE_NORMAL == _ts.state &&
                   OSS_BIT_TEST(_ts.flags, ET_FLAG_IN_DIRTY_LIST) &&
                   !OSS_BIT_TEST(_ts.flags, ET_FLAG_IO_PENDING_WRITE);
         }

         OSS_INLINE BOOLEAN readyToBeEvictedFromLRU()const
         {
            return 0 == _ts.usageCnt &&
                   ET_STATE_NORMAL == _ts.state &&
                   OSS_BIT_TEST(_ts.flags, ET_FLAG_IN_LRU_LIST) &&
                   !OSS_BIT_TEST(_ts.flags, ET_FLAG_IO_PENDING_WRITE) &&
                   !OSS_BIT_TEST(_ts.flags, ET_FLAG_DIRTY);
         }

         OSS_INLINE BOOLEAN checkTagStateFlags(UINT16 flags,
                                               UINT16 notFlags,
                                               UINT16 state = ET_STATE_NORMAL,
                                               UINT32 usageCnt = 0)const
         {
            return usageCnt == _ts.usageCnt &&
                   state == _ts.state &&
                   (0 == flags || OSS_BIT_TEST(_ts.flags, flags)) &&
                   (0 == notFlags || !OSS_BIT_TEST(_ts.flags, notFlags));
         }
         
      private:
         struct TagState
         {
            TagState()
            :state(ET_STATE_INVALID),
             flags(ET_FLAG_NONE),
             usageCnt(0)
            {}

            UINT16 state;
            UINT16 flags;
            UINT32 usageCnt;
         };

      private:
         ossSpinLatch _spinLatch;
         SHARED_MUTEX _rwMutex;

         /// readonly after firstInitTag
         GLOBAL_PAGE_ID _id;
         UINT32 _pageSize;

         /// protected by spin latch.
         ossValuePtr _diskPagePtr;
         TagState _ts;

         /// protected by rw latch.
         UINT32 _flags;
         UINT64 _minLSN;
         UINT64 _maxLSN;
         freeListPage _memPage;
         
         /// lru list, protected by lru lock
         UINT32 _lruFlags;
         ossAtomic32 _lruCnt;
         lcExtentTag *_lruPre;
         lcExtentTag *_lruNext;
         /// dirty list, protected by dirty list lock
         lcExtentTag *_dirtyPre;
         lcExtentTag *_dirtyNext;

   }; /// end of class lcExtentTag

} /// end of namespace vessel
} /// end of namespace engine


#endif /// define VESSEL_EXTENT_TAG_H_
