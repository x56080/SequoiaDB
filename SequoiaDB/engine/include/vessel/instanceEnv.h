/*******************************************************************************

   Copyright (C) 2011-Present SequoiaDB Ltd.

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

      http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.

   Source File Name = instanceEnv.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef VESSEL_INSTANCE_ENV_H_
#define VESSEL_INSTANCE_ENV_H_

#include "vessel/spaceIDLocker.h"
#include "vessel/vesselOptions.h"
#include "vessel/checkpointController.h"
#include "vessel/dataManagementService.h"
#include "vessel/backgroundWorkers.h"
#include "vessel/outerResource.h"
#include "vessel/sharedObjLatchEnv.h"
#include "vessel/lobChunkBufferPool.h"
#include "vessel/liteIOBufferPool.h"
#include "vessel/hitIndexManager.h"

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
         liteIOBufferPool ioBufferPool;
         lobChunkBufferPool lobcBufferPool;
         sharedObjLatchEnv latchEnv;
         lsmDB *lsm = nullptr;
         backgroundWorkers workers;
         outerResource resource;
         hitIndexManager hitMgr;

   }; /// end of class instanceEnv
} /// end of namespace vessel
} /// end of namespace engine

#endif // VESSEL_INSTANCE_ENV_H_