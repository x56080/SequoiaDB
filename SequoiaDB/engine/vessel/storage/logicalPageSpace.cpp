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
#include "vessel/deltaLogScanner.h"
#include "vessel/deltaLogRecordBuilder.h"
#include "ossLatchGuard.hpp"
#include "vessel/redoLogUtil.h"
#include "vessel/deltaLogRecord.h"
#include "vessel/requestContext.h"

namespace engine
{
namespace vessel
{
   static const UINT32 FULL_CHECKPOINT_LPID_CACHE_SIZE = 8388608;

///////////////logicalPageSpace::_runtimePageBufferIniter begin
   INT32 logicalPageSpace::
         _runtimePageBufferIniter::
         initWithCache(const GLOBAL_PAGE_ID &gpid,
                       UINT32 pageSize,
                       const runtimePageBuffer::options &o,
                       liteCacheTuple &tuple,
                       runtimePageBuffer &rpb)
   {
      return rpb.init(gpid, pageSize, tuple, o);
   }

   INT32 logicalPageSpace::
         _runtimePageBufferIniter::
         initWithMmap(const GLOBAL_PAGE_ID &gpid,
                      UINT32 pageSize,
                      const runtimePageBuffer::options &o,
                      const mmapPagePointer &ptr,
                      runtimePageBuffer &rpb)
   {
      return rpb.init(gpid, pageSize, ptr, o);
   }


///////////////logicalPageSpace::_runtimePageBufferIniter end
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
      _lpidCache.fini();
      _idMapFiles.close();
      _allocator.fini();
      _logConsole.fini();
      
      if (NULL != _dpc)
      {
         _dpc->close();
         _dpc = NULL;
      }

      _checkpointContext.fini();
      _creater.fini();
      return;
   }

   void logicalPageSpace::destroy()
   {
      _destroy();
      if (NULL != _dpc)
      {
         _dpc->destroy();
         _dpc = NULL;
      }
      if (_logConsole.isReady())
      {
         _logConsole.destroy();
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
                       !o.isValid()))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      _creater.init(o.sid, getSpaceType(), o.secretValue, o.dir);
   
      /// 1. create id map file
      rc = createFirstIdMapFile(o.dataArgs);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to create id map file:%d", rc);
         goto error;
      }
      baseFile = (const idMapFile *)(_idMapFiles.getBack());
      SDB_ASSERT(NULL != baseFile, "can not be null");

      /// 2. init lpid allocator.
      bitmapOptions.bitmapPageSkipped = getReservedImpCount();
      bitmapOptions.freeBound = getFreeBoundOfLpidAllocator();
      bitmapOptions.maxBitmapPageCount = ID_MAP_FILE_MAX_PAGE_COUNT;
      rc = _allocator.init(ID_MAP_PAGE_CAPACITY, bitmapOptions);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to init lpid allocator:%d", rc);
         goto error;
      }

      /// 3. init delta log
      rc = _logConsole.init(&_creater);
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

      rc = _dpc->open(o.dataArgs, &_creater,
                      NULL, getFreeBoundOfPageStorage());
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
      destroy();
      goto done;
   }

   INT32 logicalPageSpace::open(requestContext *context,
                                SPACE_ID sid,
                                const CHAR *dirPath)
   {
      INT32 rc = SDB_OK;
      storageFileLoader loader;
      strSlice dirSlice(dirPath);
      inMemBitmap::options o;
      const idMapFile *base = NULL;
      idMapFileHead baseHead;
      storageCoreArgs dataArgs;
      logicalPageIdCache::options cacheOptions;

      if (OSS_UNLIKELY(isOpen()))
      {
         SDB_ASSERT(FALSE, "do not reinit");
         close();
      }

      if (OSS_UNLIKELY(NULL == context ||
                       INVALID_SPACE_ID == sid ||
                       dirSlice.empty()))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      rc = loader.load(dirSlice, sid, getSpaceType(), TRUE);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to load files:%d", rc);
         goto error;
      }

      /// Open id map files
      rc = openIdMapFiles(sid, dirSlice, loader);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to open id map file:%d", rc);
         goto error;
      }

      base = (const idMapFile *)(_idMapFiles.getBack());
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
      _creater.init(sid, getSpaceType(),
                    base->getCommonHeadInMem().secretValue, dirSlice);
      
      /// init allocator.
      o.bitmapPageSkipped = getReservedImpCount();
      o.freeBound = getFreeBoundOfLpidAllocator();
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

      rc = _dpc->open(dataArgs, &_creater, &loader, getFreeBoundOfPageStorage());
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to init page storage:%d", rc);
         goto error;
      }

      /// init cache
      cacheOptions.bucketCount = context->getEnv()->options._spaceLpidCacheBucketCount;
      cacheOptions.bucketLatchCount = context->getEnv()->options._spaceLpidCacheBucketLatchCount;
      rc = _lpidCache.init(cacheOptions, base);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to init cache:%d", rc);
         goto error;
      }

      /// open delta log files
      rc = _logConsole.init(&_creater,
                            loader.getFileList(FILE_TYPE_DELTA_LOG),
                            baseHead.deltaLogOffset);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to init log console:%d", rc);
         goto error;
      }

      if (!_logConsole.getCheckpoint().isValid())
      {
         if (0 != base->getCommonHeadInMem().sequence)
         {
            PD_LOG(PDERROR, "can not find checkpoint in log but base sequence is[%lld]",
                   base->getCommonHeadInMem().sequence);
            rc = SDB_VESSEL_INTERNAL_ERR;
            goto error;
         }
      }
      else if (0 != base->getCommonHeadInMem().sequence &&
               _logConsole.getCheckpoint().offset < baseHead.deltaLogOffset)
      {
         PD_LOG(PDERROR, "offset in base file[%lld] does not match checkpoint[%lld]",
                baseHead.deltaLogOffset, _logConsole.getCheckpoint().offset);
         rc = SDB_VESSEL_INTERNAL_ERR;
         goto error;
      }

      /// restore allocator by base file
      rc = restoreAllocatorByBaseFile(base);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to restore allocator by base file:%d", rc);
         goto error;
      }

      if (_logConsole.getCheckpoint().isValid())
      {
         UINT64 beginOffset = (DPS_INVALID_LSN_OFFSET == baseHead.deltaLogOffset) ?
                              0 : baseHead.deltaLogOffset;
         /// restore allocator and cache to last checkpoint
         rc = replayDeltaLogWhenOpen(beginOffset);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to init in-mem pools:%d", rc);
            goto error;
         }
         _checkpointContext.setCheckpoint(_logConsole.getCheckpoint());
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

   INT32 logicalPageSpace::createCheckpoint(requestContext *context)
   {
      INT32 rc = SDB_OK;
      BOOLEAN locked = FALSE;
      BOOLEAN fullCheckpoint = FALSE;
      BOOLEAN abortCheckpoint = FALSE;
      DPS_LSN_OFFSET minDirtyLsn = DPS_INVALID_LSN_OFFSET;
      DPS_LSN_OFFSET maxDirtyLsn = DPS_INVALID_LSN_OFFSET;
      UINT32 totalImpCount = 0;
      ossPoolSet<UINT32> segments;

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

      _checkpointContext.getLatch()->lock_w();
      locked = TRUE;

      if (lpsCheckpointContext::NONE != _checkpointContext.getStatus())
      {
         rc = SDB_VESSEL_SAME_TASK_RUNNING;
         goto error;
      }

      maxDirtyLsn = _checkpointContext.getMaxDirtyLsn();
      if (DPS_INVALID_LSN_OFFSET == maxDirtyLsn)
      {
         goto done;
      }
      minDirtyLsn = _checkpointContext.getMinDirtyLsn();
      SDB_ASSERT(DPS_INVALID_LSN_OFFSET != minDirtyLsn, "impossible");

      _checkpointContext.setStatus(lpsCheckpointContext::PREPARE);
      abortCheckpoint = TRUE;

      rc = prepareToCreateCheckpoint(context);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to prepare to create checkpoint:%d", rc);
         goto error;
      }

      fullCheckpoint = (FULL_CHECKPOINT_LPID_CACHE_SIZE <= _lpidCache.getTotalCacheSize());

      rc = _logConsole.precreateCheckpoint(0, maxDirtyLsn);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to get log console ready to create checkpoint:%d", rc);
         goto error;
      }

      /// New imp may allocated in preallocating.
      /// But we can be sure page count not less than real count to be
      /// saved into new base id map file.
      totalImpCount = _allocator.getPageCount();

      rc = prepareToFlushSegments(context, fullCheckpoint, segments);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to create segment list:%d", rc);
         goto error;
      }

      _checkpointContext.getLatch()->release_w();
      locked = FALSE;

      if (!segments.empty())
      {
         _checkpointContext.setStatus(lpsCheckpointContext::FLUSH_SEGS);
         rc = flushWhenCreatingCheckpoint(context, segments);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to flush segments:%d", rc);
            goto error;
         }
      }

      _checkpointContext.setStatus(lpsCheckpointContext::COMMIT);
      rc = _logConsole.commitCheckpointPrecreated();
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to commit checkpoint:%d", rc);
         goto error;
      }

      _checkpointContext.getLatch()->lock_w();
      locked = TRUE;
      _checkpointContext.setCheckpoint(_logConsole.getCheckpoint());
      if (maxDirtyLsn == _checkpointContext.getMaxDirtyLsn())
      {
         /// no more new request
         _checkpointContext.clearLsn();
      }
      else
      {
         _checkpointContext.setMinDirtyLsn(maxDirtyLsn + 1);
      }

      if (fullCheckpoint)
      {
         _checkpointContext.setStatus(lpsCheckpointContext::CREATE_NEW_BASE);
         _checkpointContext.getLatch()->release_w();
         locked = FALSE;
         rebaseWhenCreatingCheckpoint(totalImpCount,
                                      _checkpointContext.getCheckpoint().offset);
         removeHistoryIdMapAndDeltaLogFiles();
         _checkpointContext.getLatch()->lock_w();
         locked = TRUE;
         
      }
      
      _checkpointContext.setStatus(lpsCheckpointContext::NONE);

   done:
      if (locked)
      {
         _checkpointContext.getLatch()->release_w();
      }
      return rc;
   error:
      if (abortCheckpoint)
      {
         if (!locked)
         {
            _checkpointContext.getLatch()->lock_w();
            locked = TRUE;
         }
         _checkpointContext.setStatus(lpsCheckpointContext::NONE);
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

   INT32 logicalPageSpace::getLogicalPageBuffer(requestContext *context,
                                                PAGE_ID lpid,
                                                OSS_SHARED_LATCH_MODE mode,
                                                logicalPageBuffer &lpb)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(!lpb.isValid(), "impossible");
      BOOLEAN isMutablePage = FALSE;
      idMapSlot slot;
      runtimePageBuffer::options o;

      lpb.fini();
      if (OSS_UNLIKELY(!isOpen()))
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }
      else if (OSS_UNLIKELY(NULL == context ||
                            INVALID_PAGE_ID == lpid ||
                            OSS_SHARED_LATCH_MODE_NONE == mode))
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

      rc = getPageFromCache(lpid, slot, isMutablePage);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to find lpid[%d] in cache:%d", lpid, rc);
         goto error;
      }

      rc = getRuntimePageBuffer(context, slot.pid,
                                mode, o, lpb._rpb);
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

   INT32 logicalPageSpace::makeBufferWritable(requestContext *context,
                                              logicalPageBuffer &lpb)
   {
      INT32 rc = SDB_OK;
      BOOLEAN snapshotEffective = FALSE;
      idMapSlot slot;
      runtimePageBuffer &rpb = lpb._rpb;

      if (OSS_UNLIKELY(!isOpen()))
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }
      else if (OSS_UNLIKELY(NULL == context ||
                            !lpb.isValid()))
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
      else if (OSS_UNLIKELY(OSS_SHARED_LATCH_MODE_NONE == lpb._lh.getLockMode() ||
                            OSS_SHARED_LATCH_MODE_SHARED == lpb._lh.getLockMode()))
      {
         rc = SDB_VESSEL_FORBIDDEN_OP_WLT;
         goto error;
      }
      else if (rpb.isWritingPrepared())
      {
         goto done;
      }

      if (OSS_SHARED_LATCH_MODE_UPGRADE == lpb._lh.getLockMode())
      {
         lpb._lh.unlockUpgradeAndLock();
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
      else if (!lpb.getCowTrigger().isMutablePid())
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
      OSS_SHARED_LATCH_MODE mode = OSS_SHARED_LATCH_MODE_NONE;
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
               OSS_SHARED_LATCH_MODE_EXCLUSIVE != mode)
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
      rc = initAndMapPages(context, initer, 1, &mpid);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to init page:%d", rc);
         goto error;
      }
   done:
      return rc;
   error:
      goto done;
   }

   INT32 logicalPageSpace::getPageMappingAtNonruntime(requestContext *context,
                                                      PAGE_ID lpid,
                                                      PAGE_ID &pid,
                                                      PAGE_SNAPSHOT_VERION &psv,
                                                      mmapPagePointer &ptr)
   {
      INT32 rc = SDB_OK;
      idMapSlot slot;
      BOOLEAN isMutable = FALSE;

      ptr.reset();
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
      if (SDB_OK != rc)
      {
         goto error;
      }

      rc = _lpidCache.get(lpid, slot, isMutable);
      if (SDB_OK != rc)
      {
         goto error;
      }

      rc = _dpc->getDataPagePtr(slot.pid, ptr);
      if (SDB_OK != rc)
      {
         goto error;
      }

      pid = slot.pid;
      psv = slot.psv;
   done:
      return rc;
   error:
      pid = INVALID_PAGE_ID;
      psv = INVALID_PAGE_SNAPSHOT_VERSION;
      ptr.reset();
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
      SDB_ASSERT(OSS_SHARED_LATCH_MODE_EXCLUSIVE == lpb._lh.getLockMode(), "must be exclusive");

      runtimePageBuffer &rpb = lpb._rpb;
      PAGE_SNAPSHOT_VERION psv = context->getEnv()->dms.getOnlinePageSnapshotVersion();
      PAGE_ID lpid = lpb._lh.getLpid();
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
      rc = remap(context, psv, 1, &mpid, &oldPid, releaseOld);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to remap lpid[%d], rc:%d", lpid, rc);
         goto error;
      }

      /// 4. reset cow trigger
      lpb._cowTrigger.reset(psv, TRUE);

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
      goto done;
   }


   INT32 logicalPageSpace::openIdMapFiles(SPACE_ID sid,
                                          const strSlice &dir,
                                          const storageFileLoader &loader)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(INVALID_SPACE_ID != sid, "can not be invalid");
      SDB_ASSERT(!dir.empty(), "can not be empty");

      idMapFile *file = NULL;

      const FILE_NAME_LIST *list = loader.getFileList(FILE_TYPE_ID_MAP);
      if (NULL == list || list->empty())
      {
         PD_LOG(PDERROR, "id map file not found in dir[%s]", dir.str());
         rc = SDB_FNE;
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
         else if (fn.getSpaceID() != sid)
         {
            PD_LOG(PDERROR, "space id does not match:%d, %d",
                   fn.getSpaceID(), sid);
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

         rc = file->open(dir, fn);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to open id map file:%d", rc);
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

      rc = _dpc->allocatePages(count, (PAGE_ID *)pidBuffer);
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

   INT32 logicalPageSpace::createFirstIdMapFile(const storageCoreArgs &dataArgs)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(dataArgs.isValid(), "can not be invalid");
      SDB_ASSERT(_creater.isValid(), "must be valid");
      SDB_ASSERT(_idMapFiles.isEmpty(), "must be empty");

      idMapFile *file = NULL;
      storageCoreArgs args(ID_MAP_FILE_PAGE_SIZE,
                           ID_MAP_FILE_MAX_PAGE_COUNT_IN_SEG,
                           ID_MAP_FILE_MAX_SEG_COUNT_IN_FILE);

      idMapFileHead imfHead;
      imfHead.version = ID_MAP_FILE_HEAD_VERSION;
      imfHead.flags = getIdMapFileHeadFlags();
      imfHead.dataPageSize = dataArgs.pageSize;
      imfHead.dataPageCountInSeg = dataArgs.maxPageCountPerSeg;
      imfHead.dataSegCountInFile = dataArgs.maxSegmentCountPerFile;
      imfHead.totalPageCount = 0;
      imfHead.deltaLogOffset = DPS_INVALID_LSN_OFFSET;

      slice hs(sizeof(idMapFileHead), &imfHead);

      file = SDB_OSS_NEW idMapFile();
      if (OSS_UNLIKELY(NULL == file))
      {
         PD_LOG(PDERROR, "failed to allocate mem");
         rc = SDB_OOM;
         goto error;
      }

      rc = _creater.createTmpFile(FILE_TYPE_ID_MAP,
                                  0, args, file, hs);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to create tmp id map file:%d", rc);
         goto error;
      }

      rc = file->fsync();
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to fsync file:%d", rc);
         goto error;
      }

      rc = renameToFormalAndReopen(_creater.getDirSlice(),
                                   FALSE, FILE_SHADOW_SUFFIX_TMP, file);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to rename id map file and reopen:%d", rc);
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

   INT32 logicalPageSpace::rebaseWhenCreatingCheckpoint(UINT32 totalImpCount,
                                                        UINT64 deltaLogOffset)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(isOpen(), "can not be closed");
      UINT32 oldPageCount = 0;
      UINT32 segmentCount = 0;
      storageFile *base = NULL;
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
      imfHead.deltaLogOffset = deltaLogOffset;
      slice hs(sizeof(idMapFileHead), &imfHead);

      base = _idMapFiles.getBack();
      if (NULL == base)
      {
         PD_LOG(PDERROR, "no base file exists");
         rc = SDB_VESSEL_INTERNAL_ERR;
         goto error;
      }

      rc = ((const idMapFile *)base)->getTotalPageCount(oldPageCount);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to get old page count from base:%d", rc);
         goto error;
      }

      file = SDB_OSS_NEW idMapFile();
      if (OSS_UNLIKELY(NULL == file))
      {
         PD_LOG(PDERROR, "failed to allocate mem");
         rc = SDB_OOM;
         goto error;
      }

      rc = _creater.createTmpFile(FILE_TYPE_ID_MAP,
                                  base->getCommonHeadInMem().sequence + 1,
                                  args, file, hs);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to create tmp id map file:%d", rc);
         goto error;
      }

      segmentCount = ossAlignX(totalImpCount, getStorageCoreArgs().maxPageCountPerSeg) /
                     getStorageCoreArgs().maxPageCountPerSeg;

      rc = file->ensureSegmentCountAndInit(segmentCount);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to extend new id map file:%d", rc);
         goto error;
      }

      for (UINT32 i = 0; i < oldPageCount; ++i)
      {
         ossValuePtr src = 0;
         ossValuePtr dst = 0;
         rc = base->getPagePtr(i, src);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to get page[%d] from base:%d", i, rc);
            goto error;
         }
         rc = file->getPagePtr(i, src);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to get page[%d] from file:%d", i, rc);
            goto error;
         }

         ossMemcpy((void *)dst, (const void *)src, getStorageCoreArgs().pageSize);
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

      rc = renameToFormalAndReopen(_creater.getDirSlice(),
                                   FALSE, FILE_SHADOW_SUFFIX_TMP, file);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to rename id map file and reopen:%d", rc);
         goto error;
      }

      rc = _lpidCache.resetBaseFileAndClearFlushedMaps(file);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to reset base file:%d", rc);
         goto error;
      }
      
      _idMapFiles.pushBack(file);
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

   INT32 logicalPageSpace::removeHistoryIdMapAndDeltaLogFiles()
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(isOpen(), "can not be closed");
      static const UINT32 _IMF_MIN_SIZE = 3;
      idMapFile *file = NULL;
      UINT64 sequence = 0;
      UINT64 offset = 0;

      UINT32 size = _idMapFiles.getSize(TRUE);
      if (size < _IMF_MIN_SIZE)
      {
         goto done;
      }

      file = (idMapFile *)(_idMapFiles.getBack());
      sequence = file->getCommonHeadInMem().sequence + 1 - _IMF_MIN_SIZE;
      _idMapFiles.destroyIfLess(sequence);

      file = (idMapFile *)(_idMapFiles.getFront());
      rc = file->getDeltaLogOffset(offset);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to get delta log offset:%d", rc);
         goto error;
      }

      rc = _logConsole.tryToDestroyHistroyFiles(offset);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to destroy history log files:%d", rc);
         goto error;
      }
   done:
      return rc;
   error:
      goto done;
   }

   INT32 logicalPageSpace::restoreAllocatorByBaseFile(const idMapFile *base)
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

      for (UINT32 i = 0; i < getReservedImpCount(); ++i)
      {
         rc = restoreAllocatorByReservedImp(base, i);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to restore allocator by imp[%d], rc:%d",
                   i, rc);
            goto error;
         }
      }
      
      for (UINT32 i = getReservedImpCount(); i < pageCount; ++i)
      {
         rc = restoreAllocatorByImp(base, i);
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

   INT32 logicalPageSpace::restoreAllocatorByReservedImp(const idMapFile *base,
                                                         PAGE_ID pid)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(NULL != base, "can not be null");
      SDB_ASSERT(INVALID_PAGE_ID != pid, "can not be invalid");
      ossValuePtr ptr = 0;
      rc = base->getPagePtr(pid, ptr);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to get page[%d] ptr:%d", pid, rc);
         goto error;
      }

      for (UINT32 i = 0;i < ID_MAP_PAGE_CAPACITY; ++i)
      {
         idMapSlot slot = getIdMapSlot(ptr, i);
         if (slot.isFree())
         {
            continue;
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

   INT32 logicalPageSpace::restoreAllocatorByImp(const idMapFile *base, PAGE_ID pid)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(NULL != base, "can not be null");
      SDB_ASSERT(INVALID_PAGE_ID != pid, "can not be invalid");
      UINT32 baseLpid = pid * ID_MAP_PAGE_CAPACITY;
      ossValuePtr ptr = 0;
      rc = base->getPagePtr(pid, ptr);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to get page[%d] ptr:%d", pid, rc);
         goto error;
      }

      for (UINT32 i = 0; i < ID_MAP_PAGE_CAPACITY; ++i)
      {
         PAGE_ID lpid = baseLpid + i;
         idMapSlot slot = getIdMapSlot(ptr, i);
         if (slot.isFree())
         {
            continue;
         }

         rc = ensureLogicalPidSpace(lpid);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to ensure lpid[%d] space:%d", lpid, rc);
            goto error;
         }

         rc = _allocator.occupy(lpid);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to occupy lpid[%d], rc:%d", lpid, rc);
            goto error;
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

   INT32 logicalPageSpace::replayDeltaLogWhenOpen(UINT64 beginOffset)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(_logConsole.isReady(), "must be ready");
      SDB_ASSERT(_allocator.isInitialized(), "must be inited");
      SDB_ASSERT(NULL != _dpc, "can not be null");
      SDB_ASSERT(_dpc->isOpen(), "must be open");

      deltaLogScanner reader;
      rc = _logConsole.initReaderBeforeAddingNewRecord(beginOffset, reader);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to init log reader:%d", rc);
         goto error;
      }

      do
      {
         UINT64 offset = 0;
         deltaLogRecord dlr;
         rc = reader.getNext(dlr, &offset);
         if (SDB_OK == rc)
         {
            if (!isOperationalDeltaLogRecord(dlr.getLogHead()->_type))
            {
               continue;
            }
            rc = replayLogRecord(dlr);
            if (SDB_OK != rc)
            {
               PD_LOG(PDERROR, "failed to replay log record[%lld], rc:%d", offset, rc);
               goto error;
            }
         }
         else if (SDB_VESSEL_EOC == rc)
         {
            rc = SDB_OK;
            break;
         }
         else
         {
            PD_LOG(PDERROR, "failed to get next record from reader:%d", rc);
            goto error;
         }
      }while (TRUE);
   done:
      return rc;
   error:
      goto done;
   }

   INT32 logicalPageSpace::replayLogRecord(const deltaLogRecord &dlr)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(dlr.isValid(), "must be valid");

      switch (dlr.getLogHead()->_type)
      {
      case DELTA_LOG_TYPE_MAPPING:
         rc = replayMappingLogRecord(dlr);
         break;
      case DELTA_LOG_TYPE_REMAPPING:
         rc = replayRemappingLogRecord(dlr);
         break;
      case DELTA_LOG_TYPE_UNMAPPING:
         rc = replayUnmappingLogRecord(dlr);
         break;
      case DELTA_LOG_TYPE_RELEASING:
         rc = replayReleasingLogRecord(dlr);
         break;
      default:
         rc = SDB_INVALIDARG;
         break;
      }

      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to replay log:%d", rc);
         goto error;
      }
   done:
      return rc;
   error:
      goto done;
   }

   INT32 logicalPageSpace::replayMappingLogRecord(const deltaLogRecord &dlr)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(dlr.isValid(), "must be valid");
      UINT8 count = 0;
      PAGE_SNAPSHOT_VERION psv = INVALID_PAGE_SNAPSHOT_VERSION;

      rc = dlrMappingReader::read(dlr, psv, count);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to read log record:%d", rc);
         goto error;
      }

      for (UINT32 i = 0; i < count; ++i)
      {
         idMapSlot slot;
         mappedLogicalPageId mappedId;
         rc = dlrMappingReader::getItem(dlr, i, mappedId);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to get mapped pid[%d], rc:%d", i, rc);
            goto error;
         }

         if (!isReservedLpid(mappedId.getLpid()))
         {
            rc = ensureLogicalPidSpace(mappedId.getLpid());
            if (SDB_OK != rc)
            {
               PD_LOG(PDERROR, "failed to ensure lpid[%d] space:%d", mappedId.getLpid(), rc);
               goto error;
            }

            rc = _allocator.occupy(mappedId.getLpid());
            if (SDB_OK != rc)
            {
               PD_LOG(PDERROR, "failed to occupy lpid[%d] in allocator:%d",
                      mappedId.getLpid(), rc);
               goto error;
            }
         }

         rc = _dpc->ensurePidSpace(mappedId.getPid());
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to ensure pid[%d] space:%d",
                   mappedId.getPid(), rc);
            goto error;
         }

         rc = _dpc->occupyPage(mappedId.getPid());
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to occupy pid[%d], rc:%d", mappedId.getPid(), rc);
            goto error;
         }

         slot.psv = psv;
         slot.pid = mappedId.getPid();
         rc = _lpidCache.upsertAsImmutable(mappedId.getLpid(), slot);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to put lpid[%d] to cache:%d", mappedId.getLpid(), rc);
            goto error;
         }
      }
   done:
      return rc;
   error:
      goto done;
   }

   INT32 logicalPageSpace::replayRemappingLogRecord(const deltaLogRecord &dlr)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(dlr.isValid(), "can not be invalid");

      PAGE_SNAPSHOT_VERION psv = INVALID_PAGE_SNAPSHOT_VERSION;
      UINT8 flags = 0;
      UINT8 count = 0;

      rc = dlrRemappingReader::read(dlr, psv, flags, count);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to parse log record:%d", rc);
         goto error;
      }

      for (UINT8 i = 0; i < count; ++i)
      {
         idMapSlot slot;
         mappedLogicalPageId mappedId;
         PAGE_ID oldPid = INVALID_PAGE_ID;
         rc = dlrRemappingReader::getItem(dlr, i, mappedId, oldPid);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to get item[%d], rc:%d", i, rc);
            goto error;
         }

         if (!isReservedLpid(mappedId.getLpid()))
         {
            rc = ensureLogicalPidSpace(mappedId.getLpid());
            if (SDB_OK != rc)
            {
               PD_LOG(PDERROR, "failed to ensure lpid[%d] space:%d", mappedId.getLpid(), rc);
               goto error;
            }

            rc = _allocator.occupy(mappedId.getLpid());
            if (SDB_OK != rc)
            {
               PD_LOG(PDERROR, "failed to occupy lpid[%d] in allocator:%d",
                      mappedId.getLpid(), rc);
               goto error;
            }
         }

         rc = _dpc->ensurePidSpace(mappedId.getPid());
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to ensure pid[%d] space:%d",
                   mappedId.getPid(), rc);
            goto error;
         }

         rc = _dpc->occupyPage(mappedId.getPid());
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to occupy pid[%d], rc:%d", mappedId.getPid(), rc);
            goto error;
         }

         slot.psv = psv;
         slot.pid = mappedId.getPid();
         rc = _lpidCache.upsertAsImmutable(mappedId.getLpid(), slot);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to put lpid[%d] to cache:%d", mappedId.getLpid(), rc);
            goto error;
         }

         if (0 != OSS_BIT_TEST(flags, DELTA_LOG_TYPE_REMAPPING_FLAG_RELEASE_PID))
         {
            _dpc->releasePage(oldPid);
         }
      }
   done:
      return rc;
   error:
      goto done;
   }

   INT32 logicalPageSpace::replayUnmappingLogRecord(const deltaLogRecord &dlr)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(dlr.isValid(), "can not be invalid");
      UINT8 count = 0;
      UINT8 flags = 0;

      rc = dlrUnmapingReader::read(dlr, flags, count);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to read unmapping log record:%d", rc);
         goto error;
      }

      SDB_ASSERT(0 < count, "impossible");
      for (UINT8 i = 0; i < count; ++i)
      {
         mappedLogicalPageId mappedId;
         rc = dlrUnmapingReader::getItem(dlr, i, mappedId);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to get lpid:%d", rc);
            goto error;
         }

         if (!isReservedLpid(mappedId.getLpid()))
         {
            _allocator.release(mappedId.getLpid());
         }

         rc = _lpidCache.remove(mappedId.getLpid());
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to remove lpid[%d] in cache:%d",
                   mappedId.getLpid(), rc);
            goto error;
         }

         if (0 != OSS_BIT_TEST(flags, DELTA_LOG_TYPE_UNMAPPING_FLAG_RELEASE_PID))
         {
            _dpc->releasePage(mappedId.getPid());
         }

      }
   done:
      return rc;
   error:
      goto done;
   }

   INT32 logicalPageSpace::replayReleasingLogRecord(const deltaLogRecord &dlr)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(dlr.isValid(), "must be valid");
      UINT8 count = 0;
      rc = dlrReleasingReader::read(dlr, count);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to read count from reader:%d", rc);
         goto error;
      }

      for (UINT8 i = 0; i < count; ++i)
      {
         PAGE_ID pid = INVALID_PAGE_ID;
         rc = dlrReleasingReader::getItem(dlr, i, pid);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to get pid:%d", rc);
            goto error;
         }

         _dpc->releasePage(pid);
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
      if (minCount <= _allocator.getPageCount())
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
      else if ((_allocator.getPageCount() * ID_MAP_PAGE_CAPACITY) <= lpid)
      {
         rc = SDB_VESSEL_LOGICAL_PAGE_UNMAPPED;
         goto error;
      }

   done:
      return rc;
   error:
      goto done;
   }

   INT32 logicalPageSpace::getPageFromCache(PAGE_ID lpid,
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
   done:
      return rc;
   error:
      goto done;
   }

}//namespace vessel
}//namespace engine