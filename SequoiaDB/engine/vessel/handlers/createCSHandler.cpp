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

   Source File Name = createCSHandler.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#include "vessel/createCSHandler.h"
#include "dms.hpp"
#include "pdTrace.hpp"
#include "vessel/requestContext.h"
#include "vessel/instanceEnv.h"
#include "vessel/collectionSpace.h"
#include "vessel/IRedoLogger.h"
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
      SDB_ASSERT(isInitialized(), "can not be null");

      requestContext context;
      createCSOptions options;

      if (OSS_UNLIKELY(name.empty()))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      else if (OSS_UNLIKELY(!isInitialized()))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      options.init(o);
      rc = validateOptions(name, options);
      if (SDB_OK != rc)
      {
         goto error;
      }

      context.open(getExecutor(), getEnv());
      rc = getEnv()->dms.createCS(&context, name,
                                  uniqueId, options,
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

   INT32 createCSHandler::validateOptions(const strSlice &name, const createCSOptions &options)
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

      if (!isValidSegmentSize(options.dataSegSize))
      {
         PD_LOG(PDERROR, "invalid data segment size:%d", options.dataSegSize);
         rc = SDB_INVALIDARG;
         goto error;
      }

      if (!isValidSegmentSize(options.idxSegSize))
      {
         PD_LOG(PDERROR, "invalid index segment size:%d", options.idxSegSize);
         rc = SDB_INVALIDARG;
         goto error;
      }
   done:
      return rc;
   error:
      goto done;
   }
}//namespace vessel
}//namespace engine