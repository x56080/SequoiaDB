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

   INT32 commitCreateIndexLog(requestContext *context,
                              const strSlice &fullName,
                              UINT32 indexId,
                              INT32 indexSlot,
                              const slice &indexDef)
   {
      INT32 rc = SDB_OK;
      ISession *session = NULL;
      IRedoLogger *logger = NULL;
      logRecordContext lrc;

      if (NULL == context ||
          fullName.empty() ||
          !indexDef.isValid())
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      session = context->getSession();
      logger = context->getOuterResource()->logger;

      lrc.open(LOG_TYPE_IX_CRT);
      lrc.setDDL();
      lrc.prepush(fullName.strLen() + 1);
      lrc.prepush(sizeof(INT32));
      lrc.prepush(sizeof(UINT32));
      lrc.prepush(indexDef.getSize());
      lrc.prepushDone();

      rc = logger->prepare(session, &lrc);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to prepare dps log:%d", rc);
         goto error;
      }

      rc = logger->pushLogRecordElement(session, &lrc,
                                        DPS_LOG_PUBLIC_FULLNAME,
                                        fullName.strLen() + 1,
                                        fullName.str());
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to push element fullname:%d", rc);
         goto error;
      }

      rc = logger->pushLogRecordElement(session, &lrc,
                                        DPS_LOG_IXCRT_IX_SLOT,
                                        sizeof(INT32),
                                        &indexSlot);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to push element index slot:%d", rc);
         goto error;
      }

      rc = logger->pushLogRecordElement(session, &lrc,
                                        DPS_LOG_IXCRT_IX_INDEX_ID,
                                        sizeof(UINT32),
                                        &indexId);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to push element index slot:%d", rc);
         goto error;
      }

      rc = logger->pushLogRecordElement(session, &lrc,
                                        DPS_LOG_IXCRT_IX_DEF_OBJ,
                                        indexDef.getSize(),
                                        indexDef.getRPtr());
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to push element index def:%d", rc);
         goto error;
      }

      rc = logger->commit(session, &lrc);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to commit log[%lld], rc:%d", lrc.getLsn(), rc);
         goto error;
      }

   done:
      return rc;
   error:
      if (lrc.prepared())
      {
         logger->abort(session, &lrc);
      }
      goto done;
   }

   INT32 commitCreateIndexEndLog(requestContext *context,
                                 const strSlice &fullName,
                                 const strSlice &indexName,
                                 UINT32 indexId,
                                 INT32 indexSlot,
                                 INT32 result)
   {
      INT32 rc = SDB_OK;
      ISession *session = NULL;
      IRedoLogger *logger = NULL;
      logRecordContext lrc;

      if (NULL == context ||
          fullName.empty() ||
          indexName.empty())
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      session = context->getSession();
      logger = context->getOuterResource()->logger;

      lrc.open(LOG_TYPE_IX_CRT_END);
      lrc.setDDL();

      lrc.prepush(fullName.strLen() + 1);
      lrc.prepush(sizeof(INT32));
      lrc.prepush(sizeof(UINT32));
      lrc.prepush(indexName.strLen() + 1);
      lrc.prepush(sizeof(INT32));
      lrc.prepushDone();

      rc = logger->prepare(session, &lrc);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to prepare dps log:%d", rc);
         goto error;
      }

      rc = logger->pushLogRecordElement(session, &lrc,
                                        DPS_LOG_PUBLIC_FULLNAME,
                                        fullName.strLen() + 1,
                                        fullName.str());
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to push element fullname:%d", rc);
         goto error;
      }

      rc = logger->pushLogRecordElement(session, &lrc,
                                        DPS_LOG_IXCRT_END_IX_SLOT,
                                        sizeof(INT32),
                                        &indexSlot);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to push element index slot:%d", rc);
         goto error;
      }

      rc = logger->pushLogRecordElement(session, &lrc,
                                        DPS_LOG_IXCRT_END_IX_INDEX_ID,
                                        sizeof(UINT32),
                                        &indexId);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to push element index slot:%d", rc);
         goto error;
      }

      rc = logger->pushLogRecordElement(session, &lrc,
                                        DPS_LOG_IXCRT_END_IX_NAME,
                                        indexName.strLen() + 1,
                                        indexName.str());
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to push element indexname:%d", rc);
         goto error;
      }

      rc = logger->pushLogRecordElement(session, &lrc,
                                        DPS_LOG_IXCRT_END_RC,
                                        sizeof(INT32),
                                        &result);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to push element index slot:%d", rc);
         goto error;
      }

      rc = logger->commit(session, &lrc);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to commit log[%lld], rc:%d", lrc.getLsn(), rc);
         goto error;
      }
   done:
      return rc;
   error:
      if (lrc.prepared())
      {
         logger->abort(session, &lrc);
      }
      goto done;
   }

   BOOLEAN buildFullName(UINT32 bufferSize,
                         CHAR *buffer,
                         const strSlice &csName,
                         const strSlice &clName)
   {
      SDB_ASSERT(NULL != buffer, "can not be null");
      SDB_ASSERT(!csName.empty(), "can not be empty");
      SDB_ASSERT(!clName.empty(), "can not be empty");
      BOOLEAN r = FALSE;
      UINT32 nameSize = csName.strLen() + clName.strLen() + 2;
      if (nameSize <= bufferSize)
      {
         ossMemcpy(buffer, csName.str(), csName.strLen());
         buffer[csName.strLen()] = '.';
         ossMemcpy(buffer + csName.strLen() + 1, clName.str(), clName.strLen());
         buffer[nameSize] = '\0';
         r = TRUE;
      }
      return r;
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