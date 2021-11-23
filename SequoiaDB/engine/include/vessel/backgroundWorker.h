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
#include "ossEvent.hpp"

#include <atomic> // c++11

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
         backgroundWorker(const backgroundWorker &o) = delete;
         backgroundWorker &operator=(const backgroundWorker &o) = delete;

      public:
         BOOLEAN isValid()const
         {
            return NULL != _outer &&
                   NULL != _env &&
                   NULL != _el;
         }
         void init(outerResource *outer,
                   instanceEnv *env,
                   autoEventList<backgroundEvent> *el,
                   std::atomic_int *counter);

         void activeEntry(IExecutor *executor);

         void waitAttaching();

      private:
         void handleCacheEvent(IExecutor *executor,
                               backgroundEvent &event);
         void handleLpsCheckpointEvent(IExecutor *executor,
                                       backgroundEvent &event);
         void handleLpsSegmentFlushing(IExecutor *executor,
                                       backgroundEvent &event);

      private:
         outerResource *_outer = NULL;
         instanceEnv *_env = NULL;
         autoEventList<backgroundEvent> *_el = NULL;
         ossEvent _attachEvent;
         std::atomic_int *_workingCounter = NULL;
   };//class backgroundWorker
}//namespace vessel
}//namespce engine

#endif//VESSEL_BACKGRUOND_WORKER_H_