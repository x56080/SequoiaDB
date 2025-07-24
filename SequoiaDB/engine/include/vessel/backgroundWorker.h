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

   Source File Name = backgroundWorker.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef VESSEL_BACKGRUOND_WORKER_H_
#define VESSEL_BACKGRUOND_WORKER_H_

#include "vessel/backgroundEvent.h"
#include "vessel/autoEventList.hpp"
#include "vessel/backgroundEventMsg.h"
#include "sdbInterface.hpp"

#include <mutex>
#include <condition_variable>
#include <atomic> // c++11

namespace engine
{
namespace vessel
{
   class instanceEnv;

   class backgroundWorker : public SDBObject
   {
      public:
         backgroundWorker() = default;
         backgroundWorker(instanceEnv *env,
                          autoEventList<backgroundEvent> *el,
                          std::atomic_int *counter);
         ~backgroundWorker() = default;
         backgroundWorker(const backgroundWorker &o) = delete;
         backgroundWorker &operator=(const backgroundWorker &o) = delete;

      public:
         enum class STATUS : INT32
         {
            DETACHED = 0,
            ATTACHED = 1,
         };

      public:
         OSS_INLINE BOOLEAN isValid()const
         {
            return nullptr != _env &&
                   nullptr != _el;
         }
         OSS_INLINE BOOLEAN isAttached()const {return STATUS::ATTACHED == _status;}

         void init(instanceEnv *env,
                   autoEventList<backgroundEvent> *el,
                   std::atomic_int *counter);

         INT32 active(BOOLEAN waitForAttaching);

         void waitForDetaching() {_waitForDetaching();}
         void waitForAttaching() {_waitForAttaching();}

      public:
         /// callback function!
         void attach(IExecutor *executor);

      private:
         void _handleEvent(const backgroundEvent &e);

      private:
         void _waitForAttaching();
         void _waitForDetaching();
         void _setAttached();
         void _setDetached();

      private:
         void handleDataBufferEvent(const backgroundEvent &event);
         void handleLobdBufferEvent(const backgroundEvent &event);
         void handleHitTransferEvent(const backgroundEvent &event);
      private:
         instanceEnv *_env = nullptr;
         autoEventList<backgroundEvent> *_el = nullptr;
         STATUS _status{STATUS::DETACHED};
         std::mutex _m;
         std::condition_variable _cv;
         std::atomic_int *_workingCounter = nullptr;
   };//class backgroundWorker

   using BACKGROUND_WORKER_UPTR = std::unique_ptr<backgroundWorker>;
}//namespace vessel
}//namespce engine

#endif//VESSEL_BACKGRUOND_WORKER_H_