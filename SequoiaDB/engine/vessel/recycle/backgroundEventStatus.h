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

   Source File Name = backgroundEventStatus.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#ifndef VESSEL_BACKGROUND_EVENT_STATUS_H_
#define VESSEL_BACKGROUND_EVENT_STATUS_H_

#include "core.hpp"
#include "oss.hpp"
#include <mutex>//c++11
#include <condition_variable>//c++11

namespace engine
{
namespace vessel
{
   class backgroundEventStatus : public SDBObject
   {
      public:
         backgroundEventStatus();
         ~backgroundEventStatus();
         backgroundEventStatus(const backgroundEventStatus &) = delete;
         backgroundEventStatus &operator=(const backgroundEventStatus &) = delete;

      public:
         /// not thread safe
         void reset(UINT32 size);


         void finishOne();
         void abort();

         ///return false if aborted.
         /// lastStillRunning shoulde be inited as size at first.
         BOOLEAN wait(UINT32 lastStillRunning,
                      UINT32 currentSitllRunning);

         void waitForAll(BOOLEAN &aborted);

      private:
         BOOLEAN _everFinished()const
         {
            return _running < _size;
         }
         BOOLEAN _isRunning()const
         {
            return 0 < _running;
         }
      private:
         std::mutex _mutex;
         std::condition_variable _cv;
         UINT32 _size = 0;
         UINT32 _running = 0;
         BOOLEAN _aborted = FALSE;
   };//class backgroundEventStatus
}//namespace vessel
}//namespace engine

#endif//VESSEL_BACKGROUND_EVENT_STATUS_H_