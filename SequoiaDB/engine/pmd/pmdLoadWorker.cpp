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

   Source File Name = pmdLoadWorker.cpp

   Descriptive Name =

   When/how to use: json convert bson,import to database file

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          06/05/2013  JW  Initial Draft

   Last Changed =

*******************************************************************************/
#include "pd.hpp"
#include "pmd.hpp"
#include "pdTrace.hpp"
#include "pmdTrace.hpp"
#include "pmdEDUMgr.hpp"
#include "migLoad.hpp"

namespace engine
{
   // PD_TRACE_DECLARE_FUNCTION ( SDB_PMDLOADWORKER, "pmdLoadWorkerEntryPoint" )
   INT32 pmdLoadWorkerEntryPoint ( pmdEDUCB *cb, void *pData )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB_PMDLOADWORKER );
      initWorker *dataWorker = NULL ;
      dmsStorageUnit *su     = NULL ;

      dataWorker = (initWorker *)pData ;
      migWorker worker( dataWorker->pMaster ) ;
      su = dataWorker->pSu ;
      rc = worker.importData( dataWorker->masterEDUID,
                              su,
                              dataWorker->collectionID,
                              dataWorker->clLID,
                              dataWorker->isAsynchr,
                              cb ) ;
      if ( rc )
      {
         PD_LOG ( PDERROR, "Failed to import data, rc=%d", rc ) ;
         goto error ;
      }
   done:
      PD_TRACE_EXITRC ( SDB_PMDLOADWORKER, rc );
      return rc ;
   error:
      goto done ;
   }

   /// Register
   PMD_DEFINE_ENTRYPOINT( EDU_TYPE_LOADWORKER, FALSE,
                          pmdLoadWorkerEntryPoint,
                          "MigLoadWork" ) ;

}
