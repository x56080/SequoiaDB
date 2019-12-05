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

   Source File Name = tpMetaManager.hpp

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

#ifndef TP_META_MANAGER_HPP__
#define TP_META_MANAGER_HPP__

#include "tpCBCommon.hpp"
#include "tpModule.hpp"
#include "utilSHMBuffer.hpp"
#include "tpMetaStore.hpp"
#include "msgTp.hpp"

namespace engine
{

   /*
      _tpMetaManager define
    */
   // _tpMetaManager manages meta data including shared memory and meta LSN
   class _tpMetaManager : public tpManagerBase
   {
      DECLARE_OBJ_MSG_MAP()

   public:
      _tpMetaManager( SDB_TPCB *tpCB ) ;
      virtual ~_tpMetaManager () ;

   public:
      OSS_INLINE virtual const CHAR *getModuleName() const
      {
         return TP_META_MANAGER_NAME ;
      }

      virtual void onTimer( UINT64 timerID, UINT32 interval ) ;
      virtual INT32 processMessage( NET_HANDLE handle,
                                    MsgHeader *message ) ;

   protected:
      virtual INT32 _initialize() ;
      virtual INT32 _finalize() ;
      virtual INT32 _postActivate() ;

      virtual INT32 _onChangePrimary( const MsgRouteID &primaryRID,
                                      BOOLEAN isLocalPrimary ) ;

   public:
      // get key of shared memory ( service name )
      OSS_INLINE const CHAR *getSHMKey() const
      {
         return _buffer.getKeyString() ;
      }

   protected:
      // handle meta notify ( need update meta LSN )
      INT32 _handleMetaNotify( NET_HANDLE handle,
                               const MsgTpMetaNotify *notify ) ;
      // handle meta synchronize request
      INT32 _handleMetaSyncReq( NET_HANDLE handle,
                               const MsgTpMetaSyncReq *request ) ;
      // handle meta synchronize response
      INT32 _handleMetaSyncRsp( NET_HANDLE handle,
                                const MsgTpMetaSyncRsp *response ) ;

      // send meta notify
      INT32 _sendMetaNotify( const MsgRouteID &routeID ) ;
      // send meta synchronize request
      INT32 _sendMetaSyncReq( const MsgRouteID &routeID ) ;
      // send meta synchronize response
      INT32 _sendMetaSyncRsp( NET_HANDLE handle,
                              const MsgTpMetaSyncReq *request,
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

      // get meta synchronize interval
      UINT64 _getMetaSyncInterval() ;

   protected:
      // lock to protect meta
      ossRWMutex     _mutex ;
      // shared memory buffer to meta data
      utilSHMBuffer  _buffer ;
      // meta LSN store
      tpMetaStore    _store ;
      // timeout to synchronize meta
      UINT64         _metaSyncTimeout ;
   } ;

}

#endif // TP_META_MANAGER_HPP__
