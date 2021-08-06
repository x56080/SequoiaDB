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
#include "vessel/ISesseionManager.h"
#include "vessel/ISession.h"
#include "pdTrace.hpp"
#include "ossLikely.hpp"
#include "vessel/instanceEnv.h"
#include "vessel/outerResource.h"

namespace engine
{
namespace vessel
{
   backgroundWorkers::backgroundWorkers()
   {}

   backgroundWorkers::~backgroundWorkers()
   {
      fini();
   }

   INT32 backgroundWorkers::init(outerResource *resource,
                                 instanceEnv *env,
                                 UINT32 workerCount)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(0 == _workerCount, "do not reinit");
      if (OSS_UNLIKELY(NULL == resource ||
                       !resource->isValid() ||
                       NULL == env ||
                       0 == workerCount ||
                       128 < workerCount))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      _or = resource;
      _env = env;
      _workerCount = workerCount;
      _workers = SDB_OSS_NEW backgroundWorker[workerCount];
      if (NULL == _workers)
      {
         PD_LOG(PDERROR, "failed to allocate mem");
         rc = SDB_OOM;
         goto error;
      }

      for (UINT32 i = 0; i < workerCount; ++i)
      {
         INT32 rc = _workers[i].init(_or, _env, &_el);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to init worker:%d", rc);
            goto error;
         }
      }

      for (UINT32 i = 0; i < workerCount; ++i)
      {
         _workers[i].active();
         ++_actived;
      }
      
   done:
      return rc;
   error:
      fini();
      goto done;
   }

   void backgroundWorkers::fini()
   {
      backgroundEvent event;
      if (0 <_actived)
      {
         event.setType(backgroundEvent::EVENT_TYPE_QUIT);
         for (UINT32 i = 0; i < _actived; ++i)
         {
            _el.pushPriority(event);
         }

         for (UINT32 i = 0; i < _actived; ++i)
         {
            _workers[i].join();
         }
      }

      if (NULL != _workers)
      {
         SDB_OSS_DEL []_workers;
         _workers = NULL;
      }

      while (_el.tryToPop(event))
      {
         event.release();
      }
      _or = NULL;
      _env = NULL;
      _workerCount = 0;
      _actived = 0;
      return;
   }

   void backgroundWorkers::pushEvent(const backgroundEvent &event)
   {
      SDB_ASSERT(0 < _actived, "not inited");
      SDB_ASSERT(backgroundEvent::EVENT_TYPE_INVALID != event.getType(), "can not be invalid");
      _el.push(event);
   }
}//namespace vessel
}//namespace engine
