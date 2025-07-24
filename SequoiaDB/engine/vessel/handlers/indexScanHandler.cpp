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

   Source File Name = indexScanHandler.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

*******************************************************************************/
#include "vessel/indexScanHandler.h"
#include "vessel/indexScanCursor.h"
#include "vessel/collectionSpace.h"
#include "vessel/collection.h"
#include "vessel/instanceEnv.h"

namespace engine
{
namespace vessel
{
   INT32 indexScanHandler::doit(indexScanCursor *cursor)
   {
      INT32 rc = SDB_OK;
      collectionSpace *cs = nullptr;
      collection *cl = nullptr;
      requestContext context;

      if (OSS_UNLIKELY(nullptr == cursor ||
                       !cursor->isOpen() ||
                       !cursor->getCollectionId().isValid() ||
                       !cursor->getIndexId().isValid()))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      rc = context.getEnv()->dms.getCSByCollectionSpaceId(&context,
                                                   cursor->getCollectionId().getCSIdentifier(),
                                                   SHARED, &cs);
      if (SDB_OK != rc)
      {
         goto error;
      }

      rc = cs->getCollectionById(&context,
                                 cursor->getCollectionId().getCLIdentifier(),
                                 SHARED, &cl);
      if (SDB_OK != rc)
      {
         goto error;
      }

      rc = cl->getMoreWhenIndexScan(&context, cursor);
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
