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

   Source File Name = tpSourceManager.cpp

   Descriptive Name = SequoiaDB Time Protocol Service

   When/how to use: this program may be used on binary and text-formatted
   versions of Runtime component. This file contains structure for SequoiaDB
   Time Protocol Service.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          07/30/2019  HGM Initial Draft

   Last Changed =

*******************************************************************************/

#include "tpSourceManager.hpp"
#include "tpCB.hpp"
#include "pdTrace.hpp"
#include "tpTrace.hpp"
#include "pmd.hpp"
#include "msgMessage.hpp"

using namespace bson ;

namespace engine
{

   #define TP_CLEAR_CLIENT_INTERVAL       \
                     ( TP_SEC_TO_MILLISEC( ( TP_MAX_SYNC_INTERVAL ) * 2 ) )
   #define TP_SOURCE_PUSH_INTERVAL        \
                     ( TP_SEC_TO_MILLISEC( 60 ) )
   #define TP_SOURCE_PUSH_INTERVAL_NS     \
                     ( TP_MILLISEC_TO_NANOSEC( TP_SOURCE_PUSH_INTERVAL ) )

   /*
      _tpSourceManager implement
    */
   BEGIN_OBJ_MSG_MAP( _tpSourceManager, _tpManagerBase )
      // ON_MSG
      ON_MSG( MSG_TP_REG_REQ, processMessage )
      ON_MSG( MSG_TP_TIME_SYNC_REQ, processMessage )
   END_OBJ_MSG_MAP()

   _tpSourceManager::_tpSourceManager( SDB_TPCB *tpCB )
   : tpManagerBase( tpCB ),
     _pushEvent( FALSE ),
     _lastPushTick( 0LL ),
     _clearClientTimeout( 0LL )
   {
   }

   _tpSourceManager::~_tpSourceManager()
   {
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__TPSOURCEMGR_ONTIMER, "_tpSourceManager::onTimer" )
   void _tpSourceManager::onTimer( UINT64 timerID, UINT32 interval )
   {
      PD_TRACE_ENTRY( SDB__TPSOURCEMGR_ONTIMER ) ;

      if ( timerID == _timerID )
      {
         _clearClientTimeout += interval ;
         if ( _clearClientTimeout >= TP_CLEAR_CLIENT_INTERVAL )
         {
            if ( _tpCB->isPrimaryServer() )
            {
               _clearExpiredClients() ;
            }
            _clearClientTimeout = 0LL ;
         }
         if ( _needPushTime() )
         {
            _pushTime() ;
         }
      }

      PD_TRACE_EXIT( SDB__TPSOURCEMGR_ONTIMER ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__TPSOURCEMGR_PROCESSMESSAGE, "_tpSourceManager::processMessage" )
   INT32 _tpSourceManager::processMessage( NET_HANDLE handle,
                                           MsgHeader *message )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__TPSOURCEMGR_PROCESSMESSAGE ) ;

      SDB_ASSERT( NULL != message, "message is invalid" ) ;

      switch ( message->opCode )
      {
         case MSG_TP_REG_REQ :
         {
            rc = _handleRegReq( handle, (const MsgTpRegReq *)message ) ;
            PD_RC_CHECK( rc, PDERROR, "Failed to handle register request, "
                         "rc: %d", rc ) ;
            break ;
         }
         case MSG_TP_TIME_SYNC_REQ :
         {
            rc = _handleTimeSyncReq( handle,
                                    (const MsgTpTimeSyncReq *)message ) ;
            PD_RC_CHECK( rc, PDERROR, "Failed to handle synchronize time "
                         "request, rc: %d", rc ) ;
            break ;
         }
         default :
         {
            PD_CHECK( FALSE, SDB_UNKNOWN_MESSAGE, error, PDERROR,
                      "Unknown synchronize source message [%d]",
                      message->opCode ) ;
            break ;
         }
      }

   done:
      PD_TRACE_EXITRC( SDB__TPSOURCEMGR_PROCESSMESSAGE, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__TPSOURCEMGR_ONRECEIVETIMESYNCREQ, "_tpSourceManager::onReceiveTimeSyncReq" )
   INT32 _tpSourceManager::onReceiveTimeSyncReq( MsgTpTimeSyncReq *request )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__TPSOURCEMGR_ONRECEIVETIMESYNCREQ ) ;

      SDB_ASSERT( NULL != request, "request is invalid" ) ;

      tpHPTime receiveTime = getMetaData()->getLTValue() ;
      request->receiveTimeSec = receiveTime.getSecond() ;
      request->receiveTimeNanoSec = receiveTime.getNanoSecond() ;

      PD_TRACE_EXITRC( SDB__TPSOURCEMGR_ONRECEIVETIMESYNCREQ, rc ) ;

      return rc ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__TPSOURCEMGR_ONSENDTIMESYNCRSP, "_tpSourceManager::onSendTimeSyncRes" )
   INT32 _tpSourceManager::onSendTimeSyncRes( MsgTpTimeSyncRsp *response )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__TPSOURCEMGR_ONSENDTIMESYNCRSP ) ;

      SDB_ASSERT( NULL != response, "response is invalid" ) ;

      tpHPTime sendTime = getMetaData()->getLTValue() ;
      response->rspSendTimeSec = sendTime.getSecond() ;
      response->rspSendTimeNanoSec = sendTime.getNanoSecond() ;

      PD_TRACE_EXITRC( SDB__TPSOURCEMGR_ONSENDTIMESYNCRSP, rc ) ;

      return rc ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__TPSOURCEMGR__HANDLEREGREQ, "_tpSourceManager::_handleRegReq" )
   INT32 _tpSourceManager::_handleRegReq( NET_HANDLE handle,
                                          const MsgTpRegReq *request )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__TPSOURCEMGR__HANDLEREGREQ ) ;

      SDB_ASSERT( NULL != request, "request is invalid" ) ;

      const MsgRouteID &routeID = request->header.routeID ;
      tpClientNode client ;

      PD_CHECK( _tpCB->isPrimaryServer(),
                SDB_CLS_NOT_PRIMARY, error, PDERROR,
                "Failed to register client %s, primary is not me",
                routeID2String( routeID ).c_str() ) ;

      client.setRouteID( routeID ) ;
      client.setRole( (TP_ROLE)( request->role ) ) ;
      client.setSyncInterval( request->syncInterval ) ;
      client.setMaxTimeError( request->maxTimeError ) ;
      client.setTimeError( request->timeError ) ;
      client.setOID( request->oid ) ;

      rc = registerClient( request->version, client ) ;
      if ( SDB_OK != rc && SDB_REPL_REMOTE_G_V_EXPIRED != rc )
      {
         PD_RC_CHECK( rc, PDERROR, "Failed to register client %s, rc: %d",
                      client.toString().c_str(), rc ) ;
      }

      rc = _sendRegRsp( handle, request, client, rc ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "Failed to send register response, rc: %d", rc ) ;
      }

   done:
      PD_TRACE_EXITRC( SDB__TPSOURCEMGR__HANDLEREGREQ, rc ) ;
      return rc ;

   error:
      _sendRegRsp( handle, request, client, rc ) ;
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__TPSOURCEMGR__HANDLETIMESYNCREQ, "_tpSourceManager::_handleTimeSyncReq" )
   INT32 _tpSourceManager::_handleTimeSyncReq( NET_HANDLE handle,
                                               const MsgTpTimeSyncReq *request )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__TPSOURCEMGR__HANDLETIMESYNCREQ ) ;

      SDB_ASSERT( NULL != request, "request is invalid" ) ;

      const MsgRouteID &routeID = request->header.routeID ;
      TP_SYNC_STATUS status = (TP_SYNC_STATUS)( request->status ) ;
      UINT16 flag = request->flag ;
      tpClientNode client ;

      PD_CHECK( _tpCB->isPrimaryServer(), SDB_CLS_NOT_PRIMARY, error, PDERROR,
                "Failed to register node %s, primary is not me",
                routeID2String( routeID ).c_str() ) ;

      rc = getClient( routeID, client ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to get client %s, rc: %d",
                   routeID2String( routeID ).c_str(), rc ) ;

      if ( OSS_BIT_TEST( flag, TP_SYNC_TIME_FLAG_INCTIMEERROR ) )
      {
         client.increaseTimeError() ;
         PD_LOG( PDEVENT, "Client %s is in [%s] status, time error: [%u], "
                 "increase to [%u]", client.toString().c_str(),
                 tpGetSyncStatusName( status ), request->timeError,
                 client.getTimeError() ) ;
      }
      else if ( OSS_BIT_TEST( flag, TP_SYNC_TIME_FLAG_DECTIMEERROR ) )
      {
         client.decreaseTimeError() ;
         PD_LOG( PDEVENT, "Client %s is in [%s] status, time error: [%u], "
                 "decrease to [%u]", client.toString().c_str(),
                 tpGetSyncStatusName( status ), request->timeError,
                 client.getTimeError() ) ;
      }
      else if ( OSS_BIT_TEST( flag, TP_SYNC_TIME_FLAG_PUSHTIME ) )
      {
         _signalPushTime() ;
         PD_LOG( PDEVENT, "Client %s is in [%s] status, need push time",
                 client.toString().c_str(),
                 tpGetSyncStatusName( status ) ) ;
      }
      else
      {
         PD_LOG( PDDEBUG, "Client %s is in [%s] status",
                 client.toString().c_str(),
                 tpGetSyncStatusName( status ) ) ;
      }

      client.onSyncReq( (TP_SYNC_STATUS)request->status ) ;

      rc = updateClient( request->version, client ) ;
      if ( SDB_OK != rc && SDB_REPL_REMOTE_G_V_EXPIRED != rc )
      {
         PD_RC_CHECK( rc, PDERROR, "Failed to update client %s, rc: %d",
                      client.toString().c_str(), rc ) ;
      }

      rc = _sendTimeSyncRsp( handle, request, client, rc ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "Failed to send synchronize response, rc: %d", rc ) ;
      }

   done:
      PD_TRACE_EXITRC( SDB__TPSOURCEMGR__HANDLETIMESYNCREQ, rc ) ;
      return rc ;

   error:
      _sendTimeSyncRsp( handle, request, client, rc ) ;
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__TPSOURCEMGR__SENDREGRSP, "_tpSourceManager::_sendRegRsp" )
   INT32 _tpSourceManager::_sendRegRsp( NET_HANDLE handle,
                                        const MsgTpRegReq *request,
                                        const tpClientNode &client,
                                        INT32 returnCode )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__TPSOURCEMGR__SENDREGRSP ) ;

      SDB_ASSERT( NULL != request, "request is invalid" ) ;

      MsgTpRegRsp response ;

      _fillReplyHeader( request->header, response.reply,
                        sizeof( MsgTpRegRsp ), returnCode ) ;

      response.oid = client.getOID() ;

      rc = _netAgent->syncSend( handle, &response ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to send register response, "
                   "rc: %d", rc ) ;

   done:
      PD_TRACE_EXITRC( SDB__TPSOURCEMGR__SENDREGRSP, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__TPSOURCEMGR__SENDTIMESYNCRSP, "_tpSourceManager::_sendTimeSyncRsp" )
   INT32 _tpSourceManager::_sendTimeSyncRsp( NET_HANDLE handle,
                                             const MsgTpTimeSyncReq *request,
                                             const tpClientNode &client,
                                             INT32 returnCode )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__TPSOURCEMGR__SENDTIMESYNCRSP ) ;

      SDB_ASSERT( NULL != request, "request is invalid" ) ;

      MsgTpTimeSyncRsp response ;

      _fillReplyHeader( request->header, response.reply,
                        sizeof( MsgTpTimeSyncRsp ), returnCode ) ;
      response.reqSendTimeSec = request->sendTimeSec ;
      response.reqSendTimeNanoSec = request->sendTimeNanoSec ;
      response.reqReceiveTimeSec = request->receiveTimeSec ;
      response.reqReceiveTimeNanoSec = request->receiveTimeNanoSec ;
      response.rspTimeError = client.getTimeError() ;
      response.reqTimeError = request->timeError ;

      rc = _netAgent->syncSend( handle, &response ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to send synchronize time response, "
                   "rc: %d", rc ) ;

   done:
      PD_TRACE_EXITRC( SDB__TPSOURCEMGR__SENDTIMESYNCRSP, rc ) ;
      return rc ;

   error:
      goto done ;
   }


   // PD_TRACE_DECLARE_FUNCTION ( SDB__TPSOURCEMGR_GETCLIENT, "_tpSourceManager::getClient" )
   INT32 _tpSourceManager::getClient( const MsgRouteID &routeID,
                                      tpClientNode &client )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__TPSOURCEMGR_GETCLIENT ) ;

      ossScopedRWLock lock( ( &_clientMutex ), SHARED ) ;

      TP_CLIENT_MAP::const_iterator iter = _clients.find( routeID ) ;
      PD_CHECK( iter != _clients.end(),
                SDB_INVALID_ROUTEID, error, PDERROR,
                "Failed to get client node %s, it is not found",
                routeID2String( routeID ).c_str() ) ;

      client = iter->second ;

   done:
      PD_TRACE_EXITRC( SDB__TPSOURCEMGR_GETCLIENT, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__TPSOURCEMGR_REGISTERCLIENT, "_tpSourceManager::registerClient" )
   INT32 _tpSourceManager::registerClient( UINT32 version,
                                           const tpClientNode &client )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__TPSOURCEMGR_REGISTERCLIENT ) ;

      BOOLEAN versionExpired = FALSE ;
      UINT32 localVersion = TP_GROUP_INVALID_VERSION ;
      BOOLEAN locked = FALSE ;
      TP_CLIENT_MAP::iterator iter ;

      PD_CHECK( client.isValidRole(), SDB_INVALIDARG, error, PDERROR,
                "Failed to check client node %s, role is invalid",
                client.toString().c_str() ) ;

      PD_CHECK( client.isValidRoute(), SDB_INVALID_ROUTEID, error, PDERROR,
                "Failed to check client node %s, route ID is invalid",
                client.toString().c_str() ) ;

      PD_CHECK( client.isValidOID(), SDB_INVALIDARG, error, PDERROR,
                "Failed to check client node %s, OID is invalid",
                client.toString().c_str() ) ;

      localVersion = _catalogManager->getVersion() ;
      if ( version < localVersion )
      {
         PD_LOG( PDWARNING, "Client node %s's version is expired, "
                 "given is [%u], current is [%u]", client.toString().c_str(),
                 version, localVersion ) ;
         versionExpired = TRUE ;
      }

      _clientMutex.lock_w() ;
      locked = TRUE ;

      iter = _clients.find( client.getRouteID() ) ;
      if ( iter != _clients.end() )
      {
         PD_LOG( PDWARNING, "route ID is the same, remove old client "
                 "node %s", iter->second.toString().c_str() ) ;
         _clients.erase( iter ) ;
      }

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
         rc = SDB_REPL_REMOTE_G_V_EXPIRED ;
      }
      PD_TRACE_EXITRC( SDB__TPSOURCEMGR_REGISTERCLIENT, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__TPSOURCEMGR_REMOVECLIENT, "_tpSourceManager::removeClient" )
   INT32 _tpSourceManager::removeClient( const MsgRouteID &routeID )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__TPSOURCEMGR_REMOVECLIENT ) ;

      ossScopedRWLock lock( ( &_clientMutex ), EXCLUSIVE ) ;

      TP_CLIENT_MAP::iterator iter = _clients.find( routeID ) ;

      PD_CHECK( iter != _clients.end(),
                SDB_INVALID_ROUTEID, error, PDWARNING,
                "Failed to remove client node %s, route ID is not found",
                routeID2String( routeID ).c_str() ) ;

      _clients.erase( iter ) ;

      PD_LOG( PDEVENT, "Remove client node %s done",
              routeID2String( routeID ).c_str() ) ;

   done:
      PD_TRACE_EXITRC( SDB__TPSOURCEMGR_REMOVECLIENT, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__TPSOURCEMGR_REMOVECLIENT_EXPIRED, "_tpSourceManager::removeClient" )
   INT32 _tpSourceManager::removeClient( const MsgRouteID &routeID,
                                         UINT64 syncTick )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__TPSOURCEMGR_REMOVECLIENT_EXPIRED ) ;

      ossScopedRWLock lock( ( &_clientMutex ), EXCLUSIVE ) ;

      TP_CLIENT_MAP::iterator iter = _clients.find( routeID ) ;

      PD_CHECK( iter != _clients.end(),
                SDB_INVALID_ROUTEID, error, PDWARNING,
                "Failed to remove client node %s, route ID is not found",
                routeID2String( routeID ).c_str() ) ;

      if ( iter->second.getLastSyncTick() <= syncTick )
      {
         _clients.erase( iter ) ;
      }
      else
      {
         PD_LOG( PDDEBUG, "Ignored remove expired client node %s, "
                 "synchronize time is updated",
                 routeID2String( routeID ).c_str() ) ;
      }

      PD_LOG( PDEVENT, "Remove expired client node %s done",
              routeID2String( routeID ).c_str() ) ;

   done:
      PD_TRACE_EXITRC( SDB__TPSOURCEMGR_REMOVECLIENT_EXPIRED, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__TPSOURCEMGR_UPDATECLIENT, "_tpSourceManager::updateClient" )
   INT32 _tpSourceManager::updateClient( UINT32 version,
                                         const tpClientNode &client )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__TPSOURCEMGR_UPDATECLIENT ) ;

      BOOLEAN versionExpired = FALSE ;
      UINT32 localVersion = TP_GROUP_INVALID_VERSION ;
      BOOLEAN locked = FALSE ;
      TP_CLIENT_MAP::iterator iter ;

      PD_CHECK( client.isValidRole(), SDB_INVALIDARG, error, PDERROR,
                "Failed to check client node %s, role is invalid",
                client.toString().c_str() ) ;

      localVersion = _catalogManager->getVersion() ;
      if ( version < localVersion )
      {
         PD_LOG( PDWARNING, "Client node %s's version is expired, "
                 "given is [%u], current is [%u]", client.toString().c_str(),
                 version, localVersion ) ;
         versionExpired = TRUE ;
      }

      _clientMutex.lock_w() ;
      locked = TRUE ;

      iter = _clients.find( client.getRouteID() ) ;
      PD_CHECK( iter != _clients.end(),
                SDB_INVALID_ROUTEID, error, PDERROR,
                "Failed to find client node %s", client.toString().c_str() ) ;

      PD_CHECK( iter->second.getRouteIDValue() == client.getRouteIDValue(),
                SDB_INVALIDARG, error, PDERROR,
                "Failed to update client node %s, route ID is different, "
                "expected %s, given %s", client.toString().c_str(),
                routeID2String( iter->second.getRouteID() ).c_str(),
                routeID2String( client.getRouteID() ).c_str() ) ;

      PD_CHECK( iter->second.getOID() == client.getOID(),
                SDB_INVALIDARG, error, PDERROR,
                "Failed to update client node %s, OID is different, "
                "expected %s, given %s", client.toString().c_str(),
                iter->second.getOID().toString().c_str(),
                client.getOID().toString().c_str() ) ;

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
         rc = SDB_REPL_REMOTE_G_V_EXPIRED ;
      }
      PD_TRACE_EXITRC( SDB__TPSOURCEMGR_UPDATECLIENT, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__TPSOURCEMGR_DUMPCLIENTS, "_tpSourceManager::dumpClients" )
   INT32 _tpSourceManager::dumpClients( TP_CLIENT_MAP &clients )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__TPSOURCEMGR_DUMPCLIENTS ) ;

      ossScopedRWLock lock( ( &_clientMutex ), SHARED ) ;

      try
      {
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
      PD_TRACE_EXITRC( SDB__TPSOURCEMGR_DUMPCLIENTS, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__TPSOURCEMGR__REMOVECLIENTS, "_tpSourceManager::removeClients" )
   INT32 _tpSourceManager::removeClients()
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__TPSOURCEMGR__REMOVECLIENTS ) ;

      ossScopedRWLock lock( ( &_clientMutex ), EXCLUSIVE ) ;
      _clients.clear() ;

      PD_LOG( PDEVENT, "Remove all client nodes done" ) ;

      PD_TRACE_EXITRC( SDB__TPSOURCEMGR__REMOVECLIENTS, rc ) ;

      return rc ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__TPSOURCEMGR__CLEAREXPIREDCLIENTS, "_tpSourceManager::_clearExpiredClients" )
   INT32 _tpSourceManager::_clearExpiredClients()
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__TPSOURCEMGR__CLEAREXPIREDCLIENTS ) ;

      TP_CLIENT_MAP clients ;

      rc = dumpClients( clients ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to dump clients, rc: %d", rc ) ;

      for ( TP_CLIENT_MAP::iterator iter = clients.begin() ;
            iter != clients.end() ;
            ++ iter )
      {
         UINT64 syncTick = iter->second.getLastSyncTick() ;
         UINT64 syncPassed = pmdGetTickSpanTime( syncTick ) ;
         if ( syncPassed > TP_CLEAR_CLIENT_INTERVAL )
         {
            removeClient( iter->first, syncTick ) ;
         }
      }

   done:
      PD_TRACE_EXITRC( SDB__TPSOURCEMGR__CLEAREXPIREDCLIENTS, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__TPSOURCEMGR__SIGNALPUSHTIME, "_tpSourceManager::_signalPushTime" )
   void _tpSourceManager::_signalPushTime()
   {
      PD_TRACE_ENTRY( SDB__TPSOURCEMGR__SIGNALPUSHTIME ) ;

      _pushEvent = TRUE ;

      PD_TRACE_EXIT( SDB__TPSOURCEMGR__SIGNALPUSHTIME ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__TPSOURCEMGR__NEEDPUSHTIME, "_tpSourceManager::_needPushTime" )
   BOOLEAN _tpSourceManager::_needPushTime()
   {
      BOOLEAN needPush = FALSE ;

      PD_TRACE_ENTRY( SDB__TPSOURCEMGR__NEEDPUSHTIME ) ;

      if ( _pushEvent )
      {
         UINT64 pushPassed = pmdGetTickSpanTime( _lastPushTick ) ;
         if ( pushPassed > TP_SOURCE_PUSH_INTERVAL )
         {
            needPush = TRUE ;
         }
         _pushEvent = FALSE ;
      }

      PD_TRACE_EXIT( SDB__TPSOURCEMGR__NEEDPUSHTIME ) ;

      return needPush ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__TPSOURCEMGR__PUSHTIME, "_tpSourceManager::_pushTime" )
   void _tpSourceManager::_pushTime()
   {
      getMetaData()->adjustLogicalTime( TP_SOURCE_PUSH_INTERVAL_NS ) ;

      _pushEvent = FALSE ;
      _lastPushTick = pmdGetDBTick() ;
   }

}
