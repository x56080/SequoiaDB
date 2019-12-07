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

   Source File Name = stpMetaManager.hpp

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

#ifndef STP_META_MANAGER_HPP__
#define STP_META_MANAGER_HPP__

#include "stpCBCommon.hpp"
#include "stpModule.hpp"
#include "utilSHMBuffer.hpp"
#include "stpMetaStore.hpp"
#include "stpMsg.hpp"

namespace engine
{

   /*
      _stpMetaManager define
    */
   // _stpMetaManager manages meta data including shared memory and meta LSN
   class _stpMetaManager : public stpManagerBase
   {
      DECLARE_OBJ_MSG_MAP()

   public:
      // constructor and destructor
      _stpMetaManager( STPCB *stpCB ) ;
      virtual ~_stpMetaManager () ;

   public:
      // get name of module
      OSS_INLINE virtual const CHAR *getModuleName() const
      {
         return STP_META_MANAGER_NAME ;
      }

      // on timer callback
      virtual void onTimer( UINT64 timerID, UINT32 interval ) ;

      // process message callback
      virtual INT32 processMessage( NET_HANDLE handle,
                                    MsgHeader *message ) ;

   protected:
      // internal initialize
      virtual INT32 _initialize() ;
      // internal finalize
      virtual INT32 _finalize() ;
      // on event after activated
      virtual INT32 _postActivate() ;

      // on event of primary change
      virtual INT32 _onChangePrimary( const MsgRouteID &primaryRID,
                                      BOOLEAN primaryIsMe ) ;

   public:
      // get key of shared memory ( service name )
      OSS_INLINE const CHAR *getSHMKey() const
      {
         return _buffer.getKeyString() ;
      }

   protected:
      // handle meta notify ( need update meta LSN )
      INT32 _handleMetaNotify( NET_HANDLE handle,
                               const stpMetaNotify *notify ) ;
      // handle meta synchronize request
      INT32 _handleMetaSyncReq( NET_HANDLE handle,
                               const stpMetaSyncReq *request ) ;
      // handle meta synchronize response
      INT32 _handleMetaSyncRsp( NET_HANDLE handle,
                                const stpMetaSyncRsp *response ) ;

      // send meta notify
      INT32 _sendMetaNotify( const MsgRouteID &routeID ) ;
      // send meta synchronize request
      INT32 _sendMetaSyncReq( const MsgRouteID &routeID ) ;
      // send meta synchronize response
      INT32 _sendMetaSyncRsp( NET_HANDLE handle,
                              const stpMetaSyncReq *request,
                              UINT64 time,
                              UINT32 version,
                              INT32 returnCode ) ;

      // broadcast meta notify
      INT32 _broadcastMetaNotify() ;

   public:
      // launch meta synchronize
      INT32 launchMetaSync() ;
      // get meta LSN
      INT32 getMetaLSN( UINT64 &time, UINT32 &version ) ;
      // get meta LSN in DPS_LSN format
      INT32 getMetaLSN( DPS_LSN &lsn ) ;
      // update meta LSN
      INT32 updateMetaLSN( const DPS_LSN &metaLSN ) ;
      // update meta LSN by getting logical time
      INT32 updateMetaLSN() ;

   protected:
      // set meta LSN
      INT32 _setMetaLSN( UINT64 time, UINT32 version ) ;
      // update meta LSN
      INT32 _updateMetaLSN( UINT64 time,
                            BOOLEAN increaseVersion,
                            BOOLEAN &updated ) ;
      // update meta LSN
      INT32 _updateMetaLSN( const DPS_LSN &metaLSN, BOOLEAN &updated ) ;

      // get synchronize interval of meta data
      UINT64 _getMetaSyncInterval() ;

   protected:
      // lock to protect meta
      ossRWMutex     _mutex ;
      // shared memory buffer to meta data
      utilSHMBuffer  _buffer ;
      // meta LSN store
      stpMetaStore   _store ;
      // timeout to synchronize meta
      UINT64         _metaSyncTimeout ;
   } ;

}

#endif // STP_META_MANAGER_HPP__
