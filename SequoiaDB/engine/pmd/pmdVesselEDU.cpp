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

   Source File Name = pmdVesselEDU.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#include "pd.hpp"
#include "pmd.hpp"
#include "pdTrace.hpp"
#include "pmdTrace.hpp"
#include "pmdEDUMgr.hpp"

#include "vessel/liteCacheWatcher.h"
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
      worker->activeEntry(cb);
   done:
      return rc;
   error:
      goto done;
   }

   INT32 pmdVesselWatcherEntryPoint(pmdEDUCB *cb, void *pData)
   {
      INT32 rc = SDB_OK;
      vessel::liteCacheWatcher *watcher = NULL;
      rc = cb->getEDUMgr()->activateEDU(cb);
      if ( SDB_OK != rc )
      {
         PD_LOG ( PDERROR, "Failed to active EDU" ) ;
         goto error ;
      }

      watcher = (vessel::liteCacheWatcher *)pData;
      watcher->attach(cb);

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

   PMD_DEFINE_ENTRYPOINT(EDU_TYPE_VESSEL_WORKER, FALSE,
                         pmdVesselWorkerEntryPoint,
                         "vesselWorker");

   PMD_DEFINE_ENTRYPOINT(EDU_TYPE_VESSEL_CACHE_WATCHER, FALSE,
                         pmdVesselWatcherEntryPoint,
                         "vesselCacheWatcher");
} // namespace engine
