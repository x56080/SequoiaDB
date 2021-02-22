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

   Source File Name = insertHandler.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#include "vessel/insertHandler.h"
#include "vessel/collectionHandle.h"
#include "vessel/instanceEnv.h"
#include "vessel/collection.h"
#include "vessel/collectionSpace.h"

namespace engine
{
namespace vessel
{
   INT32 insertHandler::doit(const collectionHandle *handle,
                             const slice &record,
                             const insertOptions &options)
   {
      INT32 rc = SDB_OK;
      objectContainer *container = NULL;
      collectionSpace *cs = NULL;
      collection *cl = NULL;
      BOOLEAN sidLocked = FALSE;

      if (OSS_UNLIKELY(!initialized()))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      else if (OSS_UNLIKELY(NULL == handle ||
                            !handle->valid() ||
                            !record.valid()))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      SDB_ASSERT(!getContext()->getSpaceIDLocked(), "impossible");
      SDB_ASSERT(!getContext()->mbLocked(), "impossible");

      container = &(getContext()->getEnv()->objContainer);
      if (handle->getSpaceID() == INVALID_SPACE_ID)
      {
         rc = container->getCSByLogicalID(getContext(), handle->getLogicalCSId(),
                                          SHARED, &cs);
         if (SDB_OK != rc)
         {
            goto error;
         }
      }
      else
      {
         rc = getContext()->lockSpaceID(handle->getSpaceID(), SHARED);
         if (SDB_OK != rc)
         {
            goto error;
         }
         rc = container->getCSByLockedSpaceID(getContext(), handle->getLogicalCSId(), &cs);
         if (SDB_OK != rc)
         {
            goto error;
         }
      }
      sidLocked = TRUE;

      if (handle->getMBID() == INVALID_CL_MB_ID)
      {
         rc = cs->getCollectionByLogicalID(getContext(), handle->getLogicalCLId(), SHARED, &cl);
         if (SDB_OK != rc)
         {
            goto error;
         }
      }
      else
      {
         rc = cs->getCollectionByMBID(getContext(), handle->getMBID(), handle->getLogicalCLId(), SHARED, &cl);
         if (SDB_OK != rc)
         {
            goto error;
         }
      }
      
   done:
      if (NULL != cl)
      {
         getContext()->unlockMB();
      }
      if (sidLocked)
      {
         getContext()->unlockSpaceID();
      }
      return rc;
   error:
      goto done;
   }
}//namespace vessel
}//namespace engine