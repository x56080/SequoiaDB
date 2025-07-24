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

   Source File Name = pmdCoord.cpp

   Descriptive Name =

   When/how to use:

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          12/1/2014  ly  Initial Draft

   Last Changed =

*******************************************************************************/
#include "pmd.hpp"
#include "pmdEDU.hpp"
#include "pmdEDUMgr.hpp"
#include "netRouteAgent.hpp"
#include "pd.hpp"
#include "netTimer.hpp"
#include "pdTrace.hpp"
#include "pmdTrace.hpp"

namespace engine
{
   class coordTimeoutHandler : public _netTimeoutHandler
   {
      public:
         void handleTimeout( const UINT32 &millisec,
                             const UINT32 &id )
         {
            return ;
         }
   } ;

   // PD_TRACE_DECLARE_FUNCTION ( SDB_PMDCOORDNETWKENTPNT, "pmdCoordNetWorkEntryPoint" )
   INT32 pmdCoordNetWorkEntryPoint ( pmdEDUCB *cb, void *pData )
   {
      PD_TRACE_ENTRY ( SDB_PMDCOORDNETWKENTPNT ) ;
      pmdEDUMgr *pEduMgr = cb->getEDUMgr() ;
      _netRouteAgent *pNetWork = ( _netRouteAgent* )pData ;
      MsgRouteID coordRouteID ;
      coordTimeoutHandler timeHandler ;
      UINT32 timerid = 0 ;

      INT32 rc = pEduMgr->activateEDU( cb ) ;
      PD_RC_CHECK( rc, PDERROR, "Active edu[%s] failed, rc: %d",
                   getEDUName( cb->getType() ), rc ) ;

      coordRouteID.value = 0 ;
      pNetWork->setLocalID( coordRouteID ) ;

      rc = pNetWork->addTimer( 60000, &timeHandler, timerid ) ;
      PD_RC_CHECK( rc, PDERROR, "Add coord timer failed, rc: %d", rc ) ;

      PD_LOG ( PDEVENT, "Runing coord-network..." ) ;

      pEduMgr->addIOService( pNetWork->ioservice() );
      pNetWork->run();
      pNetWork->removeTimer( timerid ) ;
      pEduMgr->deleteIOService( pNetWork->ioservice() );

      PD_LOG ( PDEVENT, "Stop coord-network" ) ;

   done:
      PD_TRACE_EXIT ( SDB_PMDCOORDNETWKENTPNT );
      return SDB_OK ;
   error:
      goto done ;
   }

}

