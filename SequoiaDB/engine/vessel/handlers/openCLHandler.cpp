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

   Source File Name = openCLHandler.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

*******************************************************************************/
#include "vessel/openCLHandler.h"
#include "vessel/instanceEnv.h"
#include "vessel/collection.h"
#include "vessel/collectionSpace.h"
#include "vessel/requestContext.h"
#include "utilFullNameParser.hpp"

namespace engine
{
namespace vessel
{
   INT32 openCLHandler::doit(const CHAR *fullName,
                             globalCollectionId &id)
   {
      INT32 rc = SDB_OK;
      collectionSpace *cs = nullptr;
      collection *cl = nullptr;
      requestContext context;
      utilFullNameParser parser;
      const CHAR *clName = nullptr;

      id.reset();

      if (!parser.parse(fullName, &clName))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      rc = context.getEnv()->dms.getCSByName(&context, strSlice(parser.getCSName()),
                                             SHARED, &cs);
      if (SDB_OK != rc)
      {
         goto error;
      }

      rc = cs->getCollectionByName(&context, strSlice(clName), SHARED, &cl);
      if (SDB_OK != rc)
      {
         goto error;
      }

      id = cl->getGlobalId();
   done:
      context.close();
      return rc;
   error:
      goto done;
   }

   INT32 openCLHandler::doit(const utilCLUniqueID &uniqueId,
                             globalCollectionId &id)
   {
      INT32 rc = SDB_OK;
      collectionSpace *cs = nullptr;
      collection *cl = nullptr;
      requestContext context;

      id.reset();

      if (!UTIL_IS_VALID_CLUNIQUEID(uniqueId))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      rc = context.getEnv()->dms.getCSByUniqueID(&context, utilGetCSUniqueID(uniqueId), 
                                                 SHARED, &cs);
      if (SDB_OK != rc)
      {
         goto error;
      }

      rc = cs->getCollectionByCLInnerID(&context, utilGetCLInnerID(uniqueId), 
                                        SHARED, &cl);
      if (SDB_OK != rc)
      {
         goto error;
      }
      
      id = cl->getGlobalId();
   done:
      context.close();
      return rc;
   error:
      goto done;
      
   }
}//namespace vessel
}//namespace engine