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
   {}
   
   INT32 mainDataSpace::initMetaPageWhenCreateCS(requestContext *context,
                                                 const csMetaRecord &record,
                                                 const slice &options)
   {
      INT32 rc = SDB_OK;
      mmapPagePointer ptr;
      PAGE_ID lpid = 0;
      PAGE_ID pid = 0;
      PAGE_SNAPSHOT_VERION psv = INVALID_PAGE_SNAPSHOT_VERSION;
      ISession *session = NULL;
      IRedoLogger *logger = NULL;
      csMetaRecord *recordOnDisk = NULL;
      logRecordContext lrc;
      SPACE_ID sid = INVALID_SPACE_ID;


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
      session = context->getSession();
      logger = context->getOuterResource()->logger;

      SDB_ASSERT(0 == _storage.getTotalSegmentCountAllocated(), "must be empty");
      rc = _storage.extendPageSpace(NULL);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to extend storage:%d", rc);
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

      /// init local mapping
      rc = mapGlobalMetaPageWhenCreating(psv, lpid, pid);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to update local mapping:%d", rc);
         goto error;
      }

      /// prepare dps log
      lrc.open(LOG_TYPE_CS_CRT);
      lrc.setDDL();
      lrc.setResetPage();
      lrc.prepush(sizeof(SPACE_ID));
      lrc.prepush(CS_META_RECORD_LEN);
      lrc.prepush(options.len());
      lrc.prepushDone();
      rc = logger->prepare(session, &lrc);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to prepare log:%d", rc);
         goto error;
      }

      /// update page lsn
      if (!updatePageLsn(ptr.get(), lrc.getLsn()))
      {
         logger->abort(session, &lrc);
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
      rc = logger->pushLogRecordElement(session, &lrc, DPS_LOG_CSCRT_VESSEL_SID,
                                        sizeof(SPACE_ID), &sid);
      if (SDB_OK != rc)
      {
         logger->abort(session, &lrc);
         PD_LOG(PDERROR, "failed to push ele[%d], rc:%d",
                DPS_LOG_CSCRT_VESSEL_SID, rc);
         goto error;
      }

      rc = logger->pushLogRecordElement(session, &lrc, DPS_LOG_CSCRT_VESSEL_META,
                                        CS_META_RECORD_LEN, &record);
      if (SDB_OK != rc)
      {
         logger->abort(session, &lrc);
         PD_LOG(PDERROR, "failed to push ele[%d], rc:%d",
                DPS_LOG_CSCRT_VESSEL_META, rc);
         goto error;
      }

      rc = logger->pushLogRecordElement(session, &lrc, DPS_LOG_CSCRT_VESSEL_OPTIONS,
                                        options.len(), options.data());
      if (SDB_OK != rc)
      {
         logger->abort(session, &lrc);
         PD_LOG(PDERROR, "failed to push ele[%d], rc:%d",
                DPS_LOG_CSCRT_VESSEL_OPTIONS, rc);
         goto error;
      }

      rc = logger->commit(session, &lrc);
      if (SDB_OK != rc)
      {
         logger->abort(session, &lrc);
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
      mmapPagePointer ptr;
      UINT32 pageSize = 0;
      PAGE_ID pid = INVALID_PAGE_ID;
      PAGE_SNAPSHOT_VERION psv = INVALID_PAGE_SNAPSHOT_VERSION;
      const csMetaRecord *recordOnDisk = NULL;

      if (OSS_UNLIKELY(!isOpen()))
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }

      pageSize = logicalPageSpace::getStorageCoreArgs().pageSize;
      SDB_ASSERT(isValidPageSize(pageSize), "must be valid");

      rc = logicalPageSpace::getPageMappingAtNonruntime(context, 0, pid, psv, ptr);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to get page mapping of meta page:%d", rc);
         goto error;
      }

      rc = validatePage(ptr.get(), PAGE_TYPE_CS_META, pageSize,
                        pid, 0, psv);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to validate page:%d", rc);
         goto error;
      }

      recordOnDisk = (const csMetaRecord *)(ptr.get() + PAGE_HEAD_SIZE);
      if (!recordOnDisk->isValid())
      {
         PD_LOG(PDERROR, "failed to read valid meta data record");
         rc = SDB_VESSEL_INTERNAL_ERR;
         goto error;
      }
      
      record = *recordOnDisk;

   done:
      return rc;
   error:
      goto done;
   }

   INT32 mainDataSpace::mapGlobalMetaPageWhenCreating(PAGE_SNAPSHOT_VERION psv,
                                                      PAGE_ID lpid,
                                                      PAGE_ID pid)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(INVALID_PAGE_SNAPSHOT_VERSION != psv, "can not be invalid");
      SDB_ASSERT(INVALID_PAGE_ID != lpid, "can not be invalid");
      SDB_ASSERT(INVALID_PAGE_ID != pid, "can not be invalid");
      mappedLogicalPageId mpid(lpid, pid);
      idMapSlot slot(psv, pid);
      deltaLogRecordBuilder builder;

      rc = builder.buildMappingLog(psv, 1, &mpid);
      if (SDB_OK!= rc)
      {
         PD_LOG(PDERROR, "failed to build mapping log:%d", rc);
         goto error;
      }

      rc = logicalPageSpace::getLogConsole().append(builder.getDeltaLogRecord());
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to append delta log:%d", rc);
         goto error;
      }

      rc = logicalPageSpace::getCache().upsert(lpid, slot);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to upsert into cache:%d", rc);
         goto error;
      }
   done:
      return rc;
   error:
      goto done;
   }

   INT32 mainDataSpace::ensureNameFile(const CHAR *csName)const
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(NULL != csName, "can not be null");
      const storageFileCreater &creater = logicalPageSpace::getCreater();
      SDB_ASSERT(creater.isValid(), "can not be invalid");
      CHAR fileName[MAX_FILE_NAME_LEN + 1] = {0};
      CHAR fullPath[OSS_MAX_PATHSIZE + 1] = {0};
      CHAR nameBuffer[DMS_COLLECTION_SPACE_NAME_SZ + 1] = {0};
      strSlice suffix(SIMPLE_FILE_SUFFIX_CSNAME);
      UINT32 flags = OSS_READWRITE|OSS_EXCLUSIVE|OSS_REPLACE;
      OSSFILE file;
      strSlice nameSlice(csName);

      if (nameSlice.empty() ||
          DMS_COLLECTION_SPACE_NAME_SZ < nameSlice.strLen())
      {
         PD_LOG(PDERROR, "invalid cs name");
         rc = SDB_INVALIDARG;
         goto error;
      }

      ossMemcpy(nameBuffer, csName, nameSlice.strLen());
      nameBuffer[nameSlice.strLen()] = '\n';

      if (!vesselFileName::buildSimpleName(logicalPageSpace::getSpaceID(),
                                           suffix, MAX_FILE_NAME_LEN + 1, fileName))
      {
         PD_LOG(PDERROR, "failed to build csname file");
         rc = SDB_VESSEL_INTERNAL_ERR;
         goto error;
      }

      rc = utilBuildFullPath(creater.getDir().c_str(),
                             fileName, OSS_MAX_PATHSIZE + 1,
                             fullPath);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to build fullpath:%d", rc);
         goto error;
      }

      rc = ossOpen(fullPath, flags, OSS_RU|OSS_WU|OSS_RG, file);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to create file[%s], rc:%d", fullPath, rc);
         goto error;
      }

      rc = ossWriteN(&file, nameBuffer, nameSlice.strLen() + 1);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to write file[%s], rc:%d", fullPath, rc);
         goto error;
      }

      rc = ossFdatasync(&file);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to fdatasync file[%s], rc:%d", fullPath, rc);
         goto error;
      }

      ossClose(file);
      ossChmod(fullPath, OSS_RU);
   done:
      return rc;
   error:
      if (file.isOpened())
      {
         ossClose(file);
         ossDelete(fullPath);
      }
      goto done;
   }

   INT32 mainDataSpace::_open(requestContext *context,
                              const storageFileLoader &loader)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(NULL == _fsm, "must be null");
      const storageFileCreater &creater = logicalPageSpace::getCreater();
      SDB_ASSERT(creater.isValid(), "can not be inalvid");
      fsmFile *file = NULL;
      const vesselFileName *fn = NULL;

      const FILE_NAME_LIST *fl = loader.getFileList(FILE_TYPE_FSM);
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

      rc = file->open(creater.getDirSlice(), *fn);
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
      SDB_ASSERT(NULL == _fsm, "must be null");
      storageCoreArgs args(FSM_FILE_PAGE_SIZE,
                           FSM_FILE_PAGE_COUNT_PER_SEG,
                           FSM_FILE_MAX_SEG_COUNT);
                           
      const storageFileCreater &creater = logicalPageSpace::getCreater();
      SDB_ASSERT(creater.isValid(), "can not be inalvid");
      fsmFile *file = SDB_OSS_NEW fsmFile();
      if (NULL == file)
      {
         PD_LOG(PDERROR, "failed to allocate mem");
         rc = SDB_OOM;
         goto error;
      }

      rc = creater.createTmpFile(FILE_TYPE_FSM, 0, args, file);
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

      rc = renameToFormalAndReopen(creater.getDirSlice(),
                                   FALSE, FILE_SHADOW_SUFFIX_TMP,
                                   file);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to rename to formal file:%d", rc);
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
         _fsm->close();
         SDB_OSS_DEL _fsm;
         _fsm = NULL;
      }
      return;
   }

   void mainDataSpace::_destroy()
   {
      INT32 rc = ensureNameFileRemoved();
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to remove name file:%d", rc);
      }
      if (NULL != _fsm)
      {
         _fsm->destroy();
         SDB_OSS_DEL _fsm;
         _fsm = NULL;
      }
      return;
   }

   INT32 mainDataSpace::ensureNameFileRemoved()
   {
      INT32 rc = SDB_OK;

      const storageFileCreater &creater = logicalPageSpace::getCreater();
      SDB_ASSERT(creater.isValid(), "can not be invalid");
      CHAR fileName[MAX_FILE_NAME_LEN + 1] = {0};
      CHAR fullPath[OSS_MAX_PATHSIZE + 1] = {0};
      strSlice suffix(SIMPLE_FILE_SUFFIX_CSNAME);

      if (!vesselFileName::buildSimpleName(logicalPageSpace::getSpaceID(),
                                           suffix, MAX_FILE_NAME_LEN + 1, fileName))
      {
         PD_LOG(PDERROR, "failed to build csname file");
         rc = SDB_VESSEL_INTERNAL_ERR;
         goto error;
      }

      rc = utilBuildFullPath(creater.getDir().c_str(),
                             fileName, OSS_MAX_PATHSIZE + 1,
                             fullPath);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to build fullpath:%d", rc);
         goto error;
      }

      rc = ossDelete(fullPath);
      if (SDB_FNE == rc)
      {
         rc = SDB_OK;
      }
      else if (SDB_OK != rc)
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