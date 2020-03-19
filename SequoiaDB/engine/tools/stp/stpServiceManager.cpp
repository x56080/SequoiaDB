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
                                            NULL ) ;
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

}
