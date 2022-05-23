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
   void backgroundWorkers::_workerFamily::clear()
   {
      for (_WORKERS::const_iterator itr = _workers.begin();
           itr != _workers.end(); ++itr)
      {
         backgroundWorker *worker = *itr;
         if (NULL != worker)
         {
            SDB_OSS_DEL worker;
         }
      }

      _workers.clear();
      _workingCounter.store(0);
      _el.clear();
   }

   backgroundWorkers::~backgroundWorkers()
   {
      fini();
   }

   INT32 backgroundWorkers::init(instanceEnv *env,
                                 const options &o)
   {
      INT32 rc = SDB_OK;

      if (OSS_UNLIKELY(NULL == env ||
                       0 == o.bufferCleaner))
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
      SDB_ASSERT(0 != o.bufferCleaner, "can not be zero");
      SDB_ASSERT(_buffer._workers.empty(), "must be empty");

      for (UINT32 i = 0; i < o.bufferCleaner; ++i)
      {
         backgroundWorker *worker = SDB_OSS_NEW backgroundWorker();
         if (OSS_UNLIKELY(NULL == worker))
         {
            PD_LOG(PDERROR, "failed to allocate mem.");
            rc = SDB_OOM;
            goto error;
         }

         worker->init(_env, &_buffer._el, &_buffer._workingCounter);
         rc = _env->resource.executorPool->startEDU(EDU_TYPE_VESSEL_WORKER,
                                                    worker);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to start new worker:%d", rc);
            goto error;
         }

         worker->waitAttaching();
         _buffer._workers.push_back(worker); 
         
      }
      PD_LOG(PDINFO, "[%d] buffer cleaners attached", _buffer._workers.size());
   done:
      return rc;
   error:
      _deactive();
      goto done;
   }

   void backgroundWorkers::fini()
   {
      if (NULL != _env)
      {
         _deactive();
         SDB_ASSERT(_buffer._el.isEmpty(), "must be empty");
         _env = NULL;
      }
      return;
   }

   void backgroundWorkers::pushEvent(const backgroundEvent &event)
   {
      SDB_ASSERT(NULL != _env, "not inited");
      SDB_ASSERT(event.isValid(), "can not be invalid");
      SDB_ASSERT(!event.isQuitEvent(), "can not be quit");
      SDB_ASSERT(!_buffer._workers.empty(), "no worker attached");

      if (BACKGROUND_EVENT_TYPE::DATA_BUF_TASK == event.getType())
      {
         _buffer._el.push(event);
      }
   }

   void backgroundWorkers::pushBufferEvent(const backgroundEvent &event)
   {
      SDB_ASSERT(NULL != _env, "not inited");
      SDB_ASSERT(event.isUserRequest(), "can not be invalid");
      SDB_ASSERT(!_buffer._workers.empty(), "no worker attached");
      _buffer._el.push(event);
   }

   void backgroundWorkers::_deactive()
   {
      SDB_ASSERT(NULL != _env, "can not be null");
      backgroundEvent event = backgroundEvent::createQuitEvent();
      autoEventList<backgroundEvent> finishList;
      event.setResponser(&finishList);
      UINT32 count = _buffer._workers.size();

      for (UINT32 i = 0; i < _buffer._workers.size(); ++i)
      {
         _buffer._el.pushPriority(event);
      }

      for (UINT32 i = 0; i < _buffer._workers.size(); ++i)
      {
         backgroundEvent response;
         finishList.popOrWait(response);
         SDB_ASSERT(response.isResponseOf(BACKGROUND_EVENT_TYPE::QUIT), "impossible");
      }

      _buffer.clear();
      PD_LOG(PDINFO, "[%d] buffer cleaners detached", count);
      return;
   }
}//namespace vessel
}//namespace engine
