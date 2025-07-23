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

   Source File Name = stpNetManager.cpp

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
#include "stpNetManager.hpp"
#include "stpCB.hpp"
#include "pdTrace.hpp"
#include "stpTrace.hpp"
#include "pmd.hpp"
#include "msgMessage.hpp"

using namespace bson ;
using namespace boost::asio ;

namespace engine
{

   /*
      _stpNetManager implement
    */
   _stpNetManager::_stpNetManager( stpNetMsgHandlerBase *handler )
   : _handler( handler ),
     _agent( handler )
   {
      SDB_ASSERT( NULL != handler, "handler is invalid" ) ;
   }

   _stpNetManager::~_stpNetManager()
   {
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__STPNETMGR_INITNETAGENT, "_stpNetManager::initNetAgent" )
   INT32 _stpNetManager::initNetAgent( const CHAR *hostName,
                                       const CHAR *serviceName,
                                       UINT32 protocolMask )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__STPNETMGR_INITNETAGENT ) ;

      SDB_ASSERT( NULL != hostName, "host name is invalid" ) ;
      SDB_ASSERT( NULL != serviceName, "service name is invalid" ) ;

      MsgRouteID routeID ;
      routeID.value = MSG_INVALID_ROUTEID ;

      rc = getRouteID( hostName, serviceName, routeID ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to get routeID for %s:%s, rc: %d",
                   hostName, serviceName, rc ) ;

      rc = updateRouteID( routeID, hostName, serviceName ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to update route ID for %s:%s, "
                   "rc: %d", hostName, serviceName, rc ) ;

      // listen on both TCP and UDP
      rc = _agent.listen( routeID, protocolMask ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to listen on port [%s], rc: %d",
                   serviceName, rc ) ;

      PD_LOG( PDEVENT, "Listening on port [%s]", serviceName ) ;

   done:
      PD_TRACE_EXITRC( SDB__STPNETMGR_INITNETAGENT, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__STPNETMGR_ACTIVENETAGENT, "_stpNetManager::activeNetAgent" )
   INT32 _stpNetManager::activeNetAgent()
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__STPNETMGR_ACTIVENETAGENT ) ;

      pmdEDUMgr *eduMgr = pmdGetKRCB()->getEDUMgr() ;
      EDUID eduID = PMD_INVALID_EDUID ;

      // start EDU for net agent
      rc = eduMgr->startEDU( EDU_TYPE_STP_NET_AGENT,
                             (void *)( &_agent ),
                             &eduID ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to start STP network EDU, rc: %d",
                   rc ) ;

      // wait until EDU is running
      rc = eduMgr->waitUntil( eduID, PMD_EDU_RUNNING ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to wait STP network to be running, "
                   "rc: %d", rc ) ;

   done:
      PD_TRACE_EXITRC( SDB__STPNETMGR_ACTIVENETAGENT, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__STPNETMGR_DEACTIVENETAGENT, "_stpNetManager::deactiveNetAgent" )
   INT32 _stpNetManager::deactiveNetAgent()
   {
      PD_TRACE_ENTRY( SDB__STPNETMGR_DEACTIVENETAGENT ) ;

      // shutdown and stop net agent
      _agent.shutdownListen() ;
      _agent.stop() ;

      PD_TRACE_EXITRC( SDB__STPNETMGR_DEACTIVENETAGENT, SDB_OK ) ;

      return SDB_OK ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__STPNETMGR_GETROUTEID, "_stpNetManager::getRouteID" )
   INT32 _stpNetManager::getRouteID( const CHAR *hostName,
                                     const CHAR *serviceName,
                                     MsgRouteID &routeID )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__STPNETMGR_GETROUTEID ) ;

      // get route ID for given host name and service name

      netUDPEndPoint udpEP ;

      // get remote end point by host and service
      rc = netRoute::getUDPEndPoint( hostName, serviceName, udpEP ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to resolve local UDP end point %s:%s, "
                   "rc: %d", hostName, serviceName, rc ) ;

      // get route ID from remote end point
      // - format IP into 32 bit integer as group ID
      // - format port to node ID
      // - always use local service
      routeID.columns.groupID = udpEP.address().to_v4().to_ulong() ;
      routeID.columns.nodeID = udpEP.port() ;
      routeID.columns.serviceID = MSG_ROUTE_LOCAL_SERVICE ;

   done:
      PD_TRACE_EXITRC( SDB__STPNETMGR_GETROUTEID, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__STPNETMGR_GETROUTEID_HANDLE, "_stpNetManager::getRouteID" )
   INT32 _stpNetManager::getRouteID( netRouteAgent *netAgent,
                                     const NET_HANDLE &handle,
                                     MsgRouteID &routeID )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__STPNETMGR_GETROUTEID_HANDLE ) ;

      SDB_ASSERT( NULL != netAgent, "net agent is invalid" ) ;

      NET_EH eh ;
      ip::address_v4 address ;
      UINT16 port = 0 ;

      eh = netAgent->getFrame()->getEventHandle( handle ) ;
      PD_CHECK( NULL != eh.get(), SDB_NET_INVALID_HANDLE, error, PDERROR,
                "Failed to get event handler for handle [%u]",
                handle ) ;

      address = eh->remoteIP() ;
      port = eh->remotePort() ;

      // get route ID from remote end point
      // - format IP into 32 bit integer as group ID
      // - format port to node ID
      // - always use local service
      routeID.columns.groupID = address.to_ulong() ;
      routeID.columns.nodeID = port ;
      routeID.columns.serviceID = MSG_ROUTE_LOCAL_SERVICE ;

   done:
      PD_TRACE_EXITRC( SDB__STPNETMGR_GETROUTEID_HANDLE, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__STPNETMGR_UPDATEROUTEID, "_stpNetManager::updateRouteID" )
   INT32 _stpNetManager::updateRouteID( const MsgRouteID &routeID,
                                        const CHAR *hostName,
                                        const CHAR *serviceName )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__STPNETMGR_UPDATEROUTEID ) ;

      // update route ID from net agent
      rc = _agent.updateRoute( routeID, hostName, serviceName ) ;
      if ( SDB_OK != rc && SDB_NET_UPDATE_EXISTING_NODE != rc )
      {
         PD_RC_CHECK( rc, PDERROR, "Failed to update route %s:%s for "
                      "route ID %s, rc: %d", hostName, serviceName,
                      routeID2String( routeID ).c_str(), rc ) ;
      }
      rc = SDB_OK ;

   done:
      PD_TRACE_EXITRC( SDB__STPNETMGR_UPDATEROUTEID, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__STPNETMGR_DELETEROUTEID, "_stpNetManager::deleteRouteID" )
   INT32 _stpNetManager::deleteRouteID( const MsgRouteID &routeID )
   {
      PD_TRACE_ENTRY( SDB__STPNETMGR_DELETEROUTEID ) ;

      // delete route ID from net agent
      _agent.delRoute( routeID ) ;

      PD_TRACE_EXITRC( SDB__STPNETMGR_DELETEROUTEID, SDB_OK ) ;

      return SDB_OK ;
   }

}
