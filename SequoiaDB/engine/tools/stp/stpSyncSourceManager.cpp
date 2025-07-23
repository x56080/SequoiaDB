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

   Source File Name = stpSyncSourceManager.cpp

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
#include "stpSyncSourceManager.hpp"
#include "stpCB.hpp"
#include "pdTrace.hpp"
#include "stpTrace.hpp"
#include "pmd.hpp"
#include "msgMessage.hpp"

using namespace bson ;

namespace engine
{

   // interval ( in milliseconds ) to push time forward
   // NOTE: push 60 seconds each time
   #define STP_SOURCE_PUSH_INTERVAL       \
                     ( STP_SEC_TO_MILLISEC( 60 ) )
   // interval ( in nanoseconds ) to push time forward
   #define STP_SOURCE_PUSH_INTERVAL_NS    \
                     ( STP_MILLISEC_TO_NANOSEC( STP_SOURCE_PUSH_INTERVAL ) )
   #define STP_SOURCE_MIN_PORT            ( 1000 )
   #define STP_SOURCE_MAX_PORT            ( 65535 )
   // retry times to allocate source port
   #define STP_SOURCE_PORT_RETRY_TIME     ( 5 )

   /*
      _stpSyncSourceManager implement
    */
   BEGIN_OBJ_MSG_MAP( _stpSyncSourceManager, _stpManagerBase )
      // ON_MSG
      ON_MSG( MSG_STP_REG_REQ, processMessage )
      ON_MSG( MSG_STP_TIME_SYNC_REQ, processMessage )
   END_OBJ_MSG_MAP()

   _stpSyncSourceManager::_stpSyncSourceManager( STPCB *stpCB )
   : stpManagerBase( stpCB ),
     _pushEvent( FALSE ),
     _lastPushTick( 0LL ),
     _clearClientTimeout( 0LL ),
     _portIndex( 0 ),
     _sysSource( stpCB, TRUE ),
     _maxSyncPorts( 0 ),
     _allowSyncPorts( 0 ),
     _defClientsPerPort( 0 )
   {
   }

   _stpSyncSourceManager::~_stpSyncSourceManager()
   {
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__STPSYNCSOURCEMGR_ONTIMER, "_stpSyncSourceManager::onTimer" )
   void _stpSyncSourceManager::onTimer( UINT64 timerID, UINT32 interval )
   {
      PD_TRACE_ENTRY( SDB__STPSYNCSOURCEMGR_ONTIMER ) ;

      if ( timerID == _timerID )
      {
         // check timeout to clear expired synchronize clients
         _clearClientTimeout += interval ;
         if ( _clearClientTimeout >= STP_CLEAR_CLIENT_INTERVAL )
         {
            if ( _stpCB->isPrimaryServer() )
            {
               _clearExpiredClients() ;
            }
            _clearClientTimeout = 0LL ;
         }
         // check if we need to push time forward
         if ( _needPushTime() )
         {
            _pushTime() ;
         }
      }

      PD_TRACE_EXIT( SDB__STPSYNCSOURCEMGR_ONTIMER ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__STPSYNCSOURCEMGR_PROCESSMESSAGE, "_stpSyncSourceManager::processMessage" )
   INT32 _stpSyncSourceManager::processMessage( NET_HANDLE handle,
                                                MsgHeader *message )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__STPSYNCSOURCEMGR_PROCESSMESSAGE ) ;

      SDB_ASSERT( NULL != message, "message is invalid" ) ;

      switch ( message->opCode )
      {
         case MSG_STP_REG_REQ :
         {
            // handle register request
            rc = _handleRegReq( handle, (const stpRegReq *)message ) ;
            PD_RC_CHECK( rc, PDERROR, "Failed to handle register request, "
                         "rc: %d", rc ) ;
            break ;
         }
         case MSG_STP_TIME_SYNC_REQ :
         {
            // handle synchronize time request
            rc = _handleTimeSyncReq( handle,
                                     (const stpTimeSyncReq *)message ) ;
            PD_RC_CHECK( rc, PDERROR, "Failed to handle synchronize time "
                         "request, rc: %d", rc ) ;
            break ;
         }
         default :
         {
            // unknown message
            PD_CHECK( FALSE, SDB_UNKNOWN_MESSAGE, error, PDERROR,
                      "Unknown synchronize source message [%d]",
                      message->opCode ) ;
            break ;
         }
      }

   done:
      PD_TRACE_EXITRC( SDB__STPSYNCSOURCEMGR_PROCESSMESSAGE, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__STPSYNCSOURCEMGR__INITIALIZE, "_stpSyncSourceManager::_initialize" )
   INT32 _stpSyncSourceManager::_initialize()
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__STPSYNCSOURCEMGR__INITIALIZE ) ;

      SDB_ASSERT( NULL != pmdGetThreadEDUCB() &&
                  EDU_TYPE_MAIN == pmdGetThreadEDUCB()->getType(),
                  "must register in main thread" ) ;

      _maxSyncPorts = _options->getMaxSyncPorts() ;
      _defClientsPerPort = _options->getDefClientsPerPort() ;

      PD_CHECK( NULL != pmdGetThreadEDUCB() &&
                EDU_TYPE_MAIN == pmdGetThreadEDUCB()->getType(),
                SDB_SYS, error, PDERROR,
                "Failed to initialize synchronize source, should be "
                "initialize in main thread" ) ;

      _allowSyncPorts = _maxSyncPorts ;
      if ( _maxSyncPorts > 1 )
      {
         if ( _options->isSyncWithSysPort() )
         {
            // system port is allowed to be used for synchronize
            rc = _addSource( &_sysSource ) ;
            PD_RC_CHECK( rc, PDERROR, "Failed to add synchronize source, "
                         "rc: %d", rc ) ;
         }
         else
         {
            // system port is not allowed to be used for synchronize
            _allowSyncPorts = _maxSyncPorts - 1 ;
         }
      }
      else
      {
         // only system port could be used for synchronize
         rc = _addSource( &_sysSource ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to add synchronize source, "
                      "rc: %d", rc ) ;
      }

   done:
      PD_TRACE_EXITRC( SDB__STPSYNCSOURCEMGR__INITIALIZE, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__STPSYNCSOURCEMGR__FINALIZE, "_stpSyncSourceManager::_finalize" )
   INT32 _stpSyncSourceManager::_finalize()
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__STPSYNCSOURCEMGR__FINALIZE ) ;

      ossScopedRWLock lock( &_sourceMutex, EXCLUSIVE ) ;

      SDB_ASSERT( NULL != pmdGetThreadEDUCB() &&
                  EDU_TYPE_MAIN == pmdGetThreadEDUCB()->getType(),
                  "must unregister in main thread" ) ;

      PD_CHECK( NULL != pmdGetThreadEDUCB() &&
                EDU_TYPE_MAIN == pmdGetThreadEDUCB()->getType(),
                SDB_SYS, error, PDERROR,
                "Failed to finalize synchronize source, should be "
                "finalize in main thread" ) ;

      for ( STP_SYNC_SOURCE_LIST::iterator iter = _syncSources.begin() ;
            iter != _syncSources.end() ;
            ++ iter )
      {
         stpSyncSource *source = ( *iter ) ;
         source->freeNetManager() ;
         if ( !( source->isSystem() ) )
         {
            SAFE_OSS_DELETE( source ) ;
         }
      }

      _syncSources.clear() ;
      _sysSource.freeNetManager() ;

   done:
      PD_TRACE_EXITRC( SDB__STPSYNCSOURCEMGR__FINALIZE, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__STPSYNCSOURCEMGR__PREACTIVE, "_stpSyncSourceManager::_preActivate" )
   INT32 _stpSyncSourceManager::_preActivate()
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__STPSYNCSOURCEMGR__PREACTIVE ) ;

      const CHAR *hostName = pmdGetKRCB()->getHostName() ;
      UINT16 port = _options->getPort() ;

      SDB_ASSERT( NULL != hostName, "host name is invalid" ) ;

      // default synchronize source used default settings
      rc = _sysSource.setNetManager( _netManager, port, NET_FRAME_MASK_UDP ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to set default synchronize source, "
                   "rc: %d", rc ) ;

      _portIndex = port ;

   done:
      PD_TRACE_EXITRC( SDB__STPSYNCSOURCEMGR__PREACTIVE, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__STPSYNCSOURCEMGR__POSTACTIVE, "_stpSyncSourceManager::_postActivate" )
   INT32 _stpSyncSourceManager::_postActivate()
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__STPSYNCSOURCEMGR__POSTACTIVE ) ;

      BOOLEAN forceSyncPorts = _options->isPreOpenPorts() ;

      // activate net manager for extra synchronize sources
      // NOTE: net manager for default synchronize source is already activated
      if ( forceSyncPorts )
      {
         for ( UINT32 index = 1 ; index < _maxSyncPorts ; ++ index )
         {
            stpSyncSource *source = NULL ;

            rc = _allocSource( &source, TRUE ) ;
            PD_RC_CHECK( rc, PDERROR, "Failed to allocate synchronize source, "
                         "rc: %d", rc ) ;
         }
      }

   done:
      PD_TRACE_EXITRC( SDB__STPSYNCSOURCEMGR__POSTACTIVE, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__STPSYNCSOURCEMGR__PREDEACTIVE, "_stpSyncSourceManager::_preDeactivate" )
   INT32 _stpSyncSourceManager::_preDeactivate()
   {
      PD_TRACE_ENTRY( SDB__STPSYNCSOURCEMGR__PREDEACTIVE ) ;

      ossScopedRWLock lock( &_sourceMutex, EXCLUSIVE ) ;

      // deactive net manager for extra sources
      // NOTE: net manager for default source is deactivated by stpCB
      for ( STP_SYNC_SOURCE_LIST::iterator iter = _syncSources.begin() ;
            iter != _syncSources.end() ;
            ++ iter )
      {
         stpSyncSource *source = ( *iter ) ;
         SDB_ASSERT( NULL != source, "synchronize source is invalid" ) ;
         source->deactiveNetManager() ;
      }

      PD_TRACE_EXITRC( SDB__STPSYNCSOURCEMGR__PREDEACTIVE, SDB_OK ) ;

      return SDB_OK ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__STPSYNCSOURCEMGR__POSTDEACTIVE, "_stpSyncSourceManager::_postDeactivate" )
   INT32 _stpSyncSourceManager::_postDeactivate()
   {
      PD_TRACE_ENTRY( SDB__STPSYNCSOURCEMGR__POSTDEACTIVE ) ;

      // remove all clients
      removeClients() ;

      PD_TRACE_EXITRC( SDB__STPSYNCSOURCEMGR__POSTDEACTIVE, SDB_OK ) ;

      return SDB_OK ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__STPSYNCSOURCEMGR_ONRECEIVETIMESYNCREQ, "_stpSyncSourceManager::onReceiveTimeSyncReq" )
   INT32 _stpSyncSourceManager::onReceiveTimeSyncReq( stpTimeSyncReq *request )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__STPSYNCSOURCEMGR_ONRECEIVETIMESYNCREQ ) ;

      SDB_ASSERT( NULL != request, "request message is invalid" ) ;
      SDB_ASSERT( MSG_STP_TIME_SYNC_REQ == request->header.opCode,
                  "opcode of message is invalid" ) ;

      // set the receive time on receiving synchronize time request
      stpHPTime receiveTime = getMetaData()->getLTValue() ;
      request->receiveTimeSec = receiveTime.getSecond() ;
      request->receiveTimeNanoSec = receiveTime.getNanoSecond() ;

      PD_TRACE_EXITRC( SDB__STPSYNCSOURCEMGR_ONRECEIVETIMESYNCREQ, rc ) ;

      return rc ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__STPSYNCSOURCEMGR_ONSENDTIMESYNCRSP, "_stpSyncSourceManager::onSendTimeSyncRsp" )
   INT32 _stpSyncSourceManager::onSendTimeSyncRsp( stpTimeSyncRsp *response )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__STPSYNCSOURCEMGR_ONSENDTIMESYNCRSP ) ;

      SDB_ASSERT( NULL != response, "response message is invalid" ) ;
      SDB_ASSERT( MSG_STP_TIME_SYNC_RSP == response->reply.header.opCode,
                  "opcode of message is invalid" ) ;

      // set the send time on sending synchronize time response
      stpHPTime sendTime = getMetaData()->getLTValue() ;
      response->rspSendTimeSec = sendTime.getSecond() ;
      response->rspSendTimeNanoSec = sendTime.getNanoSecond() ;

      PD_TRACE_EXITRC( SDB__STPSYNCSOURCEMGR_ONSENDTIMESYNCRSP, rc ) ;

      return rc ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__STPSYNCSOURCEMGR__HANDLEREGREQ, "_stpSyncSourceManager::_handleRegReq" )
   INT32 _stpSyncSourceManager::_handleRegReq( NET_HANDLE handle,
                                               const stpRegReq *request )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__STPSYNCSOURCEMGR__HANDLEREGREQ ) ;

      SDB_ASSERT( NULL != request, "request message is invalid" ) ;
      SDB_ASSERT( MSG_STP_REG_REQ == request->header.opCode,
                  "opcode of message is invalid" ) ;

      MsgRouteID routeID ;
      stpClientNode client ;

      routeID.value = MSG_INVALID_ROUTEID ;

      // only primary server could be synchronize source to handle register
      // request
      PD_CHECK( _stpCB->isPrimaryServer(),
                SDB_CLS_NOT_PRIMARY, error, PDERROR,
                "Failed to register client %s, primary is not me",
                routeID2String( request->header.routeID ).c_str() ) ;

      // get route ID from net
      // NOTE: STP nodes do not have route ID in meta data, route ID of STP
      //       node is generated by IP address bind with host name and port
      //       of STP node
      //       if the client bind host name with a local IP ( e.g.
      //       127.0.0.1 ), the route ID from client will be indistinguishable
      //       so, we need to get the real route ID from net agent
      rc = _netManager->getRouteID( _netAgent, handle, routeID ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to get route ID from handle [%u], "
                   "rc: %d", handle, rc ) ;

      if ( request->header.routeID.columns.groupID != routeID.columns.groupID )
      {
         PD_LOG( PDWARNING, "Register client with different group ID, "
                 "given [%u], get from net [%u]",
                 request->header.routeID.columns.groupID,
                 routeID.columns.groupID ) ;
      }

      // adjust node ID and service ID
      routeID.columns.nodeID = request->header.routeID.columns.nodeID ;
      routeID.columns.serviceID = MSG_ROUTE_LOCAL_SERVICE ;

      try
      {
         BSONObj regObject ;

         // check length of message
         PD_CHECK( request->header.messageLength >=
                   (INT32)( sizeof( stpRegReq ) + regObject.objsize() ),
                   SDB_SYS, error, PDERROR, "Failed to handle server "
                   "response, size of message is unexpected, "
                   "expected >= [%u], given [%u]",
                   sizeof( stpServerRsp ) + regObject.objsize(),
                   request->header.messageLength ) ;

         // extract result in BSON format
         regObject = BSONObj( (CHAR *)( request ) + sizeof( stpRegReq ) ) ;

         // parse client node
         rc = client.fromBSON( regObject, FALSE ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to parse client node object, "
                      "rc: %d", rc ) ;
      }
      catch ( exception &e )
      {
         rc = SDB_SYS ;
         PD_LOG( PDERROR, "Failed to parse client node object, "
                 "occurred unexpected error: %s", e.what() ) ;
         goto error ;
      }

      // check route ID of register client
      if ( client.getRouteIDValue() != routeID.value )
      {
         PD_LOG( PDWARNING, "Register client with different route ID, "
                 "given %s, get from net %s",
                 routeID2String( client.getRouteID() ).c_str(),
                 routeID2String( routeID ).c_str() ) ;

         client.setRouteID( routeID ) ;
      }

      // assign default port
      client.setSyncPort( _options->getPort() ) ;

      // register client node
      rc = registerClient( routeID, request->version, client ) ;
      if ( SDB_OK != rc && SDB_REPL_REMOTE_G_V_EXPIRED != rc )
      {
         PD_RC_CHECK( rc, PDERROR, "Failed to register client %s, rc: %d",
                      client.toString().c_str(), rc ) ;
      }

      // send response
      rc = _sendRegRsp( handle, request, client, rc ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "Failed to send register response, rc: %d", rc ) ;
      }

   done:
      PD_TRACE_EXITRC( SDB__STPSYNCSOURCEMGR__HANDLEREGREQ, rc ) ;
      return rc ;

   error:
      // send error response
      _sendRegRsp( handle, request, client, rc ) ;
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__STPSYNCSOURCEMGR__HANDLETIMESYNCREQ, "_stpSyncSourceManager::_handleTimeSyncReq" )
   INT32 _stpSyncSourceManager::_handleTimeSyncReq(
                                                NET_HANDLE handle,
                                                const stpTimeSyncReq *request )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__STPSYNCSOURCEMGR__HANDLETIMESYNCREQ ) ;

      SDB_ASSERT( NULL != request, "request message is invalid" ) ;
      SDB_ASSERT( MSG_STP_TIME_SYNC_REQ == request->header.opCode,
                  "opcode of message is invalid" ) ;

      rc = _sysSource.handleTimeSyncReq( handle, request ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to handle time synchronize request, "
                   "rc: %d", rc ) ;

   done:
      PD_TRACE_EXITRC( SDB__STPSYNCSOURCEMGR__HANDLETIMESYNCREQ, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__STPSYNCSOURCEMGR__SENDREGRSP, "_stpSyncSourceManager::_sendRegRsp" )
   INT32 _stpSyncSourceManager::_sendRegRsp( NET_HANDLE handle,
                                             const stpRegReq *request,
                                             const stpClientNode &client,
                                             INT32 returnCode )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__STPSYNCSOURCEMGR__SENDREGRSP ) ;

      SDB_ASSERT( NULL != request, "request is invalid" ) ;

      stpRegRsp response ;

      // fill reply header
      _netMsgHandler->fillReplyHeader( request->header,
                                       response.reply,
                                       sizeof( stpRegRsp ),
                                       returnCode ) ;

      // set real route ID
      response.routeID.value = client.getRouteIDValue() ;
      // set verified OID
      response.oid = client.getOID() ;
      // set synchronize time port
      response.port = client.getSyncPort() ;

      // send by net agent
      rc = _netAgent->syncSend( handle, &response ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to send register response, "
                   "rc: %d", rc ) ;

   done:
      PD_TRACE_EXITRC( SDB__STPSYNCSOURCEMGR__SENDREGRSP, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__STPSYNCSOURCEMGR_REGISTERCLIENT, "_stpSyncSourceManager::registerClient" )
   INT32 _stpSyncSourceManager::registerClient( const MsgRouteID &routeID,
                                                UINT32 version,
                                                stpClientNode &client )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__STPSYNCSOURCEMGR_REGISTERCLIENT ) ;

      BOOLEAN lockedSource = FALSE ;
      BOOLEAN versionExpired = FALSE ;
      UINT32 localVersion = STP_GROUP_INVALID_VERSION ;
      stpSyncSource *assignSource = NULL ;

      // check role of client
      PD_CHECK( client.isValidRole(), SDB_INVALIDARG, error, PDERROR,
                "Failed to check client node %s, role is invalid",
                client.toString().c_str() ) ;

      // check route ID of client
      PD_CHECK( client.isValidRoute(), SDB_INVALID_ROUTEID, error, PDERROR,
                "Failed to check client node %s, route ID is invalid",
                client.toString().c_str() ) ;

      // check OID of client
      PD_CHECK( client.isValidOID(), SDB_INVALIDARG, error, PDERROR,
                "Failed to check client node %s, OID is invalid",
                client.toString().c_str() ) ;

      // check group version of client, if it is too old, we need to tell
      // the client to update information of servers
      // NOTE: Even the version is expired, we could still finish the register
      //       request, just tell client to update server group by itself.
      //       Client only needs to know who is primary, which's enough for a
      //       client to finish time synchronization.
      localVersion = getNodeManager()->getVersion() ;
      if ( version < localVersion )
      {
         PD_LOG( PDWARNING, "Client node %s's version is expired, "
                 "given is [%u], current is [%u]", client.toString().c_str(),
                 version, localVersion ) ;
         versionExpired = TRUE ;
      }

      _sourceMutex.lock_w() ;
      lockedSource = TRUE ;

      rc = _assignSource( routeID, &assignSource ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to assign synchronize source, "
                   "rc: %d", rc ) ;

      PD_CHECK( NULL != assignSource, SDB_SYS, error, PDERROR,
                "Failed to assign synchronize source" ) ;

      client.setSyncPort( assignSource->getPort() ) ;

      // register to synchronize source
      rc = assignSource->registerClient( routeID, client ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to register client %s, rc: %d",
                   client.toString().c_str(), rc ) ;

      _sourceMutex.release_w() ;
      lockedSource = FALSE ;

      PD_LOG( PDEVENT, "Register client node %s done, assigned port %u",
              client.toString().c_str(), client.getSyncPort() ) ;

   done:
      if ( lockedSource )
      {
         _sourceMutex.release_w() ;
      }
      if ( versionExpired )
      {
         // tell client to update information of servers
         rc = SDB_REPL_REMOTE_G_V_EXPIRED ;
      }
      PD_TRACE_EXITRC( SDB__STPSYNCSOURCEMGR_REGISTERCLIENT, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__STPSYNCSOURCEMGR_DUMPCLIENTS, "_stpSyncSourceManager::dumpClients" )
   INT32 _stpSyncSourceManager::dumpClients( STP_CLIENT_MAP &clients )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__STPSYNCSOURCEMGR_DUMPCLIENTS ) ;

      ossScopedRWLock lock( &_sourceMutex, SHARED ) ;

      try
      {
         // dump clients from synchronize sources
         for ( STP_SYNC_SOURCE_LIST::iterator iter = _syncSources.begin() ;
               iter != _syncSources.end() ;
               ++ iter )
         {
            STP_CLIENT_MAP tempClients ;
            stpSyncSource *source = ( *iter ) ;

            SDB_ASSERT( NULL != source, "synchronize source is invalid" ) ;

            rc = source->dumpClients( tempClients ) ;
            PD_RC_CHECK( rc, PDERROR, "Failed to dump clients from "
                         "synchronize source for port [%u], rc: %d",
                         source->getPort(), rc ) ;

            // remove duplicated clients
            // NOTE: clients may register to different ports
            for ( STP_CLIENT_MAP::iterator tempIter = tempClients.begin() ;
                  tempIter != tempClients.end() ;
                  ++ tempIter )
            {
               STP_CLIENT_MAP::iterator iter = clients.find( tempIter->first ) ;
               if ( iter == clients.end() )
               {
                  // not found duplicated one
                  clients.insert( make_pair( tempIter->first,
                                             tempIter->second ) ) ;
               }
               else
               {
                  // found a duplicated one
                  // the client from current synchronize source is
                  // synchronized later replace with the newer one
                  if ( iter->second.getLastSyncTick() <
                       tempIter->second.getLastSyncTick() )
                  {
                     clients[ tempIter->first ] = tempIter->second ;
                  }
               }
            }
         }
      }
      catch ( exception &e )
      {
         PD_LOG( PDERROR, "Failed to dump clients, error: %s", e.what() ) ;
         rc = SDB_SYS ;
      }

   done:
      PD_TRACE_EXITRC( SDB__STPSYNCSOURCEMGR_DUMPCLIENTS, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__STPSYNCSOURCEMGR__REMOVECLIENTS, "_stpSyncSourceManager::removeClients" )
   INT32 _stpSyncSourceManager::removeClients()
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__STPSYNCSOURCEMGR__REMOVECLIENTS ) ;

      ossScopedRWLock lock( &_sourceMutex, SHARED ) ;

      // remove clients from extra synchronize sources
      for ( STP_SYNC_SOURCE_LIST::iterator iter = _syncSources.begin() ;
            iter != _syncSources.end() ;
            ++ iter )
      {
         stpSyncSource *source = ( *iter ) ;

         SDB_ASSERT( NULL != source, "synchronize source is invalid" ) ;

         source->removeClients() ;
      }

      PD_LOG( PDEVENT, "Remove all client nodes done" ) ;

      PD_TRACE_EXITRC( SDB__STPSYNCSOURCEMGR__REMOVECLIENTS, rc ) ;

      return rc ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__STPSYNCSOURCEMGR__CLEAREXPIREDCLIENTS, "_stpSyncSourceManager::_clearExpiredClients" )
   INT32 _stpSyncSourceManager::_clearExpiredClients()
   {
      PD_TRACE_ENTRY( SDB__STPSYNCSOURCEMGR__CLEAREXPIREDCLIENTS ) ;

      ossScopedRWLock lock( &_sourceMutex, SHARED ) ;

      for ( STP_SYNC_SOURCE_LIST::iterator iter = _syncSources.begin() ;
            iter != _syncSources.end() ;
            ++ iter )
      {
         stpSyncSource *source = ( *iter ) ;
         source->clearExpiredClients() ;
      }

      PD_TRACE_EXITRC( SDB__STPSYNCSOURCEMGR__CLEAREXPIREDCLIENTS, SDB_OK ) ;

      return SDB_OK ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__STPSYNCSOURCEMGR__ASSIGNSOURCE, "_stpSyncSourceManager::_assignSource" )
   INT32 _stpSyncSourceManager::_assignSource( const MsgRouteID &routeID,
                                               stpSyncSource **source )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__STPSYNCSOURCEMGR__ASSIGNSOURCE ) ;

      stpSyncSource *minUsedSource = NULL ;

      if ( _maxSyncPorts <= 1 )
      {
         // use system source
         *source = &_sysSource ;
         goto done ;
      }

      // check if already assigned to a source
      for ( STP_SYNC_SOURCE_LIST::iterator iter = _syncSources.begin() ;
            _syncSources.end() != iter ;
            ++ iter )
      {
         stpSyncSource *tmpSource = ( *iter ) ;
         if ( tmpSource->hasClient( routeID ) )
         {
            *source = tmpSource ;
            goto done ;
         }
      }

      // not assigned yet, assigned a new one
      // find minimum used source first
      for ( STP_SYNC_SOURCE_LIST::iterator iter = _syncSources.begin() ;
            _syncSources.end() != iter ;
            ++ iter )
      {
         stpSyncSource *tmpSource = ( *iter ) ;
         if ( NULL == minUsedSource )
         {
            minUsedSource = tmpSource ;
         }
         else if ( tmpSource->getClientNum() <
                   minUsedSource->getClientNum() )
         {
            minUsedSource = tmpSource ;
         }
      }
      // if minimum used source is not full or new port is not allowed, use
      // the minimum used source
      if ( NULL != minUsedSource &&
           ( minUsedSource->getClientNum() < _defClientsPerPort ||
             _syncSources.size() >= _allowSyncPorts ) )
      {
         // minimum
         *source = minUsedSource ;
         goto done ;
      }

      // if needed and allowed, create a new one
      rc = _allocSource( source, FALSE ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDWARNING, "Failed to allocate synchronize source, rc: %d",
                 rc ) ;
         if ( NULL != minUsedSource )
         {
            *source = minUsedSource ;
         }
         else
         {
            *source = &_sysSource ;
         }
         rc = SDB_OK ;
      }

   done:
      PD_TRACE_EXITRC( SDB__STPSYNCSOURCEMGR__ASSIGNSOURCE, rc ) ;
      return rc ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__STPSYNCSOURCEMGR__ALLOCSOURCE, "_stpSyncSourceManager::_allocSource" )
   INT32 _stpSyncSourceManager::_allocSource( stpSyncSource **source,
                                              BOOLEAN force )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__STPSYNCSOURCEMGR__ALLOCSOURCE ) ;

      SDB_ASSERT( NULL != source, "source is invalid" ) ;

      stpSyncSource *newSource = NULL ;
      UINT32 index = (UINT32)( _syncSources.size() ) ;

      UINT32 retryTimes = 0 ;
      const CHAR *hostName = pmdGetKRCB()->getHostName() ;

      newSource = SDB_OSS_NEW stpSyncSource( _stpCB, FALSE ) ;
      PD_CHECK( NULL != newSource, SDB_OOM, error, PDERROR,
                "Failed to allocate memory for synchronize "
                "source [%d]", index ) ;

      while ( TRUE )
      {
         UINT16 port = (UINT16)( ++ _portIndex ) ;
         if ( port > STP_SOURCE_MAX_PORT )
         {
            _portIndex = STP_SOURCE_MIN_PORT ;
            port = STP_SOURCE_MIN_PORT ;
         }
         rc = newSource->initNetManager( hostName,
                                         port,
                                         NET_FRAME_MASK_UDP ) ;
         if ( SDB_OK != rc )
         {
            if ( force ||
                 retryTimes >= STP_SOURCE_PORT_RETRY_TIME )
            {
               PD_LOG( PDERROR, "Failed to initialize net manager "
                       "for %s:%u, rc: %d", hostName, port, rc ) ;

               goto error ;
            }
            else
            {
               PD_LOG( PDWARNING, "Failed to initialize net manager "
                       "for %s:%u, rc: %d, retry next port", hostName, port,
                       rc ) ;

               rc = SDB_OK ;
               ++ retryTimes ;
               continue ;
            }
         }
         break ;
      }

      rc = newSource->activeNetManager() ;
      PD_RC_CHECK( rc, PDERROR, "Failed to active net manager, rc: %d", rc ) ;

      rc = _addSource( newSource ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to add synchronize source, rc: %d",
                   rc ) ;

      *source = newSource ;
      newSource = NULL ;

   done:
      PD_TRACE_EXITRC( SDB__STPSYNCSOURCEMGR__ALLOCSOURCE, rc ) ;
      return rc ;

   error:
      // on error, should free net manager
      if ( NULL != newSource )
      {
         newSource->deactiveNetManager() ;
         newSource->freeNetManager() ;
         SDB_OSS_DEL newSource ;
         newSource = NULL ;
      }
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__STPSYNCSOURCEMGR__ADDSOURCE, "_stpSyncSourceManager::_addSource" )
   INT32 _stpSyncSourceManager::_addSource( stpSyncSource *source )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__STPSYNCSOURCEMGR__ADDSOURCE ) ;

      try
      {
         _syncSources.push_back( source ) ;
      }
      catch ( exception &e )
      {
         PD_LOG( PDERROR, "Failed to add synchronize source, error: %s",
                 e.what() ) ;
         rc = SDB_SYS ;
         goto error ;
      }

   done:
      PD_TRACE_EXITRC( SDB__STPSYNCSOURCEMGR__ADDSOURCE, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__STPSYNCSOURCEMGR_SIGNALPUSHTIME, "_stpSyncSourceManager::signalPushTime" )
   void _stpSyncSourceManager::signalPushTime()
   {
      PD_TRACE_ENTRY( SDB__STPSYNCSOURCEMGR_SIGNALPUSHTIME ) ;

      // signal we need push time forward
      _pushEvent = TRUE ;

      PD_TRACE_EXIT( SDB__STPSYNCSOURCEMGR_SIGNALPUSHTIME ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__STPSYNCSOURCEMGR__NEEDPUSHTIME, "_stpSyncSourceManager::_needPushTime" )
   BOOLEAN _stpSyncSourceManager::_needPushTime()
   {
      BOOLEAN needPush = FALSE ;

      PD_TRACE_ENTRY( SDB__STPSYNCSOURCEMGR__NEEDPUSHTIME ) ;

      if ( _pushEvent )
      {
         // push event is signaled, but we need to check if we could push
         // only push one time for each 60 seconds ( not too frequently )
         UINT64 pushPassed = pmdGetTickSpanTime( _lastPushTick ) ;
         if ( pushPassed > STP_SOURCE_PUSH_INTERVAL )
         {
            needPush = TRUE ;
         }
         _pushEvent = FALSE ;
      }

      PD_TRACE_EXIT( SDB__STPSYNCSOURCEMGR__NEEDPUSHTIME ) ;

      return needPush ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__STPSYNCSOURCEMGR__PUSHTIME, "_stpSyncSourceManager::_pushTime" )
   void _stpSyncSourceManager::_pushTime()
   {
      // push time forward for 60 seconds
      getMetaData()->adjustLogicalTime( STP_SOURCE_PUSH_INTERVAL_NS ) ;

      // reset push event
      _pushEvent = FALSE ;

      // set last push tick
      _lastPushTick = pmdGetDBTick() ;
   }

}
