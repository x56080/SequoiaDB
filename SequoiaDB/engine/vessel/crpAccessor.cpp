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

   Source File Name = crpAccessor.cpp

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

#include "vessel/crpAccessor.h"
#include "vessel/collectionRecordPage.h"
#include "pdTrace.hpp"
#include "vessel/IRedoLogger.h"
#include "vessel/logRecordContext.h"
#include "vessel/requestContext.h"
#include "vessel/outerResource.h"
#include "vessel/redoLogUtil.h"
#include "dpsLogRecordDef.hpp"

namespace engine
{
namespace vessel
{
   crpAccessor::crpAccessor()
   {}

   crpAccessor::~crpAccessor()
   {}

   INT32 crpAccessor::initPage(requestContext *context,
                               PAGE_ID lpid)
   {
      INT32 rc = SDB_OK;
      collectionRecordPageHead *head = NULL;
      pageHead *pageHead = NULL;
      UINT32 capacity = 0;
      UINT32 flags = PAGE_ACCESSOR_FLAG_DIRECT |
                     PAGE_ACCESSOR_FLAG_NON_READONLY |
                     PAGE_ACCESSOR_FLAG_INIT_PAGE;
      SDB_ASSERT(OSS_BIT_TEST(getFlags(), flags), "impossible");

      if (OSS_UNLIKELY(NULL == context || INVALID_PAGE_ID == lpid))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      rc = getCapacityOfCLRecordPage(getPageSize(), capacity);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to get capacity of page size:%d", getPageSize());
         rc = SDB_VESSEL_INTERNAL_ERR;
         goto error;
      }

      rc = prepareToWrite(context);
      if (SDB_OK != rc)
      {
         goto error;
      }

      rc = initCommonPageHeadAndTail(lpid);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to init common head:%d", rc);
         goto error;
      }

      rc = getWritePtrOfHead(&pageHead);
      if (SDB_OK != rc)
      {
         goto error;
      }

      rc = memsetPageBody(0);
      if (SDB_OK != rc)
      {
         goto error;
      }

      rc = getWritableUserHeadPtr<collectionRecordPageHead>(&head);
      if (SDB_OK != rc)
      {
         goto error;
      }

      head->version = COLLECTION_RECORD_PAGE_VERSION_1;

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

   INT32 crpAccessor::update(requestContext *context,
                             DPS_LOG_TYPE ddlType,
                             UINT64 mask,
                             const collectionRecord &record,
                             const slice &adjuncts)
   {
      INT32 rc = SDB_OK;
      CL_MB_ID mbID = record.mbID;
      UINT32 capacity = 0;
      UINT32 slot = 0;
      collectionRecord old;
      const collectionRecord *ptr = NULL;
      collectionRecord *wPtr = NULL;
      logRecordContext lrc;
      BOOLEAN rollback = FALSE;
      DPS_LSN_OFFSET lsn = DPS_INVALID_LSN_OFFSET;
   
      if (OSS_UNLIKELY(NULL == context ||
                       COLLECTION_RECORD_VERSION != record.version ||
                       INVALID_CL_MB_ID == record.mbID))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      rc = getCapacityOfCLRecordPage(getPageSize(), capacity);
      if (OSS_UNLIKELY(SDB_OK != rc))
      {
         PD_LOG(PDERROR, "failed to get capacity of cl record page:%d", rc);
         goto error;
      }

      slot = mbID % capacity;
      rc = getRecordPtr(slot, &ptr);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to get ptr of mbID[%d], rc:%d", mbID, rc);
         goto error;
      }

      if (COLLECTION_RECORD_VERSION != ptr->version ||
          mbID != ptr->mbID)
      {
         PD_LOG(PDERROR, "record of mbid[%d] is invalid", mbID);
         rc = SDB_INVALIDARG;
         goto error;
      }

      ossMemcpy(&old, ptr, COLLECTION_RECORD_LEN);

      rc = prepareUpdateLog(context, &lrc, ddlType, TRUE, adjuncts);
      if (SDB_OK != rc)
      {
         goto error;
      }

      lsn = lrc.getLsn();

      rc = prepareToWrite(context);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to prepare writing:%d", rc);
         goto error;
      }

      rc = getWritableRecordPtr(slot, &wPtr);
      if (SDB_OK != rc)
      {
         goto error;
      }

      rollback = TRUE;
      if (0 == mask)
      {
         ossMemcpy(wPtr, &record, COLLECTION_RECORD_LEN);
      }
      else
      { 
         if (OSS_BIT_TEST(mask, COLLECTION_UPDATE_MASK_COMPRESSTYPE))
         {
            wPtr->compressionType = record.compressionType;
         }
         if (OSS_BIT_TEST(mask, COLLECTION_UPDATE_MASK_FLAGS))
         {
            wPtr->flags = record.flags;
         }
         if (OSS_BIT_TEST(mask, COLLECTION_UPDATE_MASK_NAME))
         {
            ossMemcpy(wPtr->name, record.name, sizeof(wPtr->name));
         }
         if (OSS_BIT_TEST(mask, COLLECTION_UPDATE_MASK_ROUTE_PAGES))
         {
            for (UINT32 i = 0; i < COLLECTION_ROUTE_PAGE_SLOT_COUNT; ++i)
            {
               wPtr->routePages[i] = record.routePages[i];
            }
         }
         if (OSS_BIT_TEST(mask, COLLECTION_UPDATE_MASK_COMPRESSION_DIC))
         {
            wPtr->compressionDic = record.compressionDic;
         }
         if (OSS_BIT_TEST(mask, COLLECTION_UPDATE_MASK_INDEX))
         {
            wPtr->uniqueIndexCount = record.nonUniqueIndexCount;
            wPtr->uniqueIndexCount = record.uniqueIndexCount;
            wPtr->indexPad = record.indexPad;
            wPtr->nextIndexID = record.nextIndexID;
            wPtr->indexSlots = record.indexSlots;
         }
         if (OSS_BIT_TEST(mask, COLLECTION_UPDATE_MASK_FS_RESERVED))
         {
            wPtr->freeSizeReserved = record.freeSizeReserved;
         }
         if (OSS_BIT_TEST(mask, COLLECTION_UPDATE_MASK_STRIPING))
         {
            wPtr->maxSGCount = record.maxSGCount;
            wPtr->minStriping = record.minStriping;
            wPtr->maxStriping = record.maxStriping;
         }
      }

      rc = commitUpdateLog(context, &lrc, ddlType,
                           mask, &old, *wPtr, adjuncts);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to commit log[%lld]. rc:%d", lsn, rc);
         goto error;
      }
      
      pageAccessor::commit(context, lsn);
      lrc.close();
      rollback = FALSE;
   done:
      return rc;
   error:
      if (lrc.prepared())
      {
         IRedoLogger *logger = context->getOuterResource()->logger;
         logger->abort(context->getSession(), &lrc);
      }
      if (rollback)
      {
         writeToSlot(slot, old);
         abortToWrite();
      }
      if (fullAccessing())
      {
         abortToWrite();
      }
      goto done;
   }

   INT32 crpAccessor::createCL(requestContext *context,
                               const collectionRecord &record)
   {
      INT32 rc = SDB_OK;
      CL_MB_ID mbID = record.mbID;
      UINT32 slot = 0;
      logRecordContext lrContext;
      BOOLEAN rollback = FALSE;
      DPS_LSN_OFFSET lsn = DPS_INVALID_LSN_OFFSET;
      const collectionRecord *recordPtr = NULL;
      UINT32 capacity;
      collectionRecord r;
      strSlice clName(record.name);

      if (OSS_UNLIKELY(NULL == context ||
                       COLLECTION_RECORD_VERSION != record.version ||
                       DMS_INVALID_LOGICCLID == record.logicalCLID ||
                       INVALID_CL_MB_ID == record.mbID ||
                       clName.empty()))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      rc = getCapacityOfCLRecordPage(getPageSize(), capacity);
      if (OSS_UNLIKELY(SDB_OK != rc))
      {
         PD_LOG(PDERROR, "failed to get capacity of cl record page:%d", rc);
         goto error;
      }

      slot = mbID % capacity;
      rc = getRecordPtr(slot, &recordPtr);
      if (SDB_OK != rc)
      {
         goto error;
      }

      if (recordPtr->version != COLLECTION_RECORD_INVALID_VERSION)
      {
         PD_LOG(PDERROR, "mbid[%d] in crp is in used", mbID);
         rc = SDB_VESSEL_INTERNAL_ERR;
         goto error;
      }

      rc = prepareUpdateLog(context, &lrContext, LOG_TYPE_CL_CRT, FALSE, slice());
      if (SDB_OK != rc)
      {
         goto error;
      }
      lsn = lrContext.getLsn();

      rc = prepareToWrite(context);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to prepare writing:%d", rc);
         goto error;
      }

      r.version = record.version;
      r.type = record.type;
      r.innerID = record.innerID;
      r.logicalCLID = record.logicalCLID;
      r.mbID = record.mbID;
      r.maxSGCount = record.maxSGCount;
      r.compressionType = record.compressionType;
      r.flags = record.flags;
      r.freeSizeReserved = record.freeSizeReserved;
      r.minStriping = record.minStriping;
      r.maxStriping = record.maxStriping;
      ossMemcpy(r.name, clName.str(), clName.strLen());
      rc = writeToSlot(slot, r);
      if (SDB_OK != rc)
      {
         goto error;
      }
      rollback = TRUE;

      rc = commitUpdateLog(context, &lrContext, LOG_TYPE_CL_CRT,
                           0, NULL, r, slice());
      if (SDB_OK != rc)
      {
         goto error;
      }

      pageAccessor::commit(context, lsn);
      lrContext.close();
      rollback = FALSE;

   done:
      return rc;
   error:
      if (lrContext.prepared())
      {
         IRedoLogger *logger = context->getOuterResource()->logger;
         logger->abort(context->getSession(), &lrContext);
      }
      if (rollback)
      {
         r.reset();
         writeToSlot(slot, r);
         abortToWrite();
      }
      if (fullAccessing())
      {
         abortToWrite();
      }
      goto done;
   }

   INT32 crpAccessor::getClRecordBySlot(UINT32 slot, collectionRecord &record)
   {
      INT32 rc = SDB_OK;
      UINT32 capacity = 0;
      const collectionRecord *ptr = NULL;

      rc = getCapacityOfCLRecordPage(getPageSize(), capacity);
      if (OSS_UNLIKELY(SDB_OK != rc))
      {
         PD_LOG(PDERROR, "failed to get capacity of cl record page:%d", rc);
         goto error;
      }

      if (capacity <= slot)
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      rc = getRecordPtr(slot, &ptr);
      if (SDB_OK != rc)
      {
         goto error;
      }

      if (COLLECTION_RECORD_VERSION != ptr->version)
      {
         rc = SDB_DMS_NOTEXIST;
         goto error;
      }

      ossMemcpy(&record, ptr, COLLECTION_RECORD_LEN);
   done:
      return rc;
   error:
      goto done;
   }

   INT32 crpAccessor::writeToSlot(UINT32 slot, const collectionRecord &record)
   {
      INT32 rc = SDB_OK;
      UINT32 offset = COLLECTION_RECORD_PAGE_HEAD_LEN + slot * COLLECTION_DISK_RECORD_LEN;
      UINT32 len = COLLECTION_RECORD_LEN;

      rc = writePageBody(offset, len, (const CHAR *)(&record));
      if (SDB_OK != rc)
      {
         goto error;
      }
   done:
      return rc;
   error:
      goto done;
   }

   INT32 crpAccessor::getRecordPtr(UINT32 slot, const collectionRecord **record)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(NULL != record, "can not be null");
      UINT32 offset = COLLECTION_RECORD_PAGE_HEAD_LEN + slot * COLLECTION_DISK_RECORD_LEN;
      const collectionRecordOnDisk *ptr = NULL;
      rc = getReadPtrOfPageBody<collectionRecordOnDisk>(offset, &ptr);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to get slot[%d], rc:%d", slot, rc);
         goto error;
      }

      *record = &(ptr->record);
   done:
      return rc;
   error:
      goto done;
   }

   INT32 crpAccessor::getWritableRecordPtr(UINT32 slot, collectionRecord **record)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(NULL != record, "can not be null");
      UINT32 offset = COLLECTION_RECORD_PAGE_HEAD_LEN + slot * COLLECTION_DISK_RECORD_LEN;
      collectionRecordOnDisk *ptr = NULL;
      rc = getWritePtrOfPageBody<collectionRecordOnDisk>(offset, &ptr);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to get slot[%d], rc:%d", slot, rc);
         goto error;
      }
      *record = &(ptr->record);
   done:
      return rc;
   error:
      goto done;
   }

   

   INT32 crpAccessor::prepareUpdateLog(requestContext *context,
                                       logRecordContext *lrc,
                                       DPS_LOG_TYPE ddlType,
                                       BOOLEAN hasOld,
                                       const slice &adjuncts)
   {
      INT32 rc = SDB_OK;

      SDB_ASSERT(NULL != lrc, "can not be null");
      SDB_ASSERT(!lrc->prepared(), "can not be prepared");
      dpsLogRecordHeader *head = NULL;

      head = &(lrc->getHead());
      head->_type = LOG_TYPE_VESSEL_UPDATE_CL_RECORD;

      if (LOG_TYPE_DUMMY != ddlType)
      {
         OSS_BIT_SET(head->_flags, DPS_VESSEL_LOG_FLAG_DDL);
      }

      lrc->prepush(sizeof(GLOBAL_PAGE_ID));
      lrc->prepush(sizeof(DPS_LOG_TYPE));
      lrc->prepush(sizeof(UINT64));
      if (hasOld)
      {
         lrc->prepush(COLLECTION_RECORD_LEN);
      }
      lrc->prepush(COLLECTION_RECORD_LEN);
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

   INT32 crpAccessor::commitUpdateLog(requestContext *context,
                                      logRecordContext *lrc,
                                      DPS_LOG_TYPE ddlType,
                                      UINT64 mask,
                                      const collectionRecord *oldRecord,
                                      const collectionRecord &newRecord,
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

      if (NULL != oldRecord)
      {
         rc = logger->pushLogRecordElement(session, lrc,
                                          DPS_LOG_VESSEL_CL_RECORD_UPDATE_OLD,
                                          COLLECTION_RECORD_LEN,
                                          oldRecord);
         if (OSS_UNLIKELY(SDB_OK != rc))
         {
            goto error;
         }
      }

      rc = logger->pushLogRecordElement(session, lrc,
                                        DPS_LOG_VESSEL_CL_RECORD_UPDATE_NEW,
                                        COLLECTION_RECORD_LEN,
                                        &newRecord);
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
         const CHAR *dumpBuf = getFullDumpBuffer();
         SDB_ASSERT(NULL != dumpBuf, "can not be null");
         rc = logger->pushLogRecordElement(session, lrc,
                                           DPS_LOG_PUBLIC_VESSEL_FULL_PAGE_DUMP,
                                           getFullDumpSize(), dumpBuf);
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