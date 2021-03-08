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

#include "vessel/createCSHandler.h"
#include "dms.hpp"
#include "pdTrace.hpp"
#include "vessel/ISession.h"
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

   INT32 createCSHandler::doit(const CHAR *name,
                               utilCSUniqueID uniqueID,
                               const createCSOptions &options)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(isInitialized(), "can not be null");
      SPACE_ID sid = INVALID_SPACE_ID;
      IRedoLogger *logger = getContext()->getOuterResource()->logger;
      CS_CONTAINER &cc = getContext()->getEnv()->csContainer;
      dpsLogRecord lr;
      strSlice nameSlice;

      if (OSS_UNLIKELY(NULL == name))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      nameSlice.reset(name);
      rc = validateOptions(nameSlice, options);
      if (SDB_OK != rc)
      {
         goto error;
      }

      rc = cc.createCS(getContext(), nameSlice, uniqueID, options);
      if (SDB_OK != rc)
      {
         goto error;
      }

      rc = initCreateCSLogRecord(name, &sid, &uniqueID, &options, lr);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to build log:%d", rc);
         goto error;
      }

      rc = logger->log(getContext()->getSession(), &lr, NULL);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to write log:%d", rc);
         goto error;
      }
   done:
      return rc;
   error:
      goto done;
   }

   INT32 createCSHandler::validateOptions(const strSlice &name, const createCSOptions &options)
   {
      INT32 rc = SDB_OK;
      if (OSS_UNLIKELY(name.empty()))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      rc = dmsCheckCSName(name.str(), options.isSystemCS());
      if (SDB_OK != rc)
      {
         LOG_ERR_AND_REPORT(getContext(), rc, "invalid cs name: %s", name.str());
         goto error;
      }

      if (DMS_PAGE_SIZE32K != options.dataPageSize &&
          DMS_PAGE_SIZE64K != options.dataPageSize)
      {
         rc = SDB_INVALIDARG;
         LOG_ERR_AND_REPORT(getContext(), rc, "invalid page size of data file:%d", options.dataPageSize);
         goto error;
      }

      if (0 == options.dataPageCountPerSegment)
      {
         rc = SDB_INVALIDARG;
         LOG_ERR_AND_REPORT(getContext(), rc, "invalid dataPageCountPerSegment of data file:%d", options.dataPageCountPerSegment);
         goto error;
      }

      if (DMS_PAGE_SIZE64K != options.idxPageSize &&
          DMS_PAGE_SIZE32K != options.idxPageSize &&
          DMS_PAGE_SIZE16K != options.idxPageSize &&
          DMS_PAGE_SIZE8K != options.idxPageSize)
      {
         rc = SDB_INVALIDARG;
         LOG_ERR_AND_REPORT(getContext(), rc, "invalid index pagesize:%d", options.idxPageSize);
         goto error;
      }

      if (0 == options.idxPageCountPerSegment)
      {
         rc = SDB_INVALIDARG;
         LOG_ERR_AND_REPORT(getContext(), rc, "invalid idxPageCountPerSegment of data file:%d", options.idxPageCountPerSegment);
         goto error;
      }
   done:
      return rc;
   error:
      goto done;
   }
}//namespace vessel
}//namespace engine