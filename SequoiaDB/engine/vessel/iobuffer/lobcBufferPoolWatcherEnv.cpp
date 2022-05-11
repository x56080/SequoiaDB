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

   Source File Name = lobcBufferPoolWatcherEnv.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

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

