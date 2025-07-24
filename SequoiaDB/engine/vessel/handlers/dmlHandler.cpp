/*******************************************************************************

   Copyright (C) 2011-Present SequoiaDB Ltd.

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

      http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.

   Source File Name = dmlHandler.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

*******************************************************************************/
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

      if (OSS_UNLIKELY(!gcid.isValid() ||
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

      if (OSS_UNLIKELY(!gcid.isValid() ||
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
      if (OSS_UNLIKELY(!gcid.isValid() ||
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

      if (OSS_UNLIKELY(!gcid.isValid() ||
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