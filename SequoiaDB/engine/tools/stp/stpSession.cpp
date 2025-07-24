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

   Source File Name = stpSession.cpp

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
#include "stpSession.hpp"
#include "stpCB.hpp"
#include "pdTrace.hpp"
#include "stpTrace.hpp"
#include "pmd.hpp"
#include "msgAuth.hpp"
#include "msgMessage.hpp"
#include "stpCommand.hpp"
#include "../bson/lib/md5.hpp"

using namespace bson ;
using namespace md5 ;

namespace engine
{

   /*
      _stpSession implement
    */
   BEGIN_OBJ_MSG_MAP( _stpSession, _pmdAsyncSession )
      // msg map or event map
      ON_MSG( MSG_AUTH_VERIFY_REQ, _handleAuthReq )
      ON_MSG( MSG_BS_QUERY_REQ, _handleQueryReq )
      ON_MSG( MSG_BS_QUERY_RES, _handleQueryRes )
   END_OBJ_MSG_MAP()

   _stpSession::_stpSession( UINT64 sessionID, STPCB *stpCB )
   : pmdAsyncSession( sessionID ),
     _stpCB( stpCB ),
     _redirectID( STP_INVALID_REDIRECT_ID ),
     _lastThreadID( 0 ),
     _lastRequestID( 0LL ),
     _curCommand( NULL )
   {
   }

   _stpSession::~_stpSession()
   {
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__STPSESSION__DEFAULTMSGFUNC, "_stpSession::_defaultMsgFunc" )
   INT32 _stpSession::_defaultMsgFunc( NET_HANDLE handle, MsgHeader *message )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__STPSESSION__DEFAULTMSGFUNC ) ;

      PD_LOG( PDWARNING, "Session[%s] received unknown message[type:[%d]%u, "
              "len:%u]", sessionName(),
              IS_REPLY_TYPE( message->opCode ) ? 1 : 0,
              GET_REQUEST_TYPE( message->opCode ),
              message->messageLength ) ;

      // send reply
      rc = _sendReply( message, SDB_UNKNOWN_MESSAGE ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to send error reply, rc: %d", rc ) ;

   done:
      PD_TRACE_EXITRC( SDB__STPSESSION__DEFAULTMSGFUNC, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__STPSESSION__HANDLEAUTHREQ, "_stpSession::_handleAuthReq" )
   INT32 _stpSession::_handleAuthReq( NET_HANDLE handle, MsgHeader *message )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__STPSESSION__HANDLEAUTHREQ ) ;

      SDB_ASSERT( NULL != message, "message is invalid" ) ;
      SDB_ASSERT( MSG_AUTH_VERIFY_REQ == message->opCode,
                  "opcode of message is invalid" ) ;

      try
      {
         BSONObj object ;
         BSONElement user, password ;

         // extract authentication fields
         rc = extractAuthMsg( message, object ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to extract authentication request,"
               "rc: %d", rc ) ;

         // user
         user = object.getField( SDB_AUTH_USER ) ;
         // password
         password = object.getField( SDB_AUTH_PASSWD ) ;

         // check user
         PD_CHECK( 0 == ossStrcmp( user.valuestrsafe(), STP_USER ),
                   SDB_AUTH_AUTHORITY_FORBIDDEN, error, PDERROR,
                   "Failed authenticate, user name[%s] is not suppport",
                   user.valuestrsafe() ) ;

         // check password
         PD_CHECK( md5simpledigest( string( STP_USERPASSWD ) ) ==
                   string( password.valuestrsafe() ),
                   SDB_AUTH_AUTHORITY_FORBIDDEN, error, PDERROR,
                   "Failed authenticate, user name[%s] is not correct",
                   password.valuestrsafe() ) ;

         // save information of user
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

      PD_TRACE_EXITRC( SDB__STPSESSION__HANDLEAUTHREQ, rc ) ;

      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__STPSESSION__HANDLEQUERYREQ, "_stpSession::_handleQueryReq" )
   INT32 _stpSession::_handleQueryReq( NET_HANDLE handle, MsgHeader *message )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__STPSESSION__HANDLEQUERYREQ ) ;

      SDB_ASSERT( NULL != message, "message is invalid" ) ;
      SDB_ASSERT( MSG_BS_QUERY_REQ == message->opCode,
                  "opcode of message is invalid" ) ;

      const CHAR *commandName = NULL ;
      const CHAR *optionBuffer = NULL ;
      stpCommand *command = NULL ;
      BOOLEAN finished = FALSE ;
      BSONObj result ;

      _redirectID = STP_INVALID_REDIRECT_ID ;

      // extract field of query, command name and option
      rc = msgExtractQuery( (const CHAR *)message, NULL, &commandName,
                            NULL, NULL, &optionBuffer, NULL, NULL, NULL ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to extract query, rc: %d", rc ) ;

      PD_CHECK( NULL != commandName, SDB_INVALIDARG, error, PDERROR,
                "Failed to get command name" ) ;

      // get command
      rc = stpGetCommand( _stpCB, commandName, &command ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to get command [%s], rc: %d",
                   commandName, rc ) ;
      PD_CHECK( NULL != command, SDB_INVALIDARG, error, PDERROR,
                "Failed to get command [%s], command is invalid",
                commandName ) ;

      // initialize command with given option
      rc = stpInitCommand( command, optionBuffer ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to initialize command [%s], rc: %d",
                   commandName, rc ) ;

      // check primary
      if ( command->needPrimary() &&
           !_stpCB->checkPrimaryServer( _pEDUCB ) )
      {
         // if this command could be redirected to primary,
         // redirect command to primary
         PD_CHECK( command->canRedirectPrimary(),
                   SDB_CLS_NOT_PRIMARY, error, PDERROR,
                   "Failed to check primary for command [%s]",
                   commandName ) ;

         // avoid redirect recursively
         PD_CHECK( MSG_INVALID_ROUTEID == message->routeID.value,
                   SDB_CLS_NOT_PRIMARY, error, PDERROR,
                   "Failed to check primary for command [%s], "
                   "command already redirected", commandName ) ;

         // redirect message
         rc = _redirectPrimary( message ) ;
         if ( SDB_OK == rc )
         {
            // redirect done
            goto done ;
         }
         else if ( SDB_CLS_NOT_SECONDARY == rc )
         {
            // now it is primary, no need to redirect
            PD_CHECK( _stpCB->isPrimaryServer(),
                      SDB_CLS_NOT_PRIMARY, error, PDERROR,
                      "Failed to check primary for command [%s] again",
                      commandName ) ;
            rc = SDB_OK ;
         }
         else
         {
            PD_RC_CHECK( rc, PDERROR, "Failed to redirect command [%s] to "
                         "primary, rc: %d", commandName, rc ) ;
         }
      }

      // check business if needed
      if ( command->needCheckBusiness() )
      {
         PD_CHECK( pmdGetKRCB()->isBusinessOK(), SDB_SYS, error, PDERROR,
                   "Failed to check business for command [%s]", commandName ) ;
      }

      // run command
      rc = stpRunCommand( command, this, message, result, finished ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to run command [%s], rc: %d",
                   commandName, rc ) ;

      if ( !finished )
      {
         // save command for further processing
         rc = _saveCommand( command ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to save command [%s] for further "
                      "processing, rc: %d", commandName, command ) ;

         command = NULL ;
         goto done ;
      }

      // send reply with query result
      rc = _sendReply( message, SDB_OK, result ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "Failed to send reply for command [%s], "
                 "rc: %d", commandName, rc ) ;
      }

   done:
      if ( NULL != command )
      {
         // release command
         stpReleaseCommand( command ) ;
      }

      if ( NULL != commandName )
      {
         PD_LOG( PDEVENT, "Done command [%s], rc: %d", commandName, rc ) ;
      }

      PD_TRACE_EXITRC( SDB__STPSESSION__HANDLEQUERYREQ, rc ) ;
      return rc ;

   error:
      // send error reply
      _sendReply( message, rc ) ;
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__STPSESSION__HANDLEQUERYRES, "_stpSession::_handleQueryRes" )
   INT32 _stpSession::_handleQueryRes( NET_HANDLE handle, MsgHeader *message )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__STPSESSION__HANDLEQUERYRES ) ;

      SDB_ASSERT( NULL != message, "message is invalid" ) ;
      SDB_ASSERT( MSG_BS_QUERY_RES == message->opCode,
                  "opcode of message is invalid" ) ;

      // for query result, it is response from redirected request, which it
      // is redirect to other nodes earlier, we should send it back to client

      // get redirect ID of query result message
      UINT64 tmpRedirectID = ossPack32To64( _pEDUCB->getTID(),
                                            (UINT32)( message->requestID ) ) ;

      // check thread ID
      PD_CHECK( message->TID == _pEDUCB->getTID(), SDB_SYS, error, PDERROR,
                "Failed to handle query result, thread ID is different, "
                "current [%u], message [%u]", _pEDUCB->getTID(),
                message->TID ) ;

      // check redirect ID
      PD_CHECK( STP_INVALID_REDIRECT_ID != _redirectID,
                SDB_SYS, error, PDERROR,
                "Failed to do handle query result, redirect ID is invalid" ) ;

      // check redirect ID against the one from message
      PD_CHECK( tmpRedirectID == _redirectID, SDB_SYS, error, PDERROR,
                "Failed to handle query result, redirect ID is different, "
                "current [%llu], message [%llu]", _redirectID,
                tmpRedirectID ) ;

      // reset thread ID and request ID
      message->TID = _lastThreadID ;
      message->requestID = _lastRequestID ;

      if ( NULL != _curCommand )
      {
         BOOLEAN finished = FALSE ;
         BSONObj result ;

         PD_LOG( PDDEBUG, "Continue process command [%s]",
                 _curCommand->getName() ) ;

         rc = _curCommand->doit( this, message, result, finished ) ;
         if ( SDB_OK == rc )
         {
            rc = _sendReply( message, SDB_OK, result ) ;
            if ( SDB_OK != rc )
            {
               PD_LOG( PDERROR, "Failed to send reply for command [%s], "
                       "rc: %d", _curCommand->getName(), rc ) ;
            }
         }
         else
         {
            PD_LOG( PDERROR, "Failed to continue process command [%s], "
                    "rc: %d", _curCommand->getName(), rc ) ;
            _sendReply( message, rc ) ;
         }

         goto done ;
      }

      // send it back to client
      rc = _sendReply( message ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to send reply, rc: %d", rc ) ;

   done:
      // reset command
      if ( NULL != _curCommand )
      {
         stpReleaseCommand( _curCommand ) ;
         _curCommand = NULL ;
      }
      // reset redirect ID
      resetRedirectID() ;

      PD_TRACE_EXITRC( SDB__STPSESSION__HANDLEQUERYRES, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__STPSESSION__SENDREPLY, "_stpSession::_sendReply" )
   INT32 _stpSession::_sendReply( MsgOpReply *reply,
                                  const CHAR *body,
                                  UINT32 bodySize )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__STPSESSION__SENDREPLY ) ;

      // check length of reply
      PD_CHECK( (UINT32)( reply->header.messageLength ) ==
                sizeof( MsgOpReply ) + bodySize,
                SDB_SYS, error, PDERROR,
                "Session [%s]: Failed to send reply message, reply message "
                "length error [%u != %u]", sessionName(),
                reply->header.messageLength, sizeof( MsgOpReply ) + bodySize ) ;

      if ( NULL != body && bodySize > 0 )
      {
         // send with results
         rc = routeAgent()->syncSend( _netHandle, (MsgHeader *)reply,
                                      (void *)body, bodySize ) ;
      }
      else
      {
         // send reply only
         rc = routeAgent()->syncSend( _netHandle, (MsgHeader *)reply ) ;
      }

      PD_RC_CHECK( rc, PDERROR, "Session[%s]: Failed to send reply message, "
                   "rc: %d", sessionName(), rc ) ;

   done:
      PD_TRACE_EXITRC( SDB__STPSESSION__SENDREPLY, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__STPSESSION__SENDREPLY_RC, "_stpSession::_sendReply" )
   INT32 _stpSession::_sendReply( MsgHeader *message, INT32 returnCode )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__STPSESSION__SENDREPLY_RC ) ;

      BSONObj dummy ;

      // send reply with empty object
      rc = _sendReply( message, returnCode, dummy ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to send reply, rc: %d", rc ) ;

   done:
      PD_TRACE_EXITRC( SDB__STPSESSION__SENDREPLY_RC, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__STPSESSION__SENDREPLY_BSON, "_stpSession::_sendReply" )
   INT32 _stpSession::_sendReply( MsgHeader *message,
                                  INT32 returnCode,
                                  const bson::BSONObj &result )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__STPSESSION__SENDREPLY_RC ) ;

      // build reply message
      MsgOpReply reply ;

      // fill reply
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

         try
         {
            // reply is not OK, fill with error message
            errorInfo = utilGetErrorBson(
                           returnCode, _pEDUCB->getInfo( EDU_INFO_ERROR ) ) ;
         }
         catch ( exception &e )
         {
            // error happened to construct error object, should continue to
            // send the return code with empty error message
            PD_LOG( PDWARNING, "Failed to build BSON for error message, "
                    "error: %s", e.what() ) ;
         }
         reply.header.messageLength += errorInfo.objsize() ;
         reply.numReturned = 1 ;

         // send reply with error message
         rc = _sendReply( &reply, errorInfo.objdata(), errorInfo.objsize() ) ;
      }
      else
      {
         // add message length with result
         reply.header.messageLength += result.objsize() ;
         reply.numReturned = 1 ;

         // send reply with result
         rc = _sendReply( &reply, result.objdata(), result.objsize() ) ;
      }
      PD_RC_CHECK( rc, PDERROR, "Failed to send reply, rc: %d", rc ) ;

   done:
      PD_TRACE_EXITRC( SDB__STPSESSION__SENDREPLY_RC, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__STPSESSION__SENDREPLY_HANDLE, "_stpSession::_sendReply" )
   INT32 _stpSession::_sendReply( MsgHeader *message )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__STPSESSION__SENDREPLY_HANDLE ) ;

      // send reply via handle
      rc = routeAgent()->syncSend( _netHandle, (MsgHeader *)message ) ;
      PD_RC_CHECK( rc, PDERROR, "Session[%s]: Failed to send reply message, "
                   "rc: %d", sessionName(), rc ) ;

   done:
      PD_TRACE_EXITRC( SDB__STPSESSION__SENDREPLY_HANDLE, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__STPSESSION__REDIRECTPRIMARY, "_stpSession::_redirectPrimary" )
   INT32 _stpSession::_redirectPrimary( MsgHeader *message )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__STPSESSION__REDIRECTPRIMARY ) ;

      rc = _stpCB->getServiceManager()->redirectPrimary( this, message ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to redirect message to primary, "
                   "rc: %d", rc ) ;

   done:
      PD_TRACE_EXITRC( SDB__STPSESSION__REDIRECTPRIMARY, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__STPSESSION__SAVECOMMAND, "_stpSession::_saveCommand" )
   INT32 _stpSession::_saveCommand( stpCommand *command )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__STPSESSION__SAVECOMMAND ) ;

      if ( NULL != _curCommand )
      {
         PD_LOG( PDEVENT, "Command [%s] is expired", _curCommand->getName() ) ;
         stpReleaseCommand( _curCommand ) ;
         _curCommand = NULL ;
      }

      if ( NULL != command )
      {
         PD_LOG( PDEVENT, "Save command [%s] for further processing",
                 command->getName() ) ;
         _curCommand = command ;
      }

      PD_TRACE_EXITRC( SDB__STPSESSION__SAVECOMMAND, rc ) ;

      return rc ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__STPSESSION_POSTMESSAGE, "_stpSession::postMessage" )
   INT32 _stpSession::postMessage( MsgHeader *message )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__STPSESSION_POSTMESSAGE ) ;

      CHAR *messageBuff = NULL ;
      UINT64 userData = PMD_MAKE_SESSION_USERDATA( _netHandle,
                                                   PMD_SESSION_MSG_UNPOOL ) ;

      // allocate post message from thread
      messageBuff = (CHAR *)SDB_THREAD_ALLOC( message->messageLength ) ;
      PD_CHECK( NULL != messageBuff, SDB_OOM, error, PDERROR,
                "Failed to allocate message [size: %d]" ) ;

      // copy message
      ossMemcpy( messageBuff, (void *)message, message->messageLength ) ;

      // try to post message
      try
      {
         _pEDUCB->postEvent( pmdEDUEvent( PMD_EDU_EVENT_MSG,
                                          PMD_EDU_MEM_THREAD,
                                          messageBuff,
                                          userData,
                                          0LL ) ) ;
         messageBuff = NULL ;
      }
      catch ( exception &e )
      {
         PD_LOG( PDERROR, "Failed to post event, occur exception: %s",
                 e.what() ) ;
         rc = SDB_SYS ;
         goto error ;
      }

   done:
      // release message if failed
      if ( NULL != messageBuff )
      {
         SDB_THREAD_FREE( messageBuff ) ;
      }
      PD_TRACE_EXITRC( SDB__STPSESSION_POSTMESSAGE, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   /*
      _stpSessionManager implement
    */
   _stpSessionManager::_stpSessionManager( STPCB *stpCB )
   : _stpCB( stpCB ),
     _curRedReqID( 0LL )
   {
   }

   _stpSessionManager::~_stpSessionManager()
   {
   }

   UINT64 _stpSessionManager::makeSessionID( const NET_HANDLE &handle,
                                             const MsgHeader *header )
   {
      // merge handle and remote TIE into session ID
      return ossPack32To64( PMD_BASE_HANDLE_ID + handle, header->TID ) ;
   }

   SDB_SESSION_TYPE _stpSessionManager::_prepareCreate( UINT64 sessionID,
                                                        INT32 startType,
                                                        INT32 opCode )
   {
      // only create STP session
      return SDB_SESSION_STP ;
   }

   BOOLEAN _stpSessionManager::_canReuse( SDB_SESSION_TYPE sessionType )
   {
      // no need to reuse session
      return FALSE ;
   }

   UINT32 _stpSessionManager::_maxCacheSize() const
   {
      // no need to cache idle session
      return 0 ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__TPSERVICEMANAGER_ONERROR, "_stpSessionManager::onErrorHanding" )
   INT32 _stpSessionManager::onErrorHanding( INT32 rc,
                                             const MsgHeader *request,
                                             const NET_HANDLE &handle,
                                             UINT64 sessionID,
                                             pmdAsyncSession *session )
   {
      INT32 ret = SDB_OK ;

      PD_TRACE_ENTRY( SDB__TPSERVICEMANAGER_ONERROR ) ;

      if ( 0 != sessionID )
      {
         // if a session is assigned, send reply with error
         ret = _reply( handle, rc, request ) ;
      }
      else
      {
         // no session is assigned, no need to reply
         ret = rc ;
      }

      PD_TRACE_EXITRC( SDB__TPSERVICEMANAGER_ONERROR, ret ) ;

      return ret ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__TPSERVICEMANAGER_HANDLEREDRES, "_stpSessionManager::handleRedirectRes" )
   INT32 _stpSessionManager::handleRedirectRes( MsgHeader *message )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__TPSERVICEMANAGER_HANDLEREDRES ) ;

      UINT64 sessionID = 0LL ;
      UINT64 redirectID = makeRedirectID( message->TID,
                                          UINT32( message->requestID ) ) ;
      stpSession *session = NULL ;
      pmdSessionScopedHold scopedHold ;

      // get session ID by redirect ID
      rc = getRedirectSess( redirectID, sessionID ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to get redirected session by "
                   "redirect ID [%llu], rc: %d", redirectID, rc ) ;

      // get session by session ID
      rc = _getSession( sessionID, &session ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to get session by "
                   "session ID [%llu], rc: %d", sessionID, rc ) ;

      scopedHold.setSession( session ) ;

      PD_CHECK( session->getRedirectID() == redirectID,
                SDB_SYS, error, PDERROR,
                "Failed to handle redirect result, redirect ID of "
                "session [%llu] is different, given [%llu], expected [%llu]",
                session->sessionID(), redirectID, session->getRedirectID() ) ;

      // post redirected message to session
      rc = session->postMessage( message ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to post message to session [%llu], "
                   "rc: %d", session->sessionID(), rc ) ;

   done:
      if ( STP_INVALID_REDIRECT_ID != redirectID )
      {
         unregRedirectSess( redirectID ) ;
      }
      PD_TRACE_EXITRC( SDB__TPSERVICEMANAGER_HANDLEREDRES, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__TPSERVICEMANAGER_REGREDSESS, "_stpSessionManager::regRedirectSess" )
   INT32 _stpSessionManager::regRedirectSess( stpSession *session,
                                              MsgHeader *message,
                                              UINT64 &redirectID )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__TPSERVICEMANAGER_REGREDSESS ) ;

      ossScopedLock lock( &_redLatch, EXCLUSIVE ) ;

      UINT64 sessionID = session->sessionID() ;
      UINT32 threadID = session->getTID() ;
      UINT64 requestID = ++ _curRedReqID ;
      STP_SESSION_MAP::iterator iter ;

      redirectID = makeRedirectID( threadID, (UINT32)requestID ) ;
      PD_CHECK( STP_INVALID_REDIRECT_ID != redirectID, SDB_SYS, error, PDERROR,
                "Failed to register redirect session [%llu], "
                "redirect ID is invalid", sessionID ) ;

      iter = _redSessions.find( redirectID ) ;
      PD_CHECK( _redSessions.end() == iter, SDB_SYS, error, PDERROR,
                "Failed to register redirect session [%llu], "
                "redirect ID [%llu] already exists",
                sessionID, redirectID ) ;

      try
      {
         _redSessions[ redirectID ] = sessionID ;
      }
      catch ( exception &e )
      {
         PD_LOG( PDERROR, "Failed to register redirected session, "
                 "redirect ID [%llu], session ID [%llu], occur exception: %s",
                 redirectID, sessionID, e.what() ) ;
         rc = SDB_OOM ;
         goto error ;
      }

      message->requestID = requestID ;
      message->TID = threadID ;
      message->routeID.value = _stpCB->getNodeManager()->getLocalRIDValue() ;

   done:
      PD_TRACE_EXITRC( SDB__TPSERVICEMANAGER_REGREDSESS, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__TPSERVICEMANAGER_UNREGREDSESS, "_stpSessionManager::unregRedirectSess" )
   void _stpSessionManager::unregRedirectSess( UINT64 redirectID )
   {
      PD_TRACE_ENTRY( SDB__TPSERVICEMANAGER_UNREGREDSESS ) ;

      ossScopedLock lock( &_redLatch, EXCLUSIVE ) ;
      _redSessions.erase( redirectID ) ;

      PD_TRACE_EXIT( SDB__TPSERVICEMANAGER_UNREGREDSESS ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__TPSERVICEMANAGER_GETREDSESS, "_stpSessionManager::getRedirectSess" )
   INT32 _stpSessionManager::getRedirectSess( UINT64 redirectID,
                                              UINT64 &sessionID )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__TPSERVICEMANAGER_GETREDSESS ) ;

      ossScopedLock lock( &_redLatch, SHARED ) ;

      sessionID = 0 ;

      STP_SESSION_MAP::iterator iter = _redSessions.find( redirectID ) ;
      PD_CHECK( _redSessions.end() != iter,
                SDB_PMD_SESSION_NOT_EXIST, error, PDERROR,
                "Failed to get redirected session, redirect ID [%llu]",
                redirectID ) ;
      sessionID = iter->second ;

   done:
      PD_TRACE_EXITRC( SDB__TPSERVICEMANAGER_GETREDSESS, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__TPSERVICEMANAGER_MAKEREDID, "_stpSessionManager::makeRedirectID" )
   UINT64 _stpSessionManager::makeRedirectID( UINT32 threadID,
                                              UINT32 requestID )
   {
      UINT64 redirectID = STP_INVALID_REDIRECT_ID ;

      PD_TRACE_ENTRY( SDB__TPSERVICEMANAGER_MAKEREDID ) ;

      // compact thread ID and request ID
      // Note: request ID is unique in each thread, plus thread ID, we could
      //       make a unique ID for redirect message
      redirectID = ossPack32To64( threadID, (UINT32)requestID ) ;

      PD_TRACE_EXIT( SDB__TPSERVICEMANAGER_MAKEREDID ) ;

      return redirectID ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__TPSERVICEMANAGER__CREATESESS, "_stpSessionManager::_createSession" )
   pmdAsyncSession *_stpSessionManager::_createSession(
                                                SDB_SESSION_TYPE sessionType,
                                                INT32 startType,
                                                UINT64 sessionID,
                                                void *data )
   {
      pmdAsyncSession *pSession = NULL ;

      PD_TRACE_ENTRY( SDB__TPSERVICEMANAGER__CREATESESS ) ;

      if ( SDB_SESSION_STP == sessionType )
      {
         // create STP session
         pSession = SDB_OSS_NEW stpSession( sessionID, _stpCB ) ;
      }
      else
      {
         // invalid session type
         PD_LOG( PDERROR, "Invalid session type [%d]", sessionType ) ;
      }

      PD_TRACE_EXIT( SDB__TPSERVICEMANAGER__CREATESESS ) ;

      return pSession ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__TPSERVICEMANAGER__GETSESS, "_stpSessionManager::_getSession" )
   INT32 _stpSessionManager::_getSession( UINT64 sessionID,
                                          stpSession **session )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__TPSERVICEMANAGER__GETSESS ) ;

      SDB_ASSERT( NULL != session, "session is invalid" ) ;

      ossScopedLock lock( &_metaLatch ) ;
      MAPSESSION_IT iterSession = _mapSession.find( sessionID ) ;
      PD_CHECK( iterSession != _mapSession.end(),
                SDB_PMD_SESSION_NOT_EXIST, error, PDERROR,
                "Failed to find session [%llu]", sessionID ) ;

      *session = (stpSession *)( iterSession->second ) ;
      // hold session
      (*session)->holdIn() ;

   done:
      PD_TRACE_EXITRC( SDB__TPSERVICEMANAGER__GETSESS, rc ) ;
      return rc ;

   error:
      goto done ;
   }

}
