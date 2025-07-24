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

   Source File Name = listCollectionSpaceHandler.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

*******************************************************************************/
#include "vessel/listCollectionSpaceHandler.h"
#include "pdTrace.hpp"
#include "vessel/listCSCursor.h"
#include "vessel/instanceEnv.h"
#include "vessel/collectionSpace.h"
#include "vessel/listCollectionSpaceDef.h"
#include "vessel/slice.h"
#include "vessel/requestContext.h"

namespace engine
{
namespace vessel
{
   INT32 listCollectionSpaceHandler::doit(listCSCursor *cursor)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(nullptr != cursor, "can not be null");
      requestContext context;

      if (OSS_UNLIKELY(nullptr == cursor ||
                       !cursor->isOpen()))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      rc = context.getEnv()->dms.listCollectionSpaces(&context, cursor);
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