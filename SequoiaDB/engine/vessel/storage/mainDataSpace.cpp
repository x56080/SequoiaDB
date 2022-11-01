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
#include "dpsLogRecordDef.hpp"
#include "utilStr.hpp"
#include "vessel/fsmFile.h"
#include "vessel/freeSpaceMapDef.h"
#include "vessel/storageUtils.h"
#include "vessel/idMapFile.h"
#include "vessel/storageFileMaintainer.h"
#include "dpsWriteReqBuilder.hpp"

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