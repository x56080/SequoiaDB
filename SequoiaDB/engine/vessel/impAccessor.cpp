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

   INT32 impAccessor::initPage(requestContext *context)
   {
      INT32 rc = SDB_OK;
      idMapPageHead *head = NULL;
      UINT32 capacity = 0;
      CHAR *ptr = NULL;
      UINT32 totalSlotSize = 0;
      UINT32 flags = PAGE_ACCESSOR_FLAG_DIRECT |
                     PAGE_ACCESSOR_FLAG_NON_READONLY |
                     PAGE_ACCESSOR_FLAG_INIT_PAGE;
      SDB_ASSERT(OSS_BIT_TEST(getFlags(), flags), "impossible");

      rc = get64AlignedCapacityOfIMP(getPageSize(), capacity);
      if (SDB_OK != rc)
      {
         goto error;
      }

      rc = prepareToWrite(context);
      if (SDB_OK != rc)
      {
         goto error;
      }

      rc = initCommonPageHeadAndTail();
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

      rc = getWritableUserHeadPtr<idMapPageHead>(&head);
      if (SDB_OK != rc)
      {
         goto error;
      }

      ossMemset(head, 0, ID_MAP_PAGE_HEAD_LEN);
      head->version = CURRENT_ID_MAP_PAGE_VERSION;

      totalSlotSize = capacity * sizeof(idMapSlot);
      getWritePtrOfPageBody(ID_MAP_PAGE_HEAD_LEN, totalSlotSize, &ptr);
      ossMemset(ptr, 0xff, totalSlotSize);

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

   INT32 impAccessor::getPidByOffset(UINT32 offset, PAGE_ID *pid, SNAPSHOT_ID *snapID)
   {
      INT32 rc = SDB_OK;
      idMapSlot slot;
      UINT32 pageSize = pageAccessor::getPageSize();
      UINT32 capacity = 0;

      rc = get64AlignedCapacityOfIMP(pageSize, capacity);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to get imp capacity:%d", rc);
         goto error;
      }

      if (capacity <= offset)
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      rc = getSlot(offset, slot);
      if (SDB_OK != rc)
      {
         goto error;
      }

      if (NULL != pid)
      {
         *pid = slot.page;
      }
      if (NULL != snapID)
      {
         *snapID = slot.snapshot;
      }
   done:
      return rc;
   error:
      goto done;
   }

   INT32 impAccessor::getPidByLpid(PAGE_ID lpid, PAGE_ID *pid, SNAPSHOT_ID *snapID)
   {
      INT32 rc = SDB_OK;
      idMapSlot slot;
      UINT32 pageSize = pageAccessor::getPageSize();
      UINT32 capacity = 0;
      UINT32 offset = 0;

      if (OSS_UNLIKELY(INVALID_PAGE_ID == lpid))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      rc = get64AlignedCapacityOfIMP(pageSize, capacity);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to get imp capacity:%d", rc);
         goto error;
      }

      offset = lpid % capacity;

      rc = getSlot(offset, slot);
      if (SDB_OK != rc)
      {
         goto error;
      }

      if (NULL != pid)
      {
         *pid = slot.page;
      }
      if (NULL != snapID)
      {
         *snapID = slot.snapshot;
      }
   done:
      return rc;
   error:
      goto done;
   }


   INT32 impAccessor::validateMap(UINT32 capacity,
                                  UINT32 count,
                                  const PAGE_ID *lpids,
                                  const PAGE_ID *pids,
                                  SNAPSHOT_ID snap)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(0 < capacity, "can not be invalid");

      if (0 == count)
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      else if (NULL == lpids || NULL == pids)
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      else if (INVALID_SNAPSHOT_ID == snap)
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      for (UINT32 i = 0; i < count; ++i)
      {
         idMapSlot slot;
         UINT32 offset = 0;
         if (INVALID_PAGE_ID == lpids[i] || INVALID_PAGE_ID == pids[i])
         {
            PD_LOG(PDERROR, "invalid page id");
            rc = SDB_INVALIDARG;
            goto error;
         }

         offset = lpids[i] % capacity;
         
         rc = getSlot(offset, slot);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to get slot, lpid[%d], rc:%d", lpids[i], rc);
            goto error;
         }

         if (!slot.free())
         {
            PD_LOG(PDERROR, "lpid[%d] is not free, it's pid:%d", lpids[i], slot.page);
            rc = SDB_INVALIDARG;
            goto error;
         }
      }
   done:
      return rc;
   error:
      goto done;
   }

   INT32 impAccessor::getSlot(UINT32 slot, idMapSlot &value)
   {
      INT32 rc = SDB_OK;
      idMapSlot tmp;

      rc = readPageBody(ID_MAP_PAGE_HEAD_LEN + (slot * sizeof(idMapSlot)),
                        sizeof(idMapSlot), (CHAR *)(&tmp));
      if (SDB_OK != rc)
      {
         goto error;
      }

      value.page = tmp.page;
      value.snapshot = tmp.snapshot;
   done:
      return rc;
   error:
      goto done;
   }

   void impAccessor::mapLpids(UINT32 capacity,
                              UINT32 count,
                              const PAGE_ID *lpids,
                              const PAGE_ID *pids,
                              SNAPSHOT_ID snap)
   {
      SDB_ASSERT(0 < capacity, "can not be invalid");
      SDB_ASSERT(NULL != lpids && NULL != pids, "can not be null");
      idMapSlot slot;
      for (UINT32 i = 0; i < count; ++i)
      {
         UINT32 offset = lpids[i] % capacity;
         slot.page = pids[i];
         slot.snapshot = snap;
         writeSlot(offset, slot);
      }
      return;
   }

   void impAccessor::unmapLpids(UINT32 capacity,
                                UINT32 count,
                                const PAGE_ID *lpids)
   {
      SDB_ASSERT(0 < capacity, "can not be invalid");
      SDB_ASSERT(NULL != lpids, "can not be null");
      idMapSlot slot;
      for (UINT32 i = 0; i < count; ++i)
      {
         UINT32 offset = lpids[i] % capacity;
         writeSlot(offset, slot);
      }
      return;
   }

   INT32 impAccessor::writeSlot(UINT32 slot, const idMapSlot &value)
   {
      INT32 rc = SDB_OK;
      rc = writePageBody(ID_MAP_PAGE_HEAD_LEN + (slot * sizeof(idMapSlot)),
                         sizeof(idMapSlot), (CHAR *)(&value));
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to write page body:%d", rc);
         goto error;
      }
   done:
      return rc;
   error:
      goto done;
   }

   INT32 impAccessor::map(requestContext *context,
                          UINT32 count,
                          const PAGE_ID *lpids,
                          const PAGE_ID *pids,
                          SNAPSHOT_ID snap)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(!OSS_BIT_TEST(getFlags(), PAGE_ACCESSOR_FLAG_DIRECT), "can not be mmap");
      SDB_ASSERT(0 != count, "can not be zero");
      SDB_ASSERT(NULL != lpids && NULL != pids, "can not be null");
      SDB_ASSERT(INVALID_SNAPSHOT_ID != snap, "can not be invalid");

      logRecordContext lrc;
      IRedoLogger *logger = NULL;
      ISession *session = NULL;
      DPS_LSN_OFFSET lsn = DPS_INVALID_LSN_OFFSET;
      UINT32 pageSize = pageAccessor::getPageSize();
      UINT32 capacity = 0;


      if (OSS_UNLIKELY(NULL == context ||
                       0 == count ||
                       NULL == lpids ||
                       NULL == pids ||
                       INVALID_SNAPSHOT_ID == snap))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      rc = get64AlignedCapacityOfIMP(pageSize, capacity);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to get capacity of imp:%d", rc);
         goto error;
      }

      rc = validateMap(capacity, count, lpids, pids, snap);
      if (SDB_OK != rc)
      {
         goto error;
      }

      logger = context->getOuterResource()->logger;
      session = context->getSession();

      rc = prepareToWrite(context);
      if (SDB_OK != rc)
      {
         goto error;
      }

      rc = prepareMapLog(context, &lrc, count);
      if (SDB_OK != rc)
      {
         goto error;
      }

      lsn = lrc.getLsn();
      mapLpids(capacity, count, lpids, pids, snap);

      commitMapLog(context, &lrc, count, lpids, pids, snap);

      pageAccessor::commit(context, lsn);
      lrc.close();

   done:
      return rc;
   error:
      if (lrc.prepared())
      {
         logger->abort(session, &lrc);
      }
      if (fullAccessing())
      {
         abortToWrite();
      }
      goto done;
   }

   INT32 impAccessor::dumpAsBitMap(UINT64 *bits, UINT32 &free)
   {
      INT32 rc = SDB_OK;
      UINT32 capacity = 0;
      UINT32 alignedBitsCount = 0;
      idMapSlot slot;
      free = 0;

      if (OSS_UNLIKELY(NULL == bits))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      rc = get64AlignedCapacityOfIMP(getPageSize(), capacity);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to get capacity of imp:%d", rc);
         rc = SDB_VESSEL_INTERNAL_ERR;
         goto error;
      }

      alignedBitsCount = (ossAlign64(capacity) >> 6);
      ossMemset(bits, 0, (alignedBitsCount << 3));
      for (UINT32 i = 0; i < capacity; ++i)
      {
         rc = getSlot(i, slot);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to get slot[%d], rc:%d", i, rc);
            goto error;
         }

         if (slot.free())
         {
            ++free;
            setFreeIfNotFree64(alignedBitsCount, bits, i);
         }
      }
   done:
      return rc;
   error:
      goto done;
   }

   INT32 impAccessor::prepareMapLog(requestContext *context,
                                    logRecordContext *lrc,
                                    UINT32 count)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(NULL != lrc, "can not be null");
      SDB_ASSERT(!lrc->prepared(), "can not be prepared");
      dpsLogRecordHeader *head = NULL;

      head = &(lrc->getHead());
      head->_type = LOG_TYPE_VESSEL_REMAP_LPID;

      lrc->prepush(sizeof(GLOBAL_PAGE_ID));
      lrc->prepush(sizeof(UINT32));
      lrc->prepush(sizeof(SNAPSHOT_ID));
      lrc->prepush(sizeof(PAGE_ID) * count);
      lrc->prepush(sizeof(PAGE_ID) * count);
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

   INT32 impAccessor::commitMapLog(requestContext *context,
                                   logRecordContext *lrc,
                                   UINT32 count,
                                   const PAGE_ID *lpids,
                                   const PAGE_ID *pids,
                                   SNAPSHOT_ID snap)
   {
      INT32 rc = SDB_OK;
      ISession *session = context->getSession();
      SDB_ASSERT(NULL != lrc, "can not be null");
      SDB_ASSERT(lrc->prepared(), "must be prepared");
      IRedoLogger *logger = context->getOuterResource()->logger;

      rc = logger->pushLogRecordElement(session, lrc,
                                        DPS_LOG_PUBLIC_VESSEL_GPID,
                                        sizeof(GLOBAL_PAGE_ID),
                                        &getGPID());
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
                                        DPS_LOG_VESSEL_IMP_REMAP_SNAP,
                                        sizeof(SNAPSHOT_ID),
                                        &snap);
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

      if (lrc->needFullDump())
      {
         const CHAR *dumpBuf = getFullDumpBuffer();
         SDB_ASSERT(NULL != dumpBuf, "can not be null");
         rc = logger->pushLogRecordElement(session, lrc,
                                           DPS_LOG_PUBLIC_VESSEL_FULL_PAGE_DUMP,
                                           getPageSize(), dumpBuf);
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