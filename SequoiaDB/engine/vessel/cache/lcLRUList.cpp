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
#include "ossUtil.hpp"
#include "vessel/lcPageTagHolder.h"
#include "vessel/lcBuckets.h"
#include "vessel/lcFreeList.h"
#include "vessel/diskIOJob.h"
#include "pdTrace.hpp"
#include "ossLikely.hpp"
#include "vessel/requestContext.h"
#include "ossMemPool.hpp"

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

   INT32 lcLRUList::insert(lcPageTagHolder &holder, const freeListPage &page)
   {
      INT32 rc = SDB_OK;
      BOOLEAN locked = FALSE;
      liteCachePageTag *tag = NULL;
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

      if (OSS_UNLIKELY(holder.tag()->isInLruList()))
      {
         PD_LOG(PDERROR, "can not insert tag which already in lru");
         rc = SDB_INVALIDARG;
         goto error;
      }

      if (OSS_UNLIKELY(holder.tag()->hasMemPage()))
      {
         PD_LOG(PDERROR, "can not insert tag with mem page to lru");
         rc = SDB_INVALIDARG;
         goto error;
      }

      if (OSS_UNLIKELY(!page.valid()))
      {
         PD_LOG(PDERROR, "can not insert with invalid page");
         rc = SDB_INVALIDARG;
         goto error;
      }

      tag = holder.tag();
      tag->setFastFlags(LC_TAG_FAST_FLAG_IN_LRU_LIST);
      tag->setMemPage(page);
      _latch.get();
      locked = TRUE;

      if (splited())
      {
         insertToMiddle(tag);
         tag->incLruTouchCnt();
         tryToTuneRightMiddle();
      }
      else
      {
         insertToHead(tag);
         tag->incLruTouchCnt();
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

   INT32 lcLRUList::tryToUpdate(lcPageTagHolder &holder)
   {
      INT32 rc = SDB_OK;
      BOOLEAN locked = FALSE;
      liteCachePageTag *tag = NULL;

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

      tag = holder.tag();
      SDB_ASSERT(tag->isInLruList(), "must be in lru");

      if (!_options.lruIncTouchCntWhenReadOnly &&
          LOCK_MODE_SHARED == holder.getLockMode())
      {
         goto done;
      }

      if (LOCK_MODE_UNIQUE == holder.getLockMode())
      {
         tag->incLruTouchCnt();
      }
      else
      {
         tag->incLruTouchCntWithCAS();
      }      
      
   done:
      return rc;
   error:
      goto done;
   }


   INT32 lcLRUList::evict(requestContext *context,
                          BOOLEAN scanUntilHitMax,
                          freeListPage &page)
   {
      INT32 rc = SDB_OK;
      UINT32 scanNum = 0;
      liteCachePageTag *itr = NULL;
      BOOLEAN locked = FALSE;
      UINT32 totalMoved = 0;
      UINT32 totalSkipped = 0;

      page.reset();
      _latch.get();
      locked = TRUE;

      if (!scanUntilHitMax)
      {
         scanNum = _options.lruScanDepth;
      }
      else
      {
         scanNum = _size * _options.lruMaxScanPercent;
      }

      itr = NULL == _evictBegin ? _tail : _evictBegin;
      for (UINT32 i = 0; i < scanNum && NULL != itr; ++i)
      {
         liteCachePageTag *tag = itr;
         itr = itr->getLruPre();
         lcPageTagHolder holder;

         if (splited())
         {
            /// do not update totalScaned here.
            if (_options.lruHotTouchCnt <= tag->getLruTouchCnt())
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

         if (!tryToEvictTagFromList(tag, page))
         {
            ++totalSkipped;
            continue;
         }

         /// do not access tag again. it may be released.
         break;
      }

      if (NULL != itr && splited() && itr->isLruCold())
      {
         _evictBegin = itr;
      }
      else
      {
         _evictBegin = NULL;
      }

      _latch.release();
      locked = FALSE;

      if (!page.valid())
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
      goto done;
   }

   INT32 lcLRUList::setPendingWriteOrEvict(requestContext *context,
                                           UINT32 scanDepth,
                                           diskIOJob *job,
                                           UINT32 *involvedMemPageCount)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(NULL != context && NULL != job, "can not be null");
      UINT32 scanNum = std::min(scanDepth, _options.lruScanDepth);
      liteCachePageTag *itr = NULL;
      BOOLEAN locked = FALSE;
      UINT32 totalEvicted = 0;
      UINT32 totalPending = 0;
      UINT32 totalMoved = 0;
      UINT32 totalSkipped = 0;
      ossPoolVector<freeListPage> pages;

      _latch.get();
      locked = TRUE;
 
      itr = _tail;
      for (UINT32 i = 0; i < scanNum && NULL != itr; ++i)
      {
         liteCachePageTag *tag = itr;
         itr = itr->getLruPre();
         BOOLEAN dirtyAndNoPending = FALSE;
         BOOLEAN mayBeEvicted = FALSE;
         freeListPage page;

         if (splited())
         {
            if (_options.lruHotTouchCnt <= tag->getLruTouchCnt())
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
            if (tryToEvictTagFromList(tag, page))
            {
               /// tag's memory may be released, do not access it again.
               pages.push_back(page);
               ++totalEvicted;
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
            ++totalPending;
         }
         else
         {
            /// do nothing.
         }
      }

      if (NULL != itr && splited() && itr->isLruCold())
      {
         _evictBegin = itr;
      }
      else
      {
         _evictBegin = NULL;
      }

      _latch.release();
      locked = FALSE;

      if (!pages.empty())
      {
         _fl->releasePages(pages.size(), pages.data());
      }

      if (NULL != involvedMemPageCount)
      {
         *involvedMemPageCount = totalEvicted + totalPending;
      }
   done:
      if (locked)
      {
         _latch.release();
      }
      return rc;
   error:
      job->abortUndispatchedTasks();
      goto done;
   }

   void lcLRUList::resetEvictBegin()
   {
      _latch.get();
      _evictBegin = NULL;
      _latch.release();
   }

   void lcLRUList::insertToMiddle(liteCachePageTag *tag)
   {
      SDB_ASSERT(NULL != _middle, "not splited");
      ++_size;
      ++_coldSize;
      liteCachePageTag *next = _middle->getLruNext();
      next->setLruPre(tag);
      _middle->setLruNext(tag);
      tag->insertIntoLru(_middle, next, TRUE);
      return;
   }

   void lcLRUList::splitLRU()
   {
      SDB_ASSERT(NULL == _middle, "already splited");
      SDB_ASSERT(_size == _options.lruMinSplitSize, "lru size must be split size");
      UINT32 steps = _size * _options.lruColdPercent;
      liteCachePageTag *tag = _tail;
      for (UINT32 i = 0; i < steps && NULL != tag; ++i)
      {
         tag->setLruCold();
         tag = tag->getLruPre();
      }
      _coldSize = steps;
      _middle = tag;

      for (;NULL != tag;)
      {
         tag->setLruUncold();
         tag = tag->getLruPre();
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
         _middle = _middle->getLruNext();
         _middle->setLruUncold();
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
         _middle->setLruCold();
         _middle = _middle->getLruPre();
      }
      _coldSize += tuneSize;
   done:
      return;
   }

   void lcLRUList::moveToHead(liteCachePageTag *tag)
   {
      SDB_ASSERT(splited(), "impossible");

      BOOLEAN cold = tag->isLruCold();

      if (_middle == tag)
      {
         /// middle is not cold, do not --coldsize
         _middle = _middle->getLruPre();
      }

      removeFromList(tag);
      insertToHead(tag);

      if (cold)
      {
         --_coldSize;
         tryToTuneLeftMiddle();
      }

      return;
   }

   void lcLRUList::removeFromList(liteCachePageTag *tag)
   {
      liteCachePageTag *pre = tag->getLruPre();
      liteCachePageTag *next = tag->getLruNext();
      --_size;
      
      if (NULL != pre)
      {
         pre->setLruNext(next);
      }
      else
      {
         _head = next;
      }

      if (NULL != next)
      {
         next->setLruPre(pre);
      }
      else
      {
         _tail = pre;
      }

      tag->removeFromLru();
      return;
   }

   void lcLRUList::insertToHead(liteCachePageTag *tag)
   {
      SDB_ASSERT(NULL != tag, "can not be null");
      ++_size;
      if (OSS_LIKELY(NULL != _head))
      {
         liteCachePageTag *oldHead = _head;
         _head = tag;
         oldHead->setLruPre(tag);
         tag->insertIntoLru(NULL, oldHead, FALSE);
      }
      else
      {
         _head = tag;
         _tail = tag;
         tag->insertIntoLru(NULL, NULL, FALSE);
      }
      return;
   }


   BOOLEAN lcLRUList::tryToEvictTagFromList(liteCachePageTag *tag,
                                            freeListPage &page)
   {
      BOOLEAN r = FALSE;
      SDB_ASSERT(NULL != tag, "can not be null");
      lcPageTagHolder holder;
      BOOLEAN removeFromBucket = FALSE;
      
      holder.reset(tag);
      if (!holder.tryLockUnique())
      {
         goto done;
      }

      if (tag->isDirty())
      {
         goto done;
      }

      if (!tag->isInDirtyList())
      {
         if (!tag->setEvictedFromLRUAndRemoving())
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
      if (splited())
      {
         removeTagAndTuneMiddle(tag);
      }
      else
      {
         removeFromList(tag);
      }

      page = tag->getMemPage();
      SDB_ASSERT(page.valid(), "must be valid");
      tag->releaseMemPage();
      tag->clearFastFlags(LC_TAG_FAST_FLAG_IN_LRU_LIST);
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

   void lcLRUList::removeTagAndTuneMiddle(liteCachePageTag *tag)
   {
      SDB_ASSERT(splited(), "must be splited");
      BOOLEAN cold = tag->isLruCold(); 
      if (_options.lruMinSplitSize < _size)
      {
         if (_middle == tag)
         {
            _middle = _middle->getLruPre();
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