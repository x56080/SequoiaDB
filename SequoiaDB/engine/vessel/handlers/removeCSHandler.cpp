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

   Source File Name = removeCSHandler.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

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
      collectionSpace *cs = nullptr;

      if (OSS_UNLIKELY(name.empty()))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      rc = context.getEnv()->dms.getCSByName(&context, name, EXCLUSIVE, &cs);
      if (SDB_OK != rc)
      {
         goto error;
      }

      context.getEnv()->ioBufferPool.discard(context.getSpaceID());
      context.getEnv()->lobcBufferPool.discard(context.getSpaceID());
      context.getEnv()->dms.removeCS(&context);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to remove cs[%d], rc:%d", context.getSpaceID(), rc);
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
