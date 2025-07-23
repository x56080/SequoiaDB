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

   Source File Name = stpServiceManager.hpp

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
#ifndef STP_SERVICE_MANAGER_HPP__
#define STP_SERVICE_MANAGER_HPP__

#include "stpCBCommon.hpp"
#include "stpModule.hpp"
#include "stpSession.hpp"
#include "pmdAsyncHandler.hpp"

namespace engine
{

   /*
      _stpServiceManager define
    */
   // _stpServiceManager manages service sessions from sdb clients, e.g. shell
   class _stpServiceManager : public stpManagerBase
   {
      DECLARE_OBJ_MSG_MAP()

   public:
      // constructor and destructor
      _stpServiceManager( STPCB *stpCB ) ;
      virtual ~_stpServiceManager() ;

   public:
      // override functions for STP module

      // get name of module
      OSS_INLINE virtual const CHAR *getModuleName() const
      {
         return STP_SERVICE_MANAGER_NAME ;
      }

      // indicate if another EDU is needed
      OSS_INLINE virtual BOOLEAN activeEDU() const
      {
         // no need to active another EDU, since we already use asynchronous
         // sessions, this service manager only redirect messages
         return FALSE ;
      }

      // on timer callback
      virtual void onTimer( UINT64 timerID, UINT32 interval ) ;

      // process message callback
      virtual INT32 processMessage( NET_HANDLE handle, MsgHeader *message ) ;

   protected:
      // override protected functions of STP module

      // initialize module
      virtual INT32 _initialize() ;

      // finalize module
      virtual INT32 _finalize() ;

      // event previous to deactivate
      virtual INT32 _preDeactivate() ;

   public:
      // get asynchronous message handler
      OSS_INLINE pmdAsyncMsgHandler *getAsyncMsgHandler()
      {
         return ( &_messageHandler ) ;
      }

      // get asynchronous timer handler
      OSS_INLINE pmdAsyncTimerHandler *getAsyncTimerHandler()
      {
         return ( &_timeoutHandler ) ;
      }

      // get asynchronous session manager
      OSS_INLINE stpSessionManager *getSessionManager()
      {
         return ( &_sessionManager ) ;
      }

      // redirect message to primary
      INT32 redirectPrimary( stpSession *session,
                             MsgHeader *message ) ;

   protected:
      // asynchronous message handler
      pmdAsyncMsgHandler   _messageHandler ;
      // asynchronous timer handler
      pmdAsyncTimerHandler _timeoutHandler ;
      // asynchronous session manager
      stpSessionManager    _sessionManager ;
   } ;

}

#endif // STP_SERVICE_MANAGER_HPP__
