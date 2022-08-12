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

   Source File Name = backgroundWorkers.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#ifndef VESSEL_BACKGROUND_WORKERS_H_
#define VESSEL_BACKGROUND_WORKERS_H_

#include "ossMemPool.hpp"
#include "vessel/autoEventList.hpp"
#include "vessel/backgroundEvent.h"
#include "vessel/backgroundWorker.h"

#include <atomic> // c++11

namespace engine
{
namespace vessel
{
   class instanceEnv;

   class backgroundWorkers : public SDBObject
   {
      public:
         backgroundWorkers(){}
         ~backgroundWorkers();
         backgroundWorkers(const backgroundWorkers &) = delete;
         backgroundWorkers &operator=(const backgroundWorkers &) = delete;

      public:
         struct options : public SDBObject
         {
            UINT32 maxWorkerNum = 8;
         };//class options

      public:
         INT32 init(instanceEnv *env,
                    const options &o);
         void fini();

         void pushEvent(const backgroundEvent &event);

         OSS_INLINE BOOLEAN isValid()const
         {
            return nullptr != _env;
         }

      private:
         INT32 _active(const options &o);
         void _deactive();

      private:
         using _WORKERS = std::vector<BACKGROUND_WORKER_UPTR>;

      private:
         instanceEnv *_env = nullptr;
         autoEventList<backgroundEvent> _el;
         std::atomic_int _workingCounter{0};
         _WORKERS _workers;
         UINT32 _actived = 0;
   };//class backgroundWorkers
}//namespace vessel
}//namespace engine

#endif//VESSEL_BACKGROUND_WORKERS_H_