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

   Source File Name = hitTransferHandler.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

*******************************************************************************/
#include "vessel/hitTransferHandler.h"
#include "pdTrace.hpp"
#include "ossLikely.hpp"
#include "vessel/hitTransferTaskCtx.h"
#include "vessel/instanceEnv.h"
#include "vessel/collectionSpace.h"
#include "vessel/collection.h"
#include "vessel/indexSpace.h"

namespace engine
{
namespace vessel
{
   INT32 hitTransferHandler::handle(hitTransferTaskCtx *ctx)
   {
      INT32 rc = SDB_OK;
      requestContext context;
      collectionSpace *cs = nullptr;
      collection *cl = nullptr;
      instanceEnv *env = context.getEnv();
      SDB_ASSERT(nullptr != env, "can not be invalid");
      globalIndexID indexId;

      if (OSS_UNLIKELY(nullptr == ctx ||
                       !ctx->isValid()))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      indexId = ctx->getTask().getGlobalIndexID();
      rc = env->dms.getCSByLogicalID(&context, indexId.getLogicalCSID(),
                                     SHARED, &cs);
      if (SDB_DMS_CS_NOTEXIST == rc)
      {
         PD_LOG(PDINFO, "cs[%d] has been removed, ignore it", indexId.getLogicalCSID());
         rc = SDB_OK;
         goto done;
      }
      else if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to get cs[%] obj:%d",
                indexId.getLogicalCSID(), rc);
         goto error;
      }

      rc = cs->getCollectionByLogicalId(&context, indexId.getLogicalCLID(),
                                        SHARED, &cl);
      if (SDB_DMS_NOTEXIST == rc)
      {
         PD_LOG(PDINFO, "cl[%d] has been removed, ignore it", indexId.getLogicalCLID());
         rc = SDB_OK;
         goto done;
      }
      else if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to get cl obj[%d]: rc:%d",
                indexId.getLogicalCLID(), rc);
         goto error;
      }

      rc = cl->transferIndexEntries(&context, ctx);
      if (SDB_IXM_NOTEXIST == rc)
      {
         PD_LOG(PDINFO, "index[%s] has been removed, ingore it", indexId.toString().c_str());
         rc = SDB_OK;
         goto done;
      }
      else if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to transfer index entries:%d", rc);
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
