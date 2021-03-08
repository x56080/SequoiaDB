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

   Source File Name = lcLRUList.cpp

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

#include "vessel/lcLRUList.h"
#include "vessel/lcExtentTag.h"
#include "ossUtil.hpp"
#include "vessel/lcExtentTagHolder.h"
#include "vessel/lcBuckets.h"
#include "vessel/lcFreeList.h"
#include "vessel/diskIOJob.h"
#include "pdTrace.hpp"
#include "ossLikely.hpp"
#include "vessel/requestContext.h"

namespace engine
{
namespace vessel
{
   lcLRUList::lcLRUList()
   :_buckets(NULL),
    _fl(NULL),
    _size(0),
    _coldSize(0),
    _head(NULL),
    _middle(NULL),
    _tail(NULL),
    _evictBegin(NULL)
   {

   }

   lcLRUList::~lcLRUList()
   {

   }

   INT32 lcLRUList::init(lcBuckets *buckets,
                          lcFreeList *fl,
                          const liteCacheOptions::lruOptions &options)
   {
      INT32 rc = SDB_OK;
      liteCacheOptions::lruOptions o = options;

      if (OSS_UNLIKELY(NULL == buckets ||
                       NULL == fl))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      if (o.lruColdPercent < 0.1 || o.lruColdPercent > 0.9)
      {
         o.lruColdPercent = 0.4;
      }
      else if (o.lruMinSplitSize < 512)
      {
         o.lruMinSplitSize = 512;
      }
      else if (o.lruScanDepth < 128)
      {
         o.lruScanDepth = 128;
      }
      else if (o.lruMaxScanPercent < 0.4)
      {
         o.lruMaxScanPercent = 0.4;
      }
      else if (o._lruColdMistakeTolerance > 512 * 0.1)
      {
         o._lruColdMistakeTolerance = 10;
      }

      _options = o;
      _buckets = buckets;
      _fl = fl;

   done:
      return rc;
   error:
      goto done;
   }

   INT32 lcLRUList::fini()
   {
      _size = 0;
      _coldSize = 0;
      _head = NULL;
      _middle = NULL;
      _tail = NULL;
      _evictBegin = NULL;
      return SDB_OK;
   }

   INT32 lcLRUList::insert(lcExtentTagHolder &holder)
   {
      INT32 rc = SDB_OK;
      BOOLEAN locked = FALSE;
      SDB_ASSERT(holder.valid(), "should be valid");
      SDB_ASSERT(LOCK_MODE_UNIQUE == holder.getLockMode(), "should holding lock");

      if (OSS_UNLIKELY(!holder.valid()))
      {
         PD_LOG(PDERROR, "insert an invalid tag");
         rc = SDB_INVALIDARG;
         goto error;
      }

      if (OSS_UNLIKELY(LOCK_MODE_UNIQUE != holder.getLockMode()))
      {
         PD_LOG(PDERROR, "holding wrong type lock");
         rc = SDB_VESSEL_FORBIDDEN_OP_WLT;
         goto error;
      }

      holder.tag()->setFlags(ET_FLAG_IN_LRU_LIST);
      _latch.get();
      locked = TRUE;

      if (splited())
      {
         insertToMiddle(holder.tag());
         tryToTuneRightMiddle();
      }
      else
      {
         insertToHead(holder.tag(), 1);
         if (_size == _options.lruMinSplitSize)
         {
            splitLRU();
         }
      }
      
   done:
      if (locked)
      {
         _latch.release();
      }
      return rc;
   error:
      goto done;
   }

   INT32 lcLRUList::tryToUpdate(lcExtentTagHolder &holder)
   {
      INT32 rc = SDB_OK;
      BOOLEAN locked = FALSE;
      lcExtentTag *tag = NULL;

      if (OSS_UNLIKELY(!holder.valid()))
      {
         PD_LOG(PDERROR, "try to update an invalid tag");
         rc = SDB_INVALIDARG;
         goto error;
      }

      if (OSS_UNLIKELY(LOCK_MODE_NONE == holder.getLockMode()))
      {
         PD_LOG(PDERROR, "holding wrong type lock");
         rc = SDB_VESSEL_FORBIDDEN_OP_WLT;
         goto error;
      }

      if (!_options.lruIncTouchCntWhenReadOnly &&
          LOCK_MODE_SHARED == holder.getLockMode())
      {
         goto done;
      }
      
      tag = holder.tag();
      _latch.get();
      locked = TRUE;

      tag->incLRUCnt();
      
   done:
      if (locked)
      {
         _latch.release();
      }
      return rc;
   error:
      goto done;
   }


   INT32 lcLRUList::evict(requestContext *context,
                          BOOLEAN scanUntilHitMax,
                          UINT32 chunkPageCount,
                          lcChunkPage *pageBuf)
   {
      INT32 rc = SDB_OK;
      UINT32 scanNum = 0;
      lcExtentTag *itr = NULL;
      BOOLEAN locked = FALSE;
      UINT32 totalMoved = 0;
      UINT32 totalSkipped = 0;
      UINT32 evictedChunkPageCount = 0;
      SDB_ASSERT(0 < chunkPageCount && chunkPageCount <= 2, "unnecessary check. but why do you need more than 2 pages");

      if (OSS_UNLIKELY(0 == chunkPageCount))
      {
         goto done;
      }
      else if (OSS_UNLIKELY(NULL == pageBuf))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      _latch.get();
      locked = TRUE;

      if (!scanUntilHitMax)
      {
         scanNum = std::max(chunkPageCount, _options.lruScanDepth);
      }
      else
      {
         scanNum = _size * _options.lruMaxScanPercent;
         scanNum = std::max(chunkPageCount, scanNum);
      }

      itr = NULL == _evictBegin ? _tail : _evictBegin;
      for (UINT32 i = 0; i < scanNum && NULL != itr; ++i)
      {
         lcExtentTag *tag = itr;
         UINT32 pageNumOfTag = tag->getPageNum();
         lcExtentTagHolder holder;
         UINT32 stillNeed = chunkPageCount - evictedChunkPageCount;

         if (splited())
         {
            /// do not update totalScaned here.
            if (_options.lruHotTouchCnt <= tag->getLRUCnt())
            {
               ++totalSkipped;
               ++totalMoved;
               moveToHead(tag);
               continue;
            }
         }

         if (!tag->lruEvictionPrecheck(NULL))
         {
            ++totalSkipped;
            continue;
         }

         if (!tryToEvictTagFromList(tag, stillNeed, pageBuf + evictedChunkPageCount))
         {
            ++totalSkipped;
            continue;
         }

         /// do not access tag again. it may be released.
         evictedChunkPageCount += pageNumOfTag;
         if (chunkPageCount <= evictedChunkPageCount)
         {
            break;
         }
      }

      if (NULL != itr && splited() && itr->isLRUCold())
      {
         _evictBegin = itr;
      }
      else
      {
         _evictBegin = NULL;
      }

      _latch.release();
      locked = FALSE;

      if (evictedChunkPageCount < chunkPageCount)
      {
         rc = SDB_VESSEL_LC_LRU_SCAN_HIT_MAX;
         goto error;
      }

   done:
      if (locked)
      {
         _latch.release();
      }
      return rc;
   error:
      if (0 < evictedChunkPageCount)
      {
         _fl->releasePages(evictedChunkPageCount, pageBuf);
      }
      goto done;
   }

   INT32 lcLRUList::setPendingWriteOrEvict(requestContext *context,
                                           UINT32 scanDepth,
                                           diskIOJob *job,
                                           UINT32 *involvedChunkPageCount)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(NULL != context && NULL != job, "can not be null");
      UINT32 scanNum = std::min(scanDepth, _options.lruScanDepth);
      lcExtentTag *itr = NULL;
      BOOLEAN locked = FALSE;
      UINT32 totalEvicted = 0;
      UINT32 totalPending = 0;
      UINT32 totalMoved = 0;
      UINT32 totalSkipped = 0;

      _latch.get();
      locked = TRUE;
 
      itr = _tail;
      for (UINT32 i = 0; i < scanNum && NULL != itr; ++i)
      {
         lcExtentTag *tag = itr;
         itr = itr->getLRUPre();
         UINT32 pageNumOfTag = tag->getPageNum();
         BOOLEAN dirtyAndNoPending = FALSE;
         BOOLEAN mayBeEvicted = FALSE;

         if (splited())
         {
            if (_options.lruHotTouchCnt <= tag->getLRUCnt())
            {
               moveToHead(tag);
               ++totalSkipped;
               ++totalMoved;
               continue;
            }
         }

         mayBeEvicted = tag->lruEvictionPrecheck(&dirtyAndNoPending);
         if (!mayBeEvicted && !dirtyAndNoPending)
         {
            ++totalSkipped;
            continue;
         }
         else if (mayBeEvicted)
         {
            if (tryToEvictTagFromList(tag, 0, NULL))
            {
               /// tag's memory may be released, do not access it again.
               totalEvicted += pageNumOfTag;
            }
            else
            {
               ++totalSkipped;
            }
         }
         else if (tag->setPendingWriteIfDirty())
         {
            rc = job->addPendingWriteTag(tag);
            if (OSS_UNLIKELY(SDB_OK != rc))
            {
               goto error;
            }
            totalPending += pageNumOfTag;
         }
         else
         {
            /// do nothing.
         }
      }

      if (NULL != itr && splited() && itr->isLRUCold())
      {
         _evictBegin = itr;
      }
      else
      {
         _evictBegin = NULL;
      }

      _latch.release();
      locked = FALSE;
      if (NULL != involvedChunkPageCount)
      {
         *involvedChunkPageCount = totalEvicted + totalPending;
      }
   done:
      if (locked)
      {
         _latch.release();
      }
      return rc;
   error:
      job->abort();
      goto done;
   }

   void lcLRUList::resetEvictBegin()
   {
      _latch.get();
      _evictBegin = NULL;
      _latch.release();
   }

   void lcLRUList::insertToMiddle(lcExtentTag *tag)
   {
      SDB_ASSERT(NULL != _middle, "not splited");
      ++_size;
      ++_coldSize;
      lcExtentTag *next = _middle->getLRUNext();
      next->setLRUPre(tag);
      _middle->setLRUNext(tag);
      tag->lruInserted(_middle, next, 1, ET_LRU_FLAG_COLD);

      return;
   }

   void lcLRUList::splitLRU()
   {
      SDB_ASSERT(NULL == _middle, "already splited");
      SDB_ASSERT(_size == _options.lruMinSplitSize, "lru size must be split size");
      UINT32 steps = _size * _options.lruColdPercent;
      lcExtentTag *tag = _tail;
      for (UINT32 i = 0; i < steps && NULL != tag; ++i)
      {
         tag->setLRUCold();
         tag = tag->getLRUPre();
      }
      _coldSize = steps;
      _middle = tag;

      for (;NULL != tag;)
      {
         tag->setLRUUncold();
         tag = tag->getLRUPre();
      }
      return;
   }

   void lcLRUList::cancelSplit()
   {
      _coldSize = 0;
      _middle = NULL;
   }

   void lcLRUList::tryToTuneRightMiddle()
   {
      SDB_ASSERT(splited(), "must splited");
      UINT32 tuneSize = 0;
      UINT32 maxColdSize = _size * _options.lruColdPercent + _options._lruColdMistakeTolerance;
      if (_coldSize <= maxColdSize)
      {
         goto done;
      }

      tuneSize = _coldSize - maxColdSize;
      for (UINT32 i = 0; i < tuneSize; ++i)
      {
         _middle = _middle->getLRUNext();
         _middle->setLRUUncold();
      }
      _coldSize -= tuneSize;
   done:
      return;
   }

   void lcLRUList::tryToTuneLeftMiddle()
   {
      SDB_ASSERT(splited(), "must splited");
      UINT32 tuneSize = 0;
      UINT32 minColdSize = _size * _options.lruColdPercent - _options._lruColdMistakeTolerance;
      if (minColdSize <= _coldSize)
      {
         goto done;
      }
      
      tuneSize = minColdSize - _coldSize;
      for (UINT32 i = 0; i < tuneSize; ++i)
      {
         _middle->setLRUCold();
         _middle = _middle->getLRUPre();
      }
      _coldSize += tuneSize;
   done:
      return;
   }

   void lcLRUList::moveToHead(lcExtentTag *tag)
   {
      SDB_ASSERT(splited(), "impossible");

      BOOLEAN cold = tag->isLRUCold();

      if (_middle == tag)
      {
         /// middle is not cold, do not --coldsize
         _middle = _middle->getLRUPre();
      }

      removeFromList(tag);
      insertToHead(tag, 0);

      if (cold)
      {
         --_coldSize;
         tryToTuneLeftMiddle();
      }

      return;
   }

   void lcLRUList::removeFromList(lcExtentTag *tag)
   {
      lcExtentTag *pre = tag->getLRUPre();
      lcExtentTag *next = tag->getLRUNext();
      --_size;
      
      if (NULL != pre)
      {
         pre->setLRUNext(next);
      }
      else
      {
         _head = next;
      }

      if (NULL != next)
      {
         next->setLRUPre(pre);
      }
      else
      {
         _tail = pre;
      }

      tag->setNoChunkPages();
      tag->lruRemoved();
      tag->clearFlags(ET_FLAG_IN_LRU_LIST);
      return;
   }

   void lcLRUList::insertToHead(lcExtentTag *tag, UINT16 cnt)
   {
      ++_size;
      if (OSS_LIKELY(NULL != _head))
      {
         lcExtentTag *oldHead = _head;
         _head = tag;
         oldHead->setLRUPre(tag);
         tag->lruInserted(NULL, oldHead, cnt, ET_LRU_FLAG_NONE);
      }
      else
      {
         _head = tag;
         _tail = tag;
         tag->lruInserted(NULL, NULL, cnt, ET_LRU_FLAG_NONE);
      }
      return;
   }


   BOOLEAN lcLRUList::tryToEvictTagFromList(lcExtentTag *tag,
                                            UINT32 bufCount,
                                            lcChunkPage *pages)
   {
      BOOLEAN r = FALSE;
      SDB_ASSERT(NULL != tag, "can not be null");
      lcChunkPage *page = NULL;
      lcExtentTagHolder holder;
      UINT32 evitedPageCount = 0;
      UINT32 totalCount = 0;
      BOOLEAN removeFromBucket = FALSE;
      
      holder.reset(tag);
      if (!holder.tryLockUnique())
      {
         goto done;
      }

      if (OSS_UNLIKELY(tag->noChunkPages()))
      {
         SDB_ASSERT(FALSE, "impossible");
         PD_LOG(PDERROR, "tag with no chunk pages in lru");
         goto done;
      }

      if (tag->isDirty())
      {
         goto done;
      }

      totalCount = tag->getPageNum();

      if (!tag->inDirtyList())
      {
         if (!tag->setEvictedFromLRUAndRemoved())
         {
            goto done;
         }
         removeFromBucket = TRUE;
      }
      else if (!tag->lruEvictionPrecheck(NULL))
      {
         goto done;
      }

      ///from here, if tag is not marked as removed:
      ///we can be sure that the page will not become a dirty page,
      ///coz we are holding w lock.
      ///but it is possible that other users have increased the reference count and waiting for lock.
      ///it does not matter, we will not delete tag unless removeFromBucket is true.
      ///others users may reinsert tag into lru by themselves. 
      
      page = tag->pages();
      for (evitedPageCount = 0; evitedPageCount < bufCount && evitedPageCount < totalCount; ++evitedPageCount)
      {
         pages[evitedPageCount] = *(page + evitedPageCount);
      }

      if (evitedPageCount < totalCount)
      {
         _fl->releasePages(totalCount - evitedPageCount, page + evitedPageCount);
      }

      if (splited())
      {
         removeTagAndTuneMiddle(tag);
      }
      else
      {
         removeFromList(tag);
      }

      holder.unlockUnique();

      if (removeFromBucket)
      {
         _buckets->releaseRemovedTag(tag);
         /// do not access tag again
      }
      r = TRUE;
   done:
      holder.unlockUnique();
      return r;
   }

   void lcLRUList::removeTagAndTuneMiddle(lcExtentTag *tag)
   {
      SDB_ASSERT(splited(), "must be splited");
      BOOLEAN cold = tag->isLRUCold(); 
      if (_size < _options.lruMinSplitSize)
      {
         if (_middle == tag)
         {
            _middle = _middle->getLRUPre();
         }
         removeFromList(tag);

         if (cold)
         {
            --_coldSize;
            tryToTuneLeftMiddle();
         }
         else
         {
            tryToTuneRightMiddle();
         }
      }
      else
      {
         removeFromList(tag);
         cancelSplit();
      }
   }

} /// end of namespace vessel
} /// end of namespace engine