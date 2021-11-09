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

   Source File Name = indexScanHandler.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#include "vessel/indexScanHandler.h"
#include "vessel/indexScanCursor.h"
#include "vessel/indexScanContext.h"
#include "vessel/collectionSpace.h"
#include "vessel/collection.h"
#include "vessel/instanceEnv.h"
#include "vessel/indexHandle.h"

namespace engine
{
namespace vessel
{
   INT32 indexScanHandler::doit(indexScanCursor *cursor)
   {
      INT32 rc = SDB_OK;
      collectionSpace *cs = NULL;
      collection *cl = NULL;
      indexScanContext context;

      if (OSS_UNLIKELY(NULL == cursor ||
                       !cursor->isOpen()))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      else if (OSS_UNLIKELY(!cursor->getCLHandle().isValid()))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      else if (OSS_UNLIKELY(cursor->getIndexName().empty() &&
                            INVALID_LOGICAL_INDEX_ID == cursor->getIndexId()))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      else if (OSS_UNLIKELY(!isInitialized()))
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }

      context.open(getExecutor(),
                   getEnv(),
                   getOuterResource());

      rc = context.lockSpaceID(cursor->getCLHandle().getSpaceID(), SHARED);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to lock sid[%d], rc:%d",
                cursor->getCLHandle().getSpaceID(), rc);
         goto error;
      }

      rc = getEnv()->dms.getCSByLockedSpaceID(&context,
                                              cursor->getCLHandle().getCSLId(),
                                              &cs);
      if (SDB_OK != rc)
      {
         goto error;
      }

      rc = cs->getCollectionByMBID(&context, cursor->getCLHandle().getMbId(),
                                   cursor->getCLHandle().getCLLId(),
                                   SHARED, &cl);
      if (SDB_OK != rc)
      {
         goto error;
      }

      context.attachIndexScanCursor(cursor);

      rc = cl->getMoreWhenIndexScan(&context);
      if (SDB_OK != rc)
      {
         goto error;
      }
   done:
      context.close();
      return rc;
   error:
      goto done;
   }

} // namespace vessel
  
} // namespace engine
