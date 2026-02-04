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

   Source File Name = clsReelection.cpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          11/01/2015  YW  Initial Draft

   Last Changed =

*******************************************************************************/
#include "clsReelection.hpp"
#include "pd.hpp"
#include "clsTrace.hpp"
#include "pdTrace.hpp"
#include "clsSyncManager.hpp"
#include "clsVoteMachine.hpp"
#include "pmd.hpp"
#include "dpsLogWrapper.hpp"
#include "clsMgr.hpp"
#include "dpsTransCB.hpp"
#include "rtnCB.hpp"

namespace engine
{
   /*
      Tool functions implement
   */
   const CHAR* clsGetReelectionStepStr( CLS_REELECTION_STEP step )
   {
      const CHAR *str = "none" ;

      switch ( step )
      {
         case CLS_REELECTION_STEP_WAIT_WRITE :
            str = "wait write operations" ;
            break ;
         case CLS_REELECTION_STEP_WAIT_REPLICA :
            str = "wait replica sync" ;
            break ;
         case CLS_REELECTION_STEP_DEST_NOTIFY :
            str = "dest node notify" ;
            break ;
         case CLS_REELECTION_STEP_STEPDOWN :
            str  = "step down" ;
            break ;
         case CLS_REELECTION_STEP_WAIT_PRIMARY :
            str = "wait new primary" ;
            break ;
         case CLS_REELECTION_STEP_DONE :
            str = "done" ;
            break ;
         default :
            break ;
      }

      return str ;
   }

   /*
      _clsReelection implement
   */
   _clsReelection::_clsReelection( _clsVoteMachine *vote,
                                   _clsSyncManager *syncMgr,
                                   _clsGroupInfo *info,
                                   _netRouteAgent *pAgent )
   :_vote( vote ),
    _syncMgr( syncMgr ),
    _info( info ),
    _pAgent( pAgent ),
    _level( CLS_REELECTION_LEVEL_NONE ),
    _step( CLS_REELECTION_STEP_NONE ),
    _waitMS( 0 ),
    _destID( 0 ),
    _blockSync( FALSE )
   {
      SDB_ASSERT( NULL != _vote &&
                  NULL != _syncMgr &&
                  NULL != _info, "can not be null" ) ;
      _event.signalAll() ;
   }

   _clsReelection::~_clsReelection()
   {
      if ( _blockSync )
      {
         _syncMgr->enableSync() ;
         _blockSync = FALSE ;
      }
   }

   // PD_TRACE_DECLARE_FUNCTION (SDB__CLSREELECTION_WAIT, "_clsReelection::wait" )
   INT32 _clsReelection::wait( pmdEDUCB *cb, UINT32 timeout )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__CLSREELECTION_WAIT ) ;
      UINT32 timePassed = 0 ;

      if ( CLS_REELECTION_LEVEL_NONE != _level )
      {
         if ( _level >= CLS_REELECTION_LEVEL_3 && cb->isTransaction() )
         {
            /// don't block transaction
            goto done ;
         }

         if ( _level >= CLS_REELECTION_LEVEL_2 && -1 != cb->getCurrentContextID() )
         {
            /// don't block write context
            goto done ;
         }

         rc = _wait( timePassed, timeout, cb, TRUE ) ;
         if ( SDB_OK != rc )
         {
            goto error ;
         }
      }

   done:
      PD_TRACE_EXITRC( SDB__CLSREELECTION_WAIT, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   void _clsReelection::signal()
   {
      if ( CLS_REELECTION_STEP_NONE != _step )
      {
         PD_LOG( PDEVENT, "Run async reelect done" ) ;
      }
      _step = CLS_REELECTION_STEP_NONE ;
      _waitMS = 0 ;
      _destID = 0 ;

      _event.signalAll() ;
      ossAtomicExchange32( &_level, CLS_REELECTION_LEVEL_NONE ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION (SDB__CLSREELECTION_RUN, "_clsReelection::run" )
   INT32 _clsReelection::run( CLS_REELECTION_LEVEL lvl,
                              INT32 seconds,
                              pmdEDUCB *cb,
                              const SET_UINT16 &setDestID )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__CLSREELECTION_RUN ) ;
      UINT32 timePassed = 0 ;
      BOOLEAN resetEvent = FALSE ;
      BOOLEAN needNtyEnd = FALSE ;
      MsgClsReelectNotify notifyMsg ;

      SET_UINT16 setTmpDestID ;

      // Save location tag to prevent local location changing during exexuting this function
      BOOLEAN isLocation = _isLocation() ;

      if ( lvl <= CLS_REELECTION_LEVEL_NONE || lvl >= CLS_REELECTION_LEVEL_MAX )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG_MSG( PDERROR, "Invalid reelection level(%d), should be range [%d, %d]",
                     lvl,CLS_REELECTION_LEVEL_NONE + 1, CLS_REELECTION_LEVEL_MAX - 1 ) ;
         goto error ;
      }

      if ( seconds < 10 )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG_MSG( PDERROR, "Seconds[%d] of reelection should be greater than 10", seconds ) ;
         goto error ;
      }

      if ( !_vote->primaryIsMe() )
      {
         if ( isLocation )
         {
            rc = SDB_CLS_NOT_LOCATION_PRIMARY ;
         }
         else
         {
            rc = SDB_CLS_NOT_PRIMARY ;
         }
         PD_LOG( PDERROR, "only primary node can reelect" ) ;
         goto error ;
      }
      // include self
      else if ( setDestID.find( pmdGetNodeID().columns.nodeID ) != setDestID.end() )
      {
         // restore
         _vote->resetShadowWeight() ;
         goto done ;
      }
      // If location primary is replica group primary, do nothing
      else if ( isLocation && pmdIsPrimary() )
      {
         rc = SDB_OPERATION_CONFLICT ;
         PD_LOG_MSG( PDERROR, "The reelectLocation operation is not "
                     "supported in primary location set" ) ;
         goto error ;
      }

      /// check the all nodes whether in maintenance mode or not
      if ( !setDestID.empty() )
      {
         try
         {
            ossScopedRWLock lock( &(_info->mtx), SHARED ) ;

            if ( _info->groupSize() > 0 )
            {
               MsgRouteID routeID = _info->local ;
               map<UINT64, _clsSharingStatus>::const_iterator citrInfo ;
               SET_UINT16::const_iterator citr = setDestID.begin() ;
               while( citr != setDestID.end() )
               {
                  routeID.columns.nodeID = *citr ;
                  citrInfo = _info->info.find( routeID.value ) ;
                  if ( citrInfo != _info->info.end() )
                  {
                     const _clsSharingStatus &statusItem = citrInfo->second ;
                     if ( ! statusItem.isInMaintenanceMode() )
                     {
                        setTmpDestID.insert( *citr ) ;
                     }
                  }
                  ++citr ;
               }

               if ( setTmpDestID.empty() )
               {
                  PD_LOG_MSG( PDERROR, "Can not reelect to maintenance node" ) ;
                  rc = SDB_OPERATION_CONFLICT ;
                  goto error ;
               }
            }
            else
            {
               setTmpDestID = setDestID ;
            }
         }
         catch( std::exception &e )
         {
            rc = ossException2RC( &e ) ;
            PD_LOG( PDERROR, "Occur exception: %s", e.what() ) ;
            goto error ;
         }
      }

      if ( !ossCompareAndSwap32( &_level, CLS_REELECTION_LEVEL_NONE, lvl ) )
      {
         PD_LOG_MSG( PDERROR, "Can not do reelection when last reelection is not done" ) ;
         rc = SDB_OPERATION_CONFLICT ;
         goto error ;
      }

      PD_LOG( PDEVENT, "Run reelect(Level:%d, Seconds:%d, DestID: %s)",
              (INT32)lvl, seconds, _nodesToString( setTmpDestID ).c_str() ) ;

      _event.reset() ;
      resetEvent = TRUE ;
      needNtyEnd = TRUE ;

      // Disable synchronize, this doesn't effect replica gruop's primary
      _syncMgr->disableSync() ;
      _blockSync = TRUE ;

      if ( isLocation )
      {
         rc = _wait4SyncDone( timePassed, seconds, lvl, cb ) ;
      }
      else
      {
         rc = _wait4AllWriteDone( timePassed, seconds, lvl, cb ) ;
      }
      if ( rc )
      {
         goto error ;
      }

      /// we need at least one replication done.
      /// otherwise this node will still be the primary.
      if ( isLocation )
      {
         rc = _wait4ReplicaByBeat( timePassed, seconds, cb, setTmpDestID ) ;
      }
      else
      {
         rc = _wait4Replica( timePassed, seconds, cb, setTmpDestID ) ;
      }
      if ( rc )
      {
         goto error ;   
      }

      if ( ! setTmpDestID.empty() )
      {
         MsgRouteID routeID = pmdGetNodeID() ;
         BOOLEAN allNtyFailed = TRUE ;
         INT32 rcTmp = SDB_OK ;

         SET_UINT16::const_iterator citr = setTmpDestID.begin() ;
         while( citr != setTmpDestID.end() )
         {
            /// notify dest node reelect begin
            notifyMsg.isLocation = isLocation ? 1 : 0 ;
            notifyMsg.type = CLS_REELECT_NOTIFY_BEGIN ;
            notifyMsg.timeout = ( timePassed + 10 < (UINT32)seconds ) ?
               ( seconds - timePassed + 5 ) * OSS_ONE_SEC : 10 * OSS_ONE_SEC ;

            routeID.columns.nodeID = *citr ;
            routeID.columns.serviceID = MSG_ROUTE_REPL_SERVICE ;

            rc = _pAgent->syncSend( routeID, &(notifyMsg.header) ) ;
            if ( rc )
            {
               PD_LOG_MSG( PDERROR, "Send reelect notify-begin to node(%u) failed, rc: %d",
                           *citr, rc ) ;
               rcTmp = rc ;
            }
            else
            {
               allNtyFailed = FALSE ;
            }

            ++citr ;
         }

         if ( allNtyFailed )
         {
            rc = rcTmp ;
            goto error ;
         }
         else
         {
            ossSleep( 300 ) ;
         }
      }

      rc = _stepDown( timePassed, seconds, isLocation, cb ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "Failed to step down, rc: %d", rc ) ;
         goto error ;
      }

   done:
      if ( ! setTmpDestID.empty() && needNtyEnd )
      {
         /// notify dest node reelect done
         MsgRouteID routeID = pmdGetNodeID() ;

         SET_UINT16::const_iterator citr = setTmpDestID.begin() ;
         while( citr != setTmpDestID.end() )
         {
            /// notify dest node reelect done
            notifyMsg.isLocation = isLocation ? 1 : 0 ;
            notifyMsg.type = CLS_REELECT_NOTIFY_END ;
            notifyMsg.timeout = 0 ;
            routeID.columns.nodeID = *citr ;
            routeID.columns.serviceID = MSG_ROUTE_REPL_SERVICE ;
            INT32 rcTmp = _pAgent->syncSend( routeID, &(notifyMsg.header) ) ;
            if ( rcTmp )
            {
               PD_LOG( PDWARNING, "Send reelect notify-end to node(%u) failed, rc: %d",
                       *citr, rcTmp ) ;
               /// ignore error
            }
            ++citr ;
         }
      }
      if ( resetEvent )
      {
         signal() ;
      }
      if ( _blockSync )
      {
         _syncMgr->enableSync() ;
         _blockSync = FALSE ;
      }
      PD_TRACE_EXITRC( SDB__CLSREELECTION_RUN, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   INT32 _clsReelection::runAsync( CLS_REELECTION_LEVEL lvl,
                                   INT32 wiatMS,
                                   UINT16 destID )
   {
      INT32 rc = SDB_OK ;

      // Save location tag to prevent local location changing during exexuting this function
      BOOLEAN isLocation = _isLocation() ;

      if ( isLocation )
      {
         rc = SDB_OPTION_NOT_SUPPORT ;
         PD_LOG_MSG( PDERROR, "Location reelect can't support async mode" ) ;
         goto error ;
      }

      if ( lvl <= CLS_REELECTION_LEVEL_NONE || lvl >= CLS_REELECTION_LEVEL_MAX )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG_MSG( PDERROR, "Invalid reelection level(%d), should be range [%d, %d]",
                     lvl,CLS_REELECTION_LEVEL_NONE + 1, CLS_REELECTION_LEVEL_MAX - 1 ) ;
         goto error ;
      }

      if ( !_vote->primaryIsMe() )
      {
         rc = SDB_CLS_NOT_PRIMARY ;
         PD_LOG( PDERROR, "only primary node can reelect" ) ;
         goto error ;
      }
      // is self
      else if ( 0 != destID && destID == pmdGetNodeID().columns.nodeID )
      {
         // restore
         _vote->resetShadowWeight() ;
         goto done ;
      }

      if ( !ossCompareAndSwap32( &_level, CLS_REELECTION_LEVEL_NONE, lvl ) )
      {
         PD_LOG_MSG( PDERROR, "Can not do reelection when last reelection is not done" ) ;
         rc = SDB_OPERATION_CONFLICT ;
         goto error ;
      }

      PD_LOG( PDEVENT, "Run async reelect(WaitMS:%d, DestID:%u)", wiatMS, destID ) ;

      _step = CLS_REELECTION_STEP_WAIT_WRITE ;
      _waitMS = wiatMS ;
      _destID = destID ;

      _event.reset() ;

      onTimer( 0 ) ;

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _clsReelection::onTimer( UINT32 interval )
   {
      INT32 rc = SDB_OK ;
      UINT32 timePassed = 0 ;
      pmdEDUCB *cb = NULL ;

      if ( _waitMS > interval )
      {
         _waitMS -= interval ;
      }
      else
      {
         _waitMS = 0 ;
      }

      if ( CLS_REELECTION_STEP_NONE == _step )
      {
         goto done ;
      }

      cb = pmdGetThreadEDUCB() ;

      if ( CLS_REELECTION_STEP_WAIT_WRITE == _step )
      {
         rc = _wait4AllWriteDone( timePassed, 0, (CLS_REELECTION_LEVEL)_level, cb ) ;
         if ( rc )
         {
            goto error ;
         }
         _step = CLS_REELECTION_STEP_WAIT_REPLICA ;
         PD_LOG( PDEVENT, "Async reelect: Wait all write done" ) ;
      }

      if ( CLS_REELECTION_STEP_WAIT_REPLICA == _step )
      {
         rc = _wait4Replica( timePassed, 0, cb, _destID ) ;
         if ( rc )
         {
            goto error ;   
         }
         _step = CLS_REELECTION_STEP_DEST_NOTIFY ;
         PD_LOG( PDEVENT, "Async reelect: Wait for replica done" ) ;
      }

      if ( CLS_REELECTION_STEP_DEST_NOTIFY == _step )
      {
         if ( 0 != _destID )
         {
            MsgClsReelectNotify notifyMsg ;
            MsgRouteID routeID = pmdGetNodeID() ;
            /// notify dest node reelect begin
            notifyMsg.isLocation = 0 ;
            notifyMsg.type = CLS_REELECT_NOTIFY_BEGIN ;
            notifyMsg.timeout = _waitMS + 5 * OSS_ONE_SEC ;

            routeID.columns.nodeID = _destID ;
            routeID.columns.serviceID = MSG_ROUTE_REPL_SERVICE ;

            rc = _pAgent->syncSend( routeID, &(notifyMsg.header) ) ;
            if ( rc )
            {
               PD_LOG_MSG( PDERROR, "Send reelect notify-begin to node(%u) failed, rc: %d",
                           _destID, rc ) ;
               goto error ;
            }
            PD_LOG( PDEVENT, "Async reelect: Notify dest node with reelect-begin done" ) ;
         }
         _step = CLS_REELECTION_STEP_STEPDOWN ;
         /// fot next time
         goto done ;
      }

      if ( CLS_REELECTION_STEP_STEPDOWN == _step )
      {
         pmdEDUMgr *eduMgr = pmdGetKRCB()->getEDUMgr() ;
         EDUID eduID = eduMgr->getSystemEDU( EDU_TYPE_CLUSTER ) ;

         rc = eduMgr->postEDUPost( eduID, PMD_EDU_EVENT_STEP_DOWN,
                                   PMD_EDU_MEM_NONE, NULL, FALSE ) ;
         if ( rc )
         {
            PD_LOG_MSG( PDERROR, "Failed to post event to repl cb, rc: %d", rc ) ;
            goto error ;
         }
         _step = CLS_REELECTION_STEP_WAIT_PRIMARY ;
         PD_LOG( PDEVENT, "Async reelect: Post step down done" ) ;

         /// for next time
         goto done ;
      }

      if ( CLS_REELECTION_STEP_WAIT_PRIMARY == _step )
      {
         rc = _wait( timePassed, 0, cb, FALSE ) ;
         if ( rc )
         {
            goto error ;
         }
         _step = CLS_REELECTION_STEP_DONE ;
         PD_LOG( PDEVENT, "Async reelect: Wait new primary done" ) ;
      }

   done:
      if ( CLS_REELECTION_STEP_DONE == _step )
      {
         if ( 0 != _destID )
         {
            MsgClsReelectNotify notifyMsg ;
            /// notify dest node reelect done
            MsgRouteID routeID = pmdGetNodeID() ;
            /// notify dest node reelect done
            notifyMsg.isLocation = 0 ;
            notifyMsg.type = CLS_REELECT_NOTIFY_END ;
            notifyMsg.timeout = 0 ;
            routeID.columns.nodeID = _destID ;
            routeID.columns.serviceID = MSG_ROUTE_REPL_SERVICE ;
            INT32 rcTmp = _pAgent->syncSend( routeID, &(notifyMsg.header) ) ;
            if ( rcTmp )
            {
               PD_LOG( PDWARNING, "Send reelect notify-end to node(%u) failed, rc: %d",
                       _destID, rcTmp ) ;
               /// ignore error
            }
         }

         signal() ;
      }
      return rc ;
   error:
      if ( SDB_TIMEOUT == rc && _waitMS > 0 )
      {
         rc = SDB_OK ;
      }
      else
      {
         PD_LOG( PDERROR, "Async reelect: Do step(%d, %s) failed, rc: %d",
                 _step, clsGetReelectionStepStr( _step ), rc ) ;
         _step = CLS_REELECTION_STEP_DONE ;
      }
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION (SDB__CLSREELECTION__WAIT4ALLWRITEDONE, "_clsReelection::_wait4AllWriteDone" )
   INT32 _clsReelection::_wait4AllWriteDone( UINT32 &timePassed,
                                             UINT32 timeout,
                                             CLS_REELECTION_LEVEL lvl,
                                             pmdEDUCB *cb )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__CLSREELECTION__WAIT4ALLWRITEDONE ) ;
      pmdEDUMgr *eduMgr = pmdGetKRCB()->getEDUMgr() ;
      dpsTransCB *transCB = pmdGetKRCB()->getTransCB() ;
      SDB_RTNCB *rtnCB = pmdGetKRCB()->getRTNCB() ;

      BOOLEAN waitTrans = lvl >= CLS_REELECTION_LEVEL_3 ? TRUE : FALSE ;
      BOOLEAN waitContext = lvl >= CLS_REELECTION_LEVEL_2 ? TRUE : FALSE ;
      BOOLEAN waitEdu = lvl >= CLS_REELECTION_LEVEL_1 ? TRUE : FALSE ;

      UINT32 waitTimes = 0 ;
      UINT32 needWait = 0 ;

      while ( timePassed <= timeout )
      {
         if ( cb->isInterrupted() )
         {
            rc = SDB_APP_INTERRUPT ;
            goto error ;
         }

         needWait = 0 ;

         /// wait current write operations
         if ( waitEdu )
         {
            if ( eduMgr->hasWritingEDU( -1, 0, EDU_BLOCK_REELECT ) )
            {
               needWait = 1 ;
            }
         }

         /// wait transactions
         if ( waitTrans && 0 == needWait )
         {
            UINT32 selfTrans = cb->isTransaction() ? 1 : 0 ;
            /// get trans edu, and except self
            if ( transCB->getTransCBSize() > selfTrans )
            {
               needWait = 2 ;
            }
         }

         /// wait write context operations
         if ( waitContext && 0 == needWait )
         {
            /// except self
            if ( rtnCB->getWritingContextNum( cb->getID() ) > 0 )
            {
               needWait = 3 ;
            }
         }

         if ( 0 != needWait )
         {
            if ( timePassed < timeout )
            {
               ossSleep( 100 ) ;
               ++waitTimes ;
            }

            if ( waitTimes >= 10 )
            {
               ++timePassed ;
               waitTimes = 0 ;
            }

            if ( timePassed >= timeout )
            {
               rc = SDB_TIMEOUT ;

               if ( timeout > 0 )
               {
                  if ( 1 == needWait )
                  {
                     PD_LOG_MSG( PDERROR, "Wait for write operations timeout" ) ;
                  }
                  else if ( 2 == needWait )
                  {
                     PD_LOG_MSG( PDERROR, "Wait for transactions timeout" ) ;
                  }
                  else
                  {
                     PD_LOG_MSG( PDERROR, "Wait for write context(lob) operations timeout" ) ;
                  }
               }

               goto error ;
            }
         }
         else
         {
            break ;
         }
      }

   done:
      PD_TRACE_EXITRC( SDB__CLSREELECTION__WAIT4ALLWRITEDONE, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION (SDB__CLSREELECTION__WAIT4ALLWRITEDONE_LOC, "_clsReelection::_wait4SyncDone" )
   INT32 _clsReelection::_wait4SyncDone( UINT32 &timePassed,
                                         UINT32 timeout,
                                         CLS_REELECTION_LEVEL lvl,
                                         pmdEDUCB *cb )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__CLSREELECTION__WAIT4ALLWRITEDONE_LOC ) ;
      UINT32 onceTime = 0 ;

      while ( timePassed <= timeout )
      {
         onceTime = timePassed < timeout ? OSS_ONE_SEC : 0 ;
         rc = sdbGetReplCB()->getSyncEmptyEvent()->wait( onceTime ) ;
         if ( SDB_OK == rc )
         {
            break ;
         }

         if ( onceTime > 0 )
         {
            ++timePassed ;
         }

         if ( timePassed >= timeout )
         {
            PD_LOG_MSG( PDERROR, "Wait for repl-sync done timeout" ) ;
            goto error ;
         }
         else if ( cb->isInterrupted() )
         {
            rc = SDB_APP_INTERRUPT ;
            goto error ;
         }
      }

      /// wait repl bucket
      while( timePassed <= timeout && sdbGetReplCB()->getBucket()->maxReplSync() > 0 )
      {
         onceTime = timePassed < timeout ? OSS_ONE_SEC : 0 ;
         rc = sdbGetReplCB()->getBucket()->waitEmpty( onceTime ) ;
         if ( SDB_OK == rc )
         {
            break ;
         }

         if ( onceTime > 0 )
         {
            ++timePassed ;
         }

         if ( timePassed >= timeout )
         {
            PD_LOG_MSG( PDERROR, "Wait for repl-sync bucket empty timeout" ) ;
            goto error ;
         }
         else if ( cb->isInterrupted() )
         {
            rc = SDB_APP_INTERRUPT ;
            goto error ;
         }
      }

   done:
      PD_TRACE_EXITRC( SDB__CLSREELECTION__WAIT4ALLWRITEDONE_LOC, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION (SDB__CLSREELECTION__WAIT4REPLICA1, "_clsReelection::_wait4Replica" )
   INT32 _clsReelection::_wait4Replica( UINT32 &timePassed,
                                        UINT32 timeout,
                                        pmdEDUCB *cb,
                                        const SET_UINT16 &setDestID )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__CLSREELECTION__WAIT4REPLICA1 ) ;
      DPS_LSN lsn = pmdGetKRCB()->getDPSCB()->expectLsn() ;
      UINT32 waitTimes = 0 ;

      while ( timePassed <= timeout )
      {
         if ( cb->isInterrupted() )
         {
            rc = SDB_APP_INTERRUPT ;
            goto error ;
         }

         if ( ! setDestID.empty() )
         {
            SET_UINT16::const_iterator citr = setDestID.begin() ;
            while( citr != setDestID.end() )
            {
               if ( _syncMgr->atLeastOne( lsn.offset, *citr ) )
               {
                  goto done ;
               }
               ++citr ;
            }
         }
         else
         {
            if ( _syncMgr->atLeastOne( lsn.offset, 0 ) )
            {
               goto done ;
            }
         }

         if ( timePassed < timeout )
         {
            ossSleep( 100 ) ;
            ++waitTimes ;
         }
         else
         {
            break ;
         }

         if ( waitTimes >= 10 )
         {
            ++timePassed ;
            waitTimes = 0 ;
         }
      }

      if ( timeout <= timePassed )
      {
         rc = SDB_TIMEOUT ;

         if ( timeout > 0 )
         {
            if ( setDestID.empty() )
            {
               PD_LOG_MSG( PDERROR, "Wait a replica-node for lsn(%lld) timeout", lsn.offset ) ;
            }
            else
            {
               PD_LOG_MSG( PDERROR, "Wait the replica-node(%s) for lsn(%lld) timeout",
                           _nodesToString( setDestID ).c_str(), lsn.offset ) ;
            }
         }

         goto error ;
      } 

   done:
      PD_TRACE_EXITRC( SDB__CLSREELECTION__WAIT4REPLICA1, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION (SDB__CLSREELECTION__WAIT4REPLICA, "_clsReelection::_wait4Replica" )
   INT32 _clsReelection::_wait4Replica( UINT32 &timePassed,
                                        UINT32 timeout,
                                        pmdEDUCB *cb,
                                        UINT16 destID )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__CLSREELECTION__WAIT4REPLICA ) ;
      DPS_LSN lsn = pmdGetKRCB()->getDPSCB()->expectLsn() ;
      UINT32 waitTimes = 0 ;

      while ( timePassed <= timeout )
      {
         if ( cb->isInterrupted() )
         {
            rc = SDB_APP_INTERRUPT ;
            goto error ;
         }

         if ( _syncMgr->atLeastOne( lsn.offset, destID ) )
         {
            goto done ;
         }

         if ( timePassed < timeout )
         {
            ossSleep( 100 ) ;
            ++waitTimes ;
         }
         else
         {
            break ;
         }

         if ( waitTimes >= 10 )
         {
            ++timePassed ;
            waitTimes = 0 ;
         }
      }

      if ( timeout <= timePassed )
      {
         rc = SDB_TIMEOUT ;

         if ( timeout > 0 )
         {
            if ( 0 == destID )
            {
               PD_LOG_MSG( PDERROR, "Wait a replica-node for lsn(%lld) timeout", lsn.offset ) ;
            }
            else
            {
               PD_LOG_MSG( PDERROR, "Wait the replica-node(%u) for lsn(%lld) timeout",
                           destID, lsn.offset ) ;
            }
         }

         goto error ;
      } 

   done:
      PD_TRACE_EXITRC( SDB__CLSREELECTION__WAIT4REPLICA, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION (SDB__CLSREELECTION__WAIT4REPLICA_LOC, "_clsReelection::_wait4ReplicaByBeat" )
   INT32 _clsReelection::_wait4ReplicaByBeat( UINT32 &timePassed,
                                              UINT32 timeout,
                                              pmdEDUCB *cb,
                                              const SET_UINT16 &setDestID )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__CLSREELECTION__WAIT4REPLICA_LOC ) ;

      DPS_LSN lsn = pmdGetKRCB()->getDPSCB()->expectLsn() ;
      UINT32 waitTimes = 0 ;

      while ( timePassed <= timeout )
      {
         BOOLEAN found = FALSE ;

         if ( cb->isInterrupted() )
         {
            rc = SDB_APP_INTERRUPT ;
            goto error ;
         }

         _info->mtx.lock_r() ;

         if ( !setDestID.empty() )
         {
            SET_UINT16::const_iterator citr = setDestID.begin() ;
            while( citr != setDestID.end() )
            {
               found = _info->atLeastOne( lsn.offset, *citr ) ;
               if ( found )
               {
                  break ;
               }
               ++citr ;
            }
         }
         else
         {
            found = _info->atLeastOne( lsn.offset, 0 ) ;
         }

         _info->mtx.release_r() ;

         if ( found )
         {
            goto done ;
         }

         if ( timePassed < timeout )
         {
            ossSleep( 100 ) ;
            ++waitTimes ;
         }
         else
         {
            break ;
         }

         if ( waitTimes >= 10 )
         {
            ++timePassed ;
            waitTimes = 0 ;
         }
      }

      if ( timeout <= timePassed )
      {
         rc = SDB_TIMEOUT ;

         if ( setDestID.empty() )
         {
            PD_LOG_MSG( PDERROR, "Wait a replica-node for lsn(%lld) timeout", lsn.offset ) ;
         }
         else
         {
            PD_LOG_MSG( PDERROR, "Wait the replica-node(%s) for lsn(%lld) timeout",
                        _nodesToString( setDestID ).c_str(), lsn.offset ) ;
         }

         goto error ;
      }

   done:
      PD_TRACE_EXITRC( SDB__CLSREELECTION__WAIT4REPLICA_LOC, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   string _clsReelection::_nodesToString( const SET_UINT16 &setDestID )
   {
      string strNode ;

      try
      {
         std::stringstream ss ;
         if ( setDestID.size() <= 0 )
         {
            ss << (UINT16)0 ;
         }
         else if ( 1 == setDestID.size() )
         {
            ss << *( setDestID.begin() ) ;
         }
         else
         {
            SET_UINT16::const_iterator citr = setDestID.begin() ;
            UINT32 tmpCount = 0 ;
            ss << "[" ;
            while( citr != setDestID.end() )
            {
               if ( tmpCount > 0 )
               {
                  ss << ", " ;
               }
               ss << *citr ;
               ++citr ;
               ++tmpCount ;
            }
            ss << "]" ;
         }
         strNode = ss.str() ;
      }
      catch( std::exception &e )
      {
         PD_LOG( PDWARNING, "Occur exception: %s", e.what() ) ;
         /// ignore error
      }

      return strNode ;
   }

   // PD_TRACE_DECLARE_FUNCTION (SDB__CLSREELECTION__STEPDOWN, "_clsReelection::_stepDown" )
   INT32 _clsReelection::_stepDown( UINT32 &timePassed,
                                    UINT32 timeout,
                                    BOOLEAN isLocation,
                                    pmdEDUCB *cb )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__CLSREELECTION__STEPDOWN ) ;
      pmdEDUMgr *eduMgr = pmdGetKRCB()->getEDUMgr() ;
      EDUID eduID = eduMgr->getSystemEDU( EDU_TYPE_CLUSTER ) ;

      rc = eduMgr->postEDUPost( eduID, PMD_EDU_EVENT_STEP_DOWN,
                                PMD_EDU_MEM_NONE, NULL, isLocation ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG_MSG( PDERROR, "Failed to post event to repl cb, rc: %d", rc ) ;
         goto error ;
      }

      rc = _wait( timePassed, timeout, cb, FALSE ) ;
      if ( SDB_OK != rc )
      {
         goto error ;
      }
   done:
      PD_TRACE_EXITRC( SDB__CLSREELECTION__STEPDOWN, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   INT32 _clsReelection::_wait( UINT32 &timePassed,
                                UINT32 timeout,
                                pmdEDUCB *cb,
                                BOOLEAN canSetBlock )
   {
      INT32 rc = SDB_OK ;
      BOOLEAN hasBlock = FALSE ;
      INT64   onceTime = 0 ; /// second
      BOOLEAN isFirst = TRUE ;
      UINT32  waitTimes = 0 ;

      while ( timePassed <= timeout )
      {
         if ( cb->isInterrupted() )
         {
            rc = SDB_APP_INTERRUPT ;
            goto error ;
         }

         if ( isFirst || timePassed >= timeout )
         {
            onceTime = 0 ;
            isFirst = FALSE ;
         }
         else
         {
            onceTime = 100 ;
         }

         rc = _event.wait( onceTime ) ;
         if ( SDB_OK == rc )
         {
            goto done ;
         }
         else if ( SDB_TIMEOUT == rc )
         {
            if ( onceTime > 0 )
            {
               ++waitTimes ;
               if ( waitTimes >= 10 )
               {
                  ++timePassed ;
                  waitTimes = 0 ;
               }
            }

            if ( timePassed >= timeout )
            {
               if ( timeout > 0 )
               {
                  PD_LOG_MSG( PDERROR, "Wait reelect new primary timeout" ) ;
               }
               goto error ;
            }

            if ( !hasBlock && canSetBlock )
            {
               cb->setBlock( EDU_BLOCK_REELECT, "Waiting for reelect" ) ;
               hasBlock = TRUE ;
            }
            rc = SDB_OK ;
            continue ;
         }
         else
         {
            PD_LOG_MSG( PDERROR, "Failed to wait reelect, rc: %d", rc ) ;
            goto error ;
         }
      }

      if ( timeout < timePassed )
      {
         rc = SDB_TIMEOUT ;
         goto error ;
      }

   done:
      if ( hasBlock )
      {
         cb->unsetBlock() ;
      }
      return rc ;
   error:
      goto done ;
   }

   OSS_INLINE BOOLEAN _clsReelection::_isLocation() const
   {
      return _vote->isLocation() ;
   }
}

