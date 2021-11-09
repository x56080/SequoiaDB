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

   Source File Name = backgroundWorker.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#include "vessel/backgroundWorker.h"
#include "pdTrace.hpp"
#include "vessel/diskIOTask.h"
#include "vessel/instanceEnv.h"
#include "vessel/requestContext.h"
#include "vessel/outerResource.h"
#include "vessel/diskIOJob.h"

namespace engine
{
namespace vessel
{
   void backgroundWorker::init(outerResource *outer,
                               instanceEnv *env,
                               IExecutor *executor,
                               autoEventList<backgroundEvent> *el)
   {
      SDB_ASSERT(NULL != outer, "can not be null");
      SDB_ASSERT(NULL != env, "can not be null");
      SDB_ASSERT(NULL != executor, "can not be null");
      SDB_ASSERT(NULL != el, "can not be null");

      _outer = outer;
      _env = env;
      _executor = executor;
      _el = el;
      return;
   }

   void backgroundWorker::activeEntry()
   {
      SDB_ASSERT(NULL != _executor, "can not be null");
      backgroundEvent event;

      do
      {
         _el->popOrWait(event);
         SDB_ASSERT(backgroundEvent::EVENT_TYPE_INVALID != event.getType(), "impossible");

         switch (event.getType())
         {
         case backgroundEvent::EVENT_TYPE_QUIT:
         {
            PD_LOG(PDINFO, "get quit event, exit");
            goto done;
         }
         case backgroundEvent::EVENT_TYPE_CACHE_TASK:
         {
            diskIOTask *task = (diskIOTask *)(event.getEventMsg());
            handleCacheEvent(*task, event.getReponseList());
            break;
         }
         case backgroundEvent::EVENT_TYPE_SYNC_SEG:
         {
            const lpsFlushingSegments *msg = (const lpsFlushingSegments *)(event.getEventMsg());
            handleLpsSegmentFlushing(*msg, event.getReponseList());
            break;
         }
         case backgroundEvent::EVENT_TYPE_LPS_CHECKPOINT:
         {
            const lpsCheckpointApplying *msg = (const lpsCheckpointApplying *)(event.getEventMsg());
            handleLpsCheckpointEvent(*msg, event.getReponseList()); 
            break;
         }
         default:
            SDB_ASSERT(FALSE, "invalid type");
            break;
         }
      } while (TRUE);

   done:
      return;
   }

   void backgroundWorker::handleCacheEvent(diskIOTask &task,
                                           autoEventList<backgroundEvent> *rl)
   {
      liteCache &cache = _env->cacheConsole.get32KBCache();
      
      requestContext context;
      UINT32 jobID = task.getJob()->getJobID();
      context.open(_executor, _env, _outer);
      INT32 rc = cache.executeIOTask(&context, &task);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to execute io task:%d", rc);
      }
      if (NULL != rl)
      {
         backgroundEvent res;
         res.setType(backgroundEvent::EVENT_TYPE_FINISHED);
         res.setEventMsg(sizeof(UINT32), &jobID);
         rl->push(res);
      }
   }

   void backgroundWorker::handleLpsCheckpointEvent(const lpsCheckpointApplying &msg,
                                                   autoEventList<backgroundEvent> *rl)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(INVALID_SPACE_ID != msg._sid &&
                 INVALID_SPACE_TYPE != msg._type, "can not be invalid");
      logicalPageSpace *lps = NULL;
      requestContext context;

      context.open(_executor, _env, _outer);
      rc = context.lockSpaceID(msg._sid, SHARED);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to lock space[%d], rc:%d", msg._sid, rc);
         goto done;
      }

      rc = _env->dms.getLogicalPageSpace(msg._sid, msg._type, &lps);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to get lps[%d,%d], rc:%d", msg._sid, msg._type, rc);
         goto done;
      }

      rc = lps->createCheckpoint(&context);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to create checkpoint on lps[%d,%d], rc:%d",
                msg._sid, msg._type, rc);
         goto done;
      }
   
   done:
      context.close();
      if (NULL != rl)
      {
         backgroundEvent res;
         res.setType(backgroundEvent::EVENT_TYPE_FINISHED);
         rl->push(res);
      }
      return;
   }

   void backgroundWorker::handleLpsSegmentFlushing(const lpsFlushingSegments &msg,
                                                   autoEventList<backgroundEvent> *rl)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(INVALID_SPACE_ID != msg._sid &&
                 INVALID_SPACE_TYPE != msg._type, "can not be invalid");
      logicalPageSpace *lps = NULL;
      requestContext context;

      context.open(_executor, _env, _outer);
      rc = context.lockSpaceID(msg._sid, SHARED);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to lock space[%d], rc:%d", msg._sid, rc);
         goto done;
      }

      rc = _env->dms.getLogicalPageSpace(msg._sid, msg._type, &lps);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to get lps[%d,%d], rc:%d", msg._sid, msg._type, rc);
         goto done;
      }

      rc = lps->fsyncSegment(msg._segmentId);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to flush segment[%d] on lps[%d,%d], rc:%d",
                msg._segmentId, msg._sid, msg._type, rc);
         goto done;
      }
   
   done:
      context.close();
      if (NULL != rl)
      {
         backgroundEvent res;
         res.setType(backgroundEvent::EVENT_TYPE_FINISHED);
         rl->push(res);
      }
   }
}//namespace vessel
}//namespace engine
