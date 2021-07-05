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
#include "vessel/deltaLogReader.h"
#include "vessel/deltaLogRecordBuilder.h"
#include "ossLatchGuard.hpp"
#include "vessel/redoLogUtil.h"
#include "vessel/deltaLogRecord.h"

namespace engine
{
namespace vessel
{
///////////////logicalPageSpace::_runtimePageBufferIniter begin
   INT32 logicalPageSpace::
         _runtimePageBufferIniter::
         initWithLiteCache(requestContext *context,
                           const GLOBAL_PAGE_ID &gpid,
                           ossSharedLatch::mode mode,
                           const runtimePageBuffer::options &options,
                           UINT32 pageSize,
                           runtimePageBuffer &rpb)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(!rpb.isValid(), "can not be valid");
      SDB_ASSERT(!rpb._tuple.isValid(), "can not be valid");
      liteCacheAllocateOptions o;
      if (OSS_UNLIKELY(NULL == context ||
                       !gpid.isValid() ||
                       ossSharedLatch::NONE == mode ||
                       rpb.isValid() ||
                       !isValidPageSize(pageSize)))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      o.lockMode = mode;
      o.resetPage = options.resetPage;
      rc = context->getEnv()->cacheConsole.allocate(context, gpid, options, rpb._tuple);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to allocate cache tuple:%d", rc);
         goto error;
      }

      rc = rpb.init(gpid, options, pageSize);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to init rpb:%d", rc);
         goto error;
      }
   done:
      return rc;
   error:
      rpb.fini();
      goto done;
   }

   INT32 logicalPageSpace::
         _runtimePageBufferIniter::
         initWithMmap(requestContext *context,
                     const GLOBAL_PAGE_ID &gpid,
                     const runtimePageBuffer::options &options,
                     UINT32 pageSize,
                     const mmapPagePointer &ptr,
                     runtimePageBuffer &rpb)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(!rpb.isValid(), "can not be valid");
      SDB_ASSERT(!rpb._tuple.isValid(), "can not be valid");

      if (OSS_UNLIKELY(NULL == context ||
                       !gpid.isValid() ||
                       rpb.isValid() ||
                       !isValidPageSize(pageSize) ||
                       !ptr.isValid()))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      rc = rpb.init(gpid, options, pageSize, ptr);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to init rpb:%d", rc);
         goto error;
      }
   done:
      return rc;
   error:
      rpb.fini();
      goto done;
   }

///////////////logicalPageSpace::_runtimePageBufferIniter end

   OSS_INLINE getRpbLockMode(ossSharedLatch::mode lpbLockMode)
   {
      SDB_ASSERT(lpbLockMode != ossSharedLatch::NONE, "impossible");
      if (ossSharedLatch::SHARED == lpbLockMode)
      {
         return ossSharedLatch::SHARED;
      }
      else if (ossSharedLatch::UPGRADE == mode)
      {
         return ossSharedLatch::UPGRADE;
      }
      else
      {
         return ossSharedLatch::UPGRADE;
      }
   }

   logicalPageSpace::~logicalPageSpace()
   {
      _close();
   }

   void logicalPageSpace::close()
   {
      _close();
      return;
   }

   void logicalPageSpace::_close()
   {
      _creater.fini();
      _idMapFiles.fini();
      _allocator.fini();
      _logConsole.fini();
      _lpidCache.fini();
      if (NULL != _dpc)
      {
         _dpc->close();
         SDB_OSS_DEL _dpc;
         _dpc = NULL;
      }
      _checkpointContext.fini();
      return;
   }

   void logicalPageSpace::destroy()
   {
      _destroy();
      return;
   }

   void logicalPageSpace::_destroy()
   {
      if (_logConsole.isReady())
      {
         _logConsole.destroy();
      }

      if (NULL != _dpc)
      {
         _dpc->destroy();
         SDB_OSS_DEL _dpc;
         _dpc = NULL;
      }

      _idMapFiles.destroy();
      _close();
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
      rc = createFirstIdMapFile();
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to create id map file:%d", rc);
         goto error;
      }
      baseFile = (const idMapFile *)(_idMapFiles.getLast());
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
      _dpc = allocateStorageObject();
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

      base = (const idMapFile *)(_idMapFiles.getLast());
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
      if (!validateIdMapFileHeadFlags(baseHead.flags))
      {
         PD_LOG(PDERROR, "invalid id map file head flags:%d", baseHead.flags);
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

      if (0 < baseHead.totalPageCount)
      {
         if (baseHead.totalPageCount < getReservedImpCount())
         {
            PD_LOG(PDERROR, "total page count[%d] in base file less than reserved count[%d]",
                   baseHead.totalPageCount, getReservedImpCount());
            rc = SDB_VESSEL_INTERNAL_ERR;
            goto error;
         }
         else if (getReservedImpCount() < baseHead.totalPageCount)
         {
            rc = _allocator.allocateNewBitMapPages(baseHead.totalPageCount - getReservedImpCount(), 0);
            if (SDB_OK != rc)
            {
               PD_LOG(PDERROR, "failed to allocate new bitmap pages:%d", rc);
               goto error;
            }
         }
         else
         {
            /// do nothing.
         }
      }
      
      /// init page storage
      _dpc = allocateStorageObject();
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
      rc = _open(context);
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

   INT32 logicalPageSpace::blockCheckpoint(requestContext *context)
   {
      INT32 rc = SDB_OK;
      if (OSS_UNLIKELY(!isOpen()))
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }
      
      SDB_ASSERT(context->getSpaceID() == _creater.getSpaceID(), "impossible");
      rc = context->getBlocker().block(_creater.getSpaceID(),
                                       getSpaceType(),
                                       _checkpointContext.getLatch());
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to block lps[%d,%d] checkpoint, rc:%d",
                _creater.getSpaceID(), getSpaceType(), rc);
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
      if (OSS_UNLIKELY(!isOpen()))
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }
      
      SDB_ASSERT(context->getSpaceID() == _creater.getSpaceID(), "impossible");
      rc = context->getBlocker().tryToBlock(_creater.getSpaceID(),
                                            getSpaceType(),
                                            _checkpointContext.getLatch(),
                                            blocked);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to block lps[%d,%d] checkpoint, rc:%d",
                _creater.getSpaceID(), getSpaceType(), rc);
         goto error;
      }
   done:
      return rc;
   error:
      goto done;
   }

   INT32 logicalPageSpace::getLogicalPageBuffer(requestContext *context,
                                                PAGE_ID lpid,
                                                ossSharedLatch::mode mode,
                                                logicalPageBuffer &lpb)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(!lpb.isValid(), "impossible");
      SDB_ASSERT(context->getSpaceID() == _creater.getSpaceID(), "must be same");
      BOOLEAN isMutablePage = FALSE;
      idMapSlot slot;
      runtimePageBuffer::options o;

      if (OSS_UNLIKELY(!isOpen()))
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }
      else if (OSS_UNLIKELY(NULL == context ||
                            INVALID_PAGE_ID == lpid ||
                            ossSharedLatch::NONE == mode))
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

      rc = _lpidCache.get(lpid, slot, isMutablePage);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to find lpid[%d] in cache:%d", lpid, rc);
         goto error;
      }

      o.resetPage = FALSE;
      o.noPageValidation = FALSE;
      rc = getRuntimePageBuffer(context, slot.pid,
                                getRpbLockMode(mode),
                                o, lpb._rpb);
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
      storageConsole *sc = NULL;
      BOOLEAN snapshotEffective = FALSE;
      CHAR *buffer = NULL;
      UINT32 bufferSize = 0;
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
      else if (OSS_UNLIKELY(ossSharedLatch::NONE == lpb._lh.getLockMode() ||
                            ossSharedLatch::SHARED == lpb._lh.getLockMode()))
      {
         rc = SDB_VESSEL_FORBIDDEN_OP_WLT;
         goto error;
      }
      else if (rpb.isWritingPrepared())
      {
         SDB_ASSERT(ossSharedLatch::EXCLUSIVE == lpb._lh.getLockMode(), "impossible");
         goto done;
      }

      if (ossSharedLatch::UPGRADE == lpb._lh.getLockMode())
      {
         lpb._lh.unlockUpgradeAndLock();
      }

      sc = &(context->getEnv()->sc);
      rc = sc->isSnapshotEffective(_creater.getSpaceID(),
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

      if (snapshotEffective ||
          (isCopyOnWrite() && !rpb.isMutable()))
      {
         bufferSize = rpb.getPageSize();
         buffer = context->allocateBuffer(bufferSize);
         if (NULL == buffer)
         {
            PD_LOG(PDERROR, "failed to allocate mem");
            rc = SDB_OOM;
            goto error;
         }

         ossMemcpy(buffer, rpb.getReadOnlyBuffer(), bufferSize);
         if (rpb.isCacheBuffer())
         {
            rpb.getLiteCacheTuple().release();
         }

         rc = remapToNewDataPage(context, rpb.getLpid(),
                                 rpb.getGlobalPid().page(),
                                 buffer, slot);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to remap lpid[%d], rc:%d", rpb.getLpid(), rc);
            goto error;
         }
      }
   done:
      if (NULL != buffer)
      {
         context->releaseBuffer(buffer, bufferSize);
      }
      return rc;
   error:
      goto done;
   }

   INT32 logicalPageSpace::ensureReservedPage(requestContext *context,
                                              PAGE_ID lpid,
                                              pageInitializer *initer,
                                              logicalPageBuffer &lpb)
   {
      INT32 rc = SDB_OK;
      lpb.fini();
      BOOLEAN blocked = FALSE;
      idMapSlot slot;
      BOOLEAN isMutable = FALSE;
      ossSharedLatch::mode lpidLockMode = ossSharedLatch::UPGRADE;
      runtimePageBuffer::options o;

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

      rc = blockCheckpoint(context);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to block checkpoint:%d", rc);
         goto error;
      }
      blocked = TRUE;

      rc = lpb._lh.lock(context, getSpaceType(), lpid, lpidLockMode);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to lock lpid[%d], rc:%d", lpid, rc);
         goto error;
      }

      rc = _lpidCache.get(lpid, slot, isMutable);
      if (SDB_OK == rc)
      {
         o.noPageValidation = FALSE;
         o.resetPage = FALSE;
         rc = getRuntimePageBuffer(context, slot.pid
                                   getRpbLockMode(lpidLockMode),
                                   o, lpb._rpb);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to get rpb of pid[%d], rc:%d", slot.pid, rc);
            goto error;
         }
         lpb.init(this, slot.psv, isMutable);
      }
      else if (SDB_VESSEL_LOGICAL_PAGE_UNMAPPED == rc)
      {
         rc = _dpc->allocatePage(pid);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to allocate new pid:%d", rc);
            goto error;
         }
      }
      else
      {
         PD_LOG(PDERROR, "failed to get lpid[%d] in cache:%d", lpid, rc);
         goto error;
      }
   done:
      if (blocked)
      {
         context->getBlocker().unblock();
      }
      return rc;
   error:
      lpb.fini();
      goto done;
   }

   INT32 logicalPageSpace::createPageAndCompleteBuffer(requestContext *context,
                                                       pageInitializer *initer,
                                                       logicalPageBuffer &lpb)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(NULL != context, "can not be null");
      SDB_ASSERT(NULL != initer, "can not be null");
      lpidLockHelper &lh = lpb._lh;
      runtimePageBuffer &rpb = lpb._rpb;
      SDB_ASSERT(lh.getLockMode() == ossSharedLatch::EXCLUSIVE, "must be exclusive");
      SDB_ASSERT(!rpb.isValid(), "can not be valid");
      runtimePageBuffer::options o;
      PAGE_ID lpid = lh.getLpid();
      PAGE_ID pid = INVALID_PAGE_ID;
      PAGE_SNAPSHOT_VERION psv = context->getEnv()->sc.getOnlinePageSnapshotVersion();

      rc = _dpc->allocatePage(pid);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to allocate new pid:%d", rc);
         goto error;
      }

      o.noPageValidation = TRUE;
      o.resetPage = TRUE;
      rc = getRuntimePageBuffer(context, pid,
                                ossSharedLatch::EXCLUSIVE,
                                o, rpb);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to get runtime buffer of pid[%d], rc:%d", pid, rc);
         goto error;
      }

      rc = initer->init(context, lpid, psv, rpb);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to init page[%d], rc:%d", pid, rc);
         goto error;
      }

      rc = map(context, psv, 1, &lpid, &pid);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to create new mapping[%d,%d], rc:%d",
                lpid, pid, rc);
         goto error;
      }

      lpb.init(this, psv, TRUE);
   done:
      return rc;
   error:
      rpb.fini();
      if (INVALID_PAGE_ID != pid)
      {
         _dpc->releasePage(pid);
      }
      goto done;
   }

   INT32 logicalPageSpace::remapBufferToNewDataPage(requestContext *context,
                                                    BOOLEAN releaseOld,
                                                    logicalPageBuffer &lpb)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(NULL != context, "can not be null");
      SDB_ASSERT(lpb.isValid(), "can not be invalid");
      SDB_ASSERT(ossSharedLatch::EXCLUSIVE == lpb._lh.getLockMode(), "must be exclusive");

      runtimePageBuffer &rpb = lpb._rpb;
      PAGE_SNAPSHOT_VERION onlinePsv = context->getEnv()->sc.getOnlinePageSnapshotVersion();
      UINT32 pageSize = _dpc->getCoreArgs().pageSize;
      PAGE_ID lpid = lpb.getLogicalPid();
      PAGE_ID oldPid = rpb.getGlobalPid().page();
      PAGE_ID newPid = INVALID_PAGE_ID;
      PAGE_ID pidToRollback = INVALID_PAGE_ID;

      DPS_LSN_OFFSET lsn = DPS_INVALID_LSN_OFFSET;

      /// 1. allocate new pid
      rc = _dpc->allocatePage(newPid);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to allocate data page:%d", rc);
         goto error;
      }
      pidToRollback = newPid;

      /// 2. copy data to new page
      if (_dpc->isStandardPage())
      {
         ossValuePtr newPagePtr = 0;
         rc = _dpc->getDataPagePtr(pid, newPagePtr);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to get page[%d] ptr:%d", pid, rc);
            goto error;
         }
         ossMemcpy((void *)newPagePtr, rpb.getReadOnlyBuffer(), pageSize);
         ((pageHead *)newPagePtr)->psv = onlinePsv;
      }
      else
      {
         rc = _dpc->copyNonstandardPage(oldPid, onlinePsv, pid);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to copy data from [%d] to [%d], rc:%d",
                   oldPid, pid, rc);
            goto error;
         }
      }
      rpb.fini();/// rpb is no longer needed.

      /// 3. remap
      if (isReplicated())
      {
         rc = replicatedRemmap(context, onlinePsv, 1, &lpid,
                              &pid, &oldPid, &lsn);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to remap lpid[%d] to new pid[%d], rc:%d",
                     lpid, pid, rc);
            goto error;
         }
      }

      pidToRollback = INVALID_PAGE_ID;
      /// 4. reinit runtime buffer
      rc = _getRuntimePageBuffer(context, pid, FALSE, rpb);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to get runtime buffer of pid[%d], rc:%d",
                pid, rc);
         goto error;
      }

   done:
      return rc;
   error:
      if (INVALID_PAGE_ID != pidToRollback)
      {
         _dpc->releasePage(pidToRollback);
      }
      goto done;
   }

   INT32 logicalPageSpace::allocatePages(requestContext *context,
                                         const pageInitializer *initer,
                                         UINT32 count,
                                         PAGE_ID *lpids,
                                         atomicOperationList *oplist)
   {
      INT32 rc = SDB_OK;

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

   done:
      return rc;
   error:
      goto done;
   }

   INT32 logicalPageSpace::preallocatePages(requestContext *context,
                                            const pageInitializer *initer,
                                            UINT32 count,
                                            preallocatedPage *pages)
   {
      INT32 rc = SDB_OK;
      PAGE_ID *pidBuffer = NULL;
      PAGE_ID *lpidBuffer = NULL;
      UINT32 bufferSize = sizeof(PAGE_ID) * count;
      BOOLEAN rollbackPid = FALSE;
      BOOLEAN rollbackLpid = FLASE;

      if (OSS_UNLIKELY(!isOpen()))
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }
      else if (OSS_UNLIKELY(NULL == context ||
                            NULL == initer ||
                            0 == count ||
                            NULL == pages))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      pidBuffer = (PAGE_ID *)(context->allocateBuffer(bufferSize));
      lpidBuffer = (PAGE_ID *)(context->allocateBuffer(bufferSize));
      if (OSS_UNLIKELY(NULL == pidBuffer ||
                       NULL == lpidBuffer))
      {
         PD_LOG(PDERROR, "failed to allocate mem");
         rc = SDB_OOM;
         goto error;
      }

      /// We should always prealloate logical pid first.
      /// Because id map file may hit the max size.
      rc = preallocateLogicalPids(context, count, lpidBuffer);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to preallocate logical pids:%d", rc);
         goto error;
      }
      rollbackLpid = TRUE;

      rc = preallocatePhysicalPids(context, count, pidBuffer);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to preallocate physical pids:%d", rc);
         goto error;
      }
      rollbackPid = TRUE;

      for (UINT32 i = 0; i < count; ++i)
      {
         ossValuePtr ptr = 0;
         rc = getPhysicalPagePtr(pidBuffer[i], ptr);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to get physical page ptr:%d", rc);
            goto error;
         }
         pages[i].reset(pidBuffer[i], lpidBuffer[i], ptr);
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
      if (rollbackLpid)
      {
         releaseLogicalPidsPreallocated(context, ocunt, lpidBuffer);
      }
      if (rollbackPid)
      {
         releasePhysicalPidsPreallocated(context, count, pidBuffer);
      }
      goto done;
   }

   INT32 logicalPageSpace::rollbackOplist(requestContext *context,
                                          atomicOperationList *oplist)
   {
      SDB_ASSERT(FALSE, "TODO");
      return SDB_OK;
   }

   INT32 logicalPageSpace::openIdMapFiles(SPACE_ID sid,
                                          const strSlice &dir,
                                          const storageFileLoader &loader)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(NULL == _base, "must be null");
      SDB_ASSERT(INVALID_SPACE_ID != sid, "can not be invalid");
      SDB_ASSERT(!dir.empty(), "can not be empty");

      idMapFile *file = NULL;

      const FILE_NAME_LIST *list = loader.getFileList(FILE_TYPE_ID_MAP);
      if (NULL == list || list->empty())
      {
         PD_LOG(PDERROR, "id map file not found in dir[%s]", dir.str());
         if (!loader.isEmpty())
         {
            PD_LOG(PDERROR, "loader is not empty but no id map file found");
            rc = SDB_VESSEL_INTERNAL_ERR;
            goto error;
         }
         else
         {
            PD_LOG(PDERROR, "no file found at all");
            rc = SDB_VESSEL_LPS_NOT_EXISTS;
            goto error;
         }
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

         rc = _imfList.insertFile(file);
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
      _imfList.fini();
      goto done;
   }

   INT32 logicalPageSpace::validateIdMapFileMap()
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(!_idMapFiles.isEmpty(), "can not be empty");
      const idMapFile *base = NULL;
      idMapFileHead baseHead;

      base = (const idMapFile *)(_idMapFiles.getLast());
      rc = base->getIdMapFileHead(baseHead);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to get imf head of file[%s], rc:%d",
                base->getFullPath(), rc);
         goto error;
      }

      if (this->isReplicated() && !baseHead.isReplicated())
      {
         PD_LOG(PDERROR, "invalid flag: isReplicated");
         rc = SDB_VESSEL_INTERNAL_ERR;
         goto error;
      }
      else if(!this->isReplicated() && baseHead.isReplicated())
      {
         PD_LOG(PDERROR, "invalid flag: isReplicated");
         rc = SDB_VESSEL_INTERNAL_ERR;
         goto error;
      }

      if (this->isCopyOnWrite() && !baseHead.isCopyOnWrite())
      {
         PD_LOG(PDERROR, "invalid flag: isCopyOnWrite");
         rc = SDB_VESSEL_INTERNAL_ERR;
         goto error;
      }
      else if (!this->isCopyOnWrite() && aseHead.isCopyOnWrite())
      {
         PD_LOG(PDERROR, "invalid flag: isCopyOnWrite");
         rc = SDB_VESSEL_INTERNAL_ERR;
         goto error;
      }

      for (storageFileMap::CONST_ITERATOR itr = _idMapFiles.begin();
           itr != _idMapFiles.end(); ++itr)
      {
         const idMapFile *fileInMap = (const idMapFile *)(*itr);
         idMapFileHead h;
         if (fileInList->getCommonHeadInMem().sequence ==
             base->getCommonHeadInMem().sequence)
         {
            break;
         }

         rc = fileInMap->getIdMapFileHead(h);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to get id map file head of file[%s], rc:%d",
                   fileInList->getFullPath(), rc);
            goto error;
         }

         if (!head.hasSameArgs(h))
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

   INT32 logicalPageSpace::openFiles(requestContext *context,
                                     SPACE_ID sid,
                                     const std::string &dir,
                                     idMapFile **out)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(NULL != context, "can not be null");
      SDB_ASSERT(INVALID_SPACE_ID != sid, "can not be invalid");
      SDB_ASSERT(!dir.empty(), "can not be empty");

      idMapFile *file = NULL;
      vesselFileName fn;
      ossPoolSet<FILE_TYPE> types = {FILE_TYPE_DATA_STORAGE};
      FILE_LIST fl;

      if (OSS_UNLIKELY(!fn.build(sid, FILE_TYPE_ID_MAP, getSpaceType())))
      {
         PD_LOG(PDERROR, "failed to build id map file name");
         rc = SDB_VESSEL_INTERNAL_ERR;
         goto error;
      }

      file = SDB_OSS_NEW idMapFile();
      if (NULL == file)
      {
         PD_LOG(PDERROR, "failed to allocate mem");
         rc = SDB_OOM;
         goto error;
      }

      rc = file->open(_dir, idmapFn);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to open id map file:%d", rc);
         goto error;
      }
      
      rc = file->getIdMapFileHead(head);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to id map file head:%d", rc);
         goto error;
      }

      rc = createFileListUnderPath(dir, sid, types, TRUE, fl);
      if (SDB_OK!= rc)
      {
         PD_LOG(PDERROR, "failed to create file list:%d", rc);
         goto error;
      }

      rc = openDataStorageFiles(context, file, dir, fl);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to open data storage files:%d", rc);
         goto error;
      }

      if (NULL != out)
      {
         *out = file;
      }
   done:
      return rc;
   error:
      if (NULL != file)
      {
         file->close();
         SDB_OSS_DEL file;
      }
      goto done;
   }

   INT32 logicalPageSpace::openDataStorageFiles(requestContext *context,
                                                idMapFile *imf,
                                                const std::string &dir,
                                                const FILE_LIST &fl)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(NULL != context, "can not be null");
      SDB_ASSERT(!_dsc.isOpen(), "must be open");
      SDB_ASSERT(!dir.empty(), "can not be empty");
      SDB_ASSERT(NULL != imf, "can not be null");

      storageFile *file = NULL;
      idMapFileHead head;
      storageCoreArgs args;
      SPACE_ID sid = imf->getCommonHeadInMem().spaceID;

      rc = _imf->getIdMapFileHead(head);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to get id map file head:%d", rc);
         goto error;
      }

      args.pageSize = head.dataPageSize;
      args.maxPageCountPerSeg = head.dataPageCountInSeg;
      args.maxSegmentCountPerFile = head.dataSegCountInFile;
      if (OSS_UNLIKELY(!args.isValid()))
      {
         PD_LOG(PDERROR, "invalid core args");
         rc = SDB_VESSEL_INTERNAL_ERR;
         goto error;
      }

      rc = _dsc.open(args);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to open data storage cluster:%d", rc);
         goto error;
      }

      for (FILE_LIST::const_iterator itr = fl.begin();
           itr != fl.end(); ++itr)
      {
         const vesselFileName &fn = *itr;
         if (!fn.isValid() ||
             fn.getSpaceID() != sid ||
             fn.getSpaceType() != getSpaceType() ||
             fn.getFileType() != FILE_TYPE_DATA_STORAGE ||
             fn.hasShadowSuffix())
         {
            PD_LOG(PDERROR, "invalid file %s to open", fn.getFileName());
            rc = SDB_VESSEL_INVALID_VESSEL_FILE;
            goto error;
         }
      
         file = SDB_OSS_NEW storageFile();
         if (NULL == file)
         {
            PD_LOG(PDERROR, "failed to allocate mem");
            rc = SDB_OOM;
            goto error;
         }

         rc = file->open(dir, fn);
         if (SDB_VESSEL_CRASHED_WHEN_CREATING == rc)
         {
            PD_LOG(PDERROR, "failed to open file:%s, rc:%d", fn.getFileName(), rc);
            goto error;
         }

         rc = _dsc.depositDataStorageFile(file);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to add file to cluster:%s, :%d", fn.getFileName(), rc);
            goto error;
         }

         file = NULL;
      }
   done:
      return rc;
   error:
      SAFE_OSS_DELETE(file);
      _dsc.close();
      goto done;
   }

   INT32 logicalPageSpace::getStorageCoreArgs(FILE_TYPE type, storageCoreArgs &args)const
   {
      INT32 rc = SDB_OK;
      if (OSS_UNLIKELY(!isOpen()))
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }
      else if (_dpc->getDataFileType() == type)
      {
         args = _dpc->getCoreArgs();
      }
      else
      {
         rc = SDB_VESSEL_INTERNAL_ERR;
         goto error;
      }
   done:
      return rc;
   error:
      goto done;
   }


   INT32 logicalPageSpace::preallocate(requestContext *context,
                                       UINT32 count,
                                       PAGE_ID *lpids,
                                       PAGE_ID *pids)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(isOpen(), "can not be closed");
      if (OSS_UNLIKELY(NULL == context ||
                       0 == count ||
                       NULL == lpids ||
                       NULL == pids))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      rc = preallocateLpids(context, count, lpids);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to preallocate lpids:%d", rc);
         goto error;
      }

      rc = _dpc->allocatePages(count, pids);
      if (SDB_OK != rc)
      {
         releaseLpidsPreallocated(context, count, lpids);
         PD_LOG(PDERROR, "failed to allocate pids:%d", rc);
         goto error;
      }
   done:
      return rc;
   error:
      goto done;
   }

   void logicalPageSpace::releasePreallocated(requestContext *context,
                                              UINT32 count,
                                              const PAGE_ID *lpids,
                                              const PAGE_ID *pids)
   {
      SDB_ASSERT(isOpen(), "can not be closed");
      SDB_ASSERT(NULL != context, "can not be null");
      SDB_ASSERT(0 < count, "can not be zero");
      SDB_ASSERT(NULL != lpids, "can not be null");
      SDB_ASSERT(NULL != pids, "can not be null");
      releaseLpidsPreallocated(context, count, lpids);
      _dpc->releasePages(count, pids);
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

   INT32 logicalPageSpace::createFirstIdMapFile()
   {
      INT32 rc = SDB_OK;

      SDB_ASSERT(_creater.isValid(), "must be valid");
      SDB_ASSERT(_dataArgs.isValid(), "must be valid");
      SDB_ASSERT(_idMapFiles.isEmpty(), "must be empty");
      SDB_ASSERT(!(isReplicated() && isCopyOnWrite()), "not supported yet");

      idMapFile *file = NULL;
      storageCoreArgs args(ID_MAP_FILE_PAGE_SIZE,
                           ID_MAP_FILE_MAX_PAGE_COUNT_IN_SEG,
                           ID_MAP_FILE_MAX_SEG_COUNT_IN_FILE);

      idMapFileHead imfHead;
      imfHead.version = ID_MAP_FILE_HEAD_VERSION;
      imfHead.flags = getIdMapFileHeadFlags();
      imfHead.dataPageSize = _dataArgs.pageSize;
      imfHead.dataPageCountInSeg = _dataArgs.maxPageCountPerSeg;
      imfHead.dataSegCountInFile = _dataArgs.maxSegmentCountPerFile;
      imfHead.totalPageCount = 0;
      imfHead.deltaLogOffset = DPS_INVALID_LSN_OFFSET;

      slice hs(sizeof(idMapFileHead), &imfHead);

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

      _idMapFiles.insert(file);
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


   INT32 logicalPageSpace::crossCheckBetweenIdMapFileAndFile(const storageFile *file)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(NULL != context, "can not be null");
      SDB_ASSERT(_dsc.isOpen(), "can not be empty");
      SDB_ASSERT(NULL != _imf, "can not be null");
      SDB_ASSERT(_imf->isOpen(), "can not be closed");
      const storageFileHead &commonHead = _imf->getCommonHeadInMem();
      idMapFileHead head;
      storageCoreArgs args;
      UINT32 dataFileCount = 0;

      rc = _imf->getIdMapFileHead(head);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to get id map file head:%d", rc);
         goto error;
      }

      args.pageSize = head.dataPageSize;
      args.maxPageCountPerSeg = head.dataPageCountInSeg;
      args.maxSegmentCountPerFile = head.dataSegCountInFile;
      if (OSS_UNLIKELY(!args.isValid()))
      {
         PD_LOG(PDERROR, "invalid core args stored in id map head");
         rc = SDB_VESSEL_INTERNAL_ERR;
         goto error;
      }

      if (args != _dsc.getInMemDataCoreArgs())
      {
         PD_LOG(PDERROR, "core args not same");
         rc = SDB_VESSEL_INVALID_VESSEL_FILE;
         goto error;
      }

      dataFileCount = _dsc.getDataStorageFileCount();
      for (UINT32 i = 0; i < dataFileCount; ++i)
      {
         storageFile *file = NULL;
         const storageFileHead *h = NULL;

         rc = _dsc.getDataStorageFile(i, &file);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to get data file with sequence[%d]", i);
            goto error;
         }

         h = &(file->getCommonHeadInMem());
         if (commonHead.secretValue != h->secretValue)
         {
            PD_LOG(PDERROR, "sercret value not same");
            rc = SDB_VESSEL_INVALID_VESSEL_FILE;
            goto error;
         }
         if (commonHead.spaceID != h->spaceID)
         {
            PD_LOG(PDERROR, "space id not same");
            rc = SDB_VESSEL_INVALID_VESSEL_FILE;
            goto error;
         }
         if (commonHead.spaceType != h->spaceType)
         {
            PD_LOG(PDERROR, "space type not same");
            rc = SDB_VESSEL_INVALID_VESSEL_FILE;
            goto error;
         }
         if (commonHead.logicalID != h->logicalID)
         {
            PD_LOG(PDERROR, "logical id not same");
            rc = SDB_VESSEL_INVALID_VESSEL_FILE;
            goto error;
         }
         if (args.pageSize != h->pageSize)
         {
            PD_LOG(PDERROR, "page sizes not same");
            rc = SDB_VESSEL_INVALID_VESSEL_FILE;
            goto error;
         }
         if (args.maxPageCountPerSeg != h->maxPageCountPerSeg)
         {
            PD_LOG(PDERROR, "max page per segment not same");
            rc = SDB_VESSEL_INVALID_VESSEL_FILE;
            goto error;
         }  
         if (args.maxSegmentCountPerFile != h->maxSegmentCountPerFile)
         {
            PD_LOG(PDERROR, "max segment count per file not same");
            rc = SDB_VESSEL_INVALID_VESSEL_FILE;
            goto error;
         }
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

      deltaLogReader reader;
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
         rc = _lpidCache.upsert(mappedId.getLpid(), slot, FALSE);
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
         rc = _lpidCache.upsert(mappedId.getLpid(), slot, FALSE);
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

         /// lpid space should be allocated already.
         if (!isReservedLpid(mappedId.getLpid()))
         {
            _allocator.release(mappedId.getLpid());
         }

         rc = _lpidCache.remove(lpid.getLpid());
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to remove lpid[%d] in cache:%d",
                   lpid.getLpid(), rc);
            goto error;
         }

         if (0 != OSS_BIT_TEST(flags, DELTA_LOG_TYPE_UNMAPPING_FLAG_RELEASE_PID))
         {
            _dpc->releasePage(lpid.getPid());
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
      SDB_ASSERT(4096 == ID_MAP_PAGE_CAPACITY, "must be 4k");
      return lpid < (getReservedImpCount() << 12);
   }

   INT32 logicalPageSpace::validateLpidBeforeGet(PAGE_ID lpid)const
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(4096 == ID_MAP_PAGE_CAPACITY, "must be 4k");

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
      else if (_allocator.getPageCount() <= (lpid >> 12))
      {
         rc = SDB_VESSEL_LOGICAL_PAGE_UNMAPPED;
         goto error;
      }

   done:
      return rc;
   error:
      goto done;
   }

   dataPageCluster *logicalPageSpace::allocateStorageObject()
   {
      return SDB_OSS_NEW dataStorageFileCluster();
   }


   INT32 logicalPageSpace::replicatedRemmap(requestContext *context,
                                            PAGE_SNAPSHOT_VERION psv,
                                            UINT32 count,
                                            const PAGE_ID *lpids,
                                            const PAGE_ID *newPids,
                                            const PAGE_ID *oldPids,
                                            DPS_LSN_OFFSET *lsn)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(NULL != context, "can not be null");
      SDB_ASSERT(INVALID_PAGE_SNAPSHOT_VERSION != psv, "can not be invalid");
      SDB_ASSERT(0 < count, "can not be zero");
      SDB_ASSERT(count < 256, "can not over 1 byte");
      SDB_ASSERT(NULL != lpids, "can not be null");
      SDB_ASSERT(NULL != newPids, "can not be null");
      SDB_ASSERT(NULL != oldPids, "can not be null");

      ossSpinXLatchGuard guard(&_mappingLatch, FALSE); /// do not get latch here.
      UINT64 deltaLogOffset = DPS_INVALID_LSN_OFFSET;
      logRecordContext lrc;
      deltaLogRecordBuilder builder;

      rc = builder.buildRemappingLog(psv, count, lpids, newPids, oldPids, FALSE);
      if (OSS_UNLIKELY(SDB_OK != rc))
      {
         PD_LOG(PDERROR, "failed to build remapping log:%d", rc);
         goto error;
      }

      guard.lock();
      /// 1. reserve lsn
      rc = lpsLogUtil::prepare(context, slice(),
                               builder.getDeltaLogRecord(),
                               lrc);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to prepare global log record:%d", rc);
         goto error;
      }

      /// 2. write local delta log
      rc = _logConsole.append(builder.getDeltaLogRecord(), &deltaLogOffset);
      if (SDB_OK != rc)
      {
         guard.unlock();
         PD_LOG(PDERROR, "failed to append record to delta log, reserved lsn[%lld], rc:%d",
                lrc.getLsn(), rc);
         INT32 tmpRc = lpsLogUtil::abort(context, lrc);
         if (SDB_OK != tmpRc)
         {
            PD_LOG(PDSEVERE, "failed to abort log[%lld], rc:%d", lrc.getLsn(), rc);
            ossPanic();
         }
         
         goto error;
      }

      /// 3. update dirty lsn
      updateDirtyLsn(lrc.getLsn());

      guard.unlock();

      rc = lpsLogUtil::commit(context, lrc,
                              _creater.getSpaceID(),
                              getSpaceType(),
                              _dpc->getDataFileType(),
                              builder.getDeltaLogRecord(),
                              slice());
      if (SDB_OK != rc)
      {
         PD_LOG(PDSEVERE, "failed to commit log[%lld], rc:%d", lrc.getLsn(), rc);
         ossPanic();
         goto error;
      }

      ///4. update cache
      for (UINT32 i = 0; i < count; ++i)
      {
         idMapSlot slot(psv, newPids[i]);
         rc = _lpidCache.upsert(lpids[i], slot, FALSE);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to upsert lpid[%d] in cache:%d", lpids[i], rc);
            goto error;
         }
      }

      if (NULL != lsn)
      {
         *lsn = lrc.getLsn();
      }
   done:
      return rc;
   error:
      goto done;
   }

   INT32 logicalPageSpace::replicatedMap(requestContext *context,
                                         PAGE_SNAPSHOT_VERION psv,
                                         UINT32 count,
                                         const PAGE_ID *lpids,
                                         const PAGE_ID *pids,
                                         const slice &initer,
                                         DPS_LSN_OFFSET *lsn)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(NULL != context, "can not be null");
      SDB_ASSERT(INVALID_PAGE_SNAPSHOT_VERSION != psv, "can not be invalid");
      SDB_ASSERT(0 < count, "can not be 0");
      SDB_ASSERT(count < 256, "can not over 1byte");
      SDB_ASSERT(NULL != lpids, "can not be null");
      SDB_ASSERT(NULL != pids, "can not be null");

      ossSpinXLatchGuard guard(&_mappingLatch, FALSE); /// do not get latch here.
      logRecordContext lrc;
      deltaLogRecordBuilder builder;      
      
      rc = builder.buildMappingLog(psv, count, lpids, pids);
      if (OSS_UNLIKELY(SDB_OK != rc))
      {
         PD_LOG(PDERROR, "failed to build mapping log:%d", rc);
         goto error;
      }

      guard.lock();
      /// 1. reserve lsn
      rc = lpsLogUtil::prepare(context, initer,
                               builder.getDeltaLogRecord(),
                               lrc);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to prepare global log record:%d", rc);
         goto error;
      }

      /// 2. write local delta log
      rc = _logConsole.append(builder.getDeltaLogRecord(), NULL);
      if (SDB_OK != rc)
      {
         guard.unlock();
         PD_LOG(PDERROR, "failed to append record to delta log, reserved lsn[%lld], rc:%d",
                lrc.getLsn(), rc);
         INT32 tmpRc = lpsLogUtil::abort(context, lrc);
         if (SDB_OK != tmpRc)
         {
            PD_LOG(PDSEVERE, "failed to abort log[%lld], rc:%d", lrc.getLsn(), rc);
            ossPanic();
         }
         
         goto error;
      }

      /// 3. update dirty lsn
      updateDirtyLsn(lrc.getLsn());

      guard.unlock();

      rc = lpsLogUtil::commit(context, lrc,
                              _creater.getSpaceID(),
                              getSpaceType(),
                              _dpc->getDataFileType(),
                              builder.getDeltaLogRecord(),
                              initer);
      if (SDB_OK != rc)
      {
         PD_LOG(PDSEVERE, "failed to commit log[%lld], rc:%d", lrc.getLsn(), rc);
         ossPanic();
         goto error;
      }

      /// 4. update cache
      for (UINT32 i = 0; i < count; ++i)
      {
         idMapSlot slot(psv, pids[i]);
         rc = _lpidCache.upsert(lpids[i], slot, FALSE);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to upsert lpid[%d] in cache:%d", lpids[i], rc);
            goto error;
         }
      }

      if (NULL != lsn)
      {
         *lsn = lrc.getLsn();
      }
   done:
      return rc;
   error:
      goto done;
   }

   INT32 logicalPageSpace::replicatedUnmmap(requestContext *context,
                                            UINT32 count,
                                            const PAGE_ID *lpids,
                                            const PAGE_ID *pids,
                                            BOOLEAN releaseOld,
                                            DPS_LSN_OFFSET *lsn)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(NULL != context, "can not be null");
      SDB_ASSERT(0 < count, "can not be zero");
      SDB_ASSERT(count < 256, "cao not over 1 byte");
      SDB_ASSERT(NULL != lpids, "can not be null");
      SDB_ASSERT(NULL != pids, "can not be null");

      ossSpinXLatchGuard guard(&_mappingLatch, FALSE); /// do not get latch here.
      logRecordContext lrc;
      deltaLogRecordBuilder builder;      
      UINT32 bufferSize = count << 3;
      CHAR *buffer = context->allocateBuffer(bufferSize);
      if (NULL == buffer)
      {
         PD_LOG(PDERROR, "failed to allocate mem");
         rc = SDB_OOM;
         goto error;
      }

      for (UINT32 i = 0; i < count; ++i)
      {
         SDB_ASSERT(INVALID_PAGE_ID != lpids[i], "can not be invalid");
         SDB_ASSERT(INVALID_PAGE_ID != pids[i], "can not be invalid");
         ((mappedLogicalPageId *)buffer)[i] = mappedLogicalPageId(lpids[i], pids[i]);
      }
      
      rc = builder.buildMappingLog(psv, count,
                                   (const mappedLogicalPageId *)buffer);
      if (OSS_UNLIKELY(SDB_OK != rc))
      {
         PD_LOG(PDERROR, "failed to build mapping log:%d", rc);
         goto error;
      }

      guard.lock();
      /// 1. reserve lsn
      rc = lpsLogUtil::prepare(context, initer,
                               builder.getDeltaLogRecord(),
                               lrc);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to prepare global log record:%d", rc);
         goto error;
      }

      /// 2. write local delta log
      rc = _logConsole.append(builder.getDeltaLogRecord(), NULL);
      if (SDB_OK != rc)
      {
         guard.unlock();
         PD_LOG(PDERROR, "failed to append record to delta log, reserved lsn[%lld], rc:%d",
                lrc.getLsn(), rc);
         INT32 tmpRc = lpsLogUtil::abort(context, lrc);
         if (SDB_OK != tmpRc)
         {
            PD_LOG(PDSEVERE, "failed to abort log[%lld], rc:%d", lrc.getLsn(), rc);
            ossPanic();
         }
         
         goto error;
      }

      /// 3. update dirty lsn
      if (DPS_INVALID_LSN_OFFSET == _minDirtyLsn)
      {
         _minDirtyLsn = lrc.getLsn();
      }
      _maxDirtyLsn = lrc.getLsn();

      guard.unlock();

      rc = lpsLogUtil::commit(context, lrc,
                              _creater.getSpaceID(),
                              getSpaceType(),
                              _dpc->getDataFileType(),
                              builder.getDeltaLogRecord(),
                              initer);
      if (SDB_OK != rc)
      {
         PD_LOG(PDSEVERE, "failed to commit log[%lld], rc:%d", lrc.getLsn(), rc);
         ossPanic();
         goto error;
      }

      if (NULL != lsn)
      {
         *lsn = lrc.getLsn();
      }
   done:
      return rc;
   error:
      goto done;
   }

}//namespace vessel
}//namespace engine