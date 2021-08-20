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

   Source File Name = scanCLHandler.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#include "vessel/scanCLHandler.h"
#include "vessel/scanCLCursor.h"
#include "vessel/collectionSpace.h"
#include "vessel/collection.h"
#include "vessel/instanceEnv.h"
#include "vessel/scanCLCursor.h"
#include "vessel/spaceIDLockHelper.h"

namespace engine
{
namespace vessel
{
   INT32 scanCLHandler::doit(scanCLCursor *cursor)
   {
      INT32 rc = SDB_OK;
      collectionSpace *cs = NULL;
      collection *cl = NULL;
      requestContext context;
      spaceIDLockHelper lh(&context);
      const collectionHandle *handle = NULL;

      if (OSS_UNLIKELY(NULL == cursor ||
                       !cursor->isOpen()))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      else if (OSS_UNLIKELY(!cursor->getHandle().isValid()))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      else if (OSS_UNLIKELY(!isInitialized()))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      rc = context.open(getSession(),
                        getEnv(),
                        getOuterResource());
      if (OSS_UNLIKELY(SDB_OK != rc))
      {
         PD_LOG(PDERROR, "failed to open context:%d", rc);
         goto error;
      }

      handle = &(cursor->getHandle());

      rc = lh.lock(handle->getSpaceID(), SHARED);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to lock space id[%d], rc:%d", handle->getSpaceID(), rc);
         goto error;
      }

      rc = getEnv()->dms.getCSByLockedSpaceID(&context,
                                                      handle->getCSLId(),
                                                      &cs);
      if (SDB_OK != rc)
      {
         goto error;
      }

      rc = cs->getCollectionByMBID(&context, cursor->getHandle().getMbId(),
                                   cursor->getHandle().getCLLId(),
                                   SHARED, &cl);
      if (SDB_OK != rc)
      {
         goto error;
      }

      rc = cl->getMoreWhenScan(&context, cursor);
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
}//namespace vessel
}//namespace engine
