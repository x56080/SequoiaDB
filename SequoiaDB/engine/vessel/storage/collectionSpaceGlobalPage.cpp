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

   Source File Name = collectionSpaceGlobalPage.cpp

   Descriptive Name =

   When/how to use: this program may be used on binary and text-formatted
   versions of PMD component. This file contains functions for agent processing.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#include "vessel/collectionSpaceGlobalPage.h"

namespace engine
{
namespace vessel
{
   BOOLEAN metaRecordIsValid(const csMetaRecord &record)
   {
      BOOLEAN r = FALSE;
      if (CMR_VERSION_1 != record.version)
      {
         goto done;
      }
      else if (DMS_INVALID_LOGICCSID == record.logicalID)
      {
         goto done;
      }
      else if (0 == record.name[0] ||
               0 != record.name[DMS_COLLECTION_SPACE_NAME_SZ])
      {
         goto done;
      }

      r = TRUE;
   done:
      return r;
   }

   BOOLEAN initGmp(UINT32 pageSize, PAGE_ID pid,
                   const strSlice &name, UINT32 uniqueID,
                   UINT32 logicalID, CHAR *buf)
   {
      BOOLEAN r = FALSE;
      csMetaRecord *head = NULL;
      if (OSS_UNLIKELY(!isValidPageSize(pageSize) ||
                       INVALID_PAGE_ID == pid ||
                       name.empty() ||
                       DMS_INVALID_LOGICCSID == logicalID ||
                       NULL == buf))
      {
         goto done;
      }

      initCommonPage(PAGE_TYPE_CS_META, pageSize, pid, buf);
      head = (csMetaRecord *)(buf + PAGE_HEAD_LEN);
      head->version = CMR_VERSION_1;
      head->status = CMR_STATUS_CREATING;
      head->flags = 0;
      head->uniqueID = uniqueID;
      head->logicalID = logicalID;
      ossMemcpy(head->name, name.str(), name.strLen() + 1);

      r = TRUE;
   done:
      return r;
   }
}//namespace vessel
}//namespace engine

