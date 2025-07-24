/*******************************************************************************

   Copyright (C) 2011-Present SequoiaDB Ltd.

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

      http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.

   Source File Name = backgroundWorker.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

*******************************************************************************/
#include "vessel/backgroundWorker.h"
#include "pdTrace.hpp"
#include "vessel/instanceEnv.h"
#include "vessel/threadContext.h"
#include "vessel/bufferFlushDef.h"

namespace engine
{
namespace vessel
{
   backgroundWorker::backgroundWorker(instanceEnv *env,
                                      autoEventList<backgroundEvent> *el,
                                      std::atomic_int *counter):
   _env(env),
   _el(el),
   _workingCounter(counter)
   {
      SDB_ASSERT(nullptr != env, "can not be invalid");
      SDB_ASSERT(nullptr != el, "can not be invalid");
   }

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

   INT32 backgroundWorker::active(BOOLEAN waitForAttaching)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(!isAttached(), "can not be attached");
      if (OSS_UNLIKELY(!isValid()))
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }

      rc = _env->resource.executorPool->startEDU(EDU_TYPE_VESSEL_WORKER, this);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to start new edu:%d", rc);
         goto error;
      }

      if (waitForAttaching)
      {
         _waitForAttaching();
      }
   done:
      return rc;
   error:
      goto done;
   }

   void backgroundWorker::attach(IExecutor *executor)
   {
      SDB_ASSERT(nullptr != executor, "can not be nullptr");
      SDB_ASSERT(isValid(), "can not be invalid");
      SDB_ASSERT(!isAttached(), "can not be attached");

      THREAD_CONTEXT_OWNER tco(executor, _env);
      THREAD_CONTEXT *tc = GET_THREAD_CONTEXT();
      backgroundEvent event;

      _setAttached();

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

         _handleEvent(event);

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
      _setDetached();
      return;
   }

   void backgroundWorker::_handleEvent(const backgroundEvent &e)
   {
      switch (e.getType())
      {
      case BACKGROUND_EVENT_TYPE::DATA_BUF_TASK:
         handleDataBufferEvent(e);
         break;
      case BACKGROUND_EVENT_TYPE::LOB_BUF_TASK:
         handleLobdBufferEvent(e);
         break;
      case BACKGROUND_EVENT_TYPE::HIT_ENTRY_TRANSFER:
         handleHitTransferEvent(e);
         break;
      default:
         PD_LOG(PDERROR, "unknown event type:%d", e.getType());
         break;
      }

      return;
   }

   void backgroundWorker::handleDataBufferEvent(const backgroundEvent &event)
   {
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

   void backgroundWorker::handleLobdBufferEvent(const backgroundEvent &event)
   {
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

   void backgroundWorker::handleHitTransferEvent(const backgroundEvent &event)
   {
      SDB_ASSERT(BACKGROUND_EVENT_TYPE::HIT_ENTRY_TRANSFER == event.getType(),
                 "can not be other type");
      UINT32 taskId = event.getShortData<UINT32>();
      INT32 rc = _env->hitMgr.executeTask(taskId);
      if (event.hasResponser())
      {
         event.getResponser()->push(event.createSimpleResponse(rc));
      }
   }

   void backgroundWorker::_waitForAttaching()
   {
      std::unique_lock<std::mutex> lock(_m);
      _cv.wait(lock, [this]{return isAttached();});
   }

   void backgroundWorker::_setAttached()
   {
      std::unique_lock<std::mutex> lock(_m);
      SDB_ASSERT(STATUS::DETACHED == _status, "can not be invalid");
      _status = STATUS::ATTACHED;
      _cv.notify_all();
   }

   void backgroundWorker::_waitForDetaching()
   {
      SDB_ASSERT(isValid(), "can not be invalid");
      std::unique_lock<std::mutex> lock(_m);
      _cv.wait(lock, [this]{return !isAttached();});
   }

   void backgroundWorker::_setDetached()
   {
      SDB_ASSERT(isValid(), "can not be invalid");
      std::unique_lock<std::mutex> lock(_m);
      SDB_ASSERT(STATUS::ATTACHED == _status, "can not be invalid");
      _status = STATUS::DETACHED;
      _cv.notify_all();
   }
}//namespace vessel
}//namespace engine
