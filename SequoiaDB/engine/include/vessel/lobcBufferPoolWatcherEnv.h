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

   Source File Name = lobcBufferPoolWatcherEnv.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

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
