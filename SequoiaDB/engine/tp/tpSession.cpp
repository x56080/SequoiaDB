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

   Source File Name = tpSession.cpp

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

#include "tpSession.hpp"
#include "tpCB.hpp"
#include "pdTrace.hpp"
#include "tpTrace.hpp"
#include "pmd.hpp"
#include "msgAuth.hpp"
#include "msgMessage.hpp"
#include "tpCommand.hpp"
#include "../bson/lib/md5.hpp"

using namespace bson ;
using namespace md5 ;

namespace engine
{

   /*
      _tpSession implement
    */
   BEGIN_OBJ_MSG_MAP( _tpSession, _pmdAsyncSession )
      // msg map or event map
      ON_MSG( MSG_AUTH_VERIFY_REQ, _handleAuthReq )
      ON_MSG( MSG_BS_QUERY_REQ, _handleQueryReq )
   END_OBJ_MSG_MAP()

   _tpSession::_tpSession( UINT64 sessionID, SDB_TPCB *tpCB )
   : pmdAsyncSession( sessionID ),
     _tpCB( tpCB )
   {
   }

   _tpSession::~_tpSession()
   {
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__TPSESSION__DEFAULTMSGFUNC, "_tpSession::_defaultMsgFunc" )
   INT32 _tpSession::_defaultMsgFunc( NET_HANDLE handle, MsgHeader *message )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__TPSESSION__DEFAULTMSGFUNC ) ;

      PD_LOG( PDWARNING, "Session[%s] Recieve unknown message[type:[%d]%u, "
              "len:%u]", sessionName(),
              IS_REPLY_TYPE( message->opCode ) ? 1 : 0,
              GET_REQUEST_TYPE( message->opCode ),
              message->messageLength ) ;

      rc = _sendReply( message, SDB_UNKNOWN_MESSAGE ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to send error reply, rc: %d", rc ) ;

   done:
      PD_TRACE_EXITRC( SDB__TPSESSION__DEFAULTMSGFUNC, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__TPSESSION__HANDLEAUTHREQ, "_tpSession::_handleAuthReq" )
   INT32 _tpSession::_handleAuthReq( NET_HANDLE handle, MsgHeader *message )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__TPSESSION__HANDLEAUTHREQ ) ;

      try
      {
         BSONObj object ;
         BSONElement user, password ;

         rc = extractAuthMsg( message, object ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to extract authentication request,"
               "rc: %d", rc ) ;

         user = object.getField( SDB_AUTH_USER ) ;
         password = object.getField( SDB_AUTH_PASSWD ) ;

         // check user and password
         PD_CHECK( 0 == ossStrcmp( user.valuestrsafe(), SDB_TP_USER ),
                   SDB_AUTH_AUTHORITY_FORBIDDEN, error, PDERROR,
                   "Failed authenticate, user name[%s] is not suppport",
                   user.valuestrsafe() ) ;

         PD_CHECK( md5simpledigest( string( SDB_TP_USERPASSWD ) ) ==
                   string( password.valuestrsafe() ),
                   SDB_AUTH_AUTHORITY_FORBIDDEN, error, PDERROR,
                   "Failed authenticate, user name[%s] is not correct",
                   password.valuestrsafe() ) ;

         getClient()->authenticate( user.valuestrsafe(),
                                    password.valuestrsafe() ) ;
      }
      catch ( exception &e )
      {
         PD_LOG( PDERROR, "Failed to handle authentication request, error: %s",
                 e.what() ) ;
         rc = SDB_SYS ;
         goto error ;
      }

   done:
      _sendReply( message, rc ) ;

      PD_TRACE_EXITRC( SDB__TPSESSION__HANDLEAUTHREQ, rc ) ;

      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__TPSESSION__HANDLEQUERYREQ, "_tpSession::_handleQueryReq" )
   INT32 _tpSession::_handleQueryReq( NET_HANDLE handle, MsgHeader *message )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__TPSESSION__HANDLEQUERYREQ ) ;

      CHAR *commandName = NULL ;
      CHAR *optionBuffer = NULL ;
      tpCommand *command = NULL ;
      BSONObj result ;

      rc = msgExtractQuery( (CHAR *)message, NULL, &commandName, NULL, NULL,
                            &optionBuffer, NULL, NULL, NULL ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to extract query, rc: %d", rc ) ;

      PD_CHECK( NULL != commandName, SDB_INVALIDARG, error, PDERROR,
                "Failed to get command name" ) ;

      rc = tpGetCommand( commandName, &command ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to get command [%s], rc: %d",
                   commandName, rc ) ;
      PD_CHECK( NULL != command, SDB_INVALIDARG, error, PDERROR,
                "Failed to get command [%s], command is invalid",
                commandName ) ;

      rc = tpInitCommand( command, optionBuffer ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to initialize command [%s], rc: %d",
                   commandName, rc ) ;

      rc = tpRunCommand( command, result ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to run command [%s], rc: %d",
                   commandName, rc ) ;

      rc = _sendReply( message, SDB_OK, result ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "Failed to send reply to %s, rc: %d",
                 routeID2String( message->routeID ).c_str(), rc ) ;
      }

   done:
      tpReleaseCommand( command ) ;
      PD_TRACE_EXITRC( SDB__TPSESSION__HANDLEQUERYREQ, rc ) ;
      return rc ;

   error:
      _sendReply( message, rc ) ;
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__TPSESSION__SENDREPLY, "_tpSession::_sendReply" )
   INT32 _tpSession::_sendReply( MsgOpReply *reply,
                                 const CHAR *body,
                                 UINT32 bodySize )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__TPSESSION__SENDREPLY ) ;

      PD_CHECK( (UINT32)( reply->header.messageLength ) ==
                sizeof( MsgOpReply ) + bodySize,
                SDB_SYS, error, PDERROR,
                "Session [%s]: Failed to send reply message, reply message "
                "length error [%u != %u]", sessionName(),
                reply->header.messageLength, sizeof( MsgOpReply ) + bodySize ) ;

      if ( NULL != body && bodySize > 0 )
      {
         rc = routeAgent()->syncSend( _netHandle, (MsgHeader *)reply,
                                      (void *)body, bodySize ) ;
      }
      else
      {
         rc = routeAgent()->syncSend( _netHandle, (void *)reply ) ;
      }

      PD_RC_CHECK( rc, PDERROR, "Session[%s]: Failed to send reply message, "
                   "rc: %d", sessionName(), rc ) ;

   done:
      PD_TRACE_EXITRC( SDB__TPSESSION__SENDREPLY, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__TPSESSION__SENDREPLY_RC, "_tpSession::_sendReply" )
   INT32 _tpSession::_sendReply( MsgHeader *message, INT32 returnCode )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__TPSESSION__SENDREPLY_RC ) ;

      BSONObj dummy ;

      rc = _sendReply( message, returnCode, dummy ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to send reply, rc: %d", rc ) ;

   done:
      PD_TRACE_EXITRC( SDB__TPSESSION__SENDREPLY_RC, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__TPSESSION__SENDREPLY_BSON, "_tpSession::_sendReply" )
   INT32 _tpSession::_sendReply( MsgHeader *message,
                                 INT32 returnCode,
                                 const bson::BSONObj &result )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__TPSESSION__SENDREPLY_RC ) ;

      //Build reply message
      MsgOpReply reply ;

      reply.header.opCode = MAKE_REPLY_TYPE( message->opCode ) ;
      reply.header.messageLength = sizeof ( MsgOpReply ) ;
      reply.header.requestID = message->requestID ;
      reply.header.TID = message->TID ;
      reply.header.routeID.value = 0 ;
      reply.flags = returnCode ;
      reply.contextID = -1 ;
      reply.numReturned = 0 ;
      reply.startFrom = 0 ;

      if ( SDB_OK != returnCode )
      {
         BSONObj errorInfo ;

         errorInfo = utilGetErrorBson(
                           returnCode, _pEDUCB->getInfo( EDU_INFO_ERROR ) ) ;
         reply.header.messageLength += errorInfo.objsize() ;
         reply.numReturned = 1 ;

         rc = _sendReply( &reply, errorInfo.objdata(), errorInfo.objsize() ) ;
      }
      else
      {
         reply.header.messageLength += result.objsize() ;
         reply.numReturned = 1 ;

         rc = _sendReply( &reply, result.objdata(), result.objsize() ) ;
      }
      PD_RC_CHECK( rc, PDERROR, "Failed to send reply, rc: %d", rc ) ;

   done:
      PD_TRACE_EXITRC( SDB__TPSESSION__SENDREPLY_RC, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   /*
      _tpSessionManager implement
    */
   _tpSessionManager::_tpSessionManager( SDB_TPCB *tpCB )
   : _tpCB( tpCB )
   {
   }

   _tpSessionManager::~_tpSessionManager()
   {
   }

   UINT64 _tpSessionManager::makeSessionID( const NET_HANDLE &handle,
                                            const MsgHeader *header )
   {
      return ossPack32To64( PMD_BASE_HANDLE_ID + handle, header->TID ) ;
   }

   SDB_SESSION_TYPE _tpSessionManager::_prepareCreate( UINT64 sessionID,
                                                       INT32 startType,
                                                       INT32 opCode )
   {
      return SDB_SESSION_TP ;
   }

   BOOLEAN _tpSessionManager::_canReuse( SDB_SESSION_TYPE sessionType )
   {
      return FALSE ;
   }

   UINT32 _tpSessionManager::_maxCacheSize() const
   {
      return 0 ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__TPSERVICEMANAGER_ONERROR, "_tpSessionManager::onErrorHanding" )
   INT32 _tpSessionManager::onErrorHanding( INT32 rc,
                                            const MsgHeader *request,
                                            const NET_HANDLE &handle,
                                            UINT64 sessionID,
                                            pmdAsyncSession *session )
   {
      INT32 ret = SDB_OK ;

      PD_TRACE_ENTRY( SDB__TPSERVICEMANAGER_ONERROR ) ;

      if ( 0 != sessionID )
      {
         ret = _reply( handle, rc, request ) ;
      }
      else
      {
         ret = rc ;
      }

      PD_TRACE_EXITRC( SDB__TPSERVICEMANAGER_ONERROR, ret ) ;

      return ret ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__TPSERVICEMANAGER__CREATESESS, "_tpSessionManager::_createSession" )
   pmdAsyncSession* _tpSessionManager::_createSession(
                                                SDB_SESSION_TYPE sessionType,
                                                INT32 startType,
                                                UINT64 sessionID,
                                                void *data )
   {
      pmdAsyncSession *pSession = NULL ;

      PD_TRACE_ENTRY( SDB__TPSERVICEMANAGER__CREATESESS ) ;

      if ( SDB_SESSION_TP == sessionType )
      {
         pSession = SDB_OSS_NEW tpSession( sessionID, _tpCB ) ;
      }
      else
      {
         PD_LOG( PDERROR, "Invalid session type [%d]", sessionType ) ;
      }

      PD_TRACE_EXIT( SDB__TPSERVICEMANAGER__CREATESESS ) ;

      return pSession ;
   }

}
