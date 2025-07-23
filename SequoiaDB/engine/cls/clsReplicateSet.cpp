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

   Source File Name = clsReplicateSet.hpp

   Descriptive Name = Replication Control Block Header

   When/how to use: this program may be used on binary and text-formatted
   versions of Replication component. This file contains structure for
   replication control block.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/14/2012  YW  Initial Draft

   Last Changed =

*******************************************************************************/
#include "clsReplicateSet.hpp"
#include "netRouteAgent.hpp"
#include "clsUtil.hpp"
#include "pmd.hpp"
#include "pmdCB.hpp"
#include "clsMgr.hpp"
#include "clsFSSrcSession.hpp"
#include "pmdStartup.hpp"
#include "msgMessage.hpp"
#include "pmdController.hpp"
#include "pdTrace.hpp"
#include "clsTrace.hpp"

namespace engine
{
   BEGIN_OBJ_MSG_MAP( _clsReplicateSet, _pmdObjBase )
      //ON_MSG ( )
      ON_MSG( MSG_CAT_GRP_RES, handleMsg )
      ON_MSG( MSG_CLS_BEAT, handleMsg )
      ON_MSG( MSG_CLS_BEAT_RES, handleMsg )
      ON_MSG( MSG_CLS_BALLOT, handleMsg )
      ON_MSG( MSG_CLS_BALLOT_RES, handleMsg )
      ON_MSG( MSG_CAT_PAIMARY_CHANGE_RES, handleMsg )
      ON_MSG( MSG_CLS_GINFO_UPDATED, handleMsg )
      ON_MSG( MSG_CLS_NODE_STATUS_NOTIFY, handleMsg )
      ON_EVENT( PMD_EDU_EVENT_STEP_DOWN, handleEvent )
      ON_EVENT( PMD_EDU_EVENT_STEP_UP, handleEvent )
   END_OBJ_MSG_MAP ()

   const UINT32 CLS_REPL_SEC_TIME = 1000 ;

   #define CLS_SYNCCTRL_BASE_TIME               (10)
   #define CLS_STOP_WAIT_HEARTBEAT_TIMEOUT      (20*OSS_ONE_SEC)
   #define CLS_PRIMARY_UP_NOTIFY_TIMES          (60)

   /*
      _clsReplicateSet define
   */
   _clsReplicateSet::_clsReplicateSet( _netRouteAgent *agent )
   : ICLSReplAgent( agent ),
     _logger( NULL ),
     _pFTMgr( NULL ),
     _clsCB( NULL ),
     _timerID( CLS_INVALID_TIMERID )
   {
      _srcSessionNum = 0 ;
      _ntyLastOffset = DPS_INVALID_LSN_OFFSET ;
      _ntyProcessedOffset = DPS_INVALID_LSN_OFFSET ;

      _totalLogSize = 0 ;
      _inSyncCtrl   = FALSE ;
      _lastTimerTick = 0 ;
      _lastConsultTick = 0 ;
      memset( _sizethreshold, 0, sizeof( _sizethreshold ) ) ;
      memset( _timeThreshold, 0, sizeof( _timeThreshold ) ) ;

      _faultEvent.reset() ;
      _syncEmptyEvent.signal() ;

      _syncwaitTimeout = 0 ;
      _shutdownWaitTimeout = 0 ;
      _fusingTimeout = 0 ;
   }

   _clsReplicateSet::~_clsReplicateSet()
   {

   }

   void _clsReplicateSet::onWriteLog( DPS_LSN_OFFSET offset )
   {
      _sync.notify( offset ) ;
   }

   void _clsReplicateSet::onPrepareLog( UINT32 csLID, UINT32 clLID,
                                        INT32 extLID, DPS_LSN_OFFSET offset )
   {
      if ( getNtySessionNum() > 0 )
      {
         _ntyLastOffset = offset ;
         _ntyQue.push( clsLSNNtyInfo( csLID, clLID, extLID ,offset ) ) ;
      }
   }

   UINT64 _clsReplicateSet::completeLsn( BOOLEAN doFast, UINT32 *pVer )
   {
      DPS_LSN lsn ;

      if ( !primaryIsMe() && _replBucket.maxReplSync() > 0 )
      {
         if ( doFast )
         {
            lsn = _replBucket.fastCompleteLSN() ;
         }
         else
         {
            lsn = _replBucket.completeLSN() ;
         }
      }

      if ( pVer )
      {
         *pVer = lsn.version ;
      }
      return lsn.offset ;
   }

   UINT32 _clsReplicateSet::lsnQueSize()
   {
      return getBucket()->bucketSize() ;
   }

   BOOLEAN _clsReplicateSet::primaryLsn( UINT64 &lsn, UINT32 *pVer )
   {
      BOOLEAN got = FALSE ;
      _clsSharingStatus tmpStatus ;

      if ( primaryIsMe() )
      {
         got = TRUE ;
         lsn = DPS_INVALID_LSN_OFFSET ;
         if ( pVer )
         {
            *pVer = DPS_INVALID_LSN_VERSION ;
         }
      }
      else if ( getPrimaryInfo( tmpStatus ) )
      {
         got = TRUE ;
         lsn = tmpStatus.beat.endLsn.offset ;
         if ( pVer )
         {
            *pVer = tmpStatus.beat.endLsn.version ;
         }
      }

      return got ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSREPPSET_NOTIFY2SESSION, "_clsReplicateSet::notify2Session" )
   void _clsReplicateSet::notify2Session( UINT32 suLID, UINT32 clLID,
                                          dmsExtentID extLID,
                                          const DPS_LSN_OFFSET & offset )
   {
      PD_TRACE_ENTRY ( SDB__CLSREPPSET_NOTIFY2SESSION );
      // the src session is not empty, should notify every one
      if ( _srcSessionNum > 0 )
      {
         UINT32 index = 0 ;
         ossScopedRWLock lock( &_vecLatch, SHARED ) ;
         while ( index < _srcSessionNum )
         {
            _vecSrcSessions[index]->notifyLSN ( suLID, clLID, extLID, offset ) ;
            ++index ;
         }
      }
      _ntyProcessedOffset = offset ;

      PD_TRACE_EXIT ( SDB__CLSREPPSET_NOTIFY2SESSION );
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSREPSET_REGSN, "_clsReplicateSet::regSession" )
   void _clsReplicateSet::regSession ( _clsDataSrcBaseSession * pSession )
   {
      PD_TRACE_ENTRY ( SDB__CLSREPSET_REGSN );
      SDB_ASSERT ( pSession, "Session can't be null" ) ;

      try
      {
         ossScopedRWLock lock( &_vecLatch, EXCLUSIVE ) ;
         _vecSrcSessions.push_back ( pSession ) ;
         _srcSessionNum++ ;
      }
      catch ( exception &e )
      {
         PD_LOG( PDERROR, "Failed to register session, "
                 "occur exception %s", e.what() ) ;
         // unable to handle the exception, throw it
         throw e ;
      }
      PD_TRACE_EXIT ( SDB__CLSREPSET_REGSN );
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSREPSET_UNREGSN, "_clsReplicateSet::unregSession" )
   void _clsReplicateSet::unregSession ( _clsDataSrcBaseSession * pSession )
   {
      PD_TRACE_ENTRY ( SDB__CLSREPSET_UNREGSN );
      SDB_ASSERT ( pSession, "Session can't be null" ) ;

      ossScopedRWLock lock( &_vecLatch, EXCLUSIVE ) ;
      std::vector<_clsDataSrcBaseSession*>::iterator it =
         _vecSrcSessions.begin() ;
      while ( it != _vecSrcSessions.end() )
      {
         if ( *it == pSession )
         {
            _vecSrcSessions.erase ( it ) ;
            _srcSessionNum-- ;
            break ;
         }
         ++it ;
      }
      PD_TRACE_EXIT ( SDB__CLSREPSET_UNREGSN );
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSREPSET_INIT, "_clsReplicateSet::initialize" )
   INT32 _clsReplicateSet::initialize ()
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB__CLSREPSET_INIT ) ;

      _netFrame *pNetFrame = NULL ;

      if ( !_agent )
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      // init start shift time
      setStartShiftTime( (INT32)pmdGetOptionCB()->startShiftTime() ) ;

      _logger = pmdGetKRCB()->getDPSCB() ;
      _pFTMgr = pmdGetKRCB()->getFTMgr() ;
      _clsCB = pmdGetKRCB()->getClsCB() ;
      SDB_ASSERT( NULL != _logger && NULL != _pFTMgr,
                  "logger should not be NULL" ) ;

      // register dps log event handler
      _logger->regEventHandler( this ) ;

      rc = _replBucket.init() ;
      PD_RC_CHECK( rc, PDERROR, "Init repl bucket failed, rc: %d", rc ) ;

      pNetFrame = _agent->getFrame() ;
      /// register repl net agent to net monitor for connections
      pNetFrame->setBeatInfo( pmdGetOptionCB()->getOprTimeout() ) ;

      rc = sdbGetPMDController()->registerNet( pNetFrame,
                                               MSG_ROUTE_REPL_SERVICE ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to register net monitor on "
                   "REPL service, rc: %d", rc ) ;

      _totalLogSize = pmdGetOptionCB()->getTotalLogSpace() ;
      // init sync control param
      {
         UINT32 rate = 2 ;
         UINT32 timeBase = CLS_SYNCCTRL_BASE_TIME ;

         for ( UINT32 idx = 0 ; idx < CLS_SYNCCTRL_THRESHOLD_SIZE ; ++idx )
         {
            rate = 2 << idx ;
            _sizethreshold[ idx ] = _totalLogSize * ( rate - 1 ) / rate ;
            _timeThreshold[ idx ] = timeBase << idx ;
         }
      }

      _syncwaitTimeout = pmdGetOptionCB()->syncwaitTimeout() * OSS_ONE_SEC ;
      _fusingTimeout = pmdGetOptionCB()->ftFusingTimeout() * OSS_ONE_SEC ;
      _shutdownWaitTimeout = pmdGetOptionCB()->shutdownWaitTimeout() *
                             OSS_ONE_SEC ;

   done:
      PD_TRACE_EXITRC ( SDB__CLSREPSET_INIT, rc );
      return rc ;
   error:
      goto done ;
   }

   INT32 _clsReplicateSet::deactive ()
   {
      SDB_ASSERT( PMD_IS_DB_DOWN(), "DB must be down" ) ;
      UINT32 timeout = 0 ;

      /// disconnect al shard agent
      if ( _clsCB )
      {
         _clsCB->getShardRouteAgent()->disconnectAll() ;
      }

      /// wait sync replay packet
      _syncEmptyEvent.wait() ;
      /// wait sync bucket
      if ( _replBucket.maxReplSync() > 0 )
      {
         // wait all repl-sync log processed
         PD_LOG( PDEVENT, "Begin to wait repl bucket empty[bucket size: %d, "
                 "all size: %d, agent number: %d]", _replBucket.bucketSize(),
                 _replBucket.size(), _replBucket.curAgentNum() ) ;

         pmdSetDoing( "Waiting repl bucket to replay empty..." ) ;
         _replBucket.waitEmpty() ;
         pmdCleanDoing() ;

         PD_LOG( PDEVENT, "Wait repl bucket empty completed" ) ;
      }

      /// wait send stop heartbeat to other nodes
      if ( _active )
      {
         PD_LOG( PDEVENT, "Begin to wait broadcast stop-heartbeat..." ) ;
         _heartbeatEvent.reset() ;
         _beatTime = CLS_SHARING_BETA_INTERVAL ;
         if ( SDB_OK != _heartbeatEvent.wait( CLS_STOP_WAIT_HEARTBEAT_TIMEOUT ) )
         {
            PD_LOG( PDWARNING, "Wait broadcast stop-heartbeat failed" ) ;
         }
         else
         {
            PD_LOG( PDEVENT, "Wait broadcast stop-heartbeat succeed" ) ;
         }

         if ( _shutdownWaitTimeout > 0 &&
              _info.groupSize() > 1 &&
              _vote.primaryIsMe() )
         {
            UINT32 nodeCnt = 0 ;
            UINT32 aliveCnt = 0 ;
            UINT32 falutCnt = 0 ;
            UINT32 ssCnt = 0 ;
            INT32 indoubtErr = 0 ;
            UINT16 indoubtNodeID = 0 ;

            PD_LOG( PDEVENT, "Begin to wait data consistent..." ) ;
            pmdSetDoing( "Waiting data consistent..." ) ;
            /// When i'm primary, wait other node keep the data consistence
            while ( _vote.primaryIsMe() && timeout < _shutdownWaitTimeout )
            {
               if ( _logger->getCurrentLsn().invalid() ||
                    _sync.atLeastOne( _logger->getCurrentLsn().offset ) )
               {
                  PD_LOG( PDEVENT,
                          "Wait other node keep data consistent succeed" ) ;
                  break ;
               }
               /// check active node
               getDetailInfo( nodeCnt, aliveCnt, falutCnt, ssCnt,
                              indoubtErr, indoubtNodeID ) ;
               if ( 1 == aliveCnt )
               {
                  /// All node stoped
                  break ;
               }

               ossSleep( OSS_ONE_SEC ) ;
               timeout += OSS_ONE_SEC ;
            }
         }
         pmdCleanDoing() ;
      }

      _deactivate() ;

      return SDB_OK ;
   }

   INT32 _clsReplicateSet::final ()
   {
      _replBucket.fini() ;
      if ( _logger )
      {
         _logger->unregEventHandler( this ) ;
      }
      if ( _agent )
      {
         sdbGetPMDController()->unregNet( _agent->getFrame() ) ;
      }
      return SDB_OK ;
   }

   void _clsReplicateSet::onConfigChange ()
   {
      if ( pmdGetOptionCB()->maxReplSync() != getBucket()->maxReplSync() )
      {
         _sync.disableSync() ;
         _syncEmptyEvent.wait() ;
         getBucket()->enforceMaxReplSync( pmdGetOptionCB()->maxReplSync() ) ;
         _sync.enableSync() ;
      }
      if ( getStartShiftTime() >= 0 )
      {
         setStartShiftTime( (INT32)pmdGetOptionCB()->startShiftTime() ) ;
      }
      _syncwaitTimeout = pmdGetOptionCB()->syncwaitTimeout() * OSS_ONE_SEC ;
      _fusingTimeout = pmdGetOptionCB()->ftFusingTimeout() * OSS_ONE_SEC ;
      _shutdownWaitTimeout = pmdGetOptionCB()->shutdownWaitTimeout() *
                             OSS_ONE_SEC ;
   }

   void _clsReplicateSet::ntyPrimaryChange( BOOLEAN primary,
                                            SDB_EVENT_OCCUR_TYPE type )
   {
      if ( primary && SDB_EVT_OCCUR_BEFORE == type )
      {
         _replBucket.reset() ;
         setLastConsultTick( 0 ) ;
      }
      else if ( !primary && SDB_EVT_OCCUR_AFTER == type )
      {
         /// when we are not primary any more, we should clear
         /// waiting list.
         _sync.cut( 0 ) ;
      }
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSREPSET_ACTIVE, "_clsReplicateSet::active" )
   INT32 _clsReplicateSet::active()
   {
      INT32 rc      = SDB_OK ;
      PD_TRACE_ENTRY ( SDB__CLSREPSET_ACTIVE );
      if ( isActive() )
      {
         goto done ;
      }

      {
         _MsgRouteID id = _clsCB->getNodeID() ;
         id.columns.serviceID = _clsCB->getReplServiceID() ;
         setLocalID( id ) ;
         _MsgCatGroupReq msg ;
         msg.id = id ;
         _cata.call( (MsgHeader *)(&msg) ) ;
         _timerID = _clsCB->setTimer( CLS_REPL, CLS_REPL_SEC_TIME ) ;
      }
   done:
      PD_TRACE_EXITRC ( SDB__CLSREPSET_ACTIVE, rc );
      return rc ;
   }

   INT32 _clsReplicateSet::callCatalog( MsgHeader *header, UINT32 times )
   {
      return _cata.call( header, times ) ;
   }

   ossEvent* _clsReplicateSet::getFaultEvent()
   {
      return &_faultEvent ;
   }

   ossEvent* _clsReplicateSet::getSyncEmptyEvent()
   {
      return &_syncEmptyEvent ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSREPSET_ONTMR, "_clsReplicateSet::onTimer" )
   void _clsReplicateSet::onTimer( UINT64 timerID, UINT32 interval )
   {
      PD_TRACE_ENTRY ( SDB__CLSREPSET_ONTMR );
      if ( _timerID == timerID )
      {
         UINT64 timeSpan = pmdGetTickSpanTime( _lastTimerTick ) ;
         /// avoid out-of-data's timeout event
         if ( timeSpan < interval / 2 )
         {
            goto done ;
         }
         else if ( 0 != _lastTimerTick && timeSpan > 3 * interval )
         {
            PD_LOG( PDWARNING, "The %u milli-seconds's timer has %u "
                    "milli-seconds not called, the cluster's main thread "
                    "maybe blocked in some operations", interval,
                    timeSpan ) ;
         }
         /// reset the timer tick
         _lastTimerTick = pmdGetDBTick() ;

         _cata.handleTimeout( interval ) ;

         _handleTimeout( interval ) ;

         /// When self is primary NOSPC or TRANSERR, should force to secondary
         if ( _vote.primaryIsMe() && _info.groupSize() > 1 )
         {
            UINT32 ftConfirmedStat = _pFTMgr->getConfirmedStat() ;
            if ( OSS_BIT_TEST( ftConfirmedStat, PMD_FT_MASK_NOSPC ) ||
                 OSS_BIT_TEST( ftConfirmedStat, PMD_FT_MASK_TRANSERR ) )
            {
               DPS_LSN lsn = _logger->getCurrentLsn() ;
               if ( lsn.invalid() || _sync.atLeastOne( lsn.offset ) )
               {
                  CHAR ftStatStr[ CLS_FORMART_STR_128 + 1 ] = { 0 } ;
                  utilFTMaskToStr( ftConfirmedStat &
                                   ( PMD_FT_MASK_NOSPC |
                                     PMD_FT_MASK_TRANSERR ),
                                   ftStatStr,
                                   CLS_FORMART_STR_128 ) ;

                  PD_LOG( PDEVENT, "Force to secondary due to %s",
                          ftStatStr ) ;
                  _vote.force( CLS_ELECTION_STATUS_SEC, 5 * OSS_ONE_SEC ) ;
                  _vote.setShadowWeight( CLS_ELECTION_WEIGHT_MIN ) ;
               }
            }
         }
      }

   done:
      PD_TRACE_EXIT ( SDB__CLSREPSET_ONTMR );
      return ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSREPSET_HNDEVENT, "_clsReplicateSet::handleEvent" )
   INT32 _clsReplicateSet::handleEvent( pmdEDUEvent *event )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__CLSREPSET_HNDEVENT ) ;
      if ( PMD_EDU_EVENT_STEP_UP == event->_eventType ||
           PMD_EDU_EVENT_STEP_DOWN == event->_eventType )
      {
         rc = _handleEvent( event ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to handle event, rc: %d",  rc ) ;
      }
      else
      {
         PD_LOG( PDERROR, "unknown event type:%d", event->_eventType ) ;
         rc = SDB_SYS ;
         goto error ;
      }
   done:
      PD_TRACE_EXITRC( SDB__CLSREPSET_HNDEVENT, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSREPSET_HNDMSG, "_clsReplicateSet::handleMsg" )
   INT32 _clsReplicateSet::handleMsg( NET_HANDLE handle, MsgHeader* msg )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB__CLSREPSET_HNDMSG ) ;
      switch ( msg->opCode )
      {
         case MSG_CLS_BEAT :
         case MSG_CLS_BEAT_RES :
         case MSG_CLS_BALLOT :
         case MSG_CLS_BALLOT_RES :
         {
            rc = _handleMsg( handle, msg ) ;
            break ;
         }
         case MSG_CAT_GRP_RES:
         {
            rc = _handleGroupRes( (const MsgCatGroupRes *)msg ) ;
            break ;
         }
         case MSG_CAT_PAIMARY_CHANGE_RES:
         {
            INT32 result = MSG_GET_INNER_REPLY_RC( msg ) ;
            if ( SDB_CLS_NOT_PRIMARY == result )
            {
               shardCB *pShardCB = _clsCB->getShardCB() ;
               if ( SDB_OK != pShardCB->updatePrimaryByReply( msg ) )
               {
                  pShardCB->updateCatGroup () ;
               }
            }
            else if ( SDB_OK == result )
            {
               _cata.remove( msg, result ) ;
            }
            break ;
         }
         case MSG_CLS_GINFO_UPDATED :
         {
            PD_LOG( PDEVENT, "Group info has been updated, download again" ) ;
            MsgCatGroupReq msg ;
            msg.id = _info.local ;
            _cata.call( (MsgHeader *)(&msg) ) ;
            break ;
         }
         case MSG_CLS_NODE_STATUS_NOTIFY :
         {
            MsgClsNodeStatusNotify *pNty = ( MsgClsNodeStatusNotify* )msg ;
            if ( SDB_DB_FULLSYNC == pNty->status )
            {
               _sync.notifyFullSync( msg->routeID ) ;
            }
            break ;
         }
         default :
         {
            PD_LOG( PDWARNING, "unknown msg: %s", msg2String( msg ).c_str() ) ;
            rc = SDB_CLS_UNKNOW_MSG ;
            goto error ;
         }
      }

   done:
      PD_TRACE_EXITRC ( SDB__CLSREPSET_HNDMSG, rc );
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSREPSET__HNDGPRES, "_clsReplicateSet::_handleGroupRes" )
   INT32 _clsReplicateSet::_handleGroupRes( const MsgCatGroupRes *msg )
   {
      INT32 rc = SDB_OK ;
      MsgHeader *pHeader = ( MsgHeader* )msg ;
      PD_TRACE_ENTRY ( SDB__CLSREPSET__HNDGPRES );

      CLS_GROUP_VERSION version ;
      NET_ROUTE_MAP group ;
      string groupName ;
      BOOLEAN changeStatus = FALSE ;
      UINT32 grpHashCode = 0 ;

      if ( SDB_OK != MSG_GET_INNER_REPLY_RC(pHeader) )
      {
         if ( SDB_CLS_NOT_PRIMARY == MSG_GET_INNER_REPLY_RC(pHeader) )
         {
            shardCB *pShardCB = _clsCB->getShardCB() ;
            if ( SDB_OK != pShardCB->updatePrimaryByReply( pHeader ) )
            {
               pShardCB->updateCatGroup() ;
            }
         }

         PD_LOG( PDWARNING, "Download group info request was refused from "
                 "node[%s], rc: %d",
                 routeID2String( pHeader->routeID ).c_str(),
                 MSG_GET_INNER_REPLY_RC(pHeader) ) ;
         goto error ;
      }

      rc = msgParseCatGroupRes( msg, version, groupName, group,
                                NULL, &grpHashCode ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDWARNING, "parse MsgCatGroupRes err, rc = %d", rc ) ;
         goto error ;
      }

      rc = _setGroupSet( version, group, grpHashCode, TRUE, changeStatus ) ;
      if ( SDB_OK != rc )
      {
         if ( SDB_REPL_REMOTE_G_V_EXPIRED != rc )
         {
            PD_LOG( PDWARNING, "set group info failed, rc = %d", rc ) ;
            goto error ;
         }
         rc = SDB_OK ;
      }

      if ( !changeStatus )
      {
         _cata.remove( &(msg->header), MSG_GET_INNER_REPLY_RC(pHeader) ) ;
      }

      pmdGetKRCB()->setGroupName ( groupName.c_str() ) ;
      if ( !isActive() )
      {
         PD_LOG( PDEVENT, "download group info successfully" ) ;

         //start repl sync session
         _clsCB->startInnerSession ( CLS_REPL, CLS_TID_REPL_SYC ) ;

         _activate() ;
      }

   done :
      PD_TRACE_EXITRC ( SDB__CLSREPSET__HNDGPRES, rc );
      return rc ;
   error:
      goto done ;
   }

   void _clsReplicateSet::setLastConsultTick( UINT64 tick )
   {
      _lastConsultTick = tick ;
   }

   UINT64 _clsReplicateSet::getLastConsultTick() const
   {
      return _lastConsultTick ;
   }

   UINT32 _clsReplicateSet::_getThresholdTime( UINT64 diffSize )
   {
      UINT32 i = 0 ;
      UINT32 threshTime = 0 ;

      for ( ; i < CLS_SYNCCTRL_THRESHOLD_SIZE ; ++i )
      {
         if ( diffSize < _sizethreshold[ i ] )
         {
            break ;
         }
      }
      if ( i > 1 || ( 1 == i && _inSyncCtrl ) )
      {
         threshTime = _timeThreshold[ i - 1 ] ;
      }
      return threshTime ;
   }

   // PD_TRACE_DECLARE_FUNCTION (SDB__CLSREPSET__CANASSIGNLOGPAGE, "_clsReplicateSet::canAssignLogPage" )
   INT32 _clsReplicateSet::canAssignLogPage( UINT32 reqLen, pmdEDUCB *cb )
   {
      PD_TRACE_ENTRY ( SDB__CLSREPSET__CANASSIGNLOGPAGE );
      INT32 rc = SDB_OK ;

      UINT32 threshTime = 0 ;
      UINT32 waitTime = 0 ;
      DPS_LSN_OFFSET offset = DPS_INVALID_LSN_OFFSET ;
      DPS_LSN expectLSN ;
      BOOLEAN hasBlock = FALSE ;

      while ( SDB_OK == rc && PMD_IS_DB_AVAILABLE() )
      {
         offset = _sync.getSyncCtrlArbitLSN() ;
         if ( DPS_INVALID_LSN_OFFSET == offset )
         {
            break ;
         }

         expectLSN = _logger->expectLsn() ;
         // when log file number == 1, make sure all other nodes has uped
         if ( offset >= expectLSN.offset )
         {
            goto done ;
         }

         threshTime = _getThresholdTime( expectLSN.offset - offset ) ;
         if ( 0 == threshTime )
         {
            goto done ;
         }

         expectLSN.offset += reqLen ;
         if ( ( expectLSN.offset > offset + _logger->getLogFileSz() &&
                _logger->calcFileID( expectLSN.offset ) ==
                _logger->calcFileID( offset ) ) ||
              ( waitTime < threshTime ) )
         {
            if ( !_inSyncCtrl )
            {
               _inSyncCtrl = TRUE ;
               pmdGetKRCB()->setFlowControl( TRUE ) ;
               PD_LOG( PDWARNING, "Begin sync control...[expectLSN: %lld, "
                       "ArbitLSN: %lld, threshTime: %d, reqLen: %d, "
                       "waitTime: %d]", expectLSN.offset, offset,
                       threshTime, reqLen, waitTime ) ;
            }

            if ( !hasBlock )
            {
               cb->setBlock( EDU_BLOCK_SYNCCONTROL,
                             "Waiting for sync control" ) ;
               hasBlock = TRUE ;
            }
            ossSleep( CLS_SYNCCTRL_BASE_TIME ) ;
            waitTime += CLS_SYNCCTRL_BASE_TIME ;
         }
         else
         {
            break ;
         }

         if ( cb->isInterrupted() )
         {
            rc = SDB_APP_INTERRUPT ;
         }
         else if ( !_vote.primaryIsMe() )
         {
            rc = SDB_CLS_NOT_PRIMARY ;
         }
      }

   done:
      if ( hasBlock )
      {
         cb->unsetBlock() ;
      }
      if ( 0 == waitTime && _inSyncCtrl )
      {
         _inSyncCtrl = FALSE ;
         pmdGetKRCB()->setFlowControl( FALSE ) ;
         PD_LOG( PDWARNING, "End sync control" ) ;
      }
      PD_TRACE_EXITRC ( SDB__CLSREPSET__CANASSIGNLOGPAGE, rc );
      return rc ;
   }

   INT64 _clsReplicateSet::netIn()
   {
      return _agent->netIn() ;
   }

   INT64 _clsReplicateSet::netOut()
   {
      return _agent->netOut() ;
   }

   BOOLEAN _clsReplicateSet::checkVoteLaunch()
   {
      BOOLEAN checkResult = TRUE ;

      if ( SDB_OK != getSyncEmptyEvent()->wait( 0 ) )
      {
         PD_LOG( PDWARNING, "Repl sync log is running, "
                 "can't initial voting" ) ;
         checkResult = FALSE ;
      }
      else if ( !getBucket()->isEmpty() )
      {
         PD_LOG( PDWARNING, "Repl log is not empty, can't initial voting, "
                 "repl bucket size: %d", getBucket()->size() ) ;
         checkResult = FALSE ;
      }
      else if ( sdbGetTransCB()->isNeedSyncTrans() &&
                pmdGetStartup().isOK() )
      {
         PD_LOG( PDWARNING, "Trans info is not sync, can't initial voting" ) ;
         checkResult = FALSE ;
      }

      return checkResult ;
   }

   DPS_LSN _clsReplicateSet::getLocalExpectLSN()
   {
      return _logger->expectLsn() ;
   }

   DPS_LSN _clsReplicateSet::getLocalCurrentLSN()
   {
      return _logger->getCurrentLsn() ;
   }

   void _clsReplicateSet::getLSNWindow( DPS_LSN &fileBeginLSN,
                                        DPS_LSN &memBeginLSN,
                                        DPS_LSN &endLSN,
                                        DPS_LSN &expectLSN )
   {
      _logger->getLsnWindow( fileBeginLSN, memBeginLSN, endLSN, &expectLSN,
                             NULL ) ;
   }

   BOOLEAN _clsReplicateSet::isLocalOK()
   {
      return pmdGetStartup().isOK() ;
   }

   BOOLEAN _clsReplicateSet::isLocalSpare()
   {
      return ( SPARE_GROUPID == _info.local.columns.groupID ) ;
   }

   UINT8 _clsReplicateSet::getVoteWeight()
   {
      return pmdGetOptionCB()->weight() ;
   }

   UINT32 _clsReplicateSet::getSharingBreakTime()
   {
      return pmdGetOptionCB()->sharingBreakTime() ;
   }

   INT32 _clsReplicateSet::getSyncStrategy()
   {
      return pmdGetOptionCB()->syncStrategy() ;
   }

   BOOLEAN _clsReplicateSet::getDetectDisk()
   {
      return pmdGetOptionCB()->detectDisk() ;
   }

   INT32 _clsReplicateSet::onLocalNotFoundInGroup()
   {
      INT32 rc = SDB_SYS ;
      PMD_RESTART_DB( rc ) ;
      return rc ;
   }

   // PD_TRACE_DECLARE_FUNCTION (SDB__CLSREPSET_REPLSZCHECK, "_clsReplicateSet::replSizeCheck" )
   INT32 _clsReplicateSet::replSizeCheck( INT16 w, INT16 &finalW,
                                          _pmdEDUCB *cb,
                                          BOOLEAN isAfterData )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__CLSREPSET_REPLSZCHECK ) ;

      UINT32 nodeCnt = 0 ;
      UINT32 aliveCnt = 0 ;
      UINT32 faultCnt = 0 ;
      UINT32 ssCnt = 0 ;
      INT32 indoubtErr = SDB_OK ;
      UINT16 indoubtNodeID = 0 ;
      INT16 adjW = 0 ;
      UINT32 timeout = 0 ;
      BOOLEAN hasBlock = FALSE ;

      /// check valid
      if ( w < -1 || w > CLS_REPLSET_MAX_NODE_SIZE )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG( PDWARNING, "Invalid replsize: %d", w ) ;
         goto error ;
      }

      cb->setOrgReplSize( w ) ;

      if ( 1 == w && ( isAfterData || !_isAllNodeFatal ) )
      {
         finalW = w ;
         goto done ;
      }

      while( TRUE )
      {
         adjW = 0 ;

         if ( cb->isInterrupted() )
         {
            rc = SDB_APP_INTERRUPT ;
            goto error ;
         }

         getDetailInfo( nodeCnt, aliveCnt, faultCnt, ssCnt,
                        indoubtErr, indoubtNodeID ) ;

         /// One node in the group
         if ( 1 == nodeCnt )
         {
            finalW = 1 ;
            break ;
         }
         else if ( 1 == w )
         {
            if ( isAfterData || !_isAllNodeFatal )
            {
               finalW = 1 ;
               break ;
            }

            if ( timeout >= _fusingTimeout )
            {
               goto error ;
            }

            ossSleep( OSS_ONE_SEC ) ;
            timeout += OSS_ONE_SEC ;
            continue ;
         }
         else if ( FT_LEVEL_WHOLE == _pFTMgr->getFTLevel() )
         {
            nodeCnt = aliveCnt ;
         }

         /// When exist fault node
         if ( faultCnt > 0 )
         {
            switch ( _pFTMgr->getFTLevel() )
            {
               case FT_LEVEL_FUSING :
                  break ;
               case FT_LEVEL_SEMI :
                  if ( -1 == w )
                  {
                     adjW = faultCnt ;
                  }
                  break ;
               case FT_LEVEL_WHOLE :
                  adjW = faultCnt ;
                  break ;
               default:
                  break ;
            }
         }

         if ( 0 == w || w > (INT16)nodeCnt )
         {
            finalW = nodeCnt ;

            if ( FT_LEVEL_WHOLE == _pFTMgr->getFTLevel() )
            {
               adjW += ssCnt ;
            }
            else
            {
               ssCnt = 0 ;
            }
         }
         else if ( -1 == w )
         {
            finalW = aliveCnt ;
            adjW += ssCnt ;
         }
         else
         {
            finalW = w ;

            if ( FT_LEVEL_WHOLE == _pFTMgr->getFTLevel() )
            {
               adjW += ssCnt ;
            }
            else
            {
               ssCnt = 0 ;
            }
         }

         if ( finalW > (INT16)aliveCnt )
         {
            if ( isAfterData )
            {
               rc = SDB_CLS_WAIT_SYNC_FAILED ;
            }
            else
            {
               PD_LOG( PDERROR, "Alive num[%d] can not meet need[%d]",
                       aliveCnt, finalW ) ;
               rc = SDB_CLS_NODE_NOT_ENOUGH ;
            }
            goto error ;
         }
         else if ( finalW <= (INT16)( aliveCnt - faultCnt - ssCnt ) )
         {
            break ;
         }
         /// down level
         else if ( aliveCnt - faultCnt >= 2 && adjW > 0 )
         {
            finalW = (INT16)( aliveCnt - faultCnt - ssCnt ) ;
            if ( finalW < 2 )
            {
               finalW = 2 ;
            }
            break ;
         }

         if ( isAfterData || timeout >= _fusingTimeout )
         {
            goto error ;
         }

         if ( !hasBlock )
         {
            hasBlock = TRUE ;
            cb->setBlock( EDU_BLOCK_FT, "Waiting for fault-tolerant" ) ;
         }

         ossSleep( OSS_ONE_SEC ) ;
         timeout += OSS_ONE_SEC ;
         continue ;
      }

   done:
      if ( hasBlock )
      {
         cb->unsetBlock() ;
      }
      PD_TRACE_EXITRC( SDB__CLSREPSET_REPLSZCHECK, rc ) ;
      return rc ;
   error:
      if ( SDB_OK == rc )
      {
         if ( isAfterData )
         {
            rc = SDB_CLS_WAIT_SYNC_FAILED ;
         }
         else
         {
            rc = SDB_CLS_NODE_NOT_ENOUGH ;
            if ( indoubtErr )
            {
               /// Can't set rc = indoubtErr.
               /// Because onOPMsg() will retry for some error
               PD_LOG_MSG( PDERROR, "Fusing operation by indoubt error(%d) "
                           "from node(%u)", indoubtErr, indoubtNodeID ) ;
            }
            else
            {
               PD_LOG_MSG( PDERROR, "Fusing operation by error(%d)",
                           rc ) ;
            }
         }
      }
      goto done ;
   }

   void _clsReplicateSet::beforePrimaryActive()
   {
      // before primary
      _clsCB->ntyPrimaryChange( TRUE, SDB_EVT_OCCUR_BEFORE ) ;
   }

   void _clsReplicateSet::onPrimaryActive( const MsgRouteID &newPrimaryRID,
                                           const MsgRouteID &oldPrimaryRID )
   {
      // set global primary
      pmdSetPrimary( TRUE ) ;
   }

   void _clsReplicateSet::afterPrimaryActive( const MsgRouteID &newPrimaryRID,
                                              const MsgRouteID &oldPrimaryRID )
   {
      MsgCatPrimaryChange msg ;

      _clsCB->ntyPrimaryChange( TRUE, SDB_EVT_OCCUR_AFTER ) ;

      // update catalog
      msg.newPrimary = newPrimaryRID ;
      msg.oldPrimary = oldPrimaryRID ;
      callCatalog( (MsgHeader *)&msg, CLS_PRIMARY_UP_NOTIFY_TIMES ) ;
   }

   void _clsReplicateSet::beforePrimaryDeactive()
   {
      // primary change before
      _clsCB->ntyPrimaryChange( FALSE, SDB_EVT_OCCUR_BEFORE ) ;
   }

   void _clsReplicateSet::onPrimaryDeactive( const MsgRouteID &newPrimaryRID,
                                             const MsgRouteID &oldPrimaryRID )
   {
      // set global primary
      pmdSetPrimary( FALSE ) ;
   }

   void _clsReplicateSet::afterPrimaryDeactive( const MsgRouteID &newPrimaryRID,
                                                const MsgRouteID &oldPrimaryRID )
   {
      MsgCatPrimaryChange msg ;

      // primary change after
      _clsCB->ntyPrimaryChange( FALSE, SDB_EVT_OCCUR_AFTER ) ;

      // update catalog
      msg.newPrimary = newPrimaryRID ;
      msg.oldPrimary = oldPrimaryRID ;
      callCatalog( (MsgHeader *)&msg, CLS_PRIMARY_UP_NOTIFY_TIMES ) ;
   }

   void _clsReplicateSet::onLocalGroupExpired()
   {
      //download ;
      MsgCatGroupReq msg ;
      msg.id = _info.local ;
      _cata.call( (MsgHeader *)(&msg) ) ;
   }

   void _clsReplicateSet::beforeFoundNewPrimary()
   {
      _cata.remove( MSG_CAT_PAIMARY_CHANGE_RES ) ;
   }

   void _clsReplicateSet::afterFoundNewPrimary( const MsgRouteID &newPrimaryRID )
   {
      // do nothing
   }

   void _clsReplicateSet::processBeatLSN( const MsgRouteID &remote,
                                          const DPS_LSN &lsn )
   {
      // do nothing
   }

}
