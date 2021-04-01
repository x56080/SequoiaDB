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
                              UINT32 maxSegmentCount,
                              UINT32 pageCountOfSeg,
                              UINT32 pageOccupied)
   {
      INT32 rc = SDB_OK;
      UINT32 capacity = 0;
      spaceManagementPageHead *head = NULL;
      UINT32 bitsCount = 0;
      UINT64 *bits = NULL;

      SDB_ASSERT(0 < maxSegmentCount, "can not be zero");
      SDB_ASSERT(0 < pageCountOfSeg, "can not be zero");

      UINT32 flags = PAGE_ACCESSOR_FLAG_INIT_PAGE | 
                     PAGE_ACCESSOR_FLAG_DIRECT |
                     PAGE_ACCESSOR_FLAG_NON_READONLY;
      SDB_ASSERT(OSS_BIT_TEST(getFlags(), flags), "impossible");
      if (OSS_UNLIKELY(NULL == context))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      else if (OSS_UNLIKELY(0 == maxSegmentCount || 0 == pageCountOfSeg))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      rc = getSMPCapacity8BytesAligned(getPageSize(), maxSegmentCount, pageCountOfSeg, capacity);
      if (SDB_OK != rc)
      {
         goto error;
      }

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
                                    UINT32 capacity,
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
      BOOLEAN rollback = FALSE;
      DPS_LSN_OFFSET lsn = DPS_INVALID_LSN_OFFSET;
      ISession *session = NULL;
      UINT32 bitsCount = capacity >> 6;/// capacity / 64

      if (OSS_UNLIKELY(NULL == context ||
                       0 == capacity ||
                       ossAlign64(capacity) != capacity ||
                       INVALID_PAGE_TYPE == type ||
                       0 == count ||
                       NULL == lpids ||
                       NULL == pids))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      logger = context->getOuterResource()->logger;
      session = context->getSession();

      rc = validatePidsToBeAllocated(bitsCount, count, pids);
      if (SDB_OK != rc)
      {
         goto error;
      }

      rc = prepareSMPAllocateLog(context, &lrContext,
                                 count, args);
      if (OSS_UNLIKELY(SDB_OK != rc))
      {
         goto error;
      }

      lsn = lrContext.getLsn();

      rc = prepareToWrite(context);
      if (SDB_OK != rc)
      {
         goto error;
      }

      setPagesNotFree(bitsCount, count, pids);
      rollback = TRUE;

      rc = commitSMPAllocateLog(context, &lrContext,
                                type, count, lpids, pids, args);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "faield to commit log:%d", rc);
         goto error;
      }

      pageAccessor::commit(context, lsn);
      rollback = FALSE;
      lrContext.close();

   done:
      return rc;
   error:
      if (lrContext.prepared())
      {
         logger->abort(session, &lrContext);
      }
      if (rollback)
      {
         setPagesFree(bitsCount, count, pids);
      }
      if (fullAccessing())
      {
         pageAccessor::abortToWrite();
      }
      goto done;
   }

   INT32 smpAccessor::dumpSMP(UINT32 bufferSize,
                              CHAR *buffer)
   {
      INT32 rc = SDB_OK;
      if (OSS_UNLIKELY(NULL == buffer))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      else if (bufferSize < getPageBodySize())
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      rc = readPageBody(0, getPageBodySize(), buffer);
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

   INT32 smpAccessor::validatePidsToBeAllocated(UINT32 bitsCount,
                                                UINT32 count,
                                                const PAGE_ID *pids)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(0 != bitsCount, "can not be zero");
      SDB_ASSERT(0 != count && NULL != pids, "can not be invalid");
      const GLOBAL_PAGE_ID &gpid = getGPID();
      const UINT64 *bits = NULL;
      UINT32 capacity = bitsCount << 6;/// bitsCount * 64

      rc = getReadPtrOfPageBody<UINT64>(SMP_HEAD_LEN, &bits);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to get bitmap ptr:%d", rc);
         goto error;
      }
      
      for (UINT32 i = 0; i < count; ++i)
      {
         PAGE_ID pid = pids[i];
         if (INVALID_PAGE_ID == pid)
         {
            PD_LOG(PDERROR, "invalid page id");
            rc = SDB_INVALIDARG;
            goto error;
         }
         else if (pid < gpid.page() || ((gpid.page() + capacity) < pid))
         {
            PD_LOG(PDERROR, "pid[%d] is out of range:[%d, %d)",
                  pid,  gpid.page(), gpid.page() + capacity);
            rc = SDB_INVALIDARG;
            goto error;
         }
         else if (!testBitIsFree(bitsCount, bits, pid - gpid.page()))
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

   void smpAccessor::setPagesNotFree(UINT32 bitsCount, UINT32 count, const PAGE_ID *pids)
   {
      SDB_ASSERT(0 < bitsCount, "can not be zero");
      SDB_ASSERT(0 < count && NULL != pids, "can not be invalid");
      UINT64 *bits = NULL;
      const GLOBAL_PAGE_ID &gpid = getGPID();
      getWritePtrOfPageBody<UINT64>(SMP_HEAD_LEN, &bits);
      SDB_ASSERT(NULL != bits, "can not be null");

      for (UINT32 i = 0; i < count; ++i)
      {
         PAGE_ID pid = pids[i];
         BOOLEAN r = setNotFreeIfFree64(bitsCount, bits, pid - gpid.page());
         SDB_ASSERT(r, "must be true");
      }
      return;
   }

   INT32 smpAccessor::prepareSMPAllocateLog(requestContext *context,
                                            logRecordContext *lrc,
                                            UINT32 count,
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
      DPS_LOG_VESSEL_SMP_ALLOCATE_PAGE_TYPE = 1,
      DPS_LOG_VESSEL_SMP_ALLOCATE_PAGE_CNT = 2,
      DPS_LOG_VESSEL_SMP_ALLOCATE_LPIDS = 3,
      DPS_LOG_VESSEL_SMP_ALLOCATE_PIDS = 4,
      DPS_LOG_VESSEL_SMP_ALLOCATE_EXT_ARGS = 5,
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
      SDB_ASSERT(NULL != lpids && NULL != pids, "can not be null");

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

