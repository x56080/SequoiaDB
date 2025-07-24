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

   Source File Name = backgroundWorkers.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

*******************************************************************************/
#include "vessel/backgroundWorkers.h"
#include "pdTrace.hpp"
#include "ossLikely.hpp"
#include "vessel/instanceEnv.h"
#include "vessel/outerResource.h"
#include "pmdDef.hpp"

namespace engine
{
namespace vessel
{
   backgroundWorkers::~backgroundWorkers()
   {
      fini();
   }

   INT32 backgroundWorkers::init(instanceEnv *env,
                                 const options &o)
   {
      INT32 rc = SDB_OK;

      if (OSS_UNLIKELY(nullptr == env ||
                       0 == o.maxWorkerNum))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      _env = env;

      rc = _active(o);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to active workers:%d", rc);
         goto error;
      }
   done:
      return rc;
   error:
      fini();
      goto done;
   }

   INT32 backgroundWorkers::_active(const options &o)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(0 != o.maxWorkerNum, "can not be zero");
      SDB_ASSERT(_workers.empty(), "must be empty");
      SDB_ASSERT(0 == _actived, "must be zero");

      _workers.reserve(o.maxWorkerNum);
      for (UINT32 i = 0; i < o.maxWorkerNum; ++i)
      {
         BACKGROUND_WORKER_UPTR worker(SDB_OSS_NEW backgroundWorker());
         if (OSS_UNLIKELY(!worker))
         {
            PD_LOG(PDERROR, "failed to allocate mem.");
            rc = SDB_OOM;
            goto error;
         }

         worker->init(_env, &_el, &_workingCounter);
         _workers.push_back(std::move(worker)); 
      }

      for (UINT32 i = 0; i < _workers.size(); ++i)
      {
         BACKGROUND_WORKER_UPTR &worker = _workers[i];
         rc = worker->active(FALSE);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to start new worker:%d", rc);
            goto error;
         }
         ++_actived;
      }

      for (UINT32 i = 0; i < _workers.size(); ++i)
      {
         BACKGROUND_WORKER_UPTR &worker = _workers[i];
         worker->waitForAttaching();
      }

      PD_LOG(PDINFO, "[%d] background workers attached", _actived);
   done:
      return rc;
   error:
      for (UINT32 i = 0; i < _actived; ++i)
      {
         BACKGROUND_WORKER_UPTR &worker = _workers[i];
         worker->waitForAttaching();
      }

      _deactive();
      goto done;
   }

   void backgroundWorkers::fini()
   {
      if (nullptr != _env)
      {
         if (!_workers.empty())
         {
            _deactive();
         }
         _env = nullptr;
      }
      return;
   }

   void backgroundWorkers::pushEvent(const backgroundEvent &event)
   {
      SDB_ASSERT(event.isValid(), "can not be invalid");
      SDB_ASSERT(!event.isQuitEvent(), "can not be quit");
      SDB_ASSERT(!_workers.empty(), "no worker attached");
      _el.push(event);
   }

   void backgroundWorkers::_deactive()
   {
      SDB_ASSERT(nullptr != _env, "can not be null");
      backgroundEvent event = backgroundEvent::createQuitEvent();

      for (UINT32 i = 0; i < _actived; ++i)
      {
         _el.pushPriority(event);
      }

      for (UINT32 i = 0; i < _workers.size(); ++i)
      {
         BACKGROUND_WORKER_UPTR &worker = _workers[i];
         if (worker->isValid())
         {
            worker->waitForDetaching();
         }
      }

      _workers.clear();
      _el.clear();
      _workingCounter.store(0, std::memory_order_relaxed);
      PD_LOG(PDINFO, "[%d] background workers detached", _actived);
      _actived = 0;
      return;
   }
}//namespace vessel
}//namespace engine
