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

   Source File Name = removeCLHandler.cpp

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

#include "vessel/removeCLHandler.h"
#include "utilFullNameParser.hpp"
#include "vessel/instanceEnv.h"
#include "vessel/collectionSpace.h"

namespace engine
{
namespace vessel
{
   INT32 removeCLHandler::doit(const globalCollectionId &gcid,
                               const dmsRemoveCLOptions &o)
   {
      INT32 rc = SDB_OK;
      collectionSpace *csObj = NULL;
      requestContext context;

      if (OSS_UNLIKELY(!gcid.isValid()))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      else if (OSS_UNLIKELY(!isInitialized()))
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }

      context.open(getExecutor(), getEnv());
      rc = getEnv()->dms.getCSBySpaceID(&context, gcid.getSpaceId(),
                                        gcid.getCSLid(), SHARED, &csObj);
      if (SDB_OK != rc)
      {
         goto error;
      }

      rc = csObj->removeCL(&context, gcid.getCLIdentifier());
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
} // namespace vessel

} // namespace engine

