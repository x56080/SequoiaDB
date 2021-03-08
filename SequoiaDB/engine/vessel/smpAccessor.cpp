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

   Source File Name = smpAccessor.cpp

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

#include "vessel/smpAccessor.h"
#include "vessel/spaceManagementPage.h"
#include "pdTrace.hpp"
#include "ossLikely.hpp"
#include "vessel/IRedoLogger.h"
#include "vessel/logRecordContext.h"
#include "vessel/outerResource.h"
#include "vessel/requestContext.h"
#include "dpsLogRecordDef.hpp"

namespace engine
{
namespace vessel
{
   smpAccessor::smpAccessor()
   {}

   smpAccessor::~smpAccessor()
   {}

   INT32 smpAccessor::initSMP(UINT32 maxSegmentCount,
                              UINT32 pageCountOfSeg,
                              UINT32 pageOccupied)
   {
      INT32 rc = SDB_OK;
      UINT32 capacity = 0;
      spaceManagementPageHead *head = NULL;
      UINT32 loop = 0;
      UINT32 tailBits = 0;

      SDB_ASSERT(0 < maxSegmentCount, "can not be zero");
      SDB_ASSERT(0 < pageCountOfSeg, "can not be zero");

      UINT32 flags = PAGE_ACCESSOR_FLAG_INIT_PAGE | 
                     PAGE_ACCESSOR_FLAG_DIRECT |
                     PAGE_ACCESSOR_FLAG_NON_READONLY;
      SDB_ASSERT(OSS_BIT_TEST(getFlags(), flags), "impossible");

      if (OSS_UNLIKELY(0 == maxSegmentCount || 0 == pageCountOfSeg))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      rc = getSMPCapacity(getPageSize(), maxSegmentCount, pageCountOfSeg, capacity);
      if (SDB_OK != rc)
      {
         goto error;
      }

      if (OSS_UNLIKELY(capacity < pageOccupied))
      {
         rc = SDB_INVALIDARG;
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

      rc = getWritableUserHeadPtr<spaceManagementPageHead>(&head);
      if (SDB_OK != rc)
      {
         goto error;
      }
      
      head->version = SMP_VERSION_1;
      head->minPid = getGPID().page();
      head->capacity = capacity;
      head->free = capacity;
      head->pad = 0;

      loop = capacity / SMP_BIT_COUNT_PER_GROUP;

      for (UINT32 i = 0; i < loop; ++i)
      {
         rc = writeBits(i, UINT32(-1));
         if (SDB_OK != rc)
         {
            goto error;
         }
      }

      tailBits = capacity & 0x1f; /// mod 32
      if (0 != tailBits)
      {
         UINT32 bits = 1;
         for (UINT32 i = 1; i < tailBits; ++i)
         {
            bits = bits << 1;
            bits |= 0x01;
         }

         rc = writeBits(loop, bits);
         if (SDB_OK != rc)
         {
            goto error;
         }
      }

      for (UINT32 i = 0; i < pageOccupied; ++i)
      {
         UINT32 slotNo = i / SMP_BIT_COUNT_PER_GROUP;
         UINT32 bit = i & 0x1f; // mod 32
         rc = setPageNotFree(slotNo, bit);
         if (OSS_UNLIKELY(SDB_OK != rc))
         {
            goto error;
         }
         --head->free;
      }

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

   INT32 smpAccessor::allocatePages(PAGE_TYPE type,
                                    UINT32 count,
                                    const PAGE_ID *lpids,
                                    const PAGE_ID *pids,
                                    const slice &args,
                                    DPS_LSN_OFFSET *oplist)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(!OSS_BIT_TEST(getFlags(), PAGE_ACCESSOR_FLAG_DIRECT), "can not be mmap");
      spaceManagementPageHead *wHead = NULL;
      logRecordContext lrContext;
      IRedoLogger *logger = getContext()->getOuterResource()->logger;
      BOOLEAN rollback = FALSE;
      DPS_LSN_OFFSET lsn = DPS_INVALID_LSN_OFFSET;
      ISession *session = getContext()->getSession();

      if (OSS_UNLIKELY(INVALID_PAGE_TYPE == type ||
                       0 == count ||
                       NULL == lpids ||
                       NULL == pids))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      rc = validatePidsToBeAllocated(count, pids);
      if (SDB_OK != rc)
      {
         goto error;
      }

      rc = prepareSMPAllocateLog(&lrContext, NULL != oplist,
                                 count, args);
      if (OSS_UNLIKELY(SDB_OK != rc))
      {
         goto error;
      }

      lsn = lrContext.getLsn();

      rc = prepareToWrite();
      if (SDB_OK != rc)
      {
         goto error;
      }

      rc = getWritableUserHeadPtr<spaceManagementPageHead>(&wHead);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to get writable head ptr:%d", rc);
         goto error;
      }

      setPagesNotFree(wHead, count, pids);
      wHead->free -= count;
      rollback = TRUE;

      rc = commitSMPAllocateLog(&lrContext,
                                PAGE_TYPE_COLLECTION_RECORD,
                                count, lpids, pids, wHead->free, args);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "faield to commit log:%d", rc);
         goto error;
      }

      pageAccessor::commit(lsn);
      rollback = FALSE;
      lrContext.close();
      if (NULL != oplist)
      {
         *oplist = lsn;
      }
   done:
      return rc;
   error:
      if (lrContext.prepared())
      {
         logger->abort(session, &lrContext);
      }
      if (rollback)
      {
         setPagesFree(wHead, count, pids);
         wHead->free += count;
      }
      if (fullAccessing())
      {
         pageAccessor::abortToWrite();
      }
      goto done;
   }


   INT32 smpAccessor::setPageNotFree(UINT32 bitsSlotNo, UINT32 bitNo)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(bitNo < SMP_BIT_COUNT_PER_GROUP, "impossible");
      UINT32 bits = 0;
      UINT32 bitFlag = 1 << bitNo;

      rc = readBits(bitsSlotNo, bits);
      if (SDB_OK != rc)
      {
         goto error;
      }

      OSS_BIT_CLEAR(bits, bitFlag);

      rc = writeBits(bitsSlotNo, bits);
      if (SDB_OK != rc)
      {
         goto error;
      }
   done:
      return rc;
   error:
      goto done;
   }
  
   INT32 smpAccessor::setPageFree(UINT32 bitsSlotNo, UINT32 bitNo)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(bitNo < SMP_BIT_COUNT_PER_GROUP, "impossible");
      UINT32 bits = 0;
      UINT32 bitFlag = 1 << bitNo;

      rc = readBits(bitsSlotNo, bits);
      if (SDB_OK != rc)
      {
         goto error;
      }

      OSS_BIT_SET(bits, bitFlag);

      rc = writeBits(bitsSlotNo, bits);
      if (SDB_OK != rc)
      {
         goto error;
      }
   done:
      return rc;
   error:
      goto done;
   }

   INT32 smpAccessor::validatePidsToBeAllocated(UINT32 count,
                                                const PAGE_ID *pids)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(0 != count && NULL != pids, "can not be invalid");
      const spaceManagementPageHead *head = NULL;

      rc = getReadableUserHeadPtr<spaceManagementPageHead>(&head);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to get readable head:%d", rc);
         goto error;
      }

      if (head->free < count)
      {
         PD_LOG(PDERROR, "no free pid in smp[%s]", getGPID().toString().c_str());
         rc = SDB_VESSEL_SMP_NO_FREE;
         goto error;
      }
      
      for (UINT32 i = 0; i < count; ++i)
      {
         UINT32 bits = 0;
         UINT32 bit = 0;
         BOOLEAN isFree = FALSE;
         PAGE_ID pid = pids[i];
         if (INVALID_PAGE_ID == pid)
         {
            PD_LOG(PDERROR, "invalid page id");
            rc = SDB_INVALIDARG;
            goto error;
         }
         if (pid < head->minPid || (head->minPid + head->capacity) <= pid)
         {
            PD_LOG(PDERROR, "pid[%d] is out of range:[%d, %d]",
                  pid, head->minPid, head->minPid+head->capacity);
            rc = SDB_INVALIDARG;
            goto error;
         }

         bits = (pid - head->minPid) >> 5; /// divide by 32
         bit = (pid - head->minPid) & 0x1f; /// mod 32
         rc = testPageFree(bits, bit, isFree);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to test page free:%d", rc);
            goto error;
         }
         else if (!isFree)
         {
            PD_LOG(PDERROR, "pid[%d] is not free", pid);
            goto error;
         }
      }
   done:
      return rc;
   error:
      goto done;
   }

   INT32 smpAccessor::testPageFree(UINT32 bitsSlotNo, UINT32 bitNo, BOOLEAN &free)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(bitNo < SMP_BIT_COUNT_PER_GROUP, "impossible");
      UINT32 bits = 0;
      UINT32 testFlag = 1 << bitNo;

      rc = readBits(bitsSlotNo, bits);
      if (SDB_OK != rc)
      {
         goto error;
      }

      free = OSS_BIT_TEST(bits, testFlag);
   done:
      return rc;
   error:
      goto done;
   }

   void smpAccessor::setPagesFree(spaceManagementPageHead *head,
                                  UINT32 count, const PAGE_ID *pids)
   {
      SDB_ASSERT(0 < count && NULL != pids, "can not be invalid");
      for (UINT32 i = 0; i < count; ++i)
      {
         UINT32 bits = 0;
         UINT32 bit = 0;
         PAGE_ID pid = pids[i];
         bits = (pid - head->minPid) >> 5; /// divide by 32
         bit = (pid - head->minPid) & 0x1f; /// mod 32
         setPageFree(bits, bit);
      }
      return;
   }

   void smpAccessor::setPagesNotFree(spaceManagementPageHead *head,
                                     UINT32 count, const PAGE_ID *pids)
   {
      SDB_ASSERT(0 < count && NULL != pids, "can not be invalid");
      for (UINT32 i = 0; i < count; ++i)
      {
         UINT32 bits = 0;
         UINT32 bit = 0;
         PAGE_ID pid = pids[i];
         bits = (pid - head->minPid) >> 5; /// divide by 32
         bit = (pid - head->minPid) & 0x1f; /// mod 32
         setPageNotFree(bits, bit);
      }
      return;
   }

   INT32 smpAccessor::readBits(UINT32 bitsSlotNo, UINT32 &bits)
   {
      INT32 rc = SDB_OK;
      UINT32 tmp = 0;
      UINT32 offset = SMP_HEAD_LEN + sizeof(UINT32) * bitsSlotNo;

      rc = pageAccessor::readPageBody(offset, sizeof(UINT32), (CHAR*)&tmp);
      if (SDB_OK != rc)
      {
         goto error;
      }
      bits = tmp;
   done:
      return rc;
   error:
      goto done;
   }

   INT32 smpAccessor::writeBits(UINT32 bitsSlotNo, UINT32 bits)
   {
      INT32 rc = SDB_OK;
      UINT32 offset = SMP_HEAD_LEN + sizeof(UINT32) * bitsSlotNo;
      rc = pageAccessor::writePageBody(offset, sizeof(UINT32), (const CHAR *)&bits);
      if (SDB_OK != rc)
      {
         goto error;
      }
   done:
      return rc;
   error:
      goto done;
   }

   INT32 smpAccessor::prepareSMPAllocateLog(logRecordContext *lrc,
                                            BOOLEAN oplist,
                                            UINT32 count,
                                            const slice &args)
   {
      INT32 rc = SDB_OK;
      ISession *session = getContext()->getSession();
      SDB_ASSERT(NULL != lrc, "can not be null");
      SDB_ASSERT(!lrc->prepared(), "can not be prepared");
      IRedoLogger *logger = getContext()->getOuterResource()->logger;
      dpsLogRecordHeader *head = NULL;

      head = &(lrc->getHead());
      head->_type = LOG_TYPE_VESSEL_SMP_ALLOCATE;
      OSS_BIT_SET(head->_flags, DPS_VESSEL_LOG_FLAG_FROM_VESSEL);
      if (oplist)
      {
         OSS_BIT_SET(head->_flags, DPS_VESSEL_LOG_FLAG_OP_HEAD);
      }

      /*enum DPS_LOG_VESSEL_SMP_ALLOCATE
   {
      //DPS_LOG_PUBLIC_VESSEL_GPID
      DPS_LOG_VESSEL_SMP_ALLOCATE_PAGE_TYPE = 1,
      DPS_LOG_VESSEL_SMP_ALLOCATE_PAGE_CNT = 2,
      DPS_LOG_VESSEL_SMP_ALLOCATE_LPIDS = 3,
      DPS_LOG_VESSEL_SMP_ALLOCATE_PIDS = 4,
      DPS_LOG_VESSEL_SMP_ALLOCATE_EXT_ARGS = 5,
      DPS_LOG_VESSEL_SMP_ALLOCATE_FREE = 6,
   };*/

      lrc->prepush(sizeof(GLOBAL_PAGE_ID));
      lrc->prepush(sizeof(PAGE_TYPE));
      lrc->prepush(sizeof(UINT32));
      lrc->prepush(sizeof(PAGE_ID) * count);
      lrc->prepush(sizeof(PAGE_ID) * count);
      if (args.valid())
      {
         lrc->prepush(args.len());
      }
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

      if (oplist)
      {
         head->_opListLSN = head->_lsn;
      }
   done:
      return rc;
   error:
      goto done;
   }

   INT32 smpAccessor::commitSMPAllocateLog(logRecordContext *lrc,
                                           PAGE_TYPE type,
                                           UINT32 count,
                                           const PAGE_ID *lpids,
                                           const PAGE_ID *pids,
                                           UINT32 free,
                                           const slice &args)
   {
      INT32 rc = SDB_OK;
      ISession *session = getContext()->getSession();
      SDB_ASSERT(NULL != lrc, "can not be null");
      SDB_ASSERT(lrc->prepared(), "must be prepared");
      SDB_ASSERT(INVALID_PAGE_TYPE != type, "can not be invalid");
      SDB_ASSERT(0 != count, "can not be invalid");
      SDB_ASSERT(NULL != lpids && NULL != pids, "can not be null");

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
                                        DPS_LOG_VESSEL_SMP_ALLOCATE_PAGE_TYPE,
                                        sizeof(PAGE_TYPE),
                                        &type);
      if (OSS_UNLIKELY(SDB_OK != rc))
      {
         goto error;
      }

      rc = logger->pushLogRecordElement(session, lrc,
                                        DPS_LOG_VESSEL_SMP_ALLOCATE_PAGE_CNT,
                                        sizeof(UINT32), &count);
      if (OSS_UNLIKELY(SDB_OK != rc))
      {
         goto error;
      }

      rc = logger->pushLogRecordElement(session, lrc,
                                        DPS_LOG_VESSEL_SMP_ALLOCATE_LPIDS,
                                        sizeof(PAGE_ID) * count, lpids);
      if (OSS_UNLIKELY(SDB_OK != rc))
      {
         goto error;
      }

      rc = logger->pushLogRecordElement(session, lrc,
                                        DPS_LOG_VESSEL_SMP_ALLOCATE_PIDS,
                                        sizeof(PAGE_ID) * count, pids);
      if (OSS_UNLIKELY(SDB_OK != rc))
      {
         goto error;
      }

      if (args.valid())
      {
         rc = logger->pushLogRecordElement(session, lrc,
                                           DPS_LOG_VESSEL_SMP_ALLOCATE_EXT_ARGS,
                                           args.len(), args.data());
         if (OSS_UNLIKELY(SDB_OK != rc))
         {
            goto error;
         }
      }

      rc = logger->pushLogRecordElement(session, lrc,
                                        DPS_LOG_VESSEL_SMP_ALLOCATE_FREE,
                                        sizeof(UINT32), &free);
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
         if (SDB_OK != rc)
         {
            goto error;
         }
      }

      rc = logger->commit(session, lrc);
      if (SDB_OK != rc)
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

