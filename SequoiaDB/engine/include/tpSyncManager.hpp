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

   Source File Name = tpSyncManager.hpp

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

#ifndef TP_SYNC_MANAGER_HPP__
#define TP_SYNC_MANAGER_HPP__

#include "tpCBCommon.hpp"
#include "tpModule.hpp"
#include "tpSyncStats.hpp"
#include "tpNode.hpp"
#include "ossUtil.hpp"

namespace engine
{

   #define TP_SYNC_RECORD_CACHE_SIZE      ( 10 )
   #define TP_SYNC_OFFSET_MAX_LIMIT       ( 100000L )
   #define TP_SYNC_OFFSET_MIN_LIMIT       ( -100000L )
   #define TP_SLEW_RATE_CHECK_INTERVAL    ( 10000 )

   /*
      _tpSyncManager define
    */
   // _tpSyncManager manages time synchronize as client
   class _tpSyncManager : public tpManagerBase
   {
      DECLARE_OBJ_MSG_MAP()

   public:
      _tpSyncManager( SDB_TPCB *tpCB ) ;
      ~_tpSyncManager() ;

   public:
      OSS_INLINE virtual const CHAR *getModuleName() const
      {
         return TP_SYNC_MANAGER_NAME ;
      }

      OSS_INLINE virtual UINT32 getModuleRoleMask() const
      {
         return TP_ROLE_MASK_CLIENT | TP_ROLE_MASK_SERVER ;
      }

      OSS_INLINE virtual BOOLEAN activeEDU() const
      {
         return FALSE ;
      }

      virtual void onTimer( UINT64 timerID, UINT32 interval ) ;
      virtual INT32 processMessage( NET_HANDLE handle, MsgHeader *message ) ;

   protected:
      virtual INT32 _postActivate() ;

   public:
      // on event to send time synchronize request
      INT32 onSendTimeSyncReq( MsgTpTimeSyncReq *request ) ;
      // on event to receive time synchronize response
      INT32 onReceiveTimeSyncRsp( MsgTpTimeSyncRsp *response ) ;

   protected:
      // handle synchronize register response
      INT32 _handleRegRsp( NET_HANDLE handle,
                           const MsgTpRegRsp *response ) ;
      // handle time synchronize response
      INT32 _handleTimeSyncRsp( NET_HANDLE handle,
                                const MsgTpTimeSyncRsp *response ) ;

      // send synchronize register request
      INT32 _sendRegReq( const MsgRouteID &routeID,
                         UINT32 version,
                         const tpClientNode &local ) ;
      // send time synchronize request ( in UDP )
      INT32 _sendTimeSyncReq( const MsgRouteID &routeID,
                              UINT32 version,
                              UINT16 flag,
                              TP_SYNC_STATUS status,
                              UINT32 timeError ) ;

   public:
      // get status of synchronize
      OSS_INLINE TP_SYNC_STATUS getStatus() const
      {
         return _status ;
      }

   public:
      // signal to start synchronize
      OSS_INLINE void signalSync()
      {
         _syncEvent = TRUE ;
      }

      // launch time synchronize
      INT32 launchSync() ;
      // restart time synchronize
      INT32 restartSync() ;
      // commit synchronize record
      INT32 commitRecord( const tpSyncRecord &record ) ;
      // get current synchronize interval by status
      UINT64 getCurrentSyncInterval() ;
      // active given status of synchronize
      void activeStatus( TP_SYNC_STATUS status ) ;

   protected:
      // check whether synchronize record is in valid ( <= time error )
      OSS_INLINE BOOLEAN _checkRecord( const tpSyncRecord &record )
      {
         return ( record.isValid() || !getMetaData()->hasSynchronized() ) ;
      }

      // check whether has enough synchronize records
      OSS_INLINE BOOLEAN _hasEnoughRecords()
      {
         return ( _syncRecords.size() >= TP_SYNC_RECORD_CACHE_SIZE ) ;
      }

      // set last synchronize request ID
      OSS_INLINE void _setLastRequestID( UINT64 requestID, UINT32 version )
      {
         _lastRequestID.swapGreaterThan( requestID ) ;
         _lastVersion = version ;
      }

      // adjust time by synchronize record
      INT32 _adjustTime( const tpSyncRecord &record ) ;
      // adjust slew rate by synchronize records
      void _adjustSlewRate( const TP_SYNC_REC_LIST &records ) ;

      // check whether we could decrease time error
      BOOLEAN _canDecTimeError( UINT32 curTimeError ) ;

   public:
      // register a given synchronize source
      INT32 registerSource( const tpSourceNode &source ) ;
      // get synchronize source by route ID
      INT32 getSource( const MsgRouteID &routeID, tpSourceNode &source ) ;
      // update synchronize source
      INT32 updateSource( const tpSourceNode &source ) ;
      // remove synchronize source by route ID
      INT32 removeSource( const MsgRouteID &routeID ) ;
      // remove expired synchronize source
      INT32 removeSource( const MsgRouteID &routeID, UINT64 expiredTick ) ;
      // dumy all synchronize sources
      INT32 dumpSources( TP_SOURCE_MAP &sources ) ;

   protected:
      // on event of synchronize register response
      INT32 _onRegRsp( const MsgRouteID &routeID ) ;
      // on event of time synchronize request
      INT32 _onSyncReq( const MsgRouteID &routeID ) ;
      // on event of time synchronize response
      INT32 _onSyncRsp( const MsgRouteID &routeID,
                        const tpSyncRecord &record,
                        BOOLEAN isValid ) ;
      // remove expired synchronize sources
      INT32 _clearExpiredSources() ;

   protected:
      // status of time synchronize
      TP_SYNC_STATUS       _status ;
      // event to start synchronize
      volatile BOOLEAN     _syncEvent ;
      // last request ID of time synchronize request
      ossAtomic64          _lastRequestID ;
      // last version of servers to send time synchronize request
      UINT32               _lastVersion ;
      // time of normal synchronize status
      UINT64               _lastNormalTick ;
      // list of synchronize records
      TP_SYNC_REC_LIST     _syncRecords ;
      // lock to protect synchronize sources
      ossRWMutex           _sourceMutex ;
      // map of synchronize sources
      // NOTE: sources contains synchronize history
      TP_SOURCE_MAP        _sources ;
      // timeout to launch time synchronize
      UINT64               _syncTimeTimeout ;
      // timeout to clear expired sources
      UINT64               _sourceClearTimeout ;
   } ;

}

#endif // TP_SYNC_MANAGER_HPP__
