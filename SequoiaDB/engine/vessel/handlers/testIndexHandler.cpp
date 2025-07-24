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

   Source File Name = testIndexHandler.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

*******************************************************************************/
#include "vessel/testIndexHandler.h"

namespace engine
{
namespace vessel
{
   INT32 testIndexHandler::doit(const globalCollectionId &gcid,
                                const strSlice &indexName,
                                indexIdentifier &indexId)
   {
      INT32 rc = SDB_OK;
      requestContext context;
      COLLECTION_PTR cl;
      indexId.reset();

      if (OSS_UNLIKELY(!gcid.isValid() ||
                        indexName.empty()))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      
      rc = requestHandler::getCollectionObject(&context, gcid, SHARED, cl);
      if (SDB_OK != rc)
      {
         goto error;
      }

      rc = cl->testNormalIndex(&context, indexName, indexId);
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
