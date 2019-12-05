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

   Source File Name = tpMsgHandler.cpp

   Descriptive Name = SequoiaDB Time Protocol Service Agent

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

#include "tpMsgHandler.hpp"
#include "pdTrace.hpp"
#include "tpTrace.hpp"
#include "tpCB.hpp"
#include "pmd.hpp"
#include "msgMessage.hpp"

namespace engine
{

   /*
      _tpNetMsgHandler implement
    */
   _tpNetMsgHandler::_tpNetMsgHandler( SDB_TPCB *tpCB )
   : INetUDPMsgHandler(),
     tpMsgHandler( tpCB ),
     _requestID( 0 )
   {
   }

   _tpNetMsgHandler::~_tpNetMsgHandler()
   {
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__TPNETMSGHANDLER_HANDLEMSG, "_tpNetMsgHandler::handleMsg" )
   INT32 _tpNetMsgHandler::handleMsg( const NET_HANDLE &handle,
                                      const MsgHeader *header,
                                      const CHAR *message )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__TPNETMSGHANDLER_HANDLEMSG ) ;

      if ( (UINT32)MSG_SYSTEM_INFO_LEN == (UINT32)( header->messageLength ) )
      {
         rc = _handleSysInfo( handle ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to handle sys info request, "
                      "rc: %d", rc ) ;
         goto done ;
      }

      switch ( header->opCode )
      {
         case MSG_BS_QUERY_REQ :
         case MSG_AUTH_VERIFY_REQ :
         {
            rc = _serviceManager->handleMessage( handle, header ) ;
            PD_RC_CHECK( rc, PDERROR, "Failed to handle service message [%d], "
                         "rc: %d", header->opCode, rc ) ;
            break ;
         }
         case MSG_TP_SERVER_REQ :
         case MSG_TP_SERVER_RSP :
         {
            rc = _catalogManager->handleMessage( handle, header ) ;
            PD_RC_CHECK( rc, PDERROR, "Failed to handle catalog message [%d], "
                         "rc: %d", header->opCode, rc ) ;
            break ;
         }
         case MSG_TP_META_NOTIFY :
         case MSG_TP_META_SYNC_REQ :
         case MSG_TP_META_SYNC_RSP :
         {
            rc = _metaManager->handleMessage( handle, header ) ;
            PD_RC_CHECK( rc, PDERROR, "Failed to handle meta data "
                         "message [%d], rc: %d", header->opCode, rc ) ;
            break ;
         }
         case MSG_TP_REG_REQ :
         case MSG_TP_TIME_SYNC_REQ :
         {
            rc = _sourceManager->handleMessage( handle, header ) ;
            PD_RC_CHECK( rc, PDERROR, "Failed to handle synchronize source "
                         "request, rc: %d", rc ) ;
            break ;
         }
         case MSG_TP_REG_RSP :
         case MSG_TP_TIME_SYNC_RSP :
         {
            rc = _syncManager->handleMessage(handle, header ) ;
            PD_RC_CHECK( rc, PDERROR, "Failed to handle synchronize client "
                         "message [%d], rc: %d", header->opCode, rc ) ;
            break ;
         }
         case MSG_CLS_BEAT :
         case MSG_CLS_BEAT_RES :
         case MSG_CLS_BALLOT :
         case MSG_CLS_BALLOT_RES :
         {
            rc = _replManager->handleMessage( handle, header ) ;
            PD_RC_CHECK( rc, PDERROR, "Failed to handle replica message [%d], "
                         "rc: %d", header->opCode, rc ) ;
            break ;
         }
         case MSG_BS_DISCONNECT :
         {
            PD_LOG( PDDEBUG, "Disconnect from handle [%u]", handle ) ;
            rc = SDB_OK ;
            break ;
         }
         default :
         {
            PD_CHECK( FALSE, SDB_UNKNOWN_MESSAGE, error, PDERROR,
                      "Unknown TP message [%d]", header->opCode ) ;
            break ;
         }
      }

   done:
      PD_TRACE_EXITRC( SDB__TPNETMSGHANDLER_HANDLEMSG, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__TPNETMSGHANDLER_HANDLECLOSE, "_tpNetMsgHandler::handleClose" )
   void _tpNetMsgHandler::handleClose( const NET_HANDLE &handle,
                                       MsgRouteID id )
   {
      PD_TRACE_ENTRY( SDB__TPNETMSGHANDLER_HANDLECLOSE ) ;

      _serviceManager->getAsyncMsgHandler()->handleClose( handle, id ) ;

      PD_TRACE_EXIT( SDB__TPNETMSGHANDLER_HANDLECLOSE ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__TPNETMSGHANDLER_ONSENDMSG, "_tpNetMsgHandler::onSendMsg" )
   void _tpNetMsgHandler::onSendMsg( const NET_HANDLE &handle,
                                     MsgRouteID id,
                                     MsgHeader *header )
   {
      PD_TRACE_ENTRY( SDB__TPNETMSGHANDLER_ONSENDMSG ) ;

      SDB_ASSERT( NULL != header, "message is invalid" ) ;

      switch ( header->opCode )
      {
         case MSG_TP_TIME_SYNC_REQ :
         {
            _syncManager->onSendTimeSyncReq( (MsgTpTimeSyncReq *)header ) ;
            break ;
         }
         case MSG_TP_TIME_SYNC_RSP :
         {
            _sourceManager->onSendTimeSyncRes( (MsgTpTimeSyncRsp *)header ) ;
            break ;
         }
         default :
         {
            // do nothing
            break ;
         }
      }

      PD_TRACE_EXIT( SDB__TPNETMSGHANDLER_ONSENDMSG ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__TPNETMSGHANDLER_ONRECEIVEMSG, "_tpNetMsgHandler::onReceiveMsg" )
   void _tpNetMsgHandler::onReceiveMsg( const NET_HANDLE &handle,
                                        MsgRouteID id,
                                        MsgHeader *header )
   {
      PD_TRACE_ENTRY( SDB__TPNETMSGHANDLER_ONRECEIVEMSG ) ;

      SDB_ASSERT( NULL != header, "message is invalid" ) ;

      switch ( header->opCode )
      {
         case MSG_TP_TIME_SYNC_REQ :
         {
            _sourceManager->onReceiveTimeSyncReq( (MsgTpTimeSyncReq *)header ) ;
            break ;
         }
         case MSG_TP_TIME_SYNC_RSP :
         {
            _syncManager->onReceiveTimeSyncRsp( (MsgTpTimeSyncRsp *)header ) ;
            break ;
         }
         default :
         {
            // do nothing
            break ;
         }
      }

      PD_TRACE_EXIT( SDB__TPNETMSGHANDLER_ONRECEIVEMSG ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__TPNETMSGHANDLER__HANDLESYSINFO, "_tpNetMsgHandler::_handleSysInfo" )
   INT32 _tpNetMsgHandler::_handleSysInfo( const NET_HANDLE &handle )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__TPNETMSGHANDLER__HANDLESYSINFO ) ;

      MsgSysInfoReply reply ;
      MsgSysInfoReply *replyBuffer = &reply ;
      INT32 replySize = sizeof( reply ) ;

      rc = msgBuildSysInfoReply( (CHAR **)( &replyBuffer ), &replySize ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to build sys info reply, rc: %d",
                   rc ) ;
      rc = _netAgent->syncSendRaw( handle, (const CHAR *)replyBuffer,
                                   (UINT32)replySize ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to send sys info reply, "
                   "rc: %d", rc ) ;

   done:
      PD_TRACE_EXITRC( SDB__TPNETMSGHANDLER__HANDLESYSINFO, rc ) ;
      return rc ;

   error:
      goto done ;
   }


   /*
      _tpPipeMsgHandler implement
    */
   _tpPipeMsgHandler::_tpPipeMsgHandler( SDB_TPCB *tpCB )
   : IPmdPipeHandler(),
     tpMsgHandler( tpCB )
   {
   }

   _tpPipeMsgHandler::~_tpPipeMsgHandler()
   {
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__TPPIPEMSGHANDLER_HANDLEMSG, "_tpPipeMsgHandler::processMessage" )
   INT32 _tpPipeMsgHandler::processMessage( CHAR *message,
                                            utilNodePipe &nodePipe )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__TPPIPEMSGHANDLER_HANDLEMSG ) ;

      if ( 0 == ossStrcmp( message, TP_PIPE_MSG_TEST ) )
      {
         PD_LOG( PDEVENT, "TP message [%s] is received", message ) ;
         INT8 test = 1 ;
         rc = nodePipe.writePipe( (CHAR *)( &test ), sizeof( test ) ) ;
      }
      else if ( 0 == ossStrcmp( message, TP_PIPE_MSG_SYNC ) )
      {
         PD_LOG( PDEVENT, "TP message [%s] is received", message ) ;
         INT8 test = 1 ;
         _syncManager->signalSync() ;
         rc = nodePipe.writePipe( (CHAR *)( &test ), sizeof( test ) ) ;
      }
      else
      {
         rc = SDB_UNKNOWN_MESSAGE ;
      }

      PD_TRACE_EXITRC( SDB__TPPIPEMSGHANDLER_HANDLEMSG, rc ) ;

      return rc ;
   }

}
