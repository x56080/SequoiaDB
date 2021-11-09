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
#include "vessel/requestContext.h"
#include "vessel/diskIOTask.h"

namespace engine
{
namespace vessel
{
   void liteCacheWatcher::active(requestContext *context)
   {
      SDB_ASSERT(NULL != context && context->isOpen(), "can not be null");
      SDB_ASSERT(!isActived(), "do not reactive");
      UINT32 millis = 10;
      backgroundEvent event;
      diskIOJob _job;
      BOOLEAN quit = FALSE;
      UINT32 flushDirtyListTimeout =
      context->getEnv()->options.cacheOptions.flush.flushDirtyListTimeout * 1000;

      _actived = TRUE;
      _lastFlushDirtyListTime = ossGetCurrentMilliseconds();

      do
      {
         if (_list.popOrWaitFor(millis, event))
         {
            SDB_ASSERT(backgroundEvent::EVENT_TYPE_INVALID != event.getType(), "impossible");
            if (event.isQuitEvent())
            {
               quit = TRUE;
            }
            else if (backgroundEvent::EVENT_TYPE_FINISHED == event.getType())
            {
               handleFinishedEvent(context, event);
               if (!quit && !hasRunningTask())
               {
                  createJobIfNecessary(context);
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
            if (hasRunningTask())
            {
               continue;
            }
            
            SDB_ASSERT(!_job.isRunning(), "impossible");
            UINT64 currentTime = ossGetCurrentMilliseconds();
            if ((_lastFlushDirtyListTime + flushDirtyListTimeout) <= currentTime)
            {
               createDirtyListJobWhenTimeout(context);
            }
            else
            {
               createJobIfNecessary(context);
            }
         }
      } while (!quit || hasRunningTask());

      fini();
      return;
   }

   void liteCacheWatcher::deactive()
   {
      if (isActived())
      {
         backgroundEvent event;
         event.setType(backgroundEvent::EVENT_TYPE_QUIT);
         _list.push(event);
         const volatile BOOLEAN *actived = &_actived;

         while ((*actived))
         {
            PD_LOG(PDINFO, "waiting for running task[%d] done", _runningTaskCount);
            ossSleep(1000);
         }
      }

      return;
   }
   
   void liteCacheWatcher::fini()
   {
      backgroundEvent event;
      while (_list.tryToPop(event))
      {
         event.release();
      }
      _lastFlushDirtyListTime = 0;
      _job.reset();
      _runningTaskCount = 0;
      _actived = FALSE;
   }

   void liteCacheWatcher::createJobIfNecessary(requestContext *context)
   {
      SDB_ASSERT(!_job.isRunning(), "can not be running");

      INT32 rc = SDB_OK;
      liteCache &cache = context->getEnv()->cacheConsole.get32KBCache();
      rc = cache.createIOJobIfNecessary(context, &_job);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to create io job on cache:%d", rc);
         goto done;
      }

      if (0 < _job.getTagCount())
      {
         dispatch(context, &_job);
      }
      else
      {
         _job.reset();
      }

   done:
      return;
   }

   void liteCacheWatcher::createDirtyListJobWhenTimeout(requestContext *context)
   {
      SDB_ASSERT(!_job.isRunning(), "can not be running");
      INT32 rc = SDB_OK;
      liteCache &cache = context->getEnv()->cacheConsole.get32KBCache();
      UINT32 depth = cache.getDirtyListSizeFast() * 0.3;
      if (depth < 128)
      {
         depth = 128;
      }

      rc = cache.createDirtyListIOJob(context, depth, DPS_INVALID_LSN_OFFSET, &_job);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to create io job on dirty list:%d", rc);
         goto done;
      }

      if (0 < _job.getTagCount())
      {
         dispatch(context, &_job);
      }
      else
      {
         _job.reset();
      }

   done:
      return;
   }

   void liteCacheWatcher::handleFinishedEvent(requestContext *context,
                                              const backgroundEvent &event)
   {
      SDB_ASSERT(backgroundEvent::EVENT_TYPE_FINISHED == event.getType(), "msut be finished");
      const UINT32 *jobID = (const UINT32 *)(event.getEventMsg());
      if (0 < _runningTaskCount && (*jobID == _job.getJobID()))
      {
         if (0 == --_runningTaskCount)
         {
            if (_job.isDirtyListJob())
            {
               context->getEnv()->cacheConsole.get32KBCache().updateMinCacheLsn();
               /// reset dirty list flushting time.
               _lastFlushDirtyListTime = ossGetCurrentMilliseconds();
            }
            else
            {
               context->getEnv()->cacheConsole.get32KBCache().resetLRUEvictBegin();
            }
            _job.reset();
         }
      }
      return;
   }

   void liteCacheWatcher::dispatch(requestContext *context, diskIOJob *job)
   {
      SDB_ASSERT(NULL != job, "can not be null");
      SDB_ASSERT(job->isRunning(), "must be running");

      BOOLEAN hitTheEnd = FALSE;
      INT32 rc = SDB_OK;

      job->prepareForDispatching();

      do
      {
         backgroundEvent event;
         diskIOTask task;
         rc = job->getNextTask(context, hitTheEnd, task);
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
         context->getEnv()->workers.pushEvent(event);
         ++_runningTaskCount;
      } while (TRUE);
      
   done:
      if (SDB_OK != rc)
      {
         job->abortUndispatchedTasks();
      }
      return;
   }
}//namespace vessel
}//namespace engine