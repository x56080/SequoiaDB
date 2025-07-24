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

   Source File Name = pmdRemoteMsgEventHandler.cpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          05/03/2017  XJH Initial Draft

   Last Changed =

*******************************************************************************/
#include "pmdDef.hpp"
#include "pmdRemoteMsgEventHandler.hpp"
#include "pmdEDU.hpp"
#include "pmdRemoteSession.hpp"
#include "msgMessageFormat.hpp"
#include "pdTrace.hpp"
#include "pmdTrace.hpp"
#include "msgMessage.hpp"
<<<<<<< HEAD
=======
#include "stpAgent.hpp"
>>>>>>> c4064a6f2c2dfdf2b1bf049c2f904b74db0494b2
#include "pmdEnv.hpp"

namespace engine
{

   /*
      _pmdRemoteMsgHandler implement
   */
   _pmdRemoteMsgHandler::_pmdRemoteMsgHandler( _pmdRemoteSessionMgr *pRSManager )
   {
      _pRSManager       = pRSManager ;
      _pMainCB          = NULL ;
   }

   _pmdRemoteMsgHandler::~_pmdRemoteMsgHandler()
   {
      _pRSManager       = NULL ;
   }

   void _pmdRemoteMsgHandler::attach( _pmdEDUCB * cb )
   {
      _pMainCB    = cb ;
   }

   void _pmdRemoteMsgHandler::detach()
   {
      _pMainCB    = NULL ;
   }

   INT32 _pmdRemoteMsgHandler::handleMsg( const NET_HANDLE &handle,
                                          const _MsgHeader *header,
                                          const CHAR *msg,
                                          UINT64 msgUserData )
   {
      INT32 rc = SDB_OK ;

      // sys info request
      if ( (UINT32)MSG_SYSTEM_INFO_LEN == (UINT32)header->messageLength )
      {
         MsgSysInfoReply reply ;
         MsgSysInfoReply *pReply = &reply ;
         INT32 replySize = sizeof(reply) ;

         rc = msgBuildSysInfoReply ( (CHAR**)&pReply, &replySize,
                                     pmdGetStartTime() ) ;
         if ( rc )
         {
            PD_LOG( PDERROR, "Failed to build sys info reply, rc: %d", rc ) ;
            rc = SDB_NET_BROKEN_MSG ;
            goto error ;
         }
         else
         {
            rc = _pRSManager->getAgent()->syncSendRaw( handle,
                                                       (const CHAR *)pReply,
                                                       (UINT32)replySize ) ;
            goto done ;
         }
      }

      // main cb msg
      if ( 0 == header->TID || ! IS_REPLY_TYPE( header->opCode ) )
      {
         rc = _postMsg( handle, header, msg ) ;
      }
      // session msg
      else
      {
         SDB_ASSERT( _pRSManager, "Remote Session Manager can't be NULL" ) ;
         rc = _pRSManager->pushMessage( handle, header ) ;
         if ( rc )
         {
            PD_LOG( ( ( SDB_INVALIDARG == rc ) ? PDWARNING : PDERROR ),
                    "Push message[%s] failed, rc: %d",
                    msg2String( header, MSG_MASK_ALL, 0 ).c_str(), rc ) ;
            rc = rc == SDB_INVALIDARG ? SDB_INVALIDARG : SDB_NET_BROKEN_MSG ;
            goto error ;
         }
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   void _pmdRemoteMsgHandler::handleClose( const NET_HANDLE &handle,
                                           _MsgRouteID id )
   {
      SDB_ASSERT( _pRSManager, "Remote session manager can't be NULL" ) ;
      _pRSManager->handleClose( handle, id ) ;

      MsgOpReply msg ;
      msg.contextID = -1 ;
      msg.flags = SDB_NETWORK_CLOSE ;
      msg.header.messageLength = sizeof( MsgOpReply ) ;
      msg.header.opCode = MSG_COM_REMOTE_DISC ;
      msg.header.requestID = 0 ;
      msg.header.routeID.value = id.value ;
      msg.header.TID = 0 ;
      msg.numReturned = 0 ;
      msg.startFrom = 0 ;
      msg.returnMask = 0 ;

      _postMsg( handle, (_MsgHeader *)&msg ) ;
      PD_LOG ( PDDEBUG, "posting event handle close %u", (UINT32)handle ) ;
   }

   INT32 _pmdRemoteMsgHandler::handleConnect( const NET_HANDLE &handle,
                                              _MsgRouteID id,
<<<<<<< HEAD
                                              BOOLEAN isPositive )
=======
                                              BOOLEAN isPositive,
                                              netUserDataHolder *userDataHolder )
>>>>>>> c4064a6f2c2dfdf2b1bf049c2f904b74db0494b2
   {
      SDB_ASSERT( _pRSManager, "Remote session manager can't be NULL" ) ;

      _pRSManager->handleConnect( handle, id, isPositive ) ;
      return SDB_OK ;
<<<<<<< HEAD
=======
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__PMDRMTMSGHDL_ONSENDMSG, "_pmdRemoteMsgHandler::onSendMsg" )
   INT32 _pmdRemoteMsgHandler::onSendMsg( const NET_HANDLE &handle,
                                          const MsgRouteID &id,
                                          MsgHeader *header )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__PMDRMTMSGHDL_ONSENDMSG ) ;

      if ( IS_GLOBTIME_TYPE( header->opCode ) )
      {
         stpAgent agent ;
         stpLogicalTimeUS currentTime ;

         rc = agent.getLogicalTimeUS( currentTime, 1, FALSE ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to get global logical time for "
                      "message %s to node %s via handle %u, rc: %d",
                      msg2String( header, MSG_MASK_ALL, 0 ).c_str(),
                      routeID2String( id ).c_str(), handle, rc ) ;

         _setSendTime( header, currentTime ) ;
      }

   done:
      PD_TRACE_EXITRC( SDB__PMDRMTMSGHDL_ONSENDMSG, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__PMDRMTMSGHDL__SETSENDTIME, "_pmdRemoteMsgHandler::_setSendTime" )
   BOOLEAN _pmdRemoteMsgHandler::_setSendTime( MsgHeader *header,
                                               const stpLogicalTimeUS &sendTime )
   {
      BOOLEAN setTime = FALSE ;

      PD_TRACE_ENTRY( SDB__PMDRMTMSGHDL__SETSENDTIME ) ;

      switch ( GET_REQUEST_TYPE( header->opCode ) )
      {
         case MSG_PACKET :
         {
            // for packet message, we need to iterate all messages to find
            // position to look for transaction begin or transaction pre-commit
            // messages to set send time
            // transaction messages will be packeted in below cases
            // - session init -> transaction begin -> transaction operation
            // - transaction begin -> transaction operation
            // - transaction pre-commit -> transaction commit
            // so we need to iterate one or two messages to set the send time
            // NOTE: transaction begin -> transaction operation -> commit
            //       will be send as one transaction operation message, DATA
            //       node will handle as auto-commit transaction
            INT32 pos = sizeof( MsgPacketReq ) ;
            while ( pos < header->messageLength )
            {
               MsgHeader *tmpMsg = (MsgHeader *)( ( CHAR *)header + pos ) ;

               setTime = _setSendTime( tmpMsg, sendTime ) ;
               if ( setTime )
               {
                  // set done, break loop
                  break ;
               }

               pos += tmpMsg->messageLength ;
            }
            break ;
         }
         case MSG_BS_TRANS_BEGIN_REQ :
         {
            MsgOpTransBegin *request = (MsgOpTransBegin *)header ;
            request->sendTime = sendTime.getTime() ;
            setTime = TRUE ;
            break ;
         }
         case MSG_BS_TRANS_COMMITPRE_REQ :
         {
            MsgOpTransCommitPre *request = (MsgOpTransCommitPre *)header ;
            MSG_TRANS_COMMIT_PRE_SET_SEND_TIME( request, sendTime.getTime() ) ;
            setTime = TRUE ;
            break ;
         }
         default :
         {
            break ;
         }
      }

      PD_TRACE_EXIT( SDB__PMDRMTMSGHDL__SETSENDTIME ) ;

      return setTime ;
>>>>>>> c4064a6f2c2dfdf2b1bf049c2f904b74db0494b2
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__PMDRMTMSGHDL__POSTMSG, "_pmdRemoteMsgHandler::_postMsg" )
   INT32 _pmdRemoteMsgHandler::_postMsg( const NET_HANDLE &handle,
                                         const MsgHeader *header,
                                         const CHAR *msg )
   {
      PD_TRACE_ENTRY ( SDB__PMDRMTMSGHDL__POSTMSG );

      CHAR *pNewMsg = NULL ;
      INT32 rc = SDB_OK ;

      SDB_ASSERT( _pMainCB, "Main cb can't be NULL" ) ;
      if ( !_pMainCB )
      {
         PD_LOG( PDERROR, "Main cb handler is null when recv "
                 "msg[opCode:%d]", header->opCode ) ;
         rc = SDB_SYS ;
         goto error ;
      }

      pNewMsg = (CHAR*)SDB_THREAD_ALLOC( header->messageLength + 1 ) ;
      if ( !pNewMsg )
      {
         rc = SDB_OOM ;
         PD_LOG( PDERROR, "Failed to alloc memory for msg[opCode: %d, "
                 "len: %d], rc: %d", header->opCode, header->messageLength,
                 rc ) ;
         goto error ;
      }

      // copy msg
      if ( NULL == msg )
      {
         ossMemcpy( (void *)pNewMsg, header, header->messageLength ) ;
      }
      else
      {
         ossMemcpy( pNewMsg, msg, header->messageLength ) ;
      }
      pNewMsg[ header->messageLength ] = 0 ;

      // push event
      _pMainCB->postEvent( pmdEDUEvent( PMD_EDU_EVENT_MSG,
                                        PMD_EDU_MEM_THREAD,
                                        pNewMsg,
                                        (UINT64)handle ) ) ;
   done:
      PD_TRACE_EXITRC ( SDB__PMDRMTMSGHDL__POSTMSG, rc );
      return rc ;
   error:
      goto done ;
   }

   /*
      _pmdRemoteTimerHandler implement
   */
   _pmdRemoteTimerHandler::_pmdRemoteTimerHandler()
   {
      _pMainCB       = NULL ;
   }

   _pmdRemoteTimerHandler::~_pmdRemoteTimerHandler()
   {
   }

   void _pmdRemoteTimerHandler::attach( _pmdEDUCB * cb )
   {
      _pMainCB       = cb ;
   }

   void _pmdRemoteTimerHandler::detach()
   {
      _pMainCB       = NULL ;
   }

   void _pmdRemoteTimerHandler::handleTimeout( const UINT32 &millisec,
                                               const UINT32 &id )
   {
      if ( !_pMainCB )
      {
         return ;
      }
      PMD_EVENT_MESSAGES *eventMsg = (PMD_EVENT_MESSAGES *)
         SDB_THREAD_ALLOC( sizeof (PMD_EVENT_MESSAGES ) ) ;

      if ( NULL == eventMsg )
      {
         PD_LOG ( PDERROR, "Failed to allocate memory for PDM timeout Event" ) ;
      }
      else
      {
         ossTimestamp ts ;
         ossGetCurrentTime( ts ) ;

         eventMsg->timeoutMsg.interval = millisec ;
         eventMsg->timeoutMsg.occurTime = ts.time ;
         eventMsg->timeoutMsg.timerID = id ;

         _pMainCB->postEvent( pmdEDUEvent ( PMD_EDU_EVENT_TIMEOUT,
                                            PMD_EDU_MEM_THREAD,
                                            (void*)eventMsg) ) ;
      }
   }

}


