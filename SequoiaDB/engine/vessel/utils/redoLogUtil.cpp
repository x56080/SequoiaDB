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

   Source File Name = redoLogUtil.cpp

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

#include "vessel/redoLogUtil.h"
#include "dpsLogRecord.hpp"
#include "ossLikely.hpp"
#include "dpsLogRecordDef.hpp"
#include "vessel/vesselOptions.h"
#include "vessel/collectionRecordPage.h"
#include "vessel/logRecordContext.h"
#include "vessel/IRedoLogger.h"
#include "vessel/strSlice.h"
#include "vessel/requestContext.h"
#include "vessel/outerResource.h"
#include "vessel/atomicOperationList.h"

namespace engine
{
namespace vessel
{
   /*
   INT32 initCreateCSLogRecord(const CHAR *name,
                               const SPACE_ID *sid,
                               const utilCSUniqueID *uniqueID,
                               const createCSOptions *options,
                               dpsLogRecord &lr)
   {
      INT32 rc = SDB_OK;
      dpsLogRecordHeader *head = NULL;
      if (OSS_UNLIKELY(NULL == name || NULL == options))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      lr.clear();
      head = &(lr.head());
      head->_type = LOG_TYPE_CS_CRT;
      OSS_BIT_SET(head->_flags, DPS_VESSEL_LOG_FLAG_FROM_VESSEL);
      rc = lr.push(DPS_LOG_CSCRT_CSNAME, ossStrlen(name)+1, name);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to push csname to log record:%d", rc);
         goto error;
      }

      rc = lr.push(DPS_LOG_CSCRT_CSUNIQUEID, sizeof(utilCSUniqueID), (const CHAR *)uniqueID);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to push unique id to log record:%d", rc);
         goto error;
      }

      rc = lr.push(DPS_LOG_CSCRT_VESSEL_CSCRT_OPTIONS, sizeof(createCSOptions), (const CHAR *)options);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to push crt options to log record:%d", rc);
         goto error;
      }

      rc = lr.push(DPS_LOG_CSCRT_VESSEL_SPACE_ID, sizeof(SPACE_ID), (const CHAR *)sid);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to push sid to log record:%d", rc);
         goto error;
      }

      head->_length = lr.alignedLen();

   done:
      return rc;
   error:
      goto done;
   }*/

   INT32 pushFullNameElement(IRedoLogger *logger,
                             ISession *session,
                             logRecordContext *lrc,
                             const strSlice &csName,
                             const strSlice &clName)
   {
      INT32 rc = SDB_OK;
      UINT32 len = csName.strLen() + clName.strLen() + 2;
      CHAR fullName[DMS_COLLECTION_NAME_SZ + DMS_COLLECTION_SPACE_NAME_SZ + 2] = {0};
      if (OSS_UNLIKELY(NULL == lrc || NULL == logger || NULL == session))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      else if (OSS_UNLIKELY(sizeof(fullName) < len))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      ossMemcpy(fullName, csName.str(), csName.strLen());
      fullName[csName.strLen()] = '.';
      ossMemcpy(fullName + csName.strLen() + 1, clName.str(), clName.strLen());
      rc = logger->pushLogRecordElement(session, lrc, DPS_LOG_PUBLIC_FULLNAME,
                                        len, fullName);
      if (SDB_OK != rc)
      {
         goto error;
      }
   done:
      return rc;
   error:
      goto done;
   }

   UINT32 packSidAndType(SPACE_ID sid,
                            SPACE_TYPE spaceType,
                              FILE_TYPE fileType)
   {
      UINT32 value = ((UINT32)fileType << 24);
      value |= ((UINT32)spaceType << 16);
      value |= (UINT32)sid;
      return value;
   }

   void unpackSidAndType(UINT32 value,
                         SPACE_ID &sid,
                         SPACE_TYPE &spaceType,
                         FILE_TYPE &fileType)
   {
      sid = value;
      spaceType = (value >> 16);
      fileType = (value >> 24);
      return;
   }

/////////////logicalPageSapceLogUtil begin
   INT32 lpsLogUtil::prepare(requestContext *context,
                              const deltaLogRecord &dlr,
                              logRecordContext &lrc)
   {
      INT32 rc = SDB_OK;
      ISession *session = NULL;
      IRedoLogger *logger = NULL;

      if (OSS_UNLIKELY(NULL == context ||
                       !dlr.isValid() ||
                       lrc.prepared()))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      session = context->getSession();
      logger = context->getOuterResource()->logger;

      lrc.open(LOG_TYPE_VESSEL_LPS_PAGE_MANAGEMENT);

      if (context->isInProcessingOplist())
      {
         atomicOperationList *oplist = context->getOplist();
         if (oplist->isWatingHead())
         {
            lrc.setOplistHead();
         }
         else
         {
            lrc.setOplist(oplist->getOplistLsn());
         }

         if (oplist->isWaitingTail())
         {
            lrc.setOplistTail();
         }
      }

      lrc.prepush(sizeof(UINT32));
      lrc.prepush(dlr.getLogHead()->_size);
      lrc.prepushDone();

      rc = logger->prepare(session, &lrc);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed prepare log record:%d", rc);
         goto error;
      }
   done:
      return rc;
   error:
      goto done;
   }

   INT32 lpsLogUtil::commit(requestContext *context,
                            logRecordContext &lrc,
                            SPACE_ID sid,
                            SPACE_TYPE spaceType,
                            FILE_TYPE fileType,
                            const deltaLogRecord &dlr)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(NULL != context, "can not be null");
      SDB_ASSERT(lrc.prepared(), "must be prepared");
      SDB_ASSERT(INVALID_SPACE_ID != sid, "can not be invalid");
      SDB_ASSERT(INVALID_SPACE_TYPE != spaceType, "can not be invalid");
      SDB_ASSERT(INVALID_FILE_TYPE != fileType, "can not be invalid");
      SDB_ASSERT(dlr.isValid(), "can not be invalid");

      ISession *session = NULL;
      IRedoLogger *logger = NULL;
      UINT32 packedSidAndType = packSidAndType(sid, spaceType, fileType);
      session = context->getSession();
      logger = context->getOuterResource()->logger;

      rc = logger->pushLogRecordElement(session, &lrc,
                                        DPS_LOG_VESSEL_LPS_PM_SID_AND_TYPE,
                                        sizeof(UINT32), &packedSidAndType);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to push packed sid:%d", rc);
         goto error;
      }

      rc = logger->pushLogRecordElement(session, &lrc,
                                        DPS_LOG_VESSEL_LPS_PM_DELTA_LOG,
                                        dlr.getLogHead()->_size,
                                        dlr.getLogHead());
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to push delta log:%d", rc);
         goto error;
      }

      rc = logger->commit(session, &lrc);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to commit log[%lld]:%d", lrc.getLsn(), rc);
         goto error;
      }

      if (context->isInProcessingOplist())
      {
         context->getOplist()->push(lrc.getLsn());
      }
      
   done:
      return rc;
   error:
      goto done;
   }

   INT32 lpsLogUtil::abort(requestContext *context,
                           logRecordContext &lrc)
   {
      INT32 rc = SDB_OK;
      ISession *session = NULL;
      IRedoLogger *logger = NULL;

      if (OSS_UNLIKELY(NULL == context ||
                       !lrc.prepared()))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      session = context->getSession();
      logger = context->getOuterResource()->logger;
      rc = logger->abort(session, &lrc);
      if (SDB_OK != rc)
      {
         goto error;
      }
   done:
      return rc;
   error:
      goto done;
   }
/////////////logicalPageSapceLogUtil end
}//namespace vessel
}//namespace engine