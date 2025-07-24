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

   Source File Name = backgroundWorkers.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

*******************************************************************************/
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