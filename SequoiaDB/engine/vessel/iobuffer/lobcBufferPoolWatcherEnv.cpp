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

   Source File Name = lobcBufferPoolWatcherEnv.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

*******************************************************************************/
#include "vessel/lobcBufferPoolWatcherEnv.h"

namespace engine
{
namespace vessel
{
   lobcBufferPoolWatcherEnv::lobcBufferPoolWatcherEnv()
   {

   }

   lobcBufferPoolWatcherEnv::~lobcBufferPoolWatcherEnv()
   {

   }

   void lobcBufferPoolWatcherEnv::resetLastFlushTime()
   {
      _lastFlushTime = _STEADY_CLOCK::now();
   }

   UINT32 lobcBufferPoolWatcherEnv::getTimeSpanFromLastFlush()const
   {
      _STEADY_CLOCK::time_point now = _STEADY_CLOCK::now();
      return std::chrono::duration_cast<std::chrono::milliseconds>
             (now - _lastFlushTime).count();
   }

   void lobcBufferPoolWatcherEnv::flushDone()
   {
      _flushList.clear();
      _taskBuilder.clear();
      _completedTaskNum = 0;
      resetLastFlushTime();
   }

   void lobcBufferPoolWatcherEnv::fini()
   {
      _flushList.clear();
      _taskBuilder.clear();
      _completedTaskNum = 0;
      _eventList.clear();
      _attached.store(0);
   }
} // namespace vessel

} // namespace engine

