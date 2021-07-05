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

   UINT32 smpAccessor::getUserPageHeadSize()const
   {
      return SMP_HEAD_SIZE;
   }

   INT32 smpAccessor::allocatePages(requestContext *context,
                                    UINT32 count,
                                    const PAGE_ID *pids)
   {
      INT32 rc = SDB_OK;
      
      if (OSS_UNLIKELY(NULL == context ||
                       0 == count ||
                       NULL == pids))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      else if (OSS_UNLIKELY(!isOpen()))
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }

      rc = validatePidsToBeAllocated(count, pids);
      if (SDB_OK != rc)
      {
         goto error;
      }

      rc = _allocatePages(context, count, pids);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to allocate pages on smp:%d", rc);
         goto error;
      }

   done:
      return rc;
   error:
      goto done;
   }

   INT32 smpAccessor::getFreeCount(UINT32 &count)const
   {
      INT32 rc = SDB_OK;
      const UINT64 *bitmap = NULL;
      UINT32 capacity = 0;
      UINT32 uint64Count = 0;

      if (OSS_UNLIKELY(!isOpen()))
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }

      if (OSS_UNLIKELY(!getSMPCapacityOrCount(getRuntimeBuffer()->getPageSize(), &capacity, NULL)))
      {
         rc = SDB_VESSEL_INTERNAL_ERR;
         goto error;
      }

      bitmap = (const UINT64 *)
               (getRuntimeBuffer()->getReadablePtrOfBody(SMP_HEAD_SIZE, capacity >> 3));
      if (NULL == bitmap)
      {
         PD_LOG(PDERROR, "failed to get bitmap ptr");
         rc = SDB_VESSEL_INTERNAL_ERR;
         goto error;
      }

      count = 0;
      uint64Count = capacity >> 6;
      for (UINT32 i = 0; i < count; ++i)
      { 
         count += ossGetNonZeroBitCount64(bitmap[i]);
      }
   done:
      return rc;
   error:
      goto done;
   }
   

   INT32 smpAccessor::_allocatePages(requestContext *context,
                                     UINT32 count,
                                     const PAGE_ID *pids)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(NULL != context, "can not be null");
      SDB_ASSERT(0 < count, "can not be zero");
      SDB_ASSERT(NULL != pids, "can not be null");
      SDB_ASSERT(pageAccessor::isOpen(), "must be open");
      SDB_ASSERT(!getRuntimeBuffer()->isWritable(), "can not be writable");
      logRecordContext lrc;
      DPS_LSN_OFFSET lsn = DPS_INVALID_LSN_OFFSET;
      UINT64 *bitmap = NULL;
      UINT32 bitsCount = 0;
      UINT32 capacity = 0;

      rc = getRuntimeBuffer()->prepareToWrite(context);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to prepare to write:%d", rc);
         goto error;
      }

      if (OSS_UNLIKELY(!getSMPCapacityOrCount(getRuntimeBuffer()->getPageSize(), &capacity, NULL)))
      {
         PD_LOG(PDERROR, "failed to get smp capacity");
         rc = SDB_VESSEL_INTERNAL_ERR;
         goto error;
      }

      if (getRuntimeBuffer()->isCacheBuffer())
      {
         rc = prepareAllocateLog(context, &lrc, count);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to prepare log:%d", rc);
            goto error;
         }
      }

      bitsCount = capacity >> 6;

      bitmap = (UINT64 *)
               (getRuntimeBuffer()->getWritablePtrOfBody(SMP_HEAD_SIZE, (capacity >> 3)));
      if (OSS_UNLIKELY(NULL == bitmap))
      {
         PD_LOG(PDERROR, "failed to get bitmap ptr");
         rc = SDB_VESSEL_INTERNAL_ERR;
         goto error;
      }

      for (UINT32 i = 0; i < count; ++i)
      {
         UINT32 offset = (pids[i] & (capacity - 1));
         BOOLEAN r = setNotFreeIfFree64(bitsCount, bitmap, offset);
         SDB_ASSERT(r, "impossible");
      }

      getRuntimeBuffer()->commit(lsn);
      if (lrc.prepared())
      {
         commitAllocateLog(context, &lrc, count, pids);
         lrc.close();
      }
   done:
      return rc;
   error:
      if (lrc.prepared())
      {
         pageAccessor::abortLog(context, &lrc);
      }
      if (getRuntimeBuffer()->isWritable())
      {
         getRuntimeBuffer()->abort();
      }
      goto done;
   }

   INT32 smpAccessor::validatePidsToBeAllocated(UINT32 count,
                                                const PAGE_ID *pids)const
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(0 != count && NULL != pids, "can not be invalid");
      SDB_ASSERT(NULL != getRuntimeBuffer(), "can not be null");
      const UINT64 *bitmap = NULL;
      UINT32 bitsCount = 0;
      UINT32 capacity = 0;
      const spaceManagementPageHead *head = NULL;
      UINT32 factor = 0;

      if (OSS_UNLIKELY(!getSMPCapacityOrCount(getRuntimeBuffer()->getPageSize(), &capacity, NULL)))
      {
         PD_LOG(PDERROR, "failed to get smp capacity");
         rc = SDB_VESSEL_INTERNAL_ERR;
         goto error;
      }

      head = getRuntimeBuffer()->getReadablePtrOfBody<spaceManagementPageHead>(0);
      if (OSS_UNLIKELY(NULL == head))
      {
         PD_LOG(PDERROR, "failed to get readable head");
         rc = SDB_VESSEL_INTERNAL_ERR;
         goto error;
      }

      if (OSS_UNLIKELY(!head->isValid()))
      {
         PD_LOG(PDERROR, "invalid smp head");
         rc = SDB_VESSEL_INTERNAL_ERR;
         goto error;
      }

      if (OSS_UNLIKELY(capacity < count))
      {
         rc = SDB_VESSEL_NOT_ENOUGH_FREE_RESOURCE;
         goto error;
      }

      bitsCount = capacity >> 6;
      bitmap = (const UINT64 *)
                (getRuntimeBuffer()->getReadablePtrOfBody(SMP_HEAD_SIZE, (capacity >> 3)));
      if (OSS_UNLIKELY(NULL == bitmap))
      {
         PD_LOG(PDERROR, "failed to get bitmap buffer");
         rc = SDB_VESSEL_INTERNAL_ERR;
         goto error;
      }

      for (UINT32 i = 0; i < count; ++i)
      {
         UINT32 offset = (pids[i] & (capacity - 1));
         if (OSS_UNLIKELY(INVALID_PAGE_ID == pids[i]))
         {
            PD_LOG(PDERROR, "can not allocate invalid pid");
            rc = SDB_INVALIDARG;
            goto error;
         }
         if (!testBitIsFree(bitsCount, bitmap, offset))
         {
            PD_LOG(PDERROR, "pid[%d] is not free on current smp[%s]",
                   pids[i], getRuntimeBuffer()->getGlobalPid().toString().c_str());
            rc = SDB_VESSEL_OPERATOION_NOT_PERMITTED;
            goto error;
         }
      }

      factor = pids[0] / capacity;
      for (UINT32 i = 1; i < count; ++i)
      {
         if ((pids[i] / capacity) != factor)
         {
            PD_LOG(PDERROR, "factor should be same");
            rc = SDB_VESSEL_OPERATOION_NOT_PERMITTED;
            goto error;
         }
      }
   done:
      return rc;
   error:
      goto done;
   }

   INT32 smpAccessor::prepareAllocateLog(requestContext *context,
                                         logRecordContext *lrc,
                                         UINT32 count)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(NULL != context, "can not be null");
      SDB_ASSERT(NULL != lrc, "can not be null");
      SDB_ASSERT(!lrc->prepared(), "can not be prepared");
      SDB_ASSERT(0 < count, "can not be zero");
      dpsLogRecordHeader *head = NULL;

      head = &(lrc->getHead());
      head->_type = DPS_LOG_VESSEL_SMP_ALLOCATE;

      lrc->prepush(sizeof(GLOBAL_PAGE_ID));
      lrc->prepush(sizeof(UINT32));
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

   INT32 smpAccessor::commitAllocateLog(requestContext *context,
                                        logRecordContext *lrc,
                                        UINT32 count,
                                        const PAGE_ID *pids)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(NULL != context, "can not be null");
      ISession *session = context->getSession();
      SDB_ASSERT(NULL != lrc, "can not be null");
      SDB_ASSERT(lrc->prepared(), "must be prepared");
      SDB_ASSERT(0 != count, "can not be zero");
      SDB_ASSERT(NULL != pids, "can not be null");
      SDB_ASSERT(NULL != getRuntimeBuffer(), "can not be null");
      SDB_ASSERT(getRuntimeBuffer()->isValid(), "must be be valid");
      SDB_ASSERT(getRuntimeBuffer()->isCacheBuffer(), "must be cache buffer");

      const GLOBAL_PAGE_ID &gpid = getRuntimeBuffer()->getGlobalPid();
      IRedoLogger *logger = context->getOuterResource()->logger;

      rc = logger->pushLogRecordElement(session, lrc,
                                        DPS_LOG_PUBLIC_VESSEL_GPID,
                                        sizeof(GLOBAL_PAGE_ID),
                                        &gpid);
      if (OSS_UNLIKELY(SDB_OK != rc))
      {
         goto error;
      }

      rc = logger->pushLogRecordElement(session, lrc,
                                        DPS_LOG_VESSEL_SMP_ALLOCATE_COUNT,
                                        sizeof(UINT32), &count);
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

      rc = pageAccessor::commitLogDone(context, lrc);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to commit log:%d", rc);
         goto error;
      }
   done:
      return rc;
   error:
      PD_LOG(PDERROR, "faield to commit log[%lld], rc:%d", lrc->getLsn(), rc);
      SDB_ASSERT(FALSE, "impossible");
      goto done;
   }

   INT32 smpAccessor::dumpBitmapSlice(requestContext *context,
                                      UINT32 beginUint64,
                                      UINT32 uint64Count,
                                      UINT32 bufferSize,
                                      void *buffer)const
   {
      INT32 rc = SDB_OK;
      const runtimePageBuffer *rpb = NULL;
      const spaceManagementPageHead *head = NULL;
      const UINT64 *bitmap = NULL;
      UINT32 capacity = 0;

      if (OSS_UNLIKELY(NULL == context ||
                       NULL == buffer))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      else if (OSS_UNLIKELY(0 == uint64Count))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      else if (OSS_UNLIKELY(bufferSize < (uint64Count << 3)))
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
      SDB_ASSERT(NULL != rpb, "can not be null");
      SDB_ASSERT(rpb->isValid(), "can not be invalid");

      if (OSS_UNLIKELY(!getSMPCapacityOrCount(rpb->getPageSize(), &capacity, NULL)))
      {
         PD_LOG(PDERROR, "failed to get smp capacity");
         rc = SDB_VESSEL_INTERNAL_ERR;
         goto error;
      }

      head = rpb->getReadablePtrOfBody<spaceManagementPageHead>(0);
      if (OSS_UNLIKELY(NULL == head))
      {
         PD_LOG(PDERROR, "failed to get smp head");
         rc = SDB_VESSEL_INTERNAL_ERR;
         goto error;
      }
      else if (OSS_UNLIKELY(!head->isValid()))
      {
         PD_LOG(PDERROR, "invalid smp head of pid[%s]",
                rpb->getGlobalPid().toString().c_str());  
      }

      if ((capacity >> 6) < (beginUint64 + uint64Count))
      {
         rc = SDB_VESSEL_INVALID_PTR_OFFSET;
         goto error;
      }

      bitmap = (const UINT64 *)
                (rpb->getReadablePtrOfBody(SMP_HEAD_SIZE, capacity >> 3));
      if (OSS_UNLIKELY(NULL == bitmap))
      {
         PD_LOG(PDERROR, "failed to get bitmap ptr");
         rc = SDB_VESSEL_INTERNAL_ERR;
         goto error;
      }

      ossMemcpy(buffer, bitmap + beginUint64, uint64Count << 3);
  
   done:
      return rc;
   error:
      goto done;
   }
}//namespace vessel
}//namespace engine

