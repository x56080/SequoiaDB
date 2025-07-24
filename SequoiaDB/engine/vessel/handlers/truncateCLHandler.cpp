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

   Source File Name = truncateCLHandler.cpp

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

*******************************************************************************/
#include "vessel/truncateCLHandler.h"
#include "utilFullNameParser.hpp"
#include "vessel/instanceEnv.h"
#include "vessel/collectionSpace.h"
#include "vessel/lpsPteWriteBatch.h"

namespace engine
{
namespace vessel
{
   INT32 truncateCLHandler::doit(const globalCollectionId &gcid,
                                 const dmsTruncateCLOptions &o)
   {
      INT32 rc = SDB_OK;
      collectionSpace *cs = nullptr;
      collection *cl = nullptr;
      requestContext context;
      LPS_PTE_WRITE_BATCH batch;

      if (OSS_UNLIKELY(!gcid.isValid()))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      rc = context.getEnv()->dms.getCSByCollectionSpaceId(&context,
                                                          gcid.getCSIdentifier(),
                                                          SHARED,
                                                          &cs);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to get cs obj:%d", rc);
         goto error;
      }

      /// init batch first to avoid deadlock.
      /// commit inside cl.
      rc = cs->getSU()->getIndexSpace().initWriteBatch(batch);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to init write batch:%d", rc);
         goto error;
      }

      rc = cs->getCollectionByLogicalId(&context, gcid.getCLLid(), EXCLUSIVE, &cl);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to get cl obj:%d", rc);
         goto error;
      }

      rc = cl->truncate(&context, batch);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to truncate cl:%d", rc);
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

