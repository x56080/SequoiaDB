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

   Source File Name = pageAccessor.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#include "vessel/pageAccessor.h"
#include "ossLikely.hpp"
#include "vessel/requestContext.h"
#include "vessel/instanceEnv.h"
#include "vessel/storageUnit.h"
#include "vessel/vesselOptions.h"
#include "vessel/outerResource.h"
#include "vessel/IRedoLogger.h"
#include "dpsLogRecordDef.hpp"
#include "vessel/logRecordContext.h"
#include "vessel/checkpointController.h"
#include "vessel/atomicOperationList.h"

namespace engine
{
namespace vessel
{
   INT32 pageAccessor::prepareLog(requestContext *context,
                                  const runtimePageBuffer *rpb,
                                  UINT16 logType,
                                  BOOLEAN resetPage,
                                  logRecordContext *lrc)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(NULL != context, "can not be null");
      SDB_ASSERT(NULL != rpb, "can not be null");
      SDB_ASSERT(rpb->isValid(), "must be valid");
      SDB_ASSERT(rpb->isCacheBuffer(), "must be cache buffer");

      ///pid and psv may changed after full dumping.
      SDB_ASSERT(rpb->isWritingPrepared(), "must be prepare");
      SDB_ASSERT(NULL != lrc, "can not be null");
      SDB_ASSERT(!lrc->needFullDump(), "can not be full dump");
      SDB_ASSERT(!lrc->prepared(), "can not be prepared");

      const checkpointController *checkpointer = &(context->getEnv()->checkpointer);
      const openDBOptions &options = context->getEnv()->options;
      strictBuffer buffer = rpb->getReadableBuffer();

      lrc->open(logType);
      if (resetPage)
      {
         lrc->setResetPage();
      }
      else if (options.fullDumpPageLog &&
               checkpointer->hasAtLeastOneCheckpoint())
      {
         DPS_LSN_OFFSET lsn = buffer.getReadableObjPtr<pageHead>(0)->lsn;
         if (DPS_INVALID_LSN_OFFSET != lsn &&
             lsn <= checkpointer->getLastCheckpointLSN())
         {
            rc = lrc->fullDumpPage(rpb->getPageSize(), buffer.getRPtr());
            if (SDB_OK != rc)
            {
               PD_LOG(PDERROR, "faile to full dump page:%d", rc);
               goto error;
            }
         }
      }

      
      if (context->isInProcessingOplist())
      {
         atomicOperationList *oplist = context->getOplist();
         if (oplist->isWatingHead())
         {
            lrc->setOplistHead();
         }
         else
         {
            lrc->setOplist(oplist->getOplistLsn());
         }

         /// not else if
         if (oplist->isWaitingTail())
         {
            lrc->setOplistTail();
         }
      }
   done:
      return rc;
   error:
      goto done;
   }

   INT32 pageAccessor::prepareLogDone(requestContext *context,
                                      logRecordContext *lrc)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(NULL != context, "can not be null");
      /// pid and psv may be different after writing prepared.
      SDB_ASSERT(NULL != lrc, "can not be null");
      SDB_ASSERT(!lrc->prepared(), "can not be prepared");

      IExecutor *executor = context->getExecutor();
      IRedoLogger *logger = context->getOuterResource()->logger;
   
      lrc->prepushDone();
      rc = logger->prepare(executor, lrc);
      if (SDB_OK != rc)
      {
         goto error;
      }
   done:
      return rc;
   error:
      goto done;
   }

   INT32 pageAccessor::pushElement(requestContext *context,
                                   UINT8 tag,
                                   UINT32 size,
                                   const void *data,
                                   logRecordContext *lrc)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(NULL != context, "can not be null");
      SDB_ASSERT(0 < size, "can not be zero");
      SDB_ASSERT(NULL != data, "can not be null");
      SDB_ASSERT(NULL != lrc, "can not be null");
      SDB_ASSERT(lrc->prepared(), "must be prepared");

      IExecutor *executor = context->getExecutor();
      IRedoLogger *logger = context->getOuterResource()->logger;

      rc = logger->pushLogRecordElement(executor, lrc, tag, size, data);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to push log ele[%d], size[%d], rc:%d", tag, size, rc);
         goto error;
      }
   done:
      return rc;
   error:
      goto done;
   }

   void pageAccessor::abortLog(requestContext *context,
                               logRecordContext *lrc)
   {
      SDB_ASSERT(NULL != context, "can not be invalid");
      SDB_ASSERT(NULL != lrc, "can not be null");
      IRedoLogger *logger = context->getOuterResource()->logger;
      if (lrc->prepared())
      {
         logger->abort(context->getExecutor(), lrc);
      }
      return;
   }

   INT32 pageAccessor::commitLog(requestContext *context,
                                 logRecordContext *lrc)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(NULL != context, "can not be null");
      SDB_ASSERT(lrc->prepared(), "can not be prepared");

      IExecutor *executor = context->getExecutor();
      IRedoLogger *logger = context->getOuterResource()->logger;

      if (lrc->needFullDump())
      {
         rc = logger->pushLogRecordElement(executor, lrc,
                                           DPS_LOG_PUBLIC_VESSEL_FULL_PAGE_DUMP,
                                           lrc->getFullDumpDataSize(),
                                           lrc->getFullDumpBuffer());
         if (SDB_OK != rc)
         {
            goto error;
         }
      }

      rc = logger->commit(executor, lrc);
      if (SDB_OK != rc)
      {
         goto error;
      }

      if (context->isInProcessingOplist())
      {
         atomicOperationList *oplist = context->getOplist();
         if (oplist->isWatingHead())
         {
            SDB_ASSERT(0 != OSS_BIT_TEST(lrc->getHead()._flags,
                                         DPS_VESSEL_LOG_FLAG_OPL_HEAD), "impossible");
         }
         else if (oplist->isWaitingTail())
         {
            SDB_ASSERT(0 != OSS_BIT_TEST(lrc->getHead()._flags,
                                         DPS_VESSEL_LOG_FLAG_OPL_TAIL), "impossible");
         }
         else
         {
            SDB_ASSERT(DPS_INVALID_LSN_OFFSET != lrc->getHead()._opListLSN, "impossible");
         }
         oplist->push(lrc->getLsn());
      }
   done:
      return rc;
   error:
      goto done;
   }

}//namespace vessel
}//namespace engine
