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

   Source File Name = routePageAccessor.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#include "vessel/routePageAccessor.h"
#include "ossLikely.hpp"
#include "vessel/routePage.h"
#include "vessel/logRecordContext.h"
#include "dpsLogRecordDef.hpp"
#include "vessel/outerResource.h"
#include "vessel/logicalPageBuffer.h"
#include "vessel/requestContext.h"

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
      logRecordContext lrc;
      UINT32 capacity = 0;
      const routePageHead *readableHead = NULL;
      routePageHead *writableHead = NULL;
      UINT32 oldCount = 0;
      UINT32 lid = DMS_INVALID_LOGICCLID;
      strictBuffer buffer;

      if (OSS_UNLIKELY(NULL == context ||
                       DMS_INVALID_LOGICCLID == context->getLogicalCLID() ||
                       0 == count ||
                       NULL == lpids ||
                       NULL == lpb ||
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

      lid = context->getLogicalCLID();

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
      if (NULL == readableHead)
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

      rc = prepareAppendLog(context, &(lpb->getRuntimeBuffer()), count, &lrc);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to prepare log:%d", rc);
         goto error;
      }

      buffer = lpb->getWritableBodyBuffer();

      writableHead = buffer.getWritableObjPtr<routePageHead>(0);
      if (NULL == readableHead)
      {
         PD_LOG(PDERROR, "failed to get writable page head");
         rc = SDB_VESSEL_INTERNAL_ERR;
         goto error;
      }

      oldCount = writableHead->size;
      for (UINT32 i = 0; i < count; ++i)
      {
         UINT32 offset = ROUTE_PAGE_HEAD_SIZE + ((writableHead->size + i) << 2);
         PAGE_ID *tmp = buffer.getWritableObjPtr<PAGE_ID>(offset);
         if (NULL == tmp)
         {
            PD_LOG(PDERROR, "failed to get writable ptr");
            rc = SDB_VESSEL_INTERNAL_ERR;
            goto error;
         }
         *tmp = lpids[i];
      }
      writableHead->size += count;

      rc = commitAppendLog(context, lpb->getGlobalPid(),
                           lpb->getLogicalPid(), oldCount,
                           count, lpids, &lrc);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to commit log:%d", rc);
         writableHead->size -= count;
         for (UINT32 i = 0; i < count; ++i)
         {
            UINT32 offset = ROUTE_PAGE_HEAD_SIZE + ((writableHead->size + i) << 2);
            PAGE_ID *tmp = buffer.getWritableObjPtr<PAGE_ID>(offset);
            if (NULL == tmp)
            {
               PD_LOG(PDERROR, "failed to get writable ptr");
            }
            *tmp = INVALID_PAGE_ID;
         }
         ossPanic();
         goto error;
      }

      lpb->commit(lrc.getLsn());
      lrc.close();
   done:
      return rc;
   error:
      if (lrc.prepared())
      {
         pageAccessor::abortLog(context, &lrc);
      }
      goto done;
   }

   INT32 routePageAccessor::getSizeAndLast(requestContext *context,
                                           INT32 targetLvl,
                                           const logicalPageBuffer *lpb,
                                           UINT32 &size,
                                           PAGE_ID &last)const
   {
      INT32 rc = SDB_OK;
      const routePageHead *readableHead = NULL;
      UINT32 offset = 0;
      const PAGE_ID *ptr = NULL;
      size = 0;
      last = INVALID_PAGE_ID;
      strictBuffer buffer;

      if (OSS_UNLIKELY(NULL == context ||
                       !isValidRoutePageLvl(targetLvl) ||
                       DMS_INVALID_LOGICCLID == context->getLogicalCLID() ||
                       NULL == lpb ||
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
      if (NULL == readableHead)
      {
         PD_LOG(PDERROR, "failed to get readable head");
         rc = SDB_VESSEL_INTERNAL_ERR;
         goto error;
      }

      if (context->getLogicalCLID() != readableHead->logicalId)
      {
         PD_LOG(PDERROR, "logicalId[%d] does not match the one on disk[%d]",
                context->getLogicalCLID(), readableHead->logicalId);
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
         if (NULL == ptr)
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
      const routePageHead *readableHead = NULL;
      UINT32 offset = 0;
      const PAGE_ID *ptr = NULL;
      strictBuffer buffer;

      if (OSS_UNLIKELY(NULL == context ||
                       DMS_INVALID_LOGICCLID == context->getLogicalCLID() ||
                       NULL == lpb ||
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
      if (NULL == readableHead)
      {
         PD_LOG(PDERROR, "failed to get readable head");
         rc = SDB_VESSEL_INTERNAL_ERR;
         goto error;
      }

      if (context->getLogicalCLID() != readableHead->logicalId)
      {
         PD_LOG(PDERROR, "logicalId[%d] does not match the one on disk[%d]",
                context->getLogicalCLID(), readableHead->logicalId);
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
      if (NULL == ptr)
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

   INT32 routePageAccessor::prepareAppendLog(requestContext *context,
                                             const runtimePageBuffer *rpb,
                                             UINT32 count,
                                             logRecordContext *lrc)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(NULL != context, "can not be null");
      SDB_ASSERT(0 != count, "can not be zero");
      SDB_ASSERT(NULL != rpb, "can not be null");
      SDB_ASSERT(NULL != lrc, "can not be null");
      SDB_ASSERT(!lrc->prepared(), "can not be prepared");
      
      rc = pageAccessor::prepareLog(context, rpb,
                                    LOG_TYPE_VESSEL_ROUTE_PAGE_UPDATE,
                                    FALSE, lrc);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to prepare dps log:%d", rc);
         goto error;
      }

      lrc->prepush(sizeof(GLOBAL_PAGE_ID));
      lrc->prepush(sizeof(PAGE_ID));
      lrc->prepush(sizeof(UINT16));
      lrc->prepush(sizeof(UINT16));
      lrc->prepush(count << 2);
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

   INT32 routePageAccessor::commitAppendLog(requestContext *context,
                                             const GLOBAL_PAGE_ID &gpid,
                                             PAGE_ID lpid,
                                             UINT16 oldCount,
                                             UINT16 size,
                                             const PAGE_ID *lpids,
                                             logRecordContext *lrc)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(NULL != context, "can not be null");
      rc = pageAccessor::pushElement(context, DPS_LOG_PUBLIC_VESSEL_GPID,
                                     sizeof(GLOBAL_PAGE_ID), &gpid, lrc);
      if (SDB_OK != rc)
      {
         goto error;
      }

      rc = pageAccessor::pushElement(context, DPS_LOG_VESSEL_ROUTE_PAGE_INSERT_LPID,
                                     sizeof(UINT32), &lpid, lrc);
      if (SDB_OK != rc)
      {
         goto error;
      }

      rc = pageAccessor::pushElement(context, DPS_LOG_VESSEL_ROUTE_PAGE_INSERT_OLD_CNT,
                                     sizeof(UINT16), &oldCount, lrc);
      if (SDB_OK != rc)
      {
         goto error;
      }

      rc = pageAccessor::pushElement(context, DPS_LOG_VESSEL_ROUTE_PAGE_INSERT_PAGES,
                                     ((UINT32)size << 2), lpids, lrc);
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