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
#include "dpsDef.hpp"
#include "vessel/deltaLogRecordBuilder.h"
#include "dpsLogRecordDef.hpp"
#include "utilStr.hpp"
#include "vessel/fsmFile.h"
#include "vessel/freeSpaceMapDef.h"
#include "vessel/storageUtils.h"
#include "vessel/idMapFile.h"
#include "vessel/storageFileMaintainer.h"
#include "dpsJournalPad.hpp"

namespace engine
{
namespace vessel
{
   mainDataSpace::mainDataSpace(const storageUnitManifest *manifest):
   logicalPageSpace(manifest)
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
      PAGE_ID pid = INVALID_SPACE_ID;
      PAGE_SNAPSHOT_VERION psv = INVALID_PAGE_SNAPSHOT_VERSION;
      IDataJournal *journal = nullptr;
      csMetaBlock *blockOnDisk = nullptr;
      lpageDescriptor desc;
      dpsStackJournalPad jpad;
      dpsLogRecordHeader jres;
      dpsPackedRequest jrequest;

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

      journal = context->getOuterResource()->journal;

      psv = context->getEnv()->dms.getOnlinePageSnapshotVersion();

      rc = _getSpaceMgr().reserve(pid);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to occupy meta page pid:%d", rc);
         goto error;
      }

      rc = getFileCluster()->getPageMmapPtr(pid, ptr);

      /// init page
      if (!initCSMetaBlockPage(getFileCluster()->getCoreArgs().pageSize,
                               pid, lpid, psv, (void *)(ptr.get())))
      {
         PD_LOG(PDERROR, "faield to init gmp");
         rc = SDB_VESSEL_INTERNAL_ERR;
         goto error;
      }

      blockOnDisk = (csMetaBlock *)(ptr.get() + PAGE_HEAD_SIZE);
      *blockOnDisk = block;

      jpad.setType(LOG_TYPE_CS_CRT);
      jpad.setFlag(DPS_LOG_FLAG_VESSEL);
      rc = jpad.appendInt32(DPS_LOG_CSCRT_VESSEL_SID,
                            logicalPageSpace::getSpaceID());
      if (OSS_UNLIKELY(SDB_OK != rc))
      {
         PD_LOG(PDERROR, "failed to append sid into pad:%d", rc);
         goto error;
      }

      rc = jpad.append(DPS_LOG_CSCRT_VESSEL_META, CS_META_BLOCK_LEN, &block);
      if (OSS_UNLIKELY(SDB_OK != rc))
      {
         PD_LOG(PDERROR, "failed to append meta block into pad:%d", rc);
         goto error;
      }

      jrequest = jpad.done();

      rc = journal->write(jrequest, dpsWriteOptions(), &jres);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to write journal:%d", rc);
         goto error;
      }

      /// update page lsn
      if (!updatePageLsn(ptr.get(), jres._lsn))
      {
         PD_LOG(PDERROR, "failed to update page lsn:%d", rc);
         goto error;
      }

      rc = getFileCluster()->fysncPage(pid);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to fsync page:%d", rc);
         goto error;
      }

      desc.pid = pid;
      desc.psv = psv;
      rc = _getPageMapping().set(lpid, desc);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to create lpage mapping:%d", rc);
         goto error;
      }

   done:
      return rc;
   error:
      //SDB_ASSERT(DPS_INVALID_LSN_OFFSET != jres._lsn, "to do: rollback");
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

      pageSize = getFileCluster()->getCoreArgs().pageSize;
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

   INT32 mainDataSpace::_onOpenFinished(const storageFileLoader &loader)
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

   INT32 mainDataSpace::_onCreationFinished()
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
      o.secretValue = getManifest()->secretValue;
                           
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

   void mainDataSpace::_onClosingStarted()
   {
      if (nullptr != _fsm)
      {
         _fsm->fsync();
         _fsm->close();
         SDB_OSS_DEL _fsm;
         _fsm = nullptr;
      }
      return;
   }

   void mainDataSpace::_onDestroyStarted()
   {
      if (nullptr != _fsm)
      {
         _fsm->destroy();
         SDB_OSS_DEL _fsm;
         _fsm = nullptr;
      }
      return;
   }

}//namespace vessel
}//namespace engine