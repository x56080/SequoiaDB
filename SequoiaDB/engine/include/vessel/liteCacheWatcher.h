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
         INT32 init(instanceEnv *env,
                    outerResource *outer);

         void fini();
         
         void attach(IExecutor *executor);
      private:
         INT32 _active();
         void _deactive();
         void _fini();
         void createJobIfNecessary(requestContext *context);
         void createDirtyListJobWhenTimeout(requestContext *context);
         void dispatch(requestContext *context, diskIOJob *job);
         void handleFinishedEvent(requestContext *context,
                                  const backgroundEvent &event);
         BOOLEAN hasRunningTask()const
         {
            return 0 < _runningTaskCount;
         }
      private:
         instanceEnv *_env = NULL;
         outerResource *_outer = NULL;
         UINT64 _lastFlushDirtyListTime = 0;
         autoEventList<backgroundEvent> _list;
         diskIOJob _job;
         UINT32 _runningTaskCount = 0;
         ossEvent _attachEvent;
         BOOLEAN _actived = FALSE;

   };//class liteCacheWatcher
}//namespace vessel
}//namespace engine

#endif//VESSEL_LITE_CACHE_WATCHER_H_