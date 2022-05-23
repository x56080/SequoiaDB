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
#include "vessel/threadContext.h"
#include "vessel/requestContext.h"
#include "vessel/bufferFlushDef.h"

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
            handleDataBufferEvent(executor, event);
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

   void backgroundWorker::handleDataBufferEvent(IExecutor *executor,
                                           backgroundEvent &event)
   {
      SDB_ASSERT(nullptr != executor, "can not be invalid");
      SDB_ASSERT(BACKGROUND_EVENT_TYPE::DATA_BUF_TASK == event.getType(),
                 "can not be other type");
      bufferFlushTaskId taskId =
             event.getShortData<bufferFlushTaskId>();
      liteIOBufferPool &pool = _env->ioBufferPool;
      INT32 rc = pool.executeFlushTask(taskId);
      if (event.hasResponser())
      {
         event.getResponser()->push(event.createSimpleResponse(rc));
      }
      return;
   }

   void backgroundWorker::handleLobdBufferEvent(IExecutor *executor,
                                                backgroundEvent &event)
   {
      SDB_ASSERT(nullptr != executor, "can not be invalid");
      SDB_ASSERT(BACKGROUND_EVENT_TYPE::LOB_BUF_TASK == event.getType(),
                 "can not be other type");
      bufferFlushTaskId taskId =
             event.getShortData<bufferFlushTaskId>();
      lobChunkBufferPool &pool = _env->lobcBufferPool;
      INT32 rc = pool.executeFlushTask(taskId);
      if (event.hasResponser())
      {
         event.getResponser()->push(event.createSimpleResponse(rc));
      }
   }
}//namespace vessel
}//namespace engine
