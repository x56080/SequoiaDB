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

   Source File Name = liteCache.cpp

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

#include "vessel/liteCache.h"
#include "ossErr.h"
#include "vessel/lcExtentTagHolder.h"
#include "vessel/vesselDef.h"
#include "vessel/collectionSpaceContainer.h"
#include "vessel/liteCacheDef.h"
#include "pdTrace.hpp"
#include "ossLikely.hpp"
#include "ossUtil.hpp"
#include "vessel/diskIOJob.h"
#include "vessel/lcLRUList.h"
#include "vessel/lcDirtyList.h"
#include "vessel/lcFreeList.h"
#include "vessel/lcBuckets.h"
#include "vessel/requestContext.h"
#include "vessel/outerResource.h"
#include "vessel/IRedoLogger.h"
#include "vessel/diskIOTask.h"
#include "vessel/storageUnit.h"

namespace engine
{
namespace vessel
{
   liteCache::liteCache()
   :_container(NULL),
   _buckets(NULL),
   _lru(NULL),
   _dl(NULL),
   _fl(NULL)
   {

   }

   liteCache::~liteCache()
   {
      fini();
   }

   INT32 liteCache::init(const liteCacheOptions &o, collectionSpaceContainer *container)
   {
      INT32 rc = SDB_OK;
      BOOLEAN rollback = FALSE;

      if (NULL == container)
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      else if (NULL != _container)
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      rollback = TRUE;
      _buckets = SDB_OSS_NEW lcBuckets();
      if (NULL == _buckets)
      {
         PD_LOG(PDERROR, "failed to allocate mem");
         rc = SDB_OOM;
         goto error;
      }

      _fl = SDB_OSS_NEW lcFreeList();
      if (NULL == _fl)
      {
         PD_LOG(PDERROR, "failed to allocate mem");
         rc = SDB_OOM;
         goto error;
      }

      _lru = SDB_OSS_NEW lcLRUList();
      if (NULL == _lru)
      {
         PD_LOG(PDERROR, "failed to allocate mem");
         rc = SDB_OOM;
         goto error;
      }

      _dl = SDB_OSS_NEW lcDirtyList();
      if (NULL == _dl)
      {
         PD_LOG(PDERROR, "failed to allocate mem");
         rc = SDB_OOM;
         goto error;
      }

      rc = _fl->init(o.freelist);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to setup free list:%d", rc);
         goto error;
      }
         
      rc = _buckets->init(o.bucket.bucketCount,
                           o.bucket.bucketLatchCount,
                           o.bucket.minRecycleCount);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to setup buckets:%d", rc);
         goto error;
      }

      rc = _dl->init();
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to setup dirty list:%d", rc);
         goto error;
      }

      rc = _lru->init(_buckets, _fl, o.lru);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to setup lru list:%d", rc);
         goto error;
      }

      _options = o;
      _container = container;
   done:
      return rc;
   error:
      if (rollback)
      {
         fini();
      }
      goto done;
   }

   INT32 liteCache::fini()
   {
      INT32 rc = SDB_OK;
      if (NULL != _dl)
      {
         rc = _dl->fini();
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to teardown dirty list:%d", rc);
         }
      }

      if (NULL != _lru)
      {
         rc = _lru->fini();
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to teardown lru list:%d", rc);
         }
      }

      if (NULL != _buckets)
      {
         rc = _buckets->fini();
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to teardown _buckets:%d", rc);
         }
      }

      if (NULL != _fl)
      {
         rc = _fl->fini();
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to teardown free list:%d", rc);
         }
      }

      SAFE_OSS_DELETE(_dl);
      SAFE_OSS_DELETE(_lru);
      SAFE_OSS_DELETE(_buckets);
      SAFE_OSS_DELETE(_fl);
      _container = NULL;

   done:
      return SDB_OK;
   }

   INT32 liteCache::allocate(requestContext *context,
                             const GLOBAL_PAGE_ID &gpid,
                             const liteCacheAllocateOptions &options,
                             liteCacheTuple &tuple)
   {
      INT32 rc = SDB_OK;
      lcExtentTagHolder holder;
      UINT32 pageSize = 0;
      ossValuePtr ptr = 0;
      storageUnit *su = NULL;
      BOOLEAN newTagInBucket = FALSE;

      if (OSS_UNLIKELY(gpid.invalid()))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      rc = _buckets->getTagAndIncUsage(gpid, holder);
      if (SDB_OK != rc)
      {
         goto error;
      }

      if (holder.valid())
      {
         rc = initTupleBeforeReturn(holder, options, tuple);
         if (SDB_OK != rc)
         {
            goto error;
         }
         goto done;
      }
      
      if (ONLY_IF_IN_POOL == options.mode)
      {
         rc = SDB_VESSEL_LC_NOT_IN_POOL;
         goto error;
      }

      rc = _container->getSUByLockedSpaceID(context, &su);
      if (OSS_UNLIKELY(SDB_OK != rc))
      {
         goto error;
      }

      rc = su->getCoreArgs(gpid.type(), &pageSize);
      if (OSS_UNLIKELY(SDB_OK != rc))
      {
         goto error;
      }

      rc = su->getPagePtr(gpid.type(), gpid.page(), ptr);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to get page ptr of [%s], rc:%d",
                gpid.toString().c_str(), rc);
         goto error;
      }

      rc = _buckets->ensureTagAndIncUsage(gpid, pageSize,
                                          holder, newTagInBucket);
      if (SDB_OK != rc)
      {
         goto error;
      }

      if (newTagInBucket)
      {
         SDB_ASSERT(LOCK_MODE_UNIQUE == holder.getLockMode(), "must be unique");
         rc = initNewTagInBucket(context, pageSize, ptr, holder);
         if (SDB_OK != rc)
         {
            holder.unlockUnique();
            holder.tag()->decUsageCnt();
            if (holder.tag()->tryToSetRecycled())
            {
               _buckets->releaseRemovedTag(holder.tag());
            }
            /// if failed to remove tag, just leave it in the bucket and wait to be recycled.
            PD_LOG(PDSEVERE, "disk page crashed, gpid:%s", gpid.toString().c_str());
            holder.reset(NULL);
            goto error;
         }

         holder.unlockUnique();
      }

      rc = initTupleBeforeReturn(holder, options, tuple);
      if (SDB_OK != rc)
      {
         goto error;
      }
   done:
      return rc;
   error:
      if (holder.valid())
      {
         SDB_ASSERT(LOCK_MODE_NONE == holder.getLockMode(), "can not holding lock");
         holder.tag()->decUsageCnt();
         holder.reset(NULL);
      }
      goto done;
   }

   void liteCache::commit(requestContext *context,
                          UINT64 lsn,
                          liteCacheTuple &tuple)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(NULL != context, "can not br null");
      lcExtentTag *tag = NULL;
      SDB_ASSERT(tuple.valid(), "tuple should be valid");
      SDB_ASSERT(tuple._holder.getLockMode() == LOCK_MODE_UNIQUE, "holding unique lock");
      SDB_ASSERT(DPS_INVALID_LSN_OFFSET != lsn, "can not be invalid");
      SDB_ASSERT(tuple._writingPrepared, "must be prepared");

      if (OSS_UNLIKELY(!tuple.valid()))
      {
         PD_LOG(PDERROR, "committed an invalid tuple");
         goto done;
      }
      else if (OSS_UNLIKELY(LOCK_MODE_UNIQUE != tuple._holder.getLockMode()))
      {
         PD_LOG(PDERROR, "holding wrong type lock");
         goto done;
      }
      else if (OSS_UNLIKELY(DPS_INVALID_LSN_OFFSET == lsn))
      {
         PD_LOG(PDERROR, "commit invalid lsn");
         goto done;
      }

      tag = tuple._holder.tag();
      SDB_ASSERT(tag->hasMemPage(), "mem page must be allocated");
      if (OSS_UNLIKELY(!tag->hasMemPage()))
      {
         PD_LOG(PDERROR, "tuple has no mem page");
         goto done;
      }

      if (tag->inDirtyList())
      {
         /// max lsn is impossible to be invalid when in dirty list.
         if (tag->getMaxLSN() < lsn)
         {
            tag->setMaxLSN(lsn);
         }
         else
         {
            SDB_ASSERT(FALSE, "redo log might be truncated");
            PD_LOG(PDSEVERE, "redo log might be truncated. current tag's max lsn:%lld, commit lsn:%lld", tag->getMaxLSN(), lsn);
         }
      }
      else
      {
         if (DPS_INVALID_LSN_OFFSET != tag->getMaxLSN() &&
             lsn <= tag->getMaxLSN())
         {
            SDB_ASSERT(FALSE, "redo log might be truncated");
            PD_LOG(PDSEVERE, "redo log might be truncated. current tag's max lsn:%lld, commit lsn:%lld", tag->getMaxLSN(), lsn);
         }

         tag->setMinAndMaxLSN(lsn);         
         rc = _dl->insert(tuple._holder);
         if (OSS_UNLIKELY(SDB_OK != rc))
         {
            SDB_ASSERT(FALSE, "must be ok");
            PD_LOG(PDSEVERE, "failed to insert tag into dirty list, gpid:%s, lsn:%lld", tag->id().toString().c_str(), lsn);
         }
      }
   done:
      return;
   }

   void liteCache::release(requestContext *context,
                           liteCacheTuple &tuple)
   {
      SDB_ASSERT(tuple.valid(), "tuple should be valid");
      SDB_ASSERT(LOCK_MODE_NONE < tuple._holder.getLockMode(), "tuple should be locked");
   
      if (OSS_LIKELY(tuple.valid()))
      {
         tuple._holder.unlock();
         tuple._holder.tag()->decUsageCnt();
         tuple._holder.reset(NULL);
         tuple._pool = NULL;
         tuple._writingPrepared = FALSE;
      }
      return;
   }

   INT32 liteCache::tryToUpdateLRU(lcExtentTagHolder &holder)
   {
      return _lru->tryToUpdate(holder);
   }

   UINT64 liteCache::getAllocatedCountFromFreeList()const
   {
      if (OSS_LIKELY(NULL != _fl))
      {
         return _fl->getTotalAllocated();
      }
      return 0;
   }

   INT32 liteCache::allocateMemPageAndInsertIntoLRU(requestContext *context,
                                                    lcExtentTagHolder &holder)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(holder.valid(), "invalid holder");
      SDB_ASSERT(LOCK_MODE_UNIQUE == holder.getLockMode(), "holding wrong type lock");
      SDB_ASSERT(!holder.tag()->hasMemPage(), "already has mem page");
      lcExtentTag *tag = NULL;
      freeListPage page;
      ossValuePtr diskPtr = 0;

      if (OSS_UNLIKELY(!holder.valid()))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      if (OSS_UNLIKELY(LOCK_MODE_UNIQUE != holder.getLockMode()))
      {
         rc = SDB_VESSEL_FORBIDDEN_OP_WLT;
         goto error;
      }

      if (OSS_UNLIKELY(holder.tag()->hasMemPage()))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      tag = holder.tag();

      diskPtr = tag->getDiskPagePtr();
      SDB_ASSERT(0 != diskPtr, "can not be invalid");

      /// 1. allocate chunk pages
      rc = ensureMemPage(context, page);
      if (SDB_OK != rc)
      {
         goto error;
      }

      /// 2. copy data and init tag's mem page
      ossMemcpy((void *)(page.buf()), (const void *)diskPtr, tag->getPageSize());

      /// 3. insert into lru
      rc = _lru->insert(holder, page);
      if (SDB_OK != rc)
      {
         goto error;
      }
      page.reset();
   done:
      return rc;
   error:
      if (page.valid())
      {
         _fl->releasePage(page);
      }
      goto done;
   }

   INT32 liteCache::batchFlushOrEvictLRU(requestContext *context,
                                        UINT32 scanDepth,
                                        diskIOJob *job,
                                        UINT32 *involvedChunkPageCount)
   {
      return _lru->setPendingWriteOrEvict(context, scanDepth, job, involvedChunkPageCount);
   }

   void liteCache::resetLRUEvictBegin()
   {
      return _lru->resetEvictBegin();
   }

   INT32 liteCache::createDirtyListIOJob(requestContext *context,
                                         UINT32 scanDepth,
                                         UINT64 minLSN,
                                         diskIOJob *job)
   {
      return _dl->setPendingWrite(context, scanDepth, minLSN, job);
   }
   
   INT32 liteCache::executeIOTask(requestContext *context,
                                  diskIOTask *task)
   {
      INT32 rc = SDB_OK;
      IRedoLogger *logger = NULL;
      lcExtentTagHolder holder;
      BOOLEAN fsync = FALSE;

      if (OSS_UNLIKELY(NULL == context || NULL == task))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      else if (OSS_UNLIKELY(!task->valid()))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      fsync = task->getJob()->needFSync();
      logger = context->getOuterResource()->logger;
      SDB_ASSERT(NULL != logger, "can not be null");

      if (fsync)
      {
         INT32 cacheRC = _dl->cacheMinDirtyLSN();
         if (SDB_OK != cacheRC)
         {
            PD_LOG(PDSEVERE, "failed to cache min dirty lsn:%d", cacheRC);
            goto error;
         }
      }

      for (UINT32 i = 0; i < task->getPageCount(); ++i)
      {
         holder.reset(NULL);
         lcExtentTag *tag = task->getTag(i);
         SDB_ASSERT(NULL != tag, "can not be null");

         holder.reset(tag);
         holder.lockUnique();

         if (OSS_UNLIKELY(!tag->pendingWrite()))
         {
            rc = SDB_VESSEL_INTERNAL_ERR;
            PD_LOG(PDERROR, "tag is not pending:%s", tag->id().toString().c_str());
            goto error;
         }

         if (OSS_UNLIKELY(!tag->inDirtyList()))
         {
            rc = SDB_VESSEL_INTERNAL_ERR;
            PD_LOG(PDERROR, "page is not in dirty list:%s", tag->id().toString().c_str());
            goto error;
         }

         if (tag->hasMemPage() && tag->isDirty())
         {
            rc = logger->pushMaxFileLSN(context->getSession(), tag->getMaxLSN());
            if (OSS_UNLIKELY(SDB_OK != rc))
            {
               PD_LOG(PDERROR, "failed to push max file lsn:%d", rc);
               goto error;
            }

            rc = tag->copyDataToDisk();
            if (OSS_UNLIKELY(SDB_OK != rc))
            {
               PD_LOG(PDERROR, "failed to copy data to disk, gpid:%s, rc:%d", tag->id().toString().c_str(), rc);
               goto error;
            }
            tag->clearFlags(ET_FLAG_DIRTY);
         }

         /// to avoid fsyncing each page separately, we remove all the pages from dirty list first.
         if (fsync)
         {
            _dl->remove(holder);
         }

         holder.unlockUnique();
         holder.reset(NULL);
      }

      if (fsync)
      {
         rc = fsyncDiskPages(context, task->getFirstPID(), task->getPageCount());
         if (SDB_OK != rc)
         {
            DPS_LSN_OFFSET lsn = _dl->getMinDirtyLSN();
            PD_LOG(PDSEVERE, "failed to fsync disk pages:%d, min dirty lsn[%lld] will fall into chaos!", lsn);
            _dl->removeCachedMinDirtyLSN();
            goto error;
         }
         _dl->removeCachedMinDirtyLSN();
      }
   done:
      if (NULL != task)
      {
         task->done();
      }
      return rc;
   error:
      if (holder.valid())
      {
         holder.unlock();
      }
      goto done;
   }

   INT32 liteCache::fsyncDiskPages(requestContext *context,
                                   const GLOBAL_PAGE_ID &gpid,
                                   UINT32 count)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(NULL != context, "can not be null");
      SDB_ASSERT(!gpid.invalid(), "can not be invalid");
      SDB_ASSERT(0 < count, "can not be zero");

      storageUnit *su = NULL;
      rc = _container->getUnlockedSU(gpid.space(), &su);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to get su[%d], rc:%d", gpid.space(), rc);
         goto error;
      }

      rc = su->fsync(gpid.type(), gpid.page(), count, TRUE);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to fsync disk pages:%d", rc);
         goto error;
      }
   done:
      return rc;
   error:
      goto done;
   }

   INT32 liteCache::ensureMemPage(requestContext *context, freeListPage &page)
   {
      INT32 rc = SDB_OK;
      UINT32 scanLoop = 0;
      
      do
      {
         if (1 < scanLoop)
         {
            PD_LOG(PDWARNING, "eviction times over one:%d", scanLoop);
            ossSleepmillis(10);
         }

         rc = _fl->allocate(page);
         if (SDB_OK == rc)
         {
            goto done;
         }
         else if (SDB_VESSEL_LC_NOT_ENOUGH_PAGES_IN_FL == rc)
         {
            rc = _lru->evict(context, 0 < scanLoop, page);
            if (SDB_OK == rc)
            {
               goto done;
            }
            else if (SDB_VESSEL_LC_LRU_SCAN_HIT_MAX == rc)
            {
               rc = SDB_OK;
               ++scanLoop;
               continue;
            }
            else
            {
               goto error;
            }
         }
         else
         {
            goto error;
         }
      } while (TRUE);

   done:
      return rc;
   error:
      goto done;
   }

   INT32 liteCache::initTupleBeforeReturn(lcExtentTagHolder &holder,
                                          const liteCacheAllocateOptions &options,
                                          liteCacheTuple &tuple)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(holder.valid(), "must be valid");
      LOCK_MODE mode = options.readonly ? LOCK_MODE_SHARED : LOCK_MODE_UPGRADE;
      holder.lockWithMode(mode);

      if (!holder.tag()->hasDiskPage())
      {
         rc = SDB_VESSEL_PAGE_CRASHED;
         goto error;
      }
      tuple._holder = holder;
      tuple._pool = this;
      tuple._writingPrepared = FALSE;
   done:
      return rc;
   error:
      holder.unlock();
      goto done;
   }

   INT32 liteCache::initNewTagInBucket(requestContext *context,
                                       UINT32 pageSize,
                                       ossValuePtr diskPage,
                                       lcExtentTagHolder &holder)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(0 < pageSize, "can not be invalid");
      SDB_ASSERT(0 != diskPage, "can not be invalid");
      SDB_ASSERT(holder.valid(), "can not be invalid");
      SDB_ASSERT(LOCK_MODE_UNIQUE == holder.getLockMode(), "must be unique lock");
      lcExtentTag *tag = NULL;
      const pageHead *head = NULL;
      
      if (!validatePageHeadAndTail(diskPage, pageSize))
      {
         rc = SDB_VESSEL_PAGE_CRASHED;
         goto error;
      }

      head = (const pageHead *)diskPage;
      tag = holder.tag();
      tag->setDiskPagePtr(diskPage);
      tag->setMinAndMaxLSN(head->lsn);
   done:
      return rc;
   error:
      goto done;
   }


} /// end of namespace vessel
} /// end of namespace engine
