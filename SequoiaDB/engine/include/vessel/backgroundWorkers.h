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

   Source File Name = backgroundWorkers.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#ifndef VESSEL_BACKGROUND_WORKERS_H_
#define VESSEL_BACKGROUND_WORKERS_H_

#include "ossMemPool.hpp"
#include "vessel/autoEventList.hpp"
#include "vessel/backgroundEvent.h"
#include "vessel/backgroundWorker.h"
#include "ossLatch.hpp"
#include "ossAtomic.hpp"

namespace engine
{
namespace vessel
{
   class outerResource;
   class instanceEnv;

   class backgroundWorkers : public SDBObject
   {
      public:
         backgroundWorkers(){}
         ~backgroundWorkers();
         backgroundWorkers(const backgroundWorkers &) = delete;
         backgroundWorkers &operator=(const backgroundWorkers &) = delete;

      public:
         void init(outerResource *resource,
                   instanceEnv *env,
                   UINT32 max);
         void fini();

         void attach(IExecutor *executor);

         void pushEvent(const backgroundEvent &event);

         UINT32 getAttachedCount()const
         {
            return _attached;
         }
      private:
         typedef ossPoolVector<backgroundWorker> _WORKER_VEC;

      private:
         outerResource *_or = NULL;
         instanceEnv *_env = NULL;
         autoEventList<backgroundEvent> _el;
         ossSpinXLatch _latch;
         _WORKER_VEC _workers;
         UINT32 _attached = 0;
   };//class backgroundWorkers
}//namespace vessel
}//namespace engine

#endif//VESSEL_BACKGROUND_WORKERS_H_