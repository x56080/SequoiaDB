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
#include "vessel/objectContainer.h"
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

   INT32 createCLHandler::doit(UINT32 logicalCSID,
                               const CHAR *clName,
                               UINT32 logicalCLID,
                               const createCLOptions &options)
   {
      INT32 rc = SDB_OK;
      objectContainer *container = &(getContext()->getEnv()->objContainer);
      collectionSpace *csObj = NULL;
      strSlice nameSlice;
      rc = validateOptions(clName, logicalCLID, options);
      if (SDB_OK != rc)
      {
         goto error;
      }

      rc = container->getCSByLogicalID(getContext(), logicalCSID,
                                       SHARED, &csObj);
      if (SDB_DMS_CS_NOTEXIST == rc)
      {
         LOG_ERR_AND_REPORT(getContext(), rc, "collection space[%d] does not exists", logicalCSID);
         goto error;
      }
      else if (SDB_OK != rc)
      {
         goto error;
      }

      nameSlice.reset(clName);
      rc = csObj->createCL(getContext(), nameSlice, logicalCLID, options);
      if (SDB_OK != rc)
      {
         goto error;
      }
   done:
      getContext()->unlockSpaceID();
      return rc;
   error:
      goto done;
   }

   INT32 createCLHandler::validateOptions(const CHAR *name,
                                          UINT32 logicalCLID,
                                          const createCLOptions &options)
   {
      INT32 rc = SDB_OK;
   done:
      return rc;
   error:
      goto done;
   }
}//namespace vessel
}//namespace engine