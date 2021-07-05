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

   Source File Name = csgpAccessor.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#include "vessel/csgpAccessor.h"
#include "vessel/collectionSpaceGlobalPage.h"
#include "pdTrace.hpp"
#include "vessel/IRedoLogger.h"
#include "vessel/logRecordContext.h"
#include "dpsLogRecordDef.hpp"
#include "vessel/outerResource.h"

namespace engine
{
namespace vessel
{
   static const UINT32 UPDATE_CS_META_TYPE_LID = 0;

   csgpAccessor::csgpAccessor()
   {}

   csgpAccessor::~csgpAccessor()
   {}

   INT32 csgpAccessor::create(requestContext *context,
                              const csMetaRecord &cmr,
                              const slice &adjuncts)
   {
      INT32 rc = SDB_OK;
      logRecordContext lrc;
      csMetaRecord *record = NULL;

      if (OSS_UNLIKELY(!isOpen()))
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }
      else if (OSS_UNLIKELY(NULL == context ||
                            !cmr.isValid()))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      SDB_ASSERT(getRuntimeBuffer()->isCacheBuffer(), "impossible");
      rc = getRuntimeBuffer()->prepareToWrite(context);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to prepare to write:%d", rc);
         goto error;
      }

      rc = prepareUpdateLog(context, &lrc, LOG_TYPE_CS_CRT, FALSE, adjuncts);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to prepare log:%d", rc);
         goto error;
      }

      record = getRuntimeBuffer()->getWritablePtrOfBody<csMetaRecord>(0);
      if (NULL == record)
      {
         PD_LOG(PDERROR, "failed to get record writable ptr");
         rc = SDB_VESSEL_INTERNAL_ERR;
         goto error;
      }

      ossMemcpy(record, &cmr, CS_META_RECORD_LEN);
      commitUpdateLog(context, &lrc, LOG_TYPE_CS_CRT, 0, NULL, cmr, adjuncts);
      getRuntimeBuffer()->commit(lrc.getLsn());
      lrc.close();
   done:
      return rc;
   error:
      if (lrc.prepared())
      {
         abortLog(context, &lrc);
      }
      if (getRuntimeBuffer()->isWritable())
      {
         getRuntimeBuffer()->abort();
      }
      goto done;
   }

   INT32 csgpAccessor::readMetaRecord(csMetaRecord &record)const
   {
      INT32 rc = SDB_OK;
      const csMetaRecord *head = NULL;
      if (OSS_UNLIKELY(!isOpen()))
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }

      head = getRuntimeBuffer()->getReadablePtrOfBody<csMetaRecord>(0);
      if (NULL == head)
      {
         PD_LOG(PDERROR, "failed to get record ptr");
         rc = SDB_VESSEL_INTERNAL_ERR;
         goto error;
      }

      record = *head;
   done:
      return rc;
   error:
      goto done;
   }

   INT32 csgpAccessor::prepareUpdateLog(requestContext *context,
                                        logRecordContext *lrc,
                                        DPS_LOG_TYPE ddlType,
                                        BOOLEAN hasOld,
                                        const slice &adjuncts)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(NULL != context, "can not be null");
      SDB_ASSERT(NULL != lrc, "can not be null");
      SDB_ASSERT(!lrc->prepared(), "can not be prepared");
      dpsLogRecordHeader *head = NULL;

      head = &(lrc->getHead());
      head->_type = LOG_TYPE_VESSEL_UPDATE_CS_META;

      if (LOG_TYPE_DUMMY != ddlType)
      {
         OSS_BIT_SET(head->_flags, DPS_VESSEL_LOG_FLAG_DDL);
      }
      

      lrc->prepush(sizeof(GLOBAL_PAGE_ID));
      lrc->prepush(sizeof(DPS_LOG_TYPE));
      lrc->prepush(sizeof(UINT64));
      if (hasOld)
      {
         lrc->prepush(CS_META_RECORD_LEN);
      }
      lrc->prepush(CS_META_RECORD_LEN);
      if (0 < adjuncts.len())
      {
         lrc->prepush(adjuncts.len());
      }
      rc = prepareLogDone(context, lrc);
      if (SDB_OK != rc)
      {
         goto error;
      }
   done:
      return rc;
   error:
      goto done;
   }

   INT32 csgpAccessor::commitUpdateLog(requestContext *context,
                                       logRecordContext *lrc,
                                       DPS_LOG_TYPE ddlType,
                                       UINT64 mask,
                                       const csMetaRecord *old,
                                       const csMetaRecord &record,
                                       const slice &adjuncts)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(NULL != context, "can not be null");
      ISession *session = context->getSession();
      SDB_ASSERT(NULL != lrc, "can not be null");
      SDB_ASSERT(lrc->prepared(), "must be prepared");
      IRedoLogger *logger = context->getOuterResource()->logger;
      const GLOBAL_PAGE_ID &gpid = getRuntimeBuffer()->getGlobalPid();

      rc = logger->pushLogRecordElement(session, lrc,
                                        DPS_LOG_PUBLIC_VESSEL_GPID,
                                        sizeof(GLOBAL_PAGE_ID),
                                        &gpid);
      if (OSS_UNLIKELY(SDB_OK != rc))
      {
         goto error;
      }

      rc = logger->pushLogRecordElement(session, lrc,
                                        DPS_LOG_VESSEL_CL_RECORD_UPDATE_DDL_TYPE,
                                        sizeof(DPS_LOG_TYPE),
                                        &ddlType);
      if (OSS_UNLIKELY(SDB_OK != rc))
      {
         goto error;
      }

      rc = logger->pushLogRecordElement(session, lrc,
                                        DPS_LOG_VESSEL_CL_RECORD_UPDATE_MASK,
                                        sizeof(UINT64),
                                        &mask);
      if (OSS_UNLIKELY(SDB_OK != rc))
      {
         goto error;
      }

      if (NULL != old)
      {
         rc = logger->pushLogRecordElement(session, lrc,
                                           DPS_LOG_VESSEL_CL_RECORD_UPDATE_OLD,
                                           CS_META_RECORD_LEN,
                                           old);
         if (OSS_UNLIKELY(SDB_OK != rc))
         {
            goto error;
         }
      }

      rc = logger->pushLogRecordElement(session, lrc,
                                        DPS_LOG_VESSEL_CL_RECORD_UPDATE_NEW,
                                        CS_META_RECORD_LEN,
                                        &record);
      if (OSS_UNLIKELY(SDB_OK != rc))
      {
         goto error;
      }

      if (0 < adjuncts.len())
      {
         rc = logger->pushLogRecordElement(session, lrc,
                                           DPS_LOG_VESSEL_CL_RECORD_UPDATE_ADJUNCTS,
                                           adjuncts.len(),
                                           adjuncts.data());
         if (OSS_UNLIKELY(SDB_OK != rc))
         {
            goto error;
         }
      }

      rc = pageAccessor::commitLogDone(context, lrc);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to commit log:%d", rc);
         goto error;
      }
   done:
      return rc;
   error:
      SDB_ASSERT(FALSE, "impossible");
      goto done;
   }
}//namespace vessel
}//namespace engine