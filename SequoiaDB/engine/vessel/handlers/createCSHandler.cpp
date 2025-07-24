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

   Source File Name = createCSHandler.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

*******************************************************************************/
#include "vessel/createCSHandler.h"
#include "dms.hpp"
#include "pdTrace.hpp"
#include "vessel/requestContext.h"
#include "vessel/instanceEnv.h"
#include "vessel/collectionSpace.h"
#include "vessel/outerResource.h"
#include "vessel/redoLogUtil.h"

namespace engine
{
namespace vessel
{
   createCSHandler::createCSHandler()
   {}

   createCSHandler::~createCSHandler()
   {

   }

   INT32 createCSHandler::doit(const strSlice &name,
                               utilCSUniqueID uniqueId,
                               const dmsCreateCSOptions &o,
                               const bson::BSONObj &adjunct,
                               collectionSpaceId &identifier)
   {
      INT32 rc = SDB_OK;

      requestContext context;

      if (OSS_UNLIKELY(name.empty()))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      rc = validateOptions(name, o);
      if (SDB_OK != rc)
      {
         goto error;
      }

      rc = context.getEnv()->dms.createCS(&context, name,
                                          uniqueId, o,
                                          identifier);
      if (SDB_OK != rc)
      {
         goto error;
      }
   done:
      context.close();
      return rc;
   error:
      identifier = collectionSpaceId();
      goto done;
   }

   INT32 createCSHandler::validateOptions(const strSlice &name,
                                          const dmsCreateCSOptions &options)
   {
      INT32 rc = SDB_OK;
      if (name.empty() || DMS_COLLECTION_SPACE_NAME_SZ < name.strLen())
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      if (DMS_PAGE_SIZE32K != options.dataPageSize &&
          DMS_PAGE_SIZE64K != options.dataPageSize)
      {
         rc = SDB_INVALIDARG;
         PD_LOG(PDERROR,"invalid page size of data file:%d", options.dataPageSize);
         goto error;
      }

      if (DMS_PAGE_SIZE64K != options.idxPageSize &&
          DMS_PAGE_SIZE32K != options.idxPageSize &&
          DMS_PAGE_SIZE16K != options.idxPageSize &&
          DMS_PAGE_SIZE8K != options.idxPageSize)
      {
         rc = SDB_INVALIDARG;
         PD_LOG(PDERROR, "invalid index pagesize:%d", options.idxPageSize);
         goto error;
      }
   done:
      return rc;
   error:
      goto done;
   }
}//namespace vessel
}//namespace engine