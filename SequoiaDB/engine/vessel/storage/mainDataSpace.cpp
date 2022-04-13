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

   Source File Name = mainDataSpace.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#include "vessel/mainDataSpace.h"
#include "ossLikely.hpp"
#include "vessel/clMetaBlockPage.h"
#include "vessel/csMetaBlockPageAccessor.h"
#include "vessel/logicalPageBuffer.h"
#include "vessel/requestContext.h"
#include "vessel/instanceEnv.h"
#include "vessel/outerResource.h"
#include "vessel/logRecordContext.h"
#include "dpsDef.hpp"
#include "vessel/IRedoLogger.h"
#include "vessel/deltaLogRecordBuilder.h"
#include "dpsLogRecordDef.hpp"
#include "utilStr.hpp"
#include "vessel/fsmFile.h"
#include "vessel/freeSpaceMapDef.h"
#include "vessel/storageUtils.h"
#include "vessel/logRecordContext.h"
#include "vessel/idMapFile.h"
#include "vessel/storageFileMaintainer.h"

namespace engine
{
namespace vessel
{
   mainDataSpace::mainDataSpace()
   {}

   mainDataSpace::~mainDataSpace()
   {
      SAFE_OSS_DELETE(_fsm);
   }
   
   INT32 mainDataSpace::initMetaPageWhenCreateCS(requestContext *context,
                                                 const csMetaBlock &block,
                                                 const slice &options)
   {
      INT32 rc = SDB_OK;
      mmapPagePointer ptr;
      PAGE_ID lpid = CS_META_BLOCK_PAGE_LPID;
      PAGE_ID pid = 0;
      PAGE_SNAPSHOT_VERION psv = INVALID_PAGE_SNAPSHOT_VERSION;
      IExecutor *executor = nullptr;
      IRedoLogger *logger = nullptr;
      csMetaBlock *blockOnDisk = nullptr;
      logRecordContext lrc;
      SPACE_ID sid = INVALID_SPACE_ID;
      deltaLogRecordBuilder builder;
      mappedLogicalPageId mid(lpid, pid);
      lpageDescriptor desc;


      if (OSS_UNLIKELY(nullptr == context ||
                       !block.isValid()))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      else if (!logicalPageSpace::isOpen())
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }

      sid = logicalPageSpace::getSpaceID();
      executor = context->getExecutor();
      logger = context->getOuterResource()->logger;

      psv = context->getEnv()->dms.getOnlinePageSnapshotVersion();

      builder.buildMappingLog(psv, 1, &mid);

      SDB_ASSERT(0 == _storage.getTotalSegmentCount(), "must be empty");
      rc = _storage.ensureSegmentCount(1);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to extend storage:%d", rc);
         goto error;
      }

      rc = _storage.occupyPage(pid);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to occupy meta page pid:%d", rc);
         goto error;
      }

      rc = _storage.getDataPagePtr(pid, ptr);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to get page[%d] ptr:%d", pid, rc);
         goto error;
      }

      /// init page
      if (!initCSMetaBlockPage(_storage.getCoreArgs().pageSize,
                               pid, lpid, psv, (void *)(ptr.get())))
      {
         PD_LOG(PDERROR, "faield to init gmp");
         rc = SDB_VESSEL_INTERNAL_ERR;
         goto error;
      }

      blockOnDisk = (csMetaBlock *)(ptr.get() + PAGE_HEAD_SIZE);
      *blockOnDisk = block;

      /// prepare dps log
      lrc.open(LOG_TYPE_CS_CRT);
      lrc.setDDL();
      lrc.setResetPage();
      lrc.prepush(sizeof(SPACE_ID));
      lrc.prepush(CS_META_BLOCK_LEN);
      lrc.prepush(options.getSize());
      lrc.prepushDone();
      rc = logger->prepare(executor, &lrc);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to prepare log:%d", rc);
         goto error;
      }

      /// update page lsn
      if (!updatePageLsn(ptr.get(), lrc.getLsn()))
      {
         logger->abort(executor, &lrc);
         PD_LOG(PDERROR, "failed to update page lsn:%d", rc);
         goto error;
      }

      rc = _storage.fysncPage(pid);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to fsync page:%d", rc);
         goto error;
      }

      /// commit dps log
      rc = logger->pushLogRecordElement(executor, &lrc, DPS_LOG_CSCRT_VESSEL_SID,
                                        sizeof(SPACE_ID), &sid);
      if (SDB_OK != rc)
      {
         logger->abort(executor, &lrc);
         PD_LOG(PDERROR, "failed to push ele[%d], rc:%d",
                DPS_LOG_CSCRT_VESSEL_SID, rc);
         goto error;
      }

      rc = logger->pushLogRecordElement(executor, &lrc, DPS_LOG_CSCRT_VESSEL_META,
                                        CS_META_BLOCK_LEN, &block);
      if (SDB_OK != rc)
      {
         logger->abort(executor, &lrc);
         PD_LOG(PDERROR, "failed to push ele[%d], rc:%d",
                DPS_LOG_CSCRT_VESSEL_META, rc);
         goto error;
      }

      rc = logger->pushLogRecordElement(executor, &lrc, DPS_LOG_CSCRT_VESSEL_OPTIONS,
                                        options.getSize(), options.getData());
      if (SDB_OK != rc)
      {
         logger->abort(executor, &lrc);
         PD_LOG(PDERROR, "failed to push ele[%d], rc:%d",
                DPS_LOG_CSCRT_VESSEL_OPTIONS, rc);
         goto error;
      }

      rc = logicalPageSpace::getLogConsole().append(builder.getDeltaLogRecord());
      if (SDB_OK != rc)
      {
         logger->abort(executor, &lrc);
         PD_LOG(PDERROR, "failed to append delta log:%d", rc);
         goto error;
      }

      desc.pid = pid;
      desc.birthTick = logicalPageSpace::getCheckpointContext().getCheckpointTick();
      desc.psv = psv;
      rc = logicalPageSpace::getMapping().set(lpid, desc);
      if (SDB_OK != rc)
      {
         logger->abort(executor, &lrc);
         PD_LOG(PDERROR, "failed to put mapping into cache:%d", rc);
         ossPanic();
         goto error;
      }

      rc = logger->commit(executor, &lrc);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to commit dps log[%lld], rc:%d",
                lrc.getLsn(), rc);
         goto error;
      }

      /// update space's dirty lsn
      logicalPageSpace::getCheckpointContext().updateDirtyLsn(lrc.getLsn());
   done:
      return rc;
   error:
      goto done;
   }

   INT32 mainDataSpace::readMetaBlockWhenOpen(requestContext *context,
                                              csMetaBlock &block)
   {
      INT32 rc = SDB_OK;
      UINT32 pageSize = 0;
      logicalPageBuffer lpb;
      csMetaBlockPageAccessor accessor;

      if (OSS_UNLIKELY(!isOpen()))
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }

      pageSize = logicalPageSpace::getStorageCoreArgs().pageSize;
      SDB_ASSERT(isValidPageSize(pageSize), "must be valid");

      rc = getLogicalPageBuffer(context, CS_META_BLOCK_PAGE_LPID,
                                ossSharedLatchMode(OSS_SHARED_LATCH_MODE_ENUM_SHARED), lpb);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to get page mapping of meta page:%d", rc);
         goto error;
      }

      rc = accessor.read(context, &lpb, block);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to read meta data:%d", rc);
         goto error;
      }

   done:
      return rc;
   error:
      goto done;
   }

   INT32 mainDataSpace::updateCSMetaBlock(requestContext *context,
                                          const csMetaBlock &block,
                                          UINT64 updateMask)
   {
      INT32 rc = SDB_OK;
      csMetaBlockPageAccessor accessor;
      logicalPageBuffer lpb;
      ossSharedLatchMode mode(OSS_SHARED_LATCH_MODE_ENUM_EXCLUSIVE);

      if (OSS_UNLIKELY(nullptr == context ||
                       !block.isValid() ||
                       0 == updateMask))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      else if (!isOpen())
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }

      rc = getLogicalPageBuffer(context, CS_META_BLOCK_PAGE_LPID, mode, lpb);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to get logical page buffer[%d]", CS_META_BLOCK_PAGE_LPID);
         goto error;
      }

      rc = accessor.update(context, &lpb, block, updateMask);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to update cs meta block, rc:%d", rc);
         goto error;
      }

   done:
      return rc;
   error:
      goto done;
   }

   INT32 mainDataSpace::_open(const storageFileLoader &loader)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(nullptr == _fsm, "must be null");
      fsmFile *file = nullptr;
      const storageFileName *fn = nullptr;
      const storagePathOptions &po = GET_THREAD_CONTEXT()->getEnv()->options.path;
      storageFileMaintainer sfm(&po, getSpaceID());
      UINT32 fileCtlFlags = storageFileCtlFlag::MMAP_DATA_SEGMENT;

      const STORAGE_FILE_NAME_LIST *fl = loader.getFileList(SPACE_TYPE_MAIN_DATA,
                                                            FILE_TYPE_FSM);
      if (nullptr == fl || fl->empty())
      {
         PD_LOG(PDERROR, "fsm file not found");
         rc = SDB_FNE;
         goto error;
      }
      else if (1 != fl->size())
      {
         PD_LOG(PDERROR, "invalid count of fms file:%d", fl->size());
         rc = SDB_VESSEL_INTERNAL_ERR;
         goto error;
      }
      fn = &(fl->front());

      file = SDB_OSS_NEW fsmFile();
      if (nullptr == file)
      {
         PD_LOG(PDERROR, "failed to allocate mem");
         rc = SDB_OOM;
         goto error;
      }

      rc = sfm.openStorageFile(*fn, fileCtlFlags, *file);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to open file[%s], rc:%d",
                fn->getFileName(), rc);
         goto error;
      }

      rc = file->initToWork();
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to init fsm file:%d", rc);
         goto error;
      }

      _fsm = file;
      file = nullptr;
   done:
      return rc;
   error:
      if (nullptr != file)
      {
         file->close();
         SDB_OSS_DEL file;
      }
      goto done;
   }

   INT32 mainDataSpace::_create()
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(nullptr == _fsm, "must be null");
      storageCoreArgs args(FSM_FILE_PAGE_SIZE,
                           FSM_FILE_PAGE_COUNT_PER_SEG,
                           FSM_FILE_MAX_SEG_COUNT);

      createStorageFileOptions o;
      o.args = args;
      o.createAsTmpFile = TRUE;
      o.replaceWhenCreate = TRUE;
      o.flags = storageFileCtlFlag::MMAP_DATA_SEGMENT;
      storageFileName fn;
      const storagePathOptions &po = GET_THREAD_CONTEXT()->getEnv()->options.path;
      storageFileMaintainer sfm(&po, getSpaceID());
      o.secretValue = logicalPageSpace::getSecretValue();
                           
      fsmFile *file = SDB_OSS_NEW fsmFile();
      if (nullptr == file)
      {
         PD_LOG(PDERROR, "failed to allocate mem");
         rc = SDB_OOM;
         goto error;
      }

      if (!fn.build(FILE_TYPE_FSM, SPACE_TYPE_MAIN_DATA, 0))
      {
         PD_LOG(PDERROR, "failed to build fsm file name");
         rc = SDB_VESSEL_INTERNAL_ERR;
         goto error;
      }

      rc = sfm.createStorageFile(fn, o, *file);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to create tmp fsm file:%d", rc);
         goto error;
      }

      rc = file->initToWork();
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to init fsm file:%d", rc);
         goto error;
      }

      file->fsync();

      rc = file->removeShadowSuffix();
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to remove shadow suffix of file[%s]:%d",
                fn.getFileName(), rc);
         goto error;
      }

      _fsm = file;
      file = nullptr;
   done:
      return rc;
   error:
      if (nullptr != file)
      {
         file->destroy();
         SDB_OSS_DEL file;
      }
      goto done;
   }

   void mainDataSpace::_close()
   {
      if (nullptr != _fsm)
      {
         _fsm->fsync();
         _fsm->close();
         SDB_OSS_DEL _fsm;
         _fsm = nullptr;
         _storage.close();
      }
      return;
   }

   void mainDataSpace::_destroy()
   {
      if (nullptr != _fsm)
      {
         _fsm->destroy();
         SDB_OSS_DEL _fsm;
         _fsm = nullptr;
      }
      _storage.destroy();
      return;
   }

   INT32 mainDataSpace::getRuntimePageBuffer(requestContext *context,
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
      liteCache *lc = nullptr;

      if (OSS_UNLIKELY(nullptr == context ||
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

      pageSize = logicalPageSpace::getStorageCoreArgs().pageSize;
      lc = context->getEnv()->cacheConsole.getCache(pageSize);
      SDB_ASSERT(nullptr != lc, "page size did not match pool");
      rc = lc->allocate(gpid, options, tuple);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to allocate cache tuple of page[%s], rc:%d",
                gpid.toString().c_str(), rc);
         goto error;
      }

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

   INT32 mainDataSpace::getRuntimePageBufferToReset(requestContext *context,
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
      UINT32 pageSize = logicalPageSpace::getStorageCoreArgs().pageSize;
      liteCache *lc = nullptr;
      
      if (OSS_UNLIKELY(nullptr == context ||
                       INVALID_PAGE_ID == pid))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      lc = context->getEnv()->cacheConsole.getCache(pageSize);
      SDB_ASSERT(nullptr != lc, "page size did not match pool");
      rc = lc->allocateToReset(gpid, tuple);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to allocate cache tuple of page[%s], rc:%d",
                gpid.toString().c_str(), rc);
         goto error;
      }

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

   INT32 mainDataSpace::copyPageAndReinitBuffer(requestContext *context,
                                                PAGE_SNAPSHOT_VERION psv,
                                                PAGE_ID newPid,
                                                runtimePageBuffer &rpb)
   {
      INT32 rc = SDB_OK;
      logicalPageSpace::_runtimePageBufferIniter initer;
      liteCacheTuple tuple;
      GLOBAL_PAGE_ID gpid;
      UINT32 pageSize = logicalPageSpace::getStorageCoreArgs().pageSize;
      CHAR *buffer = nullptr;
      logRecordContext lrc;
      slice rs;
      liteCache *lc = nullptr;

      if (OSS_UNLIKELY(nullptr == context ||
                       INVALID_PAGE_SNAPSHOT_VERSION == psv ||
                       INVALID_PAGE_ID == newPid ||
                       !rpb.isValid() ||
                       !rpb.isCacheBuffer()))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      rs = rpb.getSlice();
      if (isPageCrashed((ossValuePtr)rs.data(), pageSize))
      {
         PD_LOG(PDERROR, "page[%s] may be crashed", rpb.getGlobalPid().toString().c_str());
         rc = SDB_VESSEL_PAGE_CRASHED;
         goto error;
      }

      buffer = context->allocateBuffer(pageSize);
      ossMemcpy(buffer, rs.data(), pageSize);
      rpb.fini();

      gpid.reset(logicalPageSpace::getSpaceID(),
                 getSpaceType(),
                 getStorageFileType(),
                 newPid);
      lc = context->getEnv()->cacheConsole.getCache(pageSize);
      SDB_ASSERT(nullptr != lc, "page size did not match pool");

      rc = lc->allocateToReset(gpid, tuple);
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
                 rs.data(),
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
      if (nullptr != buffer)
      {
         context->releaseBuffer(buffer);
      }
      return rc;
   error:
      goto done;
   }

   INT32 mainDataSpace::prepareCopyLog(requestContext *context,
                                       UINT32 pageSize,
                                       logRecordContext *lrc)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(nullptr != context, "can not be null");
      SDB_ASSERT(isValidPageSize(pageSize), "can not be invalid");
      SDB_ASSERT(nullptr != lrc, "can not be null");
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

   INT32 mainDataSpace::commit(requestContext *context,
                               UINT32 pageSize,
                               const void *pageBuffer,
                               const GLOBAL_PAGE_ID &gpid,
                               PAGE_ID lpid,
                               logRecordContext *lrc)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(nullptr != context, "can not be null");
      SDB_ASSERT(isValidPageSize(pageSize), "can not be invalid");
      SDB_ASSERT(nullptr != pageBuffer, "can not be null");
      SDB_ASSERT(gpid.isValid(), "can not be invalid");
      SDB_ASSERT(INVALID_PAGE_ID != lpid, "can not be invalid");
      SDB_ASSERT(nullptr != lrc, "can not be null");
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

}//namespace vessel
}//namespace engine