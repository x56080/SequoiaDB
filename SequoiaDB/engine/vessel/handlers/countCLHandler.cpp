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

   Source File Name = countCLHandler.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#include "vessel/countCLHandler.h"
#include "vessel/collectionSpace.h"
#include "vessel/collection.h"
#include "vessel/requestContext.h"
#include "vessel/instanceEnv.h"
#include "vessel/spaceIDLockHelper.h"

namespace engine
{
namespace vessel
{
   INT32 countCLHandler::doit(const collectionHandle &handle,
                              UINT64 &count)
   {
      INT32 rc = SDB_OK;
      collectionSpace *cs = NULL;
      collection *cl = NULL;
      requestContext context;

      if (OSS_UNLIKELY(!handle.isValid()))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      else if (OSS_UNLIKELY(!isInitialized()))
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
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

      rc = getEnv()->dms.getCSBySpaceID(&context,
                                        handle.getSpaceID(),
                                        handle.getCSLId(),
                                        SHARED, &cs);
      if (SDB_OK != rc)
      {
         goto error;
      }

      rc = cs->getCollectionByMBID(&context, handle.getMbId(),
                                   handle.getCLLId(),
                                   SHARED, &cl);
      if (SDB_OK != rc)
      {
         goto error;
      }

      rc = cl->getTotalCountInRdpHead(&context, count);
      if (SDB_OK != rc)
      {
         goto error;
      }
   done:
      if (NULL != cl)
      {
         context.unlockMB();
      }
      if (NULL != cs)
      {
         context.unlockSpaceID();
      }
      context.close();
      return rc;
   error:
      goto done;
   }
}//namespace vessel
}//namespace engine
