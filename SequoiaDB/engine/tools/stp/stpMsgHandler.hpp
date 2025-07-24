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

   Source File Name = stpMsgHandler.hpp

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
#ifndef STP_MSG_HANDLER_HPP__
#define STP_MSG_HANDLER_HPP__

#include "stpCBCommon.hpp"
#include "stpModule.hpp"
#include "stpMsg.hpp"
#include "netMsgHandler.hpp"
#include "netRouteAgent.hpp"
#include "pmdPipeManager.hpp"

namespace engine
{

   class _stpNetMsgHandlerBase : public INetMsgHandler,
                                 public stpHandlerBase
   {
   public:
      _stpNetMsgHandlerBase( STPCB *stpCB ) ;
      virtual ~_stpNetMsgHandlerBase() ;

   public:
      // allocate request ID
      OSS_INLINE UINT64 allocateRequestID()
      {
         return _requestID.inc() ;
      }

      // helper function to fill request
      void fillRequestHeader( MsgHeader &request,
                              UINT32 requestSize,
                              INT32 opCode ) ;

      // helper function to fill reply
      void fillReplyHeader( const MsgHeader &request,
                             MsgOpReply &reply,
                             UINT32 replySize,
                             INT32 returnCode,
                             BOOLEAN needRouteID ) ;

      // helper function to fill internal reply
      void fillReplyHeader( const MsgHeader &request,
                            MsgInternalReplyHeader &reply,
                            UINT32 replySize,
                            INT32 returnCode ) ;

   protected:
      // current request ID ( used to allocate new request ID )
      ossAtomic64 _requestID ;
   } ;

   /*
      _stpSyncSourceMsgHandler define
    */
   // _stpSyncSourceMsgHandler handles time synchronize messages
   class _stpSyncSourceMsgHandler : public stpNetMsgHandlerBase
   {
   public:
      // construct and destructor
      _stpSyncSourceMsgHandler( STPCB *stpCB, stpSyncSource *source ) ;
      virtual ~_stpSyncSourceMsgHandler() ;

   public:
      // override functions of message handler
      // handle message
      virtual INT32 handleMsg( const NET_HANDLE &handle,
                               const MsgHeader *header,
                               const CHAR *message,
                               UINT64 msgUserData ) ;

      // handle event on sending message ( via UDP )
      virtual INT32 onSendMsg( const NET_HANDLE &handle,
                               const MsgRouteID &id,
                               MsgHeader *header ) ;
      // handle event on receiving message ( via UDP )
      virtual INT32 onReceiveMsg( const NET_HANDLE &handle,
                                  const MsgRouteID &id,
                                  MsgHeader *header,
                                  UINT32 availableSize,
                                  netUserDataHolder *userDataHolder ) ;

   protected:
      stpSyncSource * _source ;
   } ;

   /*
      _stpNetMsgHandler define
    */
   class _stpNetMsgHandler : public stpNetMsgHandlerBase
   {
   public:
      // construct and destructor
      _stpNetMsgHandler( STPCB *stpCB ) ;
      virtual ~_stpNetMsgHandler() ;

   public:
      // override functions of message handler
      // handle message
      virtual INT32 handleMsg( const NET_HANDLE &handle,
                               const MsgHeader *header,
                               const CHAR *message,
                               UINT64 msgUserData ) ;
      // handle close message ( disconnect )
      virtual void handleClose( const NET_HANDLE &handle,
                                MsgRouteID id ) ;

      // handle event on sending message ( via UDP )
      virtual INT32 onSendMsg( const NET_HANDLE &handle,
                               const MsgRouteID &id,
                               MsgHeader *header ) ;
      // handle event on receiving message ( via UDP )
      virtual INT32 onReceiveMsg( const NET_HANDLE &handle,
                                  const MsgRouteID &id,
                                  MsgHeader *header,
                                  UINT32 availableSize,
                                  netUserDataHolder *userDataHolder ) ;

   public:
      // override function of STP module
      // get name of module
      OSS_INLINE virtual const CHAR *getModuleName() const
      {
         return STP_NET_MSG_HANDLER_NAME ;
      }

   protected:
      // handle system info message
      INT32 _handleSysInfo( const NET_HANDLE &handle ) ;
   } ;

   /*
      _stpPipeMsgHandler define
    */
   class _stpPipeMsgHandler : public IPmdPipeHandler,
                              public stpHandlerBase
   {
   public:
      // constructor and destructor
      _stpPipeMsgHandler( STPCB *stpCB ) ;
      virtual ~_stpPipeMsgHandler() ;

   public:
      // override functions for pipe handler
      // process message callback
      virtual INT32 processMessage( CHAR *message, utilNodePipe &nodePipe ) ;

   public:
      // override functions for STP module
      // get name of module
      OSS_INLINE virtual const CHAR *getModuleName() const
      {
         return STP_PIPE_MSG_HANDLER_NAME ;
      }
   } ;

}

#endif // STP_MSG_HANDLER_HPP__
