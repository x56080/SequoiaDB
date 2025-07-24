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

   Source File Name = pmdVesselEDU.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

*******************************************************************************/
#include "pd.hpp"
#include "pmd.hpp"
#include "pdTrace.hpp"
#include "pmdTrace.hpp"
#include "pmdEDUMgr.hpp"

#include "vessel/backgroundWorker.h"
#include "vessel/vesselImpl.h"

namespace engine
{
   INT32 pmdVesselWorkerEntryPoint(pmdEDUCB *cb, void *pData)
   {
      INT32 rc = SDB_OK;
      vessel::backgroundWorker *worker = NULL;
      rc = cb->getEDUMgr()->activateEDU(cb);
      if ( SDB_OK != rc )
      {
         PD_LOG ( PDERROR, "Failed to active EDU" ) ;
         goto error ;
      }

      worker = (vessel::backgroundWorker *)pData;
      worker->attach(cb);
   done:
      return rc;
   error:
      goto done;
   }

   INT32 pmdVesselLitePoolWatcherEntryPoint(pmdEDUCB *cb, void *pData)
   {
      INT32 rc = SDB_OK;
      vessel::vesselImpl *impl = nullptr;
      rc = cb->getEDUMgr()->activateEDU(cb);
      if ( SDB_OK != rc )
      {
         PD_LOG ( PDERROR, "Failed to active EDU" ) ;
         goto error ;
      }

      impl = (vessel::vesselImpl *)pData;
      impl->attachLiteBufferPoolWatcher(cb);

   done:
      return rc;
   error:
      goto done;
   }

   INT32 pmdVesselLobcWatcherEntryPoint(pmdEDUCB *cb, void *pData)
   {
      INT32 rc = SDB_OK;
      vessel::vesselImpl *impl = nullptr;
      rc = cb->getEDUMgr()->activateEDU(cb);
      if ( SDB_OK != rc )
      {
         PD_LOG ( PDERROR, "Failed to active EDU" ) ;
         goto error ;
      }

      impl = (vessel::vesselImpl *)pData;
      impl->attachLobcWatcher(cb);

   done:
      return rc;
   error:
      goto done;
   }

   INT32 pmdVesselHitManagerEntryPoint(pmdEDUCB *cb, void *pData)
   {
      INT32 rc = SDB_OK;
      vessel::vesselImpl *impl = nullptr;
      rc = cb->getEDUMgr()->activateEDU(cb);
      if ( SDB_OK != rc )
      {
         PD_LOG ( PDERROR, "Failed to active EDU" ) ;
         goto error ;
      }

      impl = (vessel::vesselImpl *)pData;
      impl->attachHitManager(cb);

   done:
      return rc;
   error:
      goto done;
   }

   PMD_DEFINE_ENTRYPOINT(EDU_TYPE_VESSEL_WORKER, FALSE,
                         pmdVesselWorkerEntryPoint,
                         "vesselWorker");

   PMD_DEFINE_ENTRYPOINT(EDU_TYPE_VESSEL_LITE_BUFFER_POOL_WATCHER, FALSE,
                         pmdVesselLitePoolWatcherEntryPoint,
                         "vesselLiteBufferPoolWatcher");

   PMD_DEFINE_ENTRYPOINT(EDU_TYPE_VESSEL_LOBC_BUFFER_POOL_WATCHER, FALSE,
                         pmdVesselLobcWatcherEntryPoint,
                         "vesselLobcBufferPoolWatcher");

   PMD_DEFINE_ENTRYPOINT(EDU_TYPE_VESSEL_HIT_MANAGER, FALSE,
                         pmdVesselHitManagerEntryPoint,
                         "vesselHitManager");
} // namespace engine
