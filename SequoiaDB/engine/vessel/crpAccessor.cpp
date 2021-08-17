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
#include "vessel/logicalPageBuffer.h"

namespace engine
{
namespace vessel
{
   crpAccessor::crpAccessor()
   {}

   crpAccessor::~crpAccessor()
   {}

   INT32 crpAccessor::createCL(requestContext *context,
                               const collectionRecord &record,
                               const createCLOptions &options,
                               const strSlice &csName,
                               logicalPageBuffer *lpb)
   {
      INT32 rc = SDB_OK;
      UINT32 slot = 0;
      logRecordContext lrc;
      DPS_LSN_OFFSET lsn = DPS_INVALID_LSN_OFFSET;
      collectionRecordOnDisk *recordPtr = NULL;
      UINT32 capacity;
      ossValuePtr ptr = 0;
      CHAR *fullNameBuffer = NULL;
      UINT32 bufferSize = 0;
      strSlice clNameSlice;
      bson::BSONObj obj;
      
      if (OSS_UNLIKELY(NULL == context ||
                       !record.isValid() ||
                       !options.isValid() ||
                       csName.empty() ||
                       NULL == lpb ||
                       !lpb->isValid()))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      ptr = (ossValuePtr)(lpb->getRuntimeBuffer().getReadOnlyBuffer());

      rc = validatePage(ptr, PAGE_TYPE_CL_META,
                        lpb->getRuntimeBuffer().getPageSize(),
                        lpb->getRuntimeBuffer().getGlobalPid().page(),
                        lpb->getLogicalPid(), lpb->getCowTrigger().getPsv());
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to validate page[%s], rc:%d",
                lpb->getRuntimeBuffer().getGlobalPid().toString().c_str(), rc);
         goto error;
      }

      rc = getCapacityOfCLRecordPage(lpb->getRuntimeBuffer().getPageSize(), capacity);
      if (OSS_UNLIKELY(SDB_OK != rc))
      {
         PD_LOG(PDERROR, "failed to get capacity of cl record page:%d", rc);
         goto error;
      }

      slot = record.mbID % capacity;
      clNameSlice.reset(record.name);
      SDB_ASSERT(!clNameSlice.empty(), "can not be empty");
      bufferSize = csName.strLen() + clNameSlice.strLen() + 2;
      fullNameBuffer = context->allocateBuffer(bufferSize);
      if (NULL == fullNameBuffer)
      {
         PD_LOG(PDERROR, "failed to allocate mem");
         rc = SDB_OOM;
         goto error;
      }
      ossMemcpy(fullNameBuffer, csName.str(), csName.strLen());
      fullNameBuffer[csName.strLen()] = '.';
      ossMemcpy(fullNameBuffer + csName.strLen() + 1,
                clNameSlice.str(), clNameSlice.strLen());
      fullNameBuffer[bufferSize - 1] = '\0'; 

      rc = lpb->prepareToWrite(context);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to prepare writing:%d", rc);
         goto error;
      }

      obj = options.toBson();

      rc = prepareCreateCLLog(context, bufferSize, obj.objsize(),
                              &(lpb->getRuntimeBuffer()), &lrc);
      if (SDB_OK != rc)
      {
         goto error;
      }
      lsn = lrc.getLsn();

      recordPtr = getWritableDiskRecordPtr(&(lpb->getRuntimeBuffer()), slot);
      if (NULL == recordPtr)
      {
         PD_LOG(PDERROR, "failed to get writable disk ptr");
         rc = SDB_VESSEL_INTERNAL_ERR;
         goto error;
      }

      ossMemset(recordPtr, 0, COLLECTION_RECORD_LEN);
      recordPtr->record = record;

      rc = commitCreateCLLog(context, bufferSize, fullNameBuffer,
                             lpb->getRuntimeBuffer().getGlobalPid(),
                             record, slice(obj.objsize(), obj.objdata()), &lrc);
      if (SDB_OK != rc)
      {
         PD_LOG(PDSEVERE, "failed to commit log[%lld], rc:%d", lsn, rc);
         ossMemset(recordPtr, 0, COLLECTION_RECORD_LEN);
         ossPanic();
         goto error;
      }

      lpb->getRuntimeBuffer().commit(lsn);

   done:
      if (NULL != fullNameBuffer)
      {
         context->releaseBuffer(fullNameBuffer, bufferSize);
      }
      return rc;
   error:
      if (lrc.prepared())
      {
         pageAccessor::abortLog(context, &lrc);
      }
      if (lpb->getRuntimeBuffer().isWritingPrepared())
      {
         lpb->getRuntimeBuffer().abort();
      }
      goto done;
   }

   INT32 crpAccessor::updateRoutePages(requestContext *context,
                                       const collectionRecord &record,
                                       logicalPageBuffer *lpb)
   {
      INT32 rc = SDB_OK;
      UINT32 slot = 0;
      logRecordContext lrc;
      collectionRecordOnDisk *wptr = NULL;
      collectionRecord oldRecord;
      UINT32 capacity = 0;
      ossValuePtr ptr = 0;
      const runtimePageBuffer *rpb = NULL;
      
      if (OSS_UNLIKELY(NULL == context ||
                       !record.isValid() ||
                       NULL == lpb ||
                       !lpb->isValid()))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      rpb = &(lpb->getRuntimeBuffer());
      ptr = (ossValuePtr)(rpb->getReadOnlyBuffer());

      rc = validatePage(ptr, PAGE_TYPE_CL_META,
                        rpb->getPageSize(),
                        rpb->getGlobalPid().page(),
                        lpb->getLogicalPid(),
                        lpb->getCowTrigger().getPsv());
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to validate page[%s], rc:%d",
                lpb->getRuntimeBuffer().getGlobalPid().toString().c_str(), rc);
         goto error;
      }

      rc = getCapacityOfCLRecordPage(rpb->getPageSize(), capacity);
      if (OSS_UNLIKELY(SDB_OK != rc))
      {
         PD_LOG(PDERROR, "failed to get capacity of cl record page:%d", rc);
         goto error;
      }

      slot = record.mbID % capacity;

      rc = lpb->prepareToWrite(context);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to prepare to write:%d", rc);
         goto error;
      }

      wptr = getWritableDiskRecordPtr(rpb, slot);
      if (NULL == wptr)
      {
         PD_LOG(PDERROR, "failed to get writable ptr of slot[%d]", slot);
         rc = SDB_VESSEL_INTERNAL_ERR;
         goto error;
      }

      if (!wptr->record.isValid())
      {
         PD_LOG(PDERROR, "invalid record on disk");
         rc = SDB_VESSEL_INTERNAL_ERR;
         goto error;
      }

      oldRecord = wptr->record;
      rc = prepareUpdateLog(context, rpb, &lrc);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to prepare dps log:%d", rc);
         goto error;
      }

      for (UINT32 i = 0; i < COLLECTION_ROUTE_PAGE_SLOT_COUNT; ++i)
      {
         wptr->record.routePages[i] = record.routePages[i];
      }

      rc = commitUpdateLog(context, &lrc, rpb->getGlobalPid(),
                           lpb->getLogicalPid(),
                           COLLECTION_UPDATE_MASK_ROUTE_PAGES,
                           oldRecord, wptr->record);
      if (SDB_OK != rc)
      {
         PD_LOG(PDSEVERE, "failed to commit log[%lld], rc:%d",
                lrc.getLsn(), rc);
         ossPanic();
         for (UINT32 i = 0; i < COLLECTION_ROUTE_PAGE_SLOT_COUNT; ++i)
         {
            wptr->record.routePages[i] = oldRecord.routePages[i];
         }
         goto error;
      }

      lpb->getRuntimeBuffer().commit(lrc.getLsn());
   done:
      return rc;
   error:
      if (lrc.prepared())
      {
         pageAccessor::abortLog(context, &lrc);
      }
      if (lpb->getRuntimeBuffer().isWritingPrepared())
      {
         lpb->getRuntimeBuffer().abort();
      }
      goto done;
   }

   INT32 crpAccessor::updateIndexInfo(requestContext *context,
                                      CL_MB_ID mbID,
                                      UINT64 uniqueIndexes,
                                      UINT64 nonuniqueIndexes,
                                      logicalPageBuffer *lpb)
      {
      INT32 rc = SDB_OK;
      runtimePageBuffer *rpb = NULL;
      UINT32 capacity = 0;
      const collectionRecordOnDisk *readblePtr = NULL;
      logRecordContext lrc;
      collectionRecordOnDisk *wptr = NULL;
      collectionRecord oldRecord;
      UINT64 mask = COLLECTION_UPDATE_MASK_INDEX_INFO;

      if (OSS_UNLIKELY(NULL == context ||
                       INVALID_CL_MB_ID == mbID ||
                       NULL == lpb ||
                       !lpb->isValid()))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      rpb = &(lpb->getRuntimeBuffer());
      rc = validatePage((ossValuePtr)(rpb->getReadOnlyBuffer()),
                        PAGE_TYPE_CL_META,
                        rpb->getPageSize(),
                        rpb->getGlobalPid().page(),
                        lpb->getLogicalPid(),
                        lpb->getCowTrigger().getPsv());
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to validate page[%s], rc:%d",
                lpb->getRuntimeBuffer().getGlobalPid().toString().c_str(), rc);
         goto error;
      }

      rc = getCapacityOfCLRecordPage(rpb->getPageSize(), capacity);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to get crp capacity:%d", rc);
         goto error;
      }

      readblePtr = getReadableDiskRecordPtr(rpb, mbID % capacity);
      if (NULL == readblePtr)
      {
         PD_LOG(PDERROR, "failed to get readble record ptr");
         rc = SDB_VESSEL_INTERNAL_ERR;
         goto error;
      }
      if (!readblePtr->record.isValid())
      {
         PD_LOG(PDERROR, "record at pos[%d] is invalid", mbID % capacity);
         rc = SDB_VESSEL_INTERNAL_ERR;
         goto error;
      }

      if (readblePtr->record.mbID != mbID)
      {
         PD_LOG(PDERROR, "mbid[%d] does not match the one on disk[%d]",
                mbID, readblePtr->record.mbID);
         rc = SDB_VESSEL_PAGE_HEAD_NOT_MATCH;
         goto error;
      }

      rc = rpb->prepareToWrite(context);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to get buffer ready to write:%d", rc);
         goto error;
      }
      readblePtr = NULL;

      rc = prepareUpdateLog(context, rpb, &lrc);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to prepare create index log record:%d", rc);
         goto error;
      }

      wptr = getWritableDiskRecordPtr(rpb, mbID % capacity);
      if (NULL == wptr)
      {
         PD_LOG(PDERROR, "failed to get writable ptr");
         rc = SDB_VESSEL_INTERNAL_ERR;
         goto error;
      }

      oldRecord = wptr->record;
      wptr->record.uniqueIndexes = uniqueIndexes;
      wptr->record.nonUniqueIndexes = nonuniqueIndexes;

      rc = commitUpdateLog(context, &lrc, rpb->getGlobalPid(),
                           lpb->getLogicalPid(),
                           mask, oldRecord, wptr->record);
      if (SDB_OK != rc)
      {
         wptr->record.uniqueIndexes = oldRecord.uniqueIndexes;
         wptr->record.nonUniqueIndexes = oldRecord.nonUniqueIndexes;
         PD_LOG(PDERROR, "failed to commit dps log[%lld], rc:%d",
                lrc.getLsn(), rc);
         ossPanic();
         goto error;
      }

      rpb->commit(lrc.getLsn());
   done:
      return rc;
   error:
      if (lrc.prepared())
      {
         pageAccessor::abortLog(context, &lrc);
      }
      if (NULL != rpb && rpb->isWritingPrepared())
      {
         rpb->abort();
      }
      goto done;
   }

   collectionRecordOnDisk *crpAccessor::getWritableDiskRecordPtr(const runtimePageBuffer *rpb,
                                                                 UINT32 i)
   {
      SDB_ASSERT(NULL != rpb, "can not be null");
      UINT32 offset = COLLECTION_DISK_RECORD_LEN * i;
      collectionRecordOnDisk *ptr = rpb->getWritablePtrOfBody<collectionRecordOnDisk>(offset);
      return ptr;
   }

   const collectionRecordOnDisk *crpAccessor::getReadableDiskRecordPtr(const runtimePageBuffer *rpb,
                                                                 UINT32 i)
   {
      SDB_ASSERT(NULL != rpb, "can not be null");
      UINT32 offset = COLLECTION_DISK_RECORD_LEN * i;
      const collectionRecordOnDisk *ptr =
               rpb->getReadablePtrOfBody<collectionRecordOnDisk>(offset);
      return ptr;
   }

   INT32 crpAccessor::prepareCreateCLLog(requestContext *context,
                                         UINT32 fullNameSize,
                                         UINT32 adjunctSize,
                                         const runtimePageBuffer *rpb,
                                         logRecordContext *lrc)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(NULL != context, "can not be null");
      SDB_ASSERT(0 != fullNameSize, "can not be zero");
      SDB_ASSERT(0 != adjunctSize, "can not be zero");
      SDB_ASSERT(NULL != rpb, "can not be null");
      SDB_ASSERT(NULL != lrc, "can not be null");
      SDB_ASSERT(!lrc->prepared(), "can not be prepared");

      rc = pageAccessor::prepareLog(context, rpb,
                                    LOG_TYPE_CL_CRT,
                                    FALSE, lrc);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to prepare log:%d", rc);
         goto error;
      }

      lrc->setDDL();

      lrc->prepush(fullNameSize);
      lrc->prepush(sizeof(GLOBAL_PAGE_ID));
      lrc->prepush(sizeof(CL_MB_ID));
      lrc->prepush(sizeof(UINT32));
      lrc->prepush(sizeof(UINT32));
      lrc->prepush(adjunctSize);

      rc = pageAccessor::prepareLogDone(context, lrc);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to prepare log done:%d", rc);
         goto error;
      }
   done:
      return rc;
   error:
      goto done;
   }

   INT32 crpAccessor::commitCreateCLLog(requestContext *context,
                                        UINT32 fullNameSize,
                                        const CHAR *fullName,
                                        const GLOBAL_PAGE_ID &gpid,
                                        const collectionRecord &record,
                                        const slice &adjunct,
                                        logRecordContext *lrc)
   {
      INT32 rc = SDB_OK;
      rc = pageAccessor::pushElement(context, DPS_LOG_PUBLIC_FULLNAME,
                                     fullNameSize, fullName, lrc);
      if (SDB_OK != rc)
      {
         goto error;
      }
      rc = pageAccessor::pushElement(context, DPS_LOG_PUBLIC_VESSEL_GPID,
                                     sizeof(GLOBAL_PAGE_ID), &gpid, lrc);
      if (SDB_OK != rc)
      {
         goto error;
      }
      rc = pageAccessor::pushElement(context, DPS_LOG_CLCRT_VESSEL_MBID,
                                     sizeof(CL_MB_ID), &(record.mbID), lrc);
      if (SDB_OK != rc)
      {
         goto error;
      }
      rc = pageAccessor::pushElement(context, DPS_LOG_CLCRT_VESSEL_MBID,
                                     sizeof(UINT32), &(record.innerID), lrc);
      if (SDB_OK != rc)
      {
         goto error;
      }
      rc = pageAccessor::pushElement(context, DPS_LOG_CLCRT_VESSEL_LOGICAL_ID,
                                     sizeof(UINT32), &(record.logicalCLID), lrc);
      if (SDB_OK != rc)
      {
         goto error;
      }
      rc = pageAccessor::pushElement(context, DPS_LOG_CLCRT_VESSEL_ADJUNCT,
                                     adjunct.len(), adjunct.data(), lrc);
      if (SDB_OK != rc)
      {
         goto error;
      }
      rc = pageAccessor::commitLog(context, lrc);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to commit log[%lld], rc:%d", lrc->getLsn(), rc);
         goto error;
      }
   done:
      return rc;
   error:
      goto done;
   }

   INT32 crpAccessor::prepareUpdateLog(requestContext *context,
                                       const runtimePageBuffer *rpb,
                                       logRecordContext *lrc)
   {
      INT32 rc = SDB_OK;

      rc = pageAccessor::prepareLog(context, rpb,
                                    LOG_TYPE_VESSEL_CRP_UPDATE,
                                    FALSE, lrc);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to prepare log:%d", rc);
         goto error;
      }
      
      lrc->prepush(sizeof(GLOBAL_PAGE_ID));
      lrc->prepush(sizeof(UINT32));
      lrc->prepush(sizeof(UINT64));
      lrc->prepush(COLLECTION_RECORD_LEN);
      lrc->prepush(COLLECTION_RECORD_LEN);

      rc = pageAccessor::prepareLogDone(context, lrc);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to prepare done log:%d", rc);
         goto error;
      }
   done:
      return rc;
   error:
      goto done;
   }

   INT32 crpAccessor::commitUpdateLog(requestContext *context,
                                      logRecordContext *lrc,
                                      const GLOBAL_PAGE_ID &gpid,
                                      PAGE_ID lpid,
                                      UINT64 mask,
                                      const collectionRecord &oldRecord,
                                      const collectionRecord &newRecord)
   {
      INT32 rc = SDB_OK;
      rc = pageAccessor::pushElement(context, DPS_LOG_PUBLIC_VESSEL_GPID,
                                     sizeof(GLOBAL_PAGE_ID), &gpid, lrc);
      if (SDB_OK != rc)
      {
         goto error;
      }
      rc = pageAccessor::pushElement(context, DPS_LOG_VESSEL_CL_RECORD_UPDATE_LPID,
                                     sizeof(UINT32), &lpid, lrc);
      if (SDB_OK != rc)
      {
         goto error;
      }
      rc = pageAccessor::pushElement(context, DPS_LOG_VESSEL_CL_RECORD_UPDATE_MASK,
                                     sizeof(UINT64), &mask, lrc);
      if (SDB_OK != rc)
      {
         goto error;
      }
      rc = pageAccessor::pushElement(context, DPS_LOG_VESSEL_CL_RECORD_UPDATE_OLD,
                                     COLLECTION_RECORD_LEN, &oldRecord, lrc);
      if (SDB_OK != rc)
      {
         goto error;
      }
      rc = pageAccessor::pushElement(context, DPS_LOG_VESSEL_CL_RECORD_UPDATE_NEW,
                                     COLLECTION_RECORD_LEN, &newRecord, lrc);
      if (SDB_OK != rc)
      {
         goto error;
      }
      rc = pageAccessor::commitLog(context, lrc);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to commit log[%lld], rc:%d", lrc->getLsn(), rc);
         goto error;
      }
   done:
      return rc;
   error:
      goto done;
   }

}//namespace vessel
}//namespace engine