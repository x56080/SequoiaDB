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

   Source File Name = tpSyncManager.cpp

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

#include "tpSyncManager.hpp"
#include "tpCB.hpp"
#include "tpNode.hpp"
#include "pdTrace.hpp"
#include "tpTrace.hpp"
#include "pmd.hpp"
#include "msgMessage.hpp"

namespace engine
{

   #define TP_CLEAR_SOURCE_INTERVAL      \
                     ( TP_SEC_TO_MILLISEC( ( TP_MAX_SYNC_INTERVAL ) * 2 ) )
   #define TP_SYNC_RECHECK_INTERVAL ( TP_SEC_TO_MILLISEC( 2 * 3600 ) )

   /*
      _tpSyncManager implement
    */
   BEGIN_OBJ_MSG_MAP( _tpSyncManager, _tpManagerBase )
      // ON_MSG
      ON_MSG( MSG_TP_REG_RSP, processMessage )
      ON_MSG( MSG_TP_TIME_SYNC_RSP, processMessage )
   END_OBJ_MSG_MAP()

   _tpSyncManager::_tpSyncManager( SDB_TPCB *tpCB )
   : tpManagerBase( tpCB ),
     _status( TP_SYNC_NOSOURCE ),
     _syncEvent( FALSE ),
     _lastRequestID( 0LL ),
     _lastVersion( TP_GROUP_INVALID_VERSION ),
     _lastNormalTick( 0LL ),
     _syncTimeTimeout( 0LL ),
     _sourceClearTimeout( 0LL )
   {
   }

   _tpSyncManager::~_tpSyncManager()
   {
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__TPSYNCMGR_ONTIMER, "_tpSyncManager::onTimer" )
   void _tpSyncManager::onTimer( UINT64 timerID, UINT32 interval )
   {
      PD_TRACE_ENTRY( SDB__TPSYNCMGR_ONTIMER ) ;

      if ( timerID == _timerID )
      {
         _syncTimeTimeout += interval ;
         if ( _syncTimeTimeout >= getCurrentSyncInterval() )
         {
            INT32 rc = launchSync() ;
            if ( SDB_OK != rc )
            {
               PD_LOG( PDERROR, "Failed to launch synchronize, rc: %d", rc ) ;
            }
            _syncTimeTimeout = 0LL ;
         }
         _sourceClearTimeout += interval ;
         if ( _sourceClearTimeout >= TP_CLEAR_SOURCE_INTERVAL )
         {
            _clearExpiredSources() ;
            _sourceClearTimeout = 0LL ;
         }
      }

      PD_TRACE_EXIT( SDB__TPSYNCMGR_ONTIMER ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__TPSYNCMGR_PROCESSMESSAGE, "_tpSyncManager::processMessage" )
   INT32 _tpSyncManager::processMessage( NET_HANDLE handle,
                                         MsgHeader *message )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__TPSYNCMGR_PROCESSMESSAGE ) ;

      SDB_ASSERT( NULL != message, "message is invalid" ) ;

      switch ( message->opCode )
      {
         case MSG_TP_REG_RSP :
         {
            rc = _handleRegRsp( handle, (const MsgTpRegRsp *)message ) ;
            PD_RC_CHECK( rc, PDERROR, "Failed to handle register response, "
                         "rc: %d", rc ) ;
            break ;
         }
         case MSG_TP_TIME_SYNC_RSP :
         {
            rc = _handleTimeSyncRsp( handle,
                                    (const MsgTpTimeSyncRsp *)message ) ;
            PD_RC_CHECK( rc, PDERROR, "Failed to handle synchronize time "
                         "response, rc: %d", rc ) ;
            break ;
         }
         default :
            PD_CHECK( FALSE, SDB_UNKNOWN_MESSAGE, error, PDERROR,
                      "Unknown synchronize client message [%d]",
                      message->opCode ) ;
            break ;
      }

   done:
      PD_TRACE_EXITRC( SDB__TPSYNCMGR_PROCESSMESSAGE, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__TPSYNCMGR__POSTACTIVATE, "_tpSyncManager::_postActivate" )
   INT32 _tpSyncManager::_postActivate()
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__TPSYNCMGR__POSTACTIVATE ) ;

      _syncEvent = FALSE ;
      launchSync() ;

      PD_TRACE_EXITRC( SDB__TPSYNCMGR__POSTACTIVATE, rc ) ;

      return rc ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__TPSYNCMGR_ONSENDTIMESYNCREQ, "_tpSyncManager::onSendTimeSyncReq" )
   INT32 _tpSyncManager::onSendTimeSyncReq( MsgTpTimeSyncReq *request )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__TPSYNCMGR_ONSENDTIMESYNCREQ ) ;

      SDB_ASSERT( NULL != request, "request is invalid" ) ;

      tpHPTime sendTime = getMetaData()->getLTValue() ;
      request->sendTimeSec = sendTime.getSecond() ;
      request->sendTimeNanoSec = sendTime.getNanoSecond() ;

      PD_TRACE_EXITRC( SDB__TPSYNCMGR_ONSENDTIMESYNCREQ, rc ) ;

      return rc ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__TPSYNCMGR_ONRECEIVETIMESYNCRSP, "_tpSyncManager::onReceiveTimeSyncRsp" )
   INT32 _tpSyncManager::onReceiveTimeSyncRsp( MsgTpTimeSyncRsp *response )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__TPSYNCMGR_ONRECEIVETIMESYNCRSP ) ;

      SDB_ASSERT( NULL != response, "response is invalid" ) ;

      tpHPTime receiveTime = getMetaData()->getLTValue() ;
      response->rspReceiveTimeSec = receiveTime.getSecond() ;
      response->rspReceiveTimeNanoSec = receiveTime.getNanoSecond() ;

      PD_TRACE_EXITRC( SDB__TPSYNCMGR_ONRECEIVETIMESYNCRSP, rc ) ;

      return rc ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__TPSYNCMGR__HANDLEREGRSP, "_tpSyncManager::_handleRegRsp" )
   INT32 _tpSyncManager::_handleRegRsp( NET_HANDLE handle,
                                        const MsgTpRegRsp *response )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__TPSYNCMGR__HANDLEREGRSP ) ;

      SDB_ASSERT( NULL != response, "response is invalid" ) ;

      rc = response->reply.res ;
      if ( SDB_REPL_REMOTE_G_V_EXPIRED == rc )
      {
         _catalogManager->checkExpiredVersion( _lastVersion ) ;
         rc = SDB_OK ;
      }
      if ( SDB_OK == rc )
      {
         UINT64 responseRequestID = response->reply.header.requestID ;
         UINT64 lastRequestID = _lastRequestID.fetch() ;
         PD_CHECK( responseRequestID == lastRequestID,
                   SDB_OK, done, PDWARNING, "Received expired register "
                   "request ID [%llu], expecting request ID [%llu]",
                   responseRequestID, lastRequestID ) ;

         _onRegRsp( response->reply.header.routeID ) ;

         _catalogManager->setLocalOID( response->oid ) ;
         activeStatus( TP_SYNC_CHECKOFFSET ) ;
      }
      else
      {
         if ( SDB_CLS_NOT_PRIMARY == rc )
         {
            _catalogManager->resetPrimary() ;
         }
         else
         {
            activeStatus( TP_SYNC_NOSOURCE ) ;
         }
         PD_RC_CHECK( rc, PDERROR, "Failed to register node, "
                      "received response with error: %d", rc ) ;
      }

   done:
      PD_TRACE_EXITRC( SDB__TPSYNCMGR__HANDLEREGRSP, rc ) ;
      return rc ;

   error:
      restartSync() ;
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__TPSYNCMGR__HANDLETIMESYNCRSP, "_tpSyncManager::_handleTimeSyncRsp" )
   INT32 _tpSyncManager::_handleTimeSyncRsp( NET_HANDLE handle,
                                             const MsgTpTimeSyncRsp *response )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__TPSYNCMGR__HANDLETIMESYNCRSP ) ;

      SDB_ASSERT( NULL != response, "response is invalid" ) ;

      PD_CHECK( _catalogManager->isSyncClient(), SDB_OK, done, PDWARNING,
                "Failed to handle synchronize time response, current node is "
                "not synchronizing client" ) ;

      rc = response->reply.res ;
      if ( SDB_REPL_REMOTE_G_V_EXPIRED == rc )
      {
         _catalogManager->checkExpiredVersion( _lastVersion ) ;
         rc = SDB_OK ;
      }
      if ( SDB_OK == rc )
      {
         tpSyncRecord record( response ) ;
         UINT64 lastRequestID = _lastRequestID.fetch() ;

         PD_CHECK( record.getRequestID() == lastRequestID,
                   SDB_OK, done, PDWARNING, "Received expired synchronize "
                   "request ID [%llu], expecting request ID [%llu]",
                   record.getRequestID(), lastRequestID ) ;

         rc = commitRecord( record ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to commit synchronize record, "
                      "rc: %d", rc ) ;

         _onSyncRsp( response->reply.header.routeID, record,
                     TP_SYNC_CHECKERROR != _status ) ;

         if ( TP_SYNC_CHECKERROR == _status )
         {
            tpClientNode local ;
            UINT32 version = TP_GROUP_INVALID_VERSION ;

            _catalogManager->getLocalAndVersion( local, version ) ;
            _sendTimeSyncReq( response->reply.header.routeID,
                              version,
                              TP_SYNC_TIME_FLAG_INCTIMEERROR,
                              TP_SYNC_CHECKERROR,
                              OSS_MIN( record.getDelay(),
                                       local.getMaxTimeError() ) ) ;
         }
      }
      else
      {
         if ( SDB_CLS_NOT_PRIMARY == rc )
         {
            _catalogManager->resetPrimary() ;
         }
         PD_RC_CHECK( rc, PDERROR, "Failed to synchronize time, "
                      "received response with error: %d", rc ) ;
      }

   done:
      PD_TRACE_EXITRC( SDB__TPSYNCMGR__HANDLETIMESYNCRSP, rc ) ;
      return rc ;

   error:
      // let's restart
      restartSync() ;
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__TPSYNCMGR__SENDREGREQ, "_tpSyncManager::_sendRegReq" )
   INT32 _tpSyncManager::_sendRegReq( const MsgRouteID &routeID,
                                      UINT32 version,
                                      const tpClientNode &local )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__TPSYNCMGR__SENDREGREQ ) ;

      MsgTpRegReq request ;

      _fillRequestHeader( request.header, sizeof( MsgTpRegReq ),
                          MSG_TP_REG_REQ ) ;

      request.version = version ;
      request.role = (UINT32)( local.getRole() ) ;
      request.syncInterval = local.getSyncInterval() ;
      request.maxTimeError = local.getMaxTimeError() ;
      request.timeError = local.getTimeError() ;
      request.oid = local.getOID() ;

      _setLastRequestID( request.header.requestID, version ) ;

      rc = _netAgent->syncSend( routeID, &request ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to send register request to %s, "
                   "rc: %d", routeID2String( routeID ).c_str(), rc ) ;

   done:
      PD_TRACE_EXITRC( SDB__TPSYNCMGR__SENDREGREQ, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__TPSYNCMGR__SENDTIMESYNCREQ, "_tpSyncManager::_sendTimeSyncReq" )
   INT32 _tpSyncManager::_sendTimeSyncReq( const MsgRouteID &routeID,
                                           UINT32 version,
                                           UINT16 flag,
                                           TP_SYNC_STATUS status,
                                           UINT32 timeError )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__TPSYNCMGR__SENDTIMESYNCREQ ) ;

      MsgTpTimeSyncReq request ;

      _fillRequestHeader( request.header, sizeof( MsgTpTimeSyncReq ),
                          MSG_TP_TIME_SYNC_REQ ) ;

      request.version = version ;
      request.flag = flag ;
      request.status = (UINT16)status ;
      request.sendTimeSec = 0LL ;
      request.sendTimeNanoSec = 0LL ;
      request.receiveTimeSec = 0LL ;
      request.receiveTimeNanoSec = 0LL ;
      request.timeError = timeError ;

      _setLastRequestID( request.header.requestID, version ) ;

      _onSyncReq( routeID ) ;

      rc = _netAgent->syncSendUDP( routeID, &request ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to send synchronize request to %s, "
                   "rc: %d", routeID2String( routeID ).c_str(), rc ) ;

   done:
      PD_TRACE_EXITRC( SDB__TPSYNCMGR__SENDTIMESYNCREQ, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__TPSYNCMGR_LAUNCHSYNC, "_tpSyncManager::launchSync" )
   INT32 _tpSyncManager::launchSync()
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__TPSYNCMGR_LAUNCHSYNC ) ;

      if ( _catalogManager->isSyncClient() )
      {
         MsgRouteID primaryRID ;
         tpClientNode local ;
         UINT32 version = TP_GROUP_INVALID_VERSION ;

         _catalogManager->getLocalAndVersion( local, version ) ;

         rc = _session.getPrimaryRID( primaryRID ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to get primary RID, rc: %d", rc ) ;

         if ( TP_SYNC_NOSOURCE == _status )
         {
            local.generateOID() ;
            rc = _sendRegReq( primaryRID, version, local ) ;
            PD_RC_CHECK( rc, PDERROR, "Failed to send register request, "
                         "rc: %d", rc ) ;
         }
         else
         {
            UINT32 timeError = local.getTimeError() ;
            UINT16 flag = TP_SYNC_TIME_FLAG_EMPTY ;
            if ( _canDecTimeError( timeError ) )
            {
               OSS_BIT_SET( flag, TP_SYNC_TIME_FLAG_DECTIMEERROR ) ;
            }
            rc = _sendTimeSyncReq( primaryRID, version, flag, _status,
                                   timeError ) ;
            PD_RC_CHECK( rc, PDERROR, "Failed to send synchronize request, "
                         "rc: %d", rc ) ;
         }

         goto done ;
      }

   done:
      PD_TRACE_EXITRC( SDB__TPSYNCMGR_LAUNCHSYNC, rc ) ;
      return rc ;

   error:
      restartSync() ;
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__TPSYNCMGR_RESTARTSYNC, "_tpSyncManager::restartSync" )
   INT32 _tpSyncManager::restartSync()
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__TPSYNCMGR_RESTARTSYNC ) ;

      activeStatus( TP_SYNC_NOSOURCE ) ;

      PD_TRACE_EXITRC( SDB__TPSYNCMGR_RESTARTSYNC, rc ) ;

      return rc ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__TPSYNCMGR_COMMITRECORD, "_tpSyncManager::commitRecord" )
   INT32 _tpSyncManager::commitRecord( const tpSyncRecord &record )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__TPSYNCMGR_COMMITRECORD ) ;

      PD_LOG( PDEVENT, "Received synchronize response: status %s [%d] "
              "synchronize record: %s",
              tpGetSyncStatusName( _status ), _status,
              record.toString().c_str() ) ;

      if ( !_checkRecord( record ) )
      {
         activeStatus( TP_SYNC_CHECKERROR ) ;
         goto done ;
      }

      _adjustTime( record ) ;

      switch ( _status )
      {
         case TP_SYNC_NOSOURCE :
         {
            activeStatus( TP_SYNC_CHECKOFFSET ) ;
            break ;
         }
         case TP_SYNC_CHECKOFFSET :
         case TP_SYNC_RECHECKOFFSET :
         {
            if ( _hasEnoughRecords() )
            {
               if ( TP_SYNC_CHECKOFFSET == _status )
               {
                  activeStatus( TP_SYNC_CHECKSLEWRATE ) ;
               }
               else
               {
                  activeStatus( TP_SYNC_INTERVALCHECK ) ;
               }
            }
            break ;
         }
         case TP_SYNC_CHECKSLEWRATE :
         {
            if ( _hasEnoughRecords() )
            {
               _adjustSlewRate( _syncRecords ) ;
               activeStatus( TP_SYNC_RECHECKOFFSET ) ;
            }
            break ;
         }
         case TP_SYNC_INTERVALCHECK :
         {
            // restart checking after 2 hour
            UINT64 normalPassed = pmdGetTickSpanTime( _lastNormalTick ) ;
            if ( normalPassed > TP_SYNC_RECHECK_INTERVAL )
            {
               activeStatus( TP_SYNC_CHECKOFFSET ) ;
            }
            break ;
         }
         case TP_SYNC_CHECKERROR :
         {
            activeStatus( TP_SYNC_CHECKOFFSET ) ;
            break ;
         }
         default :
         {
            PD_CHECK( FALSE, SDB_SYS, error, PDERROR, "Failed to process "
                      "synchronize record, unknown status [%d]", _status ) ;
            break ;
         }
      }

      PD_LOG( PDEVENT, "Done synchronize time: status %s [%d], "
              "meta data: %s, records: %u",
              tpGetSyncStatusName( _status ), _status,
              getMetaData()->toString().c_str(), _syncRecords.size() ) ;

   done:
      PD_TRACE_EXITRC( SDB__TPSYNCMGR_COMMITRECORD, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__TPSYNCMGR_GETCURRENTSYNCINTERVAL, "_tpSyncManager::getCurrentSyncInterval" )
   UINT64 _tpSyncManager::getCurrentSyncInterval()
   {
      UINT64 interval = 0LL ;

      PD_TRACE_ENTRY( SDB__TPSYNCMGR_GETCURRENTSYNCINTERVAL ) ;

      if ( _syncEvent )
      {
         _syncEvent = FALSE ;
         goto done ;
      }

      interval = OSS_ONE_SEC ;

      switch ( _status )
      {
         case TP_SYNC_NOSOURCE :
         case TP_SYNC_CHECKOFFSET :
         case TP_SYNC_RECHECKOFFSET :
         case TP_SYNC_CHECKERROR :
         {
            break ;
         }
         case TP_SYNC_CHECKSLEWRATE :
         {
            interval = TP_SLEW_RATE_CHECK_INTERVAL ;
            break ;
         }
         case TP_SYNC_INTERVALCHECK :
         {
            interval = (UINT64)( _options->getSyncInterval() ) * OSS_ONE_SEC ;
            break ;
         }
         default :
         {
            SDB_ASSERT( FALSE, "status is invalid" ) ;
            break ;
         }
      }

   done:
      PD_TRACE_EXIT( SDB__TPSYNCMGR_GETCURRENTSYNCINTERVAL ) ;
      return interval ;
   }

   void _tpSyncManager::activeStatus( TP_SYNC_STATUS status )
   {
      _syncRecords.clear() ;
      _status = status ;

      if ( TP_SYNC_INTERVALCHECK == status )
      {
         if ( 0LL == _lastNormalTick )
         {
            _lastNormalTick = pmdGetDBTick() ;
         }
      }
      else
      {
         _lastNormalTick = 0LL ;
      }
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__TPSYNCMGR__ADJUSTTIME, "_tpSyncManager::_adjustTime" )
   INT32 _tpSyncManager::_adjustTime( const tpSyncRecord &record )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__TPSYNCMGR__ADJUSTTIME ) ;

      try
      {
         if ( _hasEnoughRecords() )
         {
            _syncRecords.pop_front() ;
         }
         _syncRecords.push_back( record ) ;
      }
      catch ( exception &e )
      {
         // ignore error
         PD_LOG( PDWARNING, "Failed to save synchronize record, occurred "
                 "error: %s", e.what() ) ;
      }

      getMetaData()->adjustLogicalTime( record.getOffset(),
                                        record.getRspTimeError() ) ;
      _catalogManager->syncLocal( record.getRspTimeError() ) ;

      PD_TRACE_EXITRC( SDB__TPSYNCMGR__ADJUSTTIME, rc ) ;

      return rc ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__TPSYNCMGR__ADJUSTSLEWRATE, "_tpSyncManager::_adjustSlewRate" )
   void _tpSyncManager::_adjustSlewRate( const TP_SYNC_REC_LIST &records )
   {
      PD_TRACE_ENTRY( SDB__TPSYNCMGR__ADJUSTSLEWRATE ) ;

      INT64 totalOffset = 0LL ;
      INT64 averageOffset = 0LL ;
      INT64 recordSize = (INT64)( records.size() ) ;
      INT64 recordTime = (INT64)( OSS_ONE_BILLION ) * recordSize ;

      for ( TP_SYNC_REC_LIST::const_iterator iter = records.begin() ;
            iter != records.end() ;
            iter ++ )
      {
         totalOffset += iter->getOffset() ;
      }

      averageOffset = totalOffset / recordSize ;
      if ( averageOffset <= TP_SYNC_OFFSET_MIN_LIMIT ||
           averageOffset >= TP_SYNC_OFFSET_MAX_LIMIT )
      {
         getMetaData()->adjustSlewRate( recordTime + averageOffset,
                                        recordTime ) ;
      }

      PD_TRACE_EXIT( SDB__TPSYNCMGR__ADJUSTSLEWRATE ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__TPSYNCMGR__CANDECTIMEERROR, "_tpSyncManager::_canDecTimeError" )
   BOOLEAN _tpSyncManager::_canDecTimeError( UINT32 curTimeError )
   {
      BOOLEAN canDecrease = FALSE ;

      PD_TRACE_ENTRY( SDB__TPSYNCMGR__COULDDECTIMEERROR ) ;

      if ( _hasEnoughRecords() )
      {
         UINT32 count = (UINT32)( _syncRecords.size() ) ;
         UINT64 totalDelay = 0LL ;
         UINT64 averageDelay = 0LL ;

         // decrease target, check against with 2 times of decrease step
         UINT32 decTimeError =
               tpClientNode::getDecTimeError( curTimeError,
                                               TP_MIN_TIME_ERROR, 2 ) ;
         for ( TP_SYNC_REC_LIST::const_iterator iter = _syncRecords.begin() ;
               iter != _syncRecords.end() ;
               iter ++ )
         {
            // in below case, will not decrease time error
            // 1. delay is smaller than 0, means network is asymmetric
            // 2. one delay is larger than target time error
            if ( iter->getDelay() < 0 ||
                 iter->getDelay() >= (INT64)decTimeError )
            {
               goto done ;
            }
            totalDelay += iter->getDelay() ;
         }
         averageDelay = (UINT64)( (double)totalDelay / (double)count ) ;
         if ( averageDelay < (UINT64)decTimeError )
         {
            canDecrease = TRUE ;
         }
         PD_LOG( PDDEBUG, "Check time error, average delay [%llu],"
                 "current time error [%u], decreased time error [%u], "
                 "can decrease: %s", averageDelay, curTimeError, decTimeError,
                 canDecrease ? "TRUE" : "FALSE" ) ;
      }

   done:
      PD_TRACE_EXIT( SDB__TPSYNCMGR__COULDDECTIMEERROR ) ;
      return canDecrease ;
   }


   // PD_TRACE_DECLARE_FUNCTION ( SDB__TPSYNCMGR_REGSOURCE, "_tpSyncManager::registerSource" )
   INT32 _tpSyncManager::registerSource( const tpSourceNode &source )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__TPSYNCMGR_REGSOURCE ) ;

      ossScopedRWLock lock( ( &_sourceMutex ), EXCLUSIVE ) ;

      try
      {
         TP_SOURCE_MAP::iterator iter = _sources.find( source.getRouteID() ) ;
         if ( iter != _sources.end() )
         {
            iter->second.onRegister() ;
         }
         else
         {
            _sources.insert( make_pair( source.getRouteID(), source ) ) ;
         }
      }
      catch ( exception &e )
      {
         PD_LOG( PDERROR, "Failed to register source, "
                 "occurred unexpected error: %s", e.what() ) ;
         rc = SDB_SYS ;
         goto error ;
      }

      PD_LOG( PDEVENT, "Register source node %s done",
              source.toString().c_str() ) ;

   done:
      PD_TRACE_EXITRC( SDB__TPSYNCMGR_REGSOURCE, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__TPSYNCMGR_GETSOURCE, "_tpSyncManager::getSource" )
   INT32 _tpSyncManager::getSource( const MsgRouteID &routeID,
                                    tpSourceNode &source )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__TPSYNCMGR_GETSOURCE ) ;

      ossScopedRWLock lock( ( &_sourceMutex ), SHARED ) ;

      TP_SOURCE_MAP::const_iterator iter = _sources.find( routeID ) ;
      PD_CHECK( iter != _sources.end(), SDB_INVALID_ROUTEID, error, PDERROR,
                "Failed to get source node %s, it is not found",
                routeID2String( routeID ).c_str() ) ;

      source = iter->second ;

   done:
      PD_TRACE_EXITRC( SDB__TPSYNCMGR_GETSOURCE, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__TPSYNCMGR_UPDATESOURCE, "_tpSyncManager::updateSource" )
   INT32 _tpSyncManager::updateSource( const tpSourceNode &source )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__TPSYNCMGR_UPDATESOURCE ) ;

      ossScopedRWLock lock( &_sourceMutex, EXCLUSIVE ) ;

      TP_SOURCE_MAP::iterator iter = _sources.find( source.getRouteID() ) ;
      PD_CHECK( iter != _sources.end(), SDB_INVALID_ROUTEID, error, PDERROR,
                "Failed to find source node %s", source.toString().c_str() ) ;

      iter->second = source ;

      PD_LOG( PDEVENT, "Update source node %s done",
              source.toString().c_str() ) ;

   done:
      PD_TRACE_EXITRC( SDB__TPSYNCMGR_UPDATESOURCE, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__TPSYNCMGR_REMOVESOURCE, "_tpSyncManager::removeSource" )
   INT32 _tpSyncManager::removeSource( const MsgRouteID &routeID )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__TPSYNCMGR_REMOVESOURCE ) ;

      ossScopedRWLock lock( ( &_sourceMutex ), EXCLUSIVE ) ;

      TP_SOURCE_MAP::iterator iter = _sources.find( routeID ) ;

      PD_CHECK( iter != _sources.end(),
                SDB_INVALID_ROUTEID, error, PDWARNING,
                "Failed to remove source node %s, route ID is not found",
                routeID2String( routeID ).c_str() ) ;

      _sources.erase( iter ) ;

      PD_LOG( PDEVENT, "Remove source node %s done",
              routeID2String( routeID ).c_str() ) ;

   done:
      PD_TRACE_EXITRC( SDB__TPSYNCMGR_REMOVESOURCE, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__TPSYNCMGR_REMOVESOURCE_EXPIRED, "_tpSyncManager::removeSource" )
   INT32 _tpSyncManager::removeSource( const MsgRouteID &routeID,
                                       UINT64 expiredTick )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__TPSYNCMGR_REMOVESOURCE_EXPIRED ) ;

      ossScopedRWLock lock( ( &_sourceMutex ), EXCLUSIVE ) ;

      TP_SOURCE_MAP::iterator iter = _sources.find( routeID ) ;

      PD_CHECK( iter != _sources.end(),
                SDB_INVALID_ROUTEID, error, PDWARNING,
                "Failed to remove source node %s, route ID is not found",
                routeID2String( routeID ).c_str() ) ;

      if ( iter->second.getLastSyncTick() <= expiredTick )
      {
         _sources.erase( iter ) ;
      }
      else
      {
         PD_LOG( PDDEBUG, "Ignored remove expired source node %s, "
                 "synchronize time is updated",
                 routeID2String( routeID ).c_str() ) ;
      }

      PD_LOG( PDEVENT, "Remove expired source node %s done",
              routeID2String( routeID ).c_str() ) ;

   done:
      PD_TRACE_EXITRC( SDB__TPSYNCMGR_REMOVESOURCE_EXPIRED, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__TPSYNCMGR_DUMPSOURCES, "_tpSyncManager::dumpSources" )
   INT32 _tpSyncManager::dumpSources( TP_SOURCE_MAP &sources )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__TPSYNCMGR_DUMPSOURCES ) ;

      ossScopedRWLock lock( ( &_sourceMutex ), SHARED ) ;

      try
      {
         sources = _sources ;
      }
      catch ( exception &e )
      {
         PD_LOG( PDERROR, "Failed to dump sources, "
                 "occurred unexpected error: %s", e.what() ) ;
         rc = SDB_SYS ;
         goto error ;
      }

   done:
      PD_TRACE_EXITRC( SDB__TPSYNCMGR_DUMPSOURCES, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__TPSYNCMGR__ONREGRSP, "_tpSyncManager::_onRegRsp" )
   INT32 _tpSyncManager::_onRegRsp( const MsgRouteID &routeID )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__TPSYNCMGR__ONREGRSP ) ;

      tpSourceNode source ;

      rc = _catalogManager->getServer( routeID, source ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to get server %s, rc: %d",
                   routeID2String( routeID ).c_str(), rc ) ;

      rc = registerSource( source ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to register source %s, rc: %d",
                   routeID2String( routeID ).c_str(), rc ) ;

   done:
      PD_TRACE_EXITRC( SDB__TPSYNCMGR__ONREGRSP, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__TPSYNCMGR__ONSYNCREQ, "_tpSyncManager::_onSyncReq" )
   INT32 _tpSyncManager::_onSyncReq( const MsgRouteID &routeID )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__TPSYNCMGR__ONSYNCREQ ) ;

      tpSourceNode source ;

      rc = getSource( routeID, source ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to get source %s, rc: %d",
                   routeID2String( routeID ).c_str(), rc ) ;

      source.onPreSync() ;

      rc = updateSource( source ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to update source %s, rc: %d",
                   source.toString().c_str(), rc ) ;

   done:
      PD_TRACE_EXITRC( SDB__TPSYNCMGR__ONSYNCREQ, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__TPSYNCMGR__ONSYNCRSP, "_tpSyncManager::_onSyncRsp" )
   INT32 _tpSyncManager::_onSyncRsp( const MsgRouteID &routeID,
                                     const tpSyncRecord &record,
                                     BOOLEAN isValid )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__TPSYNCMGR__ONSYNCRSP ) ;

      tpSourceNode source ;

      rc = getSource( routeID, source ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to get source %s, rc: %d",
                   routeID2String( routeID ).c_str(), rc ) ;

      source.onPostSync( record, isValid ) ;

      rc = updateSource( source ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to update source %s, rc: %d",
                   source.toString().c_str(), rc ) ;

   done:
      PD_TRACE_EXITRC( SDB__TPSYNCMGR__ONSYNCRSP, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__TPSYNCMGR__CLEAREXPIREDSOURCES, "_tpSyncManager::_clearExpiredSources" )
   INT32 _tpSyncManager::_clearExpiredSources()
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__TPSYNCMGR__CLEAREXPIREDSOURCES ) ;

      TP_SOURCE_MAP sources ;

      rc = dumpSources( sources ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to dump sources, rc: %d", rc ) ;

      for ( TP_SOURCE_MAP::iterator iter = sources.begin() ;
            iter != sources.end() ;
            ++ iter )
      {
         UINT64 syncTick = iter->second.getLastSyncTick() ;
         UINT64 syncPassed = pmdGetTickSpanTime( syncTick ) ;
         if ( syncPassed > TP_CLEAR_SOURCE_INTERVAL )
         {
            removeSource( iter->first, syncTick ) ;
         }
      }

   done:
      PD_TRACE_EXITRC( SDB__TPSYNCMGR__CLEAREXPIREDSOURCES, rc ) ;
      return rc ;

   error:
      goto done ;
   }

}
