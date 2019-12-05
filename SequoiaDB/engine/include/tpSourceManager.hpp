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

   Source File Name = tpSourceManager.hpp

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

#ifndef TP_SOURCE_MANAGER_HPP__
#define TP_SOURCE_MANAGER_HPP__

#include "tpCBCommon.hpp"
#include "tpModule.hpp"
#include "tpNode.hpp"
#include "msgTp.hpp"

namespace engine
{

   /*
      _tpSourceManager define
    */
   // _tpSourceManager manages time synchronize as source
   class _tpSourceManager : public tpManagerBase
   {
      DECLARE_OBJ_MSG_MAP()

   public:
      _tpSourceManager( SDB_TPCB *tpCB ) ;
      virtual ~_tpSourceManager() ;

   public:
      OSS_INLINE virtual const CHAR *getModuleName() const
      {
         return TP_SOURCE_MANAGER_NAME ;
      }

      OSS_INLINE virtual UINT32 getModuleRoleMask() const
      {
         return TP_ROLE_MASK_SERVER ;
      }

      OSS_INLINE virtual BOOLEAN activeEDU() const
      {
         return FALSE ;
      }

      virtual void onTimer( UINT64 timerID, UINT32 interval ) ;
      virtual INT32 processMessage( NET_HANDLE handle, MsgHeader *message ) ;

   public:
      // on event to receive time synchronize request
      INT32 onReceiveTimeSyncReq( MsgTpTimeSyncReq *request ) ;
      // on event to send time synchronize response
      INT32 onSendTimeSyncRes( MsgTpTimeSyncRsp *response ) ;

   protected:
      // handle synchronize register request
      INT32 _handleRegReq( NET_HANDLE handle, const MsgTpRegReq *request ) ;
      // handle time synchronize request
      INT32 _handleTimeSyncReq( NET_HANDLE handle,
                                const MsgTpTimeSyncReq *request ) ;

      // send synchronize register response
      INT32 _sendRegRsp( NET_HANDLE handle,
                         const MsgTpRegReq *request,
                         const tpClientNode &client,
                         INT32 returnCode ) ;
      // send time synchronize response
      INT32 _sendTimeSyncRsp( NET_HANDLE handle,
                              const MsgTpTimeSyncReq *request,
                              const tpClientNode &client,
                              INT32 returnCode ) ;

   public:
      // get synchronize client by given route ID
      INT32 getClient( const MsgRouteID &routeID, tpClientNode &client ) ;
      // register synchronize client
      INT32 registerClient( UINT32 version, const tpClientNode &client ) ;
      // remove synchronize client by given route ID
      INT32 removeClient( const MsgRouteID &routeID ) ;
      // remove expired synchronize client
      INT32 removeClient( const MsgRouteID &routeID, UINT64 syncTick ) ;
      // update synchronize client
      INT32 updateClient( UINT32 version, const tpClientNode &client ) ;
      // dump all synchronize clients
      INT32 dumpClients( TP_CLIENT_MAP &clients ) ;
      // remove all synchronize clients
      INT32 removeClients() ;

   protected:
      // remove expired synchronize clients
      INT32 _clearExpiredClients() ;

      // signal to push time forward
      void     _signalPushTime() ;
      // check whether need to push time forward
      BOOLEAN  _needPushTime() ;
      // push time forward ( by one minutes )
      void     _pushTime() ;

   protected:
      // lock to protected synchronize clients
      ossRWMutex        _clientMutex ;
      // map of synchronize clients
      // NOTE: client contains synchronize history
      TP_CLIENT_MAP     _clients ;
      // event to push time forward
      volatile BOOLEAN  _pushEvent ;
      // last time to push time forward
      UINT64            _lastPushTick ;
      // last time to clear expired synchronize clients
      UINT64            _clearClientTimeout ;
   } ;

}

#endif // TP_SOURCE_MANAGER_HPP__
