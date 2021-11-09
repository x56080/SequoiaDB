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

   Source File Name = backgroundWorker.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#ifndef VESSEL_BACKGRUOND_WORKER_H_
#define VESSEL_BACKGRUOND_WORKER_H_

#include "vessel/backgroundEvent.h"
#include "vessel/autoEventList.hpp"
#include "vessel/backgroundEventMsg.h"
#include "sdbInterface.hpp"

namespace engine
{
namespace vessel
{
   class outerResource;
   class instanceEnv;

   class backgroundWorker : public SDBObject
   {
      public:
         backgroundWorker(){}
         ~backgroundWorker(){}
         backgroundWorker(const backgroundWorker &o):
         _outer(o._outer),
         _env(o._env),
         _executor(o._executor),
         _el(o._el){}

         backgroundWorker &operator=(const backgroundWorker &o)
         {
            _outer = o._outer;
            _env = o._env;
            _executor = o._executor;
            _el = o._el;
            return *this;
         }

      public:
         BOOLEAN isValid()const
         {
            return NULL != _outer &&
                   NULL != _env &&
                   NULL != _executor &&
                   NULL != _el;
         }
         void init(outerResource *outer,
                   instanceEnv *env,
                   IExecutor *executor,
                   autoEventList<backgroundEvent> *el);

         void activeEntry();

     

      private:
         void handleCacheEvent(diskIOTask &task,
                               autoEventList<backgroundEvent> *rl);
         void handleLpsCheckpointEvent(const lpsCheckpointApplying &msg,
                                       autoEventList<backgroundEvent> *rl);
         void handleLpsSegmentFlushing(const lpsFlushingSegments &msg,
                                       autoEventList<backgroundEvent> *rl);

      private:
         outerResource *_outer = NULL;
         instanceEnv *_env = NULL;
         IExecutor *_executor = NULL;
         autoEventList<backgroundEvent> *_el = NULL;
   };//class backgroundWorker
}//namespace vessel
}//namespce engine

#endif//VESSEL_BACKGRUOND_WORKER_H_