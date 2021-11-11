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

   Source File Name = copyOnWriteLPS.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#include "vessel/copyOnWriteLPS.h"
#include "vessel/dataPageCluster.h"
#include "vessel/deltaLogRecordBuilder.h"
#include "ossLatchGuard.hpp"
#include "vessel/requestContext.h"

namespace engine
{
namespace vessel
{
   copyOnWriteLPS::copyOnWriteLPS()
   {}

   copyOnWriteLPS::~copyOnWriteLPS()
   {
      fini();
   }

   void copyOnWriteLPS::fini()
   {
      if (NULL != _waitingForReleasing)
      {
         _waitingForReleasing->clear();
         SDB_OSS_DEL _waitingForReleasing;
         _waitingForReleasing = NULL;
      }
      if (NULL != _readyForReleasing)
      {
         _readyForReleasing->clear();
         SDB_OSS_DEL _readyForReleasing;
         _readyForReleasing = NULL;
      }
      return;
   }

   void copyOnWriteLPS::_close()
   {
      fini();
   }

   void copyOnWriteLPS::_destroy()
   {
      fini();
   }

   INT32 copyOnWriteLPS::getRuntimePageBuffer(requestContext *context,
                                              PAGE_ID pid,
                                              const ossSharedLatchMode &mode,
                                              runtimePageBuffer &rpb)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(!rpb.isValid(), "can not be valid");
      dataPageCluster *dpc = NULL;
      logicalPageSpace::_runtimePageBufferIniter initer;
      GLOBAL_PAGE_ID gpid;
      UINT32 pageSize = 0;
      mmapPagePointer ptr;

      if (OSS_UNLIKELY(NULL == context ||
                      INVALID_PAGE_ID == pid ||
                      mode.isNone()))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      dpc = getDataStorageObj();
      SDB_ASSERT(NULL != dpc, "can not be null");

      rc = dpc->getDataPagePtr(pid, ptr);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to get page[%d], rc:%d", pid, rc);
         goto error;
      }

      gpid.reset(logicalPageSpace::getSpaceID(),
                 getSpaceType(),
                 getStorageFileType(),
                 pid);
      pageSize = logicalPageSpace::getStorageCoreArgs().pageSize;

      rc = initer.initWithMmap(gpid, pageSize, ptr, rpb);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to init rpb:%d", rc);
         goto error;
      }
   done:
      return rc;
   error:
      goto done;
   }

   INT32 copyOnWriteLPS::getRuntimePageBufferToReset(requestContext *context,
                                                     PAGE_ID pid,
                                                     runtimePageBuffer &rpb)
   {
      INT32 rc = SDB_OK;
      ossSharedLatchMode mode(OSS_SHARED_LATCH_MODE_ENUM_EXCLUSIVE);

      rc = this->getRuntimePageBuffer(context, pid,
                                      mode, /// usless actually
                                      rpb);
      if (SDB_OK != rc)
      {
         goto error;
      }

      rc = rpb.prepareToWrite(context);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "faield to get rpb ready to write:%d", rc);
         goto error;
      }
   done:
      return rc;
   error:
      rpb.fini();
      goto done;
   }

   INT32 copyOnWriteLPS::copyPageAndReinitBuffer(requestContext *context,
                                                 PAGE_SNAPSHOT_VERION psv,
                                                 PAGE_ID newPid,
                                                 runtimePageBuffer &rpb)
   {
      INT32 rc = SDB_OK;
      UINT32 pageSize = logicalPageSpace::getStorageCoreArgs().pageSize;
      mmapPagePointer ptr;
      GLOBAL_PAGE_ID gpid;
      logicalPageSpace::_runtimePageBufferIniter initer;
      slice rs;

      if (OSS_UNLIKELY(NULL == context ||
                       INVALID_PAGE_SNAPSHOT_VERSION == psv ||
                       INVALID_PAGE_ID == newPid ||
                       !rpb.isValid() ||
                       rpb.isCacheBuffer()))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      
      rs = rpb.getReadbleSlice();
      if (isPageCrashed((ossValuePtr)(rs.getRPtr()), pageSize))
      {
         PD_LOG(PDERROR, "page[%s] may be crashed", rpb.getGlobalPid().toString().c_str());
         rc = SDB_VESSEL_PAGE_CRASHED;
         goto error;
      }

      rc = getDataStorageObj()->getDataPagePtr(newPid, ptr);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "faield to get new page[%d] ptr:%d", newPid, rc);
         goto error;
      }

      ossMemcpy((void *)(ptr.get()), rs.getRPtr(), pageSize);
      ((pageHead *)(ptr.get()))->pid = newPid;
      ((pageHead *)(ptr.get()))->psv = psv;

      rpb.fini();

      gpid.reset(logicalPageSpace::getSpaceID(),
                 getSpaceType(),
                 getStorageFileType(),
                 newPid);

      rc = initer.initWithMmap(gpid, pageSize, ptr, rpb);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to init rpb:%d", rc);
         goto error;
      }
   done:
      return rc;
   error:
      goto done;
   }

   INT32 copyOnWriteLPS::map(requestContext *context,
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
      deltaLogRecordBuilder builder;
      deltaLogRecord dlr;
      ossXLatchGuard guard(logicalPageSpace::getMappingLatch(), FALSE);
      DPS_LSN_OFFSET lsn = context->getExecutor()->getEndLsn();

      if (DPS_INVALID_LSN_OFFSET == lsn)
      {
         PD_LOG(PDERROR, "last lsn of executor is invalid");
         rc = SDB_VESSEL_OPERATOION_NOT_PERMITTED;
         goto error;
      }

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

      guard.lock();

      /// commit delta log
      rc = logicalPageSpace::getLogConsole().append(dlr);
      if (SDB_OK != rc)
      {
         guard.unlock();
         PD_LOG(PDERROR, "failed to append record to delta log, lsn[%lld], rc:%d",
                lsn, rc);
         goto error;
      }

      /// update lsn
      logicalPageSpace::getCheckpointContext().updateDirtyLsn(lsn);
      guard.unlock();

      /// update cache
      for (UINT32 i = 0; i < count; ++i)
      {
         idMapSlot slot(psv, mpids[i].getPid());
         rc = logicalPageSpace::getCache().upsert(mpids[i].getLpid(), slot);
         if (SDB_OK != rc)
         {
            PD_LOG(PDSEVERE, "failed to insert [%d,%d] into cache, lsn:%lld, rc:%d",
                   mpids[i].getLpid(), mpids[i].getPid(), lsn, rc);
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

   INT32 copyOnWriteLPS::remap(requestContext *context,
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
      ossXLatchGuard guard(logicalPageSpace::getMappingLatch(), FALSE); /// do not get latch here.
      deltaLogRecordBuilder builder;
      deltaLogRecord dlr;

      DPS_LSN_OFFSET lsn = context->getExecutor()->getEndLsn();

      if (DPS_INVALID_LSN_OFFSET == lsn)
      {
         PD_LOG(PDERROR, "last lsn of executor is invalid");
         rc = SDB_VESSEL_OPERATOION_NOT_PERMITTED;
         goto error;
      }

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

      guard.lock();
      /// write local delta log
      rc = logicalPageSpace::getLogConsole().append(dlr);
      if (SDB_OK != rc)
      {
         guard.unlock();
         PD_LOG(PDERROR, "failed to append record to delta log, lsn[%lld], rc:%d",
                lsn, rc);         
         goto error;
      }

      /// update dirty lsn
      logicalPageSpace::getCheckpointContext().updateDirtyLsn(lsn);

      if (releaseOld)
      {
         pushIntoRemovingList(count, oldPids);
      }
      guard.unlock();

      ///update cache
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

   done:
      if (checkpointBlocked)
      {
         context->unblockCheckpoint();
      }
      return rc;
   error:
      goto done;
   }

   INT32 copyOnWriteLPS::unmap(requestContext *context,
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
      ossXLatchGuard guard(logicalPageSpace::getMappingLatch(), FALSE); /// do not get latch here.
      deltaLogRecordBuilder builder;
      deltaLogRecord dlr;
      CHAR *buffer = NULL;
      UINT32 bufferSize = count * sizeof(PAGE_ID);

      DPS_LSN_OFFSET lsn = context->getExecutor()->getEndLsn();

      if (DPS_INVALID_LSN_OFFSET == lsn)
      {
         PD_LOG(PDERROR, "last lsn of session is invalid");
         rc = SDB_VESSEL_OPERATOION_NOT_PERMITTED;
         goto error;
      }

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

      guard.lock();
      /// write local delta log
      rc = logicalPageSpace::getLogConsole().append(dlr);
      if (SDB_OK != rc)
      {
         guard.unlock();
         PD_LOG(PDERROR, "failed to append record to delta log, lsn[%lld], rc:%d",
                lsn, rc);
         goto error;
      }

      /// update dirty lsn
      logicalPageSpace::getCheckpointContext().updateDirtyLsn(lsn);

      if (releasePid)
      {
         pushIntoRemovingList(count,
                              (const PAGE_ID *)buffer);
      }
      guard.unlock();

      /// update cache
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

   INT32 copyOnWriteLPS::releasePids(requestContext *context,
                                     UINT32 count,
                                     const PAGE_ID *pids)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(NULL != context, "can not be null");
      SDB_ASSERT(0 < count, "can not be zero");
      SDB_ASSERT(count < 256, "cao not over 1 byte");
      SDB_ASSERT(NULL != pids, "can not be null");

      BOOLEAN checkpointBlocked = FALSE;
      ossXLatchGuard guard(logicalPageSpace::getMappingLatch(), FALSE); /// do not get latch here.
      deltaLogRecordBuilder builder;
      deltaLogRecord dlr;

      DPS_LSN_OFFSET lsn = context->getExecutor()->getEndLsn();
      if (DPS_INVALID_LSN_OFFSET == lsn)
      {
         PD_LOG(PDERROR, "last lsn of session is invalid");
         rc = SDB_VESSEL_OPERATOION_NOT_PERMITTED;
         goto error;
      }

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

      guard.lock();
      /// write local delta log
      rc = logicalPageSpace::getLogConsole().append(dlr);
      if (SDB_OK != rc)
      {
         guard.unlock();
         PD_LOG(PDERROR, "failed to append record to delta log, lsn[%lld], rc:%d",
                lsn, rc);
         
         goto error;
      }

      ///update dirty lsn
      logicalPageSpace::getCheckpointContext().updateDirtyLsn(lsn);
      guard.unlock();

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

   INT32 copyOnWriteLPS::prepareToCreateCheckpoint(requestContext *context,
                                                   BOOLEAN fullCheckpoint,
                                                   ossPoolSet<UINT32> &dirtySegments)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(NULL != context, "can not be null");
      
      if (fullCheckpoint)
      {
         rc = getCache().prepareToCreateNewBase(getStorageCoreArgs().maxPageCountPerSeg,
                                                &dirtySegments);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to get ready to create new base:%d", rc);
            goto error;
         }
      }
      else
      {
         rc = getCache().setPagesImmutable(getStorageCoreArgs().maxPageCountPerSeg,
                                           dirtySegments);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to set pages immutable:%d", rc);
            goto error;
         }
      }

      switchRemovingList();

   done:
      return rc;
   error:
      goto done;
   }

   void copyOnWriteLPS::endToCreateCheckpoint(requestContext *context)
   {
      static const UINT32 _SIZE = 16;
      ossScopedRWLock guard(logicalPageSpace::getCheckpointContext().getLatch(), SHARED);

      /// no checkpoint can be created now, coz we are holding shared latch
      if (NULL != _readyForReleasing)
      {
         ossPoolVector<PAGE_ID> pids;
         pids.reserve(_SIZE);

         while (0 != _readyForReleasing->getSize())
         {
            for (UINT32 i = 0; i < _SIZE; ++i)
            {
               PAGE_ID pid = INVALID_PAGE_ID;
               if (_readyForReleasing->popForward(pid))
               {
                  if (OSS_LIKELY(INVALID_PAGE_ID != pid))
                  {
                     pids.push_back(pid);
                  }
               }
               else
               {
                  break;
               }
            }

            if (pids.empty())
            {
               continue;
            }
            else if (1 < pids.size())
            {
               //std::sort(pids.begin(), pids.end());
            }
            getDataStorageObj()->releasePages(pids.size(), pids.data());
            pids.clear();
         }

         SDB_OSS_DEL _readyForReleasing;
         _readyForReleasing = NULL;
      }
      return;
   }

   void copyOnWriteLPS::pushIntoRemovingList(UINT32 count,
                                             const PAGE_ID *pids)
   {
      SDB_ASSERT(0 < count, "can not be zero");
      SDB_ASSERT(NULL != pids, "can not be null");
      if (NULL == _waitingForReleasing)
      {
         _waitingForReleasing = SDB_OSS_NEW forwardList<PAGE_ID>();
         if (NULL == _waitingForReleasing)
         {
            PD_LOG(PDSEVERE, "failed to allocate mem for removing list");
            goto done;
         }
      }

      for (UINT32 i = 0; i < count; ++i)
      {
         SDB_ASSERT(INVALID_PAGE_ID != pids[i], "can not be invalid");
         INT32 rc = _waitingForReleasing->pushForward(pids[i]);
         if (SDB_OK != rc)
         {
            PD_LOG(PDSEVERE, "failed to push pid[%d] into list:%d", rc);
         }
      }
   done:
      return;
   }

   void copyOnWriteLPS::switchRemovingList()
   {
      INT32 rc = SDB_OK;
      PAGE_ID pid = INVALID_PAGE_ID;
      
      if (NULL == _waitingForReleasing || _waitingForReleasing->getSize() == 0)
      {
         goto done;
      }
      else if (NULL == _readyForReleasing)
      {
         _readyForReleasing = _waitingForReleasing;
         _waitingForReleasing = NULL;
         goto done;
      }

      /// for some reason last checkpoint not done.
      while (_waitingForReleasing->popForward(pid))
      {
         rc = _readyForReleasing->pushForward(pid);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to push pid[%d] into backup list:%d",
                   pid, rc);
         }
      }
   done:
      return;
   }
}//namespace vessel
}//namespace engine
