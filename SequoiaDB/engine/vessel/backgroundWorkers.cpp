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

namespace engine
{
namespace vessel
{
   backgroundWorkers::~backgroundWorkers()
   {
      fini();
   }

   void backgroundWorkers::init(outerResource *resource,
                                instanceEnv *env,
                                UINT32 max)
   {
      SDB_ASSERT(NULL != resource, "can not be null");
      SDB_ASSERT(NULL != env, "can not be null");
      SDB_ASSERT(0 < max, "can not be zero");
      SDB_ASSERT(0 == _workers.size(), "do not reinit");
      _or = resource;
      _env = env;
      _workers.resize(max);
      _attached = 0;
      return;
   }

   void backgroundWorkers::attach(IExecutor *executor)
   {
      SDB_ASSERT(NULL != executor, "can not be null");
      SDB_ASSERT(NULL != _env, "can not be null");
      EDUID id = 0;
      UINT32 pos = 0;
      {
         ossScopedLock guard(&_latch);
         SDB_ASSERT(_attached < _workers.size(), "already full");
         backgroundWorker worker;
         worker.init(_or, _env, executor, &_el);
         SDB_ASSERT(!_workers.at(_attached).isValid(), "impossible");
         _workers[_attached] = worker;
         pos = _attached++;
      }

      id = executor->getID();
      PD_LOG(PDDEBUG, "worker[%lld] attached", id);

      /// loop handle event
      _workers[pos].activeEntry();

      /// quit
      _workers[pos] = backgroundWorker();
      {
         ossScopedLock guard(&_latch);
         SDB_ASSERT(0 < _attached, "impossible");
         --_attached;
      }

      PD_LOG(PDDEBUG, "worker[%lld] detached", id);
      return;
   }

   void backgroundWorkers::fini()
   {
      backgroundEvent event;
      event.setType(backgroundEvent::EVENT_TYPE_QUIT);
      for (UINT32 i = 0; i < _attached; ++i)
      {
         _el.pushPriority(event);
      }

      do
      {
         {
            ossScopedLock guard(&_latch);
            if (0 == _attached)
            {
               break;
            }
         }
         
         ossSleep(1000);
      } while (TRUE);

      while (_el.tryToPop(event))
      {
         event.release();
      }
      _or = NULL;
      _env = NULL;
      _workers.clear();
      _attached = 0;
      return;
   }

   void backgroundWorkers::pushEvent(const backgroundEvent &event)
   {
      SDB_ASSERT(NULL != _or, "not inited");
      SDB_ASSERT(backgroundEvent::EVENT_TYPE_INVALID != event.getType(), "can not be invalid");
      SDB_ASSERT(0 < _attached, "no worker attached");
      _el.push(event);
   }
}//namespace vessel
}//namespace engine
