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

   Source File Name = omMsgEventHandler.hpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          25/06/2014  XJH Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef OM_MSG_EVENT_HANDLER_HPP__
#define OM_MSG_EVENT_HANDLER_HPP__

#include "omDef.hpp"
#include "netMsgHandler.hpp"
#include "netTimer.hpp"

namespace engine
{
   class _pmdRemoteSessionMgr ;
   class _pmdEDUCB ;

   /*
      _omMsgHandler define
   */
   class _omMsgHandler : public _netMsgHandler
   {
      public:
         _omMsgHandler( _pmdRemoteSessionMgr *pRSManager ) ;
         virtual ~_omMsgHandler() ;

         void  attach( _pmdEDUCB *cb ) ;
         void  detach() ;

         virtual INT32 handleMsg( const NET_HANDLE &handle,
                                  const _MsgHeader *header,
                                  const CHAR *msg ) ;
         virtual void  handleClose( const NET_HANDLE &handle, _MsgRouteID id ) ;
         virtual void  handleConnect( const NET_HANDLE &handle,
                                      _MsgRouteID id,
                                      BOOLEAN isPositive ) ;

      protected:
         _pmdRemoteSessionMgr                *_pRSManager ;
         _pmdEDUCB                           *_pMainCB ;

   } ;
   typedef _omMsgHandler omMsgHandler ;

   /*
      _omTimerHandler define
   */
   class _omTimerHandler : public _netTimeoutHandler
   {
      public:
         _omTimerHandler() ;
         virtual ~_omTimerHandler() ;

         void  attach( _pmdEDUCB *cb ) ;
         void  detach() ;

         virtual void handleTimeout( const UINT32 &millisec,
                                     const UINT32 &id ) ;

      private:
         _pmdEDUCB               *_pMainCB ;

   } ;
   typedef _omTimerHandler omTimerHandler ;

}

#endif // OM_MSG_EVENT_HANDLER_HPP__

