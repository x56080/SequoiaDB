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

   Source File Name = idxMBackupFile.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/
#include "vessel/idxMBackupFile.h"
#include "vessel/copyOnWriteSpaceCheckpoint.h"

namespace engine
{
namespace vessel
{
   static const UINT32 BACKUP_HEAD_VERSION = 1;
   INT32 idxMBackupFile::validateUserDefinedHead(const void *head)
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

   INT32 idxMBackupFile::initUserDefinedHead(const void *userDefinedOptions,
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