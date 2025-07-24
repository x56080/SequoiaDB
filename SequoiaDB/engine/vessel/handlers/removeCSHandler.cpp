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

   Source File Name = removeCSHandler.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

*******************************************************************************/
#include "vessel/removeCSHandler.h"
#include "pdTrace.hpp"
#include "vessel/instanceEnv.h"
#include "vessel/collectionSpace.h"

namespace engine
{
namespace vessel
{
   INT32 removeCSHandler::doit(const strSlice &name)
   {
      INT32 rc = SDB_OK;
      requestContext context;
      collectionSpaceId id;

      if (OSS_UNLIKELY(name.empty()))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      rc = context.getEnv()->dms.testCS(name, id);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to test cs[%s], rc:%d", name.str(), rc);
         goto error;
      }

      rc = context.lockSpaceID(id.getSpaceId(), EXCLUSIVE);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to lock space id:%d", rc);
         goto error;
      }

      rc = context.getEnv()->dms.removeCS(&context, id);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to remove cs[%s], rc:%d", name.str(), rc);
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
