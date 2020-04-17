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

   Source File Name = stpMsgHandler.cpp

   Descriptive Name = Serial Time Protocol Agent

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

#include "stpMsgHandler.hpp"
#include "pdTrace.hpp"
#include "stpTrace.hpp"
#include "stpCB.hpp"
#include "pmd.hpp"
#include "msgMessage.hpp"

namespace engine
{

   /*
      _stpNetMsgHandler implement
    */
   _stpNetMsgHandler::_stpNetMsgHandler( STPCB *stpCB )
   : INetMsgHandler(),
     stpHandlerBase( stpCB ),
     _requestID( 0 )
   {
   }

   _stpNetMsgHandler::~_stpNetMsgHandler()
   {
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__STPNETMSGHANDLER_HANDLEMSG, "_stpNetMsgHandler::handleMsg" )
   INT32 _stpNetMsgHandler::handleMsg( const NET_HANDLE &handle,
                                       const MsgHeader *header,
                                       const CHAR *message,
                                       UINT64 msgUserData )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__STPNETMSGHANDLER_HANDLEMSG ) ;

      // check if it is a system info message
      if ( (UINT32)MSG_SYSTEM_INFO_LEN == (UINT32)( header->messageLength ) )
      {
         // handle system info message
         rc = _handleSysInfo( handle ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to handle sys info request, "
                      "rc: %d", rc ) ;
         goto done ;
      }

      // check operator code of message
      switch ( header->opCode )
      {
         case MSG_BS_QUERY_REQ :
         case MSG_AUTH_VERIFY_REQ :
         {
            // service messages, handle by service manager
            rc = getServiceManager()->handleMessage( handle, header ) ;
            PD_RC_CHECK( rc, PDERROR, "Failed to handle service message [%d], "
                         "rc: %d", header->opCode, rc ) ;
            break ;
         }
         case MSG_STP_SERVER_REQ :
         case MSG_STP_SERVER_RSP :
         {
            // node messages, handle by node manager
            rc = getNodeManager()->handleMessage( handle, header ) ;
            PD_RC_CHECK( rc, PDERROR, "Failed to handle catalog message [%d], "
                         "rc: %d", header->opCode, rc ) ;
            break ;
         }
         case MSG_STP_META_NOTIFY :
         case MSG_STP_META_SYNC_REQ :
         case MSG_STP_META_SYNC_RSP :
         {
            // meta messages, handle by meta manager
            rc = getMetaManager()->handleMessage( handle, header ) ;
            PD_RC_CHECK( rc, PDERROR, "Failed to handle meta data "
                         "message [%d], rc: %d", header->opCode, rc ) ;
            break ;
         }
         case MSG_STP_REG_REQ :
         case MSG_STP_TIME_SYNC_REQ :
         {
            // synchronize requests, handle by synchronize source manager
            rc = getSyncSourceManager()->handleMessage( handle, header ) ;
            PD_RC_CHECK( rc, PDERROR, "Failed to handle synchronize source "
                         "request, rc: %d", rc ) ;
            break ;
         }
         case MSG_STP_REG_RSP :
         case MSG_STP_TIME_SYNC_RSP :
         {
            // synchronize response, handle by synchronize client manager
            rc = getSyncClientManager()->handleMessage(handle, header ) ;
            PD_RC_CHECK( rc, PDERROR, "Failed to handle synchronize client "
                         "message [%d], rc: %d", header->opCode, rc ) ;
            break ;
         }
         case MSG_CLS_BEAT :
         case MSG_CLS_BEAT_RES :
         case MSG_CLS_BALLOT :
         case MSG_CLS_BALLOT_RES :
         {
            // replica messages, handle by replica manager
            rc = getReplManager()->handleMessage( handle, header ) ;
            PD_RC_CHECK( rc, PDERROR, "Failed to handle replica message [%d], "
                         "rc: %d", header->opCode, rc ) ;
            break ;
         }
         case MSG_BS_DISCONNECT :
         {
            // disconnect message, do nothing
            PD_LOG( PDDEBUG, "Disconnect from handle [%u]", handle ) ;
            rc = SDB_OK ;
            break ;
         }
         default :
         {
            // unknown message
            PD_CHECK( FALSE, SDB_UNKNOWN_MESSAGE, error, PDERROR,
                      "Unknown STP message [%d]", header->opCode ) ;
            break ;
         }
      }

   done:
      PD_TRACE_EXITRC( SDB__STPNETMSGHANDLER_HANDLEMSG, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__STPNETMSGHANDLER_HANDLECLOSE, "_stpNetMsgHandler::handleClose" )
   void _stpNetMsgHandler::handleClose( const NET_HANDLE &handle,
                                        MsgRouteID id )
   {
      PD_TRACE_ENTRY( SDB__STPNETMSGHANDLER_HANDLECLOSE ) ;

      // redirect close event to asynchronous message handler
      // NOTE: it will ignore other non-asynchronous sessions
      getServiceManager()->getAsyncMsgHandler()->handleClose( handle, id ) ;

      PD_TRACE_EXIT( SDB__STPNETMSGHANDLER_HANDLECLOSE ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__STPNETMSGHANDLER_ONSENDMSG, "_stpNetMsgHandler::onSendMsg" )
   INT32 _stpNetMsgHandler::onSendMsg( const NET_HANDLE &handle,
                                       const MsgRouteID &id,
                                       MsgHeader *header )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__STPNETMSGHANDLER_ONSENDMSG ) ;

      SDB_ASSERT( NULL != header, "message is invalid" ) ;

      switch ( header->opCode )
      {
         case MSG_STP_TIME_SYNC_REQ :
         {
            // on sending synchronize time request
            getSyncClientManager()->onSendTimeSyncReq(
                                                   (stpTimeSyncReq *)header ) ;
            break ;
         }
         case MSG_STP_TIME_SYNC_RSP :
         {
            // on sending synchronize time response
            getSyncSourceManager()->onSendTimeSyncRsp(
                                                   (stpTimeSyncRsp *)header ) ;
            break ;
         }
         default :
         {
            // do nothing
            break ;
         }
      }

      PD_TRACE_EXITRC( SDB__STPNETMSGHANDLER_ONSENDMSG, rc ) ;

      return rc ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__STPNETMSGHANDLER_ONRECEIVEMSG, "_stpNetMsgHandler::onReceiveMsg" )
   INT32 _stpNetMsgHandler::onReceiveMsg( const NET_HANDLE &handle,
                                          const MsgRouteID &id,
                                          MsgHeader *header,
                                          UINT32 availableSize,
                                          netUserDataHolder *userDataHolder )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__STPNETMSGHANDLER_ONRECEIVEMSG ) ;

      SDB_ASSERT( NULL != header, "message is invalid" ) ;

      switch ( header->opCode )
      {
         case MSG_STP_TIME_SYNC_REQ :
         {
            // on receiving synchronize time request
            getSyncSourceManager()->onReceiveTimeSyncReq(
                                                   (stpTimeSyncReq *)header ) ;
            break ;
         }
         case MSG_STP_TIME_SYNC_RSP :
         {
            // on receiving synchronize time response
            getSyncClientManager()->onReceiveTimeSyncRsp(
                                                   (stpTimeSyncRsp *)header ) ;
            break ;
         }
         default :
         {
            // do nothing
            break ;
         }
      }

      PD_TRACE_EXITRC( SDB__STPNETMSGHANDLER_ONRECEIVEMSG, rc ) ;

      return rc ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__STPNETMSGHANDLER__HANDLESYSINFO, "_stpNetMsgHandler::_handleSysInfo" )
   INT32 _stpNetMsgHandler::_handleSysInfo( const NET_HANDLE &handle )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__STPNETMSGHANDLER__HANDLESYSINFO ) ;

      MsgSysInfoReply reply ;
      MsgSysInfoReply *replyBuffer = &reply ;
      INT32 replySize = sizeof( reply ) ;

      // fill system info reply
      rc = msgBuildSysInfoReply( (CHAR **)( &replyBuffer ), &replySize ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to build system info reply, rc: %d",
                   rc ) ;
      // send system info reply by net agent
      rc = _netAgent->syncSendRaw( handle, (const CHAR *)replyBuffer,
                                   (UINT32)replySize ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to send system info reply, "
                   "rc: %d", rc ) ;

   done:
      PD_TRACE_EXITRC( SDB__STPNETMSGHANDLER__HANDLESYSINFO, rc ) ;
      return rc ;

   error:
      goto done ;
   }


   /*
      _stpPipeMsgHandler implement
    */
   _stpPipeMsgHandler::_stpPipeMsgHandler( STPCB *stpCB )
   : IPmdPipeHandler(),
     stpHandlerBase( stpCB )
   {
   }

   _stpPipeMsgHandler::~_stpPipeMsgHandler()
   {
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__STPPIPEMSGHANDLER_HANDLEMSG, "_stpPipeMsgHandler::processMessage" )
   INT32 _stpPipeMsgHandler::processMessage( CHAR *message,
                                             utilNodePipe &nodePipe )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__STPPIPEMSGHANDLER_HANDLEMSG ) ;

      PD_LOG( PDEVENT, "STP message [%s] is received", message ) ;

      if ( 0 == ossStrcmp( message, STP_PIPE_MSG_TEST ) )
      {
         // test message
         INT8 test = 1 ;
         // write response back to sender
         rc = nodePipe.writePipe( (CHAR *)( &test ), sizeof( test ) ) ;
      }
      else if ( 0 == ossStrcmp( message, STP_PIPE_MSG_SYNC ) )
      {
         // got synchronize message
         // signal to synchronize time
         getSyncClientManager()->signalSync() ;
         // no need to write response back to sender
      }
      else
      {
         // unknown message, throw to caller
         rc = SDB_UNKNOWN_MESSAGE ;
      }

      PD_TRACE_EXITRC( SDB__STPPIPEMSGHANDLER_HANDLEMSG, rc ) ;

      return rc ;
   }

}
