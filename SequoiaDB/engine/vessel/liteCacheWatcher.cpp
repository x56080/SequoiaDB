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

   Source File Name = liteCacheWatcher.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#include "vessel/liteCacheWatcher.h"
#include "vessel/instanceEnv.h"
#include "vessel/outerResource.h"
#include "pdTrace.hpp"
#include "vessel/liteCache.h"
#include "vessel/diskIOTask.h"
#include "pmdDef.hpp"
#include "vessel/threadContext.h"

namespace engine
{
namespace vessel
{
   INT32 liteCacheWatcher::init(instanceEnv *env)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(nullptr == _env, "do not reinit");
      if (OSS_UNLIKELY(nullptr == env))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      _env = env;

      rc = _active();
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to active watcher:%d", rc);
         goto error;
      }

   done:
      return rc;
   error:
      _fini();
      goto done;
   }

   void liteCacheWatcher::fini()
   {
      if (_actived)
      {
         _deactive();
      }

      _fini();
      return;
   }

   INT32 liteCacheWatcher::_active()
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(nullptr != _env, "can not be null");
      SDB_ASSERT(!_actived, "do not reactive");

      _attachEvent.reset();
      rc = _env->resource.executorPool->startEDU(EDU_TYPE_VESSEL_CACHE_WATCHER,
                                                this);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to start new watcher:%d", rc);
         goto error;
      }

      _attachEvent.wait();
      SDB_ASSERT(_actived, "must be actived");
   done:
      return rc;
   error:
      goto done;
   }

   void liteCacheWatcher::attach(IExecutor *executor)
   {
      SDB_ASSERT(nullptr != executor, "can not be null");
      SDB_ASSERT(nullptr != _env, "can not be null");
      SDB_ASSERT(!_actived, "already been actived");
      THREAD_CONTEXT_OWNER tco(executor, _env);
      THREAD_CONTEXT *tc = GET_THREAD_CONTEXT();

      UINT32 millis = 10;
      backgroundEvent event;
      diskIOJob _job;
      BOOLEAN quit = FALSE;
      UINT32 flushDirtyListTimeout =
      _env->options.cacheOptions.flush.flushDirtyListTimeout * 1000;
      _lastFlushDirtyListTime = ossGetCurrentMilliseconds();

      _actived = TRUE;
      _attachEvent.signalAll();

      do
      {
         if (_list.popOrWaitFor(millis, event))
         {
            SDB_ASSERT(backgroundEvent::EVENT_TYPE_INVALID != event.getType(), "impossible");
            if (event.isQuitEvent())
            {
               quit = TRUE;
            }
            else if (backgroundEvent::EVENT_TYPE_CACHE_WATCHER_NOTIFY == event.getType())
            {
               //PD_LOG(PDDEBUG, "get notification from lite cache");
               if (!quit)
               {
                  createJobIfNecessary();
               }
            }
            else if (backgroundEvent::EVENT_TYPE_FINISHED == event.getType())
            {
               handleFinishedEvent(event);
               if (!quit)
               {
                  createJobIfNecessary();
               }
            }
            else
            {
               PD_LOG(PDERROR, "invalid event type found in list:%d", event.getType());
            }
         }
         else
         {
            /// timeout
            if (!_hasRunningTask())
            {
               createJobIfNecessary();
            }
            
            if (0 == _jobs[_JOB_ID_DIRTY_LIST].runningTask)
            {
               UINT64 currentTime = ossGetCurrentMilliseconds();
               if ((_lastFlushDirtyListTime + flushDirtyListTimeout) <= currentTime)
               {
                  flushDirtyListWhenTimeout();
               }
            }
         }

         SDB_ASSERT(!tc->hasUnfreeBuffer(), "buffer should be free at the end of loop");
      } while (!quit || _hasRunningTask());

      _actived = FALSE;
      _attachEvent.signalAll();
      return;
   }

   void liteCacheWatcher::_deactive()
   {
      SDB_ASSERT(_actived, "not actived yet");
      _attachEvent.reset();
      backgroundEvent event;
      event.setType(backgroundEvent::EVENT_TYPE_QUIT);
      _list.push(event);

      do
      {
         INT32 timeout = _attachEvent.wait(1000, nullptr);
         if (SDB_OK == timeout)
         {
            PD_LOG(PDINFO, "cache watcher deactived");
            break;
         }
         else
         {
            PD_LOG(PDINFO, "waiting for watcher deactived, running task[%d]",
                  _jobs[_JOB_ID_DIRTY_LIST].runningTask + _jobs[_JOB_ID_LRU].runningTask);
         }
      } while (TRUE);

      SDB_ASSERT(!_actived, "still be actived");

      return;
   }
   
   void liteCacheWatcher::_fini()
   {
      backgroundEvent event;
      while (_list.tryToPop(event))
      {
         event.release();
      }

      for (UINT32 i = 0; i < (UINT32)_JOG_ID_COUNT; ++i)
      {
         _jobs[i].clear();
      }
      _lastFlushDirtyListTime = 0;
      _attachEvent.reset();
      _actived = FALSE;
      _env = nullptr;
      _notifyFlag.clear();
   }

   void liteCacheWatcher::createJobIfNecessary()
   {
      
      if (!_jobs[_JOB_ID_LRU].job.isRunning())
      {
         tryToTrimLRU();
      }

      if (!_jobs[_JOB_ID_DIRTY_LIST].job.isRunning())
      {
         tryToFlushDirtyList();
      }

   done:
      return;
   }

   void liteCacheWatcher::tryToTrimLRU()
   {

      liteCache *cache = _env->cacheConsole.getCacheByPoolNo();
      INT32 rc = SDB_OK;
      diskIOJob &job = _jobs[_JOB_ID_LRU].job;
      SDB_ASSERT(!job.isRunning(), "can not be running");
      BOOLEAN paused = FALSE;
      UINT32 evicted = 0;

      /// pause first
      _notifyFlag.test_and_set(std::memory_order_acquire);
      paused = TRUE;

      rc = cache->autoTrimLRU(&job, evicted);
      if (SDB_OK != rc)
      {
         PD_LOG(PDSEVERE, "failed to trim lru:%d", rc);
         goto done;
      }

      if (0 < job.getTagCount())
      {  
         dispatch(_JOB_ID_LRU);
         /// do not notify watcher when job is running.
         paused = FALSE;  
      }
      else
      {
         job.reset();
      }
   done:
      if (paused)
      {
         _notifyFlag.clear(std::memory_order_release);
      }
      return;
   }

   void liteCacheWatcher::tryToFlushDirtyList()
   {
      liteCache *cache = _env->cacheConsole.getCacheByPoolNo();
      INT32 rc = SDB_OK;
      diskIOJob &job = _jobs[_JOB_ID_DIRTY_LIST].job;
      SDB_ASSERT(!job.isRunning(), "can not be running");

      rc = cache->createElasticDirtyListJob(&job);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to create dirty list io job:%d", rc);
         goto done;
      }

      if (0 < job.getTagCount())
      {
         dispatch(_JOB_ID_DIRTY_LIST);
      }
      else
      {
         job.reset();
      }

   done:
      return;
   }

   void liteCacheWatcher::flushDirtyListWhenTimeout()
   {
      liteCache *cache = _env->cacheConsole.getCacheByPoolNo();
      INT32 rc = SDB_OK;
      diskIOJob &job = _jobs[_JOB_ID_DIRTY_LIST].job;
      SDB_ASSERT(!job.isRunning(), "can not be running");
      UINT32 depth = cache->getDirtyListSizeFast() * 0.3;
      if (depth < 128)
      {
         depth = 128;
      }

      rc = cache->createDirtyListIOJob(depth,
                                      DPS_INVALID_LSN_OFFSET,
                                      &job);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to create io job on dirty list:%d", rc);
         goto done;
      }

      if (0 < job.getTagCount())
      {
         dispatch(_JOB_ID_DIRTY_LIST);
      }
      else
      {
         job.reset();
      }

   done:
      return;
   }

   void liteCacheWatcher::handleFinishedEvent(const backgroundEvent &event)
   {
      SDB_ASSERT(backgroundEvent::EVENT_TYPE_FINISHED == event.getType(), "msut be finished");
      const UINT32 *jobID = (const UINT32 *)(event.getEventMsg());
      liteCache *cache = _env->cacheConsole.getCacheByPoolNo();

      for (UINT32 i = 0; i < (UINT32)_JOG_ID_COUNT; ++i)
      {
         _JOB_CONTEXT &jc = _jobs[(_JOB_ID)i];
         if (jc.job.isRunning() &&
            jc.job.getJobID() == *jobID)
         {
            SDB_ASSERT(0 != jc.runningTask, "impossible");
            if (0 == --jc.runningTask)
            {
               if (jc.job.isDirtyListJob())
               {
                  cache->updateMinCacheLsn();
                  /// reset dirty list flushting time.
                  _lastFlushDirtyListTime = ossGetCurrentMilliseconds();
                  //PD_LOG(PDDEBUG, "dirty list job done, tag count:%d", jc.job.getTagCount());
               }
               else
               {
                  cache->resetLRUEvictBegin();
                  //PD_LOG(PDDEBUG, "lru job done, tag count:%d", jc.job.getTagCount());
               }

               jc.job.reset();
            }

            break;
         }
      }

   done:
      return;
   }

   void liteCacheWatcher::dispatch(_JOB_ID jid)
   {
      SDB_ASSERT(jid <= _JOB_ID_MAX, "out of bound");

      BOOLEAN hitTheEnd = FALSE;
      INT32 rc = SDB_OK;

      _JOB_CONTEXT &jc = _jobs[jid];
      SDB_ASSERT(0 == jc.runningTask, "must be zero");
      jc.runningTask = 0;
      SDB_ASSERT(0 < jc.job.getTagCount(), "can not be empty");
      jc.job.prepareForDispatching();

      do
      {
         backgroundEvent event;
         diskIOTask task;
         rc = jc.job.getNextTask(hitTheEnd, task);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to get the next task:%d", rc);
            break;
         }

         if (hitTheEnd)
         {
            break;
         }

         event.setType(backgroundEvent::EVENT_TYPE_CACHE_TASK);
         event.setEventMsg(sizeof(diskIOTask), &task);
         event.setResponseList(&_list);
         _env->workers.pushEvent(event);
         ++jc.runningTask;
      } while (TRUE);
      
   done:
      if (SDB_OK != rc)
      {
         jc.job.abortUndispatchedTasks();
      }
      return;
   }

   void liteCacheWatcher::notify()
   {
      SDB_ASSERT(_actived, "must be actived");
      if (!_notifyFlag.test_and_set(std::memory_order_acquire))
      {
         backgroundEvent event;
         event.setType(backgroundEvent::EVENT_TYPE_CACHE_WATCHER_NOTIFY);
         _list.push(event);
      }
   }
}//namespace vessel
}//namespace engine