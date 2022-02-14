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

   Source File Name = openCLHandler.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

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
      SDB_ASSERT(isInitialized(), "must be inited");
      collectionSpace *cs = NULL;
      collection *cl = NULL;
      requestContext context;
      utilFullNameParser parser;
      const CHAR *clName = NULL;

      id.reset();

      if (OSS_UNLIKELY(!isInitialized()))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      if (!parser.parse(fullName, &clName))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      context.open(getExecutor(), getEnv());

      rc = getEnv()->dms.getCSByName(&context, strSlice(parser.getCSName()),
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
      collectionSpace *cs = NULL;
      collection *cl = NULL;
      requestContext context;

      id.reset();
      if (OSS_UNLIKELY(!isInitialized()))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      if (!UTIL_IS_VALID_CLUNIQUEID(uniqueId))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      context.open(getExecutor(), getEnv());

      rc = getEnv()->dms.getCSByUniqueID(&context, utilGetCSUniqueID(uniqueId), 
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