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

   Source File Name = pmdRemoteMsgEventHandler.hpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          05/03/2017  XJH Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef PMD_REMOTE_MSG_EVENT_HANDLER_HPP__
#define PMD_REMOTE_MSG_EVENT_HANDLER_HPP__

#include "netMsgHandler.hpp"
#include "netTimer.hpp"

namespace engine
{
   class _pmdRemoteSessionMgr ;
   class _pmdEDUCB ;
   class _IExecutorEventHandler ;

   /*
      _pmdRemoteMsgHandler define
   */
   class _pmdRemoteMsgHandler : public _netMsgHandler
   {
      public:
         _pmdRemoteMsgHandler( _pmdRemoteSessionMgr *pRSManager,
                               _IExecutorEventHandler *pExeHandler = NULL ) ;
         virtual ~_pmdRemoteMsgHandler() ;

         void  attach( _pmdEDUCB *cb ) ;
         void  detach() ;

         virtual INT32 handleMsg( const NET_HANDLE &handle,
                                  const _MsgHeader *header,
                                  const CHAR *msg ) ;
         virtual void  handleClose( const NET_HANDLE &handle, _MsgRouteID id ) ;
         virtual INT32 handleConnect( const NET_HANDLE &handle,
                                      _MsgRouteID id,
                                      BOOLEAN isPositive ) ;
      protected:
         INT32 _postMsg( const NET_HANDLE &handle,
                         const MsgHeader *header,
                         const CHAR *msg = NULL ) ;

      protected:
         _pmdRemoteSessionMgr                *_pRSManager ;
         _pmdEDUCB                           *_pMainCB ;
         _IExecutorEventHandler              *_pExeHandler ;

   } ;
   typedef _pmdRemoteMsgHandler pmdRemoteMsgHandler ;

   /*
      _pmdRemoteTimerHandler define
   */
   class _pmdRemoteTimerHandler : public _netTimeoutHandler
   {
      public:
         _pmdRemoteTimerHandler() ;
         virtual ~_pmdRemoteTimerHandler() ;

         void  attach( _pmdEDUCB *cb ) ;
         void  detach() ;

         virtual void handleTimeout( const UINT32 &millisec,
                                     const UINT32 &id ) ;

      private:
         _pmdEDUCB               *_pMainCB ;

   } ;
   typedef _pmdRemoteTimerHandler pmdRemoteTimerHandler ;

}

#endif // PMD_REMOTE_MSG_EVENT_HANDLER_HPP__

