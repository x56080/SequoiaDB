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

   Source File Name = tpServiceManager.hpp

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

#ifndef TP_SERVICE_MANAGER_HPP__
#define TP_SERVICE_MANAGER_HPP__

#include "tpCBCommon.hpp"
#include "tpModule.hpp"
#include "tpSession.hpp"
#include "pmdAsyncHandler.hpp"

namespace engine
{

   /*
      _tpServiceManager define
    */
   // _tpServiceManager manages service sessions from sdb clients, e.g. shell
   class _tpServiceManager : public tpManagerBase
   {
      DECLARE_OBJ_MSG_MAP()

   public:
      _tpServiceManager( SDB_TPCB *tpCB ) ;
      virtual ~_tpServiceManager() ;

   public:
      OSS_INLINE virtual const CHAR *getModuleName() const
      {
         return TP_SERVICE_MANAGER_NAME ;
      }

      OSS_INLINE virtual BOOLEAN activeEDU() const
      {
         return FALSE ;
      }

      virtual void onTimer( UINT64 timerID, UINT32 interval ) ;
      virtual INT32 processMessage( NET_HANDLE handle, MsgHeader *message ) ;

   protected:
      virtual INT32 _initialize() ;
      virtual INT32 _finalize() ;
      virtual INT32 _preDeactivate() ;

   public:
      OSS_INLINE pmdAsyncMsgHandler *getAsyncMsgHandler()
      {
         return ( &_messageHandler ) ;
      }

      OSS_INLINE pmdAsyncTimerHandler *getAsyncTimerHandler()
      {
         return ( &_timeoutHandler ) ;
      }

      OSS_INLINE tpSessionManager *getSessionManager()
      {
         return ( &_sessionManager ) ;
      }

   protected:
      // async message handler
      pmdAsyncMsgHandler   _messageHandler ;
      // aysnc timer handler
      pmdAsyncTimerHandler _timeoutHandler ;
      // session manager
      tpSessionManager     _sessionManager ;
   } ;

}

#endif // TP_SERVICE_MANAGER_HPP__
