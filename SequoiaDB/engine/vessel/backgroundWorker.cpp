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

   Source File Name = backgroundWorker.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#include "vessel/backgroundWorker.h"
#include "vessel/ISesseionManager.h"
#include "vessel/ISession.h"
#include "pdTrace.hpp"
#include "vessel/diskIOTask.h"
#include "vessel/instanceEnv.h"
#include "vessel/requestContext.h"
#include "vessel/outerResource.h"
#include "vessel/diskIOJob.h"

namespace engine
{
namespace vessel
{
   backgroundWorker::backgroundWorker()
   {}

   backgroundWorker::~backgroundWorker()
   {}

   INT32 backgroundWorker::init(outerResource *resource,
                               instanceEnv *env,
                               autoEventList<backgroundEvent> *el)
   {
      INT32 rc = SDB_OK;
      if (NULL == resource ||
          NULL == env ||
          NULL == el)
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      _session = resource->sessionMgr->createNewSession();
      if (NULL == _session)
      {
         PD_LOG(PDERROR, "failed to create new session");
         rc = SDB_VESSEL_INTERNAL_ERR;
         goto error;
      }

      _resource = resource;
      _env = env;
      _el = el;

   done:
      return rc;
   error:
      goto done;
   }

   void backgroundWorker::activeEntry()
   {
      SDB_ASSERT(NULL != _resource, "can not be null");
      backgroundEvent event;

      do
      {
         _el->popOrWait(event);
         SDB_ASSERT(backgroundEvent::EVENT_TYPE_INVALID != event.getType(), "impossible");
         if (event.isQuitEvent())
         {
            PD_LOG(PDINFO, "get quit event, exit");
            break;
         }
         else if (backgroundEvent::EVENT_TYPE_CACHE_TASK == event.getType())
         {
            diskIOTask *task = (diskIOTask *)(event.getEventMsg());
            handleCacheEvent(*task, event.getReponseList());
         }
         else
         {
            SDB_ASSERT(FALSE, "invalid type");
         }
      } while (TRUE);
      
      fini();
   }

   void backgroundWorker::fini()
   {
      if (NULL != _session)
      {
         _resource->sessionMgr->destroySession(_session);
      }
      _resource = NULL;
      _env = NULL;
      _session = NULL;
      _el = NULL;
      return;
   }

   void backgroundWorker::handleCacheEvent(diskIOTask &task,
                                           autoEventList<backgroundEvent> *rl)
   {
      liteCache &cache = _env->cacheConsole.get32KBCache();
      
      requestContext context;
      UINT32 jobID = task.getJob()->getJobID();
      context.open(_session, _env, _resource);
      INT32 rc = cache.executeIOTask(&context, &task);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to execute io task:%d", rc);
      }
      if (NULL != rl)
      {
         backgroundEvent res;
         res.setType(backgroundEvent::EVENT_TYPE_FINISHED);
         res.setEventMsg(sizeof(UINT32), &jobID);
         rl->push(res);
      }
   }
}//namespace vessel
}//namespace engine
