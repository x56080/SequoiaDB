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

   Source File Name = stpSyncClientManager.cpp

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
#include "stpSyncClientManager.hpp"
#include "stpCB.hpp"
#include "stpNode.hpp"
#include "pdTrace.hpp"
#include "stpTrace.hpp"
#include "pmd.hpp"
#include "msgMessage.hpp"

namespace engine
{

   // interval to clear expired sources ( without synchronize in 2 hours )
   #define STP_CLEAR_SOURCE_INTERVAL   ( STP_CLEAR_SYNCHRONIZE_INTERVAL )

   // to restart synchronize time to check-offset status in 2 hours
   #define STP_SYNC_RECHECK_INTERVAL   ( STP_SEC_TO_MILLISEC( 2 * 3600 ) )

   // maximum number of records to be saved as history
   #define STP_SYNC_RECORD_CACHE_SIZE  ( 10 )

   // maximum retry times for slew rate checking
   #define STP_MAX_SLEW_RATE_CHECK_TIMES ( STP_SYNC_RECORD_CACHE_SIZE * 2 )

   // synchronize record with offset in valid range means the offset is too
   // trivial to adjust slew rate
   // maximum valid offset to slew rate check
   #define STP_SLEWRATE_OFFSET_MAX_LIMIT     ( 100000L )
   // minimum valid offset to slew rate check
   #define STP_SLEWRATE_OFFSET_MIN_LIMIT     ( -100000L )

   // maximum limit for standard deviation for slew rate check
   #define STP_SLEWRATE_DEVIATION_MAX_LIMIT  ( STP_SLEWRATE_OFFSET_MAX_LIMIT * 2 )

   // interval ( in milliseconds ) to each slew rate check by synchronize time
   // request ( now send request for each 10 seconds )
   #define STP_SLEWRATE_CHECK_INTERVAL       ( STP_SEC_TO_MILLISEC( 10 ) )
   // interval ( in nanoseconds ) to each slew rate check by synchronize time
   // request
   #define STP_SLEWRATE_CHECK_INTARVAL_NS    \
                     ( STP_MILLISEC_TO_NANOSEC( STP_SLEWRATE_CHECK_INTERVAL ) )

   /*
      _stpSyncClientManager implement
    */
   BEGIN_OBJ_MSG_MAP( _stpSyncClientManager, _stpManagerBase )
      // ON_MSG
      ON_MSG( MSG_STP_REG_RSP, processMessage )
      ON_MSG( MSG_STP_TIME_SYNC_RSP, processMessage )
   END_OBJ_MSG_MAP()

   _stpSyncClientManager::_stpSyncClientManager( STPCB *stpCB )
   : stpManagerBase( stpCB ),
     _status( STP_SYNC_NOSOURCE ),
     _curStatusCount( 0 ),
     _syncEvent( FALSE ),
     _lastRequestID( 0LL ),
     _lastVersion( STP_GROUP_INVALID_VERSION ),
     _lastStableTick( 0LL ),
     _syncTimeTimeout( 0LL ),
     _waitSyncRsp( FALSE ),
     _sourceClearTimeout( 0LL )
   {
      _regSourceRID.value = MSG_INVALID_ROUTEID ;
      _syncSourceRID.value = MSG_INVALID_ROUTEID ;
   }

   _stpSyncClientManager::~_stpSyncClientManager()
   {
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__TPSYNCCLIENTMGR_ONTIMER, "_stpSyncClientManager::onTimer" )
   void _stpSyncClientManager::onTimer( UINT64 timerID, UINT32 interval )
   {
      PD_TRACE_ENTRY( SDB__TPSYNCCLIENTMGR_ONTIMER ) ;

      if ( timerID == _timerID )
      {
         if ( _waitSyncRsp )
         {
            PD_LOG( PDWARNING, "Failed to wait for synchronize response, "
                    "timeout, restart synchronize" ) ;
            restartSync() ;
            _waitSyncRsp = FALSE ;
         }

         // check timeout to synchronize time
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

         // check timeout to clear expired sources
         // ( no synchronize for a long time, e.g. 2 hours )
         _sourceClearTimeout += interval ;
         if ( _sourceClearTimeout >= STP_CLEAR_SOURCE_INTERVAL )
         {
            _clearExpiredSources() ;
            _sourceClearTimeout = 0LL ;
         }
      }

      PD_TRACE_EXIT( SDB__TPSYNCCLIENTMGR_ONTIMER ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__TPSYNCCLIENTMGR_PROCESSMESSAGE, "_stpSyncClientManager::processMessage" )
   INT32 _stpSyncClientManager::processMessage( NET_HANDLE handle,
                                                MsgHeader *message )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__TPSYNCCLIENTMGR_PROCESSMESSAGE ) ;

      SDB_ASSERT( NULL != message, "message is invalid" ) ;

      switch ( message->opCode )
      {
         case MSG_STP_REG_RSP :
         {
            // handle register response
            rc = _handleRegRsp( handle, (const stpRegRsp *)message ) ;
            PD_RC_CHECK( rc, PDERROR, "Failed to handle register response, "
                         "rc: %d", rc ) ;
            break ;
         }
         case MSG_STP_TIME_SYNC_RSP :
         {
            // handle synchronzie time response
            rc = _handleTimeSyncRsp( handle,
                                     (const stpTimeSyncRsp *)message ) ;
            PD_RC_CHECK( rc, PDERROR, "Failed to handle synchronize time "
                         "response, rc: %d", rc ) ;
            break ;
         }
         default :
         {
            // unknown message
            PD_CHECK( FALSE, SDB_UNKNOWN_MESSAGE, error, PDERROR,
                      "Unknown synchronize client message [%d]",
                      message->opCode ) ;
            break ;
         }
      }

   done:
      PD_TRACE_EXITRC( SDB__TPSYNCCLIENTMGR_PROCESSMESSAGE, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__TPSYNCCLIENTMGR__POSTACTIVATE, "_stpSyncClientManager::_postActivate" )
   INT32 _stpSyncClientManager::_postActivate()
   {
      PD_TRACE_ENTRY( SDB__TPSYNCCLIENTMGR__POSTACTIVATE ) ;

      // launch synchronize after activated
      _syncEvent = FALSE ;
      launchSync() ;

      PD_TRACE_EXITRC( SDB__TPSYNCCLIENTMGR__POSTACTIVATE, SDB_OK ) ;

      return SDB_OK ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__TPSYNCCLIENTMGR_ONSENDTIMESYNCREQ, "_stpSyncClientManager::onSendTimeSyncReq" )
   INT32 _stpSyncClientManager::onSendTimeSyncReq( stpTimeSyncReq *request )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__TPSYNCCLIENTMGR_ONSENDTIMESYNCREQ ) ;

      SDB_ASSERT( NULL != request, "request message is invalid" ) ;
      SDB_ASSERT( MSG_STP_TIME_SYNC_REQ == request->header.opCode,
                  "opcode of message is invalid" ) ;

      // set the send time on receiving synchronize time request
      stpHPTime sendTime = getMetaData()->getLTValue() ;
      request->sendTimeSec = sendTime.getSecond() ;
      request->sendTimeNanoSec = sendTime.getNanoSecond() ;

      PD_TRACE_EXITRC( SDB__TPSYNCCLIENTMGR_ONSENDTIMESYNCREQ, rc ) ;

      return rc ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__TPSYNCCLIENTMGR_ONRECEIVETIMESYNCRSP, "_stpSyncClientManager::onReceiveTimeSyncRsp" )
   INT32 _stpSyncClientManager::onReceiveTimeSyncRsp( stpTimeSyncRsp *response )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__TPSYNCCLIENTMGR_ONRECEIVETIMESYNCRSP ) ;

      SDB_ASSERT( NULL != response, "response message is invalid" ) ;
      SDB_ASSERT( MSG_STP_TIME_SYNC_RSP == response->reply.header.opCode,
                  "opcode of message is invalid" ) ;

      // set the receive time on receiving synchronize time response
      stpHPTime receiveTime = getMetaData()->getLTValue() ;
      response->rspReceiveTimeSec = receiveTime.getSecond() ;
      response->rspReceiveTimeNanoSec = receiveTime.getNanoSecond() ;

      PD_TRACE_EXITRC( SDB__TPSYNCCLIENTMGR_ONRECEIVETIMESYNCRSP, rc ) ;

      return rc ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__TPSYNCCLIENTMGR__HANDLEREGRSP, "_stpSyncClientManager::_handleRegRsp" )
   INT32 _stpSyncClientManager::_handleRegRsp( NET_HANDLE handle,
                                               const stpRegRsp *response )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__TPSYNCCLIENTMGR__HANDLEREGRSP ) ;

      SDB_ASSERT( NULL != response, "response message is invalid" ) ;
      SDB_ASSERT( MSG_STP_REG_RSP == response->reply.header.opCode,
                  "opcode of message is invalid" ) ;

      // get return code of response
      rc = response->reply.res ;

      // check if we need to update servers
      // We could still finish the register even if the group version is
      // expired, we only need primary for the synchronization, just tell the
      // node manager to update servers
      if ( SDB_REPL_REMOTE_G_V_EXPIRED == rc )
      {
         // check with last version only, if group version is already updated,
         // and last version already updated by later requests, this check will
         // not launch server checks
         _nodeManager->checkExpiredVersion( _lastVersion ) ;
         rc = SDB_OK ;
      }

      // check return code of response
      if ( SDB_OK == rc )
      {
         // check request ID, if it is expired, ignore this response
         UINT64 responseRequestID = response->reply.header.requestID ;
         UINT64 lastRequestID = _lastRequestID.fetch() ;
         if ( responseRequestID != lastRequestID )
         {
            PD_LOG( PDWARNING, "Received expired register "
                    "request ID [%llu], expecting request ID [%llu]",
                    responseRequestID, lastRequestID ) ;
            goto done ;
         }

         // on event of register response, register a source node to save
         // synchronize history
         rc = _onRegRsp( response->reply.header.routeID, response->port ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to call event on register, rc: %d",
                      rc ) ;

         // update verified OID of local node
         _nodeManager->updateLocalSyncInfo( response->routeID,
                                            response->oid,
                                            response->port ) ;

         // active check-offset status to start synchronize time
         activeStatus( STP_SYNC_CHECKOFFSET ) ;
      }
      else
      {
         // handle error response
         if ( SDB_CLS_NOT_PRIMARY == rc )
         {
            // not primary, reset primary to node manager
            _nodeManager->resetPrimary() ;
         }
         PD_RC_CHECK( rc, PDERROR, "Failed to register node, "
                      "received response with error: %d", rc ) ;
      }

   done:
      PD_TRACE_EXITRC( SDB__TPSYNCCLIENTMGR__HANDLEREGRSP, rc ) ;
      return rc ;

   error:
      restartSync() ;
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__TPSYNCCLIENTMGR__HANDLETIMESYNCRSP, "_stpSyncClientManager::_handleTimeSyncRsp" )
   INT32 _stpSyncClientManager::_handleTimeSyncRsp(
                                             NET_HANDLE handle,
                                             const stpTimeSyncRsp *response )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__TPSYNCCLIENTMGR__HANDLETIMESYNCRSP ) ;

      SDB_ASSERT( NULL != response, "response is invalid" ) ;
      SDB_ASSERT( MSG_STP_TIME_SYNC_RSP == response->reply.header.opCode,
                  "opcode of message is invalid" ) ;

      // reset wait synchronize response
      _waitSyncRsp = FALSE ;

      // only synchronize client to handle synchronize time response
      PD_CHECK( _nodeManager->isSyncClient(), SDB_OK, done, PDWARNING,
                "Failed to handle synchronize time response, current node is "
                "not synchronizing client" ) ;

      // get return code of response
      rc = response->reply.res ;

      // check if we need to update servers
      // We could still finish the time synchronization even if the group
      // version is expired, we only need primary for the synchronization,
      // just tell the node manager to update servers
      if ( SDB_REPL_REMOTE_G_V_EXPIRED == rc )
      {
         // check with last version only, if group version is already updated,
         // and last version already updated by later requests, this check will
         // not launch server checks
         _nodeManager->checkExpiredVersion( _lastVersion ) ;
         rc = SDB_OK ;
      }

      // check return code of response
      if ( SDB_OK == rc )
      {
         stpSyncRecord record( response, getStatus() ) ;

         // check request ID, if it is expired, ignore this response
         UINT64 lastRequestID = _lastRequestID.fetch() ;
         if (  record.getRequestID() != lastRequestID )
         {
            PD_LOG( PDWARNING, "Received expired synchronize "
                    "request ID [%llu], expecting request ID [%llu]",
                    record.getRequestID(), lastRequestID ) ;
            goto done ;
         }

         // commit synchronize record to adjust time
         rc = commitRecord( record ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to commit synchronize record, "
                      "rc: %d", rc ) ;

         // save synchronize record to history of source
         _onSyncRsp( response->reply.header.routeID, record,
                     STP_SYNC_CHECKERROR != _status ) ;

         // for check-error status, re-send synchronize request immediately
         // - to increase time error
         // - to get network bandwidth
         if ( STP_SYNC_CHECKERROR == _status )
         {
            stpClientNode local ;
            UINT32 version = STP_GROUP_INVALID_VERSION ;

            // get local node and group version
            _nodeManager->getLocalAndVersion( local, version ) ;

            // send time synchronize request
            // - indicate to increase time error
            // - tell source the delay of last synchronize round-trip
            _sendTimeSyncReq( _syncSourceRID,
                              version,
                              STP_SYNC_TIME_FLAG_INCTIMEERROR,
                              STP_SYNC_CHECKERROR,
                              OSS_MIN( record.getDelay(),
                                       local.getMaxTimeError() ) ) ;
         }
      }
      else
      {
         // for other errors
         if ( SDB_CLS_NOT_PRIMARY == rc )
         {
            // not primary, reset primary to node manager
            _nodeManager->resetPrimary() ;
         }
         PD_RC_CHECK( rc, PDERROR, "Failed to synchronize time, "
                      "received response with error: %d", rc ) ;
      }

   done:
      PD_TRACE_EXITRC( SDB__TPSYNCCLIENTMGR__HANDLETIMESYNCRSP, rc ) ;
      return rc ;

   error:
      // let's restart
      restartSync() ;
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__TPSYNCCLIENTMGR__SENDREGREQ, "_stpSyncClientManager::_sendRegReq" )
   INT32 _stpSyncClientManager::_sendRegReq( const MsgRouteID &routeID,
                                             UINT32 version,
                                             const bson::BSONObj &regObject )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__TPSYNCCLIENTMGR__SENDREGREQ ) ;

      stpRegReq request ;
      UINT32 requestSize = sizeof( stpRegReq ) + regObject.objsize() ;

      // fill request header
      _netMsgHandler->fillRequestHeader( request.header,
                                         requestSize,
                                         MSG_STP_REG_REQ ) ;

      // fill fields for register request
      request.version = version ;

      // update last request ID
      _setLastRequestID( request.header.requestID, version ) ;

      // send by net agent
      rc = _netAgent->syncSend( routeID,
                                (MsgHeader *)( &request ),
                                (void *)( regObject.objdata() ),
                                regObject.objsize() ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to send register request to %s, "
                   "rc: %d", routeID2String( routeID ).c_str(), rc ) ;

   done:
      PD_TRACE_EXITRC( SDB__TPSYNCCLIENTMGR__SENDREGREQ, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__TPSYNCCLIENTMGR__SENDTIMESYNCREQ, "_stpSyncClientManager::_sendTimeSyncReq" )
   INT32 _stpSyncClientManager::_sendTimeSyncReq( const MsgRouteID &routeID,
                                                  UINT32 version,
                                                  UINT16 flag,
                                                  STP_SYNC_STATUS status,
                                                  UINT32 timeError )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__TPSYNCCLIENTMGR__SENDTIMESYNCREQ ) ;

      stpTimeSyncReq request ;

      // fill request header
      _netMsgHandler->fillRequestHeader( request.header,
                                         sizeof( stpTimeSyncReq ),
                                         MSG_STP_TIME_SYNC_REQ ) ;

      // fill fields of synchronize time request
      request.version = version ;
      request.flag = flag ;
      request.status = (UINT16)status ;
      // send time will be filled in callback, set 0 here
      request.sendTimeSec = 0LL ;
      request.sendTimeNanoSec = 0LL ;
      // receive time will be filled by source, set 0 here
      request.receiveTimeSec = 0LL ;
      request.receiveTimeNanoSec = 0LL ;
      request.timeError = timeError ;

      // set last request ID
      _setLastRequestID( request.header.requestID, version ) ;

      // on sending request event: save synchronize history
      _onSyncReq( _regSourceRID ) ;

      // send by net agent with UDP
      rc = _netAgent->syncSendUDP( routeID, &request ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to send synchronize request to %s, "
                   "rc: %d", routeID2String( routeID ).c_str(), rc ) ;

      // set waiting for synchronize response
      _waitSyncRsp = TRUE ;

   done:
      PD_TRACE_EXITRC( SDB__TPSYNCCLIENTMGR__SENDTIMESYNCREQ, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__TPSYNCCLIENTMGR_LAUNCHSYNC, "_stpSyncClientManager::launchSync" )
   INT32 _stpSyncClientManager::launchSync()
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__TPSYNCCLIENTMGR_LAUNCHSYNC ) ;

      // only synchronize client could start time synchronize
      if ( _nodeManager->isSyncClient() )
      {
         stpClientNode localNode ;
         UINT32 version = STP_GROUP_INVALID_VERSION ;

         // get local node and group version
         _nodeManager->getLocalAndVersion( localNode, version ) ;

         if ( STP_SYNC_NOSOURCE == _status )
         {
            MsgRouteID primaryRID ;

            // get primary server as source
            rc = _session.getPrimaryRID( primaryRID ) ;
            PD_RC_CHECK( rc, PDERROR, "Failed to get primary RID, rc: %d", rc ) ;

            // current is no source status, means we are first time to
            // synchronize to the node in this round, send register request
            // first
            rc = _launchRegister( primaryRID, version, localNode ) ;
            PD_RC_CHECK( rc, PDERROR, "Failed to launch register, rc: %d",
                         rc ) ;
         }
         else
         {
            MsgRouteID sourceRID ;
            if ( MSG_INVALID_ROUTEID == _syncSourceRID.value )
            {
               // get primary server as source
               rc = _session.getPrimaryRID( sourceRID ) ;
               PD_RC_CHECK( rc, PDERROR, "Failed to get primary RID, "
                            "rc: %d", rc ) ;
            }
            else
            {
               // source ID is valid, use it
               sourceRID.value = _syncSourceRID.value ;
            }

            // for other status, send synchronize time request
            rc = _launchTimeSync( sourceRID, version, localNode ) ;
            PD_RC_CHECK( rc, PDERROR, "Failed to launch time synchronize, "
                         "rc: %d", rc ) ;
         }
      }

   done:
      PD_TRACE_EXITRC( SDB__TPSYNCCLIENTMGR_LAUNCHSYNC, rc ) ;
      return rc ;

   error:
      // restart synchronize when error happened
      restartSync() ;
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__TPSYNCCLIENTMGR_RESTARTSYNC, "_stpSyncClientManager::restartSync" )
   INT32 _stpSyncClientManager::restartSync()
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__TPSYNCCLIENTMGR_RESTARTSYNC ) ;

      // set status to no source to restart synchronize
      activeStatus( STP_SYNC_NOSOURCE ) ;

      PD_TRACE_EXITRC( SDB__TPSYNCCLIENTMGR_RESTARTSYNC, rc ) ;

      return rc ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__TPSYNCCLIENTMGR_COMMITRECORD, "_stpSyncClientManager::commitRecord" )
   INT32 _stpSyncClientManager::commitRecord( const stpSyncRecord &record )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__TPSYNCCLIENTMGR_COMMITRECORD ) ;

      PD_LOG( PDEVENT, "Received synchronize response: status %s [%d] "
              "synchronize record: %s",
              stpGetSyncStatusName( _status ), _status,
              record.toString().c_str() ) ;

      // check if record is valid ( delay <= time error )
      if ( !_checkRecord( record ) )
      {
         // it is not valid, active check error status
         activeStatus( STP_SYNC_CHECKERROR ) ;
         goto done ;
      }

      ++ _curStatusCount ;

      // adjust time by synchronize record
      _adjustTime( record ) ;

      switch ( _status )
      {
         case STP_SYNC_NOSOURCE :
         {
            // got response from no-source status, active check-offset status
            activeStatus( STP_SYNC_CHECKOFFSET ) ;
            break ;
         }
         case STP_SYNC_CHECKOFFSET :
         case STP_SYNC_RECHECKOFFSET :
         {
            // for check-offset and recheck-offset status, if we have enough
            // results, we could active the next status
            if ( _hasEnoughRecords() )
            {
               if ( STP_SYNC_CHECKOFFSET == _status )
               {
                  // active check-slew-rate status for check-offset status
                  activeStatus( STP_SYNC_CHECKSLEWRATE ) ;
               }
               else
               {
                  // active interval-check status for recheck-offset status
                  activeStatus( STP_SYNC_INTERVALCHECK ) ;
               }
            }
            break ;
         }
         case STP_SYNC_CHECKSLEWRATE :
         {
            // for check-slew-rate status, if we have enough results,
            // adjust slew rate, and active the recheck-offset status
            if ( _hasEnoughRecords() &&
                 _adjustSlewRate( _syncRecords ) )
            {
               // active recheck-offset status
               activeStatus( STP_SYNC_RECHECKOFFSET ) ;
            }
            break ;
         }
         case STP_SYNC_INTERVALCHECK :
         {
            // for interval-check status, restart checking after 2 hour
            // if STP is deployed in virtual machine, we have need to check
            // slew-rate after a period of interval-check ( here we go to
            // check-offset directly )
            UINT64 normalPassed = pmdGetTickSpanTime( _lastStableTick ) ;
            if ( normalPassed > STP_SYNC_RECHECK_INTERVAL )
            {
               // active check-offset status
               activeStatus( STP_SYNC_CHECKOFFSET ) ;
            }
            break ;
         }
         case STP_SYNC_CHECKERROR :
         {
            // for check-error status, we received a valid synchonize record,
            // so we could stop check-error status, and active check-offset
            // status
            activeStatus( STP_SYNC_CHECKOFFSET ) ;
            break ;
         }
         default :
         {
            // unknown status
            PD_CHECK( FALSE, SDB_SYS, error, PDERROR, "Failed to process "
                      "synchronize record, unknown status [%d]", _status ) ;
            break ;
         }
      }

      PD_LOG( PDEVENT, "Done synchronize time: status %s [%d], "
              "meta data: %s, records: %u",
              stpGetSyncStatusName( _status ), _status,
              getMetaData()->toString().c_str(), _syncRecords.size() ) ;

   done:
      PD_TRACE_EXITRC( SDB__TPSYNCCLIENTMGR_COMMITRECORD, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__TPSYNCCLIENTMGR_GETCURRENTSYNCINTERVAL, "_stpSyncClientManager::getCurrentSyncInterval" )
   UINT64 _stpSyncClientManager::getCurrentSyncInterval()
   {
      UINT64 interval = 0LL ;

      PD_TRACE_ENTRY( SDB__TPSYNCCLIENTMGR_GETCURRENTSYNCINTERVAL ) ;

      // if synchronize event is signaled, launch synchronize immediately,
      // and reset it to avoid launch synchronize too frequently
      if ( _syncEvent )
      {
         _syncEvent = FALSE ;
         goto done ;
      }

      // default interval is one second
      interval = OSS_ONE_SEC ;

      switch ( _status )
      {
         case STP_SYNC_NOSOURCE :
         case STP_SYNC_CHECKOFFSET :
         case STP_SYNC_RECHECKOFFSET :
         case STP_SYNC_CHECKERROR :
         {
            // for no-source, check-offset, recheck-offset, check-status
            // to synchronize interval is one second
            break ;
         }
         case STP_SYNC_CHECKSLEWRATE :
         {
            // for check-slew-rate status, the interval is 10 seconds
            interval = STP_SLEWRATE_CHECK_INTERVAL ;
            break ;
         }
         case STP_SYNC_INTERVALCHECK :
         {
            // for interval-check status, the interval is specified by
            // configuration
            interval = (UINT64)( _options->getSyncInterval() ) * OSS_ONE_SEC ;
            break ;
         }
         default :
         {
            // unknown status
            SDB_ASSERT( FALSE, "status is invalid" ) ;
            break ;
         }
      }

   done:
      PD_TRACE_EXIT( SDB__TPSYNCCLIENTMGR_GETCURRENTSYNCINTERVAL ) ;
      return interval ;
   }

   void _stpSyncClientManager::activeStatus( STP_SYNC_STATUS status )
   {
      // clear saved synchronize records
      _syncRecords.clear() ;

      // set status
      _status = status ;
      // restart counting
      _curStatusCount = 0 ;

      // if interval-check status is activated, set last stable tick which is
      // used to test if we need to restart checking phases periodically
      if ( STP_SYNC_INTERVALCHECK == status )
      {
         if ( 0LL == _lastStableTick )
         {
            _lastStableTick = pmdGetDBTick() ;
         }
      }
      else
      {
         // set last stable tick to 0, means it is not stable relatively
         _lastStableTick = 0LL ;
      }

      if ( STP_SYNC_NOSOURCE == status )
      {
         _resetSourceRID() ;
      }
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__TPSYNCCLIENTMGR__HASENOUGHRECORDS, "_stpSyncClientManager::_hasEnoughRecords" )
   BOOLEAN _stpSyncClientManager::_hasEnoughRecords()
   {
      BOOLEAN result = FALSE ;

      PD_TRACE_ENTRY( SDB__TPSYNCCLIENTMGR__HASENOUGHRECORDS ) ;

      // if we have 10 records, that's enough
      result = ( _syncRecords.size() >= STP_SYNC_RECORD_CACHE_SIZE ) ;

      PD_TRACE_EXIT( SDB__TPSYNCCLIENTMGR__HASENOUGHRECORDS ) ;

      return result ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__TPSYNCCLIENTMGR__ADJUSTTIME, "_stpSyncClientManager::_adjustTime" )
   void _stpSyncClientManager::_adjustTime( const stpSyncRecord &record )
   {
      PD_TRACE_ENTRY( SDB__TPSYNCCLIENTMGR__ADJUSTTIME ) ;

      // save synchronize records ( only keep last 10 records )
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

      // adjust logical time by offset
      getMetaData()->adjustLogicalTime( record.getOffset(),
                                        record.getRspTimeError() ) ;
      // adjust time error of local node
      _nodeManager->updateLocalTimeError( record.getRspTimeError() ) ;

      PD_TRACE_EXIT( SDB__TPSYNCCLIENTMGR__ADJUSTTIME ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__TPSYNCCLIENTMGR__ADJUSTSLEWRATE, "_stpSyncClientManager::_adjustSlewRate" )
   BOOLEAN _stpSyncClientManager::_adjustSlewRate( STP_SYNC_REC_LIST &records )
   {
      BOOLEAN finishedOrSkipped = FALSE ;

      PD_TRACE_ENTRY( SDB__TPSYNCCLIENTMGR__ADJUSTSLEWRATE ) ;

      INT64 totalOffset = 0LL ;
      INT64 averageOffset = 0LL ;
      INT64 numRecords = (INT64)( records.size() ) ;
      INT64 recordTime = (INT64)STP_SLEWRATE_CHECK_INTARVAL_NS * numRecords ;

      if ( numRecords < STP_SYNC_RECORD_CACHE_SIZE )
      {
         // not enough records
         goto done ;
      }

      // check if records are validated to calculate slew rate
      if ( !_checkSlewRateValid( records, totalOffset, averageOffset ) )
      {
         if ( _curStatusCount > STP_MAX_SLEW_RATE_CHECK_TIMES )
         {
            // failed to check records for slew rate calculation for few times
            // skip slew rate checking
            PD_LOG( PDWARNING, "Failed to check synchronize records for "
                    "slew rate calculation for %u times, skip slew rate check",
                    _curStatusCount ) ;
            finishedOrSkipped = TRUE ;
         }
         else
         {
            PD_LOG( PDEVENT, "Failed to check synchronize records for "
                    "slew rate calculation" ) ;
         }
         goto done ;
      }

      // if the average offset is in valid range, means the offset change is
      // too trivial for adjust slew rate, no need to adjust
      if ( averageOffset > STP_SLEWRATE_OFFSET_MIN_LIMIT &&
           averageOffset < STP_SLEWRATE_OFFSET_MAX_LIMIT )
      {
         finishedOrSkipped = TRUE ;
         goto done ;
      }

      // adjust slew rate
      getMetaData()->adjustSlewRate( recordTime + totalOffset, recordTime ) ;
      finishedOrSkipped = TRUE ;

   done:
      PD_TRACE_EXIT( SDB__TPSYNCCLIENTMGR__ADJUSTSLEWRATE ) ;
      return finishedOrSkipped ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__TPSYNCCLIENTMGR__CHKSLEWRATEVALID, "_stpSyncClientManager::_checkSlewRateValid" )
   BOOLEAN _stpSyncClientManager::_checkSlewRateValid( STP_SYNC_REC_LIST &records,
                                                       INT64 &totalOffset,
                                                       INT64 &averageOffset )
   {
      BOOLEAN isValid = FALSE ;

      PD_TRACE_ENTRY( SDB__TPSYNCCLIENTMGR__CHKSLEWRATEVALID ) ;

      INT64 offsetArray[ STP_SYNC_RECORD_CACHE_SIZE ] = { 0LL } ;
      INT64 numRecords = records.size() ;
      UINT32 pos = 0, q3Pos = 0, q1Pos = 0 ;
      INT64 q3Offset = 0LL, q1Offset = 0LL, outlierStep = 0LL ;
      INT64 lowBoundOffset = 0LL, upBoundOffset = 0LL ;
      INT64 totalVariance = 0LL, maxDiffOffset = -1LL ;
      FLOAT64 variance = 0.0f, deviation = 0.0f ;
      STP_SYNC_REC_LIST::iterator iter, iterToMaxDiff ;
      UINT32 numOutlier = 0 ;

      totalOffset = 0LL ;
      averageOffset = 0LL ;

      SDB_ASSERT( numRecords > 0, "synchronize record is empty" ) ;
      if ( 0 == numRecords )
      {
         goto done ;
      }

      // 3 steps to check synchronize records for slew rate calculation
      // - calculate total offset and average offset
      // - check outliers with Inter-Quantile Range
      // - check standard deviation for average offset

      // calculate total offset and average offset first
      // and fill offset array for sorting for next IQR phase
      pos = 0 ;
      for ( STP_SYNC_REC_LIST::iterator iter = records.begin() ;
            iter != records.end() ;
            ++ iter, ++ pos )
      {
         totalOffset += iter->getOffset() ;
         offsetArray[ pos ] = iter->getOffset() ;
      }

      // calculate average offset
      averageOffset = totalOffset / numRecords ;

      PD_LOG( PDEVENT, "Got average offset of synchronize records, size: [%u], "
              "total offset: [%lld], average offset: [%lld]", numRecords,
              totalOffset, averageOffset ) ;

      // check InterQuantile Range (IQR) for outliers
      // if synchronize result is an outlier amoug all results, we should
      // kick it out from later processing

      // definition of IQR:
      //
      // - Q1 lower quantile, the 25th percentile in ascending order
      // - Q3 upper quantile, the 75th percentile in ascending order
      //
      // <------- | ------ | --- | --- | ------ | ------->
      //       Q1-1.5*IQR  Q1  Median  Q3  Q3+1.5*IQR
      // outliers |        |<-- IQR -->|        | outliers

      sort( offsetArray, offsetArray + numRecords ) ;

      q3Pos = (UINT32)( (FLOAT64)numRecords * 0.75 ) ;
      q1Pos = (UINT32)( (FLOAT64)numRecords * 0.25 ) ;
      q3Offset = offsetArray[ q3Pos ] ;
      q1Offset = offsetArray[ q1Pos ] ;

      // interquantile range to outlier step
      outlierStep = (INT64)( (FLOAT64)( q3Offset - q1Offset ) * 1.5 ) ;

      // lower bound = Q1 - outlier step
      lowBoundOffset = q1Offset - outlierStep ;
      // upper bound = Q3 + outlier step
      upBoundOffset = q3Offset + outlierStep ;

      PD_LOG( PDDEBUG, "Got quantile of synchronize records, size: [%u], "
              "Q1 offset: [%lld], Q3 offset: [%lld], low bound: [%lld], "
              "up bound: [%lld]", numRecords, q1Offset, q3Offset,
              lowBoundOffset, upBoundOffset ) ;

      // check outliers and calculate variance for next standard
      // deviation step
      iter = records.begin() ;
      while ( iter != records.end() )
      {
         // check outlier
         if ( iter->getOffset() > upBoundOffset ||
              iter->getOffset() < lowBoundOffset )
         {
            // kick out outlier
            iter = records.erase( iter ) ;
            ++ numOutlier ;
            continue ;
         }
         else if ( 0 == numOutlier )
         {
            // no outlier is kicked yet, we could calculate variance for
            // next standard deviation step
            INT64 diffOffset = iter->getOffset() - averageOffset ;
            diffOffset *= diffOffset ;

            if ( diffOffset > maxDiffOffset )
            {
               iterToMaxDiff = iter ;
            }

            totalVariance += diffOffset ;
         }
         ++ iter ;
      }

      if ( numOutlier > 0 )
      {
         PD_LOG( PDEVENT, "Got quantile of synchronize records, size: [%u], "
                 "Q1 offset: [%lld], Q3 offset: [%lld], low bound: [%lld], "
                 "up bound: [%lld], kick out [%u] outliers", numRecords,
                 q1Offset, q3Offset, lowBoundOffset, upBoundOffset,
                 numOutlier ) ;
         // outliers are kicked out
         goto done ;
      }

      // check standard deviation of offsets
      //
      // variance = 1 / n * sum( ( offset_i - average ) ^ 2 )
      // standard deviation = sqrt( variance )

      // calculate variance
      variance = (FLOAT64)totalVariance / (FLOAT64)numRecords ;
      // calculate deviation
      deviation = sqrt( variance ) ;

      PD_LOG( PDDEBUG, "Got standard deviation of synchronize records, "
              "size: [%u], variance: [%.6f], deviation: [%.6f], "
              "average: [%lld]", numRecords, variance, deviation,
              averageOffset ) ;

      if ( deviation > (FLOAT64)STP_SLEWRATE_DEVIATION_MAX_LIMIT )
      {
         // standard deviation is too large, means the results are changing
         // dramatically, we should not use these results to calculate
         // slew rate, kick out one result with maximum deviation
         // and continue to collection synchronize results
         records.erase( iterToMaxDiff ) ;
         PD_LOG( PDDEBUG, "Got standard deviation of synchronize records, "
                 "size: [%u], variance: [%.6f], deviation: [%.6f], "
                 "average: [%lld], deviation is too large, kick out "
                 "max different record", numRecords, variance, deviation,
                 averageOffset ) ;
         goto done ;
      }

      isValid = TRUE ;

   done:
      PD_TRACE_EXIT( SDB__TPSYNCCLIENTMGR__CHKSLEWRATEVALID ) ;

      return isValid ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__TPSYNCCLIENTMGR__CANDECTIMEERROR, "_stpSyncClientManager::_canDecTimeError" )
   BOOLEAN _stpSyncClientManager::_canDecTimeError( UINT32 curTimeError )
   {
      BOOLEAN canDecrease = FALSE ;

      PD_TRACE_ENTRY( SDB__TPSYNCCLIENTMGR__COULDDECTIMEERROR ) ;

      // only check if we have enough records ( samples )
      if ( _hasEnoughRecords() )
      {
         UINT32 count = (UINT32)( _syncRecords.size() ) ;
         UINT64 totalDelay = 0LL ;
         UINT64 averageDelay = 0LL ;

         // decrease target, check against with 2 times of decrease step
         // if all delays are smaller than decrease target, we could decrease
         // time error
         UINT32 decTimeError =
               stpClientNode::getDecTimeError( curTimeError,
                                               STP_MIN_TIME_ERROR, 2 ) ;
         for ( STP_SYNC_REC_LIST::const_iterator iter = _syncRecords.begin() ;
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

         // also, average delay should be smaller than decrease target
         averageDelay = (UINT64)( (FLOAT64)totalDelay / (FLOAT64)count ) ;
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
      PD_TRACE_EXIT( SDB__TPSYNCCLIENTMGR__COULDDECTIMEERROR ) ;
      return canDecrease ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__TPSYNCCLIENTMGR__LAUNCHREGISTER, "_stpSyncClientManager::_launchRegister" )
   INT32 _stpSyncClientManager::_launchRegister( const MsgRouteID &primaryRID,
                                                 UINT32 version,
                                                 const stpClientNode &local )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__TPSYNCCLIENTMGR__LAUNCHREGISTER ) ;

      BSONObj regObject ;

      try
      {
         BSONObjBuilder builder ;
         stpClientNode temp = local ;

         // regenerate OID
         temp.generateOID() ;

         // build BSON object
         rc = temp.toBSON( builder, FALSE ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to build BSON object for register "
                      "node [%s], rc: %d", temp.toString().c_str(), rc ) ;

         regObject = builder.obj() ;
      }
      catch ( exception &e )
      {
         PD_LOG( PDERROR, "Failed to build BSON object for register node, "
                 "error: %s", e.what() ) ;
         rc = SDB_SYS ;
         goto error ;
      }

      // send register request
      rc = _sendRegReq( primaryRID, version, regObject ) ;
      if ( SDB_OK != rc )
      {
         // Failed to send request to primary, reset session and primary
         _session.resetCurServerRID() ;
         _nodeManager->resetPrimary() ;
      }
      PD_RC_CHECK( rc, PDERROR, "Failed to send register request, rc: %d",
                   rc ) ;

   done:
      PD_TRACE_EXITRC( SDB__TPSYNCCLIENTMGR__LAUNCHREGISTER, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__TPSYNCCLIENTMGR__LAUNCHTIMESYNC, "_stpSyncClientManager::_launchTimeSync" )
   INT32 _stpSyncClientManager::_launchTimeSync( const MsgRouteID &sourceRID,
                                                 UINT32 version,
                                                 const stpClientNode &local )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__TPSYNCCLIENTMGR__LAUNCHTIMESYNC ) ;

      // for other status, send synchronize time request
      UINT32 timeError = local.getTimeError() ;
      UINT16 flag = STP_SYNC_TIME_FLAG_EMPTY ;

      // check if we could decrease time error
      if ( _canDecTimeError( timeError ) )
      {
         // set flag to tell source to decrease the time error
         OSS_BIT_SET( flag, STP_SYNC_TIME_FLAG_DECTIMEERROR ) ;
      }

      // send time synchronize request
      rc = _sendTimeSyncReq( sourceRID, version, flag, _status, timeError ) ;
      if ( SDB_OK != rc )
      {
         // Failed to send request to primary, reset session and primary
         _session.resetCurServerRID() ;
         _nodeManager->resetPrimary() ;
      }
      PD_RC_CHECK( rc, PDERROR, "Failed to send synchronize request, "
                   "rc: %d", rc ) ;

   done:
      PD_TRACE_EXITRC( SDB__TPSYNCCLIENTMGR__LAUNCHTIMESYNC, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__TPSYNCCLIENTMGR_REGSOURCE, "_stpSyncClientManager::registerSource" )
   INT32 _stpSyncClientManager::registerSource( const stpSourceNode &source )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__TPSYNCCLIENTMGR_REGSOURCE ) ;

      ossScopedRWLock lock( ( &_sourceMutex ), EXCLUSIVE ) ;

      try
      {
         // find source by route ID
         STP_SOURCE_MAP::iterator iter = _sources.find( source.getRouteID() ) ;
         if ( iter != _sources.end() )
         {
            // if exists, means the source had been used before, reuse the
            // source ( call on register to merge histories )
            iter->second.onRegister() ;
         }
         else
         {
            // new source
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
      PD_TRACE_EXITRC( SDB__TPSYNCCLIENTMGR_REGSOURCE, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__TPSYNCCLIENTMGR_GETSOURCE, "_stpSyncClientManager::getSource" )
   INT32 _stpSyncClientManager::getSource( const MsgRouteID &routeID,
                                           stpSourceNode &source )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__TPSYNCCLIENTMGR_GETSOURCE ) ;

      ossScopedRWLock lock( ( &_sourceMutex ), SHARED ) ;

      // find source by route ID
      STP_SOURCE_MAP::const_iterator iter = _sources.find( routeID ) ;
      PD_CHECK( iter != _sources.end(), SDB_INVALID_ROUTEID, error, PDERROR,
                "Failed to get source node %s, it is not found",
                routeID2String( routeID ).c_str() ) ;

      // copy source to output
      source = iter->second ;

   done:
      PD_TRACE_EXITRC( SDB__TPSYNCCLIENTMGR_GETSOURCE, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__TPSYNCCLIENTMGR__ONSYNCREQ, "_stpSyncClientManager::_onSyncReq" )
   INT32 _stpSyncClientManager::_onSyncReq( const MsgRouteID &routeID )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__TPSYNCCLIENTMGR__ONSYNCREQ ) ;

      ossScopedRWLock lock( &_sourceMutex, EXCLUSIVE ) ;

      // find source by route ID
      STP_SOURCE_MAP::iterator iter = _sources.find( routeID ) ;
      PD_CHECK( iter != _sources.end(), SDB_INVALID_ROUTEID, error, PDERROR,
                "Failed to get source node %s, it is not found",
                routeID2String( routeID ).c_str() ) ;

      // on previous to send synchronize time request
      // increase synchronize count, etc
      iter->second.onPreSync() ;

   done:
      PD_TRACE_EXITRC( SDB__TPSYNCCLIENTMGR__ONSYNCREQ, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__TPSYNCCLIENTMGR__ONSYNCRSP, "_stpSyncClientManager::_onSyncRsp" )
   INT32 _stpSyncClientManager::_onSyncRsp( const MsgRouteID &routeID,
                                            const stpSyncRecord &record,
                                            BOOLEAN isValid )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__TPSYNCCLIENTMGR__ONSYNCRSP ) ;

      ossScopedRWLock lock( &_sourceMutex, EXCLUSIVE ) ;

      // find source by route ID
      STP_SOURCE_MAP::iterator iter = _sources.find( routeID ) ;
      PD_CHECK( iter != _sources.end(), SDB_INVALID_ROUTEID, error, PDERROR,
                "Failed to get source node %s, it is not found",
                routeID2String( routeID ).c_str() ) ;

      // on post synchronize time, update source history by synchronize
      // record
      iter->second.onPostSync( record, isValid, _options->getMaxSyncHist() ) ;

   done:
      PD_TRACE_EXITRC( SDB__TPSYNCCLIENTMGR__ONSYNCRSP, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__TPSYNCCLIENTMGR_REMOVESOURCE, "_stpSyncClientManager::removeSource" )
   INT32 _stpSyncClientManager::removeSource( const MsgRouteID &routeID )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__TPSYNCCLIENTMGR_REMOVESOURCE ) ;

      ossScopedRWLock lock( ( &_sourceMutex ), EXCLUSIVE ) ;

      STP_SOURCE_MAP::iterator iter = _sources.find( routeID ) ;

      // find source by route ID
      PD_CHECK( iter != _sources.end(),
                SDB_INVALID_ROUTEID, error, PDWARNING,
                "Failed to remove source node %s, route ID is not found",
                routeID2String( routeID ).c_str() ) ;

      // remove source
      _sources.erase( iter ) ;

      PD_LOG( PDEVENT, "Remove source node %s done",
              routeID2String( routeID ).c_str() ) ;

   done:
      PD_TRACE_EXITRC( SDB__TPSYNCCLIENTMGR_REMOVESOURCE, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__TPSYNCCLIENTMGR_REMOVESOURCE_EXPIRED, "_stpSyncClientManager::removeSource" )
   INT32 _stpSyncClientManager::removeSource( const MsgRouteID &routeID,
                                              UINT64 expiredTick )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__TPSYNCCLIENTMGR_REMOVESOURCE_EXPIRED ) ;

      ossScopedRWLock lock( ( &_sourceMutex ), EXCLUSIVE ) ;

      STP_SOURCE_MAP::iterator iter = _sources.find( routeID ) ;

      // find source by route ID
      PD_CHECK( iter != _sources.end(),
                SDB_INVALID_ROUTEID, error, PDWARNING,
                "Failed to remove source node %s, route ID is not found",
                routeID2String( routeID ).c_str() ) ;

      // check if the source is expired ( no synchronize since given expired
      // tick )
      if ( iter->second.getUpdateTick() <= expiredTick )
      {
         // remove expired source
         _sources.erase( iter ) ;
      }
      else
      {
         // not expired ( new synchronization happened after we first check
         // expiration ), ignore
         PD_LOG( PDDEBUG, "Ignored remove expired source node %s, "
                 "synchronize time is updated",
                 routeID2String( routeID ).c_str() ) ;
         goto done ;
      }

      PD_LOG( PDEVENT, "Remove expired source node %s done",
              routeID2String( routeID ).c_str() ) ;

   done:
      PD_TRACE_EXITRC( SDB__TPSYNCCLIENTMGR_REMOVESOURCE_EXPIRED, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__TPSYNCCLIENTMGR_DUMPSOURCES, "_stpSyncClientManager::dumpSources" )
   INT32 _stpSyncClientManager::dumpSources( STP_SOURCE_MAP &sources )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__TPSYNCCLIENTMGR_DUMPSOURCES ) ;

      ossScopedRWLock lock( ( &_sourceMutex ), SHARED ) ;

      try
      {
         // copy clients to output
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
      PD_TRACE_EXITRC( SDB__TPSYNCCLIENTMGR_DUMPSOURCES, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__TPSYNCCLIENTMGR__ONREGRSP, "_stpSyncClientManager::_onRegRsp" )
   INT32 _stpSyncClientManager::_onRegRsp( const MsgRouteID &routeID,
                                           UINT16 port )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__TPSYNCCLIENTMGR__ONREGRSP ) ;

      // on event we received register response, which means we had been
      // register to a server ( of given route ID ) to start time
      // synchronization, we need to:
      // - check if route ID is known by node manager
      // - register this server as source
      // - assign port

      stpSourceNode source ;

      // check if given route ID is a server, get this server as source
      rc = _nodeManager->getServer( routeID, source ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to get server %s, rc: %d",
                   routeID2String( routeID ).c_str(), rc ) ;

      // register source
      rc = registerSource( source ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to register source %s, rc: %d",
                   routeID2String( routeID ).c_str(), rc ) ;

      // assigned port by source
      _syncSourceRID.value = source.getRouteIDValue() ;

      if ( port != source.getNodeID() )
      {
         // assigned to extra synchronize port
         CHAR serviceName[ OSS_MAX_SERVICENAME + 1 ] = { '\0' } ;
         ossSnprintf( serviceName, OSS_MAX_SERVICENAME, "%u", port ) ;

         _syncSourceRID.columns.nodeID = port ;

         rc = _netManager->updateRouteID( _syncSourceRID,
                                          source.getHostName(),
                                          serviceName ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to update route ID %s, rc: %d",
                      routeID2String( _syncSourceRID ).c_str(), rc ) ;
      }

      // set register source ID
      _regSourceRID.value = source.getRouteIDValue() ;

      PD_LOG( PDEVENT, "Register synchronize to %s with port %u",
              source.toString().c_str(), port ) ;

   done:
      PD_TRACE_EXITRC( SDB__TPSYNCCLIENTMGR__ONREGRSP, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__TPSYNCCLIENTMGR__CLEAREXPIREDSOURCES, "_stpSyncClientManager::_clearExpiredSources" )
   INT32 _stpSyncClientManager::_clearExpiredSources()
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__TPSYNCCLIENTMGR__CLEAREXPIREDSOURCES ) ;

      STP_SOURCE_MAP sources ;

      // copy all sources to check
      rc = dumpSources( sources ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to dump sources, rc: %d", rc ) ;

      // check each source
      for ( STP_SOURCE_MAP::iterator iter = sources.begin() ;
            iter != sources.end() ;
            ++ iter )
      {
         // check if no synchronize for a long time
         UINT64 updateTick = iter->second.getUpdateTick() ;
         UINT64 updatePassed = pmdGetTickSpanTime( updateTick ) ;
         if ( updatePassed > STP_CLEAR_SOURCE_INTERVAL )
         {
            // if no synchronize for 2 hours, remove this source
            removeSource( iter->second.getRouteID(), updateTick ) ;
         }
      }

   done:
      PD_TRACE_EXITRC( SDB__TPSYNCCLIENTMGR__CLEAREXPIREDSOURCES, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__TPSYNCCLIENTMGR__RESETSOURCERID, "_stpSyncClientManager::_resetSourceRID" )
   void _stpSyncClientManager::_resetSourceRID()
   {
      PD_TRACE_ENTRY( SDB__TPSYNCCLIENTMGR__RESETSOURCERID ) ;

      if ( _syncSourceRID.value != _regSourceRID.value &&
            MSG_INVALID_ROUTEID != _syncSourceRID.value &&
            MSG_INVALID_ROUTEID != _regSourceRID.value )
      {
         // synchronize source route ID is different from register
         // source route ID, means using extra synchronize port
         // remove route ID with extra port
         _netManager->deleteRouteID( _syncSourceRID ) ;
      }
      _syncSourceRID.value = MSG_INVALID_ROUTEID ;
      _regSourceRID.value = MSG_INVALID_ROUTEID ;

      PD_TRACE_EXIT( SDB__TPSYNCCLIENTMGR__RESETSOURCERID ) ;
   }

}
