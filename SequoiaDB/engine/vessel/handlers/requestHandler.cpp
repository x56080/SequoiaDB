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

   Source File Name = requestHandler.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

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