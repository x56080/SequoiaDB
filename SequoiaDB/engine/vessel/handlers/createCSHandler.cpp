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

   INT32 createCSHandler::run(const CHAR *name,
                              const createCSOptions &options)
   {
      INT32 rc = SDB_OK;
      SPACE_ID sid = INVALID_SPACE_ID;
      objectContainer *objContainer = NULL;
      extentSUContainer *suContainer = NULL;
      collectionSpace *obj = NULL;
      extentStorageUnit *su = NULL;
      IRedoLogger *logger = NULL;
      objContainer = &(getEnv()->objContainer);
      suContainer = &(getEnv()->suContainer);
      logger = getContext()->getOuterResource()->logger;
      dpsLogRecord lr;
      createSUOptions suOptions;
      static const UINT64 maxFileSize = 0x04ull * 1024 * 1024 * 1024;

      rc = validateOptions(name, options);
      if (SDB_OK != rc)
      {
         goto error;
      }

      if (objContainer->csExists(name, options.logicalID))
      {
         LOG_ERR_AND_REPORT(getContext(), rc, "collection space already exists:%s, %d", name, options.logicalID);
         rc = SDB_DMS_CS_EXIST;
         goto error;
      }

      rc = suContainer->allocateSpaceID(sid);
      if (SDB_DMS_SU_OUTRANGE == rc)
      {
         LOG_ERR_AND_REPORT(getContext(), rc, "the count of collection space has already hit the max value");
         goto error;
      }
      else if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to allocate space id:%d", rc);
         goto error;
      }

      rc = getContext()->lockSpaceID(sid, EXCLUSIVE);
      if (SDB_OK != rc)
      {
         goto error;
      }

      /// cs with same name may be created. whatever, just return error.
      rc = objContainer->allocateCSObj(getContext(), name,
                                       options.logicalID, &obj);
      if (SDB_OK != rc)
      {
         goto error;
      }

      suOptions.sid = sid;
      suOptions.csName.reset(name);
      suOptions.csOptions = &options;

      suOptions.metaArgs.pageSize = DMS_PAGE_SIZE32K;
      suOptions.metaArgs.maxPageCountPerSeg = 32;
      suOptions.metaArgs.maxSegmentCountPerFile = 4096;

      suOptions.dataArgs.pageSize = options.dataPageSize;
      suOptions.dataArgs.maxPageCountPerSeg = options.dataPageCountPerSegment;
      suOptions.dataArgs.maxSegmentCountPerFile = maxFileSize / options.dataPageSize /options.dataPageCountPerSegment;

      suOptions.idxMetaArgs.pageSize = DMS_PAGE_SIZE32K;
      suOptions.idxMetaArgs.maxPageCountPerSeg = 32;
      suOptions.idxMetaArgs.maxSegmentCountPerFile = 4096;

      suOptions.idxArgs.pageSize = options.idxPageSize;
      suOptions.idxArgs.maxPageCountPerSeg = options.idxPageCountPerSegment;
      suOptions.idxArgs.maxSegmentCountPerFile = maxFileSize / options.idxPageSize / options.idxPageCountPerSegment;

      rc = suContainer->createSU(getContext(), suOptions, &su);
      if (SDB_OK != rc)
      {
         goto error;
      }

      rc = obj->setup(getContext(), su);
      if (SDB_OK != rc)
      {
         goto error;
      }

      rc = initCreateCSLogRecord(name, &sid, &options, lr);
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

      getContext()->unlockSpaceID();
   done:
      return rc;
   error:
      if (NULL != obj)
      {
         objContainer->releaseCSObj(getContext());
      }
      if (NULL != su)
      {
         suContainer->dropSU(getContext(), FALSE);
      }
      if (INVALID_SPACE_ID != sid)
      {
         getContext()->unlockSpaceID();
         suContainer->releaseSpaceID(sid);
      }

      goto done;
   }

   INT32 createCSHandler::validateOptions(const CHAR *name, const createCSOptions &options)
   {
      INT32 rc = SDB_OK;
      if (OSS_UNLIKELY(NULL == name))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      rc = dmsCheckCSName(name, options.isSystemCS());
      if (SDB_OK != rc)
      {
         LOG_ERR_AND_REPORT(getContext(), rc, "invalid cs name: %s", name);
         goto error;
      }

      if (DMS_INVALID_LOGICCSID == options.logicalID)
      {
         rc = SDB_INVALIDARG;
         LOG_ERR_AND_REPORT(getContext(), rc, "invalid logical id");
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