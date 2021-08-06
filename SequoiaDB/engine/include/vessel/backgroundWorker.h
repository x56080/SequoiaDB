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
#include "ossThread.h"
#include "vessel/autoEventList.hpp"

namespace engine
{
namespace vessel
{
   class outerResource;
   class ISession;
   class instanceEnv;

   class backgroundWorker : public ossThread
   {
      public:
         backgroundWorker();
         virtual ~backgroundWorker();

      public:
         INT32 init(outerResource *resource,
                   instanceEnv *env,
                   autoEventList<backgroundEvent> *el);

         virtual void activeEntry();

      private:
         void fini();
         void handleCacheEvent(diskIOTask &task,
                               autoEventList<backgroundEvent> *rl);

      private:
         outerResource *_resource = NULL;
         instanceEnv *_env = NULL;
         ISession *_session = NULL;
         autoEventList<backgroundEvent> *_el = NULL;
   };//class backgroundWorker
}//namespace vessel
}//namespce engine

#endif//VESSEL_BACKGRUOND_WORKER_H_