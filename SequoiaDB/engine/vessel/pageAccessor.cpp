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
#include "vessel/vesselOptions.h"
#include "vessel/checkpointController.h"
#include "vessel/storageConsole.h"
#include "vessel/logicalPageSpace.h"
#include "vessel/requestContext.h"
#include "vessel/atomicOperationList.h"

namespace engine
{
namespace vessel
{
   INT32 pageAccessor::prepareLog(requestContext *context,
                                  runtimePageBuffer *rpb,
                                  UINT16 logType,
                                  logRecordContext *lrc)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(NULL != context, "can not be null");
      SDB_ASSERT(NULL != rpb, "can not be null");
      SDB_ASSERT(rpb->isValid(), "must be valid");
      SDB_ASSERT(rpb->isCacheBuffer(), "must be cache buffer");

      /// pid and psv may be different after writing prepared.
      SDB_ASSERT(rpb->isWritingPrepared(), "must be writing prepared");
      SDB_ASSERT(NULL != lrc, "can not be null");
      SDB_ASSERT(!lrc->needFullDump(), "can not be full dump");
      SDB_ASSERT(!lrc->prepared(), "can not be prepared");

      DPS_LSN_OFFSET lsn = DPS_INVALID_LSN_OFFSET;
      const checkpointController *checkpointer = &(context->getEnv()->checkpointer);
      const openDBOptions &options = context->getEnv()->options;

      lrc->open(logType);
      if (context->isInProcessingOplistAttached())
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

      if (!rpb->isResetPage() &&
          options.fullDumpPageLog &&
          checkpointer->hasAtLeastOneCheckpoint())
      {
         lsn = rpb->getPageHead()->lsn;
         SDB_ASSERT(DPS_INVALID_LSN_OFFSET != lsn, "impossible");
         if (lsn <= checkpointer->getLastCheckpointLSN())
         {
            rc = lrc->fullDumpPage(rpb->getPageSize(), rpb->getReadOnlyBuffer());
            if (SDB_OK != rc)
            {
               PD_LOG(PDERROR, "faile to full dump page:%d", rc);
               goto error;
            }
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
      SDB_ASSERT(NULL != lrc, "can not be null");
      SDB_ASSERT(!lrc->prepared(), "can not be prepared");

      ISession *session = context->getSession();
      IRedoLogger *logger = context->getOuterResource()->logger;

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

   void pageAccessor::abortLog(requestContext *context,
                               logRecordContext *lrc)
   {
      SDB_ASSERT(NULL != context, "can not be invalid");
      SDB_ASSERT(NULL != lrc, "can not be null");
      SDB_ASSERT(lrc->prepared(), "must be prepared");
      IRedoLogger *logger = context->getOuterResource()->logger;
      if (OSS_UNLIKELY(NULL != logger))
      {
         logger->abort(context->getSession(), lrc);
      }
      return;
   }

   INT32 pageAccessor::commitLog(requestContext *context,
                                 logRecordContext *lrc)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(NULL != context, "can not be null");
      SDB_ASSERT(lrc->prepared(), "can not be prepared");

      ISession *session = context->getSession();
      IRedoLogger *logger = context->getOuterResource()->logger;

      if (lrc->needFullDump())
      {
         rc = logger->pushLogRecordElement(session, lrc,
                                           DPS_LOG_PUBLIC_VESSEL_FULL_PAGE_DUMP,
                                           lrc->getFullDumpDataSize(),
                                           lrc->getFullDumpBuffer());
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

      if (context->isInProcessingOplistAttached())
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
