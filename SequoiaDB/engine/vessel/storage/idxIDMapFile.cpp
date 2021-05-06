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

   Source File Name = idxIDMapFile.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#include "vessel/idxIDMapFile.h"
#include "vessel/idxMBackupFile.h"
#include "vessel/copyOnWriteSpaceCheckpoint.h"

namespace engine
{
namespace vessel
{
   INT32 idxIDMapFile::restoreFromBackup(idxMBackupFile *backupFile)
   {
      INT32 rc = SDB_OK;
      ossValuePtr ptr = 0;
      ossValuePtr ptrOfBackup = 0;
      const storageFileHead *head = NULL;
      const storageFileHead *headOfBk = NULL;
      UINT32 segmentSize = 0;
      SDB_ASSERT(NULL != backupFile, "can not be null");

      if (NULL == backupFile ||
          !backupFile->isOpen() ||
          !isOpen())
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }

      head = &getCommonHeadInMem();
      headOfBk = &(backupFile->getCommonHeadInMem());
      if (head->spaceID != headOfBk->spaceID)
      {
         PD_LOG(PDERROR, "space id[%d, %d] not same",
                head->spaceID, headOfBk->spaceID);
         rc = SDB_VESSEL_INTERNAL_ERR;
         goto error;
      }
      else if (head->logicalID != headOfBk->logicalID)
      {
         PD_LOG(PDERROR, "logical id[%d, %d] not same",
                head->logicalID, headOfBk->logicalID);
         rc = SDB_VESSEL_INTERNAL_ERR;
         goto error;
      }
      else if (head->userDefinedHeadLen != headOfBk->userDefinedHeadLen)
      {
         PD_LOG(PDERROR, "userDefinedHeadLen [%d, %d] not same",
                head->userDefinedHeadLen, headOfBk->userDefinedHeadLen);
         rc = SDB_VESSEL_INTERNAL_ERR;
         goto error;
      }
      else if (head->maxPageCountPerSeg != headOfBk->maxPageCountPerSeg)
      {
         PD_LOG(PDERROR, "maxPageCountPerSeg [%d, %d] not same",
                head->maxPageCountPerSeg, headOfBk->maxPageCountPerSeg);
         rc = SDB_VESSEL_INTERNAL_ERR;
         goto error;
      }
      else if (head->pageSize != headOfBk->pageSize)
      {
         PD_LOG(PDERROR, "pageSize [%d, %d] not same",
                head->pageSize, headOfBk->pageSize);
         rc = SDB_VESSEL_INTERNAL_ERR;
         goto error;
      }
      else if (getSegmentCount() < backupFile->getSegmentCount())
      {
         PD_LOG(PDERROR, "invalid segment count[%d] in idx m file", getSegmentCount());
         rc = SDB_VESSEL_INTERNAL_ERR;
         goto error;
      }
      

      rc = backupFile->getUserDefinedHeadPtr(ptrOfBackup);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to get user defined head of backup:%d", rc);
         goto error;
      }

      rc = getUserDefinedHeadPtr(ptr);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to get user defined head:%d", rc);
         goto error;
      }

      ossMemcpy((void *)ptr, (const void *)ptrOfBackup, head->userDefinedHeadLen);
      segmentSize = head->pageSize * head->maxPageCountPerSeg;
      /// do not use head->getSegmentCount()
      for (UINT32 i = 0; i < backupFile->getSegmentCount(); ++i)
      {
         rc = backupFile->getSegmentPtr(i, ptrOfBackup);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to get segment ptr of backup:%d", rc);
            goto error;
         }

         rc = getSegmentPtr(i, ptr);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to get segment ptr:%d", rc);
            goto error;
         }

         ossMemcpy((void *)ptr, (const void *)ptrOfBackup, segmentSize);
      }

      rc = ossMmapFile::flushAll(TRUE);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to flush file:%d", rc);
         goto error;
      }

   done:
      return rc;
   error:
      goto done;
   }

   INT32 idxIDMapFile::getCheckpointInHead(copyOnWriteSpaceCheckpoint &checkpoint)
   {
      INT32 rc = SDB_OK;
      ossValuePtr ptr = 0;
      rc = getUserDefinedHeadPtr(ptr);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to get user defined head ptr:%d", rc);
         goto error;
      }

      checkpoint = *((const copyOnWriteSpaceCheckpoint *)ptr);
   done:
      return rc;
   error:
      goto done;
   }

   INT32 idxIDMapFile::validateUserDefinedHead(const void *head)
   {
      INT32 rc = SDB_OK;
      const copyOnWriteSpaceCheckpoint *checkpoint = (const copyOnWriteSpaceCheckpoint *)head;
      if (NULL == head)
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      if (!checkpoint->isValid())
      {
         PD_LOG(PDERROR, "checkpoint is not valid");
         rc = SDB_VESSEL_PAGE_CRASHED;
         goto error;
      }

      if (0 == checkpoint->idxMFilePageCount)
      {
         PD_LOG(PDERROR, "idxMFilePageCount impossible to be zero");
         rc = SDB_VESSEL_PAGE_CRASHED;
         goto error;
      }

   done:
      return rc;
   error:
      goto done;
   }

   INT32 idxIDMapFile::initUserDefinedHead(const void *userDefinedOptions,
                                            CHAR *headBuf)
   {
      INT32 rc = SDB_OK;
      copyOnWriteSpaceCheckpoint *checkpoint = NULL;
      if (NULL == userDefinedOptions ||
          NULL == headBuf)
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      checkpoint = (copyOnWriteSpaceCheckpoint *)headBuf;
      *checkpoint = *((const copyOnWriteSpaceCheckpoint *)userDefinedOptions);
   done:
      return rc;
   error:
      goto done;
   }
}//namespace vessel
}//namespace engine