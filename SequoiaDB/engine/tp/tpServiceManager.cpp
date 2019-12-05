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

   Source File Name = tpServiceManager.cpp

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

#include "tpServiceManager.hpp"
#include "tpCB.hpp"
#include "pdTrace.hpp"
#include "tpTrace.hpp"
#include "pmd.hpp"
#include "msgMessage.hpp"

using namespace bson ;

namespace engine
{

   /*
      _tpServiceManager implement
    */
   BEGIN_OBJ_MSG_MAP( _tpServiceManager, _tpManagerBase )
      // ON_MSG
      ON_MSG( MSG_AUTH_VERIFY_REQ, processMessage )
      ON_MSG( MSG_BS_QUERY_REQ, processMessage )
   END_OBJ_MSG_MAP()

   _tpServiceManager::_tpServiceManager( SDB_TPCB *tpCB )
   : tpManagerBase( tpCB ),
     _messageHandler( &_sessionManager ),
     _timeoutHandler( &_sessionManager ),
     _sessionManager( tpCB )
   {
   }

   _tpServiceManager::~_tpServiceManager()
   {
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__TPSERVICEMGR_ONTIMER, "_tpServiceManager::onTimer" )
   void _tpServiceManager::onTimer( UINT64 timerID, UINT32 interval )
   {
      PD_TRACE_ENTRY( SDB__TPSERVICEMGR_ONTIMER ) ;

      _sessionManager.onTimer( interval ) ;

      PD_TRACE_EXIT( SDB__TPSERVICEMGR_ONTIMER ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__TPSERVICEMGR_PROCESSMESSAGE, "_tpServiceManager::processMessage" )
   INT32 _tpServiceManager::processMessage( NET_HANDLE handle,
                                            MsgHeader *message )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__TPSERVICEMGR_PROCESSMESSAGE ) ;

      SDB_ASSERT( NULL != message, "message is invalid" ) ;

      switch ( message->opCode )
      {
         case MSG_AUTH_VERIFY_REQ :
         case MSG_BS_QUERY_REQ :
         {
            rc = _messageHandler.handleMsg( handle, message,
                                            (const CHAR *)message ) ;
            PD_RC_CHECK( rc, PDERROR, "Failed to handle message [%d], rc: %d",
                         message->opCode, rc ) ;
            break ;
         }
         default :
            PD_CHECK( FALSE, SDB_UNKNOWN_MESSAGE, error, PDERROR,
                      "Unknown service message [%d]",
                      message->opCode ) ;
            break ;
      }

   done:
      PD_TRACE_EXITRC( SDB__TPSERVICEMGR_PROCESSMESSAGE, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__TPSERVICEMGR__INITIALIZE, "_tpServiceManager::_initialize" )
   INT32 _tpServiceManager::_initialize()
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__TPSERVICEMGR__INITIALIZE ) ;

      _sessionManager.init( _netAgent, &_timeoutHandler, OSS_ONE_SEC ) ;

      PD_TRACE_EXITRC( SDB__TPSERVICEMGR__INITIALIZE, rc ) ;

      return rc ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__TPSERVICEMGR__FINALIZE, "_tpServiceManager::_finalize" )
   INT32 _tpServiceManager::_finalize()
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__TPSERVICEMGR__FINALIZE ) ;

      _sessionManager.fini() ;

      PD_TRACE_EXITRC( SDB__TPSERVICEMGR__FINALIZE, rc ) ;

      return rc ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__TPSERVICEMGR__PREDEACTIVE, "_tpServiceManager::_preDeactivate" )
   INT32 _tpServiceManager::_preDeactivate()
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__TPSERVICEMGR__PREDEACTIVE ) ;

      _sessionManager.handleStop() ;
      _sessionManager.setForced() ;

      PD_TRACE_EXITRC( SDB__TPSERVICEMGR__PREDEACTIVE, rc ) ;

      return rc ;
   }

}
