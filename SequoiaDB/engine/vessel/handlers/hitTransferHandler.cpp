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

   Source File Name = hitTransferHandler.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

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
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to get cs[%] obj:%d",
                indexId.getLogicalCSID(), rc);
         goto error;
      }

      rc = cs->getCollectionByLogicalId(&context, indexId.getLogicalCLID(),
                                        SHARED, &cl);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to get cl obj[%d]: rc:%d",
                indexId.getLogicalCLID(), rc);
         goto error;
      }

      rc = cl->transferIndexEntries(&context, ctx);
      if (SDB_OK != rc)
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
