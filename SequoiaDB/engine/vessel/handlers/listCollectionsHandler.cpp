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

   Source File Name = listCollectionsHandler.cpp

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

#include "vessel/listCollectionsHandler.h"
#include "pdTrace.hpp"
#include "vessel/listCLCursor.h"
#include "vessel/instanceEnv.h"
#include "vessel/collectionSpace.h"
#include "vessel/slice.h"

namespace engine
{
namespace vessel
{
   INT32 listCollectionsHandler::doit(listCLCursor *cursor)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(isInitialized(), "can not be null");
      collectionSpace *obj = NULL;
      SDB_ASSERT(NULL != cursor, "can not be null");
      CS_CONTAINER &cc = getEnv()->csContainer;
      SPACE_ID sid = INVALID_SPACE_ID;
      UINT32 logicalID = DMS_INVALID_LOGICCSID;
      BOOLEAN locked = FALSE;

      if (OSS_UNLIKELY(NULL == cursor || !cursor->isOpen()))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      sid = cursor->getSpaceID();
      logicalID = cursor->getCSLogicalID();

      rc = getContext()->lockSpaceID(sid, SHARED);
      if (SDB_OK != rc)
      {
         goto error;
      }
      locked = TRUE;

      rc = cc.getCSByLockedSpaceID(getContext(), logicalID, &obj);
      if (SDB_OK != rc)
      {
         goto error;
      }

      rc = obj->listCollections(getContext(), cursor);
      if (SDB_OK != rc)
      {
         goto error;
      }
      
   done:
      if (locked)
      {
         getContext()->unlockSpaceID();
      }
      return rc;
   error:
      goto done;
   }
}//namespace vessel
}//namespace engine