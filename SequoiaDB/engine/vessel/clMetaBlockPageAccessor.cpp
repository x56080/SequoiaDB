/*******************************************************************************

   Copyright (C) 2011-Present SequoiaDB Ltd.

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

      http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.

   Source File Name = clMetaBlockPageAccessor.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

*******************************************************************************/
#include "vessel/clMetaBlockPageAccessor.h"
#include "vessel/clMetaBlockPage.h"
#include "pdTrace.hpp"
#include "vessel/requestContext.h"
#include "vessel/outerResource.h"
#include "dpsLogRecordDef.hpp"
#include "vessel/logicalPageBuffer.h"
#include "dpsWriteReqBuilder.hpp"

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

      

      blockPtr = buffer.getWritableObjPtr<clMetaBlockOnDisk>
                 (CL_DISK_META_BLOCK_LEN * slot);
      if (NULL == blockPtr)
      {
         PD_LOG(PDERROR, "failed to get writable disk ptr");
         rc = SDB_VESSEL_INTERNAL_ERR;
         goto error;
      }

      rc = writeCreateCLJournal(context, lpb->getGlobalPid(),
                                block, slice(obj.objsize(), obj.objdata()), lsn);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to write journal:%d", rc);
         goto error;
      }

      ossMemset(blockPtr, 0, CL_DISK_META_BLOCK_LEN);
      blockPtr->block = block;
      lpb->commit(lsn);

   done:
      return rc;
   error:
      goto done;
   }

   INT32 clMetaBlockPageAccessor::removeCL(requestContext *context,
                                           logicalPageBuffer *lpb)
   {
      INT32 rc = SDB_OK;
      UINT32 slot = 0;
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

      rc = writeRemoveJournal(context, lpb->getGlobalPid(), lsn);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to write journal:%d", rc);
         goto error;
      }

      ossMemset(blockPtr, 0, CL_DISK_META_BLOCK_LEN);
      blockPtr->block.reset();

      lpb->commit(lsn);
   done:
      return rc;
   error:
      goto done;
   }

   INT32 clMetaBlockPageAccessor::updateRoutePages(requestContext *context,
                                                   const PAGE_ID *pages,
                                                   UINT32 count,
                                                   logicalPageBuffer *lpb)
   {
      INT32 rc = SDB_OK;
      UINT32 slot = 0;
      clMetaBlockOnDisk *wptr = NULL;
      clMetaBlock oldBlock;
      UINT32 capacity = 0;
      strictBuffer buffer;
      DPS_LSN_OFFSET lsn = DPS_INVALID_LSN_OFFSET;
      
      if (OSS_UNLIKELY(NULL == context ||
                       !context->isClPropertiesSet() ||
                       NULL == lpb ||
                       !lpb->isValid() ||
                       0 == count ||
                       COLLECTION_ROUTE_PAGE_SLOT_COUNT < count ||
                       nullptr == pages))
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
      for (UINT32 i = 0; i < count; ++i)
      {
         wptr->block.routePages[i] = pages[i];
      }

      rc = writeUpdateJournal(context, lpb->getGlobalPid(),
                              COLLECTION_UPDATE_MASK_ROUTE_PAGES,
                              oldBlock, wptr->block, lsn);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to write journal:%d", rc);
         goto error;
      }

      lpb->commit(lsn);
   done:
      return rc;
   error:
      goto done;
   }

   INT32 clMetaBlockPageAccessor::truncateRouteMap(requestContext *context,
                                                   logicalPageBuffer *lpb)
   {
      INT32 rc = SDB_OK;
      UINT32 slot = 0;
      clMetaBlockOnDisk *wptr = NULL;
      clMetaBlock oldBlock;
      UINT32 capacity = 0;
      strictBuffer buffer;
      PAGE_ID backup[COLLECTION_ROUTE_PAGE_SLOT_COUNT];
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
      ossMemcpy(backup, oldBlock.routePages, sizeof(backup));

      for (UINT32 i = 0; i < COLLECTION_ROUTE_PAGE_SLOT_COUNT; ++i)
      {
         wptr->block.routePages[i] = INVALID_PAGE_ID;
      }

      rc = writeUpdateJournal(context, lpb->getGlobalPid(),
                              COLLECTION_UPDATE_MASK_ROUTE_PAGES,
                              oldBlock, wptr->block, lsn);
      if (SDB_OK != rc)
      {
         ossMemcpy(wptr->block.routePages, backup, sizeof(backup));
         PD_LOG(PDERROR, "failed to write journal:%d", rc);
         goto error;
      }

      lpb->commit(lsn);
   done:
      return rc;
   error:
      goto done;
   }

   const clMetaBlockOnDisk *clMetaBlockPageAccessor::getReadableDiskBlockPtr(const runtimePageBuffer *rpb,
                                                                             UINT32 i)
   {
      SDB_ASSERT(NULL != rpb && rpb->isValid(), "can not be null");
      UINT32 offset = CL_DISK_META_BLOCK_LEN * i;
      return rpb->getReadableBodyBuffer().getReadableObjPtr<clMetaBlockOnDisk>(offset);
   }

   INT32 clMetaBlockPageAccessor::writeCreateCLJournal(requestContext *context,
                                                       const GLOBAL_PAGE_ID &gpid,
                                                       const clMetaBlock &block,
                                                       const slice &adjunct,
                                                       DPS_LSN_OFFSET &lsn)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(nullptr != context, "can not be null");
      IDataJournal *journal = context->getOuterResource()->journal;
      dpsWriteReqBuilder jpad;
      dpsWriteRequest jrequest;
      dpsLogRecordHeader jres;

      jpad.setType(LOG_TYPE_CL_CRT);

      rc = jpad.append(DPS_LOG_PUBLIC_VESSEL_GPID,
                       GLOBAL_PAGE_ID_SIZE,
                       &gpid);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failedt append gpid:%d", rc);
         goto error;
      }

      jrequest = jpad.reap();
      rc = journal->write(context->getExecutor(), jrequest, dpsWriteOptions(), &jres);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to write journal:%d", rc);
         goto error;
      }
      lsn = jres._lsn;
   done:
      return rc;
   error:
      goto done;
   }

   INT32 clMetaBlockPageAccessor::writeUpdateJournal(requestContext *context,
                                                      const GLOBAL_PAGE_ID &gpid,
                                                      UINT64 mask,
                                                      const clMetaBlock &oldBlock,
                                                      const clMetaBlock &newBlock,
                                                      DPS_LSN_OFFSET &lsn)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(nullptr != context, "can not be null");
      IDataJournal *journal = context->getOuterResource()->journal;
      dpsWriteReqBuilder jpad;
      dpsWriteRequest jrequest;
      dpsLogRecordHeader jres;

      jpad.setType(LOG_TYPE_VESSEL_CRP_UPDATE);

      rc = jpad.append(DPS_LOG_PUBLIC_VESSEL_GPID,
                       GLOBAL_PAGE_ID_SIZE,
                       &gpid);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failedt append gpid:%d", rc);
         goto error;
      }

      jrequest = jpad.reap();
      rc = journal->write(context->getExecutor(), jrequest, dpsWriteOptions(), &jres);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to write journal:%d", rc);
         goto error;
      }
      lsn = jres._lsn;
   done:
      return rc;
   error:
      goto done;
   }

   INT32 clMetaBlockPageAccessor::writeRemoveJournal(requestContext *context,
                                                      const GLOBAL_PAGE_ID &gpid,
                                                      DPS_LSN_OFFSET &lsn)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(nullptr != context, "can not be null");
      IDataJournal *journal = context->getOuterResource()->journal;
      dpsWriteReqBuilder jpad;
      dpsWriteRequest jrequest;
      dpsLogRecordHeader jres;

      jpad.setType(LOG_TYPE_CL_DELETE);

      jrequest = jpad.reap();
      rc = journal->write(context->getExecutor(), jrequest, dpsWriteOptions(), &jres);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to write journal:%d", rc);
         goto error;
      }
      lsn = jres._lsn;
   done:
      return rc;
   error:
      goto done;
   }
}//namespace vessel
}//namespace engine