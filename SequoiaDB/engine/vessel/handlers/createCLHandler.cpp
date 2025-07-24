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

*******************************************************************************/
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
      collectionSpace *csObj = NULL;
      requestContext context;
      utilFullNameParser parser;
      const CHAR *clName = NULL;
      strSlice clNameSlice;
      strSlice csName;
      createCLOptions options;

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
      rc = context.getEnv()->dms.getCSByName(&context, csName, SHARED, &csObj);
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
      context.close();
      return rc;
   error:
      goto done;
   }

}//namespace vessel
}//namespace engine