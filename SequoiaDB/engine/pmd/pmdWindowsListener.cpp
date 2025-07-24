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

   Source File Name = pmdWindowsListener.cpp

   Descriptive Name = Process MoDel Windows Listener

   When/how to use: this program may be used on binary and text-formatted
   versions of PMD component. This file contains entry point for local listener
   that only avaliable on Windows.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/14/2012  TW  Initial Draft

   Last Changed =

*******************************************************************************/
#include <stdio.h>
#include "pd.hpp"
#include "pmd.hpp"
#include "pmdEDUMgr.hpp"
#include "pmdPipeManager.hpp"
#include "pdTrace.hpp"
#include "pmdTrace.hpp"

namespace engine
{

   // PD_TRACE_DECLARE_FUNCTION ( SDB_PMDPIPELSTNNPNTPNT, "pmdPipeListenerEntryPoint" )
   INT32 pmdPipeListenerEntryPoint( pmdEDUCB *cb, void *pData )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_PMDPIPELSTNNPNTPNT ) ;

      pmdEDUMgr *eduMgr = cb->getEDUMgr() ;
      pmdPipeManager *pipeManager = (pmdPipeManager *)pData ;

      PD_CHECK( pipeManager->isInitialized(), SDB_SYS, error, PDERROR,
                "Failed to start pipe manager [%s], it is not initialized",
                pipeManager->getServiceName() ) ;

      rc = eduMgr->activateEDU( cb ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to activate EDU for pipe manager [%s], "
                   "rc: %d", pipeManager->getServiceName(), rc ) ;

      rc = pipeManager->active() ;
      PD_RC_CHECK( rc, PDERROR, "Failed to active pipe manager [%s], rc: %d",
                   pipeManager->getServiceName(), rc ) ;

      rc = pipeManager->run( cb ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to run pipe manager [%s], rc: %d",
                   pipeManager->getServiceName(), rc ) ;

   done:
      pipeManager->deactive() ;

      PD_TRACE_EXITRC( SDB_PMDPIPELSTNNPNTPNT, rc ) ;
      return rc ;

   error:
      switch ( rc )
      {
         case SDB_SYS :
            PD_LOG( PDSEVERE, "System error occured" ) ;
            break ;
         default :
            PD_LOG( PDSEVERE, "Internal error" ) ;
      }

      PD_LOG( PDSEVERE, "Shutdown database" ) ;
      PMD_SHUTDOWN_DB( rc ) ;

      goto done ;
   }

   /// Register
   PMD_DEFINE_ENTRYPOINT( EDU_TYPE_PIPESLISTENER, TRUE,
                          pmdPipeListenerEntryPoint,
                          "PipeListener" ) ;
}
