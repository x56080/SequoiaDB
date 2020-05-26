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

   Source File Name = stpSyncSource.hpp

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

#ifndef STP_SYNC_SOURCE_HPP__
#define STP_SYNC_SOURCE_HPP__

#include "stpCBCommon.hpp"
#include "stpModule.hpp"
#include "stpNode.hpp"
#include "stpNetManager.hpp"
#include "stpMsgHandler.hpp"

namespace engine
{

   #define STP_SYS_SYNC_SOURCE_INDEX ( 0 )

   /*
      _stpSyncSource define
    */
   class _stpSyncSource : public utilPooledObject
   {
   public:
      _stpSyncSource( STPCB *stpCB, BOOLEAN isSystem ) ;
      virtual ~_stpSyncSource() ;

   public:
      OSS_INLINE UINT16 getPort() const
      {
         return _port ;
      }

      OSS_INLINE stpNetManager *getNetManager()
      {
         return _netManager ;
      }

      OSS_INLINE netRouteAgent *getNetAgent()
      {
         SDB_ASSERT( NULL != _netManager, "net manager is invalid" ) ;
         return _netManager->getNetAgent() ;
      }

      OSS_INLINE BOOLEAN isSystem() const
      {
         return _isSystem ;
      }

      INT32 initNetManager( const CHAR *hostName,
                            UINT16 port,
                            UINT32 protocolMask ) ;

      INT32 setNetManager( stpNetManager *netManager,
                           UINT16 port,
                           UINT32 protocolMask ) ;

      INT32 activeNetManager() ;
      INT32 deactiveNetManager() ;

      void freeNetManager() ;

      virtual INT32 handleTimeSyncReq( NET_HANDLE handle,
                                       const stpTimeSyncReq *request ) ;

   protected:
      // send time synchronize response
      INT32 _sendTimeSyncRsp( NET_HANDLE handle,
                              const stpTimeSyncReq *request,
                              const stpClientNode &client,
                              INT32 returnCode ) ;

   public:
      // functions to manage synchronize clients
      // get synchronize client by given route ID
      INT32 getClient( const MsgRouteID &routeID, stpClientNode &client ) ;
      BOOLEAN hasClient( const MsgRouteID &routeID ) ;
      // register synchronize client
      INT32 registerClient( const MsgRouteID &routeID,
                            const stpClientNode &client ) ;
      // remove expired synchronize client
      INT32 removeClient( const MsgRouteID &routeID, UINT64 syncTick ) ;
      // update synchronize client
      INT32 updateClient( const MsgRouteID &routeID,
                          const stpClientNode &client ) ;
      // dump all synchronize clients
      INT32 dumpClients( STP_CLIENT_MAP &clients ) ;
      // remove all synchronize clients
      INT32 removeClients() ;
      // get number of clients
      UINT32 getClientNum() ;

      // remove expired synchronize clients
      INT32 clearExpiredClients() ;

   protected:
      STPCB *              _stpCB ;
      BOOLEAN              _isSystem ;
      UINT16               _port ;
      stpNetManager *      _netManager ;
      stpSyncSourceMsgHandler _msgHandler ;
      // lock to protected synchronize clients
      ossRWMutex           _clientMutex ;
      // map of synchronize clients
      // NOTE: client contains synchronize history
      STP_CLIENT_MAP       _clients ;
   } ;

   typedef class _stpSyncSource stpSyncSource ;
   typedef ossPoolVector< stpSyncSource * > STP_SYNC_SOURCE_LIST ;

}

#endif // STP_SYNC_SOURCE_HPP__
