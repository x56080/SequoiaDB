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

   Source File Name = dmlHandler.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#include "vessel/dmlHandler.h"
#include "vessel/instanceEnv.h"
#include "vessel/collection.h"
#include "vessel/collectionSpace.h"
#include "vessel/spaceIDLockHelper.h"
#include "vessel/dmlContext.h"

namespace engine
{
namespace vessel
{
   INT32 dmlHandler::insert(const globalCollectionId &gcid,
                            const dmlInsertRequest &request,
                            utilInsertResult *res)
   {
      INT32 rc = SDB_OK;
      COLLECTION_PTR cl;
      dmlContext context;

      if (OSS_UNLIKELY(!isInitialized()))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      else if (OSS_UNLIKELY(!gcid.isValid() ||
                            !request.isValid()))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      rc = getCollectionObject(&context, gcid, SHARED, cl);
      if (SDB_OK != rc)
      {
         goto error;
      }

      rc = cl->insert(&context, request, res);
      if (SDB_IXM_DUP_KEY == rc)
      {
         goto error;
      }
      else if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to insert record into collection:%d", rc);
         goto error;
      }
   done:
      context.close();
      return rc;
   error:
      goto done;
   }

   INT32 dmlHandler::insertBatch(const globalCollectionId &gcid,
                                 const dmlBatchInsertRequest &request,
                                 utilInsertResult *res)
   {
      INT32 rc = SDB_OK;
      COLLECTION_PTR cl;
      dmlContext context;

      if (OSS_UNLIKELY(!isInitialized()))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      else if (OSS_UNLIKELY(!gcid.isValid() ||
                            !request.isValid()))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      for (UINT32 i = 0; i < request.batch.size(); ++i)
      {
         if (!request.batch[i].isValid())
         {
            rc = SDB_INVALIDARG;
            goto error;
         }
      }

      
      rc = getCollectionObject(&context, gcid, SHARED, cl);
      if (SDB_OK != rc)
      {
         goto error;
      }

      for (UINT32 i = 0; i < request.batch.size(); ++i)
      {
         dmlInsertRequest req;
         req.o = request.o;
         req.record = request.batch[i];

         rc = cl->insert(&context, req, res);
         if (SDB_OK != rc)
         {
            if (SDB_IXM_DUP_KEY != rc)
            {
               PD_LOG(PDERROR, "failed to insert record to collection[%s], rc:%d",
                      cl->getName(), rc);
            }
            goto error;
         }
      }
   done:
      context.close();
      return rc;
   error:
      goto done;
   }

   INT32 dmlHandler::update(const globalCollectionId &gcid,
                            const dmlUpdateRequest &request,
                            IRecordUpdater *updater,
                            utilUpdateResult *res)
   {
      INT32 rc = SDB_OK;
      dmlContext context;
      COLLECTION_PTR cl;
      if (OSS_UNLIKELY(!isInitialized()))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      else if (OSS_UNLIKELY(!gcid.isValid() ||
                            !request.isValid() ||
                            NULL == updater))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      
      rc = getCollectionObject(&context, gcid, SHARED, cl);
      if (SDB_OK != rc)
      {
         goto error;
      }

      rc = cl->update(&context, request, updater, res);
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

   INT32 dmlHandler::remove(const globalCollectionId &gcid,
                            const dmlRemoveRequest &request,
                            utilDeleteResult *res)
   {
      INT32 rc = SDB_OK;
      dmlContext context;
      COLLECTION_PTR cl;

      if (OSS_UNLIKELY(!isInitialized()))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      else if (OSS_UNLIKELY(!gcid.isValid() ||
                            !request.isValid()))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      
      rc = getCollectionObject(&context, gcid, SHARED, cl);
      if (SDB_OK != rc)
      {
         goto error;
      }

      rc = cl->remove(&context, request, res);
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