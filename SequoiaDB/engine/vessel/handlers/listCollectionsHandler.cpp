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
#include "vessel/extentStorageUnit.h"

namespace engine
{
namespace vessel
{
   INT32 listCollectionsHandler::doit(listCLCursor *cursor)
   {
      INT32 rc = SDB_OK;
      SPACE_ID sid = INVALID_SPACE_ID;
      collectionSpace *obj = NULL;
      SDB_ASSERT(NULL != cursor, "can not be null");
      objectContainer *container = NULL;

      if (OSS_UNLIKELY(NULL == cursor || !cursor->isOpen()))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      container = &(getEnv()->objContainer);
      rc = container->getCSByLogicalID(getContext(),
                                       cursor->getCSLogicalID(),
                                       SHARED,
                                       &obj);
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
      getContext()->unlockSpaceID();
      return rc;
   error:
      goto done;
   }
}//namespace vessel
}//namespace engine