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

   Source File Name = removeIndexHandler.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#include "vessel/removeIndexHandler.h"

namespace engine
{
namespace vessel
{
   INT32 removeIndexHandler::doit(const globalCollectionId &gcid,
                                  const CHAR *indexName)
   {
      INT32 rc = SDB_OK;
      COLLECTION_PTR cl;
      requestContext context;
      strSlice nameSlice(indexName);

      if (OSS_UNLIKELY(!isInitialized()))
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }
      else if (OSS_UNLIKELY(!gcid.isValid() ||
                            nameSlice.empty()))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      context.open(getExecutor(), getEnv());
      rc = requestHandler::getCollectionObject(&context, gcid, SHARED, cl);
      if (SDB_OK != rc)
      {
         goto error;
      }

      rc = cl->removeIndex(&context, nameSlice);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to remove index[%s], rc:%d", indexName, rc);
         goto error;
      }
   done:
      context.close();
      return rc;
   error:
      goto done;
   }
} // namespace vesel

} // namespace engine
