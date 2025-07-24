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

   Source File Name = stpServerSession.cpp

   Descriptive Name = Serial Time Protocol

   When/how to use: this program may be used on binary and text-formatted
   versions of STP component. This file contains structure for Serial Time
   Protocol.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          07/30/2019  HGM Initial Draft

   Last Changed =

*******************************************************************************/
#include "stpServerSession.hpp"
#include "stpCB.hpp"
#include "pdTrace.hpp"
#include "stpTrace.hpp"

namespace engine
{

   /*
      _stpServerSession implement
    */
   _stpServerSession::_stpServerSession( STPCB *stpCB )
   : _nodeManager( stpCB->getNodeManager() )
   {
      _curServerRID.value = MSG_INVALID_ROUTEID ;
   }

   _stpServerSession::~_stpServerSession()
   {
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__STPSERVERSESSION_GETPRIMARYRID, "_stpServerSession::getPrimaryRID" )
   INT32 _stpServerSession::getPrimaryRID( MsgRouteID &routeID )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__STPSERVERSESSION_GETPRIMARYRID ) ;

      // get primary from node manager
      rc = _nodeManager->chooseServerRID( _curServerRID, TRUE, routeID ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to get primary server route ID, "
                   "rc: %d", rc ) ;

      // copy route ID to current route ID
      _curServerRID.value = routeID.value ;

   done:
      PD_TRACE_EXITRC( SDB__STPSERVERSESSION_GETPRIMARYRID, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__STPSERVERSESSION_GETSERVERRID, "_stpServerSession::getServerRID" )
   INT32 _stpServerSession::getServerRID( MsgRouteID &routeID )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__STPSERVERSESSION_GETSERVERRID ) ;

      // get route ID of next server to the current route ID used before
      // from node manager
      rc = _nodeManager->chooseServerRID( _curServerRID, FALSE, routeID ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to get server route ID, "
                   "rc: %d", rc ) ;

      // copy route ID to current route ID
      _curServerRID.value = routeID.value ;

   done:
      PD_TRACE_EXITRC( SDB__STPSERVERSESSION_GETSERVERRID, rc ) ;
      return rc ;

   error:
      goto done ;
   }

}
