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

   Source File Name = pmdAsyncNetEntryPoint.cpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          30/11/2012  Xu Jianhui  Initial Draft

   Last Changed =

*******************************************************************************/
#include "pmdEDUMgr.hpp"
#include "netRouteAgent.hpp"
#include "pd.hpp"
#include "pdTrace.hpp"
#include "pmdTrace.hpp"

namespace engine
{

   // PD_TRACE_DECLARE_FUNCTION ( SDB_PMDASYNCNETEP, "pmdAsyncNetEntryPoint" )
   INT32 pmdAsyncNetEntryPoint ( pmdEDUCB *cb, void *pData )
   {
      INT32 rc = SDB_OK;
      PD_TRACE_ENTRY ( SDB_PMDASYNCNETEP ) ;
      pmdEDUMgr *pEDUMgr = cb->getEDUMgr () ;
      _netRouteAgent *pRouteAgent = (_netRouteAgent *)pData;
      BOOLEAN hasReg = FALSE ;

      rc = pEDUMgr->activateEDU( cb ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG ( PDERROR, "Failed to active EDU[type:%d,ID:%lld]",
                  cb->getType(), cb->getID() ) ;
         goto error ;
      }

      //start run
      PD_LOG ( PDEVENT, "Run %s[Type: %d] ...", getEDUName( cb->getType() ),
               cb->getType() ) ;

      pEDUMgr->addIOService( pRouteAgent->ioservice() ) ;
      hasReg = TRUE ;
      try
      {
         pRouteAgent->run() ;
      }
      catch ( std::exception &e )
      {
         PD_LOG ( PDERROR, "Exception during start %s[Type: %d]: %s",
                  getEDUName( cb->getType() ), cb->getType(),
                  e.what() ) ;
         rc = SDB_SYS ;
         goto error ;
      }
      PD_LOG ( PDEVENT, "Stop %s[Type: %d]", getEDUName( cb->getType() ),
               cb->getType() ) ;

   done:
      if ( hasReg )
      {
         pEDUMgr->deleteIOService ( pRouteAgent->ioservice() ) ;
      }
      PD_TRACE_EXITRC ( SDB_PMDASYNCNETEP, rc );
      return rc;
   error:
      goto done;
   }

}

