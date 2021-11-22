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
#include "vessel/collectionRecordPage.h"
#include "vessel/csgpAccessor.h"
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
                                                 const csMetaRecord &record,
                                                 const slice &options)
   {
      INT32 rc = SDB_OK;
      mmapPagePointer ptr;
      PAGE_ID lpid = COLLECTION_SPACE_GP_LPID;
      PAGE_ID pid = 0;
      PAGE_SNAPSHOT_VERION psv = INVALID_PAGE_SNAPSHOT_VERSION;
      IExecutor *executor = NULL;
      IRedoLogger *logger = NULL;
      csMetaRecord *recordOnDisk = NULL;
      logRecordContext lrc;
      SPACE_ID sid = INVALID_SPACE_ID;
      deltaLogRecordBuilder builder;
      mappedLogicalPageId mid(lpid, pid);


      if (OSS_UNLIKELY(NULL == context ||
                       !record.isValid()))
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

      SDB_ASSERT(0 == _storage.getTotalSegmentCountAllocated(), "must be empty");
      rc = _storage.extendPageSpace(context, 1, NULL);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to extend storage:%d", rc);
         goto error;
      }

      rc = _storage.occupyPage(context, pid);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to occupy meta page pid:%d", rc);
         goto error;
      }

      psv = context->getEnv()->dms.getOnlinePageSnapshotVersion();
      rc = _storage.getDataPagePtr(pid, ptr);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to get page[%d] ptr:%d", pid, rc);
         goto error;
      }


      /// init page
      if (!initGmp(_storage.getCoreArgs().pageSize,
                   pid, lpid, psv, (void *)(ptr.get())))
      {
         PD_LOG(PDERROR, "faield to init gmp");
         rc = SDB_VESSEL_INTERNAL_ERR;
         goto error;
      }

      recordOnDisk = (csMetaRecord *)(ptr.get() + PAGE_HEAD_SIZE);
      *recordOnDisk = record;

      /// prepare dps log
      lrc.open(LOG_TYPE_CS_CRT);
      lrc.setDDL();
      lrc.setResetPage();
      lrc.prepush(sizeof(SPACE_ID));
      lrc.prepush(CS_META_RECORD_LEN);
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
                                        CS_META_RECORD_LEN, &record);
      if (SDB_OK != rc)
      {
         logger->abort(executor, &lrc);
         PD_LOG(PDERROR, "failed to push ele[%d], rc:%d",
                DPS_LOG_CSCRT_VESSEL_META, rc);
         goto error;
      }

      rc = logger->pushLogRecordElement(executor, &lrc, DPS_LOG_CSCRT_VESSEL_OPTIONS,
                                        options.getSize(), options.getRPtr());
      if (SDB_OK != rc)
      {
         logger->abort(executor, &lrc);
         PD_LOG(PDERROR, "failed to push ele[%d], rc:%d",
                DPS_LOG_CSCRT_VESSEL_OPTIONS, rc);
         goto error;
      }

      rc = logicalPageSpace::getLogConsole().append(context, builder.getDeltaLogRecord());
      if (SDB_OK != rc)
      {
         logger->abort(executor, &lrc);
         PD_LOG(PDERROR, "failed to append delta log:%d", rc);
         goto error;
      }

      rc = logicalPageSpace::getCache().put(lpid, idMapSlot(psv, pid));
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

   INT32 mainDataSpace::readMetaRecordWhenOpen(requestContext *context,
                                               csMetaRecord &record)
   {
      INT32 rc = SDB_OK;
      UINT32 pageSize = 0;
      logicalPageBuffer lpb;
      csgpAccessor accessor;

      if (OSS_UNLIKELY(!isOpen()))
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }

      pageSize = logicalPageSpace::getStorageCoreArgs().pageSize;
      SDB_ASSERT(isValidPageSize(pageSize), "must be valid");

      rc = getLogicalPageBuffer(context, COLLECTION_SPACE_GP_LPID,
                                ossSharedLatchMode(OSS_SHARED_LATCH_MODE_ENUM_SHARED), lpb);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to get page mapping of meta page:%d", rc);
         goto error;
      }

      rc = accessor.read(context, &lpb, record);
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

   INT32 mainDataSpace::_open(requestContext *context,
                              const storageFileLoader &loader)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(NULL != context, "can not be null");
      SDB_ASSERT(NULL == _fsm, "must be null");
      fsmFile *file = NULL;
      const vesselFileName *fn = NULL;
      storageUnit *su = context->getEnv()->dms.getStorageUnit(context->getSpaceID());
      SDB_ASSERT(NULL != su, "can not be null");

      const FILE_NAME_LIST *fl = loader.getFileList(SPACE_TYPE_MAIN_DATA,
                                                    FILE_TYPE_FSM);
      if (NULL == fl || fl->empty())
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
      if (NULL == file)
      {
         PD_LOG(PDERROR, "failed to allocate mem");
         rc = SDB_OOM;
         goto error;
      }

      rc = su->openStorageFile(*fn, file);
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
      file = NULL;
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

   INT32 mainDataSpace::_create(requestContext *context)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(NULL != context, "can not be null");
      SDB_ASSERT(NULL == _fsm, "must be null");
      storageCoreArgs args(FSM_FILE_PAGE_SIZE,
                           FSM_FILE_PAGE_COUNT_PER_SEG,
                           FSM_FILE_MAX_SEG_COUNT);

      createStorageFileOptions o;
      o.args = args;
      o.createAsTmpFile = TRUE;
      o.replaceWhenCreate = TRUE;
      vesselFileName fn;

      const sortedStorageFileList &imf = getIdMapFileList();
      SDB_ASSERT(!imf.isEmpty(), "can not be empty");
      o.secretValue = imf.getBack<idMapFile>()->getCommonHeadInMem().secretValue;

      storageUnit *su = context->getEnv()->dms.getStorageUnit(context->getSpaceID());
      SDB_ASSERT(NULL != su, "can not be null");
                           
      fsmFile *file = SDB_OSS_NEW fsmFile();
      if (NULL == file)
      {
         PD_LOG(PDERROR, "failed to allocate mem");
         rc = SDB_OOM;
         goto error;
      }

      if (!fn.build(context->getSpaceID(), FILE_TYPE_FSM,
                    SPACE_TYPE_MAIN_DATA, 0))
      {
         PD_LOG(PDERROR, "failed to build fsm file name");
         rc = SDB_VESSEL_INTERNAL_ERR;
         goto error;
      }

      rc = su->createStorageFile(fn, o, slice(), file);
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

   void mainDataSpace::_close()
   {
      if (NULL != _fsm)
      {
         _fsm->fsync();
         _fsm->close();
         SDB_OSS_DEL _fsm;
         _fsm = NULL;
      }
      return;
   }

   void mainDataSpace::_destroy(requestContext *context)
   {
      if (NULL != _fsm)
      {
         _fsm->destroy();
         SDB_OSS_DEL _fsm;
         _fsm = NULL;
      }
      return;
   }

}//namespace vessel
}//namespace engine