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

   Source File Name = logicalPageSpace.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#include "vessel/logicalPageSpace.h"
#include "vessel/storageUnit.h"
#include "vessel/idMapPage.h"
#include "ossLikely.hpp"
#include "vessel/idMapFile.h"
#include "vessel/instanceEnv.h"
#include "utilStr.hpp"
#include "vessel/atomicOperationList.h"
#include "vessel/storageUtils.h"
#include "vessel/pageInitializer.h"
#include "vessel/dataPageCluster.h"
#include "vessel/dataStorageFileCluster.h"
#include "vessel/storageFileLoader.h"
#include "vessel/deltaLogRecordReader.h"
#include "vessel/deltaLogRecordBuilder.h"
#include "ossLatchGuard.hpp"
#include "vessel/redoLogUtil.h"
#include "vessel/deltaLogRecord.h"
#include "vessel/requestContext.h"
#include "vessel/outerResource.h"
#include "vessel/IRedoLogger.h"
#include "vessel/backgroundEventMsg.h"
#include "vessel/deltaLogFileDef.h"
#include "vessel/memoryBlock.h"

namespace engine
{
namespace vessel
{
   constexpr UINT64 FULL_CHECKPOINT_LPID_CACHE_SIZE = 8388608; /// 8MB
   constexpr UINT64 FULL_CHECKPOINT_DELTA_LOG_SIZE = 128ull * 1024 * 1024; // 128MB
   constexpr UINT64 CHECKPOINT_TRIGGER_DIRTY_PAGE_SIZE = 1024 * 1024 * 1024;

///////////////logicalPageSpace::_runtimePageBufferIniter begin
   INT32 logicalPageSpace::
         _runtimePageBufferIniter::
         initWithCache(const GLOBAL_PAGE_ID &gpid,
                       UINT32 pageSize,
                       liteCacheTuple &tuple,
                       runtimePageBuffer &rpb)
   {
      return rpb.init(gpid, pageSize, tuple);
   }

   INT32 logicalPageSpace::
         _runtimePageBufferIniter::
         initWithMmap(const GLOBAL_PAGE_ID &gpid,
                      UINT32 pageSize,
                      const mmapPagePointer &ptr,
                      runtimePageBuffer &rpb)
   {
      return rpb.init(gpid, pageSize, ptr);
   }


///////////////logicalPageSpace::_runtimePageBufferIniter end

//////////////logicalPageSpace
   logicalPageSpace::~logicalPageSpace()
   {
      fini();
   }

   void logicalPageSpace::close()
   {
      _close();
      fini();
      return;
   }

   void logicalPageSpace::fini()
   {
      _sid = INVALID_SPACE_ID;
      _lpidCache.fini();
      _idMapFiles.close();
      _allocator.fini();
      _logConsole.fini();
      _dpc = NULL;
      _checkpointContext.fini();
      return;
   }

   void logicalPageSpace::destroy(requestContext *context)
   {
      _destroy(context);
      if (NULL != _dpc)
      {
         _dpc->destroy();
         _dpc = NULL;
      }
      if (_logConsole.isReady())
      {
         _logConsole.destroy(context);
      }
      _idMapFiles.destroy();
      fini();
      return;
   }

   INT32 logicalPageSpace::create(requestContext *context,
                                  const createLogicalPageSpaceOptions &o)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(!isOpen(), "do not reinit");
      
      const idMapFile *baseFile = NULL;
      inMemBitmap::options bitmapOptions;
      logicalPageIdCache::options cacheOptions;
      const openDBOptions *globalOptions = NULL;

      if (OSS_UNLIKELY(isOpen()))
      {
         close();
      }

      if (OSS_UNLIKELY(NULL == context ||
                       !context->isOpen() ||
                       !o.isValid()))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      _sid = context->getSpaceID();
   
      /// 1. create id map file
      rc = createFirstIdMapFile(context, o);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to create id map file:%d", rc);
         goto error;
      }
      baseFile = _idMapFiles.getBack<idMapFile>();
      SDB_ASSERT(NULL != baseFile, "can not be null");

      /// 2. init lpid allocator.
      bitmapOptions.bitmapBeginPage = getReservedImpCount();
      bitmapOptions.maxBitmapPageCount = ID_MAP_FILE_MAX_PAGE_COUNT;
      rc = _allocator.init(ID_MAP_PAGE_CAPACITY, bitmapOptions);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to init lpid allocator:%d", rc);
         goto error;
      }

      /// 3. init delta log
      rc = _logConsole.init(context, baseFile, NULL);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to init cached args:%d", rc);
         goto error;
      }

      /// 4. init cache
      globalOptions = &(context->getEnv()->options);
      cacheOptions.bucketCount = globalOptions->_spaceLpidCacheBucketCount;
      cacheOptions.bucketLatchCount = globalOptions->_spaceLpidCacheBucketLatchCount;
      rc = _lpidCache.init(cacheOptions, baseFile);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to init lpid cache:%d", rc);
         goto error;
      }

      /// 5. init page stoarge
      _dpc = getDataStorageObj();
      if (NULL == _dpc)
      {
         PD_LOG(PDERROR, "failed to get storage object");
         rc = SDB_VESSEL_INTERNAL_ERR;
         goto error;
      }

      rc = _dpc->open(context, getSpaceType(),
                      baseFile->getCommonHeadInMem().secretValue,
                      NULL, o.dataArgs,
                      getStorageAllocatorOptions());
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to init page storage:%d", rc);
         goto error;
      }

      /// 6. do something else to create.
      rc = _create(context);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to end to create space:%d", rc);
         goto error;
      }
   done:
      return rc;
   error:
      destroy(context);
      goto done;
   }

   INT32 logicalPageSpace::open(requestContext *context,
                                const storageFileLoader &loader)
   {
      INT32 rc = SDB_OK;
      inMemBitmap::options o;
      const idMapFile *base = NULL;
      idMapFileHead baseHead;
      storageCoreArgs dataArgs;
      logicalPageIdCache::options cacheOptions;
      const openDBOptions *globalOptions = NULL;

      if (OSS_UNLIKELY(isOpen()))
      {
         SDB_ASSERT(FALSE, "do not reinit");
         close();
      }

      if (OSS_UNLIKELY(NULL == context ||
                       !context->isOpen() ||
                       !loader.isValid()))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      globalOptions = &(context->getEnv()->options);
      _sid = context->getSpaceID();
      SDB_ASSERT(loader.getSpaceID() == _sid, "must be same");

      /// Open id map files
      rc = openIdMapFiles(context, loader);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to open id map file:%d", rc);
         goto error;
      }

      base = _idMapFiles.getBack<idMapFile>();
      SDB_ASSERT(NULL != base, "can not be null");

      rc = base->getIdMapFileHead(baseHead);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to get imf head:%d", rc);
         goto error;
      }
      dataArgs.pageSize = baseHead.dataPageSize;
      dataArgs.maxPageCountPerSeg = baseHead.dataPageCountInSeg;
      dataArgs.maxSegmentCountPerFile = baseHead.dataSegCountInFile;
      if (!dataArgs.isValid())
      {
         PD_LOG(PDERROR, "invalid data args");
         rc = SDB_VESSEL_INTERNAL_ERR;
         goto error;
      }
      
      /// init allocator.
      o.bitmapBeginPage = getReservedImpCount();
      o.maxBitmapPageCount = ID_MAP_FILE_MAX_PAGE_COUNT;
      rc = _allocator.init(ID_MAP_PAGE_CAPACITY, o);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to init allocator:%d", rc);
         goto error;
      }

      /// init page storage
      _dpc = getDataStorageObj();
      if (NULL == _dpc)
      {
         PD_LOG(PDERROR, "failed to allocate page storage object");
         rc = SDB_VESSEL_INTERNAL_ERR;
         goto error;
      }

      rc = _dpc->open(context, getSpaceType(),
                      base->getCommonHeadInMem().secretValue,
                      &loader, dataArgs,
                      getStorageAllocatorOptions());
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to init page storage:%d", rc);
         goto error;
      }

      /// open delta log files
      rc = _logConsole.init(context, base,
                            loader.getFileList(getSpaceType(),
                                               FILE_TYPE_DELTA_LOG));
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to init log console:%d", rc);
         goto error;
      }

      cacheOptions.bucketCount = globalOptions->_spaceLpidCacheBucketCount;
      cacheOptions.bucketLatchCount = globalOptions->_spaceLpidCacheBucketLatchCount;
      rc = _lpidCache.init(cacheOptions, base);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to init lpid cache:%d", rc);
         goto error;
      }

      rc = restoreToLatestCheckpoint(context);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to restore to last checkpoint:%d", rc);
         goto error;
      }

      /// do something else.
      rc = _open(context, loader);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to _open:%d", rc);
         goto error;
      }
   done:
      return rc;
   error:
      close();
      goto done;
   }

   INT32 logicalPageSpace::createCheckpoint(requestContext *context,
                                            BOOLEAN forceFullCheckpoint)
   {
      INT32 rc = SDB_OK;
      BOOLEAN fullCheckpoint = FALSE;

      if (OSS_UNLIKELY(!isOpen()))
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }
      else if (OSS_UNLIKELY(NULL == context))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      if (DPS_INVALID_LSN_OFFSET == _checkpointContext.getMaxDirtyLsn())
      {
         goto done;
      }

      fullCheckpoint = forceFullCheckpoint ?
                       TRUE : needFullCheckpoint();
      if (fullCheckpoint)
      {
         rc = createFullCheckpoint(context);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to create full checkpoint:%d", rc);
            goto error;
         }
      }
      else
      {
         rc = createDeltaCheckpoint(context);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to create delta checkpoint:%d", rc);
            goto error;
         }
      }
      
   done:
      return rc;
   error:
      goto done;
   }

   INT32 logicalPageSpace::createDeltaCheckpoint(requestContext *context)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(isOpen(), "can not be invalid");
      SDB_ASSERT(NULL != context, "can not be null");
      checkpointLSN lsn;
      logicalPageSpaceCheckpoint checkpoint;
      BOOLEAN locked = FALSE;
      BOOLEAN abortCheckpoint = FALSE;
      IRedoLogger *logger = context->getOuterResource()->logger;
      ossPoolVector<memoryBlock> mutableBuffers;
      ossPoolSet<UINT32> segments;
      idMapFile *base = _idMapFiles.getBack<idMapFile>();
      SDB_ASSERT(NULL != base, "can not be null");
      DPS_LSN_OFFSET pushLSN = DPS_INVALID_LSN_OFFSET;
      DPS_LSN_OFFSET currentMaxLSN = DPS_INVALID_LSN_OFFSET;

      if (!_checkpointContext.tryToSetRunningFromNoneOrApplying())
      {
         PD_LOG(PDERROR, "lps[%d,%d] checkpoint task is running",
                getSpaceID(), getSpaceType());
         rc = SDB_VESSEL_SAME_TASK_RUNNING;
         goto error;
      }
      abortCheckpoint = TRUE;

      _checkpointContext.getLatch()->lock_w();
      locked = TRUE;

      if (DPS_INVALID_LSN_OFFSET == _checkpointContext.getMaxDirtyLsn())
      {
         goto done;
      }

      PD_LOG(PDINFO, "begin to create checkpoint (delta) on lps[%d,%d]."
                      "current dirty lsn:[%lld, %lld].",
                      getSpaceID(), getSpaceType(),
                      _checkpointContext.getMinDirtyLsn(),
                      _checkpointContext.getMaxDirtyLsn());

      /// actually we only need push lsn to max dirty lsn when space is replicated.
      pushLSN = logger->getCurrentLSN();
      currentMaxLSN = _checkpointContext.getMaxDirtyLsn();

      rc = _lpidCache.dumpBufferAndSetImmutable(getStorageCoreArgs().maxPageCountPerSeg,
                                                mutableBuffers,
                                                isCopyOnWrite() ? &segments : NULL);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to dump mutable buffers:%d", rc);
         goto error;
      }

      rc = prepareToCreateCheckpoint(context, FALSE, lsn);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to prepare to create checkpoint:%d", rc);
         goto error;
      }

      SDB_ASSERT(lsn.isValid(), "can not be invalid");
      _checkpointContext.clearDirtyLSN(FALSE);
      _checkpointContext.getLatch()->release_w();
      locked = FALSE;

      if (isCopyOnWrite())
      {
         rc = flushSegmentsAtCheckpoint(context, segments);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to flush segments:%d", rc);
            goto error;
         }
      }

      logger->pushMaxFileLSN(context->getExecutor(), pushLSN);

      checkpoint.init(0, lsn, ossGetCurrentMilliseconds());
      rc = _logConsole.append(context, mutableBuffers, checkpoint);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to commit delta log:%d", rc);
         goto error;
      }

      mutableBuffers.clear();
      _checkpointContext.setCheckpoint(checkpoint);
      _checkpointContext.setStatus(lpsCheckpointContext::STATUS::ENDING);
      endToCreateCheckpoint(context);
      /// TOOD: update global status here

      _checkpointContext.setStatus(lpsCheckpointContext::STATUS::NONE);
      abortCheckpoint = FALSE;
      currentMaxLSN = DPS_INVALID_LSN_OFFSET;
      PD_LOG(PDINFO, "end to create checkpoint (delta) on lps[%d,%d], :%s",
             getSpaceID(), getSpaceType(),
             _checkpointContext.getCheckpoint().toString().c_str());
      
   done:
      if (locked)
      {
         _checkpointContext.getLatch()->release_w();
      }
      if (DPS_INVALID_LSN_OFFSET != currentMaxLSN)
      {
         /// rollback dir lsn if necessary
         _checkpointContext.updateDirtyLsn(currentMaxLSN);
      }
      if (abortCheckpoint)
      {
         _checkpointContext.setStatus(lpsCheckpointContext::STATUS::NONE);
      }
      return rc;
   error:
      goto done;
   }

   INT32 logicalPageSpace::createFullCheckpoint(requestContext *context)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(isOpen(), "can not be invalid");
      SDB_ASSERT(NULL != context, "can not be null");
      checkpointLSN lsn;
      LPS_CHECKPOINT checkpoint;
      BOOLEAN locked = FALSE;
      BOOLEAN abortCheckpoint = FALSE;
      IRedoLogger *logger = context->getOuterResource()->logger;
      ossPoolSet<UINT32> segments;
      DPS_LSN_OFFSET pushLSN = DPS_INVALID_LSN_OFFSET;
      DPS_LSN_OFFSET currentMaxLSN = DPS_INVALID_LSN_OFFSET;
      UINT32 totalImpCount = 0;

      if (!_checkpointContext.tryToSetRunningFromNoneOrApplying())
      {
         PD_LOG(PDERROR, "lps[%d,%d] checkpoint task is running",
                getSpaceID(), getSpaceType());
         rc = SDB_VESSEL_SAME_TASK_RUNNING;
         goto error;
      }
      abortCheckpoint = TRUE;

      _checkpointContext.getLatch()->lock_w();
      locked = TRUE;

      if (DPS_INVALID_LSN_OFFSET == _checkpointContext.getMaxDirtyLsn())
      {
         goto done;
      }

      PD_LOG(PDINFO, "begin to create checkpoint (full) on lps[%d,%d]."
                      "current dirty lsn:[%lld, %lld].",
                      getSpaceID(), getSpaceType(),
                      _checkpointContext.getMinDirtyLsn(),
                      _checkpointContext.getMaxDirtyLsn());

      /// New imp may allocated in preallocating.
      /// But we can be sure page count not less than real count to be
      /// saved into new base id map file.
      totalImpCount = _allocator.getCustomizedPageCount();
      currentMaxLSN = _checkpointContext.getMaxDirtyLsn();
      pushLSN = logger->getCurrentLSN();

      rc = _lpidCache.prepareToCreateNewBase(getStorageCoreArgs().maxPageCountPerSeg,
                                             isCopyOnWrite() ? &segments : NULL);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to prepare to create new base:%d", rc);
         goto error;
      }

      rc = prepareToCreateCheckpoint(context, TRUE, lsn);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to prepare to create checkpoint:%d", rc);
         goto error;
      }
      SDB_ASSERT(lsn.isValid(), "can not be invalid");

      _checkpointContext.clearDirtyLSN(FALSE);
      _checkpointContext.getLatch()->release_w();
      locked = FALSE;

      if (isCopyOnWrite())
      {
         rc = flushSegmentsAtCheckpoint(context, segments);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to flush segments:%d", rc);
            goto error;
         }
      }

      logger->pushMaxFileLSN(context->getExecutor(), pushLSN);

      checkpoint.init(LPS_CHECKPOINT::FLAG_FULL_CHECKPOINT, lsn,
                      ossGetCurrentMilliseconds());

      _checkpointContext.setStatus(lpsCheckpointContext::STATUS::CREATING_NEW_BASE);
      rc = rebaseWhenCreatingCheckpoint(context, checkpoint, totalImpCount);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to complete delta log:%d", rc);
         goto error;
      }

      _checkpointContext.setCheckpoint(checkpoint);
      _checkpointContext.setStatus(lpsCheckpointContext::STATUS::ENDING);
      endToCreateCheckpoint(context);
      /// TOOD: update global status here

      _checkpointContext.setStatus(lpsCheckpointContext::STATUS::NONE);
      abortCheckpoint = FALSE;
      currentMaxLSN = DPS_INVALID_LSN_OFFSET;
      PD_LOG(PDINFO, "end to create checkpoint (full) on lps[%d,%d], :%s",
            getSpaceID(), getSpaceType(),
            _checkpointContext.getCheckpoint().toString().c_str());
   done:
      if (locked)
      {
         _checkpointContext.getLatch()->release_w();
      }
      if (DPS_INVALID_LSN_OFFSET != currentMaxLSN)
      {
         /// rollback dir lsn if necessary
         _checkpointContext.updateDirtyLsn(currentMaxLSN);
      }
      if (abortCheckpoint)
      {
         _checkpointContext.setStatus(lpsCheckpointContext::STATUS::NONE);
      }
      return rc;
   error:
      goto done;
   }

   INT32 logicalPageSpace::blockCheckpoint(requestContext *context)
   {
      INT32 rc = SDB_OK;
      if (OSS_UNLIKELY(!isOpen()))
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }
      
      SDB_ASSERT(context->getSpaceID() == getSpaceID(), "impossible");
      rc = context->blockCheckpoint(getSpaceType(), _checkpointContext.getLatch());
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to block lps[%d,%d] checkpoint, rc:%d",
                getSpaceID(), getSpaceType(), rc);
         goto error;
      }
   done:
      return rc;
   error:
      goto done;
   }

   INT32 logicalPageSpace::tryToBlockCheckpoint(requestContext *context,
                                                BOOLEAN &blocked)
   {
      INT32 rc = SDB_OK;
      blocked = FALSE;
      if (OSS_UNLIKELY(!isOpen()))
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }
      
      SDB_ASSERT(context->getSpaceID() == getSpaceID(), "impossible");
      rc = context->tryToBlockCheckpoint(getSpaceType(),
                                         _checkpointContext.getLatch(),
                                         blocked);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to block lps[%d,%d] checkpoint, rc:%d",
                getSpaceID(), getSpaceType(), rc);
         goto error;
      }
   done:
      return rc;
   error:
      goto done;
   }

   INT32 logicalPageSpace::getLogicalPageBuffer(requestContext *context,
                                                PAGE_ID lpid,
                                                const ossSharedLatchMode &mode,
                                                logicalPageBuffer &lpb)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(!lpb.isValid(), "impossible");
      BOOLEAN isMutablePage = FALSE;
      idMapSlot slot;

      lpb.fini();
      if (OSS_UNLIKELY(!isOpen()))
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }
      else if (OSS_UNLIKELY(NULL == context ||
                            INVALID_PAGE_ID == lpid ||
                            mode.isNone()))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      rc = validateLpidBeforeGet(lpid);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "can not get lpid[%d], rc:%d", lpid, rc);
         goto error;
      }

      rc = lpb._lh.lock(context, getSpaceType(),
                        lpid, mode);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to lock lpid[%d], rc:%d", lpid, rc);
         goto error;
      }

      rc = getIdMapSlotFromCache(lpid, slot, isMutablePage);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to find lpid[%d] in cache:%d", lpid, rc);
         goto error;
      }

      rc = getRuntimePageBuffer(context, slot.pid,
                                mode, lpb._rpb);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to get runtime buffer of pid[%d], rc:%d", slot.pid, rc);
         goto error;
      }

      lpb.init(this, slot.psv, isMutablePage);
   done:
      return rc;
   error:
      lpb.fini();
      goto done;
   }

   INT32 logicalPageSpace::tryToGetLogicalPageBuffer(requestContext *context,
                                                     PAGE_ID lpid,
                                                     const ossSharedLatchMode &mode,
                                                     logicalPageBuffer &lpb)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(!lpb.isValid(), "impossible");
      BOOLEAN isMutablePage = FALSE;
      idMapSlot slot;

      lpb.fini();
      if (OSS_UNLIKELY(!isOpen()))
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }
      else if (OSS_UNLIKELY(NULL == context ||
                            INVALID_PAGE_ID == lpid ||
                            mode.isNone()))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      rc = validateLpidBeforeGet(lpid);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "can not get lpid[%d], rc:%d", lpid, rc);
         goto error;
      }

      rc = lpb._lh.tryLock(context, getSpaceType(),
                           lpid, mode);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to lock lpid[%d], rc:%d", lpid, rc);
         goto error;
      }

      if (!lpb._lh.isLocked())
      {
         goto done;
      }

      rc = getIdMapSlotFromCache(lpid, slot, isMutablePage);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to find lpid[%d] in cache:%d", lpid, rc);
         goto error;
      }

      rc = getRuntimePageBuffer(context, slot.pid,
                                mode, lpb._rpb);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to get runtime buffer of pid[%d], rc:%d", slot.pid, rc);
         goto error;
      }

      lpb.init(this, slot.psv, isMutablePage);
   done:
      return rc;
   error:
      lpb.fini();
      goto done;
   }

   INT32 logicalPageSpace::isLogicalPageMapped(requestContext *context,
                                                PAGE_ID lpid,
                                                BOOLEAN &mapped)
   {
      INT32 rc = SDB_OK;
      lpidLockHelper lh;
      BOOLEAN isMutablePage = FALSE;
      idMapSlot slot;
      mapped = FALSE;

      if (OSS_UNLIKELY(!isOpen()))
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }
      else if (OSS_UNLIKELY(NULL == context ||
                            INVALID_PAGE_ID == lpid))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      rc = validateLpidBeforeGet(lpid);
      if (SDB_VESSEL_LOGICAL_PAGE_UNMAPPED == rc)
      {
         rc = SDB_OK;
         mapped = FALSE;
         goto done;
      }
      else if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "can not get lpid[%d], rc:%d", lpid, rc);
         goto error;
      }

      if (!context->testLpidLocked(getSpaceType(), lpid, NULL))
      {
         rc = lh.lock(context, getSpaceType(), lpid,
                      ossSharedLatchMode(OSS_SHARED_LATCH_MODE_ENUM_EXCLUSIVE));
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to lock lpid[%d], rc:%d", rc);
            goto error;
         }
      }

      rc = getIdMapSlotFromCache(lpid, slot, isMutablePage);
      if (SDB_VESSEL_LOGICAL_PAGE_UNMAPPED == rc)
      {
         rc = SDB_OK;
         mapped = FALSE;
      }
      else if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to find lpid[%d] in cache:%d", lpid, rc);
         goto error;
      }
      else
      {
         mapped = TRUE;
      }
   done:
      /// auto unlock by lock helper.
      return rc;
   error:
      goto done;
   }

   INT32 logicalPageSpace::makeBufferWritable(logicalPageBuffer &lpb)
   {
      INT32 rc = SDB_OK;
      BOOLEAN snapshotEffective = FALSE;
      idMapSlot slot;
      runtimePageBuffer &rpb = lpb._rpb;
      requestContext *context = lpb._lh.getContext();
      BOOLEAN copyOnWrite = FALSE;

      if (OSS_UNLIKELY(!isOpen()))
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }
      else if (OSS_UNLIKELY(!lpb.isValid()))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      else if (OSS_UNLIKELY(lpb._lps != this))
      {
         SDB_ASSERT(FALSE, "lpb not allocated by this space");
         rc = SDB_INVALIDARG;
         goto error;
      }
      else if (OSS_UNLIKELY(lpb._lh.getLockMode().isNone() ||
                            lpb._lh.getLockMode().isShared()))
      {
         rc = SDB_VESSEL_FORBIDDEN_OP_WLT;
         goto error;
      }
      else if (rpb.isWritingPrepared())
      {
         goto done;
      }

      if (lpb._lh.getLockMode().isUpgrade())
      {
         lpb._lh.lockExclusiveFromUpgrade();
      }

      rc = context->getEnv()->dms.isSnapshotEffective(getSpaceID(),
                                                      lpb.getCowTrigger().getPsv(),
                                                      snapshotEffective);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to check if snapshot effective:%d", rc);
         goto error;
      }

      if (snapshotEffective)
      {
         rc = remapBufferToNewDataPage(context, FALSE, lpb);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to remap lpid[%d], rc:%d",
                   lpb.getLogicalPid(), rc);
            goto error;
         }

         copyOnWrite = TRUE;
      }
      else if (!lpb.getCowTrigger().isMutablePid())
      {
         rc = remapBufferToNewDataPage(context, TRUE, lpb);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to remap lpid[%d], rc:%d",
                   lpb.getLogicalPid(), rc);
            goto error;
         }

         copyOnWrite = TRUE;
      }
      
      SDB_ASSERT(rpb.isValid(), "must be valid");
      if (!rpb.isWritingPrepared())
      {
         rc = rpb.prepareToWrite(context);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to get rpb writable:%d", rc);
            goto error;
         }
      }

      if (copyOnWrite)
      {
         applyCheckpointIfNecessary(context);
      }
      
   done:
      return rc;
   error:
      goto done;
   }

   INT32 logicalPageSpace::ensureReservedPageMapped(requestContext *context,
                                                    PAGE_ID lpid,
                                                    pageInitializer *initer)
   {
      INT32 rc = SDB_OK;
      ossSharedLatchMode mode;
      idMapSlot slot;
      BOOLEAN isMutable = FALSE;
      PAGE_ID pid = INVALID_PAGE_ID;
      mappedLogicalPageId mpid;

      if (OSS_UNLIKELY(!isOpen()))
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }
      else if (OSS_UNLIKELY(NULL == context ||
                            INVALID_PAGE_ID == lpid ||
                            NULL == initer))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      else if (!isReservedLpid(lpid))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      else if (!context->testLpidLocked(getSpaceType(), lpid, &mode) ||
               !mode.isExclusive())
      {
         rc = SDB_VESSEL_FORBIDDEN_OP_WLT;
         goto error;
      }

      rc = _lpidCache.get(lpid, slot, isMutable);
      if (SDB_OK == rc)
      {
         goto done;
      }
      else if (SDB_VESSEL_LOGICAL_PAGE_UNMAPPED != rc)
      {
         PD_LOG(PDERROR, "failed to get lpid[%d] in cache:%d", lpid, rc);
         goto error;
      }

      /// lpid unmapped
      rc = _dpc->allocatePage(context, pid);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to allocate page from page cluster:%d", rc);
         goto error;
      }

      mpid.reset(lpid, pid);
      rc = initAndMapPages(context, initer, 1, &mpid);
      if (SDB_OK != rc)
      {
         _dpc->releasePage(pid);
         PD_LOG(PDERROR, "failed to init page:%d", rc);
         goto error;
      }

      applyCheckpointIfNecessary(context);
   done:
      return rc;
   error:
      goto done;
   }


   INT32 logicalPageSpace::initAndMapPages(requestContext *context,
                                           pageInitializer *initer,
                                           UINT32 count,
                                           const mappedLogicalPageId *mpids)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(NULL != context, "can not be null");
      SDB_ASSERT(NULL != initer, "can not be null");
      SDB_ASSERT(0 != count, "can not be zero");
      SDB_ASSERT(NULL != mpids, "can not be null");
      runtimePageBuffer rpb;
      PAGE_SNAPSHOT_VERION psv = context->getEnv()->dms.getOnlinePageSnapshotVersion();
      
      for (UINT32 i = 0; i < count; ++i)
      {
         if (OSS_UNLIKELY(!mpids[i].isValid()))
         {
            PD_LOG(PDERROR, "invalid mapped id");
            rc = SDB_INVALIDARG;
            goto error;
         }

         rc = getRuntimePageBufferToReset(context, mpids[i].getPid(), rpb);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to get runtime buffer of page[%d], rc:%d",
                   mpids[i].getPid(), rc);
            goto error;
         }
         SDB_ASSERT(rpb.isWritingPrepared(), "must be prepared");

         rc = initer->initInTurns(context, i, mpids[i].getLpid(),
                                  psv, &rpb);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to init page[%d,%d], rc:%d",
                   mpids[i].getLpid(), mpids[i].getPid(), rc);
            goto error;
         }
         rpb.fini();
      }

      rc = map(context, psv, count, mpids);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to map pages:%d", rc);
         goto error;
      }
   done:
      return rc;
   error:
      rpb.fini();
      goto done;
   }

   INT32 logicalPageSpace::remapBufferToNewDataPage(requestContext *context,
                                                    BOOLEAN releaseOld,
                                                    logicalPageBuffer &lpb)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(NULL != context, "can not be null");
      SDB_ASSERT(lpb.isValid(), "can not be invalid");
      SDB_ASSERT(lpb._lh.getLockMode().isExclusive(), "must be exclusive");

      runtimePageBuffer &rpb = lpb._rpb;
      PAGE_SNAPSHOT_VERION psv = context->getEnv()->dms.getOnlinePageSnapshotVersion();
      PAGE_ID lpid = lpb._lh.getLpid();
      PAGE_ID pid = INVALID_PAGE_ID;
      PAGE_ID oldPid = rpb.getGlobalPid().page();
      mappedLogicalPageId mpid;

      /// 1. allocate new pid
      rc = _dpc->allocatePage(context, pid);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to allocate data page:%d", rc);
         goto error;
      }

      /// 2. copy page and reinit
      rc = copyPageAndReinitBuffer(context, psv, pid, rpb);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to copy page:%d", rc);
         goto error;
      }

      /// 3. remap
      mpid.reset(lpid, pid);
      rc = remap(context, psv, 1, &mpid, &oldPid, releaseOld);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to remap lpid[%d], rc:%d", lpid, rc);
         goto error;
      }

      /// 4. reset cow trigger
      lpb._cowTrigger.reset(psv, TRUE);

      //PD_LOG(PDDEBUG, "remap lpid[%d] from [%d] to [%d] in lps[%d,%d]",
      //       lpb.getLogicalPid(), oldPid, pid, getSpaceID(), getSpaceType());

   done:
      return rc;
   error:
      if (INVALID_PAGE_ID != pid)
      {
         _dpc->releasePage(pid);
      }
      goto done;
   }

   INT32 logicalPageSpace::allocatePages(requestContext *context,
                                         pageInitializer *initer,
                                         UINT32 count,
                                         PAGE_ID *lpids)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(count <= 32, "impossible");
      CHAR *buffer = NULL;
      UINT32 bufferSize = 0;
      BOOLEAN rollback = FALSE;

      if (OSS_UNLIKELY(!isOpen()))
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      } 
      else if (OSS_UNLIKELY(NULL == context ||
                            NULL == initer ||
                            0 == count ||
                            NULL == lpids))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      bufferSize = count * sizeof(mappedLogicalPageId);
      buffer = context->allocateBuffer(bufferSize);
      if (OSS_UNLIKELY(NULL == buffer))
      {
         PD_LOG(PDERROR, "failed to allocate mem");
         rc = SDB_OOM;
         goto error;
      }

      rc = preallocate(context, count, (mappedLogicalPageId *)buffer);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to preallocate pages:%d", rc);
         goto error;
      }
      rollback = TRUE;

      rc = initAndMapPages(context, initer, count,
                           (const mappedLogicalPageId *)buffer);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to init and map pages:%d", rc);
         goto error;
      }

      for (UINT32 i = 0; i < count; ++i)
      {
         lpids[i] = ((const mappedLogicalPageId *)buffer)[i].getLpid();
      }

      applyCheckpointIfNecessary(context);
   done:
      if (NULL != buffer)
      {
         context->releaseBuffer(buffer, bufferSize);
      }
      return rc;
   error:
      if (rollback)
      {
         releasePreallocated(context, count,
                             (const mappedLogicalPageId *)buffer);
      }
      for (UINT32 i = 0; i < count; ++i)
      {
         lpids[i] = INVALID_PAGE_ID;
      }
      goto done;
   }

   INT32 logicalPageSpace::releasePages(requestContext *context,
                                        UINT32 count,
                                        const PAGE_ID *lpids)
   {
      INT32 rc = SDB_OK;
      dataManagementService *dms = NULL;
      static const UINT32 _BATCH_COUNT = 16;
      UINT32 i = 0;
      ossPoolVector<mappedLogicalPageId> underSnapshot;
      ossPoolVector<mappedLogicalPageId> notUnderSnapshot;
      ossPoolVector<PAGE_ID> notReservedLpids;

      if (OSS_UNLIKELY(!isOpen()))
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }
      else if (NULL == context ||
               0 == count ||
               NULL == lpids)
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      dms = &(context->getEnv()->dms);

      while (i < count)
      {
         PAGE_ID lpid = lpids[i++];
         idMapSlot slot;
         BOOLEAN isMutable = FALSE;
         mappedLogicalPageId mappedId;
         SDB_ASSERT(INVALID_PAGE_ID != lpid, "can not be invalid");
         BOOLEAN snapshotEffective = FALSE;
         ossPoolVector<mappedLogicalPageId> *vec = NULL;
         BOOLEAN releaseOld = FALSE;

         INT32 rc = getIdMapSlotFromCache(lpid, slot, isMutable);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to get page[%d] from cache, rc:%d", lpid, rc);
            goto error;
         }

         mappedId.reset(lpid, slot.pid);
         rc = dms->isSnapshotEffective(getSpaceID(), slot.psv,
                                      snapshotEffective);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to get snapshot status:%d", rc);
            goto error;
         }

         if (snapshotEffective)
         {
            vec = &underSnapshot;
            releaseOld = FALSE;
         }
         else
         {
            vec = &notUnderSnapshot;
            releaseOld = TRUE;
         }

         vec->push_back(mappedId);
         if (_BATCH_COUNT == vec->size())
         {
            unmap(context, vec->size(), vec->data(), releaseOld);
            vec->clear();
         }

         if (!isReservedLpid(lpid))
         {
            notReservedLpids.push_back(lpid);
         }
      }

      if (!underSnapshot.empty())
      {
         unmap(context, underSnapshot.size(), underSnapshot.data(), FALSE);
      }
      if (!notUnderSnapshot.empty())
      {
         unmap(context, notUnderSnapshot.size(), notUnderSnapshot.data(), TRUE);
      }
      if (!notReservedLpids.empty())
      {
         releaseLpidsPreallocated(context, notReservedLpids.size(), notReservedLpids.data());
      }

      applyCheckpointIfNecessary(context);
   done:
      return rc;
   error:
      goto done;
   }

   INT32 logicalPageSpace::openIdMapFiles(requestContext *context,
                                          const storageFileLoader &loader)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(INVALID_SPACE_ID != _sid, "can not be invalid");
      storageUnit *su = context->getEnv()->dms.getStorageUnit(_sid);
      SDB_ASSERT(NULL != su, "can not be null");
      idMapFile *file = NULL;

      const FILE_NAME_LIST *list = loader.getFileList(getSpaceType(), FILE_TYPE_ID_MAP);
      if (NULL == list || list->empty())
      {
         PD_LOG(PDERROR, "id map file not found");
         rc = SDB_VESSEL_INVALID_VESSEL_FILE;
         goto error;
      }

      for (FILE_NAME_LIST::const_iterator itr = list->begin();
           itr != list->end(); ++itr)
      {
         const vesselFileName &fn = *itr;
         if (!fn.isValid())
         {
            PD_LOG(PDERROR, "invalid file name");
            rc = SDB_INVALIDARG;
            goto error;
         }
         else if (fn.getSpaceID() != _sid)
         {
            PD_LOG(PDERROR, "space id does not match:%d, %d",
                   fn.getSpaceID(), _sid);
            rc = SDB_VESSEL_INVALID_VESSEL_FILE;
            goto error;
         }
         else if (fn.getSpaceType() != getSpaceType())
         {
            PD_LOG(PDERROR, "space type does not match:%d, %d",
                   fn.getSpaceType(), getSpaceType());
            rc = SDB_VESSEL_INVALID_VESSEL_FILE;
            goto error;
         }
         else if (fn.getFileType() != FILE_TYPE_ID_MAP)
         {
            PD_LOG(PDERROR, "file type does not match:%d",
                   fn.getFileType());
            rc = SDB_VESSEL_INVALID_VESSEL_FILE;
            goto error;
         }
         else if (fn.hasShadowSuffix())
         {
            PD_LOG(PDERROR, "file name has shadow suffix:%s",
                   fn.getFileName());
            rc = SDB_VESSEL_INVALID_VESSEL_FILE;
            goto error;
         }

         file = SDB_OSS_NEW idMapFile();
         if (OSS_UNLIKELY(NULL == file))
         {
            PD_LOG(PDERROR, "failed to allocate mem");
            rc = SDB_OOM;
            goto error;
         }

         rc = su->openStorageFile(fn, file);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to open id map file[%s], :%d", fn.getFileName(), rc);
            goto error;
         }

         if (ID_MAP_FILE_PAGE_SIZE != file->getCommonHeadInMem().pageSize)
         {
            PD_LOG(PDERROR, "page size does not match:%d",
                   file->getCommonHeadInMem().pageSize);
            rc = SDB_VESSEL_INVALID_VESSEL_FILE;
            goto error;
         }

         if (ID_MAP_FILE_MAX_PAGE_COUNT_IN_SEG != file->getCommonHeadInMem().maxPageCountPerSeg)
         {
            PD_LOG(PDERROR, "page count in seg does not match:%d",
                   file->getCommonHeadInMem().maxPageCountPerSeg);
            rc = SDB_VESSEL_INVALID_VESSEL_FILE;
            goto error;
         }

         if (ID_MAP_FILE_MAX_SEG_COUNT_IN_FILE != file->getCommonHeadInMem().maxSegmentCountPerFile)
         {
            PD_LOG(PDERROR, "segment count in file does not match:%d",
                   file->getCommonHeadInMem().maxSegmentCountPerFile);
            rc = SDB_VESSEL_INVALID_VESSEL_FILE;
            goto error;
         }

         rc = _idMapFiles.unsortedPushBack(file);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to insert file[%s] into map:%d",
                   file->getFullPath(), rc);
            goto error;
         }

         file = NULL;
      }

      if (_idMapFiles.isEmpty())
      {
         PD_LOG(PDERROR, "id map file map is empty");
         rc = SDB_VESSEL_INTERNAL_ERR;
         goto error;
      }

      _idMapFiles.resort();

      rc = validateIdMapFileMap();
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to validate id map file list:%d", rc);
         goto error;
      }
   done:
      return rc;
   error:
      if (NULL != file)
      {
         file->close();
         SDB_OSS_DEL file;
      }
      _idMapFiles.close();
      goto done;
   }

   INT32 logicalPageSpace::validateIdMapFileMap()
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(!_idMapFiles.isEmpty(), "can not be empty");
      const idMapFile *base = NULL;
      idMapFileHead baseHead;

      base = (const idMapFile *)(_idMapFiles.getBack());
      rc = base->getIdMapFileHead(baseHead);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to get imf head of file[%s], rc:%d",
                base->getFullPath(), rc);
         goto error;
      }

      if (!validateIdMapFileHeadFlags(baseHead.flags))
      {
         PD_LOG(PDERROR, "flags[%d] of base file head is not valid", baseHead.flags);
         rc = SDB_VESSEL_INVALID_VESSEL_FILE;
         goto error;
      }

      for (sortedStorageFileList::CONST_ITERATOR itr = _idMapFiles.begin();
           itr != _idMapFiles.end(); ++itr)
      {
         const idMapFile *fileInMap = (const idMapFile *)(*itr);
         idMapFileHead h;
         if (fileInMap->getCommonHeadInMem().sequence ==
             base->getCommonHeadInMem().sequence)
         {
            break;
         }

         rc = fileInMap->getIdMapFileHead(h);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to get id map file head of file[%s], rc:%d",
                   fileInMap->getFullPath(), rc);
            goto error;
         }

         if (!baseHead.hasSameArgs(h))
         {
            PD_LOG(PDERROR, "different args found in id map files");
            rc = SDB_VESSEL_INVALID_VESSEL_FILE;
            goto error;
         }

         if (fileInMap->getCommonHeadInMem().secretValue != 
             base->getCommonHeadInMem().secretValue)
         {
            PD_LOG(PDERROR, "different secret value found in file:%s",
                   fileInMap->getFullPath());
            rc = SDB_VESSEL_INVALID_VESSEL_FILE;
            goto error;
         }
      }
   done:
      return rc;
   error:
      goto done;
   }


   FILE_TYPE logicalPageSpace::getStorageFileType()const
   {
      SDB_ASSERT(isOpen(), "can not be closed");
      FILE_TYPE type = INVALID_FILE_TYPE;
      if (OSS_UNLIKELY(NULL != _dpc))
      {
         type = _dpc->getDataFileType();
      }
      return type;
   }

   const storageCoreArgs &logicalPageSpace::getStorageCoreArgs()const
   {
      SDB_ASSERT(isOpen(), "can not be closed");
      return _dpc->getCoreArgs();
   }

   INT32 logicalPageSpace::fsyncSegment(UINT32 segment)const
   {
      INT32 rc = SDB_OK;
      if (!isOpen())
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }

      rc = _dpc->fsyncSegment(segment);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to fsync segment[%d], rc:%d", segment, rc);
         goto error;
      }
   done:
      return rc;
   error:
      goto done;
   }

   INT32 logicalPageSpace::getPagePtr(FILE_TYPE type,
                                      PAGE_ID pid,
                                      mmapPagePointer &ptr)const
   {
      INT32 rc = SDB_OK;
      if (OSS_UNLIKELY(!isOpen()))
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }

      rc = _dpc->getPagePtr(type, pid, ptr);
      if (SDB_OK != rc)
      {
         goto error;
      }
   done:
      return rc;
   error:
      goto done;
   }

   INT32 logicalPageSpace::preallocate(requestContext *context,
                                       UINT32 count,
                                       mappedLogicalPageId *mpids)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(isOpen(), "can not be closed");

      CHAR *pidBuffer = NULL;
      CHAR *lpidBuffer = NULL;
      UINT32 bufferSize = 0;
      if (OSS_UNLIKELY(NULL == context ||
                       0 == count ||
                       NULL == mpids))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      bufferSize = count * sizeof(PAGE_ID);
      pidBuffer = context->allocateBuffer(bufferSize);
      if (OSS_UNLIKELY(NULL == pidBuffer))
      {
         PD_LOG(PDERROR, "failed to allocate mem");
         rc = SDB_OOM;
         goto error;
      }
      lpidBuffer = context->allocateBuffer(bufferSize);
      if (OSS_UNLIKELY(NULL == lpidBuffer))
      {
         PD_LOG(PDERROR, "failed to allocate mem");
         rc = SDB_OOM;
         goto error;
      }

      rc = preallocateLpids(context, count, (PAGE_ID *)lpidBuffer);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to preallocate lpids:%d", rc);
         goto error;
      }

      rc = _dpc->allocatePages(context, count, (PAGE_ID *)pidBuffer);
      if (SDB_OK != rc)
      {
         releaseLpidsPreallocated(context, count, (const PAGE_ID *)lpidBuffer);
         PD_LOG(PDERROR, "failed to allocate pids:%d", rc);
         goto error;
      }

      for (UINT32 i = 0; i < count; ++i)
      {
         mpids[i].reset(((const PAGE_ID *)lpidBuffer)[i],
                         ((const PAGE_ID *)pidBuffer)[i]);
      }
   done:
      if (NULL != lpidBuffer)
      {
         context->releaseBuffer(lpidBuffer, bufferSize);
      }
      if (NULL != pidBuffer)
      {
         context->releaseBuffer(pidBuffer, bufferSize);
      }
      return rc;
   error:
      goto done;
   }

   void logicalPageSpace::releasePreallocated(requestContext *context,
                                              UINT32 count,
                                              const mappedLogicalPageId *mpids)
   {
      SDB_ASSERT(isOpen(), "can not be closed");
      SDB_ASSERT(NULL != context, "can not be null");
      SDB_ASSERT(0 < count, "can not be zero");
      SDB_ASSERT(NULL != mpids, "can not be null");
      UINT32 bufferSize = 0;
      CHAR *buffer = NULL;

      if (1 == count)
      {
         SDB_ASSERT(mpids[0].isValid(), "must be valid");
         PAGE_ID lpid = mpids->getLpid();
         releaseLpidsPreallocated(context, 1, &lpid);
         _dpc->releasePage(mpids[0].getPid());
      }
      else
      {
         UINT32 bufferSize = count * sizeof(PAGE_ID);
         CHAR *buffer = context->allocateBuffer(bufferSize);
         if (OSS_UNLIKELY(NULL == buffer))
         {
            PD_LOG(PDSEVERE, "failed to allocate mem, releasing terminated!");
            goto done;
         }

         for (UINT32 i = 0; i < count; ++i)
         {
            ((PAGE_ID *)buffer)[i] = mpids[i].getLpid();
         }
         releaseLpidsPreallocated(context, count, (const PAGE_ID *)buffer);

         for (UINT32 i = 0; i < count; ++i)
         {
            ((PAGE_ID *)buffer)[i] = mpids[i].getPid();
         }
         _dpc->releasePages(count, (const PAGE_ID *)buffer);
      }
   done:
      if (NULL != buffer)
      {
         context->releaseBuffer(buffer, bufferSize);
      }
      return;
   }

   INT32 logicalPageSpace::preallocateLpids(requestContext *context,
                                            UINT32 count,
                                            PAGE_ID *lpids)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(_allocator.isInitialized(), "must be inited");
      SDB_ASSERT(NULL != context, "can not be null");
      SDB_ASSERT(0 < count, "can not be zero");
      SDB_ASSERT(NULL != lpids, "can not be null");
      
      rc = _allocator.allocateBits(count, lpids, 1);
      if (SDB_OK == rc)
      {
         goto done;
      }
      else if (SDB_VESSEL_OUT_OF_RESOURCE == rc)
      {
         rc = SDB_VESSEL_FS_UPPER_LIMIT;
         goto error;
      }
      else
      {
         PD_LOG(PDERROR, "failed to allocate from bitmap:%d", rc);
         goto error;
      }
   done:
      return rc;
   error:
      goto done;
   }

   void logicalPageSpace::releaseLpidsPreallocated(requestContext *context,
                                                   UINT32 count,
                                                   const PAGE_ID *lpids)
   {
      SDB_ASSERT(_allocator.isInitialized(), "must be inited");
      SDB_ASSERT(NULL != context, "can not be null");
      SDB_ASSERT(0 < count, "can not be zero");
      SDB_ASSERT(NULL != lpids, "can not be null");

      UINT32 reservedImpCount = getReservedImpCount();

      for (UINT32 i = 0; i < count; ++i)
      {
         SDB_ASSERT(INVALID_PAGE_ID != lpids[i], "can not be invalid");
         PAGE_ID imp = getImpPidOfLpid(lpids[i]);
         SDB_ASSERT(INVALID_PAGE_ID != imp, "can not be invalid");
         SDB_ASSERT(reservedImpCount <= imp, "impossible");
      }

      _allocator.releaseBits(count, lpids);
   done:
      return;
   }

   INT32 logicalPageSpace::createFirstIdMapFile(requestContext *context,
                                                const createLogicalPageSpaceOptions &o)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(NULL != context, "can not be null");
      SDB_ASSERT(INVALID_SPACE_ID != _sid, "can not be invalid");
      SDB_ASSERT(o.isValid(), "can not be invalid");
      SDB_ASSERT(_idMapFiles.isEmpty(), "must be empty");

      vesselFileName fn;
      createStorageFileOptions options;
      storageUnit *su = context->getEnv()->dms.getStorageUnit(getSpaceID());
      SDB_ASSERT(NULL != su, "can not be null");
      idMapFile *file = NULL;
      storageCoreArgs args(ID_MAP_FILE_PAGE_SIZE,
                           ID_MAP_FILE_MAX_PAGE_COUNT_IN_SEG,
                           ID_MAP_FILE_MAX_SEG_COUNT_IN_FILE);

      idMapFileHead imfHead;
      imfHead.version = ID_MAP_FILE_HEAD_VERSION;
      imfHead.flags = getIdMapFileHeadFlags();
      imfHead.dataPageSize = o.dataArgs.pageSize;
      imfHead.dataPageCountInSeg = o.dataArgs.maxPageCountPerSeg;
      imfHead.dataSegCountInFile = o.dataArgs.maxSegmentCountPerFile;

      slice hs(sizeof(idMapFileHead), &imfHead);

      if (!fn.build(_sid, FILE_TYPE_ID_MAP,
                    getSpaceType(), 0))
      {
         PD_LOG(PDERROR, "failed to build file name");
         rc = SDB_VESSEL_INTERNAL_ERR;
         goto error;
      }

      file = SDB_OSS_NEW idMapFile();
      if (OSS_UNLIKELY(NULL == file))
      {
         PD_LOG(PDERROR, "failed to allocate mem");
         rc = SDB_OOM;
         goto error;
      }

      options.secretValue = o.secretValue;
      options.args = args;
      options.createAsTmpFile = TRUE;
      options.replaceWhenCreate = TRUE;
      rc = su->createStorageFile(fn, options, hs, file);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to create id map file[%s], rc:%d", fn.getFileName(), rc);
         goto error;
      }

      rc = file->fsync();
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to fsync file:%d", rc);
         goto error;
      }

      rc = file->removeShadowSuffix();
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to remove file's shadow suffix:%d", rc);
         goto error;
      }

      _idMapFiles.pushBack(file);
      file = NULL;

   done:
      return rc;
   error:
      if (NULL != file)
      {
         file->destroy();
         SDB_OSS_DEL file;
      }
      goto done;
   }

   INT32 logicalPageSpace::rebaseWhenCreatingCheckpoint(requestContext *context,
                                                        const LPS_CHECKPOINT &checkpoint,
                                                        UINT32 totalImpCount)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(isOpen(), "can not be closed");
      SDB_ASSERT(NULL != context, "can not be null");
      SDB_ASSERT(0 < totalImpCount, "impossible");
      SDB_ASSERT(checkpoint.isValid(), "can not be invalid");

      UINT32 segmentCount = 0;
      idMapFile *base = _idMapFiles.getBack<idMapFile>();
      SDB_ASSERT(NULL != base, "can not be null");
      idMapFile *file = NULL;
      storageCoreArgs args(ID_MAP_FILE_PAGE_SIZE,
                           ID_MAP_FILE_MAX_PAGE_COUNT_IN_SEG,
                           ID_MAP_FILE_MAX_SEG_COUNT_IN_FILE);
      idMapFileHead imfHead;
      imfHead.version = ID_MAP_FILE_HEAD_VERSION;
      imfHead.flags = getIdMapFileHeadFlags();
      imfHead.dataPageSize = getStorageCoreArgs().pageSize;
      imfHead.dataPageCountInSeg = getStorageCoreArgs().maxPageCountPerSeg;
      imfHead.dataSegCountInFile = getStorageCoreArgs().maxSegmentCountPerFile;
      imfHead.totalPageCount = totalImpCount;
      imfHead.checkpoint = checkpoint;
      slice hs(sizeof(idMapFileHead), &imfHead);

      createStorageFileOptions o;
      o.args = args;
      o.createAsTmpFile = TRUE;
      o.replaceWhenCreate = TRUE;
      o.secretValue = base->getCommonHeadInMem().secretValue;

      vesselFileName fn;

      storageUnit *su = context->getEnv()->dms.getStorageUnit(getSpaceID());
      SDB_ASSERT(NULL != su, "can not be null");

      file = SDB_OSS_NEW idMapFile();
      if (OSS_UNLIKELY(NULL == file))
      {
         PD_LOG(PDERROR, "failed to allocate mem");
         rc = SDB_OOM;
         goto error;
      }

      if (!fn.build(getSpaceID(), FILE_TYPE_ID_MAP,
                    getSpaceType(), base->getCommonHeadInMem().sequence + 1))
      {
         PD_LOG(PDERROR, "failed to build file name");
         rc = SDB_VESSEL_INTERNAL_ERR;
         goto error;
      }

      rc = su->createStorageFile(fn, o, hs, file);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to create new file:%s, rc:%d", fn.getFileName(), rc);
         goto error;
      }

      segmentCount = ossAlignX(totalImpCount, ID_MAP_FILE_MAX_PAGE_COUNT_IN_SEG) /
                     ID_MAP_FILE_MAX_PAGE_COUNT_IN_SEG;

      rc = file->ensureSegmentCountAndInit(segmentCount);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to extend new id map file:%d", rc);
         goto error;
      }

      rc = base->copySemgmentsTo(file);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to copy segments to file:%s, rc:%d",
                file->getFullPath(), rc);
         goto error;
      }

      rc = _lpidCache.flushPreparedCacheToFile(file);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to flush lpid cache:%d", rc);
         goto error;
      }

      rc = file->fsync();
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to fsync file:%d", rc);
         goto error;
      }

      rc = file->removeShadowSuffix();
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to remvoe file suffix:%d", rc);
         goto error;
      }

      _idMapFiles.pushBack(file);
      _lpidCache.resetBaseFileAndClearFlushedMaps(file);

      _idMapFiles.truncate(3);
      rc = _logConsole.rebase(context, file->getCommonHeadInMem().sequence);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to rebase delta log:%d", rc);
         ossPanic(); /// impossible to be failed.
         goto error;
      }
      
   done:
      return rc;
   error:
      if (NULL != file)
      {
         file->destroy();
         SDB_OSS_DEL file;
      }
      goto done;
   }



   INT32 logicalPageSpace::restoreAllocatorByBaseFile(requestContext *context,
                                                      const idMapFile *base)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(_allocator.isInitialized(), "must be inited");
      SDB_ASSERT(NULL != base, "can not be null");
      UINT32 pageCount = 0;
      rc = base->getTotalPageCount(pageCount);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to get page count in id map file:%d", rc);
         goto error;
      }

      if (0 == pageCount)
      {
         goto done;
      }

      rc = _allocator.ensureBitmapPageCount(pageCount);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to reserve space from allocator:%d", rc);
         goto error;
      }
      
      for (UINT32 i = 0; i < pageCount; ++i)
      {
         ossValuePtr ptr = 0;
         rc = base->getPagePtr(i, ptr);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to get imp[%d] ptr:%d", i, rc);
            goto error;
         }
         rc = restoreAllocatorByImp(context, i, (const CHAR *)ptr);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to restore allocator by imp[%d], rc:%d",
                   i, rc);
            goto error;
         }
      }
   done:
      return rc;
   error:
      goto done;
   }

   INT32 logicalPageSpace::mergeAndRestoreAllocator(requestContext *context)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(_lpidCache.isReady(), "can not be invalid");
      dataPageCluster *dpc = getDataStorageObj();
      SDB_ASSERT(NULL != dpc && dpc->isOpen(), "can not be invalid");
      const idMapFile *base = _idMapFiles.getBack<idMapFile>();
      UINT32 basePageCount = 0;
      base->getTotalPageCount(basePageCount);
      SDB_ASSERT(NULL != base, "can not be null");
      const partialImpCacheMap &delta = _lpidCache.getImmutableMap();
      memoryBlock mb;
      partialImpCacheMap::CACHE_MAP::const_iterator deltaIterator;
      partialImpCacheMap::KEY deltaLowKey;

      if (0 < basePageCount)
      {
         rc = mb.reserve(ID_MAP_FILE_PAGE_SIZE);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to reserve mb size:%d", rc);
            goto error;
         }

         SDB_ASSERT(getReservedImpCount() <= basePageCount, "impossible");

         for (UINT32 i = 0; i < basePageCount; ++i)
         {
            partialImpCacheMap::CACHE_MAP::const_iterator lower;
            partialImpCacheMap::CACHE_MAP::const_iterator upper;
            partialImpCacheMap::KEY lowKey = std::make_pair(i, 0);
            partialImpCacheMap::KEY upKey = std::make_pair(i, ID_MAP_FILE_PAGE_SIZE);

            slice buffer;
            buffer.makeWritable(ID_MAP_FILE_PAGE_SIZE, mb.getBuffer());
            ossValuePtr ptr;
            rc = base->getPagePtr(i, ptr);
            if (SDB_OK != rc)
            {
               PD_LOG(PDERROR, "failed to get page[%d] ptr:%d", i, rc);
               goto error;
            }

            buffer.write(0, ID_MAP_FILE_PAGE_SIZE, (const CHAR *)ptr);

            lower = delta.get().lower_bound(lowKey);
            upper = delta.get().upper_bound(upKey);
            for (; lower != upper; ++lower)
            {
               SDB_ASSERT(partialImpCacheMap::isValidKey(lower->first), "can not be invalid");
               rc = buffer.write(lower->first.second,
                                 ID_MAP_PARTIAL_PAGE_CACHE_SIZE,
                                 lower->second->getBuffer());
               if (SDB_OK != rc)
               {
                  PD_LOG(PDERROR, "failed to copy data to[%d,%d]",
                         lower->first.second, ID_MAP_PARTIAL_PAGE_CACHE_SIZE);
                  goto error;
               }
            }

            rc = restoreAllocatorByImp(context, i, buffer.data());
            if (SDB_OK != rc)
            {
               PD_LOG(PDERROR, "failed to retore imp[%d], rc:%d", i, rc);
               goto error;
            }
         }
      }
      
      /// restore others
      deltaLowKey.first = basePageCount;
      deltaLowKey.second = 0;
      deltaIterator = delta.get().lower_bound(deltaLowKey);
      for (; deltaIterator != delta.end(); ++deltaIterator)
      {
         const partialImpCacheMap::KEY &key = deltaIterator->first;
         SDB_ASSERT(partialImpCacheMap::isValidKey(key), "can not be invalid");
         const partialImpCache *cache = deltaIterator->second;
         SDB_ASSERT(NULL != cache, "can not be null");
         PAGE_ID baseLpid = (key.first * ID_MAP_PAGE_CAPACITY) +
                            ((key.second / ID_MAP_PARTIAL_PAGE_CACHE_SIZE) *
                             ID_MAP_PARTIAL_CACHE_SLOT_COUNT); 

         rc = _allocator.ensureBitmapPageCount(key.first + 1);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to ensure allocator space:%d", rc);
            goto error;
         }

         for (UINT32 pos = 0; pos < ID_MAP_PARTIAL_CACHE_SLOT_COUNT; ++pos)
         {
            BOOLEAN isMutable = FALSE;
            PAGE_ID lpid = baseLpid + pos;
            const idMapSlot *slot = cache->get(pos, isMutable);
            if (slot->isFree())
            {
               continue;
            }

            if (getReservedImpCount() <= key.first)
            {
               /// not reserved, managed by allocator
               rc = _allocator.occupy(lpid);
               if (SDB_OK != rc)
               {
                  PD_LOG(PDERROR, "failed to occupy lpid[%d], rc:%d", lpid, rc);
                  goto error;
               }
            }

            rc = dpc->ensurePidSpace(context, slot->pid);
            if (SDB_OK != rc)
            {
               PD_LOG(PDERROR, "failed to ensure pid[%] space:%d", slot->pid, rc);
               goto error;
            }

            rc = dpc->occupyPage(context, slot->pid);
            if (SDB_OK != rc)
            {
               PD_LOG(PDERROR, "failed to occupy pid[%d], rc:%d", slot->pid, rc);
               goto error;
            }
      
         }
      }
   done:
      return rc;
   error:
      goto done;
   }

   INT32 logicalPageSpace::restoreAllocatorByImp(requestContext *context, 
                                                 PAGE_ID impPid,
                                                 const CHAR *page)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(INVALID_PAGE_ID != impPid, "can not be invalid");
      SDB_ASSERT(_allocator.isInitialized(), "can not be invalid");
      dataPageCluster *dpc = getDataStorageObj();
      SDB_ASSERT(NULL != dpc, "can not be invalid");
      UINT32 baseLpid = impPid * ID_MAP_PAGE_CAPACITY;

      BOOLEAN isImpReserved = impPid < getReservedImpCount();

      for (UINT32 i = 0; i < ID_MAP_PAGE_CAPACITY; ++i)
      {
         PAGE_ID lpid = baseLpid + i;
         idMapSlot slot = getIdMapSlot((ossValuePtr)page, i);
         if (slot.isFree())
         {
            continue;
         }

         if (!isImpReserved)
         {
            rc = _allocator.occupy(lpid);
            if (SDB_OK != rc)
            {
               PD_LOG(PDERROR, "failed to occupy lpid[%d], rc:%d", lpid, rc);
               goto error;
            }
         }

         rc = _dpc->ensurePidSpace(context, slot.pid);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to ensure pid[%d] space:%d", slot.pid, rc);
            goto error;
         }

         rc = _dpc->occupyPage(context, slot.pid);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to occupy pid[%d] in storage, rc:%d",
                   slot.pid, rc);
            goto error;
         }
      }
   done:
      return rc;
   error:
      goto done;
   }

   INT32 logicalPageSpace::ensureLogicalPidSpace(PAGE_ID lpid)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(_allocator.isInitialized(), "must be inited");
      SDB_ASSERT(INVALID_PAGE_ID != lpid, "can not be invalid");
      UINT32 minCount = getImpPidOfLpid(lpid) + 1;
      if (minCount <= _allocator.getCustomizedPageCount())
      {
         goto done;
      }
      rc = _allocator.ensureBitmapPageCount(minCount);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to ensure bitmap page count:%d", rc);
         goto error;
      }
   done:
      return rc;
   error:
      goto done;
   }


   BOOLEAN logicalPageSpace::isReservedLpid(PAGE_ID lpid)const
   {
      SDB_ASSERT(INVALID_PAGE_ID != lpid, "can not be invalid");
      return lpid < (getReservedImpCount() * ID_MAP_PAGE_CAPACITY);
   }

   INT32 logicalPageSpace::validateLpidBeforeGet(PAGE_ID lpid)const
   {
      INT32 rc = SDB_OK;

      if (OSS_UNLIKELY(INVALID_PAGE_ID == lpid))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      else if (OSS_UNLIKELY(MAX_LPID_COUNT <= lpid))
      {
         rc = SDB_OUT_OF_BOUND;
         goto error;
      }
      else if ((_allocator.getCustomizedPageCount() * ID_MAP_PAGE_CAPACITY) <= lpid)
      {
         rc = SDB_VESSEL_LOGICAL_PAGE_UNMAPPED;
         goto error;
      }

   done:
      return rc;
   error:
      goto done;
   }

   INT32 logicalPageSpace::getIdMapSlotFromCache(PAGE_ID lpid,
                                                 idMapSlot &slot,
                                                 BOOLEAN &isMutable)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(INVALID_PAGE_ID != lpid, "can not be invalid");
      rc = _lpidCache.get(lpid, slot, isMutable);
      if (SDB_OK != rc)
      {
         goto error;
      }

      if (!isCopyOnWrite())
      {
         isMutable = TRUE;
      }
   done:
      return rc;
   error:
      goto done;
   }

   INT32 logicalPageSpace::flushSegmentsAtCheckpoint(requestContext *context,
                                                     const ossPoolSet<UINT32> &segments)const
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(NULL != context, "can not be null");
      SDB_ASSERT(NULL != _dpc, "can not be null");
      SDB_ASSERT(_dpc->isOpen(), "can not be closed");
      SDB_ASSERT(isCopyOnWrite(), "impossible");

      static const UINT32 _DISPATCH_FLUSHING_TASK_THRESHOLD = 4;
      
      UINT32 totalCount = segments.size();
      //UINT32 failureCount = 0;
      autoEventList<backgroundEvent> rl;
      backgroundWorkers &workers = context->getEnv()->workers;

      PD_LOG(PDDEBUG, "segments[%d] to be flushed", segments.size());

      if (totalCount <= _DISPATCH_FLUSHING_TASK_THRESHOLD || !workers.isReady())
      {
         for (ossPoolSet<UINT32>::const_iterator itr = segments.begin();
              itr != segments.end(); ++itr)
         {
            rc = _dpc->fsyncSegment(*itr);
            if (SDB_OK != rc)
            {
               PD_LOG(PDSEVERE, "failed to flush global segment[%d, %d, %d], rc:%d",
                     getSpaceID(), getSpaceType(), *itr, rc);
               //++failureCount;
            }
         }
      }
      else
      {
         UINT32 taskDispatched = 0;
         UINT32 firstSegment = 0;
         UINT32 batchCount = 0;
         ossPoolSet<UINT32>::const_iterator itr = segments.begin();

         do
         {
            if (0 == batchCount)
            {
               batchCount = 1;
               firstSegment = *itr;
            }
            else if ((firstSegment + batchCount) != *itr ||
                     batchCount == _DISPATCH_FLUSHING_TASK_THRESHOLD)
            {
               backgroundEvent event;
               lpsFlushingSegments msg;
               msg._sid = getSpaceID();
               msg._type = getSpaceType();
               msg._segmentId = firstSegment;
               msg._count = (UINT8)batchCount;
               event.setType(backgroundEvent::EVENT_TYPE_SYNC_SEG);
               event.setEventMsg(sizeof(lpsFlushingSegments), &msg);
               event.setResponseList(&rl);
               workers.pushEvent(event);
               ++taskDispatched;

               firstSegment = *itr;
               batchCount = 1;
            }
            else
            {
               ++batchCount;
            }
         } while (++itr != segments.end());

         PD_LOG(PDDEBUG, "[%d] tasks dispatched, [%d] segments remained when flushing[%d,%d]",
                taskDispatched, batchCount, getSpaceID(), getSpaceType());

         for (UINT32 i = 0; i < batchCount; ++i)
         {
            rc = _dpc->fsyncSegment(firstSegment + i);
            if (SDB_OK != rc)
            {
               PD_LOG(PDSEVERE, "failed to flush global segment[%d, %d, %d], rc:%d",
                     getSpaceID(), getSpaceType(), *itr, rc);
               //++failureCount;
            }
         }

         while (0 < taskDispatched)
         {
            backgroundEvent event;
            rl.popOrWait(event);
            SDB_ASSERT(event.getType() == backgroundEvent::EVENT_TYPE_FINISHED,
                       ", must be finish");
            --taskDispatched;
         }
      }
   done:
      return rc;
   error:
      goto done;
   }

   void logicalPageSpace::applyCheckpointIfNecessary(requestContext *context)
   {
      SDB_ASSERT(isOpen(), "can not be invalid");
      SDB_ASSERT(NULL != context && context->isOpen(), "can not be invalid");

      UINT64 dirtySize = (UINT64)(_lpidCache.estimateMutablePageCount()) *
                         getStorageCoreArgs().pageSize;
      if (CHECKPOINT_TRIGGER_DIRTY_PAGE_SIZE <=
          dirtySize)
      {
         if (_checkpointContext.tryToApplyCheckpoint())
         {
            PD_LOG(PDDEBUG, "lps[%d, %d] estimated dirty size:%lld",
                   getSpaceID(), getSpaceType(), dirtySize);
            backgroundEvent event;
            lpsCheckpointApplying msg;
            msg._sid = getSpaceID();
            msg._type = getSpaceType();
            event.setType(backgroundEvent::EVENT_TYPE_LPS_CHECKPOINT);
            event.setEventMsg(sizeof(lpsCheckpointApplying), &msg);
            context->getEnv()->workers.pushEvent(event);
         }
      }

      return;
   }

   BOOLEAN logicalPageSpace::needFullCheckpoint()
   {
      SDB_ASSERT(_lpidCache.isReady(), "can not be invalid");
      const idMapFile *base = _idMapFiles.getBack<idMapFile>();
      SDB_ASSERT(NULL != base, "can not be null");
      UINT64 baseSize = base->getTotalSegmentSize();
      UINT64 cacheSize = _lpidCache.getTotalCacheSize();
      UINT64 deltaLogSize = _logConsole.getDeltaLogSize();
      PD_LOG(PDDEBUG, "lps[%d,%d] cache size:%lld, base size:%lld, delta log:%lld",
             getSpaceID(), getSpaceType(), cacheSize, baseSize, deltaLogSize);
      return FULL_CHECKPOINT_LPID_CACHE_SIZE <= cacheSize ||
             FULL_CHECKPOINT_DELTA_LOG_SIZE <= deltaLogSize ||
             (baseSize * 0.6) <= cacheSize;
   }

   INT32 logicalPageSpace::restoreToLatestCheckpoint(requestContext *context)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(NULL != context, "can not be null");
      idMapFile *base = _idMapFiles.getBack<idMapFile>();
      SDB_ASSERT(NULL != base, "base id map file not found");
      SDB_ASSERT(_logConsole.isReady(), "must be ready");
      SDB_ASSERT(_lpidCache.isReady(), "can not be ready");

      idMapFileHead baseHead;
      rc = base->getIdMapFileHead(baseHead);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to get base file head:%d", rc);
         goto error;
      }

      if (!_logConsole.hasDeltaLog())
      {
         PD_LOG(PDINFO, "no delta log records found, restore by base file[%s]",
                 base->getFullPath());
         if (0 < base->getCommonHeadInMem().sequence)
         {
            rc = restoreAllocatorByBaseFile(context, base);
            if (SDB_OK != rc)
            {
               PD_LOG(PDERROR, "failed to restore allocator by base file:%d", rc);
               goto error;
            }
            _checkpointContext.setCheckpoint(baseHead.checkpoint);
         }
      }
      else
      {
         SDB_ASSERT(_logConsole.hasOnlineFile(), "impossible");
         
         deltaLogFile *delta = _logConsole.getOnlineFile();
         SDB_ASSERT(delta->getCommonHeadInMem().sequence ==
                    base->getCommonHeadInMem().sequence, "must be same");
         PD_LOG(PDINFO, "merge base[%s] and delta[%s] to restore lps",
                base->getFullPath(), delta->getFullPath());
         deltaLogCheckpointRecord cr;
         UINT32 count = _logConsole.getValidSegmentCount();
         for (UINT32 i = 0; i < count; ++i)
         {
            slice batch = _logConsole.getDumpedRecord(i, &cr);
            SDB_ASSERT(!batch.isEmpty(), "impossible");
            for (UINT32 pos = 0; pos < cr.elementCount; ++pos)
            {
               const deltaLogDumpRecord *dump =
                         batch.getReadableObjPtr<deltaLogDumpRecord>(pos * DELTA_LOG_DUMP_RECORD_SIZE);
               if (OSS_UNLIKELY(NULL == dump))
               {
                  PD_LOG(PDERROR, "failed to read dump log in batch at seg[%d", i);
                  rc = SDB_VESSEL_INTERNAL_ERR;
                  goto error;
               }

               rc = _lpidCache.upsertWhenRestore(dump);
               if (SDB_OK != rc)
               {
                  PD_LOG(PDERROR, "failed to upsert cache at seg[%d], rc:%d", i, rc);
                  goto error;
               }
            }
         }

         SDB_ASSERT(cr.isEnd(), "must be end");
         SDB_ASSERT(cr.checkpoint.isValid(), "must be valid");

         rc = mergeAndRestoreAllocator(context);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to merge base and delta:%d", rc);
            goto error;
         }
         _checkpointContext.setCheckpoint(cr.checkpoint);
      }
   done:
      return rc;
   error:
      goto done;
   }
   
}//namespace vessel
}//namespace engine