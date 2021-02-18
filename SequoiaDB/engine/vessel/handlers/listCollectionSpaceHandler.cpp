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

   Source File Name = listCollectionSpaceHandler.cpp

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

#include "vessel/listCollectionSpaceHandler.h"
#include "pdTrace.hpp"
#include "vessel/listCSCursor.h"
#include "vessel/instanceEnv.h"
#include "vessel/collectionSpace.h"
#include "vessel/listCollectionSpaceDef.h"
#include "vessel/slice.h"
#include "vessel/extentStorageUnit.h"

namespace engine
{
namespace vessel
{
   INT32 listCollectionSpaceHandler::doit(listCSCursor *cursor)
   {
      INT32 rc = SDB_OK;
      SPACE_ID sid = INVALID_SPACE_ID;
      collectionSpace *obj = NULL;
      SDB_ASSERT(NULL != cursor, "can not be null");
      UINT32 logicalID = DMS_INVALID_LOGICCSID;

      if (OSS_UNLIKELY(!cursor->isOpen()))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      do
      {
         if (getSession()->quit())
         {
            rc = SDB_APP_INTERRUPT;
            goto error;
         }
         
         logicalID = cursor->getLogicalID();
         rc = getEnv()->objContainer.getSpaceIDByUpperBound(logicalID, sid);
         if (SDB_DMS_CS_NOTEXIST == rc)
         {
            rc = SDB_OK;
            cursor->pushEnd();
            break;
         }
         else if (SDB_OK != rc)
         {
            goto error;
         }
         
         rc = getContext()->lockSpaceID(sid, SHARED);
         if (SDB_OK != rc)
         {
            goto error;
         }

         rc = getEnv()->objContainer.getCSUnderIDLocked(getContext(), &obj);
         if (SDB_DMS_CS_NOTEXIST == rc)
         {
            getContext()->unlockSpaceID();
            continue;
         }
         else if (SDB_OK != rc)
         {
            goto error;
         }
         else
         {
            UINT32 pageSize = 0;
            UINT32 pageCountPerSeg = 0;
            obj->getSU()->getCoreArgs(SPACE_TYPE_RECORD_D, &pageSize, &pageCountPerSeg, NULL);
            UINT32 logicalID = obj->getLogicalID();
            listCollectionSpaceRecord record;
            record.version = obj->getVersion();
            ossStrncpy(record.name, obj->getCSName(), DMS_COLLECTION_SPACE_NAME_SZ);
            record.logicalID = logicalID;
            record.spaceID = obj->getSpaceID();
            record.status = obj->getStatus();
            record.flags = obj->getFlags();
            record.dataPageSize = pageSize;
            record.dataPageCountPerSeg = pageCountPerSeg;
            obj->getSU()->getCoreArgs(SPACE_TYPE_IDX_D, &pageSize, &pageCountPerSeg, NULL);
            record.idxPageSize = pageSize;
            record.idxPageCountPerSeg = pageCountPerSeg;
            slice content(sizeof(record), (const CHAR *)(&record));
            
            getContext()->unlockSpaceID();

            rc = cursor->push(content);
            if (SDB_VESSEL_CURSOR_NO_SPACE == rc)
            {
               rc = SDB_OK;
               break;
            }
            else if (SDB_OK != rc)
            {
               goto error;
            }
            else
            {
               cursor->setLogicalID(logicalID);
               continue;
            }
         }
         
      } while (TRUE);
      
   done:
      getContext()->unlockSpaceID();
      return rc;
   error:
      goto done;
   }
}//namespace vessel
}//namespace engine