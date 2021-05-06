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
   
   INT32 csgpAccessor::initPage(requestContext *context, const csMetaRecord &record)
   {
      INT32 rc = SDB_OK;
      CHAR * ptr = NULL;
      SDB_ASSERT(0 == OSS_BIT_TEST(getFlags(), PAGE_ACCESSOR_FLAG_CACHE_MODE), "impossible");
      SDB_ASSERT(0 != OSS_BIT_TEST(getFlags(), PAGE_ACCESSOR_FLAG_NON_READONLY), "impossible");
      SDB_ASSERT(0 != OSS_BIT_TEST(getFlags(), PAGE_ACCESSOR_FLAG_NO_PAGE_VALIDATION), "impossible");

      rc = prepareToWrite(context);
      if (SDB_OK != rc)
      {
         goto error;
      }

      rc = initCommonPageHeadAndTail();
      if (SDB_OK != rc)
      {
         goto error;
      }

      rc = memsetPageBody(0);
      if (SDB_OK != rc)
      {
         goto error;
      }

      rc = getWritePtrOfPageBody(0, sizeof(csMetaRecord), &ptr);
      if (SDB_OK != rc)
      {
         goto error;
      }

      ossMemcpy(ptr, &record, sizeof(csMetaRecord));

      pageAccessor::commit(context, DPS_INVALID_LSN_OFFSET);
   done:
      return rc;
   error:
      if (fullAccessing())
      {
         abortToWrite();
      }
      goto done;
   }

   INT32 csgpAccessor::setOnlineWhenCreating(requestContext *context,
                                             const dataIDMapFileHead &head)
   {
      INT32 rc = SDB_OK;
      const csMetaRecord *recordPtr = NULL;
      csMetaRecord *recordWPtr = NULL;
      logRecordContext lrc;
      DPS_LSN_OFFSET lsn = DPS_INVALID_LSN_OFFSET;
      slice adjuncts(sizeof(dataIDMapFileHead), &head);
      
      rc = getReadableUserHeadPtr<csMetaRecord>(&recordPtr);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to get meta record:%d", rc);
         goto error;
      }

      if (!metaRecordIsValid(*recordPtr))
      {
         PD_LOG(PDERROR, "page crashed[%s]", getGPID().toString().c_str());
         rc = SDB_VESSEL_PAGE_CRASHED;
         goto error;
      }

      if (CMR_STATUS_CREATING != recordPtr->status)
      {
         PD_LOG(PDERROR, "invalid cs status:%d", recordPtr->status);
         rc = SDB_VESSEL_INTERNAL_ERR;
         goto error;
      }

      rc = prepareToWrite(context);
      if (SDB_OK != rc)
      {
         goto error;
      }

      rc = prepareUpdateLog(context, &lrc, LOG_TYPE_CS_CRT, FALSE, adjuncts);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to prepare redo log:%d", rc);
         goto error;
      }

      lsn = lrc.getLsn();

      rc = getWritableUserHeadPtr<csMetaRecord>(&recordWPtr);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to get write ptr of meta:%d", rc);
         goto error;
      }

      recordWPtr->status = CMR_STATUS_ONLINE;
      commitUpdateLog(context, &lrc, LOG_TYPE_CS_CRT,
                      0, NULL, *recordWPtr, adjuncts);

      pageAccessor::commit(context, lsn);
      lrc.close();
   done:
      return rc;
   error:
      if (lrc.prepared())
      {
         IRedoLogger *logger = context->getOuterResource()->logger;
         logger->abort(context->getSession(), &lrc);
      }
      if (fullAccessing())
      {
         abortToWrite();
      }

      goto done;
   }

   INT32 csgpAccessor::readMetaRecord(csMetaRecord &record)
   {
      INT32 rc = SDB_OK;
      const CHAR *ptr = NULL;
      const csMetaRecord *recordPtr = NULL;
      rc = getReadPtrOfPageBody(0, CS_META_RECORD_LEN, &ptr);
      if (SDB_OK != rc)
      {
         goto error;
      }

      recordPtr = (const csMetaRecord *)ptr;
      if (!metaRecordIsValid(*recordPtr))
      {
         PD_LOG(PDERROR, "page crashed[%s]", getGPID().toString().c_str());
         rc = SDB_VESSEL_PAGE_CRASHED;
         goto error;
      }

      record = *recordPtr;
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
      GLOBAL_PAGE_ID gpid = getGPID();

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

      if (lrc->needFullDump())
      {
         const CHAR *dumpBuf = lrc->getFullDumpBuffer();
         SDB_ASSERT(NULL != dumpBuf, "can not be null");
         SDB_ASSERT(0 < lrc->getFullDumpDataSize(), "impossible");
         rc = logger->pushLogRecordElement(session, lrc,
                                           DPS_LOG_PUBLIC_VESSEL_FULL_PAGE_DUMP,
                                           lrc->getFullDumpDataSize(), dumpBuf);
         if (OSS_UNLIKELY(SDB_OK != rc))
         {
            goto error;
         }
      }

      rc = logger->commit(session, lrc);
      if (OSS_UNLIKELY(SDB_OK != rc))
      {
         goto error;
      }
   done:
      return rc;
   error:
      goto done;
   }
}//namespace vessel
}//namespace engine