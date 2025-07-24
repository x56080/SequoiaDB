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
      _stpNetMsgHandlerBase implement
    */
   _stpNetMsgHandlerBase::_stpNetMsgHandlerBase( STPCB *stpCB )
   : INetMsgHandler(),
     stpHandlerBase( stpCB ),
     _requestID( 0 )
   {
   }

   _stpNetMsgHandlerBase::~_stpNetMsgHandlerBase()
   {
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__STPNETMSGHANDLERBASE_FILLREQHEADER, "_stpNetMsgHandlerBase::fillRequestHeader" )
   void _stpNetMsgHandlerBase::fillRequestHeader( MsgHeader &request,
                                                  UINT32 requestSize,
                                                  INT32 opCode )
   {
      PD_TRACE_ENTRY( SDB__STPNETMSGHANDLERBASE_FILLREQHEADER ) ;

      // get route ID of local node
      MsgRouteID localRID = getNodeManager()->getLocalRID() ;

      // fill fields of request
      request.messageLength = requestSize ;
      request.opCode = opCode ;
      request.TID = 0 ;
      request.routeID.value = localRID.value ;

      // allocate request ID
      request.requestID = allocateRequestID() ;

      PD_TRACE_EXIT( SDB__STPNETMSGHANDLERBASE_FILLREQHEADER ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__STPNETMSGHANDLERBASE_FILLREPHEADER, "_stpNetMsgHandlerBase::fillReplyHeader" )
   void _stpNetMsgHandlerBase::fillReplyHeader( const MsgHeader &request,
                                                MsgOpReply &reply,
                                                UINT32 replySize,
                                                INT32 returnCode,
                                                BOOLEAN needRouteID )
   {
      PD_TRACE_ENTRY( SDB__STPNETMSGHANDLERBASE_FILLREPHEADER ) ;

      // get route ID of local node
      MsgRouteID localRID ;

      if ( needRouteID )
      {
         localRID.value = getNodeManager()->getLocalRIDValue() ;
      }
      else
      {
         localRID.value = MSG_INVALID_ROUTEID ;
      }

      // fill fields of reply
      reply.header.messageLength = replySize ;
      reply.header.opCode = MAKE_REPLY_TYPE( request.opCode ) ;
      reply.header.TID = 0 ;
      reply.header.routeID.value = localRID.value ;
      reply.header.requestID = request.requestID ;
      reply.contextID = -1 ;
      reply.flags = returnCode ;
      reply.startFrom = 0 ;
      reply.numReturned = 1 ;

      PD_TRACE_EXIT( SDB__STPNETMSGHANDLERBASE_FILLREPHEADER ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__STPNETMSGHANDLERBASE_FILLREPHEADER_INT, "_stpNetMsgHandlerBase::fillReplyHeader" )
   void _stpNetMsgHandlerBase::fillReplyHeader( const MsgHeader &request,
                                                MsgInternalReplyHeader &reply,
                                                UINT32 replySize,
                                                INT32 returnCode )
   {
      PD_TRACE_ENTRY( SDB__STPNETMSGHANDLERBASE_FILLREPHEADER_INT ) ;

      MsgRouteID localRID ;

      // get route ID of local node
      localRID.value = getNodeManager()->getLocalRIDValue() ;

      // fill fields of reply
      reply.header.messageLength = replySize ;
      reply.header.opCode = MAKE_REPLY_TYPE( request.opCode ) ;
      reply.header.TID = 0 ;
      reply.header.routeID.value = localRID.value ;
      reply.header.requestID = request.requestID ;
      reply.res = returnCode ;

      PD_TRACE_EXIT( SDB__STPNETMSGHANDLERBASE_FILLREPHEADER_INT ) ;
   }

   /*
      _stpSyncSourceMsgHandler implement
    */
   _stpSyncSourceMsgHandler::_stpSyncSourceMsgHandler( STPCB *stpCB,
                                                       stpSyncSource *source )
   : stpNetMsgHandlerBase( stpCB ),
     _source( source )
   {
   }

   _stpSyncSourceMsgHandler::~_stpSyncSourceMsgHandler()
   {
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__STPSYNCSRCMSGHANDLER_HANDLEMSG, "_stpSyncSourceMsgHandler::handleMsg" )
   INT32 _stpSyncSourceMsgHandler::handleMsg( const NET_HANDLE &handle,
                                              const MsgHeader *header,
                                              const CHAR *message,
                                              UINT64 msgUserData )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__STPSYNCSRCMSGHANDLER_HANDLEMSG ) ;

      SDB_ASSERT( NULL != header, "message is invalid" ) ;

      // check operator code of message
      switch ( header->opCode )
      {
         case MSG_STP_TIME_SYNC_REQ :
         {
            if ( NULL != _source )
            {
               rc = _source->handleTimeSyncReq(
                                    handle, (const stpTimeSyncReq *)header) ;
               PD_RC_CHECK( rc, PDERROR, "Failed to handle time synchronize "
                            "request with source, rc: %d", rc ) ;
            }
            else
            {
               // synchronize requests, handle by synchronize source manager
               rc = getSyncSourceManager()->
                     handleTimeSyncReq( handle,
                                        (const stpTimeSyncReq *)header ) ;
               PD_RC_CHECK( rc, PDERROR, "Failed to handle time synchronize "
                            "request, rc: %d", rc ) ;
            }
            break ;
         }
         default :
         {
            // unknown message
            PD_CHECK( FALSE, SDB_UNKNOWN_MESSAGE, error, PDERROR,
                      "Unknown STP synchronize message [%d]",
                      header->opCode ) ;
            break ;
         }
      }

   done:
      PD_TRACE_EXITRC( SDB__STPSYNCSRCMSGHANDLER_HANDLEMSG, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__STPSYNCSRCMSGHANDLER_ONSENDMSG, "_stpSyncSourceMsgHandler::onSendMsg" )
   INT32 _stpSyncSourceMsgHandler::onSendMsg( const NET_HANDLE &handle,
                                              const MsgRouteID &id,
                                              MsgHeader *header )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__STPSYNCSRCMSGHANDLER_ONSENDMSG ) ;

      SDB_ASSERT( NULL != header, "message is invalid" ) ;

      switch ( header->opCode )
      {
         case MSG_STP_TIME_SYNC_RSP :
         {
            // on sending synchronize time response
            getSyncSourceManager()->
                        onSendTimeSyncRsp( (stpTimeSyncRsp *)header ) ;
            break ;
         }
         default :
         {
            // do nothing
            break ;
         }
      }

      PD_TRACE_EXITRC( SDB__STPSYNCSRCMSGHANDLER_ONSENDMSG, rc ) ;

      return rc ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__STPSYNCSRCNETMSGHANDLER_ONRECEIVEMSG, "_stpSyncSourceMsgHandler::onReceiveMsg" )
   INT32 _stpSyncSourceMsgHandler::onReceiveMsg( const NET_HANDLE &handle,
                                                 const MsgRouteID &id,
                                                 MsgHeader *header,
                                                 UINT32 availableSize,
                                                 netUserDataHolder *userDataHolder )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__STPSYNCSRCNETMSGHANDLER_ONRECEIVEMSG ) ;

      SDB_ASSERT( NULL != header, "message is invalid" ) ;

      switch ( header->opCode )
      {
         case MSG_STP_TIME_SYNC_REQ :
         {
            // on receiving synchronize time request
            getSyncSourceManager()->
                        onReceiveTimeSyncReq( (stpTimeSyncReq *)header ) ;
            break ;
         }
         default :
         {
            // do nothing
            break ;
         }
      }

      PD_TRACE_EXITRC( SDB__STPSYNCSRCNETMSGHANDLER_ONRECEIVEMSG, rc ) ;

      return rc ;
   }

   /*
      _stpNetMsgHandler implement
    */
   _stpNetMsgHandler::_stpNetMsgHandler( STPCB *stpCB )
   : stpNetMsgHandlerBase( stpCB )
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

      SDB_ASSERT( NULL != header, "message is invalid" ) ;

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
         case MSG_BS_QUERY_RES :
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
            getSyncClientManager()->
                        onSendTimeSyncReq( (stpTimeSyncReq *)header ) ;
            break ;
         }
         case MSG_STP_TIME_SYNC_RSP :
         {
            // on sending synchronize time response
            getSyncSourceManager()->
                        onSendTimeSyncRsp( (stpTimeSyncRsp *)header ) ;
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
            getSyncSourceManager()->
                        onReceiveTimeSyncReq( (stpTimeSyncReq *)header ) ;
            break ;
         }
         case MSG_STP_TIME_SYNC_RSP :
         {
            // on receiving synchronize time response
            getSyncClientManager()->
                        onReceiveTimeSyncRsp( (stpTimeSyncRsp *)header ) ;
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
      rc = msgBuildSysInfoReply( (CHAR **)( &replyBuffer ), &replySize,
                                 pmdGetStartTime() ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to build system info reply, rc: %d",
                   rc ) ;
      // send system info reply by net agent
      rc = _netAgent->syncSendRaw( handle,
                                   (const CHAR *)replyBuffer,
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
