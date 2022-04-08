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

#include <atomic> // c++11

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
         class options : public SDBObject
         {
            public:
               UINT32 cacheCleaner = 8;
               UINT32 commonWorker = 16;
         };//class options

      public:
         INT32 init(instanceEnv *env,
                    const options &o);
         void fini();

         void pushEvent(const backgroundEvent &event);

         void pushBufferEvent(const backgroundEvent &event);

         OSS_INLINE BOOLEAN isReady()const
         {
            return NULL != _env;
         }

         OSS_INLINE BOOLEAN isCommonFamilyBusy()const
         {
            INT32 count = (INT32)(_common._workers.size()) * 0.8f;
            return count <= _common._workingCounter.load(std::memory_order_relaxed);
         }

      private:
         INT32 _active(const options &o);
         void _deactive();

      private:
         typedef ossPoolList<backgroundWorker *> _WORKERS;
         struct _workerFamily : public SDBObject
         {
            void clear();
            autoEventList<backgroundEvent> _el;
            _WORKERS _workers;
            std::atomic_int _workingCounter = {0};
         };

      private:
         instanceEnv *_env = NULL;
         _workerFamily _cache;
         _workerFamily _common;
   };//class backgroundWorkers
}//namespace vessel
}//namespace engine

#endif//VESSEL_BACKGROUND_WORKERS_H_