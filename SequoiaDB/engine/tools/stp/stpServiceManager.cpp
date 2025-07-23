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

   Source File Name = stpServiceManager.cpp

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
#include "stpServiceManager.hpp"
#include "stpCB.hpp"
#include "pdTrace.hpp"
#include "stpTrace.hpp"
#include "pmd.hpp"
#include "msgMessage.hpp"

using namespace bson ;

namespace engine
{

   /*
      _stpServiceManager implement
    */
   BEGIN_OBJ_MSG_MAP( _stpServiceManager, _stpManagerBase )
      // ON_MSG
      ON_MSG( MSG_AUTH_VERIFY_REQ, processMessage )
      ON_MSG( MSG_BS_QUERY_REQ, processMessage )
      ON_MSG( MSG_BS_QUERY_RES, processMessage )
   END_OBJ_MSG_MAP()

   _stpServiceManager::_stpServiceManager( STPCB *stpCB )
   : stpManagerBase( stpCB ),
     _messageHandler( &_sessionManager ),
     _timeoutHandler( &_sessionManager ),
     _sessionManager( stpCB )
   {
   }

   _stpServiceManager::~_stpServiceManager()
   {
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__STPSERVICEMGR_ONTIMER, "_stpServiceManager::onTimer" )
   void _stpServiceManager::onTimer( UINT64 timerID, UINT32 interval )
   {
      PD_TRACE_ENTRY( SDB__STPSERVICEMGR_ONTIMER ) ;

      // redirect timer to asynchronous session manager
      _sessionManager.onTimer( interval ) ;

      PD_TRACE_EXIT( SDB__STPSERVICEMGR_ONTIMER ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__STPSERVICEMGR_PROCESSMESSAGE, "_stpServiceManager::processMessage" )
   INT32 _stpServiceManager::processMessage( NET_HANDLE handle,
                                            MsgHeader *message )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__STPSERVICEMGR_PROCESSMESSAGE ) ;

      SDB_ASSERT( NULL != message, "message is invalid" ) ;

      switch ( message->opCode )
      {
         case MSG_AUTH_VERIFY_REQ :
         case MSG_BS_QUERY_REQ :
         {
            // redirect message to asynchronous message handler
            rc = _messageHandler.handleMsg( handle,
                                            message,
                                            (const CHAR *)message,
                                            0LL ) ;
            PD_RC_CHECK( rc, PDERROR, "Failed to handle message [%d], rc: %d",
                         message->opCode, rc ) ;
            break ;
         }
         case MSG_BS_QUERY_RES :
         {
            rc = _sessionManager.handleRedirectRes( message ) ;
            PD_RC_CHECK( rc, PDERROR, "Failed to handle message [%d], rc: %d",
                         message->opCode, rc ) ;
            break ;
         }
         default :
         {
            // unknown message
            PD_CHECK( FALSE, SDB_UNKNOWN_MESSAGE, error, PDERROR,
                      "Unknown service message [%d]",
                      message->opCode ) ;
            break ;
         }
      }

   done:
      PD_TRACE_EXITRC( SDB__STPSERVICEMGR_PROCESSMESSAGE, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__STPSERVICEMGR__INITIALIZE, "_stpServiceManager::_initialize" )
   INT32 _stpServiceManager::_initialize()
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__STPSERVICEMGR__INITIALIZE ) ;

      // initialize asynchronous session manager
      rc = _sessionManager.init( _netAgent, &_timeoutHandler, OSS_ONE_SEC ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to initialize session manager, "
                   "rc: %d", rc ) ;

   done:
      PD_TRACE_EXITRC( SDB__STPSERVICEMGR__INITIALIZE, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__STPSERVICEMGR__FINALIZE, "_stpServiceManager::_finalize" )
   INT32 _stpServiceManager::_finalize()
   {
      PD_TRACE_ENTRY( SDB__STPSERVICEMGR__FINALIZE ) ;

      // finalize asynchronous session manager
      _sessionManager.fini() ;

      PD_TRACE_EXITRC( SDB__STPSERVICEMGR__FINALIZE, SDB_OK ) ;

      return SDB_OK ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__STPSERVICEMGR__PREDEACTIVE, "_stpServiceManager::_preDeactivate" )
   INT32 _stpServiceManager::_preDeactivate()
   {
      PD_TRACE_ENTRY( SDB__STPSERVICEMGR__PREDEACTIVE ) ;

      // stop and force asynchronous session manager
      _sessionManager.handleStop() ;
      _sessionManager.setForced() ;

      PD_TRACE_EXITRC( SDB__STPSERVICEMGR__PREDEACTIVE, SDB_OK ) ;

      return SDB_OK ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__STPSERVICEMGR_REDIRECTPRIMARY, "_stpServiceManager::redirectPrimary" )
   INT32 _stpServiceManager::redirectPrimary( stpSession *session,
                                              MsgHeader *message )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__STPSERVICEMGR_REDIRECTPRIMARY ) ;

      SDB_ASSERT( NULL != session, "session is invalid" ) ;
      SDB_ASSERT( NULL != message, "message is invalid" ) ;

      BOOLEAN registered = FALSE ;
      UINT64 redirectID = 0LL ;
      UINT32 lastThreadID = message->TID ;
      UINT64 lastRequestID = message->requestID ;
      MsgRouteID primaryRID ;

      primaryRID.value = MSG_INVALID_ROUTEID ;

      PD_CHECK( STP_INVALID_REDIRECT_ID == session->getRedirectID(),
                SDB_SYS, error, PDERROR,
                "Failed to redirect primary, it is already redirected [%llu]",
                session->getRedirectID() ) ;

      // register redirect session
      rc = _sessionManager.regRedirectSess( session, message, redirectID ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to register redirected session, "
                   "rc: %d", rc ) ;
      registered = TRUE ;

      // get primary
      primaryRID = _nodeManager->getPrimaryRID() ;
      PD_CHECK( MSG_INVALID_ROUTEID != primaryRID.value,
                SDB_CLS_NOT_PRIMARY, error, PDERROR,
                "Failed to get route ID of primary server, "
                "primary is unknown" ) ;

      // avoid recursively redirect
      PD_CHECK( primaryRID.value != message->routeID.value,
                SDB_CLS_NOT_SECONDARY, error, PDERROR,
                "Failed to redirect message, primary is itself now" ) ;

      // save redirect information
      session->setRedirectID( redirectID, lastThreadID, lastRequestID ) ;

      // send message to primary
      rc = _netAgent->syncSend( primaryRID, message ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to send message to primary [%s], "
                   "rc: %d", routeID2String( primaryRID ).c_str(), rc ) ;

      PD_LOG( PDDEBUG, "Redirect message [%s] to primary [%s], "
              "redirect ID [%llu], session ID [%llu]",
              msg2String( message ).c_str(),
              routeID2String( primaryRID ).c_str(),
              redirectID, session->sessionID() ) ;

   done:
      PD_TRACE_EXITRC( SDB__STPSERVICEMGR_REDIRECTPRIMARY, rc ) ;
      return rc ;

   error:
      session->resetRedirectID() ;
      if ( registered )
      {
         _sessionManager.unregRedirectSess( redirectID ) ;
      }
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__STPSERVICEMGR_REDIRECTNODE, "_stpServiceManager::redirectNode" )
   INT32 _stpServiceManager::redirectNode( stpSession *session,
                                           const MsgRouteID &routeID,
                                           MsgHeader *message )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__STPSERVICEMGR_REDIRECTNODE ) ;

      SDB_ASSERT( NULL != session, "session is invalid" ) ;
      SDB_ASSERT( NULL != message, "message is invalid" ) ;

      BOOLEAN registered = FALSE ;
      UINT64 redirectID = 0LL ;
      UINT32 lastThreadID = message->TID ;
      UINT64 lastRequestID = message->requestID ;

      PD_CHECK( STP_INVALID_REDIRECT_ID == session->getRedirectID(),
                SDB_SYS, error, PDERROR,
                "Failed to redirect primary, it is already redirected [%llu]",
                session->getRedirectID() ) ;

      // register redirect session
      rc = _sessionManager.regRedirectSess( session, message, redirectID ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to register redirected session, "
                   "rc: %d", rc ) ;
      registered = TRUE ;

      // save redirect information
      session->setRedirectID( redirectID, lastThreadID, lastRequestID ) ;

      // send message to primary
      rc = _netAgent->syncSend( routeID, message ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to send message to primary [%s], "
                   "rc: %d", routeID2String( routeID ).c_str(), rc ) ;

      PD_LOG( PDDEBUG, "Redirect message [%s] to server [%s], "
              "redirect ID [%llu], session ID [%llu]",
              msg2String( message ).c_str(), routeID2String( routeID ).c_str(),
              redirectID, session->sessionID() ) ;

   done:
      PD_TRACE_EXITRC( SDB__STPSERVICEMGR_REDIRECTNODE, rc ) ;
      return rc ;

   error:
      session->resetRedirectID() ;
      if ( registered )
      {
         _sessionManager.unregRedirectSess( redirectID ) ;
      }
      goto done ;
   }

}
