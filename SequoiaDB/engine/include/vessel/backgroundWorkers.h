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
         INT32 init(outerResource *resource,
                    instanceEnv *env,
                    UINT32 workerCount);
         void fini();

         void pushEvent(const backgroundEvent &event);

         BOOLEAN isReady()const
         {
            return NULL != _or;
         }

      private:
         INT32 _active(UINT32 count);
         void _deactive();

      private:
         typedef ossPoolList<backgroundWorker *> _WORKERS;

      private:
         outerResource *_or = NULL;
         instanceEnv *_env = NULL;
         autoEventList<backgroundEvent> _el;
         _WORKERS _workers;
   };//class backgroundWorkers
}//namespace vessel
}//namespace engine

#endif//VESSEL_BACKGROUND_WORKERS_H_