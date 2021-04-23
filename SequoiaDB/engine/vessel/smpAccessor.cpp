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
#include "vessel/bitMapUtils.h"

namespace engine
{
namespace vessel
{
   smpAccessor::smpAccessor()
   {}

   smpAccessor::~smpAccessor()
   {}

   INT32 smpAccessor::initSMP(requestContext *context,
                              UINT32 pageOccupied)
   {
      INT32 rc = SDB_OK;
      spaceManagementPageHead *head = NULL;
      UINT32 bitsCount = 0;
      UINT64 *bits = NULL;
      UINT32 capacity = 0;

      UINT32 flags = PAGE_ACCESSOR_FLAG_INIT_PAGE | 
                     PAGE_ACCESSOR_FLAG_DIRECT |
                     PAGE_ACCESSOR_FLAG_NON_READONLY;
      SDB_ASSERT(OSS_BIT_TEST(getFlags(), flags), "impossible");
      if (OSS_UNLIKELY(NULL == context))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      if (!getSMPCapacityAndCount(getPageSize(), capacity, NULL))
      {
         PD_LOG(PDERROR, "failed to get capacity of page size:%d", getPageSize());
         rc = SDB_VESSEL_INTERNAL_ERR;
         goto error;
      }

      SDB_ASSERT(0 == (capacity & 63), "must be 64 aligned");

      if (OSS_UNLIKELY(capacity < pageOccupied))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      bitsCount = capacity >> 6; /// bitsCount = capacity / 64

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

      rc = getWritableUserHeadPtr<spaceManagementPageHead>(&head);
      if (SDB_OK != rc)
      {
         goto error;
      }
      
      head->version = SMP_VERSION_1;
      head->flags = 0;
      head->free = capacity;
      head->pad = 0;

      rc = getWritePtrOfPageBody<UINT64>(SMP_HEAD_LEN, &bits);
      if (SDB_OK != rc)
      {
         goto error;
      }
      ossMemset(bits, 0xFF, (capacity >> 3)); /// size of memset is (capacity / 8)

      for (UINT32 i = 0; i < pageOccupied; ++i)
      {
         if (!setNotFreeIfFree64(bitsCount, bits, i))
         {
            PD_LOG(PDERROR, "failed to occupy page offset:%d", i);
            rc = SDB_VESSEL_INTERNAL_ERR;
            goto error;
         }
      }
      head->free -= pageOccupied;

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

   INT32 smpAccessor::allocatePages(requestContext *context,
                                    PAGE_TYPE type,
                                    UINT32 count,
                                    const PAGE_ID *lpids,
                                    const PAGE_ID *pids,
                                    const slice &args)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(!OSS_BIT_TEST(getFlags(), PAGE_ACCESSOR_FLAG_DIRECT), "can not be mmap");

      logRecordContext lrContext;
      IRedoLogger *logger = NULL;
      DPS_LSN_OFFSET lsn = DPS_INVALID_LSN_OFFSET;
      ISession *session = NULL;
      UINT32 capacity = 0;
      UINT32 bitsCount = 0;
      spaceManagementPageHead backupHead;
      spaceManagementPageHead *head = NULL;

      if (OSS_UNLIKELY(NULL == context ||
                       INVALID_PAGE_TYPE == type ||
                       0 == count ||
                       NULL == pids))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      if (!getSMPCapacityAndCount(getPageSize(), capacity, NULL))
      {
         PD_LOG(PDERROR, "failed to get capacity of smp");
         rc = SDB_VESSEL_INTERNAL_ERR;
         goto error;
      }

      bitsCount = capacity >> 6;/// capacity / 64
      logger = context->getOuterResource()->logger;
      session = context->getSession();

      rc = validatePidsToBeAllocated(capacity, bitsCount, count, pids);
      if (SDB_OK != rc)
      {
         goto error;
      }

      rc = prepareToWrite(context);
      if (SDB_OK != rc)
      {
         goto error;
      }

      rc = prepareSMPAllocateLog(context, &lrContext,
                                 count, lpids, args);
      if (OSS_UNLIKELY(SDB_OK != rc))
      {
         goto error;
      }

      lsn = lrContext.getLsn();

      rc = getWritableUserHeadPtr<spaceManagementPageHead>(&head);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to get smp head:%d", rc);
         goto error;
      }

      backupHead = *head;
      setPagesNotFree(bitsCount, capacity, count, pids);
      head->free -= count;

      pageAccessor::commit(context, lsn);
      commitSMPAllocateLog(context, &lrContext, &backupHead, head,
                           type, count, lpids, pids, args);
      lrContext.close();

   done:
      return rc;
   error:
      if (lrContext.prepared())
      {
         logger->abort(session, &lrContext);
      }
      if (fullAccessing())
      {
         pageAccessor::abortToWrite();
      }
      goto done;
   }

   INT32 smpAccessor::dumpSMP(requestContext *context,
                              UINT32 bufferSize,
                              CHAR *buffer)
   {
      INT32 rc = SDB_OK;
      UINT32 capacity = 0;
      UINT32 bitsBufSize = 0;
      if (OSS_UNLIKELY(NULL == buffer))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      if (!getSMPCapacityAndCount(getPageSize(), capacity, NULL))
      {
         PD_LOG(PDERROR, "failed to get smp capacity");
         rc = SDB_VESSEL_INTERNAL_ERR;
         goto error;
      }

      bitsBufSize = capacity >> 3;
      SDB_ASSERT(0 == (bitsBufSize & 0x07), "must be 8bytes aligned");
      if (bufferSize < bitsBufSize)
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      ossMemset(buffer, 0, bitsBufSize);
      rc = readPageBody(SMP_HEAD_LEN, bitsBufSize, buffer);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to dump smp:%d", rc);
         goto error;
      }

   done:
      return rc;
   error:
      goto done;
   }

   INT32 smpAccessor::getFreeCount(requestContext *context,
                                   INT32 &free)
   {
      INT32 rc = SDB_OK;
      free = 0;
      const spaceManagementPageHead *head = NULL;
      rc = getReadableUserHeadPtr<spaceManagementPageHead>(&head);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to get smp head:%d", rc);
         goto error;
      }

      free = head->free;

   done:
      return rc;
   error:
      goto done;
   }

   INT32 smpAccessor::validatePidsToBeAllocated(UINT32 capacity,
                                                UINT32 bitsCount,
                                                UINT32 count,
                                                const PAGE_ID *pids)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(0 != bitsCount, "can not be zero");
      SDB_ASSERT(0 != count && NULL != pids, "can not be invalid");
      const GLOBAL_PAGE_ID &gpid = getGPID();
      const UINT64 *bits = NULL;
      const spaceManagementPageHead *head = NULL;

      rc = getReadableUserHeadPtr<spaceManagementPageHead>(&head);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to get page head:%d", rc);
         goto error;
      }

      if (head->free < (INT32)count)
      {
         PD_LOG(PDERROR, "free count[%d] is not enough in page", head->free);
         rc = SDB_VESSEL_INTERNAL_ERR;
         goto error;
      }

      rc = getReadPtrOfPageBody<UINT64>(SMP_HEAD_LEN, &bits);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to get bitmap ptr:%d", rc);
         goto error;
      }
      
      for (UINT32 i = 0; i < count; ++i)
      {
         PAGE_ID pid = pids[i];
         UINT32 offset = (pid & (capacity - 1));
         if (INVALID_PAGE_ID == pid)
         {
            PD_LOG(PDERROR, "invalid page id");
            rc = SDB_INVALIDARG;
            goto error;
         }
         else if (!testBitIsFree(bitsCount, bits, offset))
         {
            PD_LOG(PDERROR, "pid[%d] is not free", pid);
            rc = SDB_VESSEL_INTERNAL_ERR;
            goto error;
         }
      }
   done:
      return rc;
   error:
      goto done;
   }


   void smpAccessor::setPagesFree(UINT32 bitsCount,
                                  UINT32 count, const PAGE_ID *pids)
   {
      SDB_ASSERT(0 < bitsCount, "can not be zero");
      SDB_ASSERT(0 < count && NULL != pids, "can not be invalid");
      UINT64 *bits = NULL;
      const GLOBAL_PAGE_ID &gpid = getGPID();
      getWritePtrOfPageBody<UINT64>(SMP_HEAD_LEN, &bits);
      for (UINT32 i = 0; i < count; ++i)
      {
         PAGE_ID pid = pids[i];
         BOOLEAN r = setFreeIfNotFree64(bitsCount, bits, pid - gpid.page());
         SDB_ASSERT(r, "must be true");
      }
      return;
   }

   void smpAccessor::setPagesNotFree(UINT32 bitsCount,
                                     UINT32 capacity,
                                     UINT32 count,
                                     const PAGE_ID *pids)
   {
      SDB_ASSERT(0 < bitsCount, "can not be zero");
      SDB_ASSERT(ossIsPowerOf2(capacity), "can not be invalid");
      SDB_ASSERT(0 < count && NULL != pids, "can not be invalid");
      UINT64 *bits = NULL;
      getWritePtrOfPageBody<UINT64>(SMP_HEAD_LEN, &bits);
      SDB_ASSERT(NULL != bits, "can not be null");

      for (UINT32 i = 0; i < count; ++i)
      {
         PAGE_ID pid = pids[i];
         UINT32 offset = (pid & (capacity - 1));
         BOOLEAN r = setNotFreeIfFree64(bitsCount, bits, offset);
         SDB_ASSERT(r, "must be true");
      }
      return;
   }

   INT32 smpAccessor::prepareSMPAllocateLog(requestContext *context,
                                            logRecordContext *lrc,
                                            UINT32 count,
                                            const PAGE_ID *lpids,
                                            const slice &args)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(NULL != context, "can not be null");

      SDB_ASSERT(NULL != lrc, "can not be null");
      SDB_ASSERT(!lrc->prepared(), "can not be prepared");
      dpsLogRecordHeader *head = NULL;

      head = &(lrc->getHead());
      head->_type = LOG_TYPE_VESSEL_SMP_ALLOCATE;

      /*enum DPS_LOG_VESSEL_SMP_ALLOCATE
   {
      //DPS_LOG_PUBLIC_VESSEL_GPID
      DPS_LOG_VESSEL_SMP_ALLOCATE_OLD_HEAD = 1,
      DPS_LOG_VESSEL_SMP_ALLOCATE_NEW_HEAD = 2,
      DPS_LOG_VESSEL_SMP_ALLOCATE_PAGE_TYPE = 3,
      DPS_LOG_VESSEL_SMP_ALLOCATE_PAGE_CNT = 4,
      DPS_LOG_VESSEL_SMP_ALLOCATE_LPIDS = 5,
      DPS_LOG_VESSEL_SMP_ALLOCATE_PIDS = 6,
      DPS_LOG_VESSEL_SMP_ALLOCATE_EXT_ARGS = 7,
   };*/

      lrc->prepush(sizeof(GLOBAL_PAGE_ID));
      lrc->prepush(SMP_HEAD_LEN);
      lrc->prepush(SMP_HEAD_LEN);
      lrc->prepush(sizeof(PAGE_TYPE));
      lrc->prepush(sizeof(UINT32));
      if (NULL != lpids)
      {
         lrc->prepush(sizeof(PAGE_ID) * count);
      }
      lrc->prepush(sizeof(PAGE_ID) * count);
      if (args.valid())
      {
         lrc->prepush(args.len());
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

   INT32 smpAccessor::commitSMPAllocateLog(requestContext *context,
                                           logRecordContext *lrc,
                                           const spaceManagementPageHead *oldHead,
                                           const spaceManagementPageHead *newHead,
                                           PAGE_TYPE type,
                                           UINT32 count,
                                           const PAGE_ID *lpids,
                                           const PAGE_ID *pids,
                                           const slice &args)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(NULL != context, "can not be null");
      ISession *session = context->getSession();
      SDB_ASSERT(NULL != lrc, "can not be null");
      SDB_ASSERT(lrc->prepared(), "must be prepared");
      SDB_ASSERT(INVALID_PAGE_TYPE != type, "can not be invalid");
      SDB_ASSERT(0 != count, "can not be invalid");
      SDB_ASSERT(NULL != pids, "can not be null");

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
                                        DPS_LOG_VESSEL_SMP_ALLOCATE_OLD_HEAD,
                                        SMP_HEAD_LEN,
                                        oldHead);
      if (OSS_UNLIKELY(SDB_OK != rc))
      {
         goto error;
      }

      rc = logger->pushLogRecordElement(session, lrc,
                                        DPS_LOG_VESSEL_SMP_ALLOCATE_NEW_HEAD,
                                        SMP_HEAD_LEN,
                                        newHead);
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

      if (NULL != lpids)
      {
         rc = logger->pushLogRecordElement(session, lrc,
                                          DPS_LOG_VESSEL_SMP_ALLOCATE_LPIDS,
                                          sizeof(PAGE_ID) * count, lpids);
         if (OSS_UNLIKELY(SDB_OK != rc))
         {
            goto error;
         }
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

