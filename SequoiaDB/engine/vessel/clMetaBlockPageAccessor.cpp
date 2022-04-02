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

   Source File Name = clMetaBlockPageAccessor.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#include "vessel/clMetaBlockPageAccessor.h"
#include "vessel/clMetaBlockPage.h"
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
   clMetaBlockPageAccessor::clMetaBlockPageAccessor()
   {}

   clMetaBlockPageAccessor::~clMetaBlockPageAccessor()
   {}

   INT32 clMetaBlockPageAccessor::createCL(requestContext *context,
                                           const clMetaBlock &block,
                                           const createCLOptions &options,
                                           logicalPageBuffer *lpb)
   {
      INT32 rc = SDB_OK;
      UINT32 slot = 0;
      logRecordContext lrc;
      DPS_LSN_OFFSET lsn = DPS_INVALID_LSN_OFFSET;
      clMetaBlockOnDisk *blockPtr = NULL;
      UINT32 capacity;
      bson::BSONObj obj;
      strictBuffer buffer;
      
      if (OSS_UNLIKELY(NULL == context ||
                       !block.isValid() ||
                       !options.isValid() ||
                       NULL == lpb ||
                       !lpb->isValid()))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      rc = lpb->validatePage(PAGE_TYPE_CL_META);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to validate page[%s], rc:%d",
                lpb->getRuntimeBuffer().getGlobalPid().toString().c_str(), rc);
         goto error;
      }

      rc = getCapacityOfCLMetaBlockPage(lpb->getRuntimeBuffer().getPageSize(), capacity);
      if (OSS_UNLIKELY(SDB_OK != rc))
      {
         PD_LOG(PDERROR, "failed to get capacity of cl record page:%d", rc);
         goto error;
      }

      slot = block.mbID % capacity;

      rc = lpb->prepareToWrite();
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to prepare writing:%d", rc);
         goto error;
      }

      buffer = lpb->getWritableBodyBuffer();
      obj = options.toBson();

      rc = prepareCreateCLLog(context, obj.objsize(),
                              &(lpb->getRuntimeBuffer()), &lrc);
      if (SDB_OK != rc)
      {
         goto error;
      }
      lsn = lrc.getLsn();

      blockPtr = buffer.getWritableObjPtr<clMetaBlockOnDisk>
                 (CL_DISK_META_BLOCK_LEN * slot);
      if (NULL == blockPtr)
      {
         PD_LOG(PDERROR, "failed to get writable disk ptr");
         rc = SDB_VESSEL_INTERNAL_ERR;
         goto error;
      }

      ossMemset(blockPtr, 0, CL_DISK_META_BLOCK_LEN);
      blockPtr->block = block;

      rc = commitCreateCLLog(context, lpb->getRuntimeBuffer().getGlobalPid(),
                             block, slice(obj.objsize(), obj.objdata()), &lrc);
      if (SDB_OK != rc)
      {
         PD_LOG(PDSEVERE, "failed to commit log[%lld], rc:%d", lsn, rc);
         ossMemset(blockPtr, 0, CL_DISK_META_BLOCK_LEN);
         ossPanic();
         goto error;
      }

      lpb->commit(lsn);

   done:
      return rc;
   error:
      if (lrc.prepared())
      {
         pageAccessor::abortLog(context, &lrc);
      }
      goto done;
   }

   INT32 clMetaBlockPageAccessor::removeCL(requestContext *context,
                                           logicalPageBuffer *lpb)
   {
      INT32 rc = SDB_OK;
      UINT32 slot = 0;
      logRecordContext lrc;
      UINT32 capacity;
      strictBuffer buffer;
      clMetaBlockOnDisk *blockPtr = NULL;
      DPS_LSN_OFFSET lsn = DPS_INVALID_LSN_OFFSET;

      if (OSS_UNLIKELY(NULL == context ||
                       INVALID_CL_MB_ID == context->getMBID() ||
                       NULL == lpb ||
                       !lpb->isValid()))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      rc = lpb->validatePage(PAGE_TYPE_CL_META);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to validate page[%s], rc:%d",
                lpb->getRuntimeBuffer().getGlobalPid().toString().c_str(), rc);
         goto error;
      }

      rc = getCapacityOfCLMetaBlockPage(lpb->getRuntimeBuffer().getPageSize(), capacity);
      if (OSS_UNLIKELY(SDB_OK != rc))
      {
         PD_LOG(PDERROR, "failed to get capacity of cl record page:%d", rc);
         goto error;
      }

      slot = context->getMBID() % capacity;

      rc = lpb->prepareToWrite();
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to prepare writing:%d", rc);
         goto error;
      }

      buffer = lpb->getWritableBodyBuffer();
      blockPtr = buffer.getWritableObjPtr<clMetaBlockOnDisk>
                 (CL_DISK_META_BLOCK_LEN * slot);
      if (NULL == blockPtr)
      {
         PD_LOG(PDERROR, "failed to get writable disk ptr");
         rc = SDB_VESSEL_INTERNAL_ERR;
         goto error;
      }

      ossMemset(blockPtr, 0, CL_DISK_META_BLOCK_LEN);
      blockPtr->block.reset();

      rc = commitRemoveLog(context, &(lpb->getRuntimeBuffer()), lsn);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to commit remove log:%d", rc);
         goto error;
      }

      lpb->commit(lsn);
   done:
      return rc;
   error:
      goto done;
   }

   INT32 clMetaBlockPageAccessor::updateRoutePages(requestContext *context,
                                                   const clMetaBlock &block,
                                                   logicalPageBuffer *lpb)
   {
      INT32 rc = SDB_OK;
      UINT32 slot = 0;
      logRecordContext lrc;
      clMetaBlockOnDisk *wptr = NULL;
      clMetaBlock oldBlock;
      UINT32 capacity = 0;
      strictBuffer buffer;
      
      if (OSS_UNLIKELY(NULL == context ||
                       !block.isValid() ||
                       NULL == lpb ||
                       !lpb->isValid()))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      rc = lpb->validatePage(PAGE_TYPE_CL_META);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to validate page[%s], rc:%d",
                lpb->getRuntimeBuffer().getGlobalPid().toString().c_str(), rc);
         goto error;
      }

      rc = getCapacityOfCLMetaBlockPage(lpb->getPageSize(), capacity);
      if (OSS_UNLIKELY(SDB_OK != rc))
      {
         PD_LOG(PDERROR, "failed to get capacity of cl record page:%d", rc);
         goto error;
      }

      slot = block.mbID % capacity;

      rc = lpb->prepareToWrite();
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to prepare to write:%d", rc);
         goto error;
      }

      buffer = lpb->getWritableBodyBuffer();
      SDB_ASSERT(buffer.isWritable(), "must be writable");

      wptr = buffer.getWritableObjPtr<clMetaBlockOnDisk>
             (CL_DISK_META_BLOCK_LEN * slot);
      if (NULL == wptr)
      {
         PD_LOG(PDERROR, "failed to get writable ptr of slot[%d]", slot);
         rc = SDB_VESSEL_INTERNAL_ERR;
         goto error;
      }

      if (!wptr->block.isValid())
      {
         PD_LOG(PDERROR, "invalid record on disk");
         rc = SDB_VESSEL_INTERNAL_ERR;
         goto error;
      }

      oldBlock = wptr->block;
      rc = prepareUpdateLog(context, &(lpb->getRuntimeBuffer()), &lrc);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to prepare dps log:%d", rc);
         goto error;
      }

      for (UINT32 i = 0; i < COLLECTION_ROUTE_PAGE_SLOT_COUNT; ++i)
      {
         wptr->block.routePages[i] = block.routePages[i];
      }

      rc = commitUpdateLog(context, &lrc, lpb->getRuntimeBuffer().getGlobalPid(),
                           lpb->getLogicalPid(),
                           COLLECTION_UPDATE_MASK_ROUTE_PAGES,
                           oldBlock, wptr->block);
      if (SDB_OK != rc)
      {
         PD_LOG(PDSEVERE, "failed to commit log[%lld], rc:%d",
                lrc.getLsn(), rc);
         ossPanic();
         for (UINT32 i = 0; i < COLLECTION_ROUTE_PAGE_SLOT_COUNT; ++i)
         {
            wptr->block.routePages[i] = oldBlock.routePages[i];
         }
         goto error;
      }

      lpb->commit(lrc.getLsn());
   done:
      return rc;
   error:
      if (lrc.prepared())
      {
         pageAccessor::abortLog(context, &lrc);
      }
      goto done;
   }

   INT32 clMetaBlockPageAccessor::truncateRouteMap(requestContext *context,
                                                   logicalPageBuffer *lpb)
   {
      INT32 rc = SDB_OK;
      UINT32 slot = 0;
      logRecordContext lrc;
      clMetaBlockOnDisk *wptr = NULL;
      clMetaBlock oldBlock;
      UINT32 capacity = 0;
      strictBuffer buffer;
      
      if (OSS_UNLIKELY(NULL == context ||
                       INVALID_CL_MB_ID == context->getMBID() ||
                       NULL == lpb ||
                       !lpb->isValid()))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      rc = lpb->validatePage(PAGE_TYPE_CL_META);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to validate page[%s], rc:%d",
                lpb->getRuntimeBuffer().getGlobalPid().toString().c_str(), rc);
         goto error;
      }

      rc = getCapacityOfCLMetaBlockPage(lpb->getPageSize(), capacity);
      if (OSS_UNLIKELY(SDB_OK != rc))
      {
         PD_LOG(PDERROR, "failed to get capacity of cl record page:%d", rc);
         goto error;
      }

      slot = context->getMBID() % capacity;

      rc = lpb->prepareToWrite();
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to prepare to write:%d", rc);
         goto error;
      }

      buffer = lpb->getWritableBodyBuffer();
      SDB_ASSERT(buffer.isWritable(), "must be writable");

      wptr = buffer.getWritableObjPtr<clMetaBlockOnDisk>
             (CL_DISK_META_BLOCK_LEN * slot);
      if (NULL == wptr)
      {
         PD_LOG(PDERROR, "failed to get writable ptr of slot[%d]", slot);
         rc = SDB_VESSEL_INTERNAL_ERR;
         goto error;
      }

      if (!wptr->block.isValid())
      {
         PD_LOG(PDERROR, "invalid record on disk");
         rc = SDB_VESSEL_INTERNAL_ERR;
         goto error;
      }

      oldBlock = wptr->block;
      rc = prepareUpdateLog(context, &(lpb->getRuntimeBuffer()), &lrc);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to prepare dps log:%d", rc);
         goto error;
      }

      for (UINT32 i = 0; i < COLLECTION_ROUTE_PAGE_SLOT_COUNT; ++i)
      {
         wptr->block.routePages[i] = INVALID_PAGE_ID;
      }

      rc = commitUpdateLog(context, &lrc, lpb->getRuntimeBuffer().getGlobalPid(),
                           lpb->getLogicalPid(),
                           COLLECTION_UPDATE_MASK_ROUTE_PAGES,
                           oldBlock, wptr->block);
      if (SDB_OK != rc)
      {
         PD_LOG(PDSEVERE, "failed to commit log[%lld], rc:%d",
                lrc.getLsn(), rc);
         ossPanic();
         for (UINT32 i = 0; i < COLLECTION_ROUTE_PAGE_SLOT_COUNT; ++i)
         {
            wptr->block.routePages[i] = oldBlock.routePages[i];
         }
         goto error;
      }

      lpb->commit(lrc.getLsn());
   done:
      return rc;
   error:
      if (lrc.prepared())
      {
         pageAccessor::abortLog(context, &lrc);
      }
      goto done;
   }

/*
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
      rc = lpb->validatePage(PAGE_TYPE_CL_META);
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
   */

   const clMetaBlockOnDisk *clMetaBlockPageAccessor::getReadableDiskBlockPtr(const runtimePageBuffer *rpb,
                                                                             UINT32 i)
   {
      SDB_ASSERT(NULL != rpb && rpb->isValid(), "can not be null");
      UINT32 offset = CL_DISK_META_BLOCK_LEN * i;
      return rpb->getReadableBodyBuffer().getReadableObjPtr<clMetaBlockOnDisk>(offset);
   }

   INT32 clMetaBlockPageAccessor::prepareCreateCLLog(requestContext *context,
                                                     UINT32 adjunctSize,
                                                     const runtimePageBuffer *rpb,
                                                     logRecordContext *lrc)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(NULL != context, "can not be null");
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

   INT32 clMetaBlockPageAccessor::commitCreateCLLog(requestContext *context,
                                                    const GLOBAL_PAGE_ID &gpid,
                                                    const clMetaBlock &block,
                                                    const slice &adjunct,
                                                    logRecordContext *lrc)
   {
      INT32 rc = SDB_OK;
      rc = pageAccessor::pushElement(context, DPS_LOG_PUBLIC_VESSEL_GPID,
                                     sizeof(GLOBAL_PAGE_ID), &gpid, lrc);
      if (SDB_OK != rc)
      {
         goto error;
      }
      rc = pageAccessor::pushElement(context, DPS_LOG_CLCRT_VESSEL_MBID,
                                     sizeof(CL_MB_ID), &(block.mbID), lrc);
      if (SDB_OK != rc)
      {
         goto error;
      }
      rc = pageAccessor::pushElement(context, DPS_LOG_CLCRT_VESSEL_MBID,
                                     sizeof(UINT32), &(block.innerID), lrc);
      if (SDB_OK != rc)
      {
         goto error;
      }
      rc = pageAccessor::pushElement(context, DPS_LOG_CLCRT_VESSEL_LOGICAL_ID,
                                     sizeof(UINT32), &(block.logicalCLID), lrc);
      if (SDB_OK != rc)
      {
         goto error;
      }
      rc = pageAccessor::pushElement(context, DPS_LOG_CLCRT_VESSEL_ADJUNCT,
                                     adjunct.getSize(), adjunct.data(), lrc);
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

   INT32 clMetaBlockPageAccessor::prepareUpdateLog(requestContext *context,
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
      lrc->prepush(CL_META_BLOCK_LEN);
      lrc->prepush(CL_META_BLOCK_LEN);

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

   INT32 clMetaBlockPageAccessor::commitUpdateLog(requestContext *context,
                                                  logRecordContext *lrc,
                                                  const GLOBAL_PAGE_ID &gpid,
                                                  PAGE_ID lpid,
                                                  UINT64 mask,
                                                  const clMetaBlock &oldBlock,
                                                  const clMetaBlock &newBlock)
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
                                     CL_META_BLOCK_LEN, &oldBlock, lrc);
      if (SDB_OK != rc)
      {
         goto error;
      }
      rc = pageAccessor::pushElement(context, DPS_LOG_VESSEL_CL_RECORD_UPDATE_NEW,
                                     CL_META_BLOCK_LEN, &oldBlock, lrc);
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

   INT32 clMetaBlockPageAccessor::commitRemoveLog(requestContext *context,
                                                  const runtimePageBuffer *rpb,
                                                  DPS_LSN_OFFSET &lsn)
   {
      INT32 rc = SDB_OK;
      logRecordContext lrc;
      rc = pageAccessor::prepareLog(context, rpb,
                                    LOG_TYPE_CL_DELETE,
                                    FALSE, &lrc);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to prepare log:%d", rc);
         goto error;
      }

      lrc.setDDL();

      rc = pageAccessor::prepareLogDone(context, &lrc);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to prepare log done:%d", rc);
         goto error;
      }

      rc = pageAccessor::commitLog(context, &lrc);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to commit log[%lld], rc:%d", lrc.getLsn(), rc);
         goto error;
      }

      lsn = lrc.getLsn();
   done:
      return rc;
   error:
      goto done;
   }

}//namespace vessel
}//namespace engine