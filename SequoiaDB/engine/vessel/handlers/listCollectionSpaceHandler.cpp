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

   Source File Name = listCollectionSpaceHandler.cpp

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

#include "vessel/listCollectionSpaceHandler.h"
#include "pdTrace.hpp"
#include "vessel/listCSCursor.h"
#include "vessel/instanceEnv.h"
#include "vessel/collectionSpace.h"
#include "vessel/listCollectionSpaceDef.h"
#include "vessel/slice.h"

namespace engine
{
namespace vessel
{
   INT32 listCollectionSpaceHandler::doit(listCSCursor *cursor)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(isInitialized(), "can not be null");
      SDB_ASSERT(NULL != cursor, "can not be null");
      requestContext context;

      if (OSS_UNLIKELY(NULL == cursor ||
                       !cursor->isOpen()))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      else if (OSS_UNLIKELY(!isInitialized()))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      rc = context.open(getSession(), getEnv(), getOuterResource());
      if (OSS_UNLIKELY(SDB_OK != rc))
      {
         goto error;
      }

      rc = getEnv()->csContainer.listCollectionSpaces(&context, cursor);
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