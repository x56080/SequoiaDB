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
#include "vessel/lcPageTagHolder.h"
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
#include "vessel/instanceEnv.h"
#include "vessel/logicalPageSpace.h"

namespace engine
{
namespace vessel
{
   liteCache::liteCache()
   {}

   liteCache::~liteCache()
   {
      fini();
   }

   INT32 liteCache::init(INT32 poolNo,
                         UINT32 pageSize,
                         const liteCacheOptions &o)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(NULL == _buckets, "do not reinit");

      if (poolNo < 0)
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      else if (!isValidPageSize(pageSize))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      fini();

      _poolNo = poolNo;
      _options = o;
      correctOptions(_options);
      
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

      rc = _fl->init(pageSize, o.freelist);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to setup free list:%d", rc);
         goto error;
      }
         
      rc = _buckets->init(_options.bucket.bucketCount,
                           _options.bucket.bucketLatchCount,
                           _options.bucket.minRecycleCount);
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

      rc = _lru->init(_buckets, _fl, _options.lru);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to setup lru list:%d", rc);
         goto error;
      }

   done:
      return rc;
   error:
      fini();
      goto done;
   }

   INT32 liteCache::fini()
   {
      INT32 rc = SDB_OK;
      if (!isOpen())
      {
         goto done;
      }
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
      _poolNo = -1;
      _options = liteCacheOptions();

   done:
      return SDB_OK;
   }

   void liteCache::correctOptions(liteCacheOptions &options)
   {
      if (!ossIsPowerOf2(options.bucket.bucketCount) ||
          options.bucket.bucketCount < 4096)
      {
         options.bucket.bucketCount = 16384;
      }

      if (!ossIsPowerOf2(options.bucket.bucketLatchCount) ||
          options.bucket.bucketLatchCount < 64)
      {
         options.bucket.bucketLatchCount = 256;
      }

      if (0 == options.freelist.maxChunkCount)
      {
         options.freelist.maxChunkCount = 128;
      }

      if (options.freelist.pageCountInChunk < 512 ||
          !ossIsPowerOf2(options.freelist.pageCountInChunk))
      {
         options.freelist.pageCountInChunk = 512;
      }

      if (options.lru.lruColdPercent < 0.1 || 0.9 < options.lru.lruColdPercent)
      {
         options.lru.lruColdPercent = 0.4;
      }
      if (options.lru.lruMinSplitSize < 512)
      {
         options.lru.lruMinSplitSize = 512;
      }
      if (options.lru.lruScanDepth < 128)
      {
         options.lru.lruScanDepth = 128;
      }
      if (options.lru.lruMaxScanPercent < 0.4)
      {
         options.lru.lruMaxScanPercent = 0.4;
      }
      if (options.lru._lruColdMistakeTolerance > 10)
      {
         options.lru._lruColdMistakeTolerance = 10;
      }
   }

   INT32 liteCache::allocate(requestContext *context,
                             const GLOBAL_PAGE_ID &gpid,
                             const liteCacheAllocateOptions &options,
                             liteCacheTuple &tuple)
   {
      INT32 rc = SDB_OK;
      lcPageTagHolder holder;
      UINT32 pageSize = 0;
      mmapPagePointer ptr;
      BOOLEAN isNewTag = FALSE;

      tuple.release();
      if (OSS_UNLIKELY(NULL == context ||
                       !context->isOpen() ||
                       !gpid.isValid() ||
                       OSS_SHARED_LATCH_MODE_NONE == options.lockMode))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      else if (OSS_UNLIKELY(!isOpen()))
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }

      rc = context->getEnv()->dms.getPageSize(gpid.space(),
                                             gpid.getSpaceType(),
                                             gpid.getFileType(),
                                             pageSize);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to get page size of gpid[%s], rc:%d",
                gpid.toString().c_str(), rc);
         goto error;
      }
      else if (_fl->getPageSize() != pageSize)
      {
         PD_LOG(PDERROR, "wrong page size");
         rc = SDB_VESSEL_OPERATOION_NOT_PERMITTED;
         goto error;
      }

      rc = context->getEnv()->dms.getMmapPagePtr(gpid, ptr);
      if (OSS_UNLIKELY(SDB_OK != rc))
      {
         PD_LOG(PDERROR, "failed to get page[%s-], rc:%d",
                gpid.toString().c_str(), rc);
         goto error;
      }

      if (ONLY_IF_IN_POOL == options.mode)
      {
         if (_buckets->getTagAndIncUsage(gpid, holder))
         {
            holder.lockWithMode(options.lockMode);
            rc = tuple.init(holder.tag(), options.lockMode,
                            this, FALSE);
            if (SDB_OK != rc)
            {
               PD_LOG(PDERROR, "failed to init tuple:%d", rc);
               goto error;
            }
            goto done;
         }
         else
         {
            rc = SDB_VESSEL_LC_NOT_IN_POOL;
            goto error;
         }
      }

      rc = _buckets->ensureTagAndIncUsage(gpid, ptr, holder, isNewTag);
      if (SDB_OK != rc)
      {
         goto error;
      }

      holder.lockWithMode(options.lockMode);
      rc = tuple.init(holder.tag(), options.lockMode,
                      this, FALSE);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to init tuple:%d", rc);
         goto error;
      }

   done:
      return rc;
   error:
      if (holder.valid())
      {
         SDB_ASSERT(holder.isLocked(), "impossible");
         if (isNewTag)
         {
            BOOLEAN rollback = holder.tag()->tryToRollbackNewTag();
            holder.autoUnlock();
            if (rollback)
            {
               _buckets->releaseRemovedTag(holder.tag());
            }
         }
         else
         {
            holder.autoUnlock();
            holder.tag()->decUsageCnt();
         }
      }
      goto done;
   }

   INT32 liteCache::allocateToReset(requestContext *context,
                                    const GLOBAL_PAGE_ID &gpid,
                                    liteCacheTuple &tuple)
   {
      INT32 rc = SDB_OK;
      lcPageTagHolder holder;
      UINT32 pageSize = 0;
      mmapPagePointer ptr;
      BOOLEAN isNewTag = FALSE;
      SDB_ASSERT(!tuple.isValid(), "do not reinit");

      tuple.release();
      if (OSS_UNLIKELY(NULL == context ||
                       !context->isOpen() ||
                       !gpid.isValid()))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      else if (OSS_UNLIKELY(!isOpen()))
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }

      rc = context->getEnv()->dms.getPageSize(gpid.space(),
                                             gpid.getSpaceType(),
                                             gpid.getFileType(),
                                             pageSize);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to get page size of gpid[%s], rc:%d",
                gpid.toString().c_str(), rc);
         goto error;
      }
      else if (_fl->getPageSize() != pageSize)
      {
         PD_LOG(PDERROR, "wrong page size");
         rc = SDB_VESSEL_OPERATOION_NOT_PERMITTED;
         goto error;
      }

      rc = context->getEnv()->dms.getMmapPagePtr(gpid, ptr);
      if (OSS_UNLIKELY(SDB_OK != rc))
      {
         PD_LOG(PDERROR, "failed to get page[%s-], rc:%d",
                gpid.toString().c_str(), rc);
         goto error;
      }

      rc = _buckets->ensureTagAndIncUsage(gpid, ptr, holder, isNewTag);
      if (SDB_OK != rc)
      {
         goto error;
      }

      holder.lock();
      if (!holder.tag()->isInLruList())
      {
         rc = allocateMemPageAndInsertIntoLRU(context, TRUE, holder);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to allocate mem for tag:%d", rc);
            goto error;
         }
      }

      /// Do not update lru.
      rc = tuple.init(holder.tag(), OSS_SHARED_LATCH_MODE_EXCLUSIVE,
                      this, TRUE);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to init tuple:%d", rc);
         goto error;
      }

   done:
      return rc;
   error:
      if (holder.valid())
      {
         if (isNewTag)
         {
            BOOLEAN rollback = holder.tag()->tryToRollbackNewTag();
            holder.autoUnlock();
            if (rollback)
            {
               _buckets->releaseRemovedTag(holder.tag());
            }
         }
         else
         {
            holder.autoUnlock();
            holder.tag()->decUsageCnt();
         }
      }
      goto done;
   }

   void liteCache::commit(UINT64 lsn,
                          liteCacheTuple &tuple)
   {
      INT32 rc = SDB_OK;
      lcPageTagHolder holder;

      if (OSS_UNLIKELY(!tuple.isValid() ||
                       tuple._lockingMode != OSS_SHARED_LATCH_MODE_EXCLUSIVE ||
                       !tuple.isWritingPrepared() ||
                       !tuple._tag->hasMemPage()))
      {
         PD_LOG(PDERROR, "committed an invalid tuple");
         SDB_ASSERT(FALSE, "invalid tuple to commit");
         goto done;
      }
      else if (OSS_UNLIKELY(DPS_INVALID_LSN_OFFSET == lsn))
      {
         SDB_ASSERT(FALSE, "can not commit invalid lsn");
         PD_LOG(PDERROR, "commit invalid lsn");
         goto done;
      }
      else if (OSS_UNLIKELY(!isOpen()))
      {
         goto done;
      }

      holder = lcPageTagHolder(tuple._tag, (OSS_SHARED_LATCH_MODE)tuple._lockingMode);
      rc = _dl->upsert(lsn, holder);
      if (SDB_OK != rc)
      {
         PD_LOG(PDSEVERE, "failed to upsert dirty list with lsn[%lld], page[%s], rc:%d",
                lsn, tuple._tag->id().toString().c_str(), rc);
      }
   done:
      return;
   }

   INT32 liteCache::tryToUpdateLRU(lcPageTagHolder &holder)
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
                                                    BOOLEAN zeroed,
                                                    lcPageTagHolder &holder)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(holder.valid(), "invalid holder");
      liteCachePageTag *tag = NULL;
      freeListPage page;
      ossValuePtr diskPtr = 0;

      if (OSS_UNLIKELY(!holder.valid()))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      else if (OSS_UNLIKELY(OSS_SHARED_LATCH_MODE_EXCLUSIVE != holder.getLockMode()))
      {
         rc = SDB_VESSEL_FORBIDDEN_OP_WLT;
         goto error;
      }
      else if (OSS_UNLIKELY(holder.tag()->isInLruList()))
      {
         rc = SDB_VESSEL_OPERATOION_NOT_PERMITTED;
         goto error;
      }

      SDB_ASSERT(!holder.tag()->hasMemPage(), "impossible");

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
      if (!zeroed)
      {
         ossMemcpy((void *)(page.buf()), (const void *)diskPtr, _fl->getPageSize());
      }
      else
      {
         ossMemset((void *)(page.buf()), 0x0, _fl->getPageSize());
      }

      tag->setMemPage(page);

      /// 3. insert into lru
      rc = _lru->insert(holder, (zeroed ? 0 : 1));
      if (SDB_OK != rc)
      {
         PD_LOG(PDSEVERE, "failed to insert tag[%s] into lru:%d",
                tag->id().toString().c_str(), rc);
         ossPanic();
         goto error;
      }

   done:
      return rc;
   error:
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
      lcPageTagHolder holder;
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

      for (UINT32 i = 0; i < task->getPageCount(); ++i)
      {
         holder.reset(NULL);
         liteCachePageTag *tag = task->getTag(i);
         SDB_ASSERT(NULL != tag, "can not be null");

         holder.reset(tag);
         holder.lockUpgrade();

         SDB_ASSERT(tag->isPendingWrite(FALSE), "must be pending write");
         SDB_ASSERT(tag->isInDirtyList(), "must be in dirty list");

         /// page in dirty list job may not be dirty
         if (tag->isMemPageDirty())
         {
            INT32 tmpRC = logger->pushMaxFileLSN(context->getSession(), tag->getMaxMemDirtyLSN());
            if (OSS_UNLIKELY(SDB_OK != tmpRC))
            {
               PD_LOG(PDSEVERE, "failed to push max file lsn:%lld, rc:%d",
                      tag->getMaxMemDirtyLSN(), tmpRC);
            }

            const freeListPage &buffer = tag->getMemPage();
            void *diskPage = (void *)(tag->getDiskPagePtr());
            SDB_ASSERT(buffer.valid() && NULL != diskPage, "can not be invalid");
            ossMemcpy(diskPage, (const void *)(buffer.buf()), _fl->getPageSize());

            holder.unlockUpgradeAndLock();
            tag->setMaxMemDirtyLSN(DPS_INVALID_LSN_OFFSET);
         }

         /// to avoid fsyncing each page separately, we remove all the pages from dirty list first.
         if (fsync)
         {
            if (OSS_SHARED_LATCH_MODE_UPGRADE == holder.getLockMode())
            {
               holder.unlockUpgradeAndLock();
            }
            _dl->remove(holder);
         }

         holder.autoUnlock();
         holder.reset(NULL);
      }

      if (fsync)
      {
         rc = fsyncIOTask(context, task);
         if (SDB_OK != rc)
         {
            PD_LOG(PDSEVERE, "failed to fsync disk pages, current min dirty lsn[%lld], rc:%d",
                   _dl->getMinDirtyLSN(TRUE), rc);
            goto error;
         }
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

   void liteCache::updateMinCacheLsn()
   {
      SDB_ASSERT(isOpen(), "can not be closed");
      _dl->updateMinDirtyLsn();
   }

   INT32 liteCache::fsyncIOTask(requestContext *context,
                                diskIOTask *task)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(NULL != context, "can not be null");
      SDB_ASSERT(NULL != task, "can not be null");

      UINT32 segment = 0;
      logicalPageSpace *lps = NULL;
      GLOBAL_PAGE_ID gpid = task->getFirstPID();
      if (!gpid.isValid())
      {
         PD_LOG(PDERROR, "invalid first gpid");
         rc = SDB_VESSEL_INTERNAL_ERR;
         goto error;
      }

      rc = context->getEnv()->dms.getLogicalPageSpace(gpid.space(),
                                                      gpid.getSpaceType(),
                                                      &lps);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to get lps of gpid[%s], rc:%d",
                gpid.toString().c_str(), rc);
         goto error;
      }

      if (gpid.getFileType() != lps->getStorageFileType())
      {
         PD_LOG(PDERROR, "invalid file type:%d", gpid.getFileType());
         rc = SDB_VESSEL_INTERNAL_ERR;
         goto error;
      }

      segment = gpid.page() / lps->getStorageCoreArgs().maxPageCountPerSeg;
      rc = lps->fsyncSegment(segment);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to fsync segment of gpid[%s], rc:%d",
                gpid.toString().c_str(), rc);
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
} /// end of namespace vessel
} /// end of namespace engine
