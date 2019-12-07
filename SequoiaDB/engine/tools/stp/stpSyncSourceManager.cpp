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

   // interval to clear expired clients ( without synchronize in 2 hours )
   #define STP_CLEAR_CLIENT_INTERVAL   ( STP_CLEAR_SYNCHRONIZE_INTERVAL )

   // interval ( in milliseconds ) to push time forward
   // NOTE: push 60 seconds each time
   #define STP_SOURCE_PUSH_INTERVAL       \
                     ( STP_SEC_TO_MILLISEC( 60 ) )
   // interval ( in nanoseconds ) to push time forward
   #define STP_SOURCE_PUSH_INTERVAL_NS    \
                     ( STP_MILLISEC_TO_NANOSEC( STP_SOURCE_PUSH_INTERVAL ) )

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
     _clearClientTimeout( 0LL )
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

   // PD_TRACE_DECLARE_FUNCTION ( SDB__STPSYNCSOURCEMGR_ONRECEIVETIMESYNCREQ, "_stpSyncSourceManager::onReceiveTimeSyncReq" )
   INT32 _stpSyncSourceManager::onReceiveTimeSyncReq( stpTimeSyncReq *request )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__STPSYNCSOURCEMGR_ONRECEIVETIMESYNCREQ ) ;

      SDB_ASSERT( NULL != request, "request message is invalid" ) ;
      SDB_ASSERT( MSG_STP_TIME_SYNC_REQ == request->header.opCode,
                  "opcode of message is invalid" ) ;

      // set the receive time on receiving synchronize time request
      tpHPTime receiveTime = getMetaData()->getLTValue() ;
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
      tpHPTime sendTime = getMetaData()->getLTValue() ;
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

      const MsgRouteID &routeID = request->header.routeID ;
      stpClientNode client ;

      // only primary server could be synchronize source to handle register
      // request
      PD_CHECK( _stpCB->isPrimaryServer(),
                SDB_CLS_NOT_PRIMARY, error, PDERROR,
                "Failed to register client %s, primary is not me",
                routeID2String( routeID ).c_str() ) ;

      // set client node
      client.setRouteID( routeID ) ;
      client.setRole( (STP_ROLE)( request->role ) ) ;
      client.setSyncInterval( request->syncInterval ) ;
      client.setMaxTimeError( request->maxTimeError ) ;
      client.setTimeError( request->timeError ) ;
      client.setOID( request->oid ) ;

      // register client node
      rc = registerClient( request->version, client ) ;
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

      const MsgRouteID &routeID = request->header.routeID ;
      STP_SYNC_STATUS status = (STP_SYNC_STATUS)( request->status ) ;
      UINT16 flag = request->flag ;

      stpClientNode client ;

      // only primary server could be synchronize source to handle synchronize
      // time request
      PD_CHECK( _stpCB->isPrimaryServer(), SDB_CLS_NOT_PRIMARY, error, PDERROR,
                "Failed to register node %s, primary is not me",
                routeID2String( routeID ).c_str() ) ;

      // get registered client ( it it not registered if not found )
      rc = getClient( routeID, client ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to get client %s, rc: %d",
                   routeID2String( routeID ).c_str(), rc ) ;

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

      if ( OSS_BIT_TEST( flag, STP_SYNC_TIME_FLAG_PUSHTIME ) )
      {
         // need push time forward
         // NOTE: do it in asynchronous
         _signalPushTime() ;
         PD_LOG( PDEVENT, "Client %s is in [%s] status, need push time",
                 client.toString().c_str(),
                 stpGetSyncStatusName( status ) ) ;
      }

      PD_LOG( PDDEBUG, "Client %s is in [%s] status",
              client.toString().c_str(),
              stpGetSyncStatusName( status ) ) ;

      // save status
      client.onSync( (STP_SYNC_STATUS)request->status ) ;

      // update registered client
      rc = updateClient( request->version, client ) ;
      if ( SDB_OK != rc && SDB_REPL_REMOTE_G_V_EXPIRED != rc )
      {
         PD_RC_CHECK( rc, PDERROR, "Failed to update client %s, rc: %d",
                      client.toString().c_str(), rc ) ;
      }

      // send response
      rc = _sendTimeSyncRsp( handle, request, client, rc ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "Failed to send synchronize response, rc: %d", rc ) ;
      }

   done:
      PD_TRACE_EXITRC( SDB__STPSYNCSOURCEMGR__HANDLETIMESYNCREQ, rc ) ;
      return rc ;

   error:
      // send error response
      _sendTimeSyncRsp( handle, request, client, rc ) ;
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
      _fillReplyHeader( request->header, response.reply,
                        sizeof( stpRegRsp ), returnCode ) ;

      // set verified OID
      response.oid = client.getOID() ;
      // set synchronize time port
      // TODO: assign to different port to relieve stress
      response.port = _options->getPort() ;

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

   // PD_TRACE_DECLARE_FUNCTION ( SDB__STPSYNCSOURCEMGR__SENDTIMESYNCRSP, "_stpSyncSourceManager::_sendTimeSyncRsp" )
   INT32 _stpSyncSourceManager::_sendTimeSyncRsp( NET_HANDLE handle,
                                                  const stpTimeSyncReq *request,
                                                  const stpClientNode &client,
                                                  INT32 returnCode )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__STPSYNCSOURCEMGR__SENDTIMESYNCRSP ) ;

      SDB_ASSERT( NULL != request, "request is invalid" ) ;

      stpTimeSyncRsp response ;

      // fill reply header
      _fillReplyHeader( request->header, response.reply,
                        sizeof( stpTimeSyncRsp ), returnCode ) ;

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
      rc = _netAgent->syncSend( handle, &response ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to send synchronize time response, "
                   "rc: %d", rc ) ;

   done:
      PD_TRACE_EXITRC( SDB__STPSYNCSOURCEMGR__SENDTIMESYNCRSP, rc ) ;
      return rc ;

   error:
      goto done ;
   }


   // PD_TRACE_DECLARE_FUNCTION ( SDB__STPSYNCSOURCEMGR_GETCLIENT, "_stpSyncSourceManager::getClient" )
   INT32 _stpSyncSourceManager::getClient( const MsgRouteID &routeID,
                                           stpClientNode &client )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__STPSYNCSOURCEMGR_GETCLIENT ) ;

      ossScopedRWLock lock( ( &_clientMutex ), SHARED ) ;

      // find registered client from map
      STP_CLIENT_MAP::const_iterator iter = _clients.find( routeID ) ;
      PD_CHECK( iter != _clients.end(),
                SDB_INVALID_ROUTEID, error, PDERROR,
                "Failed to get client node %s, it is not found",
                routeID2String( routeID ).c_str() ) ;

      // copy client to output
      client = iter->second ;

   done:
      PD_TRACE_EXITRC( SDB__STPSYNCSOURCEMGR_GETCLIENT, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__STPSYNCSOURCEMGR_REGISTERCLIENT, "_stpSyncSourceManager::registerClient" )
   INT32 _stpSyncSourceManager::registerClient( UINT32 version,
                                                const stpClientNode &client )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__STPSYNCSOURCEMGR_REGISTERCLIENT ) ;

      BOOLEAN versionExpired = FALSE ;
      UINT32 localVersion = STP_GROUP_INVALID_VERSION ;
      BOOLEAN locked = FALSE ;
      STP_CLIENT_MAP::iterator iter ;

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

      _clientMutex.lock_w() ;
      locked = TRUE ;

      // find if already registered
      iter = _clients.find( client.getRouteID() ) ;
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
      _clients[ client.getRouteID() ] = client ;

      _clientMutex.release_w() ;
      locked = FALSE ;

      PD_LOG( PDEVENT, "Register client node %s done",
              client.toString().c_str() ) ;

   done:
      if ( locked )
      {
         _clientMutex.release_w() ;
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

   // PD_TRACE_DECLARE_FUNCTION ( SDB__STPSYNCSOURCEMGR_REMOVECLIENT, "_stpSyncSourceManager::removeClient" )
   INT32 _stpSyncSourceManager::removeClient( const MsgRouteID &routeID )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__STPSYNCSOURCEMGR_REMOVECLIENT ) ;

      ossScopedRWLock lock( ( &_clientMutex ), EXCLUSIVE ) ;

      STP_CLIENT_MAP::iterator iter = _clients.find( routeID ) ;

      // find client to remove
      PD_CHECK( iter != _clients.end(),
                SDB_INVALID_ROUTEID, error, PDWARNING,
                "Failed to remove client node %s, route ID is not found",
                routeID2String( routeID ).c_str() ) ;

      // remove client
      _clients.erase( iter ) ;

      PD_LOG( PDEVENT, "Remove client node %s done",
              routeID2String( routeID ).c_str() ) ;

   done:
      PD_TRACE_EXITRC( SDB__STPSYNCSOURCEMGR_REMOVECLIENT, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__STPSYNCSOURCEMGR_REMOVECLIENT_EXPIRED, "_stpSyncSourceManager::removeClient" )
   INT32 _stpSyncSourceManager::removeClient( const MsgRouteID &routeID,
                                              UINT64 syncTick )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__STPSYNCSOURCEMGR_REMOVECLIENT_EXPIRED ) ;

      ossScopedRWLock lock( ( &_clientMutex ), EXCLUSIVE ) ;

      STP_CLIENT_MAP::iterator iter = _clients.find( routeID ) ;

      // find client to be removed
      PD_CHECK( iter != _clients.end(),
                SDB_INVALID_ROUTEID, error, PDWARNING,
                "Failed to remove client node %s, route ID is not found",
                routeID2String( routeID ).c_str() ) ;

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
                 routeID2String( routeID ).c_str() ) ;
         goto done ;
      }

      PD_LOG( PDEVENT, "Remove expired client node %s done",
              routeID2String( routeID ).c_str() ) ;

   done:
      PD_TRACE_EXITRC( SDB__STPSYNCSOURCEMGR_REMOVECLIENT_EXPIRED, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__STPSYNCSOURCEMGR_UPDATECLIENT, "_stpSyncSourceManager::updateClient" )
   INT32 _stpSyncSourceManager::updateClient( UINT32 version,
                                              const stpClientNode &client )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__STPSYNCSOURCEMGR_UPDATECLIENT ) ;

      BOOLEAN versionExpired = FALSE ;
      UINT32 localVersion = STP_GROUP_INVALID_VERSION ;
      BOOLEAN locked = FALSE ;
      STP_CLIENT_MAP::iterator iter ;

      // check role of client
      PD_CHECK( client.isValidRole(), SDB_INVALIDARG, error, PDERROR,
                "Failed to check client node %s, role is invalid",
                client.toString().c_str() ) ;

      // check group version of client, if it is too old, we need to tell
      // the client to update information of servers
      // NOTE: Event the version is expired, but we still need to finish
      //       the time synchronization.
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

      _clientMutex.lock_w() ;
      locked = TRUE ;

      // find client to be updated
      iter = _clients.find( client.getRouteID() ) ;
      PD_CHECK( iter != _clients.end(),
                SDB_INVALID_ROUTEID, error, PDERROR,
                "Failed to find client node %s", client.toString().c_str() ) ;

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

      _clientMutex.release_w() ;
      locked = FALSE ;

      PD_LOG( PDEVENT, "Update client node %s done",
              client.toString().c_str() ) ;

   done:
      if ( locked )
      {
         _clientMutex.release_w() ;
      }
      if ( versionExpired )
      {
         // tell client to update information of servers
         rc = SDB_REPL_REMOTE_G_V_EXPIRED ;
      }
      PD_TRACE_EXITRC( SDB__STPSYNCSOURCEMGR_UPDATECLIENT, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__STPSYNCSOURCEMGR_DUMPCLIENTS, "_stpSyncSourceManager::dumpClients" )
   INT32 _stpSyncSourceManager::dumpClients( STP_CLIENT_MAP &clients )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__STPSYNCSOURCEMGR_DUMPCLIENTS ) ;

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

      ossScopedRWLock lock( ( &_clientMutex ), EXCLUSIVE ) ;

      // remove all clients
      _clients.clear() ;

      PD_LOG( PDEVENT, "Remove all client nodes done" ) ;

      PD_TRACE_EXITRC( SDB__STPSYNCSOURCEMGR__REMOVECLIENTS, rc ) ;

      return rc ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__STPSYNCSOURCEMGR__CLEAREXPIREDCLIENTS, "_stpSyncSourceManager::_clearExpiredClients" )
   INT32 _stpSyncSourceManager::_clearExpiredClients()
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__STPSYNCSOURCEMGR__CLEAREXPIREDCLIENTS ) ;

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
      PD_TRACE_EXITRC( SDB__STPSYNCSOURCEMGR__CLEAREXPIREDCLIENTS, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__STPSYNCSOURCEMGR__SIGNALPUSHTIME, "_stpSyncSourceManager::_signalPushTime" )
   void _stpSyncSourceManager::_signalPushTime()
   {
      PD_TRACE_ENTRY( SDB__STPSYNCSOURCEMGR__SIGNALPUSHTIME ) ;

      // signal we need push time forward
      _pushEvent = TRUE ;

      PD_TRACE_EXIT( SDB__STPSYNCSOURCEMGR__SIGNALPUSHTIME ) ;
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
