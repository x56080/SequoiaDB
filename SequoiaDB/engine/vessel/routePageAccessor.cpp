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

   Source File Name = routePageAccessor.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

*******************************************************************************/
#include "vessel/routePageAccessor.h"
#include "ossLikely.hpp"
#include "vessel/routePage.h"
#include "dpsLogRecordDef.hpp"
#include "vessel/outerResource.h"
#include "vessel/logicalPageBuffer.h"
#include "vessel/requestContext.h"
#include "vessel/collectionProperties.h"
#include "dpsWriteReqBuilder.hpp"
#include "vessel/outerResource.h"

namespace engine
{
namespace vessel
{
   INT32 routePageAccessor::append(requestContext *context,
                                    UINT32 count,
                                    const PAGE_ID *lpids,
                                    logicalPageBuffer *lpb)
   {
      INT32 rc = SDB_OK;
      UINT32 capacity = 0;
      const routePageHead *readableHead = nullptr;
      routePageHead *writableHead = nullptr;
      UINT32 oldCount = 0;
      UINT32 lid = DMS_INVALID_LOGICCLID;
      strictBuffer buffer;
      DPS_LSN_OFFSET lsn = DPS_INVALID_LSN_OFFSET;

      if (OSS_UNLIKELY(nullptr == context ||
                       !context->isClPropertiesSet() ||
                       0 == count ||
                       nullptr == lpids ||
                       nullptr == lpb ||
                       !lpb->isValid()))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      for (UINT32 i = 0; i < count; ++i)
      {
         if (INVALID_PAGE_ID == lpids[i])
         {
            rc = SDB_INVALIDARG;
            goto error;
         }
      }

      lid = context->getLogicalClId();

      rc = lpb->validatePage(PAGE_TYPE_ROUTE);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to validate page[%s], rc:%d",
                lpb->getGlobalPid().toString().c_str(), rc);
         goto error;
      }

      capacity = getCapacityOfRoutePage(lpb->getPageSize());
      if (OSS_UNLIKELY(0 == capacity))
      {
         PD_LOG(PDERROR, "failed to get capacity of route page");
         rc = SDB_VESSEL_INTERNAL_ERR;
         goto error;
      }

      buffer = lpb->getReadableBodyBuffer();
      readableHead = buffer.getReadableObjPtr<routePageHead>(0);
      if (nullptr == readableHead)
      {
         PD_LOG(PDERROR, "failed to get readable page head");
         rc = SDB_VESSEL_INTERNAL_ERR;
         goto error;
      }

      if (capacity < (readableHead->size + count))
      {
         PD_LOG(PDERROR, "not enough free space");
         rc = SDB_VESSEL_NOT_ENOUGH_FREE_RESOURCE;
         goto error;
      }

      if (lid != readableHead->logicalId)
      {
         PD_LOG(PDERROR, "logicalId[%d] does not match the one on disk[%d]",
                lid, readableHead->logicalId);
         rc = SDB_VESSEL_INTERNAL_ERR;
         goto error;
      }

      rc = lpb->prepareToWrite();
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to prepare to write:%d", rc);
         goto error;
      }

      buffer = lpb->getWritableBodyBuffer();

      writableHead = buffer.getWritableObjPtr<routePageHead>(0);
      if (nullptr == readableHead)
      {
         PD_LOG(PDERROR, "failed to get writable page head");
         rc = SDB_VESSEL_INTERNAL_ERR;
         goto error;
      }

      oldCount = writableHead->size;

      rc = writeJournal(context, lpb->getGlobalPid(),
                        oldCount, count, lpids, lsn);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to write journal:%d", rc);
         goto error;
      }

      for (UINT32 i = 0; i < count; ++i)
      {
         UINT32 offset = ROUTE_PAGE_HEAD_SIZE + ((writableHead->size + i) << 2);
         PAGE_ID *tmp = buffer.getWritableObjPtr<PAGE_ID>(offset);
         SDB_ASSERT(nullptr != tmp, "impossible");
         *tmp = lpids[i];
      }
      writableHead->size += count;

      lpb->commit(lsn);
   done:
      return rc;
   error:
      goto done;
   }

   INT32 routePageAccessor::getSizeAndLast(requestContext *context,
                                           INT32 targetLvl,
                                           const logicalPageBuffer *lpb,
                                           UINT32 &size,
                                           PAGE_ID &last)const
   {
      INT32 rc = SDB_OK;
      const routePageHead *readableHead = nullptr;
      UINT32 offset = 0;
      const PAGE_ID *ptr = nullptr;
      size = 0;
      last = INVALID_PAGE_ID;
      strictBuffer buffer;
      UINT32 clid = DMS_INVALID_LOGICCLID;

      if (OSS_UNLIKELY(nullptr == context ||
                       !isValidRoutePageLvl(targetLvl) ||
                       !context->isClPropertiesSet() ||
                       nullptr == lpb ||
                       !lpb->isValid()))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      rc = lpb->validatePage(PAGE_TYPE_ROUTE);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to validate page[%s], rc:%d",
                lpb->getGlobalPid().toString().c_str(), rc);
         goto error;
      }

      buffer = lpb->getReadableBodyBuffer();

      readableHead = buffer.getReadableObjPtr<routePageHead>(0);
      if (nullptr == readableHead)
      {
         PD_LOG(PDERROR, "failed to get readable head");
         rc = SDB_VESSEL_INTERNAL_ERR;
         goto error;
      }

      clid = context->getLogicalClId();
      if (clid != readableHead->logicalId)
      {
         PD_LOG(PDERROR, "logicalId[%d] does not match the one on disk[%d]",
                clid, readableHead->logicalId);
         rc = SDB_VESSEL_PAGE_HEAD_NOT_MATCH;
         goto error;
      }

      if (readableHead->lvl != targetLvl)
      {
         PD_LOG(PDERROR, "lvl in head[%d] does not match target[%d]",
                readableHead->lvl, targetLvl);
         rc = SDB_VESSEL_PAGE_HEAD_NOT_MATCH;
         goto error;
      }

      if (0 < readableHead->size)
      {
         offset = ROUTE_PAGE_HEAD_SIZE + ((UINT32)(readableHead->size - 1) << 2);
         ptr = buffer.getReadableObjPtr<PAGE_ID>(offset);
         if (nullptr == ptr)
         {
            PD_LOG(PDERROR, "failed to get lpid ptr");
            rc = SDB_VESSEL_INTERNAL_ERR;
            goto error;
         }
         last = *ptr;
      }

      size = readableHead->size;
   done:
      return rc;
   error:
      goto done;
   }

   INT32 routePageAccessor::get(requestContext *context,
                                UINT32 pos,
                                const logicalPageBuffer *lpb,
                                PAGE_ID &lpid)const
   {
      INT32 rc = SDB_OK;
      const routePageHead *readableHead = nullptr;
      UINT32 offset = 0;
      const PAGE_ID *ptr = nullptr;
      strictBuffer buffer;

      if (OSS_UNLIKELY(nullptr == context ||
                       !context->isClPropertiesSet() ||
                       nullptr == lpb ||
                       !lpb->isValid()))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      rc = lpb->validatePage(PAGE_TYPE_ROUTE);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to validate page[%s], rc:%d",
                lpb->getGlobalPid().toString().c_str(), rc);
         goto error;
      }

      buffer = lpb->getReadableBodyBuffer();
      readableHead = buffer.getReadableObjPtr<routePageHead>(0);
      if (nullptr == readableHead)
      {
         PD_LOG(PDERROR, "failed to get readable head");
         rc = SDB_VESSEL_INTERNAL_ERR;
         goto error;
      }

      if (context->getLogicalClId() != readableHead->logicalId)
      {
         PD_LOG(PDERROR, "logicalId[%d] does not match the one on disk[%d]",
                context->getLogicalClId(),
                readableHead->logicalId);
         rc = SDB_VESSEL_PAGE_HEAD_NOT_MATCH;
         goto error;
      }

      if (readableHead->size <= pos)
      {
         PD_LOG(PDERROR, "pos[%d] out of bound[%d]", pos, readableHead->size);
         rc = SDB_OUT_OF_BOUND;
         goto error;
      }
      
      offset = ROUTE_PAGE_HEAD_SIZE + (pos << 2);
      ptr = buffer.getReadableObjPtr<PAGE_ID>(offset);
      if (nullptr == ptr)
      {
         PD_LOG(PDERROR, "failed to get lpid ptr");
         rc = SDB_VESSEL_INTERNAL_ERR;
         goto error;
      }

      if (INVALID_PAGE_ID == *ptr)
      {
         rc = SDB_VESSEL_CL_PAGE_SEQ_NOT_EXISTS;
         goto error;
      }
      lpid = *ptr;
   done:
      return rc;
   error:
      goto done;
   }

   INT32 routePageAccessor::writeJournal(requestContext *context,
                                          const GLOBAL_PAGE_ID &gpid,
                                          UINT16 oldSize,
                                          UINT16 size,
                                          const PAGE_ID *lpids,
                                          DPS_LSN_OFFSET &lsn)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(nullptr != context, "can not be null");
      IDataJournal *journal = context->getOuterResource()->journal;
      dpsWriteReqBuilder jpad;
      dpsWriteRequest jrequest;
      dpsLogRecordHeader jres;

      jpad.setType(LOG_TYPE_VESSEL_ROUTE_PAGE_UPDATE);

      rc = jpad.append(DPS_LOG_PUBLIC_VESSEL_GPID,
                       GLOBAL_PAGE_ID_SIZE,
                       &gpid);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failedt append gpid:%d", rc);
         goto error;
      }

      rc = jpad.append(DPS_LOG_VESSEL_ROUTE_PAGE_INSERT_PAGES,
                       size << 2, lpids);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to append lpids:%d", rc);
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

   INT32 routePageAccessor::validatePage(requestContext *context,
                                         INT32 targetLvl,
                                         const logicalPageBuffer *lpb)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(nullptr != context && context->isClPropertiesSet(), "can not be invalid");
      SDB_ASSERT(isValidRoutePageLvl(targetLvl), "can not be invalid");
      SDB_ASSERT(nullptr != lpb && lpb->isValid(), "can not be invalid");
      const routePageHead *header = nullptr;

      rc = lpb->validatePage(PAGE_TYPE_ROUTE);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to validate page:%d", rc);
         goto error;
      }

      header = lpb->getReadableBodyBuffer().getReadableObjPtr<routePageHead>(0);
      if (header->logicalId != context->getLogicalClId())
      {
         PD_LOG(PDERROR, "different cl logical id found[%d, %d]",
                header->logicalId,
                context->getLogicalClId());
         rc = SDB_VESSEL_PAGE_HEAD_NOT_MATCH;
         goto error;
      }

      if (header->lvl != targetLvl)
      {
         PD_LOG(PDERROR, "invalid lvl found in header[%d, %d]",
                header->lvl, targetLvl);
         rc = SDB_VESSEL_PAGE_HEAD_NOT_MATCH;
         goto error;
      }
   done:
      return rc;
   error:
      goto done;
   }

   INT32 routePageAccessor::dumpValidPages(requestContext *context,
                                           INT32 targetLvl,
                                           logicalPageBuffer *lpb,
                                           ossPoolVector<PAGE_ID> &lpids)
   {
      INT32 rc = SDB_OK;

      const routePageHead *header = nullptr;
      UINT32 oldSize = lpids.size();

      if (OSS_UNLIKELY(nullptr == context ||
                       !context->isClPropertiesSet() ||
                       !isValidRoutePageLvl(targetLvl) ||
                       nullptr == lpb || !lpb->isValid()))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      rc = validatePage(context, targetLvl, lpb);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to validate page:%s, rc:%d",
                lpb->getGlobalPid().toString().c_str(), rc);
         goto error;
      }

      header = lpb->getReadableBodyBuffer().getReadableObjPtr<routePageHead>(0);
      if (0 == header->size)
      {
         PD_LOG(PDDEBUG, "empty route page");
         goto done;
      }

      for (UINT32 i = 0; i < (UINT32)header->size; ++i)
      {
         UINT32 offset = ROUTE_PAGE_HEAD_SIZE + (i << 2);
         const PAGE_ID *ptr = lpb->getReadableBodyBuffer().getReadableObjPtr<PAGE_ID>(offset);
         if (OSS_UNLIKELY(nullptr == ptr))
         {
            PD_LOG(PDERROR, "failed to get lpid ptr of pos[%d]", i);
            rc = SDB_VESSEL_INTERNAL_ERR;
            goto error;
         }

         if (INVALID_PAGE_ID != *ptr)
         {
            lpids.push_back(*ptr);
         }
      }
   done:
      return rc;
   error:
      lpids.resize(oldSize);
      goto done;
   }
}//namespace vessel
}//namespace engine