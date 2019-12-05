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

   Source File Name = tpCatalogManager.cpp

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

#include "tpCatalogManager.hpp"
#include "tpCB.hpp"
#include "pdTrace.hpp"
#include "tpTrace.hpp"
#include "pmd.hpp"
#include "msgMessage.hpp"
#include "pmdEnv.hpp"

using namespace std ;
using namespace bson ;

namespace engine
{

   /*
      _tpCatalogManager implement
    */
   BEGIN_OBJ_MSG_MAP( _tpCatalogManager, _tpManagerBase )
      // ON_MSG
      ON_MSG( MSG_TP_SERVER_REQ, processMessage )
      ON_MSG( MSG_TP_SERVER_RSP, processMessage )
   END_OBJ_MSG_MAP()

   _tpCatalogManager::_tpCatalogManager( SDB_TPCB *tpCB )
   : tpManagerBase( tpCB ),
     _status( TP_CATALOG_NORMAL ),
     _version( TP_GROUP_INVALID_VERSION )
   {
      _primaryRID.value = MSG_INVALID_ROUTEID ;
   }

   _tpCatalogManager::~_tpCatalogManager()
   {
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__TPCATALOGMGR_ONTIMER, "_tpCatalogManager::onTimer" )
   void _tpCatalogManager::onTimer( UINT64 timerID, UINT32 interval )
   {
      PD_TRACE_ENTRY( SDB__TPCATALOGMGR_ONTIMER ) ;

      if ( timerID == _timerID )
      {
         launchServerCheck() ;
      }

      PD_TRACE_EXIT( SDB__TPCATALOGMGR_ONTIMER ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__TPCATALOGMGR_PROCESSMESSAGE, "_tpCatalogManager::processMessage" )
   INT32 _tpCatalogManager::processMessage( NET_HANDLE handle,
                                            MsgHeader *message )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__TPCATALOGMGR_PROCESSMESSAGE ) ;

      SDB_ASSERT( NULL != message, "message is invalid" ) ;

      switch ( message->opCode )
      {
         case MSG_TP_SERVER_REQ :
         {
            rc = _handleServerReq( handle,
                                   (const MsgTpServerReq *)message ) ;
            PD_RC_CHECK( rc, PDERROR, "Failed to handle server request, "
                         "rc: %d", rc ) ;
            break ;
         }
         case MSG_TP_SERVER_RSP :
         {
            rc = _handleServerRsp( handle,
                                   (const MsgTpServerRsp *)message ) ;
            PD_RC_CHECK( rc, PDERROR, "Failed to handle server result, "
                         "rc: %d", rc ) ;
            break ;
         }
         default :
            PD_CHECK( FALSE, SDB_UNKNOWN_MESSAGE, error, PDERROR,
                      "Unknown catalog message [%d]",
                      message->opCode ) ;
            break ;
      }

   done:
      PD_TRACE_EXITRC( SDB__TPCATALOGMGR_PROCESSMESSAGE, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__TPCATALOGMGR__INITIALIZE, "_tpCatalogManager::_initialize" )
   INT32 _tpCatalogManager::_initialize()
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__TPCATALOGMGR__INITIALIZE ) ;

      rc = _initServers() ;
      PD_RC_CHECK( rc, PDERROR, "Failed to initialize server group, rc: %d",
                   rc ) ;

      rc = _initLocal() ;
      PD_RC_CHECK( rc, PDERROR, "Failed to initialize local node, rc: %d",
                   rc ) ;

      rc = _checkLocalAndServers() ;
      PD_RC_CHECK( rc, PDERROR, "Failed to check local node and server group, "
                   "rc: %d", rc ) ;

   done:
      PD_TRACE_EXITRC( SDB__TPCATALOGMGR__INITIALIZE, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__TPCATALOGMGR__POSTACTIVATE, "_tpCatalogManager::_postActivate" )
   INT32 _tpCatalogManager::_postActivate()
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__TPCATALOGMGR__POSTACTIVATE ) ;

      launchServerCheck() ;

      PD_TRACE_EXITRC( SDB__TPCATALOGMGR__POSTACTIVATE, rc ) ;

      return rc ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__TPCATALOGMGR__ONCHANGEPRIMARY, "_tpCatalogManager::_onChangePrimary" )
   INT32 _tpCatalogManager::_onChangePrimary( const MsgRouteID &primaryRID,
                                              BOOLEAN isLocalPrimary )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__TPCATALOGMGR__ONCHANGEPRIMARY ) ;

      setPrimaryRID( primaryRID ) ;

      PD_TRACE_EXITRC( SDB__TPCATALOGMGR__ONCHANGEPRIMARY, rc ) ;

      return rc ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__TPCATALOGMGR__HANDLESERVERREQ, "_tpCatalogManager::_handleServerReq" )
   INT32 _tpCatalogManager::_handleServerReq( NET_HANDLE handle,
                                              const MsgTpServerReq *request )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__TPCATALOGMGR__HANDLESERVERREQ ) ;

      MSG_TP_SERVER_REQ_TYPE reqType =
                                 (MSG_TP_SERVER_REQ_TYPE)( request->type ) ;
      MsgRouteID routeID = request->header.routeID ;
      BSONObj groupObject ;

      PD_LOG( PDEVENT, "Got server quest [%s] from %s",
              msgGetTpServerReqName( reqType ),
              routeID2String( routeID ).c_str() ) ;

      switch ( reqType )
      {
         case MSG_TP_ADD_SERVER :
         {
            rc = _handleAddServer( handle, routeID ) ;
            PD_RC_CHECK( rc, PDERROR, "Failed to handle add server "
                         "request from  %s, rc: %d",
                         routeID2String( routeID ).c_str(), rc ) ;
            break ;
         }
         case MSG_TP_REMOVE_SERVER :
         {
            rc = _handleRemoveServer( handle, routeID ) ;
            PD_RC_CHECK( rc, PDERROR, "Failed to handle remove server "
                         "request from  %s, rc: %d",
                         routeID2String( routeID ).c_str(), rc ) ;
            break ;
         }
         default :
         {
            PD_CHECK( hasPrimary(), SDB_RTN_NO_PRIMARY_FOUND, error, PDERROR,
                      "Failed to handle query server request, "
                      "current node does not know who is primary") ;
            break ;
         }
      }

      rc = getServers( groupObject ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to get server group, rc: %d", rc ) ;

      rc = _sendServerRsp( handle, request, groupObject, SDB_OK ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "Failed to send server result, rc: %d", rc ) ;
      }

   done:
      PD_TRACE_EXITRC( SDB__TPCATALOGMGR__HANDLESERVERREQ, rc ) ;
      return rc ;

   error:
      _sendServerRsp( handle, request, groupObject, rc ) ;
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__TPCATALOGMGR__HANDLESERVERRSP, "_tpCatalogManager::_handleServerRsp" )
   INT32 _tpCatalogManager::_handleServerRsp( NET_HANDLE handle,
                                              const MsgTpServerRsp *response )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__TPCATALOGMGR__HANDLESERVERRSP ) ;

      SDB_ASSERT( NULL != response, "result is invalid" ) ;

      rc = response->reply.res ;
      if ( SDB_OK == rc )
      {
         try
         {
            BSONObj object ;

            PD_CHECK( response->reply.header.messageLength >=
                      (INT32)( sizeof( MsgTpServerRsp ) +
                               object.objsize() ),
                      SDB_SYS, error, PDERROR, "Failed to handle server "
                      "response, size of message is unexpected, "
                      "expected >= [%u], given [%u]",
                      sizeof( MsgTpServerRsp ) + object.objsize(),
                      response->reply.header.messageLength ) ;

            object = BSONObj( (CHAR *)( response ) +
                              sizeof( MsgTpServerRsp ) ) ;

            rc = setServers( object ) ;
            PD_RC_CHECK( rc, PDERROR, "Failed to set server group, "
                         "rc: %d", rc ) ;
         }
         catch ( exception &e )
         {
            rc = SDB_SYS ;
            PD_LOG( PDERROR, "Failed to parse server group object, "
                    "occurred unexpected error: %s", e.what() ) ;
            goto error ;
         }
      }
      else
      {
         resetPrimary() ;
         PD_RC_CHECK( rc, PDERROR, "Failed to query server, "
                      "received result with error: %d", rc ) ;
      }

   done:
      PD_TRACE_EXITRC( SDB__TPCATALOGMGR__HANDLESERVERRSP, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__TPCATALOGMGR__HANDLEADDSERVER, "_tpCatalogManager::_handleAddServer" )
   INT32 _tpCatalogManager::_handleAddServer( NET_HANDLE handle,
                                              const MsgRouteID &routeID )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__TPCATALOGMGR__HANDLEADDSERVER ) ;

      tpServerNode server ;
      NET_EH eh ;
      CHAR serviceName[ OSS_MAX_SERVICENAME + 1 ] = { 0 } ;

      PD_CHECK( isPrimaryServer(), SDB_CLS_NOT_PRIMARY, error, PDERROR,
                "Failed to handle add server request, "
                "current node is not primary" ) ;

      eh = _netAgent->getFrame()->getEventHandle( handle ) ;
      PD_CHECK( NULL != eh.get(), SDB_NET_INVALID_HANDLE, error, PDERROR,
                "Failed to get net event handler [%u], rc: %d", handle, rc ) ;

      ossSnprintf( serviceName, OSS_MAX_SERVICENAME, "%d",
                   eh->remotePort() ) ;

      server.setRouteID( routeID ) ;
      server.setHostName( eh->remoteAddr().c_str() ) ;
      server.setServiceName( serviceName ) ;

      rc = _addServer( server, TRUE ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to add server %s, rc: %d",
                   server.toString().c_str(), rc ) ;

      _saveServers() ;
      _checkLocalAndServers() ;
      if ( TP_CATALOG_NORMAL == getStatus() )
      {
         _tpCB->onChangeServers() ;
      }

   done:
      PD_TRACE_EXITRC( SDB__TPCATALOGMGR__HANDLEADDSERVER, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__TPCATALOGMGR__HANDLEREMOVESERVER, "_tpCatalogManager::_handleRemoveServer" )
   INT32 _tpCatalogManager::_handleRemoveServer( NET_HANDLE handle,
                                                 const MsgRouteID &routeID )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__TPCATALOGMGR__HANDLEREMOVESERVER ) ;

      PD_CHECK( isPrimaryServer(), SDB_CLS_NOT_PRIMARY, error, PDERROR,
                "Failed to handle remove server request, "
                "current node is not primary" ) ;

      rc = _removeServer( routeID, TRUE ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to remove server %s, rc: %d",
                   routeID2String( routeID ).c_str(), rc ) ;

      _saveServers() ;
      _checkLocalAndServers() ;
      if ( TP_CATALOG_NORMAL == getStatus() )
      {
         _tpCB->onChangeServers() ;
      }

   done:
      PD_TRACE_EXITRC( SDB__TPCATALOGMGR__HANDLEREMOVESERVER, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__TPCATALOGMGR__SENDSERVERREQ, "_tpCatalogManager::_sendServerReq" )
   INT32 _tpCatalogManager::_sendServerReq( const MsgRouteID &routeID,
                                            MSG_TP_SERVER_REQ_TYPE type )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__TPCATALOGMGR__SENDSERVERREQ ) ;

      MsgTpServerReq request ;

      _fillRequestHeader( request.header, sizeof( MsgTpServerReq ),
                          MSG_TP_SERVER_REQ ) ;

      request.type = (UINT16)type ;

      rc = _netAgent->syncSend( routeID, &request ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to send server request to %s, "
                   "rc: %d", routeID2String( routeID ).c_str(), rc ) ;

   done:
      PD_TRACE_EXITRC( SDB__TPCATALOGMGR__SENDSERVERREQ, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__TPCATALOGMGR__SENDSERVERRSP, "_tpCatalogManager::_sendServerRsp" )
   INT32 _tpCatalogManager::_sendServerRsp( NET_HANDLE handle,
                                            const MsgTpServerReq *request,
                                            const BSONObj &object,
                                            INT32 returnCode )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__TPCATALOGMGR__SENDSERVERRSP ) ;

      SDB_ASSERT( NULL != request, "request is invalid" ) ;

      MsgTpServerRsp response ;

      _fillReplyHeader( request->header, response.reply,
                        sizeof( MsgTpServerRsp ) + object.objsize(),
                        returnCode ) ;

      rc = _netAgent->syncSend( handle,
                                (MsgHeader *)( &response ),
                                (void *)( object.objdata() ),
                                object.objsize() ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to send server response, rc: %d",
                   rc ) ;

   done:
      PD_TRACE_EXITRC( SDB__TPCATALOGMGR__SENDSERVERRSP, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__TPCATALOGMGR_SETLOCAL, "_tpCatalogManager::setLocal" )
   INT32 _tpCatalogManager::setLocal( const tpClientNode &local )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__TPCATALOGMGR_SETLOCAL ) ;

      ossScopedRWLock lock( &_mutex, EXCLUSIVE ) ;

      if ( _local.isValidRoute() )
      {
         PD_CHECK( _local.getRouteIDValue() == local.getRouteIDValue(),
                   SDB_INVALIDARG, error, PDERROR,
                   "Failed to set local, route ID is different, "
                   "expected %s, given %s",
                   routeID2String( _local.getRouteID() ).c_str(),
                   routeID2String( local.getRouteID() ).c_str() ) ;
      }
      else
      {
         PD_CHECK( local.isValidRoute(), SDB_INVALIDARG, error, PDERROR,
                   "Failed to set local, route ID is invalid" ) ;
      }

      if ( _local.isValidOID() )
      {
         PD_CHECK( _local.getOID() == local.getOID(),
                   SDB_INVALIDARG, error, PDWARNING,
                   "Failed to set local, OID is different, ignored "
                   "expected [%s], given [%s]",
                   _local.getOID().toString().c_str(),
                   local.getOID().toString().c_str() ) ;
      }

      _local = local ;
      pmdSetDBRole( SDB_ROLE_TP ) ;

   done:
      PD_TRACE_EXITRC( SDB__TPCATALOGMGR_SETLOCAL, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__TPCATALOGMGR_CHKEXPIREDVER, "_tpCatalogManager::checkExpiredVersion" )
   void _tpCatalogManager::checkExpiredVersion( UINT32 expiredVersion )
   {
      PD_TRACE_ENTRY( SDB__TPCATALOGMGR_CHKEXPIREDVER ) ;

      ossScopedRWLock lock( &_mutex, EXCLUSIVE ) ;

      if ( _version <= expiredVersion )
      {
         PD_LOG( PDWARNING, "Server is reported expired, current is [%u]",
                 _version ) ;
         _setStatus( TP_CATALOG_QUERY ) ;
      }

      PD_TRACE_EXIT( SDB__TPCATALOGMGR_CHKEXPIREDVER ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__TPCATALOGMGR_LAUNCHSERVERCHECK, "_tpCatalogManager::launchServerCheck" )
   INT32 _tpCatalogManager::launchServerCheck()
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__TPCATALOGMGR_LAUNCHSERVERCHECK ) ;

      if ( needQueryServer() )
      {
         MsgRouteID serverRID ;

         rc = _session.getPrimaryRID( serverRID ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to get server RID, rc: %d", rc ) ;

         rc = _sendServerReq( serverRID, MSG_TP_QUERY_SERVER ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to send query server request, "
                      "rc: %d", rc ) ;
      }
      else if ( needAddServer() )
      {
         MsgRouteID serverRID ;

         rc = _session.getPrimaryRID( serverRID ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to get server RID, rc: %d", rc ) ;

         rc = _sendServerReq( serverRID, MSG_TP_ADD_SERVER ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to send add server request, "
                      "rc: %d", rc ) ;
      }
      else if ( needRemoveServer() )
      {
         MsgRouteID serverRID ;

         rc = _session.getPrimaryRID( serverRID ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to get server RID, rc: %d", rc ) ;

         rc = _sendServerReq( serverRID, MSG_TP_REMOVE_SERVER ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to send remove server request, "
                      "rc: %d", rc ) ;
      }

   done:
      PD_TRACE_EXITRC( SDB__TPCATALOGMGR_LAUNCHSERVERCHECK, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__TPCATALOGMGR_GETSERVERRID, "_tpCatalogManager::getServerRID" )
   INT32 _tpCatalogManager::getServerRID( MsgRouteID &routeID,
                                          BOOLEAN preferPrimary )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__TPCATALOGMGR_GETSERVERRID ) ;

      ossScopedRWLock lock( &_mutex, SHARED ) ;

      PD_CHECK( _servers.size() > 0, SDB_INVALID_ROUTEID, error, PDERROR,
                "Failed to get server RID, server group is empty" ) ;

      // return primary if known
      if ( preferPrimary && MSG_INVALID_ROUTEID != _primaryRID.value )
      {
         routeID.value = _primaryRID.value ;
      }
      else
      {
         // loop each server
         TP_SERVER_LIST::iterator iter = find( _servers.begin(),
                                               _servers.end(),
                                               routeID ) ;
         if ( iter != _servers.end() )
         {
            ++ iter ;
         }
         if ( iter == _servers.end() )
         {
            iter = _servers.begin() ;
         }
         routeID.value = iter->getRouteIDValue() ;
      }

   done:
      PD_TRACE_EXITRC( SDB__TPCATALOGMGR_GETSERVERRID, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__TPCATALOGMGR_GETSERVER, "_tpCatalogManager::getServer" )
   INT32 _tpCatalogManager::getServer( const MsgRouteID &routeID,
                                       tpServerNode &server )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__TPCATALOGMGR_GETSERVER ) ;

      ossScopedRWLock lock( ( &_mutex ), SHARED ) ;

      TP_SERVER_LIST::const_iterator iter = find( _servers.begin(),
                                                  _servers.end(),
                                                  routeID ) ;
      PD_CHECK( iter != _servers.end(), SDB_INVALID_ROUTEID, error, PDERROR,
                "Failed to get server node %s, it is not found",
                routeID2String( routeID ).c_str() ) ;

      server = ( *iter ) ;

   done:
      PD_TRACE_EXITRC( SDB__TPCATALOGMGR_GETSERVER, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__TPCATALOGMGR_DUMPSERVERS, "_tpCatalogManager::dumpServers" )
   INT32 _tpCatalogManager::dumpServers( TP_SERVER_LIST &servers )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__TPCATALOGMGR_DUMPSERVERS ) ;

      ossScopedRWLock lock( ( &_mutex ), SHARED ) ;

      try
      {
         servers = _servers ;
      }
      catch ( exception &e )
      {
         PD_LOG( PDERROR, "Failed to dump sources, "
                 "occurred unexpected error: %s", e.what() ) ;
         rc = SDB_SYS ;
         goto error ;
      }

   done:
      PD_TRACE_EXITRC( SDB__TPCATALOGMGR_DUMPSERVERS, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__TPCATALOGMGR_REMOVESERVERS, "_tpCatalogManager::removeServers" )
   INT32 _tpCatalogManager::removeServers()
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__TPCATALOGMGR_REMOVESERVERS ) ;

      TP_SERVER_LIST servers ;

      rc = dumpServers( servers ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to dump servers, rc: %d", rc ) ;

      _mutex.lock_w() ;
      _servers.clear() ;
      _mutex.release_w() ;

      for ( TP_SERVER_LIST::iterator iter = servers.begin() ;
            iter != servers.end() ;
            ++ iter )
      {
         _netAgent->delRoute( iter->getRouteID() ) ;
      }

      PD_LOG( PDEVENT, "Remove all server nodes done" ) ;

   done:
      PD_TRACE_EXITRC( SDB__TPCATALOGMGR_REMOVESERVERS, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__TPCATALOGMGR_GETSERVERS, "_tpCatalogManager::getServers" )
   INT32 _tpCatalogManager::getServers( UINT32 &version,
                                        TP_SERVER_LIST &servers )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__TPCATALOGMGR_GETSERVERS ) ;

      MsgRouteID primaryRID ;

      _getServers( version, servers, primaryRID ) ;

      PD_TRACE_EXITRC( SDB__TPCATALOGMGR_GETSERVERS, rc ) ;

      return rc ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__TPCATALOGMGR_GETSERVERS_BSON, "_tpCatalogManager::getServers" )
   INT32 _tpCatalogManager::getServers( BSONObj &object )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__TPCATALOGMGR_GETSERVERS_BSON ) ;

      UINT32 version = TP_GROUP_INVALID_VERSION ;
      TP_SERVER_LIST servers ;
      MsgRouteID primaryRID ;

      _getServers( version, servers, primaryRID ) ;

      try
      {
         BSONObjBuilder builder ;

         rc = _buildVersion( builder, version ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to build version object, "
                      "rc: %d", rc ) ;

         rc = _buildServers( builder, servers ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to build server group object, "
                      "rc: %d", rc ) ;

         rc = _buildPrimaryNode( builder, servers, primaryRID ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to build primary node object, "
                      "rc: %d", rc ) ;

         object = builder.obj() ;
      }
      catch ( exception &e )
      {
         rc = SDB_SYS ;
         PD_LOG( PDERROR, "Failed to build server group object, "
                 "occurred unexpected error: %s", e.what() ) ;
         goto error ;
      }

   done:
      PD_TRACE_EXITRC( SDB__TPCATALOGMGR_GETSERVERS_BSON, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__TPCATALOGMGR_SETSERVERS_BSON, "_tpCatalogManager::setServers" )
   INT32 _tpCatalogManager::setServers( const BSONObj &object )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__TPCATALOGMGR_SETSERVERS_BSON ) ;

      UINT32 version = TP_GROUP_INVALID_VERSION ;
      TP_SERVER_LIST servers, removedServers ;
      MsgRouteID primaryRID ;

      try
      {
         rc = _parseVersion( object, version ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to parse version object, "
                      "rc: %d", rc ) ;

         rc = _parseServers( object, servers ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to parse server object, "
                      "rc: %d", rc ) ;

         rc = _parsePrimaryNode( object, primaryRID ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to parse primary node object, "
                      "rc: %d", rc ) ;
      }
      catch ( exception &e )
      {
         rc = SDB_SYS ;
         PD_LOG( PDERROR, "Failed to parse server group object, "
                 "occurred unexpected error: %s", e.what() ) ;
         goto error ;
      }

      for ( TP_SERVER_LIST::iterator iter = servers.begin() ;
            iter != servers.end() ;
            ++ iter )
      {
         tpServerNode &server = ( *iter ) ;
         rc = _updateRouteID( server.getRouteID(),
                              server.getHostName(),
                              server.getServiceName() ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to update route %s:%s for "
                      "server node %s, rc: %d", server.getHostName(),
                      server.getServiceName(), server.toString().c_str(),
                      rc ) ;
      }

      _setServers( version, servers, primaryRID, removedServers ) ;

      for ( TP_SERVER_LIST::iterator iter = removedServers.begin() ;
            iter != removedServers.end() ;
            ++ iter )
      {
         _deleteRouteID( iter->getRouteID() ) ;
      }

      rc = _saveServers() ;
      PD_RC_CHECK( rc, PDERROR, "Failed to save options, rc: %d", rc ) ;

      _checkLocalAndServers() ;
      if ( TP_CATALOG_NORMAL == getStatus() )
      {
         _tpCB->onChangeServers() ;
      }

   done:
      PD_TRACE_EXITRC( SDB__TPCATALOGMGR_SETSERVERS_BSON, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__TPCATALOGMGR_UPDATECONFIGS, "_tpCatalogManager::updateConfigs" )
   INT32 _tpCatalogManager::updateConfigs()
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__TPCATALOGMGR_UPDATELOCALANDSERVERS ) ;

      BOOLEAN localUpdated = FALSE, serversUpdated = FALSE ;

      rc = _updateLocalConfigs( localUpdated ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to update local, rc: %d", rc ) ;

      rc = _updateServerConfigs( serversUpdated ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to udpate servers, rc: %d", rc ) ;

      if ( localUpdated || serversUpdated )
      {
         // reset status
         setStatus( TP_CATALOG_NORMAL ) ;
         _checkLocalAndServers() ;
         if ( TP_CATALOG_NORMAL == getStatus() )
         {
            _tpCB->onChangeServers() ;
         }
      }

   done:
      PD_TRACE_EXITRC( SDB__TPCATALOGMGR_UPDATELOCALANDSERVERS, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__TPCATALOGMGR__UPDATEROUTEID, "_tpCatalogManager::_updateRouteID" )
   INT32 _tpCatalogManager::_updateRouteID( const MsgRouteID &routeID,
                                            const CHAR *hostName,
                                            const CHAR *serviceName )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__TPCATALOGMGR__UPDATEROUTEID ) ;

      // update route
      rc = _netAgent->updateRoute( routeID, hostName, serviceName ) ;
      if ( SDB_OK != rc && SDB_NET_UPDATE_EXISTING_NODE != rc )
      {
         PD_RC_CHECK( rc, PDERROR, "Failed to update route %s:%s for "
                      "route ID %s, rc: %d", hostName, serviceName,
                      routeID2String( routeID ).c_str(), rc ) ;
      }
      rc = SDB_OK ;

   done:
      PD_TRACE_EXITRC( SDB__TPCATALOGMGR__UPDATEROUTEID, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__TPCATALOGMGR__DELETEROUTEID, "_tpCatalogManager::_deleteRouteID" )
   INT32 _tpCatalogManager::_deleteRouteID( const MsgRouteID &routeID )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__TPCATALOGMGR__DELETEROUTEID ) ;

      _netAgent->delRoute( routeID ) ;

      PD_TRACE_EXITRC( SDB__TPCATALOGMGR__DELETEROUTEID, rc ) ;

      return rc ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__TPCATALOGMGR__INITLOCAL, "_tpCatalogManager::_initLocal" )
   INT32 _tpCatalogManager::_initLocal()
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__TPCATALOGMGR__INITLOCAL ) ;

      tpClientNode local ;
      const CHAR *hostName = pmdGetKRCB()->getHostName() ;
      const CHAR *serviceName = _options->getServiceName() ;
      MsgRouteID routeID ;
      tpServerNode server ;

      routeID.value = MSG_INVALID_ROUTEID ;
      rc = _getRouteID( hostName, serviceName, routeID ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to get route ID %s:%s, "
                   "rc: %d", hostName, serviceName, rc ) ;

      rc = _updateRouteID( routeID, hostName, serviceName ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to update route ID for node %s, "
                   "rc: %d", _local.toString().c_str(), rc ) ;

      local.setRouteID( routeID ) ;
      local.setRole( _options->getRole() ) ;
      local.setSyncInterval( _options->getSyncInterval() ) ;
      local.setMaxTimeError( _options->getMaxTimeError() ) ;

      setLocal( local ) ;

      PD_LOG( PDEVENT, "Initialize local node %s done",
              local.toString().c_str() ) ;

   done:
      PD_TRACE_EXITRC( SDB__TPCATALOGMGR__INITLOCAL, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__TPCATALOGGROUP__INITSERVERS, "_tpCatalogManager::_initServers" )
   INT32 _tpCatalogManager::_initServers()
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__TPCATALOGGROUP__INITSERVERS ) ;

      TP_ROLE role = _options->getRole() ;
      vector< pmdAddrPair > serverList = _options->getServerList() ;

      if ( TP_ROLE_STANDALONE == role )
      {
         setVersion( TP_GROUP_STANDALONE_VERSION ) ;
         goto done ;
      }

      PD_CHECK( TP_ROLE_SERVER == role || TP_ROLE_CLIENT == role,
                SDB_INVALIDARG, error, PDWARNING,
                "Failed to to initialize servers, unknown role: %d",
                role ) ;

      PD_CHECK( !serverList.empty(), SDB_INVALIDARG, error, PDERROR,
                "Failed to initialize servers, empty server list" ) ;

      rc = _updateServers( serverList ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to update servers, rc: %d", rc ) ;

      setVersion( TP_GROUP_INIT_VERSION ) ;

   done:
      PD_TRACE_EXITRC( SDB__TPCATALOGGROUP__INITSERVERS, rc ) ;
      return rc ;

   error:
      goto done ;
   }


   // PD_TRACE_DECLARE_FUNCTION ( SDB__TPCATALOGMGR__UPDATELOCALCONFIGS, "_tpCatalogManager::_updateLocalConfigs" )
   INT32 _tpCatalogManager::_updateLocalConfigs( BOOLEAN &changed )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__TPCATALOGMGR__UPDATELOCALCONFIGS ) ;

      changed = FALSE ;

      BOOLEAN roleChanged = FALSE ;
      BOOLEAN syncIntervalChanged = FALSE ;
      BOOLEAN maxTimeErrorChanged = FALSE ;

      _mutex.lock_w() ;

      if ( _local.getRole() != _options->getRole() )
      {
         PD_LOG( PDEVENT, "[%s] changed from [%s] to [%s]",
                 PMD_OPTION_ROLE,
                 tpGetRoleName( _local.getRole() ),
                 tpGetRoleName( _options->getRole() ) ) ;
         _local.setRole( _options->getRole() ) ;
         roleChanged = TRUE ;
      }
      if ( _local.getMaxTimeError() != _options->getMaxTimeError() )
      {
         PD_LOG( PDEVENT, "[%s] changed from [%u] to [%u]",
                 PMD_TP_OPTION_MAXTIMEERROR,
                 _local.getMaxTimeError(),
                 _options->getMaxTimeError() ) ;

         _local.setMaxTimeError( _options->getMaxTimeError() ) ;
         maxTimeErrorChanged = TRUE ;
      }
      if ( _local.getSyncInterval() != _options->getSyncInterval() )
      {
         PD_LOG( PDEVENT, "[%s] changed from [%u] to [%u]",
                 PMD_TP_OPTION_SYNCINTERVAL,
                 _local.getSyncInterval(),
                 _options->getSyncInterval() ) ;

         _local.setSyncInterval( _options->getSyncInterval() ) ;
         syncIntervalChanged = TRUE ;
      }

      _mutex.release_w() ;

      if ( roleChanged )
      {
         _tpCB->onChangeRole( _options->getRole() ) ;
      }
      if ( maxTimeErrorChanged )
      {
         getMetaData()->cutTimeError( _options->getMaxTimeError() ) ;
      }
      if ( syncIntervalChanged )
      {
         getMetaData()->setSyncInterval( _options->getSyncInterval() ) ;
      }

      if ( roleChanged | maxTimeErrorChanged | syncIntervalChanged )
      {
         // need restart synchronize
         _syncManager->activeStatus( TP_SYNC_NOSOURCE ) ;
      }

      changed = roleChanged ;

      PD_TRACE_EXITRC( SDB__TPCATALOGMGR__UPDATELOCALCONFIGS, rc ) ;

      return rc ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__TPCATALOGMGR__UPDATESERVERCONFIGS, "_tpCatalogManager::_updateServerConfigs" )
   INT32 _tpCatalogManager::_updateServerConfigs( BOOLEAN &changed )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__TPCATALOGMGR__UPDATESERVERCONFIGS ) ;

      changed = FALSE ;

      tpOptions tmpOptions ;
      TP_SERVER_LIST servers ;
      vector< pmdAddrPair > serverList ;
      TP_ROLE role = _options->getRole() ;

      if ( TP_ROLE_STANDALONE == _options->getRole() )
      {
         removeServers() ;
         setVersion( TP_GROUP_STANDALONE_VERSION ) ;
         goto done ;
      }

      rc = dumpServers( servers ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to dump servers, rc: %d", rc ) ;

      for ( TP_SERVER_LIST::iterator iter = servers.begin() ;
            iter != servers.end() ;
            ++ iter )
      {
         tmpOptions.addServerAddress( iter->getHostName(),
                                      iter->getServiceName() ) ;
      }
      tmpOptions.formatServerList() ;

      if ( 0 == ossStrcmp( tmpOptions.getServerListString(),
                           _options->getServerListString() ) )
      {
         // nothing changed
         goto done ;
      }

      PD_LOG( PDEVENT, "[%s] changed from [%s] to [%s]",
              PMD_TP_OPTION_SERVERLIST,
              tmpOptions.getServerListString(),
              _options->getServerListString() ) ;

      serverList = _options->getServerList() ;

      PD_CHECK( TP_ROLE_SERVER == role || TP_ROLE_CLIENT == role,
                SDB_INVALIDARG, error, PDWARNING,
                "Failed to to update servers, unknown role: %d",
                role ) ;

      PD_CHECK( !serverList.empty(), SDB_INVALIDARG, error, PDERROR,
                "Failed to update servers, empty server list" ) ;

      rc = _updateServers( serverList ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to update servers, rc: %d", rc ) ;

      if ( isPrimaryServer() )
      {
         increaseVersion() ;
      }

      changed = TRUE ;

   done:
      PD_TRACE_EXITRC( SDB__TPCATALOGMGR__UPDATESERVERCONFIGS, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__TPCATALOGMGR__CHECKLOCALANDSERVERS, "_tpCatalogManager::_checkLocalAndServers" )
   INT32 _tpCatalogManager::_checkLocalAndServers()
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__TPCATALOGMGR__CHECKLOCALANDSERVERS ) ;

      ossScopedRWLock lock( &_mutex, EXCLUSIVE ) ;

      if ( _servers.empty() )
      {
         if ( TP_ROLE_STANDALONE != _local.getRole() )
         {
            PD_LOG( PDWARNING, "Local role is invalid, "
                    "expected [%s], given [%s]",
                    tpGetRoleName( TP_ROLE_STANDALONE ),
                    tpGetRoleName( _local.getRole() ) ) ;
            _local.setRole( TP_ROLE_STANDALONE ) ;
         }
         _setStatus( TP_CATALOG_NORMAL ) ;
      }
      else if ( _servers.end() != find( _servers.begin(),
                                        _servers.end(),
                                        _local.getRouteID() ) )
      {
         if ( TP_ROLE_SERVER != _local.getRole() )
         {
            PD_LOG( PDWARNING, "Local is not %s, need remove server",
                    tpGetRoleName( TP_ROLE_SERVER ) ) ;
            _setStatus( TP_CATALOG_REMOVESERVER ) ;
         }
         else
         {
            _setStatus( TP_CATALOG_NORMAL ) ;
         }
      }
      else
      {
         if ( TP_ROLE_CLIENT != _local.getRole() )
         {
            PD_LOG( PDWARNING, "Local is not %s, need add server",
                    tpGetRoleName( TP_ROLE_CLIENT ) ) ;
            _setStatus( TP_CATALOG_ADDSERVER ) ;
         }
         else
         {
            _setStatus( TP_CATALOG_NORMAL ) ;
         }
      }

      if ( TP_ROLE_STANDALONE == _local.getRole() )
      {
         _primaryRID = _local.getRouteID() ;
         pmdSetPrimary( TRUE ) ;
      }
      else if ( TP_CATALOG_NORMAL != _getStatus() )
      {
         _primaryRID.value = MSG_INVALID_ROUTEID ;
      }

      PD_TRACE_EXITRC( SDB__TPCATALOGMGR__CHECKLOCALANDSERVERS, rc ) ;

      return rc ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__TPCATALOGMGR__SAVESERVERS, "_tpCatalogManager::_saveServers" )
   INT32 _tpCatalogManager::_saveServers()
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__TPCATALOGMGR__SAVESERVERS ) ;

      TP_SERVER_LIST servers ;

      rc = dumpServers( servers ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to dump servers, rc: %d", rc ) ;

      _options->clearServerList() ;

      for ( TP_SERVER_LIST::iterator iter = servers.begin() ;
            iter != servers.end() ;
            ++ iter )
      {
         _options->addServerAddress( iter->getHostName(),
                                     iter->getServiceName() ) ;
      }

      rc = _options->save() ;
      PD_RC_CHECK( rc, PDERROR, "Failed to save catalog options, rc: %d",
                   rc ) ;

      PD_LOG( PDEVENT, "Update server list %s",
              _options->getServerListString() ) ;

   done:
      PD_TRACE_EXITRC( SDB__TPCATALOGMGR__SAVESERVERS, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__TPCATALOGMGR__GETSERVERS, "_tpCatalogManager::_getServers" )
   void _tpCatalogManager::_getServers( UINT32 &version,
                                        TP_SERVER_LIST &servers,
                                        MsgRouteID &primaryRID )
   {
      PD_TRACE_ENTRY( SDB__TPCATALOGMGR__GETSERVERS ) ;

      ossScopedRWLock lock( &_mutex, SHARED ) ;

      version = _version ;
      servers = _servers ;
      primaryRID = _primaryRID ;

      PD_TRACE_EXIT( SDB__TPCATALOGMGR__GETSERVERS ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__TPCATALOGMGR__SETSERVERS, "_tpCatalogManager::_setServers" )
   void _tpCatalogManager::_setServers( UINT32 version,
                                        const TP_SERVER_LIST &servers,
                                        const MsgRouteID &primaryRID,
                                        TP_SERVER_LIST &removedServers )
   {
      PD_TRACE_ENTRY( SDB__TPCATALOGMGR__SETSERVERS ) ;

      ossScopedRWLock lock( &_mutex, SHARED ) ;

      for ( TP_SERVER_LIST::iterator iter = _servers.begin() ;
            iter != _servers.end() ;
            ++ iter )
      {
         tpServerNode &server = ( *iter ) ;
         if ( servers.end() == find( servers.begin(),
                                     servers.end(),
                                     server ) )
         {
            removedServers.push_back( server ) ;
         }
      }

      _version = version ;
      _servers = servers ;
      _primaryRID = primaryRID ;

      PD_LOG( PDEVENT, "Update primary %s, version %u done",
              routeID2String( _primaryRID ).c_str(), _version ) ;

      PD_TRACE_EXIT( SDB__TPCATALOGMGR__SETSERVERS ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__TPCATALOGMGR__BUILDVERSION, "_tpCatalogManager::_buildVersion" )
   INT32 _tpCatalogManager::_buildVersion( BSONObjBuilder &builder,
                                           UINT32 version )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__TPCATALOGMGR__BUILDVERSION ) ;

      try
      {
         builder.append( TP_FIELD_NAME_VERSION, (INT32)version ) ;
      }
      catch ( exception &e )
      {
         rc = SDB_SYS ;
         PD_LOG( PDERROR, "Failed to build version object, "
                 "occurred unexpected error: %s", e.what() ) ;
         goto error ;
      }

   done:
      PD_TRACE_EXITRC( SDB__TPCATALOGMGR__BUILDVERSION, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__TPCATALOGMGR__BUILDSERVERS, "_tpCatalogManager::_buildServers" )
   INT32 _tpCatalogManager::_buildServers( BSONObjBuilder &builder,
                                           const TP_SERVER_LIST &servers )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__TPCATALOGMGR__BUILDSERVERS ) ;

      try
      {
         BSONArrayBuilder groupBuilder(
                              builder.subarrayStart( TP_FIELD_NAME_GROUP ) ) ;
         for ( TP_SERVER_LIST::const_iterator iter = servers.begin() ;
               iter != servers.end() ;
               ++ iter )
         {
            BSONObjBuilder nodeBuilder( groupBuilder.subobjStart() ) ;
            rc = iter->toBSON( nodeBuilder ) ;
            PD_RC_CHECK( rc, PDERROR, "Failed to build BSON for server "
                         "node %s, rc: %d", iter->toString().c_str(),
                         rc ) ;
            nodeBuilder.doneFast() ;
         }
         groupBuilder.doneFast() ;
      }
      catch ( exception &e )
      {
         rc = SDB_SYS ;
         PD_LOG( PDERROR, "Failed to build server group object, "
                 "occurred unexpected error: %s", e.what() ) ;
         goto error ;
      }

   done:
      PD_TRACE_EXITRC( SDB__TPCATALOGMGR__BUILDSERVERS, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__TPCATALOGMGR__BUILDPRIMARYNODE, "_tpCatalogManager::_buildPrimaryNode" )
   INT32 _tpCatalogManager::_buildPrimaryNode( BSONObjBuilder &builder,
                                               const TP_SERVER_LIST &servers,
                                               const MsgRouteID &primaryRID )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__TPCATALOGMGR__BUILDPRIMARYNODE ) ;

      try
      {
         BSONObjBuilder primaryBuilder(
                                 builder.subobjStart( TP_FIELD_NAME_PRIMARY ) ) ;
         primaryBuilder.append( TP_FIELD_NAME_GROUPID,
                                (INT32)( primaryRID.columns.groupID ) ) ;
         primaryBuilder.append( TP_FIELD_NAME_NODEID,
                                (INT32)( primaryRID.columns.nodeID ) ) ;
         primaryBuilder.doneFast() ;
      }
      catch ( exception &e )
      {
         rc = SDB_SYS ;
         PD_LOG( PDERROR, "Failed to build primary node object, "
                 "occurred unexpected error: %s", e.what() ) ;
         goto error ;
      }

   done:
      PD_TRACE_EXITRC( SDB__TPCATALOGMGR__BUILDPRIMARYNODE, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__TPCATALOGMGR__PARSEVERSION, "_tpCatalogManager::_parseVersion" )
   INT32 _tpCatalogManager::_parseVersion( const BSONObj &object,
                                           UINT32 &version )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__TPCATALOGMGR__PARSEVERSION ) ;

      try
      {
         BSONElement versionElement =
                                 object.getField( TP_FIELD_NAME_VERSION ) ;
         PD_CHECK( EOO != versionElement.type(),
                   SDB_INVALIDARG, error, PDERROR,
                   "Failed to parse version object, "
                   "field [%s] is not found",
                   TP_FIELD_NAME_VERSION ) ;
         PD_CHECK( NumberInt == versionElement.type(),
                   SDB_INVALIDARG, error, PDERROR,
                   "Failed to parse version object, "
                   "field [%s] is not an integer",
                   TP_FIELD_NAME_VERSION ) ;
         version = (UINT32)versionElement.numberInt() ;
      }
      catch ( exception &e )
      {
         rc = SDB_SYS ;
         PD_LOG( PDERROR, "Failed to parse version object, "
                 "occurred unexpected error: %s", e.what() ) ;
         goto error ;
      }

   done:
      PD_TRACE_EXITRC( SDB__TPCATALOGMGR__PARSEVERSION, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__TPCATALOGMGR__PARSESERVERS, "_tpCatalogManager::_parseServers" )
   INT32 _tpCatalogManager::_parseServers( const BSONObj &object,
                                           TP_SERVER_LIST &servers )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__TPCATALOGMGR__PARSESERVERS ) ;

      try
      {
         BSONElement groupElement = object.getField( TP_FIELD_NAME_GROUP ) ;
         if ( Array != groupElement.type() )
         {
            PD_CHECK( EOO != groupElement.type(),
                      SDB_INVALIDARG, error, PDERROR,
                      "Failed to parse server group object, "
                      "field [%s] is not found",
                      TP_FIELD_NAME_GROUP ) ;
            PD_CHECK( Array == groupElement.type(),
                      SDB_INVALIDARG, error, PDERROR,
                      "Failed to parse server group object, "
                      "field [%s] is not an array",
                      TP_FIELD_NAME_GROUP ) ;
         }
         else
         {
            BSONObjIterator iter( groupElement.embeddedObject() ) ;
            while ( iter.more() )
            {
               tpServerNode server ;

               BSONElement nodeElement( iter.next() ) ;
               PD_CHECK( Object == nodeElement.type(),
                         SDB_INVALIDARG, error, PDERROR,
                         "Failed to parse server group object, "
                         "node object is not an object" ) ;

               rc = server.fromBSON( nodeElement.embeddedObject() ) ;
               PD_RC_CHECK( rc, PDERROR, "Failed to parse server node %s, "
                            "rc: %d", nodeElement.toPoolString().c_str(), rc ) ;

               servers.push_back( server ) ;
            }
         }
      }
      catch ( exception &e )
      {
         rc = SDB_SYS ;
         PD_LOG( PDERROR, "Failed to parse server group object, "
                 "occurred unexpected error: %s", e.what() ) ;
         goto error ;
      }

      PD_CHECK( !servers.empty(), SDB_INVALIDARG, error, PDERROR,
                "Failed to parse server group object, it is empty" ) ;

   done:
      PD_TRACE_EXITRC( SDB__TPCATALOGMGR__PARSESERVERS, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__TPCATALOGMGR__PARSEPRIMARYNODE, "_tpCatalogManager::_parsePrimaryNode" )
   INT32 _tpCatalogManager::_parsePrimaryNode( const BSONObj &object,
                                               MsgRouteID &primaryRID )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__TPCATALOGMGR__PARSEPRIMARYNODE ) ;

      try
      {
         BSONElement primaryElement =
                                 object.getField( TP_FIELD_NAME_PRIMARY ) ;
         if ( Object != primaryElement.type() )
         {
            PD_CHECK( EOO != primaryElement.type(),
                      SDB_INVALIDARG, error, PDERROR,
                      "Failed to parse primary node object, "
                      "field [%s] is not found",
                      TP_FIELD_NAME_PRIMARY ) ;
            PD_CHECK( Object == primaryElement.type(),
                      SDB_INVALIDARG, error, PDERROR,
                      "Failed to parse primary node object, "
                      "field [%s] is not an object",
                      TP_FIELD_NAME_PRIMARY ) ;
         }
         else
         {
            BSONObj primaryObject = primaryElement.embeddedObject() ;
            BSONElement element ;

            element = primaryObject.getField( TP_FIELD_NAME_GROUPID ) ;
            PD_CHECK( EOO != element.type(),
                      SDB_INVALIDARG, error, PDERROR,
                      "Failed to parse primary node object, "
                      "field [%s] is not found",
                      TP_FIELD_NAME_GROUPID ) ;
            PD_CHECK( NumberInt == element.type(),
                      SDB_INVALIDARG, error, PDERROR,
                      "Failed to parse primary node object, "
                      "field [%s] is not an integer",
                      TP_FIELD_NAME_GROUPID ) ;
            primaryRID.columns.groupID = (UINT32)element.numberInt() ;

            element = primaryObject.getField( TP_FIELD_NAME_NODEID ) ;
            PD_CHECK( EOO != element.type(),
                      SDB_INVALIDARG, error, PDERROR,
                      "Failed to parse primary node object, "
                      "field [%s] is not found",
                      TP_FIELD_NAME_NODEID ) ;
            PD_CHECK( NumberInt == element.type(),
                      SDB_INVALIDARG, error, PDERROR,
                      "Failed to parse primary node object, "
                      "field [%s] is not an integer",
                      TP_FIELD_NAME_NODEID ) ;
            primaryRID.columns.nodeID = (UINT32)element.numberInt() ;

            primaryRID.columns.serviceID = MSG_ROUTE_LOCAL_SERVICE ;
         }
      }
      catch ( exception &e )
      {
         rc = SDB_SYS ;
         PD_LOG( PDERROR, "Failed to parse primary node object, "
                 "occurred unexpected error: %s", e.what() ) ;
         goto error ;
      }

      PD_CHECK( INVALID_GROUPID != primaryRID.columns.groupID,
                SDB_INVALIDARG, error, PDERROR,
                "Failed to parse primary node object, group ID [%u] is invalid",
                primaryRID.columns.groupID ) ;

      PD_CHECK( INVALID_NODEID != primaryRID.columns.nodeID,
                SDB_INVALIDARG, error, PDERROR,
                "Failed to parse primary node object, node ID [%u] is invalid",
                primaryRID.columns.nodeID ) ;

   done:
      PD_TRACE_EXITRC( SDB__TPCATALOGMGR__PARSEPRIMARYNODE, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__TPCATALOGMGR__ADDSERVER_VER, "_tpCatalogManager::_addServer" )
   INT32 _tpCatalogManager::_addServer( const tpServerNode &server,
                                        BOOLEAN increaseVersion )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__TPCATALOGMGR__ADDSERVER_VER ) ;

      BOOLEAN locked = FALSE ;
      TP_SERVER_LIST::iterator iter ;

      rc = _updateRouteID( server.getRouteID(), server.getHostName(),
                           server.getServiceName() ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to update routeID for node %s, rc: %d",
                   server.toString().c_str(), rc ) ;

      PD_CHECK( server.isValidRole(), SDB_INVALIDARG, error, PDERROR,
                "Failed to check server node %s, role is invalid",
                server.toString().c_str() ) ;

      PD_CHECK( server.isValidRoute(), SDB_INVALID_ROUTEID, error, PDERROR,
                "Failed to check server node %s, route ID is invalid",
                server.toString().c_str() ) ;

      _mutex.lock_w() ;
      locked = TRUE ;

      iter = find( _servers.begin(), _servers.end(), server ) ;
      PD_CHECK( iter == _servers.end(),
                SDB_INVALID_ROUTEID, error, PDERROR,
                "Failed to check server node %s, route ID is conflict",
                server.toString().c_str() ) ;

      // force to replace
      _servers.push_back( server ) ;

      if ( increaseVersion )
      {
         ++ _version ;
      }

      _mutex.release_w() ;
      locked = FALSE ;

      PD_LOG( PDEVENT, "Add server node %s done", server.toString().c_str() ) ;

   done:
      if ( locked )
      {
         _mutex.release_w() ;
      }
      PD_TRACE_EXITRC( SDB__TPCATALOGMGR__ADDSERVER_VER, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__TPCATALOGMGR__REMOVESERVER_VER, "_tpCatalogManager::_removeServer" )
   INT32 _tpCatalogManager::_removeServer( const MsgRouteID &routeID,
                                           BOOLEAN increaseVersion )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__TPCATALOGMGR__REMOVESERVER_VER ) ;

      BOOLEAN locked = FALSE ;

      _mutex.lock_w() ;
      locked = TRUE ;

      TP_SERVER_LIST::iterator iter = find( _servers.begin(),
                                            _servers.end(),
                                            routeID ) ;
      PD_CHECK( iter != _servers.end(),
                SDB_INVALID_ROUTEID, error, PDWARNING,
                "Failed to remove server node %s, route ID is not found",
                routeID2String( routeID ).c_str() ) ;

      _servers.erase( iter ) ;

      if ( increaseVersion )
      {
         ++ _version ;
      }

      _mutex.release_w() ;
      locked = FALSE ;

      _deleteRouteID( routeID ) ;

      PD_LOG( PDEVENT, "Remove server node %s done",
              routeID2String( routeID ).c_str() ) ;

   done:
      if ( locked )
      {
         _mutex.release_w() ;
      }
      PD_TRACE_EXITRC( SDB__TPCATALOGMGR__REMOVESERVER_VER, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__TPCATALOGMGR__UPDATESERVERS_LIST, "_tpCatalogManager::_updateServers" )
   INT32 _tpCatalogManager::_updateServers( const vector< pmdAddrPair > &serverList )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__TPCATALOGMGR__UPDATESERVERS_LIST ) ;

      rc = removeServers() ;
      PD_RC_CHECK( rc, PDERROR, "Failed to remove servers, rc: %d", rc ) ;

      for ( vector< pmdAddrPair >::const_iterator iter = serverList.begin() ;
            iter != serverList.end() ;
            ++ iter )
      {
         tpServerNode server ;
         const CHAR *hostName = iter->_host ;
         const CHAR *serviceName = iter->_service ;
         MsgRouteID routeID ;

         routeID.value = MSG_INVALID_ROUTEID ;
         rc = _getRouteID( hostName, serviceName, routeID ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to get route ID %s:%s, rc: %d",
                      hostName, serviceName, rc ) ;

         server.setRouteID( routeID ) ;
         server.setHostName( hostName ) ;
         server.setServiceName( serviceName ) ;

         rc = _addServer( server, FALSE ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to register server node %s, "
                      "rc: %d", server.toString().c_str(), rc ) ;

         PD_LOG( PDEVENT, "Initialize server node %s done",
                 server.toString().c_str() ) ;
      }

   done:
      PD_TRACE_EXITRC( SDB__TPCATALOGMGR__UPDATESERVERS_LIST, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__TPCATALOGMGR__GETROUTEID_HOST, "_tpCatalogManager::_getRouteID" )
   INT32 _tpCatalogManager::_getRouteID( const CHAR *hostName,
                                         const CHAR *serviceName,
                                         MsgRouteID &routeID )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__TPCATALOGMGR__GETROUTEID_HOST ) ;

      netUDPEndPoint udpEP ;
      rc = netRoute::getUDPEndPoint( hostName, serviceName, udpEP ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to resolve local UDP end point %s:%s, "
                   "rc: %d", hostName, serviceName, rc ) ;

      rc = _getRouteID( udpEP, routeID ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to get local route ID %s:%s, "
                   "rc: %d", rc ) ;

   done:
      PD_TRACE_EXITRC( SDB__TPCATALOGMGR__GETROUTEID_HOST, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__TPCATALOGMGR__GETROUTEID_UDP, "_tpCatalogManager::_getRouteID" )
   INT32 _tpCatalogManager::_getRouteID( const netUDPEndPoint &udpEP,
                                         MsgRouteID &routeID )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__TPCATALOGMGR__GETROUTEID_UDP ) ;

      routeID.columns.groupID = udpEP.address().to_v4().to_ulong() ;
      routeID.columns.nodeID = udpEP.port() ;
      routeID.columns.serviceID = MSG_ROUTE_LOCAL_SERVICE ;

      PD_TRACE_EXITRC( SDB__TPCATALOGMGR__GETROUTEID_UDP, rc ) ;

      return rc ;
   }

}
