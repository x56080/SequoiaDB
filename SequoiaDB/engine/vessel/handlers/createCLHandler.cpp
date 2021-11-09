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

   INT32 createCLHandler::doit(const strSlice &csName,
                               const strSlice &clName,
                               utilCLInnerID innerID,
                               const createCLOptions &options)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(isInitialized(), "can not be null");
      collectionSpace *csObj = NULL;
      requestContext context;

      rc = validateOptions(csName, clName, options);
      if (SDB_OK != rc)
      {
         goto error;
      }

      if (OSS_UNLIKELY(!isInitialized()))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      context.open(getExecutor(), getEnv(), getOuterResource());

      rc = getEnv()->dms.getCSByName(&context, csName, SHARED, &csObj);
      if (SDB_OK != rc)
      {
         goto error;
      }

      rc = csObj->createCL(&context, clName, innerID, options);
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

   INT32 createCLHandler::validateOptions(const strSlice &csName,
                                          const strSlice &clName,
                                          const createCLOptions &options)
   {
      INT32 rc = SDB_OK;
      if (csName.empty())
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      else if (clName.empty())
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      else if (DMS_COLLECTION_NAME_SZ < clName.strLen())
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      else if (!options.isValid())
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
   done:
      return rc;
   error:
      goto done;
   }
}//namespace vessel
}//namespace engine