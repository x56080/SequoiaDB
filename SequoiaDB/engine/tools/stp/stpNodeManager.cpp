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

   Source File Name = stpNodeManager.cpp

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
#include "stpNodeManager.hpp"
#include "stpCB.hpp"
#include "pdTrace.hpp"
#include "stpTrace.hpp"
#include "pmd.hpp"
#include "msgMessage.hpp"
#include "pmdEnv.hpp"

using namespace std ;
using namespace bson ;

namespace engine
{

   /*
      _stpNodeManager implement
    */
   BEGIN_OBJ_MSG_MAP( _stpNodeManager, _stpManagerBase )
      // ON_MSG
      ON_MSG( MSG_STP_SERVER_REQ, processMessage )
      ON_MSG( MSG_STP_SERVER_RSP, processMessage )
   END_OBJ_MSG_MAP()

   _stpNodeManager::_stpNodeManager( STPCB *stpCB )
   : stpManagerBase( stpCB ),
     _status( STP_NODE_NORMAL ),
     _version( STP_GROUP_INVALID_VERSION )
   {
      _primaryRID.value = MSG_INVALID_ROUTEID ;
   }

   _stpNodeManager::~_stpNodeManager()
   {
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__STPNODEMGR_ONTIMER, "_stpNodeManager::onTimer" )
   void _stpNodeManager::onTimer( UINT64 timerID, UINT32 interval )
   {
      PD_TRACE_ENTRY( SDB__STPNODEMGR_ONTIMER ) ;

      if ( timerID == _timerID )
      {
         // launch server check ( for each interval, 1 second )
         launchServerCheck() ;
      }

      PD_TRACE_EXIT( SDB__STPNODEMGR_ONTIMER ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__STPNODEMGR_PROCESSMESSAGE, "_stpNodeManager::processMessage" )
   INT32 _stpNodeManager::processMessage( NET_HANDLE handle,
                                          MsgHeader *message )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__STPNODEMGR_PROCESSMESSAGE ) ;

      SDB_ASSERT( NULL != message, "message is invalid" ) ;

      switch ( message->opCode )
      {
         case MSG_STP_SERVER_REQ :
         {
            // handle server request
            rc = _handleServerReq( handle,
                                   (const stpServerReq *)message ) ;
            PD_RC_CHECK( rc, PDERROR, "Failed to handle server request, "
                         "rc: %d", rc ) ;
            break ;
         }
         case MSG_STP_SERVER_RSP :
         {
            // handle server response
            rc = _handleServerRsp( handle,
                                   (const stpServerRsp *)message ) ;
            PD_RC_CHECK( rc, PDERROR, "Failed to handle server result, "
                         "rc: %d", rc ) ;
            break ;
         }
         default :
         {
            // unknown message
            PD_CHECK( FALSE, SDB_UNKNOWN_MESSAGE, error, PDERROR,
                      "Unknown catalog message [%d]",
                      message->opCode ) ;
            break ;
         }
      }

   done:
      PD_TRACE_EXITRC( SDB__STPNODEMGR_PROCESSMESSAGE, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__STPNODEMGR__INITIALIZE, "_stpNodeManager::_initialize" )
   INT32 _stpNodeManager::_initialize()
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__STPNODEMGR__INITIALIZE ) ;

      // initialize servers
      rc = _initServers() ;
      PD_RC_CHECK( rc, PDERROR, "Failed to initialize server group, rc: %d",
                   rc ) ;

      // initialize local node
      rc = _initLocal() ;
      PD_RC_CHECK( rc, PDERROR, "Failed to initialize local node, rc: %d",
                   rc ) ;

      // check conflicts between local and servers
      rc = _checkLocalAndServers() ;
      PD_RC_CHECK( rc, PDERROR, "Failed to check local and servers, "
                   "rc: %d", rc ) ;

   done:
      PD_TRACE_EXITRC( SDB__STPNODEMGR__INITIALIZE, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__STPNODEMGR__POSTACTIVATE, "_stpNodeManager::_postActivate" )
   INT32 _stpNodeManager::_postActivate()
   {
      PD_TRACE_ENTRY( SDB__STPNODEMGR__POSTACTIVATE ) ;

      // launch server check after activated
      // NOTE: ignore error here, if error happened, it will launch again
      //       in timer
      launchServerCheck() ;

      PD_TRACE_EXITRC( SDB__STPNODEMGR__POSTACTIVATE, SDB_OK ) ;

      return SDB_OK ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__STPNODEMGR__AFTERCHANGEPRIMARY, "_stpNodeManager::_afterChangePrimary" )
   INT32 _stpNodeManager::_afterChangePrimary( const MsgRouteID &primaryRID,
                                               BOOLEAN isLocalPrimary )
   {
      PD_TRACE_ENTRY( SDB__STPNODEMGR__AFTERCHANGEPRIMARY ) ;

      // set primary if changed
      setPrimaryRID( primaryRID ) ;

      PD_TRACE_EXITRC( SDB__STPNODEMGR__AFTERCHANGEPRIMARY, SDB_OK ) ;

      return SDB_OK ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__STPNODEMGR__HANDLESERVERREQ, "_stpNodeManager::_handleServerReq" )
   INT32 _stpNodeManager::_handleServerReq( NET_HANDLE handle,
                                            const stpServerReq *request )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__STPNODEMGR__HANDLESERVERREQ ) ;

      SDB_ASSERT( NULL != request, "request message is invalid" ) ;
      SDB_ASSERT( MSG_STP_SERVER_REQ == request->header.opCode,
                  "opcode of message is invalid" ) ;

      // extract fields from request
      STP_SERVER_REQ_TYPE reqType = (STP_SERVER_REQ_TYPE)( request->type ) ;
      MsgRouteID routeID ;
      BSONObj groupObject ;

      // set route ID
      routeID.value = request->header.routeID.value ;

      PD_LOG( PDEVENT, "Got server quest [%s] from %s",
              stpGetServerReqName( reqType ),
              routeID2String( routeID ).c_str() ) ;

      switch ( reqType )
      {
         case STP_SERVER_REQ_ADDSERVER :
         {
            // handle add server request
            rc = _handleAddServer( handle, routeID ) ;
            PD_RC_CHECK( rc, PDERROR, "Failed to handle add server "
                         "request from  %s, rc: %d",
                         routeID2String( routeID ).c_str(), rc ) ;
            break ;
         }
         case STP_SERVER_REQ_REMOVESERVER :
         {
            // handle remove server request
            rc = _handleRemoveServer( handle, routeID ) ;
            PD_RC_CHECK( rc, PDERROR, "Failed to handle remove server "
                         "request from  %s, rc: %d",
                         routeID2String( routeID ).c_str(), rc ) ;
            break ;
         }
         default :
         {
            // handle query server request
            // should know primary to handle query server request
            PD_CHECK( hasPrimary(), SDB_RTN_NO_PRIMARY_FOUND, error, PDERROR,
                      "Failed to handle query server request, "
                      "current node does not know who is primary") ;
            break ;
         }
      }

      // get servers
      // NOTE: for add or remove server request, also return information with
      //       response
      rc = getServers( groupObject, FALSE ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to get server group, rc: %d", rc ) ;

      // send server response
      rc = _sendServerRsp( handle, request, groupObject, SDB_OK ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "Failed to send server result, rc: %d", rc ) ;
      }

   done:
      PD_TRACE_EXITRC( SDB__STPNODEMGR__HANDLESERVERREQ, rc ) ;
      return rc ;

   error:
      // send error response
      _sendServerRsp( handle, request, groupObject, rc ) ;
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__STPNODEMGR__HANDLESERVERRSP, "_stpNodeManager::_handleServerRsp" )
   INT32 _stpNodeManager::_handleServerRsp( NET_HANDLE handle,
                                            const stpServerRsp *response )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__STPNODEMGR__HANDLESERVERRSP ) ;

      SDB_ASSERT( NULL != response, "response message is invalid" ) ;
      SDB_ASSERT( MSG_STP_SERVER_RSP == response->reply.header.opCode,
                  "opcode of message is invalid" ) ;

      // get return code of response
      rc = response->reply.res ;

      // check return code
      if ( SDB_OK == rc )
      {
         try
         {
            BSONObj object ;

            // check length of message
            PD_CHECK( response->reply.header.messageLength >=
                      (INT32)( sizeof( stpServerRsp ) + object.objsize() ),
                      SDB_SYS, error, PDERROR, "Failed to handle server "
                      "response, size of message is unexpected, "
                      "expected >= [%u], given [%u]",
                      sizeof( stpServerRsp ) + object.objsize(),
                      response->reply.header.messageLength ) ;

            // extract result in BSON format
            object = BSONObj( (CHAR *)( response ) + sizeof( stpServerRsp ) ) ;

            // update servers
            rc = setServers( object ) ;
            PD_RC_CHECK( rc, PDERROR, "Failed to set server group, "
                         "rc: %d", rc ) ;
         }
         catch ( exception &e )
         {
            rc = ossException2RC( &e ) ;
            PD_LOG( PDERROR, "Failed to parse server group object, "
                    "occurred unexpected error: %s", e.what() ) ;
            goto error ;
         }
      }
      else
      {
         if ( SDB_CLS_NOT_PRIMARY == rc )
         {
            // with error return codes, reset primary
            // will launch server check from another server later
            resetPrimaryOnError( response->reply.header.routeID ) ;
         }
         PD_RC_CHECK( rc, PDERROR, "Failed to query server, "
                      "received result with error: %d", rc ) ;
      }

   done:
      PD_TRACE_EXITRC( SDB__STPNODEMGR__HANDLESERVERRSP, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__STPNODEMGR__HANDLEADDSERVER, "_stpNodeManager::_handleAddServer" )
   INT32 _stpNodeManager::_handleAddServer( NET_HANDLE handle,
                                            const MsgRouteID &routeID )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__STPNODEMGR__HANDLEADDSERVER ) ;

      stpServerNode server ;
      NET_EH eh ;
      CHAR serviceName[ OSS_MAX_SERVICENAME + 1 ] = { 0 } ;

      // only primary server could handle add server request
      PD_CHECK( isPrimaryServer(), SDB_CLS_NOT_PRIMARY, error, PDERROR,
                "Failed to handle add server request, "
                "current node is not primary" ) ;

      eh = _netAgent->getFrame()->getEventHandle( handle ) ;
      PD_CHECK( NULL != eh.get(), SDB_NET_INVALID_HANDLE, error, PDERROR,
                "Failed to get event handler for handle [%u]",
                handle ) ;

      ossSnprintf( serviceName, OSS_MAX_SERVICENAME, "%u",
                   routeID.columns.nodeID ) ;

      // set fields of new server
      server.setRouteID( routeID ) ;
      server.setHostName( eh->remoteAddr().c_str() ) ;
      server.setServiceName( serviceName ) ;

      // add server and increase version
      rc = _addServer( server, TRUE ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to add server %s, rc: %d",
                   server.toString().c_str(), rc ) ;

      // save server list to configs
      rc = _saveServers() ;
      PD_RC_CHECK( rc, PDERROR, "Failed to save server list to config file, "
                   "rc: %d", rc ) ;

      // check conflicts between local and servers
      rc = _checkLocalAndServers() ;
      PD_RC_CHECK( rc, PDERROR, "Failed to check local and servers, "
                   "rc: %d", rc ) ;

      // if status is normal, notify other modules to update servers
      if ( STP_NODE_NORMAL == getStatus() )
      {
         _stpCB->onChangeServers() ;
      }

   done:
      PD_TRACE_EXITRC( SDB__STPNODEMGR__HANDLEADDSERVER, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__STPNODEMGR__HANDLEREMOVESERVER, "_stpNodeManager::_handleRemoveServer" )
   INT32 _stpNodeManager::_handleRemoveServer( NET_HANDLE handle,
                                               const MsgRouteID &routeID )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__STPNODEMGR__HANDLEREMOVESERVER ) ;

      // only handle remove server request in primary server
      PD_CHECK( isPrimaryServer(), SDB_CLS_NOT_PRIMARY, error, PDERROR,
                "Failed to handle remove server request, "
                "current node is not primary" ) ;

      // remove server and increase version
      rc = _removeServer( routeID, TRUE ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to remove server %s, rc: %d",
                   routeID2String( routeID ).c_str(), rc ) ;

      // save servers into configs
      rc = _saveServers() ;
      PD_RC_CHECK( rc, PDERROR, "Failed to save server list to config file, "
                   "rc: %d", rc ) ;

      // check conflicts between local and servers
      rc = _checkLocalAndServers() ;
      PD_RC_CHECK( rc, PDERROR, "Failed to check local and servers, "
                   "rc: %d", rc ) ;

      // if status is normal, notify other modules to update servers
      if ( STP_NODE_NORMAL == getStatus() )
      {
         _stpCB->onChangeServers() ;
      }

   done:
      PD_TRACE_EXITRC( SDB__STPNODEMGR__HANDLEREMOVESERVER, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__STPNODEMGR__SENDSERVERREQ, "_stpNodeManager::_sendServerReq" )
   INT32 _stpNodeManager::_sendServerReq( const MsgRouteID &routeID,
                                          STP_SERVER_REQ_TYPE type )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__STPNODEMGR__SENDSERVERREQ ) ;

      stpServerReq request ;

      // fill request header
      _netMsgHandler->fillRequestHeader( request.header,
                                         sizeof( stpServerReq ),
                                         MSG_STP_SERVER_REQ ) ;

      // fill type of server request
      request.type = (UINT16)type ;

      // send server request
      rc = _netAgent->syncSend( routeID, (MsgHeader *)&request ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to send server request to %s, "
                   "rc: %d", routeID2String( routeID ).c_str(), rc ) ;

   done:
      PD_TRACE_EXITRC( SDB__STPNODEMGR__SENDSERVERREQ, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__STPNODEMGR__SENDSERVERRSP, "_stpNodeManager::_sendServerRsp" )
   INT32 _stpNodeManager::_sendServerRsp( NET_HANDLE handle,
                                          const stpServerReq *request,
                                          const BSONObj &object,
                                          INT32 returnCode )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__STPNODEMGR__SENDSERVERRSP ) ;

      SDB_ASSERT( NULL != request, "request is invalid" ) ;

      stpServerRsp response ;
      UINT32 replySize = sizeof( stpServerRsp ) + object.objsize() ;

      // fill reply header
      _netMsgHandler->fillReplyHeader( request->header,
                                       response.reply,
                                       replySize,
                                       returnCode ) ;

      // send server response with result
      rc = _netAgent->syncSend( handle,
                                (MsgHeader *)( &response ),
                                (void *)( object.objdata() ),
                                object.objsize() ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to send server response, rc: %d",
                   rc ) ;

   done:
      PD_TRACE_EXITRC( SDB__STPNODEMGR__SENDSERVERRSP, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__STPNODEMGR_SETLOCAL, "_stpNodeManager::setLocal" )
   INT32 _stpNodeManager::setLocal( const stpClientNode &local )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__STPNODEMGR_SETLOCAL ) ;

      ossScopedRWLock lock( &_mutex, EXCLUSIVE ) ;

      // check route ID
      if ( _local.isValidRoute() )
      {
         // route ID should be changed
         PD_CHECK( _local.getRouteIDValue() == local.getRouteIDValue(),
                   SDB_INVALIDARG, error, PDERROR,
                   "Failed to set local, route ID is different, "
                   "expected %s, given %s",
                   routeID2String( _local.getRouteID() ).c_str(),
                   routeID2String( local.getRouteID() ).c_str() ) ;
      }
      else
      {
         // check if route ID is valid
         PD_CHECK( local.isValidRoute(), SDB_INVALIDARG, error, PDERROR,
                   "Failed to set local, route ID is invalid" ) ;
      }

      // check OID
      if ( _local.isValidOID() )
      {
         // OID should not be changed
         PD_CHECK( _local.getOID() == local.getOID(),
                   SDB_INVALIDARG, error, PDWARNING,
                   "Failed to set local, OID is different, ignored "
                   "expected [%s], given [%s]",
                   _local.getOID().toString().c_str(),
                   local.getOID().toString().c_str() ) ;
      }

      // set local
      _local = local ;

      // set node role ( NOTE: db node, not STP role )
      pmdSetDBRole( SDB_ROLE_STP ) ;

   done:
      PD_TRACE_EXITRC( SDB__STPNODEMGR_SETLOCAL, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   void _stpNodeManager::updateLocalTimeError( UINT32 timeError )
   {
      ossScopedRWLock lock( &_mutex, EXCLUSIVE ) ;
      if ( timeError > _local.getMaxTimeError() )
      {
         PD_LOG( PDWARNING, "Time error [%u] is larger than maximum time "
                 "error [%u], round with maximum value", timeError,
                 _local.getMaxTimeError() ) ;
         timeError = _local.getMaxTimeError() ;
      }
      else if ( timeError < STP_MIN_TIME_ERROR )
      {
         PD_LOG( PDWARNING, "Time error [%u] is smaller than minimum time "
                 "error [%u], round with minimum value", timeError,
                 STP_MIN_TIME_ERROR ) ;
         timeError = STP_MIN_TIME_ERROR ;
      }

      _local.setTimeError( timeError ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__STPNODEMGR_CHKEXPIREDVER, "_stpNodeManager::checkExpiredVersion" )
   void _stpNodeManager::checkExpiredVersion( UINT32 expiredVersion )
   {
      PD_TRACE_ENTRY( SDB__STPNODEMGR_CHKEXPIREDVER ) ;

      ossScopedRWLock lock( &_mutex, EXCLUSIVE ) ;

      if ( _version <= expiredVersion )
      {
         // given version is mark expired, if version is no larger than given
         // version, means the version is expired, set status to query servers
         PD_LOG( PDWARNING, "Server is reported expired, current is [%u], "
                 "given is [%u]", _version, expiredVersion ) ;
         _setStatus( STP_NODE_QUERYSERVERS ) ;
      }

      PD_TRACE_EXIT( SDB__STPNODEMGR_CHKEXPIREDVER ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__STPNODEMGR_LAUNCHSERVERCHECK, "_stpNodeManager::launchServerCheck" )
   INT32 _stpNodeManager::launchServerCheck()
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__STPNODEMGR_LAUNCHSERVERCHECK ) ;

      MsgRouteID serverRID ;
      serverRID.value = MSG_INVALID_ROUTEID ;

      // check if we need to check server
      if ( needServerCheck() )
      {
         STP_SERVER_REQ_TYPE requestType = STP_SERVER_REQ_QUERYSERVERS ;

         if ( needAddServer() )
         {
            // need add this node into servers
            requestType = STP_SERVER_REQ_ADDSERVER ;
         }
         else if ( needRemoveServer() )
         {
            // need remove this node from servers
            requestType = STP_SERVER_REQ_REMOVESERVER ;
         }

         // get route ID of primary to send request
         rc = _session.getPrimaryRID( serverRID ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to get server RID, rc: %d", rc ) ;

         // send server request
         rc = _sendServerReq( serverRID, requestType ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to send [%s] server request, "
                      "rc: %d", stpGetServerReqName( requestType ), rc ) ;
      }

   done:
      PD_TRACE_EXITRC( SDB__STPNODEMGR_LAUNCHSERVERCHECK, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__STPNODEMGR_CHOOSESERVERRID, "_stpNodeManager::chooseServerRID" )
   INT32 _stpNodeManager::chooseServerRID( const MsgRouteID &curRouteID,
                                           BOOLEAN preferPrimary,
                                           MsgRouteID &routeID )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__STPNODEMGR_CHOOSESERVERRID ) ;

      ossScopedRWLock lock( &_mutex, SHARED ) ;

      // check if server is empty
      PD_CHECK( _servers.size() > 0, SDB_INVALID_ROUTEID, error, PDERROR,
                "Failed to get server RID, server group is empty" ) ;

      if ( preferPrimary && MSG_INVALID_ROUTEID != _primaryRID.value )
      {
         // return primary if known
         routeID.value = _primaryRID.value ;
      }
      else
      {
         // loop each server if primary is unknown
         STP_SERVER_LIST::iterator iter = find( _servers.begin(),
                                                _servers.end(),
                                                curRouteID ) ;
         // move to next server of given route ID
         // NOTE: the given route ID is used earlier by caller, so now we need
         //       to move to the next
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
      PD_TRACE_EXITRC( SDB__STPNODEMGR_CHOOSESERVERRID, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__STPNODEMGR_GETSERVER, "_stpNodeManager::getServer" )
   INT32 _stpNodeManager::getServer( const MsgRouteID &routeID,
                                     stpServerNode &server )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__STPNODEMGR_GETSERVER ) ;

      ossScopedRWLock lock( ( &_mutex ), SHARED ) ;

      // find server by route ID
      STP_SERVER_LIST::const_iterator iter = find( _servers.begin(),
                                                  _servers.end(),
                                                  routeID ) ;
      PD_CHECK( iter != _servers.end(), SDB_INVALID_ROUTEID, error, PDERROR,
                "Failed to get server node %s, it is not found",
                routeID2String( routeID ).c_str() ) ;

      // copy server to output
      server = ( *iter ) ;

   done:
      PD_TRACE_EXITRC( SDB__STPNODEMGR_GETSERVER, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__STPNODEMGR_GETSERVER_HOST, "_stpNodeManager::getServer" )
   INT32 _stpNodeManager::getServer( const CHAR *hostName,
                                     stpServerNode &server )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__STPNODEMGR_GETSERVER_HOST ) ;

      SDB_ASSERT( NULL != hostName, "host name is invalid" ) ;

      ossScopedRWLock lock( ( &_mutex ), SHARED ) ;

      // find server by host name
      STP_SERVER_LIST::const_iterator iter = _servers.begin() ;
      while ( iter != _servers.end() )
      {
         if ( 0 == ossStrcmp( iter->getHostName(), hostName ) )
         {
            break ;
         }
         ++ iter ;
      }
      PD_CHECK( iter != _servers.end(), SDB_INVALID_ROUTEID, error, PDERROR,
                "Failed to get server node by host name [%s], it is not found",
                hostName ) ;

      // copy server to output
      server = ( *iter ) ;

   done:
      PD_TRACE_EXITRC( SDB__STPNODEMGR_GETSERVER_HOST, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__STPNODEMGR_DUMPSERVERS, "_stpNodeManager::dumpServers" )
   INT32 _stpNodeManager::dumpServers( STP_SERVER_LIST &servers )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__STPNODEMGR_DUMPSERVERS ) ;

      ossScopedRWLock lock( ( &_mutex ), SHARED ) ;

      try
      {
         // copy servers to output
         servers = _servers ;
      }
      catch ( exception &e )
      {
         PD_LOG( PDERROR, "Failed to dump sources, "
                 "occurred unexpected error: %s", e.what() ) ;
         rc = ossException2RC( &e ) ;
         goto error ;
      }

   done:
      PD_TRACE_EXITRC( SDB__STPNODEMGR_DUMPSERVERS, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__STPNODEMGR_REMOVESERVERS, "_stpNodeManager::removeServers" )
   INT32 _stpNodeManager::removeServers()
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__STPNODEMGR_REMOVESERVERS ) ;

      STP_SERVER_LIST servers ;

      // copy servers
      rc = dumpServers( servers ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to dump servers, rc: %d", rc ) ;

      _mutex.lock_w() ;

      // remove all servers
      _servers.clear() ;

      _mutex.release_w() ;

      // for removed servers, delete route ID from net agent
      for ( STP_SERVER_LIST::iterator iter = servers.begin() ;
            iter != servers.end() ;
            ++ iter )
      {
         _netManager->deleteRouteID( iter->getRouteID() ) ;
      }

      PD_LOG( PDEVENT, "Remove all server nodes done" ) ;

   done:
      PD_TRACE_EXITRC( SDB__STPNODEMGR_REMOVESERVERS, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__STPNODEMGR_GETSERVERS, "_stpNodeManager::getServers" )
   INT32 _stpNodeManager::getServers( UINT32 &version,
                                      STP_SERVER_LIST &servers )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__STPNODEMGR_GETSERVERS ) ;

      MsgRouteID primaryRID ;

      // get servers, including version and primary ( primary is not needed )
      _getServers( version, servers, primaryRID ) ;

      PD_TRACE_EXITRC( SDB__STPNODEMGR_GETSERVERS, rc ) ;

      return rc ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__STPNODEMGR_GETSERVERS_BSON, "_stpNodeManager::getServers" )
   INT32 _stpNodeManager::getServers( BSONObj &object, BOOLEAN forDisplay )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__STPNODEMGR_GETSERVERS_BSON ) ;

      UINT32 version = STP_GROUP_INVALID_VERSION ;
      STP_SERVER_LIST servers ;
      MsgRouteID primaryRID ;

      // get servers, including version, primary
      _getServers( version, servers, primaryRID ) ;

      try
      {
         BSONObjBuilder builder ;

         // build version into BSON object
         rc = _buildVersion( builder, version ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to build version object, "
                      "rc: %d", rc ) ;

         // build servers into BSON object
         rc = _buildServers( builder, servers, forDisplay ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to build server group object, "
                      "rc: %d", rc ) ;

         // build primary into BSON object
         rc = _buildPrimaryNode( builder, servers, primaryRID, forDisplay ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to build primary node object, "
                      "rc: %d", rc ) ;

         object = builder.obj() ;
      }
      catch ( exception &e )
      {
         rc = ossException2RC( &e ) ;
         PD_LOG( PDERROR, "Failed to build server group object, "
                 "occurred unexpected error: %s", e.what() ) ;
         goto error ;
      }

   done:
      PD_TRACE_EXITRC( SDB__STPNODEMGR_GETSERVERS_BSON, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__STPNODEMGR_SETSERVERS_BSON, "_stpNodeManager::setServers" )
   INT32 _stpNodeManager::setServers( const BSONObj &object )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__STPNODEMGR_SETSERVERS_BSON ) ;

      UINT32 version = STP_GROUP_INVALID_VERSION ;
      STP_SERVER_LIST servers, removedServers ;
      MsgRouteID primaryRID ;

      try
      {
         // parse version from BSON object
         rc = _parseVersion( object, version ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to parse version object, "
                      "rc: %d", rc ) ;

         // parse servers from BSON object
         rc = _parseServers( object, servers ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to parse server object, "
                      "rc: %d", rc ) ;

         // parse primary from BSON object
         rc = _parsePrimaryNode( object, primaryRID ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to parse primary node object, "
                      "rc: %d", rc ) ;
      }
      catch ( exception &e )
      {
         rc = ossException2RC( &e ) ;
         PD_LOG( PDERROR, "Failed to parse server group object, "
                 "occurred unexpected error: %s", e.what() ) ;
         goto error ;
      }

      // for each server, update route ID from net agent
      for ( STP_SERVER_LIST::iterator iter = servers.begin() ;
            iter != servers.end() ;
            ++ iter )
      {
         stpServerNode &server = ( *iter ) ;
         rc = _netManager->updateRouteID( server.getRouteID(),
                                          server.getHostName(),
                                          server.getServiceName() ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to update route %s:%s for "
                      "server node %s, rc: %d", server.getHostName(),
                      server.getServiceName(), server.toString().c_str(),
                      rc ) ;
      }

      // update servers, including version, servers and primary
      _setServers( version, servers, primaryRID, removedServers ) ;

      // for removed servers, remove route ID from net agent
      for ( STP_SERVER_LIST::iterator iter = removedServers.begin() ;
            iter != removedServers.end() ;
            ++ iter )
      {
         _netManager->deleteRouteID( iter->getRouteID() ) ;
      }

      // save servers to configs
      rc = _saveServers() ;
      PD_RC_CHECK( rc, PDERROR, "Failed to save options, rc: %d", rc ) ;

      // check conflicts between local and servers
      rc = _checkLocalAndServers() ;
      PD_RC_CHECK( rc, PDERROR, "Failed to check local and servers, "
                   "rc: %d", rc ) ;

      if ( STP_NODE_NORMAL == getStatus() )
      {
         // if status is normal after changed, notify other modules to
         // change servers
         _stpCB->onChangeServers() ;
      }

   done:
      PD_TRACE_EXITRC( SDB__STPNODEMGR_SETSERVERS_BSON, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__STPNODEMGR_UPDATECONFIGS, "_stpNodeManager::updateConfigs" )
   INT32 _stpNodeManager::updateConfigs()
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__STPNODEMGR_UPDATECONFIGS ) ;

      BOOLEAN localRoleUpdated = FALSE, serversUpdated = FALSE ;

      // update local configs
      rc = _updateLocalConfigs( localRoleUpdated ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to update local, rc: %d", rc ) ;

      // update servers configs
      rc = _updateServerConfigs( serversUpdated ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to udpate servers, rc: %d", rc ) ;

      if ( localRoleUpdated || serversUpdated )
      {
         // reset status
         setStatus( STP_NODE_NORMAL ) ;

         // check conflicts between local and servers
         rc = _checkLocalAndServers() ;
         PD_RC_CHECK( rc, PDERROR, "Failed to check local and servers, "
                      "rc: %d", rc ) ;

         if ( STP_NODE_NORMAL == getStatus() )
         {
            // if status is normal after changes, update versions if needed
            if ( localRoleUpdated && serversUpdated )
            {
               // set initial version for server or client role
               setVersion( STP_GROUP_INIT_VERSION ) ;
            }
            // notify other modules on servers changed
            _stpCB->onChangeServers() ;
         }
      }

   done:
      PD_TRACE_EXITRC( SDB__STPNODEMGR_UPDATECONFIGS, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__STPNODEMGR__INITLOCAL, "_stpNodeManager::_initLocal" )
   INT32 _stpNodeManager::_initLocal()
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__STPNODEMGR__INITLOCAL ) ;

      stpClientNode local ;

      // get host name
      const CHAR *hostName = pmdGetKRCB()->getHostName() ;
      // get service name
      const CHAR *serviceName = _options->getServiceName() ;

      MsgRouteID routeID ;
      stpServerNode server ;

      // get route ID by host and service
      routeID.value = MSG_INVALID_ROUTEID ;
      rc = _netManager->getRouteID( hostName, serviceName, routeID ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to get route ID %s:%s, "
                   "rc: %d", hostName, serviceName, rc ) ;

      // set fields of local node
      local.setHostName( hostName ) ;
      local.setServiceName( serviceName ) ;
      local.setRouteID( routeID ) ;
      local.setRole( _options->getRole() ) ;
      local.setSyncInterval( _options->getSyncInterval() ) ;
      local.setMaxTimeError( _options->getMaxTimeErrorNS() ) ;

      // set local
      setLocal( local ) ;

      PD_LOG( PDEVENT, "Initialize local node %s done",
              local.toString().c_str() ) ;

   done:
      PD_TRACE_EXITRC( SDB__STPNODEMGR__INITLOCAL, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__TPCATALOGGROUP__INITSERVERS, "_stpNodeManager::_initServers" )
   INT32 _stpNodeManager::_initServers()
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__TPCATALOGGROUP__INITSERVERS ) ;

      // get role and server list from options
      STP_ROLE role = _options->getRole() ;
      vector< pmdAddrPair > serverList = _options->getServerList() ;

      if ( _options->isTestMode() )
      {
         // set group version to initial version
         setVersion( STP_GROUP_INIT_VERSION ) ;
         goto done ;
      }

      // check role
      PD_CHECK( STP_ROLE_SERVER == role || STP_ROLE_CLIENT == role,
                SDB_INVALIDARG, error, PDWARNING,
                "Failed to to initialize servers, unknown role: %d",
                role ) ;

      if ( STP_ROLE_SERVER == role && serverList.empty() )
      {
         // if server list is empty for server role
         // add self into server list

         // get host name
         const CHAR *hostName = pmdGetKRCB()->getHostName() ;
         // get service name
         const CHAR *serviceName = _options->getServiceName() ;
         // set address pair
         pmdAddrPair selfAddr( hostName, serviceName ) ;

         // add to server list
         try
         {
            serverList.push_back( selfAddr ) ;
         }
         catch ( exception &e )
         {
            PD_LOG( PDERROR, "Failed to add self to server list, error: %s",
                    e.what() ) ;
            rc = ossException2RC( &e ) ;
            goto error ;
         }

         PD_LOG( PDEVENT, "No server list is given for server role, "
                 "add self [%s:%s] to server list", hostName, serviceName ) ;
      }

      // check if server list is empty
      PD_CHECK( !serverList.empty(), SDB_INVALIDARG, error, PDERROR,
                "Failed to initialize servers, empty server list" ) ;

      // update servers by server list
      rc = _updateServers( serverList ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to update servers, rc: %d", rc ) ;

      // set group version to initial version
      setVersion( STP_GROUP_INIT_VERSION ) ;

   done:
      PD_TRACE_EXITRC( SDB__TPCATALOGGROUP__INITSERVERS, rc ) ;
      return rc ;

   error:
      goto done ;
   }


   // PD_TRACE_DECLARE_FUNCTION ( SDB__STPNODEMGR__UPDATELOCALCONFIGS, "_stpNodeManager::_updateLocalConfigs" )
   INT32 _stpNodeManager::_updateLocalConfigs( BOOLEAN &roleChanged )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__STPNODEMGR__UPDATELOCALCONFIGS ) ;


      roleChanged = FALSE ;
      BOOLEAN syncIntervalChanged = FALSE ;
      BOOLEAN maxTimeErrorChanged = FALSE ;

      _mutex.lock_w() ;

      // check if role is changed
      if ( _local.getRole() != _options->getRole() )
      {
         PD_LOG( PDEVENT, "[%s] changed from [%s] to [%s]",
                 STP_OPTION_ROLE, stpGetRoleName( _local.getRole() ),
                 stpGetRoleName( _options->getRole() ) ) ;

         // set role
         _local.setRole( _options->getRole() ) ;

         // mark role changed
         roleChanged = TRUE ;
      }

      // check if max time error is changed
      if ( _local.getMaxTimeError() != _options->getMaxTimeErrorNS() )
      {
         PD_LOG( PDEVENT, "[%s] changed from [%u] to [%u]",
                 STP_OPTION_MAXTIMEERROR, _local.getMaxTimeError(),
                 _options->getMaxTimeErrorNS() ) ;

         // set max time error ( NOTE: used nanosecond internal )
         _local.setMaxTimeError( _options->getMaxTimeErrorNS() ) ;

         // mark time error changed
         maxTimeErrorChanged = TRUE ;
      }

      // check if synchronize interval is changed
      if ( _local.getSyncInterval() != _options->getSyncInterval() )
      {
         PD_LOG( PDEVENT, "[%s] changed from [%u] to [%u]",
                 STP_OPTION_SYNCINTERVAL, _local.getSyncInterval(),
                 _options->getSyncInterval() ) ;

         // set synchronize interval
         _local.setSyncInterval( _options->getSyncInterval() ) ;

         // mark synchronize interval changed
         syncIntervalChanged = TRUE ;
      }

      _mutex.release_w() ;

      if ( roleChanged )
      {
         // if role changed, reset primary
         pmdSetPrimary( FALSE ) ;
         resetPrimary() ;
         // notify other modules to change role
         _stpCB->onChangeRole( _options->getRole() ) ;
      }
      if ( maxTimeErrorChanged )
      {
         // cut time error of meta data by new max time error
         // NOTE: use nanoseconds internal
         getMetaData()->cutTimeError( _options->getMaxTimeErrorNS() ) ;
      }
      if ( syncIntervalChanged )
      {
         // set synchronize interval into meta data
         getMetaData()->setSyncInterval( _options->getSyncInterval() ) ;
      }

      if ( roleChanged | maxTimeErrorChanged | syncIntervalChanged )
      {
         // need restart synchronize
         _syncClientManager->activeStatus( STP_SYNC_NOSOURCE ) ;
      }

      PD_TRACE_EXITRC( SDB__STPNODEMGR__UPDATELOCALCONFIGS, rc ) ;

      return rc ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__STPNODEMGR__UPDATESERVERCONFIGS, "_stpNodeManager::_updateServerConfigs" )
   INT32 _stpNodeManager::_updateServerConfigs( BOOLEAN &changed )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__STPNODEMGR__UPDATESERVERCONFIGS ) ;

      changed = FALSE ;

      stpOptions tmpOptions ;
      STP_SERVER_LIST servers ;
      vector< pmdAddrPair > serverList ;
      STP_ROLE role = _options->getRole() ;

      // copy server
      rc = dumpServers( servers ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to dump servers, rc: %d", rc ) ;

      // construct formated old server list
      for ( STP_SERVER_LIST::iterator iter = servers.begin() ;
            iter != servers.end() ;
            ++ iter )
      {
         tmpOptions.addServerAddress( iter->getHostName(),
                                      iter->getServiceName() ) ;
      }
      tmpOptions.formatServerList() ;

      // compare if old server list is the same as new server list
      // NOTE: order of servers will affect result ( but when constructs
      //       replica group, we only take first 7 nodes, so order of servers
      //       need to be considered )
      if ( 0 == ossStrcmp( tmpOptions.getServerListString(),
                           _options->getServerListString() ) )
      {
         // nothing changed
         goto done ;
      }

      PD_LOG( PDEVENT, "[%s] changed from [%s] to [%s]",
              STP_OPTION_SERVERLIST, tmpOptions.getServerListString(),
              _options->getServerListString() ) ;

      // get new server list
      serverList = _options->getServerList() ;

      // check role
      PD_CHECK( STP_ROLE_SERVER == role || STP_ROLE_CLIENT == role,
                SDB_INVALIDARG, error, PDWARNING,
                "Failed to to update servers, unknown role: %d",
                role ) ;

      // check if server list is empty
      PD_CHECK( !serverList.empty(), SDB_INVALIDARG, error, PDERROR,
                "Failed to update servers, empty server list" ) ;

      // update server with given server list
      rc = _updateServers( serverList ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to update servers, rc: %d", rc ) ;

      // increase version if this node is primary
      // NOTE: increase version could notify other nodes to update
      if ( isPrimaryServer() )
      {
         increaseVersion() ;
      }

      // mark changed
      changed = TRUE ;

   done:
      PD_TRACE_EXITRC( SDB__STPNODEMGR__UPDATESERVERCONFIGS, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__STPNODEMGR__CHECKLOCALANDSERVERS, "_stpNodeManager::_checkLocalAndServers" )
   INT32 _stpNodeManager::_checkLocalAndServers()
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__STPNODEMGR__CHECKLOCALANDSERVERS ) ;

      ossScopedRWLock lock( &_mutex, EXCLUSIVE ) ;

      if ( _servers.empty() )
      {
         _setStatus( STP_NODE_NORMAL ) ;
      }
      else if ( _servers.end() != find( _servers.begin(),
                                        _servers.end(),
                                        _local.getRouteID() ) )
      {
         // this node is found in server list, expected to be server role
         if ( STP_ROLE_SERVER != _local.getRole() )
         {
            // this node is not set to server role
            // set status to remove from server list
            PD_LOG( PDWARNING, "Local is not %s, need remove server",
                    stpGetRoleName( STP_ROLE_SERVER ) ) ;
            _setStatus( STP_NODE_REMOVESERVER ) ;
         }
         else
         {
            // set status to normal
            _setStatus( STP_NODE_NORMAL ) ;
         }
      }
      else
      {
         // this node is not found in server list, expected to be client role
         if ( STP_ROLE_CLIENT != _local.getRole() )
         {
            // this node is not set to client role
            // set status to add into server list
            PD_LOG( PDWARNING, "Local is not %s, need add server",
                    stpGetRoleName( STP_ROLE_CLIENT ) ) ;
            _setStatus( STP_NODE_ADDSERVER ) ;
         }
         else
         {
            // set status to normal
            _setStatus( STP_NODE_NORMAL ) ;
         }
      }

      if ( _options->isTestMode() )
      {
         // test mode, this node is primary
         _primaryRID = _local.getRouteID() ;
         pmdSetPrimary( TRUE ) ;
      }
      else if ( STP_NODE_NORMAL != _getStatus() )
      {
         // status is not normal, reset primary
         _primaryRID.value = MSG_INVALID_ROUTEID ;
      }

      PD_TRACE_EXITRC( SDB__STPNODEMGR__CHECKLOCALANDSERVERS, rc ) ;

      return rc ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__STPNODEMGR__SAVESERVERS, "_stpNodeManager::_saveServers" )
   INT32 _stpNodeManager::_saveServers()
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__STPNODEMGR__SAVESERVERS ) ;

      STP_SERVER_LIST servers ;

      // copy servers
      rc = dumpServers( servers ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to dump servers, rc: %d", rc ) ;

      // clear server list from options
      _options->clearServerList() ;

      // add server into server list of options
      for ( STP_SERVER_LIST::iterator iter = servers.begin() ;
            iter != servers.end() ;
            ++ iter )
      {
         _options->addServerAddress( iter->getHostName(),
                                     iter->getServiceName() ) ;
      }

      // save options
      rc = _options->save() ;
      PD_RC_CHECK( rc, PDERROR, "Failed to save catalog options, rc: %d",
                   rc ) ;

      PD_LOG( PDEVENT, "Update server list %s",
              _options->getServerListString() ) ;

   done:
      PD_TRACE_EXITRC( SDB__STPNODEMGR__SAVESERVERS, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__STPNODEMGR__GETSERVERS, "_stpNodeManager::_getServers" )
   void _stpNodeManager::_getServers( UINT32 &version,
                                      STP_SERVER_LIST &servers,
                                      MsgRouteID &primaryRID )
   {
      PD_TRACE_ENTRY( SDB__STPNODEMGR__GETSERVERS ) ;

      BOOLEAN needCheckRepl = FALSE ;

      {
         ossScopedRWLock lock( &_mutex, SHARED ) ;

         // update version, servers and primary
         version = _version ;
         servers = _servers ;
         primaryRID = _primaryRID ;

         if ( STP_ROLE_SERVER == _local.getRole() )
         {
            needCheckRepl = TRUE ;
         }
      }

      // for STP server, use the primary RID in replica manager
      if ( needCheckRepl )
      {
         primaryRID = _replManager->getPrimary() ;
      }

      PD_TRACE_EXIT( SDB__STPNODEMGR__GETSERVERS ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__STPNODEMGR__SETSERVERS, "_stpNodeManager::_setServers" )
   void _stpNodeManager::_setServers( UINT32 version,
                                      const STP_SERVER_LIST &servers,
                                      const MsgRouteID &primaryRID,
                                      STP_SERVER_LIST &removedServers )
   {
      PD_TRACE_ENTRY( SDB__STPNODEMGR__SETSERVERS ) ;

      ossScopedRWLock lock( &_mutex, SHARED ) ;

      // check removed servers
      for ( STP_SERVER_LIST::iterator iter = _servers.begin() ;
            iter != _servers.end() ;
            ++ iter )
      {
         stpServerNode &server = ( *iter ) ;
         // if server is not found in new server list, it is removed
         // save to remove list for later process
         if ( servers.end() == find( servers.begin(),
                                     servers.end(),
                                     server ) )
         {
            removedServers.push_back( server ) ;
         }
      }

      // update version, servers, and primary
      _version = version ;
      _servers = servers ;
      _primaryRID = primaryRID ;

      PD_LOG( PDEVENT, "Update primary %s, version %u done",
              routeID2String( _primaryRID ).c_str(), _version ) ;

      PD_TRACE_EXIT( SDB__STPNODEMGR__SETSERVERS ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__STPNODEMGR__BUILDVERSION, "_stpNodeManager::_buildVersion" )
   INT32 _stpNodeManager::_buildVersion( BSONObjBuilder &builder,
                                         UINT32 version )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__STPNODEMGR__BUILDVERSION ) ;

      try
      {
         // build version element
         builder.append( STP_FIELD_NAME_VERSION, (INT32)version ) ;
      }
      catch ( exception &e )
      {
         rc = ossException2RC( &e ) ;
         PD_LOG( PDERROR, "Failed to build version object, "
                 "occurred unexpected error: %s", e.what() ) ;
         goto error ;
      }

   done:
      PD_TRACE_EXITRC( SDB__STPNODEMGR__BUILDVERSION, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__STPNODEMGR__BUILDSERVERS, "_stpNodeManager::_buildServers" )
   INT32 _stpNodeManager::_buildServers( BSONObjBuilder &builder,
                                         const STP_SERVER_LIST &servers,
                                         BOOLEAN forDisplay )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__STPNODEMGR__BUILDSERVERS ) ;

      try
      {
         // build group object from servers
         BSONArrayBuilder groupBuilder(
                              builder.subarrayStart( STP_FIELD_NAME_GROUP ) ) ;
         for ( STP_SERVER_LIST::const_iterator iter = servers.begin() ;
               iter != servers.end() ;
               ++ iter )
         {
            BSONObjBuilder nodeBuilder( groupBuilder.subobjStart() ) ;

            // format server into BSON object
            rc = iter->toBSON( nodeBuilder, forDisplay ) ;
            PD_RC_CHECK( rc, PDERROR, "Failed to build BSON for server "
                         "node %s, rc: %d", iter->toString().c_str(),
                         rc ) ;
            nodeBuilder.doneFast() ;
         }
         groupBuilder.doneFast() ;
      }
      catch ( exception &e )
      {
         rc = ossException2RC( &e ) ;
         PD_LOG( PDERROR, "Failed to build server group object, "
                 "occurred unexpected error: %s", e.what() ) ;
         goto error ;
      }

   done:
      PD_TRACE_EXITRC( SDB__STPNODEMGR__BUILDSERVERS, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__STPNODEMGR__BUILDPRIMARYNODE, "_stpNodeManager::_buildPrimaryNode" )
   INT32 _stpNodeManager::_buildPrimaryNode( BSONObjBuilder &builder,
                                             const STP_SERVER_LIST &servers,
                                             const MsgRouteID &primaryRID,
                                             BOOLEAN forDisplay )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__STPNODEMGR__BUILDPRIMARYNODE ) ;

      try
      {
         // build primary object
         BSONObjBuilder primaryBuilder(
                             builder.subobjStart( STP_FIELD_NAME_PRIMARY ) ) ;
         if ( forDisplay )
         {
            // for display, use host and service name
            BOOLEAN found = FALSE ;

            // find primary from server list
            if ( MSG_INVALID_ROUTEID != primaryRID.value )
            {
               STP_SERVER_LIST::const_iterator iter = find( servers.begin(),
                                                            servers.end(),
                                                            primaryRID ) ;
               if ( servers.end() != iter )
               {
                  const stpServerNode &primaryServer = ( *iter ) ;

                  // if found primary, append host and service names
                  primaryBuilder.append( STP_FIELD_NAME_HOST,
                                         primaryServer.getHostName() ) ;
                  primaryBuilder.append( STP_FIELD_NAME_SERVICE,
                                         primaryServer.getServiceName() ) ;

                  found = TRUE ;
               }
            }

            if ( !found )
            {
               // if not found primary, append unknown host and service names
               primaryBuilder.append( STP_FIELD_NAME_HOST,
                                      STP_UNKNOWN_HOST_NAME ) ;
               primaryBuilder.append( STP_FIELD_NAME_SERVICE,
                                      STP_UNKNOWN_SERVICE_NAME ) ;
            }
         }
         else
         {
            // not for display, use route ID
            // add group ID
            primaryBuilder.append( STP_FIELD_NAME_GROUPID,
                                   (INT32)( primaryRID.columns.groupID ) ) ;
            // add node ID
            primaryBuilder.append( STP_FIELD_NAME_NODEID,
                                   (INT32)( primaryRID.columns.nodeID ) ) ;
         }
         primaryBuilder.doneFast() ;
      }
      catch ( exception &e )
      {
         rc = ossException2RC( &e ) ;
         PD_LOG( PDERROR, "Failed to build primary node object, "
                 "occurred unexpected error: %s", e.what() ) ;
         goto error ;
      }

   done:
      PD_TRACE_EXITRC( SDB__STPNODEMGR__BUILDPRIMARYNODE, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__STPNODEMGR__PARSEVERSION, "_stpNodeManager::_parseVersion" )
   INT32 _stpNodeManager::_parseVersion( const BSONObj &object,
                                         UINT32 &version )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__STPNODEMGR__PARSEVERSION ) ;

      try
      {
         // get version element
         BSONElement versionElement =
                                 object.getField( STP_FIELD_NAME_VERSION ) ;
         // version element should not be empty
         PD_CHECK( EOO != versionElement.type(),
                   SDB_INVALIDARG, error, PDERROR,
                   "Failed to parse version object, "
                   "field [%s] is not found",
                   STP_FIELD_NAME_VERSION ) ;
         // version element should be integer type
         PD_CHECK( NumberInt == versionElement.type(),
                   SDB_INVALIDARG, error, PDERROR,
                   "Failed to parse version object, "
                   "field [%s] is not an integer",
                   STP_FIELD_NAME_VERSION ) ;

         // set version to output
         version = (UINT32)versionElement.numberInt() ;
      }
      catch ( exception &e )
      {
         rc = ossException2RC( &e ) ;
         PD_LOG( PDERROR, "Failed to parse version object, "
                 "occurred unexpected error: %s", e.what() ) ;
         goto error ;
      }

   done:
      PD_TRACE_EXITRC( SDB__STPNODEMGR__PARSEVERSION, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__STPNODEMGR__PARSESERVERS, "_stpNodeManager::_parseServers" )
   INT32 _stpNodeManager::_parseServers( const BSONObj &object,
                                         STP_SERVER_LIST &servers )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__STPNODEMGR__PARSESERVERS ) ;

      try
      {
         // get group element
         BSONElement groupElement = object.getField( STP_FIELD_NAME_GROUP ) ;
         if ( Array != groupElement.type() )
         {
            // group element should not be empty
            PD_CHECK( EOO != groupElement.type(),
                      SDB_INVALIDARG, error, PDERROR,
                      "Failed to parse server group object, "
                      "field [%s] is not found",
                      STP_FIELD_NAME_GROUP ) ;
            // group element should be array
            PD_CHECK( Array == groupElement.type(),
                      SDB_INVALIDARG, error, PDERROR,
                      "Failed to parse server group object, "
                      "field [%s] is not an array",
                      STP_FIELD_NAME_GROUP ) ;
         }
         else
         {
            // parse each object in group element
            BSONObjIterator iter( groupElement.embeddedObject() ) ;
            while ( iter.more() )
            {
               stpServerNode server ;

               // get next server element
               BSONElement serverElement( iter.next() ) ;

               // server element should be object type
               PD_CHECK( Object == serverElement.type(),
                         SDB_INVALIDARG, error, PDERROR,
                         "Failed to parse server group object, "
                         "node object is not an object" ) ;

               // parse BSON into server
               rc = server.fromBSON( serverElement.embeddedObject(), FALSE ) ;
               PD_RC_CHECK( rc, PDERROR, "Failed to parse server node %s, "
                            "rc: %d", serverElement.toPoolString().c_str(),
                            rc ) ;

               // add into server list
               servers.push_back( server ) ;
            }
         }
      }
      catch ( exception &e )
      {
         rc = ossException2RC( &e ) ;
         PD_LOG( PDERROR, "Failed to parse server group object, "
                 "occurred unexpected error: %s", e.what() ) ;
         goto error ;
      }

      // check if servers is empty
      PD_CHECK( !servers.empty(), SDB_INVALIDARG, error, PDERROR,
                "Failed to parse server group object, it is empty" ) ;

   done:
      PD_TRACE_EXITRC( SDB__STPNODEMGR__PARSESERVERS, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__STPNODEMGR__PARSEPRIMARYNODE, "_stpNodeManager::_parsePrimaryNode" )
   INT32 _stpNodeManager::_parsePrimaryNode( const BSONObj &object,
                                             MsgRouteID &primaryRID )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__STPNODEMGR__PARSEPRIMARYNODE ) ;

      try
      {
         // get primary element
         BSONElement primaryElement =
                                 object.getField( STP_FIELD_NAME_PRIMARY ) ;
         if ( Object != primaryElement.type() )
         {
            // primary element should not be empty
            PD_CHECK( EOO != primaryElement.type(),
                      SDB_INVALIDARG, error, PDERROR,
                      "Failed to parse primary node object, "
                      "field [%s] is not found",
                      STP_FIELD_NAME_PRIMARY ) ;
            // primary element should be object
            PD_CHECK( Object == primaryElement.type(),
                      SDB_INVALIDARG, error, PDERROR,
                      "Failed to parse primary node object, "
                      "field [%s] is not an object",
                      STP_FIELD_NAME_PRIMARY ) ;
         }
         else
         {
            BSONObj primaryObject = primaryElement.embeddedObject() ;
            BSONElement element ;

            // get group ID element
            element = primaryObject.getField( STP_FIELD_NAME_GROUPID ) ;
            // group ID element should not be empty
            PD_CHECK( EOO != element.type(),
                      SDB_INVALIDARG, error, PDERROR,
                      "Failed to parse primary node object, "
                      "field [%s] is not found",
                      STP_FIELD_NAME_GROUPID ) ;
            // group ID element should be integer type
            PD_CHECK( NumberInt == element.type(),
                      SDB_INVALIDARG, error, PDERROR,
                      "Failed to parse primary node object, "
                      "field [%s] is not an integer",
                      STP_FIELD_NAME_GROUPID ) ;
            // set group ID
            primaryRID.columns.groupID = (UINT32)element.numberInt() ;

            // get node ID element
            element = primaryObject.getField( STP_FIELD_NAME_NODEID ) ;
            // node ID element should no be empty
            PD_CHECK( EOO != element.type(),
                      SDB_INVALIDARG, error, PDERROR,
                      "Failed to parse primary node object, "
                      "field [%s] is not found",
                      STP_FIELD_NAME_NODEID ) ;
            // node ID element should be integer type
            PD_CHECK( NumberInt == element.type(),
                      SDB_INVALIDARG, error, PDERROR,
                      "Failed to parse primary node object, "
                      "field [%s] is not an integer",
                      STP_FIELD_NAME_NODEID ) ;
            // set node ID
            primaryRID.columns.nodeID = (UINT32)element.numberInt() ;

            // always use local service
            primaryRID.columns.serviceID = MSG_ROUTE_LOCAL_SERVICE ;
         }
      }
      catch ( exception &e )
      {
         rc = ossException2RC( &e ) ;
         PD_LOG( PDERROR, "Failed to parse primary node object, "
                 "occurred unexpected error: %s", e.what() ) ;
         goto error ;
      }

      // check if group ID of primary is valid
      PD_CHECK( INVALID_GROUPID != primaryRID.columns.groupID,
                SDB_INVALIDARG, error, PDERROR,
                "Failed to parse primary node object, group ID [%u] is invalid",
                primaryRID.columns.groupID ) ;

      // check if node id of primary is valid
      PD_CHECK( INVALID_NODEID != primaryRID.columns.nodeID,
                SDB_INVALIDARG, error, PDERROR,
                "Failed to parse primary node object, node ID [%u] is invalid",
                primaryRID.columns.nodeID ) ;

   done:
      PD_TRACE_EXITRC( SDB__STPNODEMGR__PARSEPRIMARYNODE, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__STPNODEMGR__ADDSERVER_VER, "_stpNodeManager::_addServer" )
   INT32 _stpNodeManager::_addServer( const stpServerNode &server,
                                      BOOLEAN increaseVersion )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__STPNODEMGR__ADDSERVER_VER ) ;

      BOOLEAN locked = FALSE ;
      STP_SERVER_LIST::iterator iter ;

      // update route ID to net agent
      rc = _netManager->updateRouteID( server.getRouteID(),
                                       server.getHostName(),
                                       server.getServiceName() ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to update routeID for node %s, rc: %d",
                   server.toString().c_str(), rc ) ;

      // check if role is valid
      PD_CHECK( server.isValidRole(), SDB_INVALIDARG, error, PDERROR,
                "Failed to check server node %s, role is invalid",
                server.toString().c_str() ) ;

      // check if route ID is valid
      PD_CHECK( server.isValidRoute(), SDB_INVALID_ROUTEID, error, PDERROR,
                "Failed to check server node %s, route ID is invalid",
                server.toString().c_str() ) ;

      _mutex.lock_w() ;
      locked = TRUE ;

      // check if server already exists
      iter = find( _servers.begin(), _servers.end(), server ) ;
      PD_CHECK( iter == _servers.end(),
                SDB_INVALID_ROUTEID, error, PDERROR,
                "Failed to check server node %s, route ID is conflict",
                server.toString().c_str() ) ;

      // add server to server list
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
      PD_TRACE_EXITRC( SDB__STPNODEMGR__ADDSERVER_VER, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__STPNODEMGR__REMOVESERVER_VER, "_stpNodeManager::_removeServer" )
   INT32 _stpNodeManager::_removeServer( const MsgRouteID &routeID,
                                         BOOLEAN increaseVersion )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__STPNODEMGR__REMOVESERVER_VER ) ;

      BOOLEAN locked = FALSE ;

      _mutex.lock_w() ;
      locked = TRUE ;

      // find server by route ID
      STP_SERVER_LIST::iterator iter = find( _servers.begin(),
                                             _servers.end(),
                                             routeID ) ;
      PD_CHECK( iter != _servers.end(),
                SDB_INVALID_ROUTEID, error, PDWARNING,
                "Failed to remove server node %s, route ID is not found",
                routeID2String( routeID ).c_str() ) ;

      // remove server
      _servers.erase( iter ) ;

      // increase version if needed
      if ( increaseVersion )
      {
         ++ _version ;
      }

      _mutex.release_w() ;
      locked = FALSE ;

      // delete route ID from net agent
      _netManager->deleteRouteID( routeID ) ;

      PD_LOG( PDEVENT, "Remove server node %s done",
              routeID2String( routeID ).c_str() ) ;

   done:
      if ( locked )
      {
         _mutex.release_w() ;
      }
      PD_TRACE_EXITRC( SDB__STPNODEMGR__REMOVESERVER_VER, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__STPNODEMGR__UPDATESERVERS_LIST, "_stpNodeManager::_updateServers" )
   INT32 _stpNodeManager::_updateServers(
                                    const vector< pmdAddrPair > &serverList )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__STPNODEMGR__UPDATESERVERS_LIST ) ;

      // clear old servers
      rc = removeServers() ;
      PD_RC_CHECK( rc, PDERROR, "Failed to remove servers, rc: %d", rc ) ;

      // add server for each address in server list
      for ( vector< pmdAddrPair >::const_iterator iter = serverList.begin() ;
            iter != serverList.end() ;
            ++ iter )
      {
         stpServerNode server ;
         const CHAR *hostName = iter->_host ;
         const CHAR *serviceName = iter->_service ;
         MsgRouteID routeID ;

         // get route ID by host and service
         // format IP address into 32 bit integer used as group ID
         // and port is used as node ID
         routeID.value = MSG_INVALID_ROUTEID ;
         rc = _netManager->getRouteID( hostName, serviceName, routeID ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to get route ID %s:%s, rc: %d",
                      hostName, serviceName, rc ) ;

         // set fields of server
         server.setRouteID( routeID ) ;
         server.setHostName( hostName ) ;
         server.setServiceName( serviceName ) ;

         // add server ( NOTE: no need to increase group version )
         rc = _addServer( server, FALSE ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to register server node %s, "
                      "rc: %d", server.toString().c_str(), rc ) ;

         PD_LOG( PDEVENT, "Initialize server node %s done",
                 server.toString().c_str() ) ;
      }

   done:
      PD_TRACE_EXITRC( SDB__STPNODEMGR__UPDATESERVERS_LIST, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__STPNODEMGR_HASPRIMARY, "_stpNodeManager::hasPrimary" )
   BOOLEAN _stpNodeManager::hasPrimary()
   {
      BOOLEAN result = FALSE ;

      PD_TRACE_ENTRY( SDB__STPNODEMGR_HASPRIMARY ) ;

      BOOLEAN needCheckRepl = FALSE ;

      {
         ossScopedRWLock lock( &_mutex, SHARED ) ;
         if ( STP_ROLE_SERVER == _local.getRole() )
         {
            needCheckRepl = TRUE ;
         }
         result = _hasPrimary() ;
      }

      // for STP server, use the primary RID in replica manager
      if ( needCheckRepl )
      {
         result = _replManager->getPrimary().value != MSG_INVALID_ROUTEID ;
      }

      PD_TRACE_EXIT( SDB__STPNODEMGR_HASPRIMARY ) ;

      return result ;
   }

}
