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

   Source File Name = backgroundWorkers.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

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
