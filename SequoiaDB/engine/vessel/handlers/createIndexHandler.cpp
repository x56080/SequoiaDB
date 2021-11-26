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

   Source File Name = createIndexHandler.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#include "vessel/createIndexHandler.h"
#include "ossLikely.hpp"
#include "vessel/collectionSpace.h"
#include "vessel/collection.h"
#include "vessel/requestContext.h"
#include "pdTrace.hpp"
#include "vessel/instanceEnv.h"



namespace engine
{
namespace vessel
{
   INT32 createIndexHandler::doit(const globalCollectionId &gcid,
                                  const strSlice &indexName,
                                  const bson::BSONObj &keyPattern,
                                  const indexParameters &params,
                                  const createIndexOptions &options)
   {
      INT32 rc = SDB_OK;
      collectionSpace *cs = NULL;
      collection *cl = NULL;
      requestContext context;
      indexKeyPattern pattern;

      if (OSS_UNLIKELY(!isInitialized()))
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }
      else if (!gcid.isValid())
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      else if (indexName.empty() ||
               keyPattern.isEmpty() ||
               !params.isValid())
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      rc = pattern.set(keyPattern);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to build key pattern from obj[%s], rc%d",
                keyPattern.toString().c_str(), rc);
         goto error;
      }
      
      context.open(getExecutor(), getEnv(), getOuterResource());
      rc = getEnv()->dms.getCSBySpaceID(&context,
                                        gcid.getSpaceId(),
                                        gcid.getCSLid(),
                                        SHARED, &cs);
      if (SDB_OK != rc)
      {
         goto error;
      }

      rc = cs->getCollectionByMBID(&context, gcid.getMbId(),
                                   gcid.getCLLid(), SHARED, &cl);
      if (SDB_OK != rc)
      {
         goto error;
      }

      rc = cl->createIndex(&context, indexName, pattern, params, options);
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
