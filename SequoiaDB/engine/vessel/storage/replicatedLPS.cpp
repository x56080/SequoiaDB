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

   Source File Name = replicatedLPS.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#include "vessel/replicatedLPS.h"
#include "ossLatchGuard.hpp"
#include "vessel/logRecordContext.h"
#include "vessel/deltaLogRecordBuilder.h"
#include "vessel/requestContext.h"
#include "vessel/redoLogUtil.h"
#include "vessel/requestContext.h"
#include "vessel/instanceEnv.h"
#include "vessel/logRecordContext.h"
#include "dpsDef.hpp"
#include "vessel/outerResource.h"
#include "vessel/IRedoLogger.h"
#include "dpsLogRecordDef.hpp"

namespace engine
{
namespace vessel
{
   replicatedLPS::replicatedLPS()
   {}

   replicatedLPS::~replicatedLPS()
   {}

   INT32 replicatedLPS::map(requestContext *context,
                            PAGE_SNAPSHOT_VERION psv,
                            UINT32 count,
                            const mappedLogicalPageId *mpids)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(NULL != context, "can not be null");
      SDB_ASSERT(INVALID_PAGE_SNAPSHOT_VERSION != psv, "can not be invalid");
      SDB_ASSERT(0 < count, "can not be 0");
      SDB_ASSERT(count < 256, "can not over 1byte");
      SDB_ASSERT(NULL != mpids, "can not be null");

      BOOLEAN checkpointBlocked = FALSE;
      logRecordContext lrc;
      deltaLogRecordBuilder builder;
      deltaLogRecord dlr;

      /// build delta log
      rc = builder.buildMappingLog(psv, count, mpids);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to build mapping log:%d", rc);
         goto error;
      }
      dlr = builder.getDeltaLogRecord();
      
      /// block checkpoint
      rc = context->blockCheckpoint(getSpaceType(),
                                    logicalPageSpace::getCheckpointContext().getLatch());
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to block checkpoint:%d", rc);
         goto error;
      }
      checkpointBlocked = TRUE;

      /// prepare dps log
      rc = lpsLogUtil::prepare(context, dlr, lrc);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to prepare log:%d", rc);
         goto error;
      }
   

      /// update lsn
      logicalPageSpace::getCheckpointContext().updateDirtyLsn(lrc.getLsn());

      /// commit dps log
      rc = lpsLogUtil::commit(context, lrc,
                              logicalPageSpace::getSpaceID(),
                              getSpaceType(),
                              getStorageFileType(), dlr);
      if (SDB_OK != rc)
      {
         PD_LOG(PDSEVERE, "failed to commit log[%lld], rc:%d", lrc.getLsn(), rc);
         ossPanic();
         goto error;
      }

      /// update cache
      for (UINT32 i = 0; i < count; ++i)
      {
         idMapSlot slot(psv, mpids[i].getPid());
         rc = logicalPageSpace::getCache().upsert(mpids[i].getLpid(), slot);
         if (SDB_OK != rc)
         {
            PD_LOG(PDSEVERE, "failed to insert [%d,%d] into cache, lsn:%lld, rc:%d",
                   mpids[i].getLpid(), mpids[i].getPid(), lrc.getLsn(), rc);
            ossPanic();
            goto error;
         }
      }

   done:
      /// Can not unblock checkpoint by bloker.isBlocking().
      /// It may be already blocked out side.
      if (checkpointBlocked)
      {
         context->unblockCheckpoint();
      }
      return rc;
   error:
      goto done;
   }

   INT32 replicatedLPS::remap(requestContext *context,
                              PAGE_SNAPSHOT_VERION psv,
                              UINT32 count,
                              const mappedLogicalPageId *mpids,
                              const PAGE_ID *oldPids,
                              BOOLEAN releaseOld)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(NULL != context, "can not be null");
      SDB_ASSERT(INVALID_PAGE_SNAPSHOT_VERSION != psv, "can not be invalid");
      SDB_ASSERT(0 < count, "can not be zero");
      SDB_ASSERT(count < 256, "can not over 1 byte");
      SDB_ASSERT(NULL != mpids, "can not be null");

      BOOLEAN checkpointBlocked = FALSE;
      logRecordContext lrc;
      deltaLogRecordBuilder builder;
      deltaLogRecord dlr;

      rc = builder.buildRemappingLog(psv, count, mpids, oldPids, releaseOld);
      if (OSS_UNLIKELY(SDB_OK != rc))
      {
         PD_LOG(PDERROR, "failed to build remapping log:%d", rc);
         goto error;
      }
      dlr = builder.getDeltaLogRecord();

      rc = context->blockCheckpoint(getSpaceType(),
                                    logicalPageSpace::getCheckpointContext().getLatch());
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to block checkpoint:%d", rc);
         goto error;
      }
      checkpointBlocked = TRUE;

      rc = lpsLogUtil::prepare(context, dlr, lrc);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to prepare global log record:%d", rc);
         goto error;
      }

      logicalPageSpace::getCheckpointContext().updateDirtyLsn(lrc.getLsn());

      rc = lpsLogUtil::commit(context, lrc, logicalPageSpace::getSpaceID(),
                              getSpaceType(),
                              getStorageFileType(), dlr);
      if (SDB_OK != rc)
      {
         PD_LOG(PDSEVERE, "failed to commit log[%lld], rc:%d", lrc.getLsn(), rc);
         ossPanic();
         goto error;
      }

      ///4. update cache
      for (UINT32 i = 0; i < count; ++i)
      {
         idMapSlot slot(psv, mpids[i].getPid());
         rc = logicalPageSpace::getCache().upsert(mpids[i].getLpid(), slot);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to upsert lpid[%d] in cache:%d", mpids[i].getLpid(), rc);
            ossPanic();
            goto error;
         }
      }

      context->unblockCheckpoint();
      checkpointBlocked = FALSE;

      if (releaseOld)
      {
         getDataStorageObj()->releasePages(count, oldPids);
      }

   done:
      if (checkpointBlocked)
      {
         context->unblockCheckpoint();
      }
      return rc;
   error:
      goto done;
   }

   INT32 replicatedLPS::unmap(requestContext *context,
                              UINT32 count,
                              const mappedLogicalPageId *mpids,
                              BOOLEAN releasePid)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(NULL != context, "can not be null");
      SDB_ASSERT(0 < count, "can not be zero");
      SDB_ASSERT(count < 256, "cao not over 1 byte");
      SDB_ASSERT(NULL != mpids, "can not be null");

      BOOLEAN checkpointBlocked = FALSE;
      logRecordContext lrc;
      deltaLogRecordBuilder builder;
      deltaLogRecord dlr;
      CHAR *buffer = NULL;
      UINT32 bufferSize = count * sizeof(PAGE_ID);

      if (releasePid)
      {
         buffer = context->allocateBuffer(bufferSize);
         if (NULL == buffer)
         {
            PD_LOG(PDERROR, "failed to allocate mem");
            rc = SDB_OOM;
            goto error;
         }

         for (UINT32 i = 0; i < count; ++i)
         {
            if (!mpids[i].isValid())
            {
               PD_LOG(PDERROR, "invalid mpid found");
               SDB_ASSERT(FALSE, "impossible");
               rc = SDB_INVALIDARG;
               goto error;
            }

            ((PAGE_ID *)buffer)[i] = mpids[i].getPid();
         }
      }

      rc = builder.buildUnmappingLog(count, mpids, releasePid);
      if (OSS_UNLIKELY(SDB_OK != rc))
      {
         PD_LOG(PDERROR, "failed to build unmapping log:%d", rc);
         goto error;
      }
      dlr = builder.getDeltaLogRecord();

      rc = context->blockCheckpoint(getSpaceType(),
                                    logicalPageSpace::getCheckpointContext().getLatch());
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to block checkpoint:%d", rc);
         goto error;
      }
      checkpointBlocked = TRUE;

      rc = lpsLogUtil::prepare(context, dlr, lrc);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to prepare global log record:%d", rc);
         goto error;
      }

      /// 3. update dirty lsn
      logicalPageSpace::getCheckpointContext().updateDirtyLsn(lrc.getLsn());

      rc = lpsLogUtil::commit(context, lrc, logicalPageSpace::getSpaceID(),
                              getSpaceType(),
                              getStorageFileType(), dlr);
      if (SDB_OK != rc)
      {
         PD_LOG(PDSEVERE, "failed to commit log[%lld], rc:%d", lrc.getLsn(), rc);
         ossPanic();
         goto error;
      }

      ///4. update cache
      for (UINT32 i = 0; i < count; ++i)
      {
         rc = logicalPageSpace::getCache().remove(mpids[i].getLpid());
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to remove lpid[%d] in cache:%d", mpids[i].getLpid(), rc);
            ossPanic();
            goto error;
         }
      }

      context->unblockCheckpoint();
      checkpointBlocked = FALSE;

      if (releasePid)
      {
         getDataStorageObj()->releasePages(count,
                                          (const PAGE_ID *)buffer);
      }
   done:
      if (checkpointBlocked)
      {
         context->unblockCheckpoint();
      }
      if (NULL != buffer)
      {
         context->releaseBuffer(buffer, bufferSize);
      }
      return rc;
   error:
      goto done;
   }

   INT32 replicatedLPS::releasePids(requestContext *context,
                                    UINT32 count,
                                    const PAGE_ID *pids)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(NULL != context, "can not be null");
      SDB_ASSERT(0 < count, "can not be zero");
      SDB_ASSERT(count < 256, "cao not over 1 byte");
      SDB_ASSERT(NULL != pids, "can not be null");

      BOOLEAN checkpointBlocked = FALSE;
      logRecordContext lrc;
      deltaLogRecordBuilder builder;
      deltaLogRecord dlr;

      rc = builder.buildReleasingLog(count, pids);
      if (OSS_UNLIKELY(SDB_OK != rc))
      {
         PD_LOG(PDERROR, "failed to build unmapping log:%d", rc);
         goto error;
      }
      dlr = builder.getDeltaLogRecord();

      rc = context->blockCheckpoint(getSpaceType(),
                                    logicalPageSpace::getCheckpointContext().getLatch());
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to block checkpoint:%d", rc);
         goto error;
      }
      checkpointBlocked = TRUE;

      rc = lpsLogUtil::prepare(context, dlr, lrc);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to prepare global log record:%d", rc);
         goto error;
      }

      logicalPageSpace::getCheckpointContext().updateDirtyLsn(lrc.getLsn());
      

      rc = lpsLogUtil::commit(context, lrc, logicalPageSpace::getSpaceID(),
                              getSpaceType(),
                              getStorageFileType(), dlr);
      if (SDB_OK != rc)
      {
         PD_LOG(PDSEVERE, "failed to commit log[%lld], rc:%d", lrc.getLsn(), rc);
         ossPanic();
         goto error;
      }

      context->unblockCheckpoint();
      checkpointBlocked = FALSE;

      getDataStorageObj()->releasePages(count, pids);
   done:
      if (checkpointBlocked)
      {
         context->unblockCheckpoint();
      }
      return rc;
   error:
      goto done;
   }

   INT32 replicatedLPS::getRuntimePageBuffer(requestContext *context,
                                             PAGE_ID pid,
                                             const ossSharedLatchMode &mode,
                                             runtimePageBuffer &rpb)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(!rpb.isValid(), "can not be valid");
      logicalPageSpace::_runtimePageBufferIniter initer;
      liteCacheAllocateOptions options;
      options.lockMode = mode;
      liteCacheTuple tuple;
      GLOBAL_PAGE_ID gpid;
      UINT32 pageSize = 0;

      if (OSS_UNLIKELY(NULL == context ||
                      INVALID_PAGE_ID == pid ||
                      mode.isNone()))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      gpid.reset(logicalPageSpace::getSpaceID(),
                 getSpaceType(),
                 getStorageFileType(),
                 pid);

      rc = context->getEnv()->cacheConsole.allocate(context, gpid, options, tuple);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to allocate cache tuple of page[%s], rc:%d",
                gpid.toString().c_str(), rc);
         goto error;
      }

      pageSize = logicalPageSpace::getStorageCoreArgs().pageSize;
      rc = initer.initWithCache(gpid, pageSize, tuple, rpb);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to init rpb[%s] with cache tuple:%d",
                gpid.toString().c_str(), rc);
         goto error;
      }
   done:
      tuple.release();
      return rc;
   error:
      goto done;
   }

   INT32 replicatedLPS::getRuntimePageBufferToReset(requestContext *context,
                                                    PAGE_ID pid,
                                                    runtimePageBuffer &rpb)
   {
      INT32 rc = SDB_OK;
      logicalPageSpace::_runtimePageBufferIniter initer;
      liteCacheTuple tuple;
      GLOBAL_PAGE_ID gpid(logicalPageSpace::getSpaceID(),
                          getSpaceType(),
                          getStorageFileType(),
                          pid);
      UINT32 pageSize = 0;
      
      if (OSS_UNLIKELY(NULL == context ||
                       INVALID_PAGE_ID == pid))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      rc = context->getEnv()->cacheConsole.allocateToReset(context, gpid, tuple);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to allocate cache tuple of page[%s], rc:%d",
                gpid.toString().c_str(), rc);
         goto error;
      }

      pageSize = logicalPageSpace::getStorageCoreArgs().pageSize;
      rc = initer.initWithCache(gpid, pageSize, tuple, rpb);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to init rpb[%s] with cache tuple:%d",
                gpid.toString().c_str(), rc);
         goto error;
      }

      rc = rpb.prepareToWrite(context);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to prepare to write:%d", rc);
         goto error;
      }
   done:
      tuple.release();
      return rc;
   error:
      rpb.fini();
      goto done;
   }

   INT32 replicatedLPS::copyPageAndReinitBuffer(requestContext *context,
                                                PAGE_SNAPSHOT_VERION psv,
                                                PAGE_ID newPid,
                                                runtimePageBuffer &rpb)
   {
      INT32 rc = SDB_OK;
      logicalPageSpace::_runtimePageBufferIniter initer;
      liteCacheTuple tuple;
      GLOBAL_PAGE_ID gpid;
      UINT32 pageSize = logicalPageSpace::getStorageCoreArgs().pageSize;
      CHAR *buffer = NULL;
      logRecordContext lrc;
      slice rs;

      if (OSS_UNLIKELY(NULL == context ||
                       INVALID_PAGE_SNAPSHOT_VERSION == psv ||
                       INVALID_PAGE_ID == newPid ||
                       !rpb.isValid() ||
                       !rpb.isCacheBuffer()))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      rs = rpb.getReadbleSlice();
      if (isPageCrashed((ossValuePtr)rs.getRPtr(), pageSize))
      {
         PD_LOG(PDERROR, "page[%s] may be crashed", rpb.getGlobalPid().toString().c_str());
         rc = SDB_VESSEL_PAGE_CRASHED;
         goto error;
      }

      buffer = context->allocateBuffer(pageSize);
      ossMemcpy(buffer, rs.getRPtr(), pageSize);
      rpb.fini();

      gpid.reset(logicalPageSpace::getSpaceID(),
                 getSpaceType(),
                 getStorageFileType(),
                 newPid);
      rc = context->getEnv()->cacheConsole.allocateToReset(context, gpid, tuple);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to allocate cache tuple of page[%s], rc:%d",
                gpid.toString().c_str(), rc);
         goto error;
      }

      rc = prepareCopyLog(context, pageSize, &lrc);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to prepare log:%d", rc);
         goto error;
      }

      ossMemcpy((void *)(tuple.getWritableBuffer()),
                 rs.getRPtr(),
                 pageSize);
      ((pageHead *)(tuple.getWritableBuffer()))->pid = newPid;
      ((pageHead *)(tuple.getWritableBuffer()))->psv = psv;
      updatePageLsn((ossValuePtr)(tuple.getWritableBuffer()), lrc.getLsn());

      rc = commit(context, pageSize,
                  (const void *)(tuple.getReadableBuffer()),
                  gpid,
                  ((const pageHead *)(tuple.getReadableBuffer()))->lpid,
                  &lrc);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to commit log[%lld]:%d", lrc.getLsn(), rc);
         ossPanic();
         goto error;
      }
      
      tuple.commit(lrc.getLsn());
      rpb.fini();

      rc = initer.initWithCache(gpid, pageSize, tuple, rpb);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to reinit rpb:%d", rc);
         ossPanic();
         goto error;
      }

      rc = rpb.prepareToWrite(context);
      if (SDB_OK != rc)
      {
         rpb.fini();
         PD_LOG(PDERROR, "failed to prepare to write:%d", rc);
         SDB_ASSERT(FALSE, "impossible");
         goto error;
      }
   done:
      tuple.release();
      if (NULL != buffer)
      {
         context->releaseBuffer(buffer, pageSize);
      }
      return rc;
   error:
      goto done;
   }

   INT32 replicatedLPS::prepareCopyLog(requestContext *context,
                                       UINT32 pageSize,
                                       logRecordContext *lrc)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(NULL != context, "can not be null");
      SDB_ASSERT(isValidPageSize(pageSize), "can not be invalid");
      SDB_ASSERT(NULL != lrc, "can not be null");
      IRedoLogger *logger = context->getOuterResource()->logger;
      lrc->open(LOG_TYPE_VESSEL_COPY_PAGE);
      lrc->setResetPage();
      lrc->prepush(sizeof(GLOBAL_PAGE_ID));
      lrc->prepush(pageSize);
      lrc->prepush(sizeof(PAGE_ID));
      lrc->prepushDone();
      rc = logger->prepare(context->getExecutor(), lrc);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to prepare log:%d", rc);
         goto error;
      }
   done:
      return rc;
   error:
      goto done;
   }

   INT32 replicatedLPS::commit(requestContext *context,
                               UINT32 pageSize,
                               const void *pageBuffer,
                               const GLOBAL_PAGE_ID &gpid,
                               PAGE_ID lpid,
                               logRecordContext *lrc)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(NULL != context, "can not be null");
      SDB_ASSERT(isValidPageSize(pageSize), "can not be invalid");
      SDB_ASSERT(NULL != pageBuffer, "can not be null");
      SDB_ASSERT(gpid.isValid(), "can not be invalid");
      SDB_ASSERT(INVALID_PAGE_ID != lpid, "can not be invalid");
      SDB_ASSERT(NULL != lrc, "can not be null");
      SDB_ASSERT(lrc->prepared(), "must be prepared");

      IRedoLogger *logger = context->getOuterResource()->logger;

      rc = logger->pushLogRecordElement(context->getExecutor(), lrc,
                                        DPS_LOG_PUBLIC_VESSEL_GPID,
                                        sizeof(gpid),
                                        &gpid);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to push gpid:%d", rc);
         goto error;
      }

      rc = logger->pushLogRecordElement(context->getExecutor(), lrc,
                                        DPS_LOG_VESSEL_COPY_PAGE_LPID,
                                        sizeof(PAGE_ID), &lpid);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to push page lpid:%d", rc);
         goto error;
      }

      rc = logger->pushLogRecordElement(context->getExecutor(), lrc,
                                        DPS_LOG_PUBLIC_VESSEL_FULL_PAGE_DUMP,
                                        pageSize, pageBuffer);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to push page buffer:%d", rc);
         goto error;
      }

      rc = logger->commit(context->getExecutor(), lrc);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to commit:%d", rc);
         goto error;
      }
   done:
      return rc;
   error:
      goto done;
   }

   void replicatedLPS::abort(requestContext *context,
                             logRecordContext *lrc)
   {
      SDB_ASSERT(NULL != context, "can not be null");
      SDB_ASSERT(NULL != lrc, "can not be null");
      SDB_ASSERT(lrc->prepared(), "must be prepared");
      IRedoLogger *logger = context->getOuterResource()->logger;
      logger->abort(context->getExecutor(), lrc);
      return;
   }

   INT32 replicatedLPS::prepareToCreateCheckpoint(requestContext *context,
                                                  BOOLEAN fullCheckpoint,
                                                  checkpointLSN &lsn)
   {
      SDB_ASSERT(NULL != context, "can not be null");
      IRedoLogger *logger = context->getOuterResource()->logger;
      lsn._lsn = logger->getCurrentLSN();
      lsn._minDirtyLSN = lsn._lsn + 1;
      lsn._minUncompletedLSN = lsn._minDirtyLSN;
      return SDB_OK;
   }
}//namespace vessel
}//namespace engine