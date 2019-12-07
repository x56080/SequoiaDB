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
