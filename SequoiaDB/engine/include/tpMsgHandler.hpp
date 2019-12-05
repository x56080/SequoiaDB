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

   Source File Name = tpMsgHandler.hpp

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

#ifndef TP_MSG_HANDLER_HPP__
#define TP_MSG_HANDLER_HPP__

#include "tpCBCommon.hpp"
#include "tpModule.hpp"
#include "msgTp.hpp"
#include "netMsgHandler.hpp"
#include "netRouteAgent.hpp"
#include "pmdPipeManager.hpp"

namespace engine
{

   /*
      _tpMsgHandler define
    */
   typedef class _tpHandlerBase _tpMsgHandler ;
   typedef class _tpHandlerBase tpMsgHandler ;

   /*
      _tpNetMsgHandler define
    */
   class _tpNetMsgHandler : public INetUDPMsgHandler,
                            public tpMsgHandler
   {
   public :
      _tpNetMsgHandler( SDB_TPCB *tpCB ) ;
      virtual ~_tpNetMsgHandler() ;

   public:
      virtual INT32 handleMsg( const NET_HANDLE &handle,
                               const MsgHeader *header,
                               const CHAR *message ) ;
      virtual void handleClose( const NET_HANDLE &handle,
                                MsgRouteID id ) ;

      virtual void onSendMsg( const NET_HANDLE &handle,
                              MsgRouteID id,
                              MsgHeader *header ) ;
      virtual void onReceiveMsg( const NET_HANDLE &handle,
                                 MsgRouteID id,
                                 MsgHeader *header ) ;

   public:
      OSS_INLINE virtual const CHAR *getModuleName() const
      {
         return TP_NET_MSG_HANDLER_NAME ;
      }

   public:
      OSS_INLINE UINT64 allocateRequestID()
      {
         return _requestID.inc() ;
      }

   protected:
      INT32 _handleSysInfo( const NET_HANDLE &handle ) ;

   public:
      ossAtomic64 _requestID ;
   } ;

   /*
      _tpPipeMsgHandler define
    */
   class _tpPipeMsgHandler : public IPmdPipeHandler,
                             public tpMsgHandler
   {
   public:
      _tpPipeMsgHandler( SDB_TPCB *tpCB ) ;
      virtual ~_tpPipeMsgHandler() ;

   public:
      virtual INT32 processMessage( CHAR *message,
                                     utilNodePipe &nodePipe ) ;

   public:
      OSS_INLINE virtual const CHAR *getModuleName() const
      {
         return TP_PIPE_MSG_HANDLER_NAME ;
      }
   } ;

}

#endif // TP_MSG_HANDLER_HPP__
