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
#include "vessel/IRedoLogger.h"
#include "dpsLogRecordDef.hpp"
#include "vessel/outerResource.h"

namespace engine
{
namespace vessel
{
   INT32 routePageAccessor::initPage(requestContext *context,
                                     PAGE_ID lpid,
                                     UINT32 logicalId)
   {
      INT32 rc = SDB_OK;
      UINT32 flags = PAGE_ACCESSOR_FLAG_INIT_PAGE | 
                     PAGE_ACCESSOR_FLAG_DIRECT |
                     PAGE_ACCESSOR_FLAG_NON_READONLY;
      SDB_ASSERT(OSS_BIT_TEST(getFlags(), flags), "impossible");
      UINT32 capacity;
      routePageHead *head = NULL;
      CHAR *buffer = NULL;

      if (OSS_UNLIKELY(INVALID_PAGE_ID == lpid ||
                       DMS_INVALID_LOGICCLID == logicalId))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      capacity = getCapacityOfRoutePage(pageAccessor::getPageSize());
      if (OSS_UNLIKELY(0 == capacity))
      {
         PD_LOG(PDERROR, "failed to get capacity of route page");
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

      rc = memsetPageBody(0);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to memset page body:%d", rc);
         goto error;
      }

      rc = getWritableUserHeadPtr<routePageHead>(&head);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to get page head ptr:%d", rc);
         goto error;
      }

      head->version = ROUTE_PAGE_VERSION;
      head->logicalId = logicalId;
      head->count = 0;
      head->pad = 0;

      buffer = (CHAR *)head + ROUTE_PAGE_HEAD_LEN;
      ossMemset(buffer, 0xFF, capacity << 2);
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

   INT32 routePageAccessor::appendSlots(requestContext *context,
                                        UINT32 logicalId,
                                        UINT32 slot,
                                        UINT32 count,
                                        const PAGE_ID *lpids)
   {
      INT32 rc = SDB_OK;
      const routePageHead *rHead = NULL;
      routePageHead *wHead = NULL;
      routePageHead old;
      UINT32 capacity = 0;
      PAGE_ID lpid = INVALID_PAGE_ID;
      const pageHead *head = NULL;
      BOOLEAN rollback = FALSE;
      logRecordContext lrc;

      if (OSS_UNLIKELY(NULL == context ||
                       DMS_INVALID_LOGICCLID == logicalId ||
                       0 == count ||
                       NULL == lpids))
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

      capacity = getCapacityOfRoutePage(pageAccessor::getPageSize());
      if (OSS_UNLIKELY(0 == capacity))
      {
         PD_LOG(PDERROR, "failed to get capacity of route page");
         rc = SDB_VESSEL_INTERNAL_ERR;
         goto error;
      }

      if (capacity <= slot || capacity <= (slot + count))
      {
         PD_LOG(PDERROR, "slot is out of valid range:%d", slot);
         rc = SDB_INVALIDARG;
         goto error;
      }

      rc = getReadPtrOfHead(&head);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to get page of head:%d", rc);
         goto error;
      }

      rc = getReadableUserHeadPtr<routePageHead>(&rHead);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to get route page head:%d", rc);
         goto error;
      }

      if (logicalId != rHead->logicalId)
      {
         PD_LOG(PDERROR, "logicalId[%d] is not as same as id on disk[%d]",
                logicalId, rHead->logicalId);
         rc = SDB_VESSEL_INTERNAL_ERR;
         goto error;
      }

      if (slot != (UINT32)(rHead->count))
      {
         PD_LOG(PDERROR, "count in head is %d, can not append from :%d",
                rHead->count, slot);
         rc = SDB_VESSEL_INTERNAL_ERR;
         goto error;
      }

      rc = prpareAppendLog(context, &lrc, count);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to prepare log:%d", rc);
         goto error;
      }

      rc = prepareToWrite(context);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to prepare to write:%d", rc);
         goto error;
      }

      rollback = TRUE;
      lpid = head->pageID;
      old = *rHead;

      rc = writeSlots(slot, count, lpids);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to write slots, being[%d], rc:%d", slot, rc);
         goto error;
      }
      rollback = TRUE;
      wHead->count += count;

      rc = commitAppendLog(context, &lrc, lpid, old, *wHead, count, lpids);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to commit log[%lld], rc:%d", lrc.getLsn(), rc);
         goto error;
      }

      pageAccessor::commit(context, lrc.getLsn());
      lrc.close();
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
         wHead->count -= count;
         for (UINT32 i = 0; i < count; ++i)
         {
            writeSlots(slot + i, 1, lpids + i);
         }
         abortToWrite();
      }
      if (fullAccessing())
      {
         abortToWrite();
      }
      goto done;
   }

   INT32 routePageAccessor::readSlot(requestContext *context,
                                     UINT32 slot,
                                     PAGE_ID &lpid)
   {
      INT32 rc = SDB_OK;
      UINT32 capacity = getCapacityOfRoutePage(pageAccessor::getPageSize());
      if (OSS_UNLIKELY(0 == capacity))
      {
         PD_LOG(PDERROR, "failed to get capacity of route page");
         rc = SDB_VESSEL_INTERNAL_ERR;
         goto error;
      }

      if (capacity <= slot)
      {
         PD_LOG(PDERROR, "slot is out of valid range:%d", slot);
         rc = SDB_INVALIDARG;
         goto error;
      }

      rc = readSlot(slot, lpid);
      if (SDB_OK != rc)
      {
         goto error;
      }
   done:
      return rc;
   error:
      goto done;
   }

   INT32 routePageAccessor::readSlot(UINT32 slot, PAGE_ID &lpid)
   {
      INT32 rc = SDB_OK;
      PAGE_ID tmp = INVALID_PAGE_ID;
      UINT32 offset = ROUTE_PAGE_HEAD_LEN + (slot << 2);
      rc = readPageBody(offset, sizeof(PAGE_ID), (CHAR*)(&tmp));
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to read page body[%d:%d], rc:%d",
                offset, sizeof(PAGE_ID), rc);
         goto error;
      }
      lpid = tmp;
   done:
      return rc;
   error:
      goto done;
   }

   INT32 routePageAccessor::writeSlots(UINT32 slot,
                                       UINT32 count,
                                       const PAGE_ID *lpids)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(NULL != lpids, "can not be invalid");
      UINT32 offset = ROUTE_PAGE_HEAD_LEN + (slot << 2);
      rc = writePageBody(offset, (count << 2), (const CHAR*)lpids);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to read page body[%d:%d], rc:%d",
                offset, sizeof(PAGE_ID), rc);
         goto error;
      }
   done:
      return rc;
   error:
      goto done;
   }

   INT32 routePageAccessor::prpareAppendLog(requestContext *context,
                                            logRecordContext *lrc,
                                            UINT32 count)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(NULL != context, "can not be null");
      SDB_ASSERT(NULL != lrc, "can not be null");
      SDB_ASSERT(!lrc->prepared(), "can not be prepared");

      dpsLogRecordHeader &head = lrc->getHead();
      head._type = LOG_TYPE_VESSEL_UPDATE_ROUTE_PAGE;

      lrc->prepush(sizeof(GLOBAL_PAGE_ID));
      lrc->prepush(sizeof(PAGE_ID));
      lrc->prepush(ROUTE_PAGE_HEAD_LEN);
      lrc->prepush(ROUTE_PAGE_HEAD_LEN);
      lrc->prepush(count << 2);
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

   INT32 routePageAccessor::commitAppendLog(requestContext *context,
                                            logRecordContext *lrc,
                                            PAGE_ID lpid,
                                            const routePageHead &oldHead,
                                            const routePageHead &newHead,
                                            UINT32 count,
                                            const PAGE_ID *lpids)
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
                                        DPS_LOG_VESSEL_UPDATE_ROUTE_PAGE_LPID,
                                        sizeof(PAGE_ID),
                                        &lpid);
      if (OSS_UNLIKELY(SDB_OK != rc))
      {
         goto error;
      }

      rc = logger->pushLogRecordElement(session, lrc,
                                        DPS_LOG_VESSEL_UPDATE_ROUTE_PAGE_OLD_HEAD,
                                        ROUTE_PAGE_HEAD_LEN,
                                        &oldHead);
      if (OSS_UNLIKELY(SDB_OK != rc))
      {
         goto error;
      }

      rc = logger->pushLogRecordElement(session, lrc,
                                        DPS_LOG_VESSEL_UPDATE_ROUTE_PAGE_NEW_HEAD,
                                        ROUTE_PAGE_HEAD_LEN,
                                        &newHead);
      if (OSS_UNLIKELY(SDB_OK != rc))
      {
         goto error;
      }

      rc = logger->pushLogRecordElement(session, lrc,
                                        DPS_LOG_VESSEL_UPDATE_ROUTE_PAGE_NEW_VALUES,
                                        count << 2,
                                        lpids);
      if (OSS_UNLIKELY(SDB_OK != rc))
      {
         goto error;
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