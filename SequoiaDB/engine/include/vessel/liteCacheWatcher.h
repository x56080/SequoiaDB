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

   Source File Name = liteCacheWatcher.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#ifndef VESSEL_LITE_CACHE_WATCHER_H_
#define VESSEL_LITE_CACHE_WATCHER_H_

#include "vessel/diskIOJob.h"
#include "vessel/autoEventList.hpp"
#include "vessel/backgroundEvent.h"
#include "ossEvent.hpp"
#include "sdbInterface.hpp"

#include <atomic> // c++11
namespace engine
{
namespace vessel
{
   class instanceEnv;
   class outerResource;
   class requestContext;

   class liteCacheWatcher : public SDBObject
   {
   public:
         liteCacheWatcher(){}
         ~liteCacheWatcher(){}
         liteCacheWatcher(const liteCacheWatcher &) = delete;
         liteCacheWatcher &operator=(const liteCacheWatcher &) = delete;

      public:
         INT32 init(instanceEnv *env);

         void fini();
         
         void attach(IExecutor *executor);

         void notify();
      private:
         enum _JOB_ID
         {
            _JOB_ID_DIRTY_LIST = 0,
            _JOB_ID_LRU = 1,
            _JOB_ID_MAX = _JOB_ID_LRU,
            _JOG_ID_COUNT = _JOB_ID_MAX + 1,
         };//enum _JOB_ID

      private:
         INT32 _active();
         void _deactive();
         void _fini();
         void createJobIfNecessary(requestContext *context);
         void createDirtyListJobWhenTimeout(requestContext *context);
         void dispatch(requestContext *context, _JOB_ID jid);
         void handleFinishedEvent(requestContext *context,
                                  const backgroundEvent &event);

         void tryToTrimLRU(requestContext *context);
         void tryToFlushDirtyList(requestContext *context);
         void flushDirtyListWhenTimeout(requestContext *context);

         BOOLEAN _hasRunningTask()const
         {
            return 0 != _jobs[_JOB_ID_DIRTY_LIST].runningTask ||
                   0 != _jobs[_JOB_ID_LRU].runningTask;
         }

      

      private:
         struct _JOB_CONTEXT
         {
            void clear()
            {
               runningTask = 0;
               job.reset();
            }

            UINT32 runningTask = 0;
            diskIOJob job;
         };//struct _JOB_CONTEXT
      private:
         instanceEnv *_env = NULL;
         UINT64 _lastFlushDirtyListTime = 0;
         autoEventList<backgroundEvent> _list;
         _JOB_CONTEXT _jobs[_JOG_ID_COUNT];
         ossEvent _attachEvent;
         BOOLEAN _actived = FALSE;
         std::atomic_flag _notifyFlag = ATOMIC_FLAG_INIT;

   };//class liteCacheWatcher
}//namespace vessel
}//namespace engine

#endif//VESSEL_LITE_CACHE_WATCHER_H_