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

   Source File Name = createCLHandler.cpp

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

******************************************************************************/

#include "vessel/createCLHandler.h"
#include "vessel/instanceEnv.h"
#include "vessel/collectionSpace.h"
#include "utilFullNameParser.hpp"

namespace engine
{
namespace vessel
{
   createCLHandler::createCLHandler()
   {

   }

   createCLHandler::~createCLHandler()
   {
      
   }

   INT32 createCLHandler::doit(const CHAR *fullName,
                               const utilCLUniqueID &uniqueId,
                               const dmsCreateCLOptions &o,
                               const bson::BSONObj &adjunct)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(isInitialized(), "can not be null");
      collectionSpace *csObj = NULL;
      requestContext context;
      utilFullNameParser parser;
      const CHAR *clName = NULL;
      strSlice clNameSlice;
      strSlice csName;
      createCLOptions options;

      if (OSS_UNLIKELY(!isInitialized()))
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }

      if (!parser.parse(fullName, &clName))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      clNameSlice.reset(clName);
      if (DMS_COLLECTION_NAME_SZ < clNameSlice.strLen())
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      options.compressionType = o.compressor;
      options.minFreePercent = o.pageMinFreePercent;

      csName.reset(parser.getCSName());
      rc = getEnv()->dms.getCSByName(&context, csName, SHARED, &csObj);
      if (SDB_OK != rc)
      {
         goto error;
      }

      rc = csObj->createCL(&context, clNameSlice,
                           utilGetCLInnerID(uniqueId), options);
      if (SDB_OK != rc)
      {
         goto error;
      }
   done:
      if (NULL != csObj)
      {
         context.unlockSpaceID();
      }
      context.close();
      return rc;
   error:
      goto done;
   }

}//namespace vessel
}//namespace engine