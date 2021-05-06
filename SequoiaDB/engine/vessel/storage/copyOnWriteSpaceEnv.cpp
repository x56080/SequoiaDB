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

   Source File Name = copyOnWriteSpaceEnv.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#include "vessel/copyOnWriteSpaceEnv.h"
#include "vessel/requestContext.h"
#include "vessel/storageUnit.h"
#include "vessel/instanceEnv.h"
#include "utilStr.hpp"
#include "vessel/idxMBackupFile.h"
#include "vessel/idxIDMapFile.h"

namespace engine
{
namespace vessel
{
   copyOnWriteSpaceEnv::copyOnWriteSpaceEnv()
   {}

   copyOnWriteSpaceEnv::~copyOnWriteSpaceEnv()
   {
      close();
   }

   INT32 copyOnWriteSpaceEnv::open(requestContext *context,
                                   storageUnit *su)
   {
      INT32 rc = SDB_OK;
      strSlice pathSlice;
      strSlice dirSlice;

      SDB_ASSERT(!isOpen(), "do not reinit");
      if (NULL== context ||
          NULL == su ||
          !su->isOpen())
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      _su = su;

      rc = _idxDeltaTable.init(context->getEnv()->options._cowsDeltaTableBucketCount,
                               context->getEnv()->options._cowsDeltaTableLatchCount);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to init idx page table:%d", rc);
         goto error;
      }

      pathSlice.reset(context->getEnv()->options.path.dataPath.c_str(),
                      context->getEnv()->options.path.dataPath.size());
      dirSlice.reset(su->getDirName());
      rc = openControlFile(context, pathSlice, dirSlice);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to open control file:%d", rc);
         goto error;
      }

      rc = restoreEnvToLastCheckpoint(context);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to restore cow env:%d", rc);
         goto error;
      }

   done:
      return rc;
   error:
      close();
      goto done;
   }

   void copyOnWriteSpaceEnv::close()
   {
      _open = FALSE;
      _su = NULL;
      _controlFile.close();
      _checkpoint = copyOnWriteSpaceCheckpoint();
      _idxDeltaTable.fini();
      SAFE_OSS_FREE(_deltaLogBuffer);
      return;
   }

   INT32 copyOnWriteSpaceEnv::openControlFile(requestContext *context,
                                              const strSlice &dataPath,
                                              const strSlice &suDir)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(NULL != context, "can not be null");
      SDB_ASSERT(!dataPath.empty() && !suDir.empty(), "can not be empty");
      SDB_ASSERT(!_controlFile.isOpen(), "do not reopen");

      UINT32 bufferSize = dataPath.strLen() + suDir.strLen() + 8;
      CHAR *buffer = context->allocateBuffer(bufferSize);
      if (NULL == buffer)
      {
         PD_LOG(PDERROR, "failed to allocate mem");
         rc = SDB_OOM;
         goto error;
      }

      rc = utilBuildFullPath(dataPath.str(), suDir.str(),
                             bufferSize, buffer);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to build path:%d", rc);
         goto error;
      }

      rc = _controlFile.open(buffer, TRUE);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to open control file:%d", rc);
         goto error;
      }

      if (0 == _controlFile.getAvailableVersionCount())
      {
         PD_LOG(PDERROR, "non available control file");
         rc = SDB_VESSEL_INTERNAL_ERR;
         goto error;
      }
   done:
      if (NULL != buffer)
      {
         context->releaseBuffer(buffer, bufferSize);
      }
      return rc;
   error:
      _controlFile.close();
      goto done;
   }

   INT32 copyOnWriteSpaceEnv::restoreEnvToLastCheckpoint(requestContext *context)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(NULL != context, "can not be null");
      SDB_ASSERT(NULL != _su, "can not be null");
      SDB_ASSERT(_controlFile.isOpen(), "must be open");
      SDB_ASSERT(0 < _controlFile.getAvailableVersionCount(), "impossible");
      copyOnWriteSpaceCheckpoint oldest;
      UINT64 cv = 0;

      if (0 == _controlFile.getAliveVersionCount())
      {
         goto done;
      }

      rc = _controlFile.readLatestVersion(cv, sizeof(copyOnWriteSpaceCheckpoint),
                                          &_checkpoint);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to read latest checkpoint:%d", rc);
         goto error;
      }

      rc = _controlFile.readOldestVersion(cv, sizeof(copyOnWriteSpaceCheckpoint),
                                          &oldest);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to read oldest checkpoint:%d", rc);
         goto error;
      }

      rc = restoreStorageUnit(context, oldest, _checkpoint);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to restore storage unit:%d", rc);
         goto error;
      }

   done:
      return rc;
   error:
      _checkpoint = copyOnWriteSpaceCheckpoint();
      goto done;
   }

   INT32 copyOnWriteSpaceEnv::restoreStorageUnit(requestContext *context,
                                                 const copyOnWriteSpaceCheckpoint &oldest,
                                                 const copyOnWriteSpaceCheckpoint &latest)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(NULL != _su && _su->isOpen(), "can not be invalid");
      SDB_ASSERT(NULL != context, "can not be null");
      SDB_ASSERT(oldest.isValid() && latest.isValid(), "must be valid");

      rc = restoreIdxMetaFile(context, latest);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to restore idx meta file:%d", rc);
         goto error;
      }
   done:
      return rc;
   error:
      goto done;
   }

   INT32 copyOnWriteSpaceEnv::restoreIdxMetaFile(requestContext *context,
                                                 const copyOnWriteSpaceCheckpoint &checkpoint)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(NULL != _su && _su->isOpen(), "can not be invalid");
      SDB_ASSERT(NULL != context, "can not be null");
      SDB_ASSERT(checkpoint.isValid(), "must be valid");
      
      BOOLEAN isIdxMetaBackupOpen = (NULL != _su->getIdxMBackupFile());
      if (0 == checkpoint.idxMFileFullyFlushedTimes)
      {
         goto done;
      }

      if (isIdxMetaBackupOpen)
      {
         rc = restoreIdxMetaFileFromBackup(context, checkpoint);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to restore idx meta file from backup:%d", rc);
            goto error;
         }
      }
   done:
      return rc;
   error:
      goto done;
   }

   INT32 copyOnWriteSpaceEnv::restoreIdxMetaFileFromBackup(requestContext *context,
                                                           const copyOnWriteSpaceCheckpoint &checkpoint)
   {
      INT32 rc = SDB_OK;
      idxIDMapFile *idx = _su->getIdxMetaFile();
      idxMBackupFile *backup = _su->getIdxMBackupFile();
      copyOnWriteSpaceCheckpoint checkpointInHead;

      if (NULL == idx)
      {
         PD_LOG(PDERROR, "idx m file not found");
         rc = SDB_VESSEL_INTERNAL_ERR;
         goto error;
      }
      if (NULL == backup)
      {
         PD_LOG(PDERROR, "idx m backup file not found");
         rc = SDB_VESSEL_INTERNAL_ERR;
         goto error;
      }

      rc = idx->restoreFromBackup(backup);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to restore idx m file form backup:%d", rc);
         goto error;
      }

      rc = idx->getCheckpointInHead(checkpointInHead);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to get checkpoint in head:%d", rc);
         goto error;
      }

      if (!(checkpoint == checkpointInHead))
      {
         PD_LOG(PDERROR, "checkpoint does not match user defined head");
         rc = SDB_VESSEL_INTERNAL_ERR;
         goto error;
      }

      rc = _su->destroyIdxMBackupFile();
      if (SDB_OK != rc)
      {
         PD_LOG(PDSEVERE, "failed to destroy idx m backup file:%d", rc);
         goto error;
      }
   done:
      return rc;
   error:
      goto done;
   }

   INT32 copyOnWriteSpaceEnv::ensureDeltaLogBuffer()
   {
      INT32 rc = SDB_OK;
      if (NULL != _deltaLogBuffer)
      {
         goto done;
      }

      _deltaLogBuffer = (CHAR *)SDB_OSS_MALLOC(COW_DELTA_LOG_PAGE_SIZE);
      if (NULL == _deltaLogBuffer)
      {
         PD_LOG(PDERROR, "failed to allocate mem");
         rc = SDB_OOM;
         goto error;
      }
   done:
      return rc;
   error:
      goto done;
   }
}//namespace vessel
}//namespace engine