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
   constexpr UINT64 FULL_CHECKPOINT_DELTA_LOG_SIZE = 64ull * 1024 * 1024; // 64MB
   constexpr UINT64 CHECKPOINT_TRIGGER_DIRTY_PAGE_SIZE = 1024ull * 1024 * 1024; // 1GB

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
      /// TODO: move to sub class
      _dpc = getDataStorageObj();
      if (NULL == _dpc)
      {
         PD_LOG(PDERROR, "failed to get storage object");
         rc = SDB_VESSEL_INTERNAL_ERR;
         goto error;
      }

      rc = _dpc->open(_sid, getSpaceType(),
                      baseFile->getCommonHeadInMem().secretValue,
                      NULL, o.dataArgs,
                      getStorageOptions());
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
      /// TODO: move to sub class
      _dpc = getDataStorageObj();
      if (NULL == _dpc)
      {
         PD_LOG(PDERROR, "failed to allocate page storage object");
         rc = SDB_VESSEL_INTERNAL_ERR;
         goto error;
      }

      rc = _dpc->open(_sid, getSpaceType(),
                      base->getCommonHeadInMem().secretValue,
                      &loader, dataArgs,
                      getStorageOptions());
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

      rc = resumeToLatestCheckpoint(context);
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
      ossPoolSet<UINT32> segments;
      idMapFile *base = _idMapFiles.getBack<idMapFile>();
      SDB_ASSERT(NULL != base, "can not be null");
      DPS_LSN_OFFSET pushLSN = DPS_INVALID_LSN_OFFSET;
      DPS_LSN_OFFSET minDirtyLSN = DPS_INVALID_LSN_OFFSET;
      DPS_LSN_OFFSET maxDirtyLSN = DPS_INVALID_LSN_OFFSET;

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

      minDirtyLSN = _checkpointContext.getMinDirtyLsn();
      maxDirtyLSN = _checkpointContext.getMaxDirtyLsn();
      pushLSN = isCopyOnWrite() ? logger->getCurrentLSN() : maxDirtyLSN;
      
      rc = _logConsole.reserveCheckpoint(context);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to reserve checkpoint in log console:%d", rc);
         goto error;
      }

      lsn._lsn = logger->getCurrentLSN();
      lsn._minDirtyLSN = minDirtyLSN;
      rc = getMinUncompletedLSN(context, lsn._minUncompletedLSN);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to get min uncompleted lsn:%d", rc);
         goto error;
      }

      if (isCopyOnWrite())
      {
         extractDirtySegmentsInCache(segments);
      }

      _lpidCache.setAllPagesImmutable();
      mergeWaitingListIntoReadyList();

      SDB_ASSERT(lsn.isValid(), "can not be invalid");
      _checkpointContext.clearDirtyLSN();
      _checkpointContext.getLatch()->release_w();
      locked = FALSE;

      if (isCopyOnWrite() && !segments.empty())
      {
         rc = flushSegmentsAtCheckpoint(context, segments);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to flush segments:%d", rc);
            goto error;
         }
      }

      logger->pushMaxFileLSN(context->getExecutor(), pushLSN);

      checkpoint.init(0, lsn, _checkpointContext.getCheckpoint().sequence + 1,
                      ossGetCurrentMilliseconds());
      _logConsole.commit(checkpoint);

      freePidsInReadyList();

      _checkpointContext.setCheckpoint(checkpoint);
      _checkpointContext.setStatus(lpsCheckpointContext::STATUS::ENDING);
      
      endToCreateCheckpoint(context);
      /// TOOD: update global status here

      _checkpointContext.setStatus(lpsCheckpointContext::STATUS::NONE);
      abortCheckpoint = FALSE;

      PD_LOG(PDINFO, "end to create checkpoint (delta) on lps[%d,%d], :%s",
             getSpaceID(), getSpaceType(),
             _checkpointContext.getCheckpoint().toString().c_str());
      
   done:
      if (locked)
      {
         _checkpointContext.getLatch()->release_w();
      }
      if (abortCheckpoint)
      {
         _checkpointContext.setStatus(lpsCheckpointContext::STATUS::NONE);
      }
      return rc;
   error:
      if (DPS_INVALID_LSN_OFFSET != minDirtyLSN)
      {
         _mappingLatch.get();
         _checkpointContext.updateDirtyLsn(minDirtyLSN);
         _checkpointContext.updateDirtyLsn(maxDirtyLSN);
         _mappingLatch.release();
      }
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
      DPS_LSN_OFFSET minDirtyLSN = DPS_INVALID_LSN_OFFSET;
      DPS_LSN_OFFSET maxDirtyLSN = DPS_INVALID_LSN_OFFSET;

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

      minDirtyLSN = _checkpointContext.getMaxDirtyLsn();
      maxDirtyLSN = _checkpointContext.getMaxDirtyLsn();
      pushLSN = isCopyOnWrite() ? logger->getCurrentLSN() : maxDirtyLSN;

      lsn._lsn = logger->getCurrentLSN();
      lsn._minDirtyLSN = minDirtyLSN;
      rc = getMinUncompletedLSN(context, lsn._minUncompletedLSN);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to get min uncompleted lsn:%d", rc);
         goto error;
      }

      if (isCopyOnWrite())
      {
         extractDirtySegmentsInCache(segments);  
      }

      _lpidCache.setAllPagesImmutable();
      mergeWaitingListIntoReadyList();

      _logConsole.rebase(context, _logConsole.getBaseSequence() + 1, FALSE);

      _checkpointContext.clearDirtyLSN();
      _checkpointContext.getLatch()->release_w();
      locked = FALSE;

      if (isCopyOnWrite() && !segments.empty())
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
                      _checkpointContext.getCheckpoint().sequence + 1,
                      ossGetCurrentMilliseconds());

      _checkpointContext.setStatus(lpsCheckpointContext::STATUS::CREATING_NEW_BASE);
      rc = rebaseWhenCreatingCheckpoint(context, checkpoint);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to rebase id map file:%d", rc);
         goto error;
      }

      freePidsInReadyList();

      _checkpointContext.setCheckpoint(checkpoint);
      _checkpointContext.setStatus(lpsCheckpointContext::STATUS::ENDING);
      endToCreateCheckpoint(context);
      /// TOOD: update global status here

      _checkpointContext.setStatus(lpsCheckpointContext::STATUS::NONE);
      abortCheckpoint = FALSE;

      PD_LOG(PDINFO, "end to create checkpoint (full) on lps[%d,%d], :%s",
            getSpaceID(), getSpaceType(),
            _checkpointContext.getCheckpoint().toString().c_str());
   done:
      if (locked)
      {
         _checkpointContext.getLatch()->release_w();
      }
      if (abortCheckpoint)
      {
         _checkpointContext.setStatus(lpsCheckpointContext::STATUS::NONE);
      }
      return rc;
   error:
      if (DPS_INVALID_LSN_OFFSET != minDirtyLSN)
      {
         _mappingLatch.get();
         _checkpointContext.updateDirtyLsn(minDirtyLSN);
         _checkpointContext.updateDirtyLsn(maxDirtyLSN);
         _mappingLatch.release();
      }
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

   void logicalPageSpace::waitCheckpoint()
   {
      SDB_ASSERT(isOpen(), "can not be closed");
      _checkpointContext.getLatch()->lock_r();
      while (_checkpointContext.isRunning())
      {
         ossSleepmillis(100);
      }
      _checkpointContext.getLatch()->release_r();
      return;
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
      BOOLEAN locked = FALSE;

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

      rc = context->lockLpid(getSpaceType(), lpid, mode);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to lock lpid[%d], rc:%d", lpid, rc);
         goto error;
      }
      locked = TRUE;

      rc = _lpidCache.get(lpid, slot, isMutablePage);
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

      lpb._lpid = lpid;
      lpb._mode = mode;
      lpb._context = context;
      lpb._lps = this;
      lpb._cowTrigger.reset(slot.psv, isMutablePage);

   done:
      return rc;
   error:
      if (lpb._rpb.isValid())
      {
         lpb._rpb.fini();
      }
      if (locked)
      {
         context->unlockLpid(getSpaceType(), lpid);
      }
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
      BOOLEAN locked = FALSE;

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

      rc = context->tryLockLpid(getSpaceType(), lpid, mode, locked);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to lock lpid[%d], rc:%d", lpid, rc);
         goto error;
      }
      else if (!locked)
      {
         goto done;
      }

      rc = _lpidCache.get(lpid, slot, isMutablePage);
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

      lpb._lpid = lpid;
      lpb._mode = mode;
      lpb._context = context;
      lpb._lps = this;
      lpb._cowTrigger.reset(slot.psv, isMutablePage);
   done:
      return rc;
   error:
      if (lpb._rpb.isValid())
      {
         lpb._rpb.fini();
      }
      if (locked)
      {
         context->unlockLpid(getSpaceType(), lpid);
      }
      goto done;
   }

   INT32 logicalPageSpace::isLogicalPageMapped(requestContext *context,
                                                PAGE_ID lpid,
                                                BOOLEAN &mapped)
   {
      INT32 rc = SDB_OK;
      BOOLEAN isMutablePage = FALSE;
      idMapSlot slot;
      BOOLEAN locked = FALSE;
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
         goto done;
      }
      else if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "can not validate lpid[%d] before get, rc:%d", lpid, rc);
         goto error;
      }

      if (!context->testLpidLocked(getSpaceType(), lpid, NULL))
      {
         ossSharedLatchMode mode(OSS_SHARED_LATCH_MODE_ENUM_SHARED);
         rc = context->lockLpid(getSpaceType(), lpid, mode);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to lock lpid[%d], rc:%d", rc);
            goto error;
         }
         locked = TRUE;
      }

      rc = _lpidCache.get(lpid, slot, isMutablePage);
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
      if (locked)
      {
         context->unlockLpid(getSpaceType(), lpid);
      }
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
      requestContext *context = lpb._context;

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
      else if (OSS_UNLIKELY(!lpb._mode.isExclusiveOrUpgrade()))
      {
         rc = SDB_VESSEL_FORBIDDEN_OP_WLT;
         goto error;
      }
      else if (rpb.isWritingPrepared())
      {
         goto done;
      }

      if (lpb._mode.isUpgrade())
      {
         lpb.lockExclusiveFromUpgrade();
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
      }
      else if (isCopyOnWrite() && !lpb.getCowTrigger().isMutablePid())
      {
         rc = remapBufferToNewDataPage(context, TRUE, lpb);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to remap lpid[%d], rc:%d",
                   lpb.getLogicalPid(), rc);
            goto error;
         }
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
      rc = _dpc->allocatePage(pid);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to allocate page from page cluster:%d", rc);
         goto error;
      }

      mpid.reset(lpid, pid);
      rc = initAndMapPages(context, initer, LPID_MAPPING_ARRAY(&mpid, 1));
      if (SDB_OK != rc)
      {
         _dpc->releasePage(pid);
         PD_LOG(PDERROR, "failed to init page:%d", rc);
         goto error;
      }
   done:
      return rc;
   error:
      goto done;
   }


   INT32 logicalPageSpace::initAndMapPages(requestContext *context,
                                           pageInitializer *initer,
                                           const LPID_MAPPING_ARRAY &mapping)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(NULL != context, "can not be null");
      SDB_ASSERT(NULL != initer, "can not be null");
      SDB_ASSERT(mapping.isValid(), "can not be invalid");
      runtimePageBuffer rpb;
      PAGE_SNAPSHOT_VERION psv = context->getEnv()->dms.getOnlinePageSnapshotVersion();
      
      /// TODO: Should we block checkpoint if this is not
      /// a cow lps to ensure lps's checkpoint contains all
      /// repl log?

      for (UINT32 i = 0; i < mapping.getSize(); ++i)
      {
         if (OSS_UNLIKELY(!mapping[i].isValid()))
         {
            PD_LOG(PDERROR, "invalid mapped id");
            rc = SDB_INVALIDARG;
            goto error;
         }

         rc = getRuntimePageBufferToReset(context, mapping[i].getPid(), rpb);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to get runtime buffer of page[%d], rc:%d",
                   mapping[i].getPid(), rc);
            goto error;
         }
         SDB_ASSERT(rpb.isWritingPrepared(), "must be prepared");

         rc = initer->initInTurns(context, i, mapping[i].getLpid(),
                                  psv, &rpb);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to init page[%d,%d], rc:%d",
                   mapping[i].getLpid(), mapping[i].getPid(), rc);
            goto error;
         }
         rpb.fini();
      }

      rc = map(context, psv, mapping);
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
      SDB_ASSERT(lpb._mode.isExclusive(), "must be exclusive");

      runtimePageBuffer &rpb = lpb._rpb;
      PAGE_SNAPSHOT_VERION psv = context->getEnv()->dms.getOnlinePageSnapshotVersion();
      PAGE_ID lpid = lpb._lpid;
      PAGE_ID pid = INVALID_PAGE_ID;
      PAGE_ID oldPid = rpb.getGlobalPid().page();
      mappedLogicalPageId mpid;

      /// 1. allocate new pid
      rc = _dpc->allocatePage(pid);
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
      rc = remap(context, psv,
                 LPID_MAPPING_ARRAY(&mpid, 1),
                 PID_ARRAY(&oldPid, 1),
                 releaseOld);
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
      LPID_MAPPING_ARRAY mapping;
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

      mapping = context->allocateArray<mappedLogicalPageId>(count);
      if (OSS_UNLIKELY(!mapping.isValid()))
      {
         PD_LOG(PDERROR, "failed to allocate mem");
         rc = SDB_OOM;
         goto error;
      }

      rc = reservePagesInMem(context, mapping);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to preallocate pages:%d", rc);
         goto error;
      }
      rollback = TRUE;

      rc = initAndMapPages(context, initer, mapping);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to init and map pages:%d", rc);
         goto error;
      }

      for (UINT32 i = 0; i < count; ++i)
      {
         lpids[i] = mapping[i].getLpid();
      }

   done:
      if (mapping.isValid())
      {
         context->releaseBuffer(mapping.data());
      }
      return rc;
   error:
      if (rollback)
      {
         freePagesInMem(context, mapping);
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

         INT32 rc = _lpidCache.get(lpid, slot, isMutable);
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
            LPID_MAPPING_ARRAY array(vec->data(), vec->size());
            unmap(context, array, releaseOld);
            vec->clear();
         }

         if (!isReservedLpid(lpid))
         {
            notReservedLpids.push_back(lpid);
         }
      }

      if (!underSnapshot.empty())
      {
         LPID_MAPPING_ARRAY array(underSnapshot.data(), underSnapshot.size());
         unmap(context, array, FALSE);
      }
      if (!notUnderSnapshot.empty())
      {
         LPID_MAPPING_ARRAY array(notUnderSnapshot.data(), notUnderSnapshot.size());
         unmap(context, array, TRUE);
      }
      if (!notReservedLpids.empty())
      {
         PID_ARRAY array(notReservedLpids.data(), notReservedLpids.size());
         freeLpidsInMem(context, array);
      }
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

      const STORAGE_FILE_NAME_LIST *list = loader.getFileList(getSpaceType(), 
                                                              FILE_TYPE_ID_MAP);
      if (NULL == list || list->empty())
      {
         PD_LOG(PDERROR, "id map file not found");
         rc = SDB_VESSEL_INVALID_VESSEL_FILE;
         goto error;
      }

      for (STORAGE_FILE_NAME_LIST::const_iterator itr = list->begin();
           itr != list->end(); ++itr)
      {
         const storageFileName &fn = *itr;
         if (!fn.isValid())
         {
            PD_LOG(PDERROR, "invalid file name");
            rc = SDB_INVALIDARG;
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
         if (fileInMap->getSequence() == base->getSequence())
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

   INT32 logicalPageSpace::reservePagesInMem(requestContext *context,
                                             LPID_MAPPING_ARRAY pages)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(isOpen(), "can not be closed");
      SDB_ASSERT(pages.isValid(), "can not be invalid");
      SDB_ASSERT(NULL != context, "can not be null");

      pages.fill();

      PID_ARRAY pids;
      PID_ARRAY lpids;

      pids = context->allocateArray<PAGE_ID>(pages.getSize());
      if (OSS_UNLIKELY(!pids.isValid()))
      {
         PD_LOG(PDERROR, "failed to allocate mem");
         rc = SDB_OOM;
         goto error;
      }
      lpids = context->allocateArray<PAGE_ID>(pages.getSize());
      if (OSS_UNLIKELY(!lpids.isValid()))
      {
         PD_LOG(PDERROR, "failed to allocate mem");
         rc = SDB_OOM;
         goto error;
      }

      rc = reserveLpidsInMem(context, lpids);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to preallocate lpids:%d", rc);
         goto error;
      }

      rc = _dpc->allocatePages(pids.getSize(), pids.data());
      if (SDB_OK != rc)
      {
         freeLpidsInMem(context, lpids);
         PD_LOG(PDERROR, "failed to allocate pids:%d", rc);
         goto error;
      }

      for (UINT32 i = 0; i < pages.getSize(); ++i)
      {
         pages[i].reset(lpids[i], pids[i]);
      }
   done:
      if (lpids.isValid())
      {
         context->releaseBuffer(lpids.data());
      }
      if (pids.isValid())
      {
         context->releaseBuffer(pids.data());
      }
      return rc;
   error:
      goto done;
   }

   void logicalPageSpace::freePagesInMem(requestContext *context,
                                         const LPID_MAPPING_ARRAY &pages)
   {
      SDB_ASSERT(isOpen(), "can not be closed");
      SDB_ASSERT(NULL != context, "can not be null");
      SDB_ASSERT(pages.isValid(), "can not be invalid");
      PID_ARRAY array;

      if (1 == pages.getSize())
      {
         PAGE_ID lpid = pages[0].getLpid();
         SDB_ASSERT(INVALID_PAGE_ID != lpid, "can not invalid");
         freeLpidInMem(context, lpid);
         _dpc->releasePage(pages[0].getPid());
      }
      else
      {
         array = context->allocateArray<PAGE_ID>(pages.getSize());
         if (OSS_UNLIKELY(!array.isValid()))
         {
            PD_LOG(PDSEVERE, "failed to allocate mem, releasing terminated!");
            goto done;
         }

         for (UINT32 i = 0; i < pages.getSize(); ++i)
         {
            array[i] = pages[i].getLpid();
         }
         freeLpidsInMem(context, array);

         for (UINT32 i = 0; i < pages.getSize(); ++i)
         {
            array[i] = pages[i].getPid();
         }
         _dpc->releasePages(array.getSize(), array.data());
      }
   done:
      if (array.isValid())
      {
         context->releaseBuffer(array.data());
      }
      return;
   }

   INT32 logicalPageSpace::reserveLpidsInMem(requestContext *context,
                                             PID_ARRAY lpids)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(_allocator.isInitialized(), "must be inited");
      SDB_ASSERT(NULL != context, "can not be null");
      SDB_ASSERT(lpids.isValid(), "can not be invalid");
      lpids.fill(INVALID_PAGE_ID);
      
      rc = _allocator.allocateBits(lpids.getSize(), lpids.data(), 1);
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

   void logicalPageSpace::freeLpidInMem(requestContext *context,
                                        PAGE_ID lpid)
   {
      SDB_ASSERT(INVALID_PAGE_ID != lpid, "can not be invalid");
      UINT32 reservedImpCount = getReservedImpCount();
      PAGE_ID imp = getImpPidOfLpid(lpid);
      SDB_ASSERT(INVALID_PAGE_ID != imp, "can not be invalid");
      SDB_ASSERT(reservedImpCount <= imp, "impossible");
      _allocator.release(lpid);
      return;
   }

   void logicalPageSpace::freeLpidsInMem(requestContext *context,
                                          const PID_ARRAY &lpids)
   {
      SDB_ASSERT(_allocator.isInitialized(), "must be inited");
      SDB_ASSERT(NULL != context, "can not be null");
      SDB_ASSERT(lpids.isValid(), "can not be null");

      UINT32 reservedImpCount = getReservedImpCount();

      for (UINT32 i = 0; i < lpids.getSize(); ++i)
      {
         SDB_ASSERT(INVALID_PAGE_ID != lpids[i], "can not be invalid");
         PAGE_ID imp = getImpPidOfLpid(lpids[i]);
         SDB_ASSERT(INVALID_PAGE_ID != imp, "can not be invalid");
         SDB_ASSERT(reservedImpCount <= imp, "impossible");
      }

      _allocator.releaseBits(lpids.getSize(), lpids.data());
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

      storageFileName fn;
      createStorageFileOptions options;
      storageUnit *su = context->getEnv()->dms.getStorageUnit(getSpaceID());
      SDB_ASSERT(NULL != su, "can not be null");
      idMapFile *file = NULL;
      storageCoreArgs args(ID_MAP_FILE_PAGE_SIZE,
                           ID_MAP_FILE_MAX_PAGE_COUNT_IN_SEG,
                           ID_MAP_FILE_MAX_SEG_COUNT_IN_FILE);

      idMapFileHead imfHead;
      imfHead.version = ID_MAP_FILE_HEAD_VERSION;
      imfHead.flags = createIdMapFileHeadFlags();
      imfHead.dataPageSize = o.dataArgs.pageSize;
      imfHead.dataPageCountInSeg = o.dataArgs.maxPageCountPerSeg;
      imfHead.dataSegCountInFile = o.dataArgs.maxSegmentCountPerFile;

      slice hs(sizeof(idMapFileHead), &imfHead);

      if (!fn.build(FILE_TYPE_ID_MAP, getSpaceType(), 0))
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
                                                        const LPS_CHECKPOINT &checkpoint)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(isOpen(), "can not be closed");
      SDB_ASSERT(NULL != context, "can not be null");
      SDB_ASSERT(checkpoint.isValid(), "can not be invalid");

      UINT32 oldPageCount = 0;
      idMapFile *base = _idMapFiles.getBack<idMapFile>();
      SDB_ASSERT(NULL != base, "can not be null");
      base->getTotalPageCount(oldPageCount);
      idMapFile *file = NULL;
      ossPoolMap<PAGE_ID, idMapSlot> sorter;
      
      storageCoreArgs args(ID_MAP_FILE_PAGE_SIZE,
                           ID_MAP_FILE_MAX_PAGE_COUNT_IN_SEG,
                           ID_MAP_FILE_MAX_SEG_COUNT_IN_FILE);
      idMapFileHead imfHead;
      imfHead.version = ID_MAP_FILE_HEAD_VERSION;
      imfHead.flags = createIdMapFileHeadFlags();
      imfHead.dataPageSize = getStorageCoreArgs().pageSize;
      imfHead.dataPageCountInSeg = getStorageCoreArgs().maxPageCountPerSeg;
      imfHead.dataSegCountInFile = getStorageCoreArgs().maxSegmentCountPerFile;
      imfHead.totalPageCount = oldPageCount; /// will be reset soon
      imfHead.checkpoint = checkpoint;
      slice hs(sizeof(idMapFileHead), &imfHead);

      createStorageFileOptions o;
      o.args = args;
      o.createAsTmpFile = TRUE;
      o.replaceWhenCreate = TRUE;
      o.secretValue = base->getCommonHeadInMem().secretValue;

      storageFileName fn;

      storageUnit *su = context->getEnv()->dms.getStorageUnit(getSpaceID());
      SDB_ASSERT(NULL != su, "can not be null");

      file = SDB_OSS_NEW idMapFile();
      if (OSS_UNLIKELY(NULL == file))
      {
         PD_LOG(PDERROR, "failed to allocate mem");
         rc = SDB_OOM;
         goto error;
      }

      if (!fn.build(FILE_TYPE_ID_MAP, getSpaceType(), base->getSequence() + 1))
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

      rc = base->copySemgmentsTo(file);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to copy segments to file:%s, rc:%d",
                file->getFullPath(), rc);
         goto error;
      }

      /// to avoid random io, sort first
      for (UINT32 i = 0; i < _lpidCache.getBucketCount(); ++i)
      {
         const partialImpCacheMap::CACHE_MAP &cm = _lpidCache.getImmutableMap(i);
         for (partialImpCacheMap::CACHE_MAP::const_iterator itr = cm.begin();
              itr != cm.end(); ++itr)
         {
            SDB_ASSERT(INVALID_PAGE_ID != itr->first, "impossible");
            sorter[itr->first] = itr->second;
         }
      }

      SDB_ASSERT(ossIsPowerOf2(ID_MAP_FILE_MAX_PAGE_COUNT_IN_SEG), "must be power of 2");

      if (!sorter.empty())
      {
         PAGE_ID maxLpid = sorter.rbegin()->first;
         PAGE_ID maxImp = getImpPidOfLpid(maxLpid);
         UINT32 minSegCnt = ossAlignX(maxImp + 1, ID_MAP_FILE_MAX_PAGE_COUNT_IN_SEG) /
                            ID_MAP_FILE_MAX_PAGE_COUNT_IN_SEG;
         imfHead.totalPageCount = maxImp + 1;
         rc = file->updateUserDefinedHead(hs);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to update id map page head:%d", rc);
            goto error;
         }

         rc = file->ensureSegmentCountAndInit(minSegCnt);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to extend file space:%d", rc);
            goto error;
         }
      }

      for (ossPoolMap<PAGE_ID, idMapSlot>::const_iterator itr = sorter.begin();
           itr != sorter.end(); ++itr)
      {
         UINT32 pos = getIdMapSlotNo(itr->first);
         ossValuePtr ptr = 0;
         PAGE_ID imp = getImpPidOfLpid(itr->first);
         rc = file->getPagePtr(imp, ptr);
         if (OSS_UNLIKELY(SDB_OK != rc))
         {
            PD_LOG(PDERROR, "failed to get page[%d] ptr:%d", imp, rc);
            goto error;
         }

         *(((idMapSlot *)ptr) + pos) = itr->second;
      }

      sorter.clear();
      file->fsync();

      rc = file->removeShadowSuffix();
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to remvoe file suffix:%d", rc);
         goto error;
      }

      _idMapFiles.pushBack(file);
      _lpidCache.resetBaseAndClearImmutableMaps(file);
      _logConsole.destroyHistoryFiles(context);
      _idMapFiles.truncate(3);
      
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

   INT32 logicalPageSpace::replayDeltaLog(requestContext *context)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(_lpidCache.isReady(), "can not be invalid");
      SDB_ASSERT(_logConsole.getLastCheckpoint().isValid(), "can not be invalid");
      dataPageCluster *dpc = getDataStorageObj();
      SDB_ASSERT(NULL != dpc && dpc->isOpen(), "can not be invalid");
      const storageFile &delta = _logConsole.getWorkingFile();
      SDB_ASSERT(delta.isOpen(), "can not be closed");
      PAGE_ID maxPid = _logConsole.getLastCheckpointPid();
      SDB_ASSERT(INVALID_PAGE_ID != maxPid, "can not be invalid");

      for (PAGE_ID i = 0; i <= maxPid; ++i)
      {
         const deltaLogFilePage *page = NULL;
         ossValuePtr ptr = 0;
         rc = delta.getPagePtr(i, ptr);
         if (OSS_UNLIKELY(SDB_OK != rc))
         {
            PD_LOG(PDERROR, "failed to get page[%d] ptr:%d", i, rc);
            goto error;
         }

         page = (const deltaLogFilePage *)ptr;
         for (UINT32 offset = 0; offset < page->dataOffset;)
         {
            deltaLogRecord dlr;
            dlr.reset(page->data + offset);
            if (!dlr.isValid())
            {
               PD_LOG(PDERROR, "failed to load log record at file[%s], page[%d], offset[%d]",
                      delta.getFullPath(), i, offset);
               rc = SDB_VESSEL_INVALID_VESSEL_FILE;
               goto error;
            }
            
            if (DELTA_LOG_TYPE_MAPPING == dlr.getLogType())
            {
               rc = replayMappingDeltaLog(context, dlr);
            }
            else if (DELTA_LOG_TYPE_REMAPPING == dlr.getLogType())
            {
               rc = replayRemmapingDeltaLog(context, dlr);
            }
            else if (DELTA_LOG_TYPE_UNMAPPING == dlr.getLogType())
            {
               rc = replayUnmappingDeltaLog(context, dlr);
            }
            else
            {
               /// do nothing.
            }

            if (SDB_OK != rc)
            {
               PD_LOG(PDERROR, "failed to replay log record at file[%s], page[%d], offset[%d], rc:%d",
                      delta.getFullPath(), i, offset, rc);
               goto error;
            }

            offset += dlr.getLogSize();
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

         rc = _dpc->ensurePidSpace(slot.pid);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to ensure pid[%d] space:%d", slot.pid, rc);
            goto error;
         }

         rc = _dpc->occupyPage(slot.pid);
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

   INT32 logicalPageSpace::flushSegmentsAtCheckpoint(requestContext *context,
                                                     const ossPoolSet<UINT32> &segments)const
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(NULL != context, "can not be null");
      SDB_ASSERT(NULL != _dpc, "can not be null");
      SDB_ASSERT(_dpc->isOpen(), "can not be closed");
      SDB_ASSERT(isCopyOnWrite(), "impossible");

      autoEventList<backgroundEvent> rl;
      backgroundWorkers &workers = context->getEnv()->workers;
      UINT32 dispatched = 0;
      UINT32 responsed = 0;

      PD_LOG(PDDEBUG, "begin to flush [%d] segments at checkpoint", segments.size());

      for (ossPoolSet<UINT32>::const_iterator itr = segments.begin();
              itr != segments.end(); ++itr)
      {
         if (workers.isReady() &&
             !workers.isCommonFamilyBusy())
         {
            backgroundEvent event;
            lpsFlushingSegments msg;
            msg._sid = getSpaceID();
            msg._type = getSpaceType();
            msg._segmentId = *itr;
            msg._count = 1;
            event.setType(backgroundEvent::EVENT_TYPE_SYNC_SEG);
            event.setEventMsg(sizeof(lpsFlushingSegments), &msg);
            event.setResponseList(&rl);
            workers.pushEvent(event);
            ++dispatched;
         }
         else
         {
            rc = _dpc->fsyncSegment(*itr);
            if (SDB_OK != rc)
            {
               PD_LOG(PDSEVERE, "failed to flush global segment[%d, %d, %d], rc:%d",
                     getSpaceID(), getSpaceType(), *itr, rc);
               rc = SDB_OK;
            }
         }
      }

      while (responsed < dispatched)
      {
         backgroundEvent event;
         rl.popOrWait(event);
         SDB_ASSERT(event.getType() == backgroundEvent::EVENT_TYPE_FINISHED,
                     ", must be finish");
         ++responsed;
      }

      PD_LOG(PDDEBUG, "end to flush [%d] segments, [%d]tasks dispatched, lps[%d,%d]",
             segments.size(), dispatched, getSpaceID(), getSpaceType());
   done:
      return rc;
   error:
      goto done;
   }

   void logicalPageSpace::postCheckpointIfNecessary(requestContext *context)
   {
      SDB_ASSERT(isOpen(), "can not be invalid");
      SDB_ASSERT(NULL != context && context->isOpen(), "can not be invalid");

      UINT64 dirtySize = (UINT64)(getStorageCoreArgs().pageSize) *
                         _lpidCache.getFuzzyMutablePageCount();
      if (CHECKPOINT_TRIGGER_DIRTY_PAGE_SIZE <= dirtySize)
      {
         if (_checkpointContext.tryToApplyCheckpoint())
         {
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
      UINT32 impCount = 0;
      base->getTotalPageCount(impCount);
      UINT64 baseSize = (UINT64)impCount * ID_MAP_FILE_PAGE_SIZE;
      UINT64 cacheSize = _lpidCache.getFuzzyCacheSize();
      UINT64 deltaLogSize = _logConsole.getDeltaLogSize();
      PD_LOG(PDDEBUG, "lps[%d,%d] cache size:%lld, base size:%lld, delta log:%lld",
             getSpaceID(), getSpaceType(), cacheSize, baseSize, deltaLogSize);
      return FULL_CHECKPOINT_LPID_CACHE_SIZE <= cacheSize ||
             FULL_CHECKPOINT_DELTA_LOG_SIZE <= deltaLogSize;
   }

   INT32 logicalPageSpace::resumeToLatestCheckpoint(requestContext *context)
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

      rc = restoreAllocatorByBaseFile(context, base);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to restore allocator by base file:%d", rc);
         goto error;
      }

      if (baseHead.checkpoint.isValid())
      {
         _checkpointContext.setCheckpoint(baseHead.checkpoint);
      }

      if (_logConsole.getLastCheckpoint().isValid())
      {
         SDB_ASSERT(_logConsole.getBaseSequence() == base->getSequence(), 
                    "must be same");
         rc = replayDeltaLog(context);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to replay delta log, rc:%d", rc);
            goto error;
         }

         _checkpointContext.setCheckpoint(_logConsole.getLastCheckpoint());
      }
   done:
      return rc;
   error:
      goto done;
   }

   void logicalPageSpace::extractDirtySegmentsInCache(ossPoolSet<UINT32> &segments)
   {
      SDB_ASSERT(isOpen(), "can not be invalid");
      SDB_ASSERT(_lpidCache.isReady(), "can not be invalid");
      UINT32 pageCountPerSegment = getStorageCoreArgs().maxPageCountPerSeg;

      for (UINT32 i = 0; i < _lpidCache.getBucketCount(); ++i)
      {
         const partialImpCacheMap::CACHE_MAP &cm = _lpidCache.getMutableMap(i);
         for (partialImpCacheMap::CACHE_MAP::const_iterator itr = cm.begin();
              itr != cm.end(); ++itr)
         {
            if (!itr->second.isFree())
            {
               segments.insert(itr->second.pid / pageCountPerSegment);
            }
         }
      }
      return;
   }

   INT32 logicalPageSpace::replayMappingDeltaLog(requestContext *context,
                                                 const deltaLogRecord &dlr)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(NULL != context, "can not be null");
      SDB_ASSERT(_lpidCache.isReady(), "can not be invalid");
      SDB_ASSERT(dlr.getLogType() == DELTA_LOG_TYPE_MAPPING, "must be mapping log");
      SDB_ASSERT(NULL != _dpc && _dpc->isOpen(), "can not be invalid");
      SDB_ASSERT(_allocator.isInitialized(), "can not be invalid");

      UINT8 count = 0;
      PAGE_SNAPSHOT_VERION psv = INVALID_PAGE_SNAPSHOT_VERSION;
      mappedLogicalPageId mid;

      rc = dlrMappingReader::read(dlr, psv, count);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to read mapping log record:%d", rc);
         goto error;
      }

      for (UINT8 i = 0; i < count; ++i)
      {
         PAGE_ID imp = INVALID_PAGE_ID;

         rc = dlrMappingReader::getItem(dlr, i, mid);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to read item[%d], rc:%d", i, rc);
            goto error;
         }

         imp = getImpPidOfLpid(mid.getLpid());
         if (getReservedImpCount() <= imp)
         {
            rc = _allocator.ensureBitmapPageCount(imp + 1);
            if (SDB_OK != rc)
            {
               PD_LOG(PDERROR, "failed to ensure imp [%d] in allocator:%d",
                      imp, rc);
               goto error;
            }

            rc = _allocator.occupy(mid.getLpid());
            if (SDB_OK != rc)
            {
               PD_LOG(PDERROR, "failed to occupy lpid[%d], rc:%d", mid.getLpid(), rc);
               goto error;
            }
         }

         rc = _dpc->ensurePidSpace(mid.getPid());
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to ensure pid[%d] space:%d", mid.getPid(), rc);
            goto error;
         }

         rc = _dpc->occupyPage(mid.getPid());
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to occupy page[%d] in storage:%d", mid.getPid(), rc);
            goto error;
         }

         rc = _lpidCache.upsertWhenRestore(mid.getLpid(), idMapSlot(psv, mid.getPid()));
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to upsert lpid[%d] to id cache:%d", mid.getLpid(), rc);
            goto error;
         }
      }

   done:
      return rc;
   error:
      goto done;
   }

   INT32 logicalPageSpace::replayUnmappingDeltaLog(requestContext *context,
                                                   const deltaLogRecord &dlr)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(NULL != context, "can not be null");
      SDB_ASSERT(_lpidCache.isReady(), "can not be invalid");
      SDB_ASSERT(dlr.getLogType() == DELTA_LOG_TYPE_UNMAPPING, "must be mapping log");
      SDB_ASSERT(NULL != _dpc && _dpc->isOpen(), "can not be invalid");
      SDB_ASSERT(_allocator.isInitialized(), "can not be invalid");

      UINT8 count = 0;
      UINT8 flags = 0;
      mappedLogicalPageId mid;
      BOOLEAN releasePid = FALSE;

      rc = dlrUnmapingReader::read(dlr, flags, count);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to read unmapping log record:%d", rc);
         goto error;
      }

      releasePid = (0 != OSS_BIT_TEST(flags, DELTA_LOG_TYPE_UNMAPPING_FLAG_RELEASE_PID));
      for (UINT8 i = 0; i < count; ++i)
      {
         PAGE_ID imp = INVALID_PAGE_ID;
         mappedLogicalPageId mid;
         rc = dlrUnmapingReader::getItem(dlr, i, mid);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to get item[%d] in unmapping log:%d", i, rc);
            goto error;
         }

         imp = getImpPidOfLpid(mid.getLpid());
         if (getReservedImpCount() <= imp)
         {
            _allocator.release(mid.getLpid());
         }

         if (releasePid)
         {
            _dpc->releasePage(mid.getPid());
         }

         rc = _lpidCache.upsertWhenRestore(mid.getLpid(), idMapSlot());
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to upsert lpid[%d] into cache:%d", mid.getLpid(), rc);
            goto error;
         }
      }
   done:
      return rc;
   error:
      goto done;
   }
   
   INT32 logicalPageSpace::replayRemmapingDeltaLog(requestContext *context,
                                                   const deltaLogRecord &dlr)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(NULL != context, "can not be null");
      SDB_ASSERT(_lpidCache.isReady(), "can not be invalid");
      SDB_ASSERT(dlr.getLogType() == DELTA_LOG_TYPE_REMAPPING, "must be mapping log");
      SDB_ASSERT(NULL != _dpc && _dpc->isOpen(), "can not be invalid");
      SDB_ASSERT(_allocator.isInitialized(), "can not be invalid");

      UINT8 count = 0;
      UINT8 flags = 0;
      PAGE_SNAPSHOT_VERION psv = INVALID_PAGE_SNAPSHOT_VERSION;
      mappedLogicalPageId mid;
      BOOLEAN releasePid = FALSE;

      rc = dlrRemappingReader::read(dlr, psv, flags, count);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to read remapping log record:%d", rc);
         goto error;
      }

      releasePid = (0 != OSS_BIT_TEST(flags, DELTA_LOG_TYPE_REMAPPING_FLAG_RELEASE_PID));

      for (UINT8 i = 0; i < count; ++i)
      {
         mappedLogicalPageId mid;
         PAGE_ID oldPid = INVALID_PAGE_ID;
         rc = dlrRemappingReader::getItem(dlr, i, mid, oldPid);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to get item[%d] in record:%d", i, rc);
            goto error;
         }

         rc = _dpc->ensurePidSpace(mid.getPid());
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to ensure pid[%d] in storage:%d", mid.getPid(), rc);
            goto error;
         }

         rc = _dpc->occupyPage(mid.getPid());
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to occupy pid[%d] in storeage:%d", mid.getPid(), rc);
            goto error;
         }

         if (releasePid)
         {
            _dpc->releasePage(oldPid);
         }

         rc = _lpidCache.upsertWhenRestore(mid.getLpid(), idMapSlot(psv, mid.getPid()));
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to upsert lpid[%d] into lpid cache:%d", mid.getLpid(), rc);
            goto error;
         }
      }
   done:
      return rc;
   error:
      goto done;
   }
   

   INT32 logicalPageSpace::map(requestContext *context,
                               PAGE_SNAPSHOT_VERION psv,
                               const LPID_MAPPING_ARRAY &mapping)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(isOpen(), "can not be closed");
      SDB_ASSERT(NULL != context, "can not be null");
      SDB_ASSERT(INVALID_PAGE_SNAPSHOT_VERSION != psv, "can not be invalid");
      SDB_ASSERT(mapping.isValid(), "can not be invalid");

      deltaLogRecordBuilder builder;
      BOOLEAN blocked = FALSE;
      DPS_LSN_OFFSET lsn = context->getExecutor()->getEndLsn();
      SDB_ASSERT(DPS_INVALID_LSN_OFFSET != lsn, "can not be invalid");

      /// build delta log
      rc = builder.buildMappingLog(psv, mapping.getSize(), mapping.data());
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to build mapping log:%d", rc);
         goto error;
      }

      /// block checkpoint
      rc = context->blockCheckpoint(getSpaceType(),
                                    _checkpointContext.getLatch());
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to block checkpoint:%d", rc);
         goto error;
      }
      blocked = TRUE;

      /// do mapping
      {
         ossXLatchGuard guard(&_mappingLatch);
         rc = _logConsole.append(context, builder.getDeltaLogRecord());
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR,"failed to append delta log record:%d", rc);
            goto error;
         }

         _checkpointContext.updateDirtyLsn(lsn);
      }

      ///update cache
      for (UINT32 i = 0; i < mapping.getSize(); ++i)
      {
         idMapSlot slot(psv, mapping[i].getPid());
         rc = _lpidCache.put(mapping[i].getLpid(), slot);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to upsert lpid[%d] in cache:%d", mapping[i].getLpid(), rc);
            ossPanic();
            goto error;
         }
      }

      context->unblockCheckpoint();
      blocked = FALSE;
      postCheckpointIfNecessary(context);

   done:
      if (blocked)
      {
         context->unblockCheckpoint();
      }
      return rc;
   error:
      goto done;
   }

   INT32 logicalPageSpace::remap(requestContext *context,
                                 PAGE_SNAPSHOT_VERION psv,
                                 const LPID_MAPPING_ARRAY &mapping,
                                 const PID_ARRAY &oldPids,
                                 BOOLEAN releaseOld)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(NULL != context, "can not be null");
      SDB_ASSERT(INVALID_PAGE_SNAPSHOT_VERSION != psv, "can not be invalid");
      SDB_ASSERT(mapping.isValid() && oldPids.isValid(), "can not be invalid");
      SDB_ASSERT(mapping.getSize() == oldPids.getSize(), "must be same");
      SDB_ASSERT(mapping.getSize() < 256, "out of size");

      deltaLogRecordBuilder builder;
      BOOLEAN blocked = FALSE;
      DPS_LSN_OFFSET lsn = context->getExecutor()->getEndLsn();
      SDB_ASSERT(DPS_INVALID_LSN_OFFSET != lsn, "can not be invalid");

      rc = builder.buildRemappingLog(psv, mapping.getSize(),
                                     mapping.data(),
                                     oldPids.data(),
                                     releaseOld);
      if (OSS_UNLIKELY(SDB_OK != rc))
      {
         PD_LOG(PDERROR, "failed to build remapping log:%d", rc);
         goto error;
      }

      rc = context->blockCheckpoint(getSpaceType(),
                                    _checkpointContext.getLatch());
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to block checkpoint:%d", rc);
         goto error;
      }
      blocked = TRUE;

      {
         ossXLatchGuard guard(&_mappingLatch);
         rc = _logConsole.append(context, builder.getDeltaLogRecord());
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR,"failed to append delta log record:%d", rc);
            goto error;
         }

         _checkpointContext.updateDirtyLsn(lsn);
      }

      for (UINT32 i = 0; i < mapping.getSize(); ++i)
      {
         idMapSlot slot(psv, mapping[i].getPid());
         rc = _lpidCache.put(mapping[i].getLpid(), slot);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to upsert lpid[%d] in cache:%d", mapping[i].getLpid(), rc);
            ossPanic();
            goto error;
         }
      }

      /// it is not necessary to push pids into waiting list
      /// if space is not copy-on-write.
      /// we just do it for the safety.
      if (releaseOld)
      {
         pushIntoWaitingFreeList(oldPids);
      }

      context->unblockCheckpoint();
      blocked = FALSE;
      postCheckpointIfNecessary(context);
   done:
      if (blocked)
      {
         context->unblockCheckpoint();
      }
      return rc;
   error:
      goto done;
   }

   INT32 logicalPageSpace::unmap(requestContext *context,
                                 const LPID_MAPPING_ARRAY &mapping,
                                 BOOLEAN releasePid)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(isOpen(), "can not be closed");
      SDB_ASSERT(NULL != context, "can not be null");
      SDB_ASSERT(mapping.isValid(), "can not be invalid");
      deltaLogRecordBuilder builder;
      DPS_LSN_OFFSET lsn = context->getExecutor()->getEndLsn();
      SDB_ASSERT(DPS_INVALID_LSN_OFFSET != lsn, "can not be invalid");
      BOOLEAN blocked = FALSE;

      PID_ARRAY pids;
      if (releasePid)
      {
         pids = context->allocateArray<PAGE_ID>(mapping.getSize());
         if (OSS_UNLIKELY(!pids.isValid()))
         {
            PD_LOG(PDERROR, "failed to allocate mem");
            rc = SDB_OOM;
            goto error;
         }

         for (UINT32 i = 0; i < mapping.getSize(); ++i)
         {
            pids[i] = mapping[i].getPid();
         }
      }

      rc = builder.buildUnmappingLog(mapping.getSize(), mapping.data(), releasePid);
      if (OSS_UNLIKELY(SDB_OK != rc))
      {
         PD_LOG(PDERROR, "failed to build unmapping log:%d", rc);
         goto error;
      }

      rc = context->blockCheckpoint(getSpaceType(),
                                    _checkpointContext.getLatch());
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to block checkpoint:%d", rc);
         goto error;
      }
      blocked = TRUE;

      {
         ossXLatchGuard guard(&_mappingLatch);
         rc = _logConsole.append(context, builder.getDeltaLogRecord());
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR,"failed to append delta log record:%d", rc);
            goto error;
         }

         _checkpointContext.updateDirtyLsn(lsn);
      }

      for (UINT32 i = 0; i < mapping.getSize(); ++i)
      {
         _lpidCache.remove(mapping[i].getLpid());
      }

      if (releasePid)
      {
         pushIntoWaitingFreeList(pids);
      }

      context->unblockCheckpoint();
      blocked = FALSE;
      postCheckpointIfNecessary(context);
   done:
      if (blocked)
      {
         context->unblockCheckpoint();
      }
      if (pids.isValid())
      {
         context->releaseBuffer(pids.data());
      }
      return rc;
   error:
      goto done;
   }

   void logicalPageSpace::pushIntoWaitingFreeList(const PID_ARRAY &pids)
   {
      SDB_ASSERT(isOpen(), "can not be invalid");
      SDB_ASSERT(pids.isValid(), "can not be invalid");
      ossXLatchGuard guard(&_freeLatch);
      for (UINT32 i = 0; i < pids.getSize(); ++i)
      {
         PAGE_ID pid = pids[i];
         if (OSS_LIKELY(INVALID_PAGE_ID != pid))
         {
            _waitingToFree.push(pid);
         }
         else
         {
            SDB_ASSERT(FALSE, "invalid pid");
         }
      }
      return;
   }

   void logicalPageSpace::mergeWaitingListIntoReadyList()
   {
      SDB_ASSERT(isOpen(), "can not be invalid");
      ossXLatchGuard guard(&_freeLatch);
      if (!_waitingToFree.isEmpty())
      {
         _waitingToFree.transferTo(_readyToFree);
      }
      return;
   }

   void logicalPageSpace::freePidsInReadyList()
   {
      SDB_ASSERT(isOpen(), "can not be invalid");
      pidBatchList bl;

      {
         ossXLatchGuard guard(&_freeLatch);
         if (!_readyToFree.isEmpty())
         {
            _readyToFree.transferTo(bl);
         }
      }

      for (pidBatchList::BATCH_LIST::const_iterator itr = bl.begin();
           itr != bl.end(); ++itr)
      {
         getDataStorageObj()->releasePages(itr->size(), itr->data());
      }

      bl.clear();
      return;
   }

   UINT32 logicalPageSpace::createIdMapFileHeadFlags()const
   {
      UINT32 flags = 0;
      if (isCopyOnWrite())
      {
         OSS_BIT_SET(flags, IMF_FLAG_COPY_ON_WRITE);
      }
      return flags;
   }

   BOOLEAN logicalPageSpace::validateIdMapFileHeadFlags(UINT32 flags)const
   {
      return createIdMapFileHeadFlags() == flags;
   }

   INT32 logicalPageSpace::getMinUncompletedLSN(requestContext *context,
                                                DPS_LSN_OFFSET &lsn)
   {
      SDB_ASSERT(NULL != context, "can not be null");
      IRedoLogger *logger = context->getOuterResource()->logger;
      lsn = logger->getMinUncommitedLSN();
      return SDB_OK;
   }
}//namespace vessel
}//namespace engine