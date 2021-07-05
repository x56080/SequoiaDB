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

   Source File Name = impAccessor.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#include "vessel/impAccessor.h"
#include "vessel/idMapPage.h"
#include "ossLikely.hpp"
#include "pdTrace.hpp"
#include "vessel/requestContext.h"
#include "vessel/IRedoLogger.h"
#include "vessel/logRecordContext.h"
#include "dpsLogRecordDef.hpp"
#include "vessel/outerResource.h"
#include "vessel/bitMapUtils.h"

namespace engine
{
namespace vessel
{
   impAccessor::impAccessor()
   {

   }

   impAccessor::~impAccessor()
   {

   }

   INT32 impAccessor::getPidByOffset(UINT32 offset,
                                     PAGE_ID *pid,
                                     PAGE_SNAPSHOT_VERION *psv)const
   {
      INT32 rc = SDB_OK;
      UINT32 capacity = 0;
      const idMapSlot *slots = NULL;

      if (OSS_UNLIKELY(!isOpen()))
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }

      if (OSS_UNLIKELY(!get64AlignedIMPCapacity(getRuntimeBuffer()->getPageSize(), capacity)))
      {
         PD_LOG(PDERROR, "failed to get imp capacity");
         rc = SDB_VESSEL_INTERNAL_ERR;
         goto error;
      }

      if (capacity <= offset)
      {
         rc = SDB_OUT_OF_BOUND;
         goto error;
      }

      slots = (const idMapSlot *)
              (getRuntimeBuffer()->getReadablePtrOfBody(ID_MAP_PAGE_HEAD_SIZE, capacity * sizeof(idMapSlot)));
      if (NULL == slots)
      {
         PD_LOG(PDERROR, "failed to get slots ptr");
         rc = SDB_VESSEL_INTERNAL_ERR;
         goto error;
      }

      if (NULL != pid)
      {
         *pid = slots[offset].pid;
      }
      if (NULL != psv)
      {
         *psv = slots[offset].psv;
      }
   done:
      return rc;
   error:
      goto done;
   }

   INT32 impAccessor::validateNewMapping(requestContext *context,
                                         UINT32 count,
                                         const PAGE_ID *lpids,
                                         const PAGE_ID *pids,
                                         PAGE_SNAPSHOT_VERION psv)const
   {
      INT32 rc = SDB_OK;
      const idMapPageHead *head = NULL;
      const idMapSlot *slots = NULL;
      UINT32 slotsBufSize = 0;
      UINT32 capacity = 0;
      UINT32 factor = 0;
      UINT32 pageSize = 0;

      if (OSS_UNLIKELY(NULL == context ||
                       0 == count ||
                       NULL == lpids ||
                       NULL == pids ||
                       INVALID_PAGE_SNAPSHOT_VERSION == psv))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      
      head = getRuntimeBuffer()->getReadablePtrOfBody<idMapPageHead>(0);
      if (OSS_UNLIKELY(NULL == head))
      {
         PD_LOG(PDERROR, "failed to get imp head ptr");
         rc = SDB_VESSEL_INTERNAL_ERR;
         goto error;
      }

      if (!head->isValid())
      {
         PD_LOG(PDERROR, "invalid imp head of pid[%s]",
                getRuntimeBuffer()->getGlobalPid().toString().c_str());
         rc = SDB_VESSEL_PAGE_CRASHED;
         goto error;
      }

      pageSize = getRuntimeBuffer()->getPageSize();
      if (OSS_UNLIKELY(!get64AlignedIMPCapacity(pageSize, capacity)))
      {
         PD_LOG(PDERROR, "failed to get imp capacity");
         rc = SDB_VESSEL_INTERNAL_ERR;
         goto error;
      }

      slotsBufSize = capacity * sizeof(idMapSlot);

      slots = (const idMapSlot *)
              (getRuntimeBuffer()->getReadablePtrOfBody(ID_MAP_PAGE_HEAD_SIZE, slotsBufSize));
      if (NULL == slots)
      {
         PD_LOG(PDERROR, "failed to get slots ptr");
         rc = SDB_VESSEL_INTERNAL_ERR;
         goto error;
      }

      for (UINT32 i = 0; i < count; ++i)
      {
         UINT32 offset = lpids[i] % capacity;
         if (OSS_UNLIKELY(INVALID_PAGE_ID == lpids[i] ||
                          INVALID_PAGE_ID == pids[i]))
         {
            rc = SDB_INVALIDARG;
            goto error;
         }
         if (!slots[offset].isFree())
         {
            PD_LOG(PDERROR, "lpid[%d] is not free", lpids[i]);
            rc = SDB_VESSEL_OPERATOION_NOT_PERMITTED;
            goto error;
         }
      }

      factor = lpids[0] / capacity;
      for (UINT32 i = 1; i < count; ++i)
      {
         if (factor != (lpids[i] / capacity))
         {
            PD_LOG(PDERROR, "factors should be same");
            rc = SDB_VESSEL_OPERATOION_NOT_PERMITTED;
            goto error;
         }
      }
      
   done:
      return rc;
   error:
      goto done;
   }

   INT32 impAccessor::mapNewLpids(requestContext *context,
                                  UINT32 count,
                                  const PAGE_ID *lpids,
                                  const PAGE_ID *pids,
                                  PAGE_SNAPSHOT_VERION psv)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(isOpen(), "must be open");

      if (OSS_UNLIKELY(!isOpen()))
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }

      rc = validateNewMapping(context, count, lpids, pids, psv);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to validate new mapping:%d", rc);
         goto error;
      }

      rc = _mapNewLpids(context, count, lpids, pids, psv);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to map new lpids:%d", rc);
         goto error;
      }
      
   done:
      return rc;
   error:
      goto done;
   }

   INT32 impAccessor::_mapNewLpids(requestContext *context,
                                   UINT32 count,
                                   const PAGE_ID *lpids,
                                   const PAGE_ID *pids,
                                   PAGE_SNAPSHOT_VERION psv)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(NULL != context, "can not be null");
      SDB_ASSERT(0 < count, "can not be zero");
      SDB_ASSERT(NULL != lpids, "can not be null");
      SDB_ASSERT(NULL != pids, "can not be null");
      SDB_ASSERT(INVALID_PAGE_SNAPSHOT_VERSION != psv, "can not be invalid");

      logRecordContext lrc;
      idMapSlot *slots = NULL;
      UINT32 capacity = 0;

      rc = getRuntimeBuffer()->prepareToWrite(context);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to get runtime buffer writable:%d", rc);
         goto error;
      }

      if (OSS_UNLIKELY(!get64AlignedIMPCapacity(getRuntimeBuffer()->getPageSize(), capacity)))
      {
         PD_LOG(PDERROR, "failed to get imp capacity");
         rc = SDB_VESSEL_INTERNAL_ERR;
         goto error;
      }

      if (getRuntimeBuffer()->isCacheBuffer())
      {
         rc = prepareRemapLog(context, &lrc, count, 0);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to prepare mapping log:%d", rc);
            goto error;
         }
      }

      slots = (idMapSlot *)
              getRuntimeBuffer()->getWritablePtrOfBody(ID_MAP_PAGE_HEAD_SIZE,
                                                       capacity * sizeof(idMapSlot));
      if (NULL == slots)
      {
         PD_LOG(PDERROR, "failed to get slots writable ptr");
         rc = SDB_VESSEL_INTERNAL_ERR;
         goto error;
      }

      for (UINT32 i = 0; i < count; ++i)
      {
         UINT32 offset = lpids[i] % capacity;
         SDB_ASSERT(slots[offset].isFree(), "impossible");
         slots[offset].pid = pids[i];
         slots[offset].psv = psv;
      }

      getRuntimeBuffer()->commit(lrc.getLsn());
      if (lrc.prepared())
      {
         commitRemapLog(context, &lrc, count, lpids, pids, psv, NULL);
         lrc.close();
      }
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

   INT32 impAccessor::dumpToBitmap(requestContext *context,
                                   inMemBitMap &bitmap)const
   {
      INT32 rc = SDB_OK;
      const idMapPageHead *head = NULL;
      const runtimePageBuffer *rpb = NULL;
      const idMapSlot *slots = NULL;
      CHAR *buffer = NULL;
      UINT32 bufferSize = 0;
      UINT32 capacity = 0;
      UINT32 bitsCount = 0;

      if (OSS_UNLIKELY(NULL == context ||
                       !bitmap.isInitialized()))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      else if (OSS_UNLIKELY(!isOpen()))
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }

      rpb = getRuntimeBuffer();

      if (OSS_UNLIKELY(!get64AlignedIMPCapacity(rpb->getPageSize(), capacity)))
      {
         PD_LOG(PDERROR, "failed to get imp capacity");
         rc = SDB_VESSEL_INTERNAL_ERR;
         goto error;
      }

      if (bitmap.getPageCapacity() != capacity)
      {
         PD_LOG(PDERROR, "capacity not same[%d,%d]",
                 bitmap.getPageCapacity(), capacity);
         rc = SDB_VESSEL_INTERNAL_ERR;
         goto error;
      }

      bitsCount = capacity >> 6;
      bufferSize = capacity >> 3;

      
      head = rpb->getReadablePtrOfBody<idMapPageHead>(0);
      if (NULL == head)
      {
         PD_LOG(PDERROR, "failed to get imp head");
         rc = SDB_VESSEL_INTERNAL_ERR;
         goto error;
      }
      else if (!head->isValid())
      {
         PD_LOG(PDERROR, "invalid imp head");
         rc = SDB_VESSEL_PAGE_CRASHED;
         goto error;
      }
      
      slots = (const idMapSlot *)
              (rpb->getReadablePtrOfBody(ID_MAP_PAGE_HEAD_SIZE, (capacity * sizeof(idMapSlot))));
      if (NULL == slots)
      {
         PD_LOG(PDERROR, "failed to get slots ptr");
         rc = SDB_VESSEL_INTERNAL_ERR;
         goto error;
      }

      buffer = context->allocateBuffer(bufferSize);
      if (NULL == buffer)
      {
         PD_LOG(PDERROR, "failed to allocate mem");
         rc = SDB_OOM;
         goto error;
      }

      resetBitMap64(bitsCount, (UINT64 *)buffer, TRUE);
      for (UINT32 i = 0; i < capacity; ++i)
      {
         const idMapSlot &slot = slots[i];
         if (!slot.isFree())
         {
            if (!setNotFreeIfFree64(bitsCount, (UINT64 *)buffer, i))
            {
               PD_LOG(PDERROR, "failed to set bit[%d] not free", i);
               rc = SDB_VESSEL_INTERNAL_ERR;
               goto error;
            }
         }
      }

      rc = bitmap.mapNewBitPage(capacity, (const UINT64 *)buffer);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to map bitmap:%d", rc);
         goto error;
      }
      
   done:
      if (NULL != buffer)
      {
         context->releaseBuffer(buffer, bufferSize);
      }
      return rc;
   error:
      goto done;
   }

   INT32 impAccessor::prepareRemapLog(requestContext *context,
                                      logRecordContext *lrc,
                                      UINT32 count,
                                      BOOLEAN hasOldSlots)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(NULL != lrc, "can not be null");
      SDB_ASSERT(!lrc->prepared(), "can not be prepared");
      SDB_ASSERT(0 < count, "can not be zero");
      dpsLogRecordHeader *head = NULL;

      head = &(lrc->getHead());
      head->_type = LOG_TYPE_VESSEL_REMAP_LPID;

      lrc->prepush(sizeof(GLOBAL_PAGE_ID));
      lrc->prepush(sizeof(UINT32));
      lrc->prepush(sizeof(PAGE_SNAPSHOT_VERION));
      lrc->prepush(count * sizeof(PAGE_ID));
      lrc->prepush(count * sizeof(PAGE_ID));

      if (hasOldSlots)
      {
         lrc->prepush(count * sizeof(idMapSlot));
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

   INT32 impAccessor::commitRemapLog(requestContext *context,
                                    logRecordContext *lrc,
                                    UINT32 count,
                                    const PAGE_ID *lpids,
                                    const PAGE_ID *pids,
                                    PAGE_SNAPSHOT_VERION psv,
                                    const idMapSlot *oldSlots)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(isOpen(), "can not be closed");
      SDB_ASSERT(NULL != lrc, "can not be null");
      SDB_ASSERT(lrc->prepared(), "must be prepared");
      SDB_ASSERT(0 < count, "can not be zero");
      ISession *session = context->getSession();
      IRedoLogger *logger = context->getOuterResource()->logger;

      rc = logger->pushLogRecordElement(session, lrc,
                                        DPS_LOG_PUBLIC_VESSEL_GPID,
                                        sizeof(GLOBAL_PAGE_ID),
                                        &(getRuntimeBuffer()->getGlobalPid()));
      if (OSS_UNLIKELY(SDB_OK != rc))
      {
         goto error;
      }

      rc = logger->pushLogRecordElement(session, lrc,
                                        DPS_LOG_VESSEL_IMP_REMAP_COUNT,
                                        sizeof(UINT32),
                                        &count);
      if (OSS_UNLIKELY(SDB_OK != rc))
      {
         goto error;
      }

      rc = logger->pushLogRecordElement(session, lrc,
                                        DPS_LOG_VESSEL_IMP_REMAP_PSV,
                                        sizeof(PAGE_SNAPSHOT_VERION),
                                        &psv);
      if (OSS_UNLIKELY(SDB_OK != rc))
      {
         goto error;
      }

      rc = logger->pushLogRecordElement(session, lrc,
                                        DPS_LOG_VESSEL_IMP_REMAP_LPIDS,
                                        sizeof(PAGE_ID) * count,
                                        lpids);
      if (OSS_UNLIKELY(SDB_OK != rc))
      {
         goto error;
      }

      rc = logger->pushLogRecordElement(session, lrc,
                                        DPS_LOG_VESSEL_IMP_REMAP_PIDS,
                                        sizeof(PAGE_ID) * count,
                                        pids);
      if (OSS_UNLIKELY(SDB_OK != rc))
      {
         goto error;
      }

      if (NULL != oldSlots)
      {
         rc = logger->pushLogRecordElement(session, lrc,
                                        DPS_LOG_VESSEL_IMP_REMAP_OLD,
                                        sizeof(idMapSlot) * count,
                                        oldSlots);
         if (OSS_UNLIKELY(SDB_OK != rc))
         {
            goto error;
         }
      }

      rc = pageAccessor::commitLogDone(context, lrc);
      if (SDB_OK != rc)
      {
         goto error;
      }
   done:
      return rc;
   error:
      PD_LOG(PDERROR, "faield to commit log[%lld], rc:%d", lrc->getLsn(), rc);
      SDB_ASSERT(FALSE, "impossible");
      goto done;
   }
}//namespace vessel
}//namespace engine