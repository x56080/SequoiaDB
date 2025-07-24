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

   Source File Name = stpSyncClientManager.hpp

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
#ifndef STP_SYNC_CLIENT_MANAGER_HPP__
#define STP_SYNC_CLIENT_MANAGER_HPP__

#include "stpCBCommon.hpp"
#include "stpModule.hpp"
#include "stpSyncStats.hpp"
#include "stpNode.hpp"
#include "ossUtil.hpp"

namespace engine
{

   /*
      _stpSyncClientManager define
    */
   // _stpSyncClientManager manages time synchronize as client
   class _stpSyncClientManager : public stpManagerBase
   {
      DECLARE_OBJ_MSG_MAP()

   public:
      _stpSyncClientManager( STPCB *stpCB ) ;
      ~_stpSyncClientManager() ;

   public:
      // override functions for STP module

      // get name of module
      OSS_INLINE virtual const CHAR *getModuleName() const
      {
         return STP_SYNC_CLIENT_MANAGER_NAME ;
      }

      // get role mask of module
      OSS_INLINE virtual UINT32 getModuleRoleMask() const
      {
         // synchronize client used in both client and server roles
         return STP_ROLE_MASK_CLIENT | STP_ROLE_MASK_SERVER ;
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
      // protected override functions for STP module

      // on event post activate
      virtual INT32 _postActivate() ;

   public:
      // on event to send time synchronize request
      INT32 onSendTimeSyncReq( stpTimeSyncReq *request ) ;
      // on event to receive time synchronize response
      INT32 onReceiveTimeSyncRsp( stpTimeSyncRsp *response ) ;

   protected:
      // handle synchronize register response
      INT32 _handleRegRsp( NET_HANDLE handle,
                           const stpRegRsp *response ) ;
      // handle time synchronize response
      INT32 _handleTimeSyncRsp( NET_HANDLE handle,
                                const stpTimeSyncRsp *response ) ;

      // send synchronize register request
      INT32 _sendRegReq( const MsgRouteID &routeID,
                         UINT32 version,
                         const bson::BSONObj &regObject ) ;
      // send time synchronize request ( in UDP )
      INT32 _sendTimeSyncReq( const MsgRouteID &routeID,
                              UINT32 version,
                              UINT16 flag,
                              STP_SYNC_STATUS status,
                              UINT32 timeError ) ;

   public:
      // get status of synchronize
      OSS_INLINE STP_SYNC_STATUS getStatus() const
      {
         return _status ;
      }

      // get registered source route ID
      OSS_INLINE const MsgRouteID &getRegSourceRID() const
      {
         return _regSourceRID ;
      }

      OSS_INLINE UINT64 getRegSourceRIDValue() const
      {
         return _regSourceRID.value ;
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
      INT32 commitRecord( const stpSyncRecord &record ) ;
      // get current synchronize interval by status
      UINT64 getCurrentSyncInterval() ;
      // active given status of synchronize
      void activeStatus( STP_SYNC_STATUS status ) ;

   protected:
      // check whether synchronize record is in valid ( <= time error )
      OSS_INLINE BOOLEAN _checkRecord( const stpSyncRecord &record )
      {
         return ( record.isValid() || !getMetaData()->hasSynchronized() ) ;
      }

      // set last synchronize request ID
      OSS_INLINE void _setLastRequestID( UINT64 requestID, UINT32 version )
      {
         _lastRequestID.swapGreaterThan( requestID ) ;
         _lastVersion = version ;
      }

      // check whether has enough synchronize records
      BOOLEAN _hasEnoughRecords() ;

      // adjust time by synchronize record
      void _adjustTime( const stpSyncRecord &record ) ;
      // adjust slew rate by synchronize records
      BOOLEAN _adjustSlewRate( STP_SYNC_REC_LIST &records ) ;

      // check synchronize records are validated for slew rate calculation
      // - calculate total offset and average offset
      // - check outliers with Inter-Quantile Range
      // - check standard deviation for average offset
      BOOLEAN _checkSlewRateValid( STP_SYNC_REC_LIST &records,
                                   INT64 &totalOffset,
                                   INT64 &averageOffset ) ;

      // check whether we could decrease time error
      BOOLEAN _canDecTimeError( UINT32 curTimeError ) ;

      // calculate the synchronize interval in seconds
      OSS_INLINE UINT32 _calcCurSyncIntSec( UINT32 maxSyncIntSec )
      {
         UINT32 tmpSyncIntSec = STP_SYNC_STEP_TO_INT( _syncTimeSteps ) ;
         return OSS_MIN( maxSyncIntSec, tmpSyncIntSec ) ;
      }

      OSS_INLINE void _resetSyncTimeSteps()
      {
         _syncTimeSteps = 0 ;
         _lastStableStepCount = 0 ;
      }

      BOOLEAN _updateSyncTimeSteps( const stpSyncRecord &record ) ;

      // launch register
      INT32 _launchRegister( const MsgRouteID &primaryRID,
                             UINT32 version,
                             const stpClientNode &local ) ;
      // launch time synchronization
      INT32 _launchTimeSync( const MsgRouteID &sourceRID,
                             UINT32 version,
                             const stpClientNode &local ) ;

   public:
      // register a given synchronize source
      INT32 registerSource( const stpSourceNode &source ) ;
      // get synchronize source by route ID
      INT32 getSource( const MsgRouteID &routeID, stpSourceNode &source ) ;
      // remove synchronize source by route ID
      INT32 removeSource( const MsgRouteID &routeID ) ;
      // remove expired synchronize source
      INT32 removeSource( const MsgRouteID &routeID, UINT64 expiredTick ) ;
      // dump all synchronize sources
      INT32 dumpSources( STP_SOURCE_MAP &sources ) ;

   protected:
      // on event of synchronize register response
      INT32 _onRegRsp( const MsgRouteID &routeID, UINT16 port ) ;

      // on event of time synchronize request
      INT32 _onSyncReq( const MsgRouteID &routeID ) ;

      // on event of time synchronize response
      INT32 _onSyncRsp( const MsgRouteID &routeID,
                        const stpSyncRecord &record,
                        BOOLEAN isValid ) ;

      // remove expired synchronize sources
      INT32 _clearExpiredSources() ;

      void  _resetSourceRID() ;

   protected:
      // status of time synchronize
      STP_SYNC_STATUS      _status ;
      // synchronize count of current status
      UINT32               _curStatusCount ;
      // event to start synchronize
      volatile BOOLEAN     _syncEvent ;
      // route ID to registered synchronize
      // NOTE: default port
      MsgRouteID           _regSourceRID ;
      // route ID to send time synchronize messages
      // NOTE: may be assigned to an extra synchronize port
      MsgRouteID           _syncSourceRID ;
      // last request ID of time synchronize request
      ossAtomic64          _lastRequestID ;
      // last version of servers to send time synchronize request
      UINT32               _lastVersion ;
      // time of stable synchronize status starts ( interval-check status,
      // a relatively stable status )
      UINT64               _lastStableTick ;
      // list of synchronize records
      // cache last few synchronize records for later calculations, e.g.
      // check-slew-rate status requires total offsets of last few intervals
      STP_SYNC_REC_LIST    _syncRecords ;
      // lock to protect synchronize sources
      ossRWMutex           _sourceMutex ;
      // map of synchronize sources
      // NOTE: sources contains synchronize history
      STP_SOURCE_MAP       _sources ;
      // timeout to launch time synchronize
      UINT64               _syncTimeTimeout ;
      // indicates whether to wait for synchronize response
      BOOLEAN              _waitSyncRsp ;
      // timeout to clear expired sources
      UINT64               _sourceClearTimeout ;

      // steps to calculate synchronize interval ( 1 step for 10s )
      UINT32               _syncTimeSteps ;
      // counts to have the same steps of synchronize interval
      UINT32               _lastStableStepCount ;
   } ;

}

#endif // STP_SYNC_CLIENT_MANAGER_HPP__
