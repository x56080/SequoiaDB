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

   INT32 impAccessor::writeSlot(UINT32 slot, const idMapSlot &value)
   {
      INT32 rc = SDB_OK;
      rc = writePageBody(ID_MAP_PAGE_HEAD_LEN + (slot * sizeof(idMapSlot)),
                         sizeof(idMapSlot), (CHAR *)(&value));
      if (SDB_OK != rc)
      {
         goto error;
      }
   done:
      return rc;
   error:
      goto done;
   }

   INT32 impAccessor::remap(PAGE_ID lpid,
                            PAGE_ID pid,
                            SNAPSHOT_ID snap,
                            const DPS_LSN_OFFSET *oplist)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(!OSS_BIT_TEST(getFlags(), PAGE_ACCESSOR_FLAG_DIRECT), "can not be mmap");
      const idMapPageHead *head = NULL;
      idMapPageHead *wHead = NULL;
      idMapSlot oldSlot;
      idMapSlot newSlot;
      logRecordContext lrc;
      IRedoLogger *logger = getContext()->getOuterResource()->logger;
      ISession *session = getContext()->getSession();
      DPS_LSN_OFFSET lsn = DPS_INVALID_LSN_OFFSET;
      BOOLEAN rollback = FALSE;

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

      rc = getSlot(lpid - head->minLpid, oldSlot);
      if (SDB_OK != rc)
      {
         goto error;
      }

      if (oldSlot.free())
      {
         if (0 == head->free)
         {
            PD_LOG(PDERROR, "old slot is free but no free count is zero, gpid[%s]", getGPID().toString().c_str());
            rc = SDB_VESSEL_INTERNAL_ERR;
            goto error;
         }
      }

      newSlot.snapshot = snap;
      newSlot.page = pid;

      rc = prepareRemapLog(&lrc, oplist, lpid, oldSlot, newSlot);
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

      rc = writeSlot(lpid - head->minLpid, newSlot);
      if (SDB_OK != rc)
      {
         goto error;
      }

      if (oldSlot.free())
      {
         --wHead->free;
      }

      rollback = TRUE;
      rc = commitRemapLog(&lrc, lpid, oldSlot, newSlot);
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
         if (oldSlot.free())
         {
            ++wHead->free;
         }

         writeSlot(lpid - head->minLpid, oldSlot);
      }
      if (fullAccessing())
      {
         abortToWrite();
      }
      goto done;
   }

   INT32 impAccessor::getHeadContent(PAGE_ID &minLpid, UINT32 &capacity, UINT32 &free)
   {
      INT32 rc = SDB_OK;
      const idMapPageHead *head = NULL;
      rc = getReadableUserHeadPtr<idMapPageHead>(&head);
      if (SDB_OK != rc)
      {
         goto error;
      }

      minLpid = head->minLpid;
      capacity = head->capacity;
      free = head->free;

   done:
      return rc;
   error:
      goto done;
   }

   INT32 impAccessor::prepareRemapLog(logRecordContext *lrc,
                                      const DPS_LSN_OFFSET *oplist,
                                      PAGE_ID lpid,
                                      const idMapSlot &oldSlot,
                                      const idMapSlot &newSlot)
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
      if (NULL != oplist)
      {
         OSS_BIT_SET(head->_flags, DPS_VESSEL_LOG_FLAG_OP_TAIL);
         head->_opListLSN = *oplist;
      }

      lrc->prepush(sizeof(GLOBAL_PAGE_ID));
      lrc->prepush(sizeof(PAGE_ID));
      lrc->prepush(sizeof(idMapSlot));
      lrc->prepush(sizeof(idMapSlot));
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

   INT32 impAccessor::commitRemapLog(logRecordContext *lrc,
                                     PAGE_ID lpid,
                                     const idMapSlot &oldSlot,
                                     const idMapSlot &newSlot)
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
                                        DPS_LOG_VESSEL_IMP_REMAP_LPID,
                                        sizeof(PAGE_ID),
                                        &lpid);
      if (OSS_UNLIKELY(SDB_OK != rc))
      {
         goto error;
      }

      rc = logger->pushLogRecordElement(session, lrc,
                                        DPS_LOG_VESSEL_IMP_REMAP_OLD_SLOT,
                                        sizeof(idMapSlot),
                                        &oldSlot);
      if (OSS_UNLIKELY(SDB_OK != rc))
      {
         goto error;
      }

      rc = logger->pushLogRecordElement(session, lrc,
                                        DPS_LOG_VESSEL_IMP_REMAP_NEW_SLOT,
                                        sizeof(idMapSlot),
                                        &newSlot);
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