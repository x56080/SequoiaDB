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

   Source File Name = lobcBufferPoolWatcherEnv.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef VESSEL_LOBC_BUFFER_POOL_WATCHER_ENV_H_
#define VESSEL_LOBC_BUFFER_POOL_WATCHER_ENV_H_

#include "vessel/lobcFlushList.h"
#include "vessel/lobcFlushTaskBuilder.h"
#include "vessel/backgroundEvent.h"

#include <chrono> //c++11
#include <atomic> //c++11

namespace engine
{
namespace vessel
{
   class lobcBufferPoolWatcherEnv : public SDBObject
   {
      public:
         lobcBufferPoolWatcherEnv();
         ~lobcBufferPoolWatcherEnv();
         lobcBufferPoolWatcherEnv(const lobcBufferPoolWatcherEnv &) = delete;
         lobcBufferPoolWatcherEnv &operator=(const lobcBufferPoolWatcherEnv &) = delete;

      private:
         typedef std::chrono::steady_clock _STEADY_CLOCK;

      public:
         void fini();
         void resetLastFlushTime();

         ///milliseconds
         UINT32 getTimeSpanFromLastFlush()const;
         void flushDone();

      public:
         std::atomic_int _attached = {0};
         lobcFlushList _flushList;
         lobcFlushTaskBuilder _taskBuilder;
         UINT32 _completedTaskNum = 0;
         _STEADY_CLOCK::time_point _lastFlushTime; /// init when watcher attaching.
         autoEventList<backgroundEvent> _eventList;
   };
} // namespace vessel

} // namespace engine


#endif//VESSEL_LOBC_BUFFER_POOL_WATCHER_ENV_H_
