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

   Source File Name = stpSyncSource.cpp

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
#include "stpSyncSource.hpp"
#include "stpCB.hpp"
#include "pdTrace.hpp"
#include "stpTrace.hpp"
#include "pmd.hpp"
#include "msgMessage.hpp"

using namespace bson ;

namespace engine
{

   /*
      _stpSyncSource implement
    */
   _stpSyncSource::_stpSyncSource( STPCB *stpCB, BOOLEAN isSystem )
   : _stpCB( stpCB ),
     _isSystem( isSystem ),
     _port( STP_INVALID_SYNCPORT ),
     _netManager( NULL ),
     _msgHandler( stpCB, this )
   {
   }

   _stpSyncSource::~_stpSyncSource()
   {
      freeNetManager() ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__STPSYNCSOURCE_INITNETMGR, "_stpSyncSource::initNetManager" )
   INT32 _stpSyncSource::initNetManager( const CHAR *hostName,
                                         UINT16 port,
                                         UINT32 protocolMask )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__STPSYNCSOURCE_INITNETMGR ) ;

      SDB_ASSERT( NULL != hostName, "host name is invalid" ) ;
      SDB_ASSERT( !_isSystem, "should not be system synchronize source" ) ;

      CHAR serviceName[ OSS_MAX_SERVICENAME + 1 ] = { '\0' } ;

      freeNetManager() ;

      // get service name
      ossSnprintf( serviceName, OSS_MAX_SERVICENAME, "%u", port ) ;

      _netManager = SDB_OSS_NEW stpNetManager( &_msgHandler ) ;
      PD_CHECK( NULL != _netManager, SDB_OOM, error, PDERROR,
                "Failed to allocate net manager" ) ;

      rc = _netManager->initNetAgent( hostName, serviceName, protocolMask ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed initialize net agent on %s:%s, rc: %d",
                   hostName, serviceName, rc ) ;

      // assign port
      _port = port ;

   done:
      PD_TRACE_EXITRC( SDB__STPSYNCSOURCE_INITNETMGR, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__STPSYNCSOURCE_SETNETMGR, "_stpSyncSource::setNetManager" )
   INT32 _stpSyncSource::setNetManager( stpNetManager *netManager,
                                        UINT16 port,
                                        UINT32 protocolMask )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__STPSYNCSOURCE_SETNETMGR ) ;

      SDB_ASSERT( NULL != netManager, "net manager is invalid" ) ;
      SDB_ASSERT( _isSystem, "should be system synchronize source" ) ;

      freeNetManager() ;

      PD_CHECK( NULL != netManager, SDB_INVALIDARG, error, PDERROR,
                "Failed to set net manager, net manager is invalid" ) ;
      PD_CHECK( STP_INVALID_SYNCPORT != port, SDB_INVALIDARG, error, PDERROR,
                "Failed to set net manager, net manager is invalid" ) ;

      PD_CHECK( netManager->getNetAgent()->isListening( protocolMask ),
                SDB_SYS, error, PDERROR,
                "Failed to set net manager, net agent is not listening" ) ;

      _netManager = netManager ;
      _port = port ;

   done:
      PD_TRACE_EXITRC( SDB__STPSYNCSOURCE_SETNETMGR, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__STPSYNCSOURCE_ACTIVENETMANAGER, "_stpSyncSource::activeNetManager" )
   INT32 _stpSyncSource::activeNetManager()
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__STPSYNCSOURCE_ACTIVENETMANAGER ) ;

      if ( _isSystem )
      {
         goto done ;
      }

      PD_CHECK( NULL != _netManager, SDB_SYS, error, PDERROR,
                "Failed to active net manager, net manager is invalid" ) ;

      rc = _netManager->activeNetAgent() ;
      PD_RC_CHECK( rc, PDERROR, "Failed to active net agent, rc: %d", rc ) ;

   done:
      PD_TRACE_EXITRC( SDB__STPSYNCSOURCE_ACTIVENETMANAGER, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__STPSYNCSOURCE_DEACTIVENETMANAGER, "_stpSyncSource::deactiveNetManager" )
   INT32 _stpSyncSource::deactiveNetManager()
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__STPSYNCSOURCE_DEACTIVENETMANAGER ) ;

      if ( _isSystem )
      {
         goto done ;
      }

      PD_CHECK( NULL != _netManager, SDB_SYS, error, PDERROR,
                "Failed to deactive net manager, net manager is invalid" ) ;

      rc = _netManager->deactiveNetAgent() ;
      PD_RC_CHECK( rc, PDERROR, "Failed to deactive net agent, rc: %d", rc ) ;

   done:
      PD_TRACE_EXITRC( SDB__STPSYNCSOURCE_DEACTIVENETMANAGER, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__STPSYNCSOURCE_FREENETMANAGER, "_stpSyncSource::freeNetManager" )
   void _stpSyncSource::freeNetManager()
   {
      PD_TRACE_ENTRY( SDB__STPSYNCSOURCE_FREENETMANAGER ) ;

      if ( !_isSystem )
      {
         SAFE_OSS_DELETE( _netManager ) ;
      }
      _netManager = NULL ;
      _port = STP_INVALID_SYNCPORT ;

      PD_TRACE_EXIT( SDB__STPSYNCSOURCE_FREENETMANAGER ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__STPSYNCSOURCE_HANDLETIMESYNCREQ, "_stpSyncSource::handleTimeSyncReq" )
   INT32 _stpSyncSource::handleTimeSyncReq( NET_HANDLE handle,
                                            const stpTimeSyncReq *request )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__STPSYNCSOURCE_HANDLETIMESYNCREQ ) ;

      SDB_ASSERT( NULL != request, "request message is invalid" ) ;
      SDB_ASSERT( MSG_STP_TIME_SYNC_REQ == request->header.opCode,
                  "opcode of message is invalid" ) ;

      MsgRouteID routeID ;
      UINT32 remoteVersion = request->version ;
      STP_SYNC_STATUS status = (STP_SYNC_STATUS)( request->status ) ;
      UINT16 flag = request->flag ;

      stpClientNode client ;

      BOOLEAN versionExpired = FALSE ;
      UINT32 localVersion = STP_GROUP_INVALID_VERSION ;

      routeID.value = request->header.routeID.value ;

      PD_LOG( PDDEBUG, "Handle synchronize time request from %s from port %u",
              routeID2String( routeID ).c_str(), _port ) ;

      // only primary server could be synchronize source to handle synchronize
      // time request
      PD_CHECK( _stpCB->isPrimaryServer(), SDB_CLS_NOT_PRIMARY, error, PDERROR,
                "Failed to register node %s, primary is not me",
                routeID2String( routeID ).c_str() ) ;

      // check group version of client, if it is too old, we need to tell
      // the client to update information of servers
      // NOTE: Event the version is expired, but we still need to finish
      //       the time synchronization.
      //       Client only needs to know who is primary, which's enough for a
      //       client to finish time synchronization.
      localVersion = _stpCB->getNodeManager()->getVersion() ;
      if ( remoteVersion < localVersion )
      {
         PD_LOG( PDWARNING, "Client node %s's version is expired, "
                 "given is [%u], current is [%u]",
                 routeID2String( routeID ).c_str(), remoteVersion,
                 localVersion ) ;
         versionExpired = TRUE ;
      }

      // get registered client ( it it not registered if not found )
      rc = getClient( routeID, client ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to get client, rc: %d", rc ) ;

      // check role of client
      PD_CHECK( client.isValidRole(), SDB_INVALIDARG, error, PDERROR,
                "Failed to check client node %s, role is invalid",
                client.toString().c_str() ) ;

      // handle flags
      // NOTE: increase time error should be exclusive with decrease time error
      if ( OSS_BIT_TEST( flag, STP_SYNC_TIME_FLAG_INCTIMEERROR ) &&
           !OSS_BIT_TEST( flag, STP_SYNC_TIME_FLAG_DECTIMEERROR ) )
      {
         // need increase time error
         client.incTimeError() ;
         PD_LOG( PDEVENT, "Client %s is in [%s] status, time error: [%u], "
                 "increase to [%u]", client.toString().c_str(),
                 stpGetSyncStatusName( status ), request->timeError,
                 client.getTimeError() ) ;
      }
      else if ( OSS_BIT_TEST( flag, STP_SYNC_TIME_FLAG_DECTIMEERROR ) &&
                !OSS_BIT_TEST( flag, STP_SYNC_TIME_FLAG_INCTIMEERROR ) )
      {
         // need decrease time error
         client.decTimeError() ;
         PD_LOG( PDEVENT, "Client %s is in [%s] status, time error: [%u], "
                 "decrease to [%u]", client.toString().c_str(),
                 stpGetSyncStatusName( status ), request->timeError,
                 client.getTimeError() ) ;
      }

      PD_LOG( PDDEBUG, "Client %s is in [%s] status",
              client.toString().c_str(),
              stpGetSyncStatusName( status ) ) ;

      // hack return code for server group version expired case
      if ( versionExpired )
      {
         rc = SDB_REPL_REMOTE_G_V_EXPIRED ;
      }

      // send response
      rc = _sendTimeSyncRsp( handle, request, client, rc ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "Failed to send synchronize response, rc: %d", rc ) ;
      }

      // after send response, update client
      // save client status
      client.onSync( (STP_SYNC_STATUS)request->status ) ;

      // update registered client
      rc = updateClient( routeID, client ) ;
      if ( SDB_OK != rc )
      {
         PD_RC_CHECK( rc, PDERROR, "Failed to update client %s, rc: %d",
                      client.toString().c_str(), rc ) ;
      }

      if ( OSS_BIT_TEST( flag, STP_SYNC_TIME_FLAG_PUSHTIME ) )
      {
         // need push time forward
         // NOTE: do it in asynchronous
         _stpCB->getSyncSourceManager()->signalPushTime() ;
         PD_LOG( PDEVENT, "Client %s is in [%s] status, need push time",
                 client.toString().c_str(),
                 stpGetSyncStatusName( status ) ) ;
      }

   done:
      PD_TRACE_EXITRC( SDB__STPSYNCSOURCE_HANDLETIMESYNCREQ, rc ) ;
      return rc ;

   error:
      // send error response
      _sendTimeSyncRsp( handle, request, client, rc ) ;
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__STPSYNCSOURCE__SENDTIMESYNCRSP, "_stpSyncSource::_sendTimeSyncRsp" )
   INT32 _stpSyncSource::_sendTimeSyncRsp( NET_HANDLE handle,
                                           const stpTimeSyncReq *request,
                                           const stpClientNode &client,
                                           INT32 returnCode )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__STPSYNCSOURCE__SENDTIMESYNCRSP ) ;

      SDB_ASSERT( NULL != request, "request is invalid" ) ;

      stpTimeSyncRsp response ;

      // fill reply header
      _msgHandler.fillReplyHeader( request->header,
                                   response.reply,
                                   sizeof( stpTimeSyncRsp ),
                                   returnCode ) ;

      // copy field's from request
      response.reqSendTimeSec = request->sendTimeSec ;
      response.reqSendTimeNanoSec = request->sendTimeNanoSec ;
      response.reqReceiveTimeSec = request->receiveTimeSec ;
      response.reqReceiveTimeNanoSec = request->receiveTimeNanoSec ;
      response.reqTimeError = request->timeError ;
      // send time is filled in callback, set 0 here
      response.rspSendTimeSec = 0LL ;
      response.rspSendTimeNanoSec = 0LL ;
      // receive time is filled by client
      response.rspReceiveTimeSec = 0LL ;
      response.rspReceiveTimeNanoSec = 0LL ;
      // set adjusted time error
      response.rspTimeError = client.getTimeError() ;

      // send by net agent
      rc = _netManager->getNetAgent()->syncSend( handle,
                                                 (MsgHeader *)&response ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to send synchronize time response, "
                   "rc: %d", rc ) ;

   done:
      PD_TRACE_EXITRC( SDB__STPSYNCSOURCE__SENDTIMESYNCRSP, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__STPSYNCSOURCE_GETCLIENT, "_stpSyncSource::getClient" )
   INT32 _stpSyncSource::getClient( const MsgRouteID &routeID,
                                    stpClientNode &client )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__STPSYNCSOURCE_GETCLIENT ) ;

      ossScopedRWLock lock( ( &_clientMutex ), SHARED ) ;

      // find registered client from map
      STP_CLIENT_MAP::const_iterator iter = _clients.find( routeID ) ;
      PD_CHECK( iter != _clients.end(),
                SDB_INVALID_ROUTEID, error, PDERROR,
                "Failed to get client node for route ID %s, "
                "it is not found", routeID2String( routeID ).c_str() ) ;

      // copy client to output
      client = iter->second ;

   done:
      PD_TRACE_EXITRC( SDB__STPSYNCSOURCE_GETCLIENT, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__STPSYNCSOURCE_HASCLIENT, "_stpSyncSource::hasClient" )
   BOOLEAN _stpSyncSource::hasClient( const MsgRouteID &routeID )
   {
      BOOLEAN found = FALSE ;

      PD_TRACE_ENTRY( SDB__STPSYNCSOURCE_HASCLIENT ) ;

      ossScopedRWLock lock( ( &_clientMutex ), SHARED ) ;

      // find registered client from map
      STP_CLIENT_MAP::const_iterator iter = _clients.find( routeID ) ;
      if ( _clients.end() != iter )
      {
         found = TRUE ;
      }

      PD_TRACE_EXIT( SDB__STPSYNCSOURCE_HASCLIENT ) ;

      return found ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__STPSYNCSOURCE_REGISTERCLIENT, "_stpSyncSource::registerClient" )
   INT32 _stpSyncSource::registerClient( const MsgRouteID &routeID,
                                         const stpClientNode &client )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__STPSYNCSOURCE_REGISTERCLIENT ) ;

      ossScopedRWLock lock( &_clientMutex, EXCLUSIVE ) ;

      // find if already registered
      STP_CLIENT_MAP::iterator iter = _clients.find( routeID ) ;
      if ( iter != _clients.end() )
      {
         PD_LOG( PDWARNING, "route ID is the same, remove old client "
                 "node %s", iter->second.toString().c_str() ) ;
         _clients.erase( iter ) ;
      }

      // check if has the same OID
      for ( iter = _clients.begin() ;
            iter != _clients.end() ;
            ++ iter )
      {
         PD_CHECK( client.getOID() != iter->second.getOID(),
                   SDB_INVALIDARG, error, PDERROR,
                   "Failed to register client node %s, OID is conflicts",
                   client.toString().c_str() ) ;
      }

      // force to replace
      _clients[ routeID ] = client ;

   done:
      PD_TRACE_EXITRC( SDB__STPSYNCSOURCE_REGISTERCLIENT, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__STPSYNCSOURCE_REMOVECLIENT_EXPIRED, "_stpSyncSource::removeClient" )
   INT32 _stpSyncSource::removeClient( const MsgRouteID &routeID,
                                       UINT64 syncTick )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__STPSYNCSOURCE_REMOVECLIENT_EXPIRED ) ;

      ossScopedRWLock lock( ( &_clientMutex ), EXCLUSIVE ) ;

      STP_CLIENT_MAP::iterator iter = _clients.find( routeID ) ;

      // find client to be removed
      PD_CHECK( iter != _clients.end(),
                SDB_INVALID_ROUTEID, error, PDWARNING,
                "Failed to remove client node for route ID %s, "
                "node is not found", routeID2String( routeID ).c_str() ) ;

      // check if expired ( no synchronize since given time )
      if ( iter->second.getLastSyncTick() <= syncTick )
      {
         // remove expired client
         _clients.erase( iter ) ;
      }
      else
      {
         // not expired ( new synchronization happened after we first check
         // expiration ), ignore
         PD_LOG( PDDEBUG, "Ignored remove expired client node %s, "
                 "synchronize time is updated",
                 iter->second.toString().c_str() ) ;
         goto done ;
      }

      PD_LOG( PDEVENT, "Remove expired client node for route ID %s done",
              routeID2String( routeID ).c_str() ) ;

   done:
      PD_TRACE_EXITRC( SDB__STPSYNCSOURCE_REMOVECLIENT_EXPIRED, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__STPSYNCSOURCE_UPDATECLIENT, "_stpSyncSource::updateClient" )
   INT32 _stpSyncSource::updateClient( const MsgRouteID &routeID,
                                       const stpClientNode &client )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__STPSYNCSOURCE_UPDATECLIENT ) ;

      ossScopedRWLock lock( ( &_clientMutex ), EXCLUSIVE ) ;

      // find client to be updated
      STP_CLIENT_MAP::iterator iter = _clients.find( routeID ) ;
      PD_CHECK( iter != _clients.end(),
                SDB_INVALID_ROUTEID, error, PDERROR,
                "Failed to find client node %s for route ID %s",
                client.toString().c_str(),
                routeID2String( routeID ).c_str() ) ;

      // check if has the same route ID
      PD_CHECK( iter->second.getRouteIDValue() == client.getRouteIDValue(),
                SDB_INVALIDARG, error, PDERROR,
                "Failed to update client node %s, route ID is different, "
                "expected %s, given %s", client.toString().c_str(),
                routeID2String( iter->second.getRouteID() ).c_str(),
                routeID2String( client.getRouteID() ).c_str() ) ;

      // check if has the same OID
      PD_CHECK( iter->second.getOID() == client.getOID(),
                SDB_INVALIDARG, error, PDERROR,
                "Failed to update client node %s, OID is different, "
                "expected %s, given %s", client.toString().c_str(),
                iter->second.getOID().toString().c_str(),
                client.getOID().toString().c_str() ) ;

      // update client
      iter->second = client ;

   done:
      PD_TRACE_EXITRC( SDB__STPSYNCSOURCE_UPDATECLIENT, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__STPSYNCSOURCE_DUMPCLIENTS, "_stpSyncSource::dumpClients" )
   INT32 _stpSyncSource::dumpClients( STP_CLIENT_MAP &clients )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__STPSYNCSOURCE_DUMPCLIENTS ) ;

      ossScopedRWLock lock( ( &_clientMutex ), SHARED ) ;

      try
      {
         // copy clients to output
         clients = _clients ;
      }
      catch ( exception &e )
      {
         PD_LOG( PDERROR, "Failed to dump servers, "
                 "occurred unexpected error: %s", e.what() ) ;
         rc = SDB_SYS ;
         goto error ;
      }

   done:
      PD_TRACE_EXITRC( SDB__STPSYNCSOURCE_DUMPCLIENTS, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__STPSYNCSOURCE_REMOVECLIENTS, "_stpSyncSource::removeClients" )
   INT32 _stpSyncSource::removeClients()
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__STPSYNCSOURCE_REMOVECLIENTS ) ;

      ossScopedRWLock lock( ( &_clientMutex ), EXCLUSIVE ) ;

      // remove all clients
      _clients.clear() ;

      PD_LOG( PDEVENT, "Remove all client nodes done" ) ;

      PD_TRACE_EXITRC( SDB__STPSYNCSOURCE_REMOVECLIENTS, rc ) ;

      return rc ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__STPSYNCSOURCE_GETCLIENTNUM, "_stpSyncSource::getClientNum" )
   UINT32 _stpSyncSource::getClientNum()
   {
      UINT32 clientNum = 0 ;

      PD_TRACE_ENTRY( SDB__STPSYNCSOURCE_GETCLIENTNUM ) ;

      ossScopedRWLock lock( ( &_clientMutex ), SHARED ) ;

      // remove all clients
      clientNum = (UINT32)( _clients.size() ) ;

      PD_TRACE_EXIT( SDB__STPSYNCSOURCE_GETCLIENTNUM ) ;

      return clientNum ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__STPSYNCSOURCE_CLEAREXPIREDCLIENTS, "_stpSyncSource::clearExpiredClients" )
   INT32 _stpSyncSource::clearExpiredClients()
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__STPSYNCSOURCE_CLEAREXPIREDCLIENTS ) ;

      STP_CLIENT_MAP clients ;

      // dump all clients to check
      rc = dumpClients( clients ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to dump clients, rc: %d", rc ) ;

      // check each client
      for ( STP_CLIENT_MAP::iterator iter = clients.begin() ;
            iter != clients.end() ;
            ++ iter )
      {
         // check if no synchronize for a long time
         UINT64 syncTick = iter->second.getLastSyncTick() ;
         UINT64 syncPassed = pmdGetTickSpanTime( syncTick ) ;
         if ( syncPassed > STP_CLEAR_CLIENT_INTERVAL )
         {
            // if no synchronize for 2 hours, remove this client
            removeClient( iter->first, syncTick ) ;
         }
      }

   done:
      PD_TRACE_EXITRC( SDB__STPSYNCSOURCE_CLEAREXPIREDCLIENTS, rc ) ;
      return rc ;

   error:
      goto done ;
   }

}
