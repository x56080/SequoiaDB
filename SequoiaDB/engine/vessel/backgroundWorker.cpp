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
#include "vessel/instanceEnv.h"
#include "vessel/diskIOTask.h"
#include "vessel/diskIOJob.h"
#include "vessel/threadContext.h"
#include "vessel/requestContext.h"
#include "vessel/lobcFlushTaskBuilder.h"
#include "vessel/lobChunkBufferPool.h"

namespace engine
{
namespace vessel
{
   void backgroundWorker::init(instanceEnv *env,
                               autoEventList<backgroundEvent> *el,
                               std::atomic_int *counter)
   {
      SDB_ASSERT(nullptr != env, "can not be nullptr");
      SDB_ASSERT(nullptr != el, "can not be nullptr");

      _env = env;
      _el = el;
      _workingCounter = counter;
      return;
   }

   void backgroundWorker::waitAttaching()
   {
      _attachEvent.wait();
      return;
   }

   void backgroundWorker::activeEntry(IExecutor *executor)
   {
      SDB_ASSERT(nullptr != executor, "can not be nullptr");
      SDB_ASSERT(nullptr != _env, "can not be nullptr");
      THREAD_CONTEXT_OWNER tco(executor, _env);
      THREAD_CONTEXT *tc = GET_THREAD_CONTEXT();
      backgroundEvent event;
      _attachEvent.signalAll();

      do
      {
         _el->popOrWait(event);
         SDB_ASSERT(event.isValid(), "impossible");
         

         if (event.isQuitEvent())
         {
            //PD_LOG(PDDEBUG, "get quit event, exit");
            goto done;
         }

         if (nullptr != _workingCounter)
         {
            _workingCounter->fetch_add(1, std::memory_order_relaxed);
         }

         switch (event.getType())
         {
         case BACKGROUND_EVENT_TYPE::DATA_BUF_TASK:
         {
            handleCacheEvent(executor, event);
            break;
         }
         case BACKGROUND_EVENT_TYPE::FLUSH_SEG:
         {
            handleLpsSegmentFlushing(executor, event);
            break;
         }
         case BACKGROUND_EVENT_TYPE::LPS_CHECKPOINT:
         {
            handleLpsCheckpointEvent(executor, event); 
            break;
         }
         case BACKGROUND_EVENT_TYPE::LOB_BUF_TASK:
         {
            handleLobdBufferEvent(executor, event);
            break;
         }
         default:
            SDB_ASSERT(FALSE, "invalid type");
            break;
         }

         if (nullptr != _workingCounter)
         {
            _workingCounter->fetch_sub(1, std::memory_order_relaxed);
         }
         event.reset();

         SDB_ASSERT(!tc->hasUnfreeBuffer(), "should be free at the end of loop");
      } while (TRUE);

   done:
      SDB_ASSERT(event.isQuitEvent(), "must be quit");
      if (event.hasResponser())
      {
         backgroundEvent quitRes;
         quitRes.initAsResponse(BACKGROUND_EVENT_TYPE::QUIT);
         event.getResponser()->push(quitRes);
      }
      return;
   }

   void backgroundWorker::handleCacheEvent(IExecutor *executor,
                                           backgroundEvent &event)
   {
      SDB_ASSERT(nullptr != executor, "can not be invalid");
      SDB_ASSERT(BACKGROUND_EVENT_TYPE::DATA_BUF_TASK == event.getType(),
                 "can not be other type");

      liteCache *cache = _env->cacheConsole.getCacheByPoolNo();
      diskIOTask task = event.getShortData<diskIOTask>();

      UINT32 jobID = task.getJob()->getJobID();

      INT32 rc = cache->executeIOTask(&task);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to execute io task:%d", rc);
      }

      if (event.hasResponser())
      {
         backgroundEvent res;
         res.initAsResponse(BACKGROUND_EVENT_TYPE::DATA_BUF_TASK);
         res.setRC(rc);
         res.setShortData(jobID);
         event.getResponser()->push(res);
      }

      return;
   }

   void backgroundWorker::handleLpsCheckpointEvent(IExecutor *executor,
                                                   backgroundEvent &event)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(nullptr != executor, "can not be invalid");
      SDB_ASSERT(BACKGROUND_EVENT_TYPE::LPS_CHECKPOINT == event.getType(),
                 "can not be other type");
      SDB_ASSERT(!event.hasResponser(), "impossible");

      logicalPageSpace *lps = nullptr;
      const lpsCheckpointApplying &msg = event.getShortData<lpsCheckpointApplying>();
      requestContext context;
      
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

      rc = lps->createCheckpoint(&context, FALSE);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to create checkpoint on lps[%d,%d], rc:%d",
                msg._sid, msg._type, rc);
         goto done;
      }
   
   done:
      return;
   }

   void backgroundWorker::handleLpsSegmentFlushing(IExecutor *executor,
                                                   backgroundEvent &event)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(nullptr != executor, "can not be invalid");
      SDB_ASSERT(BACKGROUND_EVENT_TYPE::FLUSH_SEG == event.getType(),
                 "can not be other type");
      logicalPageSpace *lps = nullptr;
      const lpsFlushingSegments *msg = &(event.getShortData<lpsFlushingSegments>());
      UINT32 count = msg->_count;

      //PD_LOG(PDDEBUG, "begin to sync segments[%d, %d]", msg->_segmentId, count);
      requestContext context;

      rc = context.lockSpaceID(msg->_sid, SHARED);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to lock space[%d], rc:%d", msg->_sid, rc);
         goto done;
      }

      rc = _env->dms.getLogicalPageSpace(msg->_sid, msg->_type, &lps);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to get lps[%d,%d], rc:%d", msg->_sid, msg->_type, rc);
         goto done;
      }

      for (UINT32 i = 0; i < count; ++i)
      {
         rc = lps->fsyncSegment(msg->_segmentId + i);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to flush segment[%d] on lps[%d,%d], rc:%d",
                  msg->_segmentId, msg->_sid, msg->_type, rc);
            goto done;
         }
      }
   
   done:
      context.close();
      if (event.hasResponser())
      {
         backgroundEvent res;
         res.initAsResponse(BACKGROUND_EVENT_TYPE::FLUSH_SEG);
         res.setRC(rc);
         res.setShortData(msg->_segmentId);
         event.getResponser()->push(res);
      }
   }

   void backgroundWorker::handleLobdBufferEvent(IExecutor *executor,
                                                backgroundEvent &event)
   {
      SDB_ASSERT(nullptr != executor, "can not be invalid");
      SDB_ASSERT(BACKGROUND_EVENT_TYPE::LOB_BUF_TASK == event.getType(),
                 "can not be other type");
      lobcFlushTaskBuilder::taskId taskId =
             event.getShortData<lobcFlushTaskBuilder::taskId>();
      lobChunkBufferPool &pool = _env->lobcBufferPool;
      INT32 rc = pool.executeFlushTask(taskId);
      if (event.hasResponser())
      {
         event.getResponser()->push(event.createSimpleResponse(rc));
      }
   }
}//namespace vessel
}//namespace engine
