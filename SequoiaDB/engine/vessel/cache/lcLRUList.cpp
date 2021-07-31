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
#include "ossLatchGuard.hpp"

namespace engine
{
namespace vessel
{
   lcLRUList::lcLRUList()
   {

   }

   lcLRUList::~lcLRUList()
   {

   }

   INT32 lcLRUList::init(lcFreeList *fl,
                          const liteCacheOptions::lruOptions &options)
   {
      INT32 rc = SDB_OK;

      if (OSS_UNLIKELY(NULL == fl))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      _options = options;
      _fl = fl;

   done:
      return rc;
   error:
      goto done;
   }

   void lcLRUList::fini()
   {
      _fl = NULL;
      _size = 0;
      _coldSize = 0;
      _head = NULL;
      _middle = NULL;
      _tail = NULL;
      _evictBegin = NULL;
      return;
   }

   INT32 lcLRUList::insert(lcPageTagHolder &holder,
                           UINT32 beginTouchCount)
   {
      INT32 rc = SDB_OK;
      liteCachePageTag *tag = NULL;
      ossXLatchGuard guard(&_latch, FALSE);

      if (OSS_UNLIKELY(!holder.valid()))
      {
         PD_LOG(PDERROR, "insert an invalid tag");
         SDB_ASSERT(FALSE, "impossible");
         rc = SDB_INVALIDARG;
         goto error;
      }
      else if (OSS_UNLIKELY(OSS_SHARED_LATCH_MODE_EXCLUSIVE != holder.getLockMode()))
      {
         PD_LOG(PDERROR, "holding wrong type lock");
         SDB_ASSERT(FALSE, "impossible");
         rc = SDB_VESSEL_FORBIDDEN_OP_WLT;
         goto error;
      }
      else if (OSS_UNLIKELY(holder.tag()->isInLruList()))
      {
         PD_LOG(PDERROR, "can not insert tag which already in lru");
         SDB_ASSERT(FALSE, "impossible");
         rc = SDB_INVALIDARG;
         goto error;
      }
      else if (OSS_UNLIKELY(!holder.tag()->hasMemPage()))
      {
         PD_LOG(PDERROR, "can not insert tag with no mem page to lru");
         SDB_ASSERT(FALSE, "impossible");
         rc = SDB_INVALIDARG;
         goto error;
      }

      tag = holder.tag();
      guard.lock();

      if (splited())
      {
         insertToMiddle(tag);
         tag->setLruTouchCnt(beginTouchCount);
         tryToTuneRightMiddle();
      }
      else
      {
         insertToHead(tag);
         tag->setLruTouchCnt(beginTouchCount);
         if (_size == _options.lruMinSplitSize)
         {
            splitLRU();
         }
      }
      
   done:
      return rc;
   error:
      goto done;
   }

   INT32 lcLRUList::tryToUpdate(lcPageTagHolder &holder)
   {
      INT32 rc = SDB_OK;
      liteCachePageTag *tag = NULL;

      if (OSS_UNLIKELY(!holder.valid()))
      {
         PD_LOG(PDERROR, "try to update an invalid tag");
         SDB_ASSERT(FALSE, "impossible");
         rc = SDB_INVALIDARG;
         goto error;
      }
      else if (OSS_UNLIKELY(OSS_SHARED_LATCH_MODE_NONE == holder.getLockMode()))
      {
         PD_LOG(PDERROR, "holding wrong type lock");
         SDB_ASSERT(FALSE, "impossible");
         rc = SDB_VESSEL_FORBIDDEN_OP_WLT;
         goto error;
      }
      else if (!holder.tag()->isInLruList())
      {
         SDB_ASSERT(FALSE, "must be in lru");
         rc = SDB_INVALIDARG;
         goto error;
      }

      tag = holder.tag();
      if (OSS_SHARED_LATCH_MODE_EXCLUSIVE == holder.getLockMode())
      {
         tag->incLruTouchCnt();
      }
      else
      {
         tag->incLruTouchCntWithAtom();
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
      UINT32 totalMoved = 0;
      UINT32 totalSkipped = 0;
      page.reset();
      ossXLatchGuard guard(&_latch, FALSE);

      if (!scanUntilHitMax)
      {
         scanNum = _options.lruScanDepth;
      }
      else
      {
         scanNum = _size * _options.lruMaxScanPercent;
      }

      guard.lock();
      itr = (NULL == _evictBegin) ? _tail : _evictBegin;
      for (UINT32 i = 0; i < scanNum && NULL != itr; ++i)
      {
         liteCachePageTag *tag = itr;
         itr = itr->getLruPre();
         lcPageTagHolder holder;

         if (splited() && (_options.lruHotTouchCnt <= tag->getLruTouchCnt()))
         {
            ++totalSkipped;
            ++totalMoved;
            moveToHead(tag);
            continue;
         }

         if (!tag->fastTestIfCanBeEvictedFromLru(FALSE))
         {
            ++totalSkipped;
            continue;
         }

         if (!tryToEvictTagFromList(tag, page))
         {
            ++totalSkipped;
            continue;
         }
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
   done:
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
      SDB_ASSERT(NULL != job, "can not be null");
      UINT32 scanNum = std::min(scanDepth, _options.lruScanDepth);
      liteCachePageTag *itr = NULL;
      UINT32 totalEvicted = 0;
      UINT32 totalPending = 0;
      UINT32 totalMoved = 0;
      UINT32 totalSkipped = 0;
      ossPoolVector<freeListPage> pages;
      ossXLatchGuard guard(&_latch);
 
      itr = _tail;
      for (UINT32 i = 0; i < scanNum && NULL != itr; ++i)
      {
         liteCachePageTag *tag = itr;
         itr = itr->getLruPre();

         if (splited() && (_options.lruHotTouchCnt <= tag->getLruTouchCnt()))
         {
            moveToHead(tag);
            ++totalSkipped;
            ++totalMoved;
            continue;
         }

         if (tag->fastTestIfCanBeEvictedFromLru(FALSE))
         {
            freeListPage page;
            if (tryToEvictTagFromList(tag, page))
            {
               pages.push_back(page);
               ++totalEvicted;
               continue;
            }
         }

         /// isMemDirty() not protected by rw latch.
         /// We hold the list latch first, here should not
         /// read fake status.
         if (tag->isMemPageDirty() && tag->setPendingWrite())
         {
            rc = job->addPendingWriteTag(tag);
            if (OSS_UNLIKELY(SDB_OK != rc))
            {
               tag->setUnPendingWrite();
               goto error;
            }
            ++totalPending;
            continue;
         }

         ++totalSkipped;
      }

      if (NULL != itr && splited() && itr->isLruCold())
      {
         _evictBegin = itr;
      }
      else
      {
         _evictBegin = NULL;
      }

      guard.unlock();

      if (NULL != involvedMemPageCount)
      {
         *involvedMemPageCount = totalEvicted + totalPending;
      }
   done:
      if (!pages.empty())
      {
         _fl->releasePages(pages.size(), pages.data());
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
      tag->setLruTouchCnt(0);

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
      
      holder.reset(tag);
      if (!holder.tryLock())
      {
         goto done;
      }

      /// check again under latch
      if (!tag->fastTestIfCanBeEvictedFromLru(TRUE))
      {
         goto done;
      }

      ///We can be sure that the page will not become a mem dirty page,
      ///coz we are holding w lock.
      ///But it is possible that other users have increased the reference count and waiting for lock.
      ///It does not matter, we will not delete tag.
      ///Others users may reinsert tag into lru by themselves. 
      /// Or, tag is marked as io pending by dirty list. But because mem is not dirty, we
      /// can still go on.
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
      r = TRUE;
   done:
      holder.autoUnlock();
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