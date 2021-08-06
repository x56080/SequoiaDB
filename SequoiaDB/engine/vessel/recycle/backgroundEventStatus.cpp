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

   Source File Name = backgroundEventStatus.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/
#include "vessel/backgroundEventStatus.h"
#include "pdTrace.hpp"

namespace engine
{
namespace vessel
{
   backgroundEventStatus::backgroundEventStatus()
   {}

   backgroundEventStatus::~backgroundEventStatus()
   {}

   void backgroundEventStatus::reset(UINT32 size)
   {
      SDB_ASSERT(0 < size, "can not be zero");
      _size = size;
      _running = size;
      _aborted = FALSE;
   }

   void backgroundEventStatus::finishOne()
   {
      std::unique_lock<std::mutex> lk(_mutex);
      SDB_ASSERT(0 < _size, "not inited");
      /// will not be running if aborted.
      if (_isRunning())
      {
         --_running;
         lk.unlock();
         _cv.notify_all();
      }
      return;
   }

   void backgroundEventStatus::abort()
   {
      std::unique_lock<std::mutex> lk(_mutex);
      SDB_ASSERT(0 < _size, "not inited");
      _running = 0;
      _aborted = TRUE;
      lk.unlock();
      _cv.notify_all();
   }

   BOOLEAN backgroundEventStatus::wait()
   {
      BOOLEAN r = FALSE;
      std::unique_lock<std::mutex> lk(_mutex);
      if (_isRunning())
      {
         _cv.wait(lk);
         r = _isRunning();
      }
      
      return r;
   }

   void backgroundEventStatus::waitForAll()
   {
      std::unique_lock<std::mutex> lk(_mutex);
      _cv.wait(lk, [this]{return !_isRunning();});
      return;
   }
}//namespace vessel
}//namespace engine