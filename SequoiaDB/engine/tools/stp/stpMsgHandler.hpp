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

   /*
      _stpNetMsgHandler define
    */
   class _stpNetMsgHandler : public INetUDPMsgHandler,
                             public stpHandlerBase
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
                               const CHAR *message ) ;
      // handle close message ( disconnect )
      virtual void handleClose( const NET_HANDLE &handle,
                                MsgRouteID id ) ;

      // handle event on sending message ( via UDP )
      virtual void onSendMsg( const NET_HANDLE &handle,
                              MsgRouteID id,
                              MsgHeader *header ) ;
      // handle event on receiving message ( via UDP )
      virtual void onReceiveMsg( const NET_HANDLE &handle,
                                 MsgRouteID id,
                                 MsgHeader *header ) ;

   public:
      // override function of STP module
      // get name of module
      OSS_INLINE virtual const CHAR *getModuleName() const
      {
         return STP_NET_MSG_HANDLER_NAME ;
      }

   public:
      // allocate request ID
      OSS_INLINE UINT64 allocateRequestID()
      {
         return _requestID.inc() ;
      }

   protected:
      // handle system info message
      INT32 _handleSysInfo( const NET_HANDLE &handle ) ;

   public:
      // current request ID ( used to allocate new request ID )
      ossAtomic64 _requestID ;
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
