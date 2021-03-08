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

#include "vessel/impAccessor.h"
#include "vessel/idMapPage.h"
#include "ossLikely.hpp"
#include "pdTrace.hpp"
#include "vessel/requestContext.h"
#include "vessel/IRedoLogger.h"
#include "vessel/logRecordContext.h"
#include "dpsLogRecordDef.hpp"
#include "vessel/outerResource.h"

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

   INT32 impAccessor::initPage(PAGE_ID minLpid)
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
      
      if (OSS_UNLIKELY(INVALID_PAGE_ID == minLpid))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      rc = getCapacityOfIMP(getPageSize(), capacity);
      if (SDB_OK != rc)
      {
         goto error;
      }

      rc = prepareToWrite();
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

      head->version = CURRENT_ID_MAP_PAGE_VERSION;
      head->capacity = capacity;
      head->free = capacity;
      head->minLpid = minLpid;
      head->flags = 0;
      head->pad = 0;

      totalSlotSize = capacity * sizeof(idMapSlot);
      getWritePtrOfPageBody(ID_MAP_PAGE_HEAD_LEN, totalSlotSize, &ptr);
      ossMemset(ptr, 0xff, totalSlotSize);

      pageAccessor::commit(DPS_INVALID_LSN_OFFSET);

   done:
      return rc;
   error:
      if (fullAccessing())
      {
         abortToWrite();
      }
      goto done;
   }

   INT32 impAccessor::getPid(PAGE_ID lpid, PAGE_ID *pid, SNAPSHOT_ID *snapID)
   {
      INT32 rc = SDB_OK;
      const idMapPageHead *head = NULL;
      idMapSlot slot;

      rc = getReadableUserHeadPtr<idMapPageHead>(&head);
      if (SDB_OK != rc)
      {
         goto error;
      }

      if (OSS_UNLIKELY(lpid < head->minLpid ||
                       (head->minLpid + head->capacity) <= lpid))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      rc = getSlot(lpid - head->minLpid, slot);
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

   INT32 impAccessor::getNextValidPid(PAGE_ID &iterator,
                                      UINT32 &fetched,
                                      PAGE_ID &pid,
                                      SNAPSHOT_ID &snapID,
                                      BOOLEAN &hitTheEnd)
   {
      INT32 rc = SDB_OK;
      const idMapPageHead *head = NULL;
      idMapSlot slot;
      PAGE_ID p = iterator;

      rc = getReadableUserHeadPtr<idMapPageHead>(&head);
      if (SDB_OK != rc)
      {
         goto error;
      }

      if (INVALID_PAGE_ID == p)
      {
         p = head->minLpid;
         fetched = 0;
      }
      else if (p < head->minLpid)
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      else
      {
         ++p;
      }

      while ((p < (head->minLpid + head->capacity)) &&
             (fetched < (head->capacity - head->free)))
      {
         rc = getSlot(p - head->minLpid, slot);
         if (SDB_OK != rc)
         {
            goto error;
         }
         if (!slot.free())
         {
            iterator = p;
            ++fetched;
            pid = slot.page;
            snapID = slot.snapshot;
            goto done;
         }
         
         ++p;
      }

      hitTheEnd = TRUE;
   done:
      return rc;
   error:
      goto done;
   }

   INT32 impAccessor::validateMap(UINT32 count,
                                  const PAGE_ID *lpids,
                                  const PAGE_ID *pids,
                                  SNAPSHOT_ID snap)
   {
      INT32 rc = SDB_OK;
      const idMapPageHead *head = NULL;
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

      rc = getReadableUserHeadPtr<idMapPageHead>(&head);
      if (SDB_OK != rc)
      {
         goto error;
      }

      if (head->free < count)
      {
         PD_LOG(PDERROR, "not enough free slots");
         rc = SDB_INVALIDARG;
         goto error;
      }

      for (UINT32 i = 0; i < count; ++i)
      {
         idMapSlot slot;
         PAGE_ID lpid = lpids[i];
         if (INVALID_PAGE_ID == lpid || INVALID_PAGE_ID == pids[i])
         {
            PD_LOG(PDERROR, "invalid page id");
            rc = SDB_INVALIDARG;
            goto error;
         }
         else if (lpid < head->minLpid || (head->minLpid + head->capacity) < lpid)
         {
            PD_LOG(PDERROR, "lpid[%d] out of range[%d,%d]", lpid, head->minLpid, head->capacity);
            rc = SDB_INVALIDARG;
            goto error;
         }
         
         rc = getSlot(lpid - head->minLpid, slot);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to get slot, lpid[%d], rc:%d", lpid, rc);
            goto error;
         }

         if (!slot.free())
         {
            PD_LOG(PDERROR, "lpid[%d] is not free, it's pid:%d", lpid, slot.page);
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

   void impAccessor::mapLpids(idMapPageHead *head,
                              UINT32 count,
                              const PAGE_ID *lpids,
                              const PAGE_ID *pids,
                              SNAPSHOT_ID snap)
   {
      idMapSlot slot;
      for (UINT32 i = 0; i < count; ++i)
      {
         slot.page = pids[i];
         slot.snapshot = snap;
         SDB_ASSERT(head->minLpid <= lpids[i], "impossible");
         writeSlot(lpids[i] - head->minLpid, slot);
      }
      return;
   }

   void impAccessor::unmapLpids(idMapPageHead *head,
                                UINT32 count,
                                const PAGE_ID *lpids)
   {
      idMapSlot slot;
      for (UINT32 i = 0; i < count; ++i)
      {
         SDB_ASSERT(head->minLpid <= lpids[i], "impossible");
         writeSlot(lpids[i] - head->minLpid, slot);
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

   INT32 impAccessor::map(UINT32 count,
                          const PAGE_ID *lpids,
                          const PAGE_ID *pids,
                          SNAPSHOT_ID snap,
                          const DPS_LSN_OFFSET *oplist)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(!OSS_BIT_TEST(getFlags(), PAGE_ACCESSOR_FLAG_DIRECT), "can not be mmap");
      SDB_ASSERT(0 != count, "can not be zero");
      SDB_ASSERT(NULL != lpids && NULL != pids, "can not be null");
      SDB_ASSERT(INVALID_SNAPSHOT_ID != snap, "can not be invalid");
      idMapPageHead *wHead = NULL;
      logRecordContext lrc;
      IRedoLogger *logger = getContext()->getOuterResource()->logger;
      ISession *session = getContext()->getSession();
      DPS_LSN_OFFSET lsn = DPS_INVALID_LSN_OFFSET;
      BOOLEAN rollback = FALSE;

      rc = validateMap(count, lpids, pids, snap);
      if (SDB_OK != rc)
      {
         goto error;
      }

      rc = prepareMapLog(&lrc, oplist, count);
      if (SDB_OK != rc)
      {
         goto error;
      }

      lsn = lrc.getLsn();

      rc = prepareToWrite();
      if (SDB_OK != rc)
      {
         goto error;
      }

      rc = getWritableUserHeadPtr<idMapPageHead>(&wHead);
      if (SDB_OK != rc)
      {
         goto error;
      }

      mapLpids(wHead, count, lpids, pids, snap);
      wHead->free -= count;
      rollback = TRUE;

      rc = commitMapLog(&lrc, count, lpids, pids, snap, wHead->free);
      if (SDB_OK != rc)
      {
         goto error;
      }

      pageAccessor::commit(lsn);
      lrc.close();
      rollback = FALSE;

   done:
      return rc;
   error:
      if (lrc.prepared())
      {
         logger->abort(session, &lrc);
      }
      if (rollback)
      {
         unmapLpids(wHead, count, lpids);
         wHead->free += count;
      }
      if (fullAccessing())
      {
         abortToWrite();
      }
      goto done;
   }

   INT32 impAccessor::prepareMapLog(logRecordContext *lrc,
                                    const DPS_LSN_OFFSET *oplist,
                                    UINT32 count)
   {
      INT32 rc = SDB_OK;
      ISession *session = getContext()->getSession();
      SDB_ASSERT(NULL != lrc, "can not be null");
      SDB_ASSERT(!lrc->prepared(), "can not be prepared");
      IRedoLogger *logger = getContext()->getOuterResource()->logger;
      dpsLogRecordHeader *head = NULL;

      head = &(lrc->getHead());
      head->_type = LOG_TYPE_VESSEL_REMAP_LPID;
      OSS_BIT_SET(head->_flags, DPS_VESSEL_LOG_FLAG_FROM_VESSEL);
      if (NULL != oplist && DPS_INVALID_LSN_OFFSET != *oplist)
      {
         OSS_BIT_SET(head->_flags, DPS_VESSEL_LOG_FLAG_OP_TAIL);
         head->_opListLSN = *oplist;
      }

      lrc->prepush(sizeof(GLOBAL_PAGE_ID));
      lrc->prepush(sizeof(UINT32));
      lrc->prepush(sizeof(SNAPSHOT_ID));
      lrc->prepush(sizeof(PAGE_ID) * count);
      lrc->prepush(sizeof(PAGE_ID) * count);
      lrc->prepush(sizeof(UINT32));
      rc = prepareFullDumpLogWhenNecessary(lrc);
      if (SDB_OK != rc)
      {
         goto error;
      }
      lrc->prepushDone();

      rc = logger->prepare(session, lrc);
      if (SDB_OK != rc)
      {
         goto error;
      }

   done:
      return rc;
   error:
      goto done;
   }

   INT32 impAccessor::commitMapLog(logRecordContext *lrc,
                                   UINT32 count,
                                   const PAGE_ID *lpids,
                                   const PAGE_ID *pids,
                                   SNAPSHOT_ID snap,
                                   UINT32 free)
   {
      INT32 rc = SDB_OK;
      ISession *session = getContext()->getSession();
      SDB_ASSERT(NULL != lrc, "can not be null");
      SDB_ASSERT(lrc->prepared(), "must be prepared");
      IRedoLogger *logger = getContext()->getOuterResource()->logger;

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

      rc = logger->pushLogRecordElement(session, lrc,
                                        DPS_LOG_VESSEL_IMP_REMAP_FREE,
                                        sizeof(UINT32),
                                        &free);
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