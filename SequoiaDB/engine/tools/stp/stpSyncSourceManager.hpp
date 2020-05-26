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

   Source File Name = stpSyncSourceManager.hpp

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

#ifndef STP_SYNC_SOURCE_MANAGER_HPP__
#define STP_SYNC_SOURCE_MANAGER_HPP__

#include "stpCBCommon.hpp"
#include "stpModule.hpp"
#include "stpNode.hpp"
#include "stpSyncSource.hpp"
#include "stpMsg.hpp"

namespace engine
{

   /*
      _stpSyncSourceManager define
    */
   // _stpSyncSourceManager manages time synchronize as source
   class _stpSyncSourceManager : public stpManagerBase
   {
      DECLARE_OBJ_MSG_MAP()

   public:
      // constructor and destructor
      _stpSyncSourceManager( STPCB *stpCB ) ;
      virtual ~_stpSyncSourceManager() ;

   public:
      // override functions

      // get name of module
      OSS_INLINE virtual const CHAR *getModuleName() const
      {
         return STP_SYNC_SOURCE_MANAGER_NAME ;
      }

      // get role mask of module
      OSS_INLINE virtual UINT32 getModuleRoleMask() const
      {
         return STP_ROLE_MASK_SERVER ;
      }

      // if we need to active EDU
      OSS_INLINE virtual BOOLEAN activeEDU() const
      {
         // no need to active EDU
         // always handle message in main thread of net agent
         return FALSE ;
      }

      // on timer callback
      virtual void onTimer( UINT64 timerID, UINT32 interval ) ;

      // process message callback
      virtual INT32 processMessage( NET_HANDLE handle, MsgHeader *message ) ;

   protected:
      // override functions of STP module

      // internal call of initialize
      virtual INT32 _initialize() ;

      // internal call of finalize
      virtual INT32 _finalize() ;

      // internal call on event before activate
      virtual INT32 _preActivate() ;

      // internal call on event after activate
      virtual INT32 _postActivate() ;

      // internal call on event before deactivate
      virtual INT32 _preDeactivate() ;

      // internal call on event after deactivate
      virtual INT32 _postDeactivate() ;

   public:
      // handle time synchronize request
      OSS_INLINE INT32 handleTimeSyncReq( NET_HANDLE handle,
                                          const stpTimeSyncReq *request )
      {
         return _handleTimeSyncReq( handle, request ) ;
      }

      // on event to receive time synchronize request
      INT32 onReceiveTimeSyncReq( stpTimeSyncReq *request ) ;
      // on event to send time synchronize response
      INT32 onSendTimeSyncRsp( stpTimeSyncRsp *response ) ;

   protected:
      // handle synchronize register request
      INT32 _handleRegReq( NET_HANDLE handle, const stpRegReq *request ) ;
      // handle time synchronize request
      INT32 _handleTimeSyncReq( NET_HANDLE handle,
                                const stpTimeSyncReq *request ) ;

      // send synchronize register response
      INT32 _sendRegRsp( NET_HANDLE handle,
                         const stpRegReq *request,
                         const stpClientNode &client,
                         INT32 returnCode ) ;

   public:
      // functions to manage synchronize clients
      // register synchronize client
      INT32 registerClient( const MsgRouteID &routeID,
                            UINT32 version,
                            stpClientNode &client ) ;
      // dump all synchronize clients
      INT32 dumpClients( STP_CLIENT_MAP &clients ) ;
      // remove all synchronize clients
      INT32 removeClients() ;

      // signal to push time forward
      void signalPushTime() ;

   protected:
      // remove expired synchronize clients
      INT32 _clearExpiredClients() ;

      // assign a synchronize client to synchronize source
      // WARNING: should be protected by source mutex
      INT32 _assignSource( const MsgRouteID &routeID,
                           stpSyncSource **source ) ;

      // allocate a synchronize source
      // - force: force to listen a specified port
      // WARNING: should be protected by source mutex
      INT32 _allocSource( stpSyncSource **source,
                          BOOLEAN force ) ;

      // add synchronize source into list
      // WARNING: should be protected by source mutex
      INT32 _addSource( stpSyncSource *source ) ;

      // check whether need to push time forward
      BOOLEAN _needPushTime() ;
      // push time forward ( by one minutes )
      void _pushTime() ;

   protected:
      // event to push time forward
      volatile BOOLEAN  _pushEvent ;
      // last time to push time forward
      UINT64            _lastPushTick ;
      // last time to clear expired synchronize clients
      UINT64            _clearClientTimeout ;
      // mutex to protect synchronize sources
      ossRWMutex        _sourceMutex ;
      // index to dispatch ports to synchronize client
      UINT16            _portIndex ;
      // system synchronize source
      stpSyncSource     _sysSource ;
      // list of synchronize sources
      // WARNING: should be protected by source mutex
      STP_SYNC_SOURCE_LIST _syncSources ;
      // option of maximum number of synchronize ports
      UINT32            _maxSyncPorts ;
      // number of ports is allowed to use for synchronize
      UINT32            _allowSyncPorts ;
      // option of default number of clients per port
      UINT32            _defClientsPerPort ;
   } ;

}

#endif // STP_SYNC_SOURCE_MANAGER_HPP__
