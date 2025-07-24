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

   Source File Name = requestHandler.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

*******************************************************************************/
#include "vessel/requestHandler.h"
#include "ossLikely.hpp"
#include "vessel/requestContext.h"
#include "vessel/instanceEnv.h"
#include "vessel/outerResource.h"
#include "vessel/collectionSpace.h"

namespace engine
{
namespace vessel
{
   INT32 requestHandler::getCollectionObject(requestContext *context,
                                             const globalCollectionId &gcid,
                                             OSS_LATCH_MODE mode,
                                             COLLECTION_PTR &out)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(nullptr != context, "can not be null");
      SDB_ASSERT(gcid.isValid(), "can not be invalid");

      instanceEnv *env = context->getEnv();
      SDB_ASSERT(nullptr != env, "can not be null");
      collectionSpace *cs = nullptr;
      collection *cl = nullptr;
      out.reset();

      rc = env->dms.getCSByCollectionSpaceId(context, gcid.getCSIdentifier(), SHARED, &cs);
      if (SDB_OK != rc)
      {
         goto error;
      }

      rc = cs->getCollectionById(context, gcid.getCLIdentifier(), mode, &cl);
      if (SDB_OK != rc)
      {
         goto error;
      }
      
      out = COLLECTION_PTR(cl);
   done:
      return rc;
   error:
      if (nullptr != cs)
      {
         context->close();
      }
      goto done;
   }
}//namespace vessel
}//namespace engine