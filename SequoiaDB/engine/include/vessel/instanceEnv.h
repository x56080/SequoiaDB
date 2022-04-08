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

   Source File Name = instanceEnv.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#ifndef VESSEL_INSTANCE_ENV_H_
#define VESSEL_INSTANCE_ENV_H_

#include "vessel/spaceIDLocker.h"
#include "vessel/liteCache.h"
#include "vessel/vesselOptions.h"
#include "vessel/checkpointController.h"
#include "vessel/dataManagementService.h"
#include "vessel/liteCacheConsole.h"
#include "vessel/lsm/lsmDB.hpp"
#include "vessel/backgroundWorkers.h"
#include "vessel/liteCacheWatcher.h"
#include "vessel/outerResource.h"
#include "vessel/sharedObjLatchEnv.h"
#include "vessel/lobChunkBufferPool.h"

namespace engine
{
namespace vessel
{
   class lsmDB;

   class instanceEnv : public SDBObject
   {
      public:
         instanceEnv(){}
         ~instanceEnv(){}
         instanceEnv(const instanceEnv &) = delete;
         instanceEnv &operator=(const instanceEnv &) = delete;

      public:
         openDBOptions options;
         checkpointController checkpointer;
         spaceIDLocker spaceLocker;
         dataManagementService dms;
         liteCacheConsole cacheConsole;
         lobChunkBufferPool lobcBufferPool;
         sharedObjLatchEnv latchEnv;
         lsmDB *lsm = nullptr;

         ///TODO: move cache watcher into lite cache
         liteCacheWatcher cacheWatcher;
         backgroundWorkers workers;
         outerResource resource;

   }; /// end of class instanceEnv
} /// end of namespace vessel
} /// end of namespace engine

#endif // VESSEL_INSTANCE_ENV_H_