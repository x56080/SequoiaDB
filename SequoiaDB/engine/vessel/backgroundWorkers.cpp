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

   INT32 backgroundWorkers::init(outerResource *resource,
                                instanceEnv *env,
                                UINT32 workerCount)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(NULL == _or, "do not reinit");

      if (OSS_UNLIKELY(NULL == resource ||
                       NULL == env ||
                       0 == workerCount))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      _or = resource;
      _env = env;

      rc = _active(workerCount);
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

   INT32 backgroundWorkers::_active(UINT32 count)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(NULL != _or, "can not be null");
      SDB_ASSERT(0 != count, "can not be zero");
      SDB_ASSERT(_workers.empty(), "must be empty");

      for (UINT32 i = 0; i < count; ++i)
      {
         backgroundWorker *worker = SDB_OSS_NEW backgroundWorker();
         if (OSS_UNLIKELY(NULL == worker))
         {
            PD_LOG(PDERROR, "failed to allocate mem.");
            rc = SDB_OOM;
            goto error;
         }

         worker->init(_or, _env, &_el, &_workingCounter);
         rc = _or->executorPool->startEDU(EDU_TYPE_VESSEL_WORKER,
                                          worker);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to start new worker:%d", rc);
            goto error;
         }

         worker->waitAttaching();
         _workers.push_back(worker); 
         PD_LOG(PDINFO, "[%d] workers attached", _workers.size());
      }
   done:
      return rc;
   error:
      _deactive();
      goto done;
   }

   void backgroundWorkers::fini()
   {
      if (NULL != _or)
      {
         _deactive();

         backgroundEvent e;
         while (_el.tryToPop(e))
         {
            /// do nothing
         } 
         _or = NULL;
         _env = NULL; 
      }
      return;
   }

   void backgroundWorkers::pushEvent(const backgroundEvent &event)
   {
      SDB_ASSERT(NULL != _or, "not inited");
      SDB_ASSERT(backgroundEvent::EVENT_TYPE_INVALID != event.getType(),
                 "can not be invalid");
      SDB_ASSERT(!event.isQuitEvent(), "can not be quit");
      SDB_ASSERT(!_workers.empty(), "no worker attached");
      _el.push(event);
   }

   void backgroundWorkers::_deactive()
   {
      SDB_ASSERT(NULL != _or, "can not be null");
      backgroundEvent event;
      autoEventList<backgroundEvent> finishList;
      event.setType(backgroundEvent::EVENT_TYPE_QUIT);
      event.setResponseList(&finishList);

      for (UINT32 i = 0; i < _workers.size(); ++i)
      {
         _el.pushPriority(event);
      }

      for (UINT32 i = 0; i < _workers.size(); ++i)
      {
         event.release();
         finishList.popOrWait(event);
         SDB_ASSERT(backgroundEvent::EVENT_TYPE_FINISHED == event.getType(), "impossible");
         PD_LOG(PDINFO, "[%d] workers detached", i+1);
      }

      for (_WORKERS::const_iterator itr = _workers.begin();
           itr != _workers.end(); ++itr)
      {
         backgroundWorker *worker = *itr;
         SDB_OSS_DEL worker;
      }

      _workers.clear();

      return;
   }
}//namespace vessel
}//namespace engine
