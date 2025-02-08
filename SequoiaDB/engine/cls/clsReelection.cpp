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
   _clsReelection::_clsReelection( _clsVoteMachine *vote,
                                   _clsSyncManager *syncMgr,
                                   _clsGroupInfo *info,
                                   _netRouteAgent *pAgent )
   :_vote( vote ),
    _syncMgr( syncMgr ),
    _info( info ),
    _pAgent( pAgent ),
    _level( CLS_REELECTION_LEVEL_NONE ),
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
      _event.signalAll() ;
      ossAtomicExchange32( &_level, CLS_REELECTION_LEVEL_NONE ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION (SDB__CLSREELECTION_RUN, "_clsReelection::run" )
   INT32 _clsReelection::run( CLS_REELECTION_LEVEL lvl,
                              INT32 seconds,
                              pmdEDUCB *cb,
                              UINT16 destID )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__CLSREELECTION_RUN ) ;
      UINT32 timePassed = 0 ;
      BOOLEAN resetEvent = FALSE ;
      BOOLEAN needNtyEnd = FALSE ;
      MsgClsReelectNotify notifyMsg ;

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
      // If location primary is replica group primary, do nothing
      else if ( isLocation && pmdIsPrimary() )
      {
         rc = SDB_OPERATION_CONFLICT ;
         PD_LOG_MSG( PDERROR, "The reelectLocation operation is not "
                     "supported in primary location set" ) ;
         goto error ;
      }
      // is self
      else if ( 0 != destID && destID == pmdGetNodeID().columns.nodeID )
      {
         // restore
         _vote->setShadowWeight( CLS_ELECTION_WEIGHT_USR_MIN ) ;
         _vote->resetElectionWeight( CLS_ELECTION_WEIGHT_REELECT_TARGET_NODE ) ;
         goto done ;
      }

      if ( !ossCompareAndSwap32( &_level, CLS_REELECTION_LEVEL_NONE, lvl ) )
      {
         PD_LOG_MSG( PDERROR, "Can not do reelection when last reelection is not done" ) ;
         rc = SDB_OPERATION_CONFLICT ;
         goto error ;
      }

      _event.reset() ;
      resetEvent = TRUE ;
      needNtyEnd = TRUE ;

      // Disable synchronize, this doesn't effect replica gruop's primary
      _syncMgr->disableSync() ;
      _blockSync = TRUE ;

      if ( isLocation )
      {
         rc = _wait4SyncDone( timePassed, seconds, lvl ) ;
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
         rc = _wait4ReplicaByBeat( timePassed, seconds, destID ) ;
      }
      else
      {
         rc = _wait4Replica( timePassed, seconds, cb, destID ) ;
      }
      if ( rc )
      {
         goto error ;   
      }

      if ( 0 != destID )
      {
         MsgRouteID routeID = pmdGetNodeID() ;

         /// notify dest node reelect begin
         notifyMsg.isLocation = isLocation ? 1 : 0 ;
         notifyMsg.type = CLS_REELECT_NOTIFY_BEGIN ;
         notifyMsg.timeout = ( timePassed + 10 < (UINT32)seconds ) ?
            ( seconds - timePassed + 5 ) * OSS_ONE_SEC : 10 * OSS_ONE_SEC ;

         routeID.columns.nodeID = destID ;
         routeID.columns.serviceID = MSG_ROUTE_REPL_SERVICE ;

         rc = _pAgent->syncSend( routeID, &(notifyMsg.header) ) ;
         if ( rc )
         {
            PD_LOG_MSG( PDERROR, "Send reelect notify-begin to node(%u) failed, rc: %d",
                        destID, rc ) ;
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
      if ( 0 != destID && needNtyEnd )
      {
         /// notify dest node reelect done
         MsgRouteID routeID = pmdGetNodeID() ;
         /// notify dest node reelect done
         notifyMsg.isLocation = isLocation ? 1 : 0 ;
         notifyMsg.type = CLS_REELECT_NOTIFY_END ;
         notifyMsg.timeout = 0 ;
         routeID.columns.nodeID = destID ;
         routeID.columns.serviceID = MSG_ROUTE_REPL_SERVICE ;
         INT32 rcTmp = _pAgent->syncSend( routeID, &(notifyMsg.header) ) ;
         if ( rcTmp )
         {
            PD_LOG( PDWARNING, "Send reelect notify-end to node(%u) failed, rc: %d",
                    destID, rcTmp ) ;
            /// ignore error
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

      while ( timePassed < timeout )
      {
         if ( cb->isInterrupted() )
         {
            rc = SDB_APP_INTERRUPT ;
            goto error ;
         }

         needWait = 0 ;

         /// wait transactions
         if ( waitTrans )
         {
            UINT32 selfTrans = cb->isTransaction() ? 1 : 0 ;
            /// get trans edu, and except self
            if ( transCB->getTransCBSize() > selfTrans )
            {
               needWait = 1 ;
            }
         }

         /// wait write context operations
         if ( waitContext && 0 == needWait )
         {
            /// except self
            if ( rtnCB->getWritingContextNum( cb->getID() ) > 0 )
            {
               needWait = 2 ;
            }
         }

         /// wait current write operations
         if ( waitEdu && 0 == needWait )
         {
            if ( eduMgr->hasWritingEDU( -1, 0, EDU_BLOCK_REELECT ) )
            {
               needWait = 3 ;
            }
         }

         if ( 0 != needWait )
         {
            ossSleep( 100 ) ;
            ++waitTimes ;

            if ( waitTimes >= 10 )
            {
               ++timePassed ;
               waitTimes = 0 ;
            }

            if ( timePassed >= timeout )
            {
               rc = SDB_TIMEOUT ;

               if ( 1 == needWait )
               {
                  PD_LOG_MSG( PDERROR, "Wait for transactions timeout" ) ;
               }
               else if ( 2 == needWait )
               {
                  PD_LOG_MSG( PDERROR, "Wait for write context(lob) operations timeout" ) ;
               }
               else
               {
                  PD_LOG_MSG( PDERROR, "Wait for write operations timeout" ) ;
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
                                         CLS_REELECTION_LEVEL lvl )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__CLSREELECTION__WAIT4ALLWRITEDONE_LOC ) ;

      while ( timePassed < timeout )
      {
         rc = sdbGetReplCB()->getSyncEmptyEvent()->wait( OSS_ONE_SEC ) ;
         if ( SDB_OK == rc )
         {
            break ;
         }

         ++timePassed ;
         rc = SDB_TIMEOUT ;
         PD_LOG_MSG( PDERROR, "Wait for repl-sync done timeout" ) ;
      }

      PD_TRACE_EXITRC( SDB__CLSREELECTION__WAIT4ALLWRITEDONE_LOC, rc ) ;
      return rc ;
   }

   // PD_TRACE_DECLARE_FUNCTION (SDB__CLSREELECTION__WAIT4REPLICA, "_clsReelection::_wait4Replica" )
   INT32 _clsReelection::_wait4Replica( UINT32 &timePassed,
                                        UINT32 timeout,
                                        pmdEDUCB *cb,
                                        UINT16 destID )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__CLSREELECTION__WAIT4REPLICA ) ;
      DPS_LSN lsn = pmdGetKRCB()->getDPSCB()->getCurrentLsn() ;
      UINT32 waitTimes = 0 ;

      while ( timePassed < timeout )
      {
         if ( cb->isInterrupted() )
         {
            rc = SDB_APP_INTERRUPT ;
            goto error ;
         }

         if ( _syncMgr->atLeastOne( lsn.offset, destID ) )
         {
            break ;
         }

         ossSleep( 100 ) ;
         ++waitTimes ;

         if ( waitTimes >= 10 )
         {
            ++timePassed ;
            waitTimes = 0 ;
         }
      }

      if ( timeout <= timePassed )
      {
         rc = SDB_TIMEOUT ;

         if ( 0 == destID )
         {
            PD_LOG_MSG( PDERROR, "Wait a replica-node for lsn(%lld) timeout", lsn.offset ) ;
         }
         else
         {
            PD_LOG_MSG( PDERROR, "Wait the replica-node(%u) for lsn(%lld) timeout",
                        destID, lsn.offset ) ;
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
                                              UINT16 destID )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__CLSREELECTION__WAIT4REPLICA_LOC ) ;

      map<UINT64, _clsSharingStatus>::const_iterator itr ;
      DPS_LSN lsn = pmdGetKRCB()->getDPSCB()->expectLsn() ;
      UINT32 waitTimes = 0 ;

      while ( timePassed < timeout )
      {
         BOOLEAN found = FALSE ;

         _info->mtx.lock_r() ;
         itr = _info->info.begin() ;
         while ( itr != _info->info.end() )
         {
            if ( 0 >= lsn.compare( itr->second.beat.endLsn ) )
            {
               if ( 0 == destID || destID == itr->second.beat.identity.columns.nodeID )
               {
                  found = TRUE ;
                  break ;
               }
            }
            ++itr ;
         }
         _info->mtx.release_r() ;

         if ( found )
         {
            break ;
         }
         ossSleep( 100 ) ;
         ++waitTimes ;

         if ( waitTimes >= 10 )
         {
            ++timePassed ;
            waitTimes = 0 ;
         }
      }

      if ( timeout <= timePassed )
      {
         rc = SDB_TIMEOUT ;

         if ( 0 == destID )
         {
            PD_LOG_MSG( PDERROR, "Wait a replica-node for lsn(%lld) timeout", lsn.offset ) ;
         }
         else
         {
            PD_LOG_MSG( PDERROR, "Wait the replica-node(%u) for lsn(%lld) timeout",
                        destID, lsn.offset ) ;
         }

         goto error ;
      }

   done:
      PD_TRACE_EXITRC( SDB__CLSREELECTION__WAIT4REPLICA_LOC, rc ) ;
      return rc ;
   error:
      goto done ;
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
            break ;
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
               PD_LOG_MSG( PDERROR, "Wait reelect new primary timeout", rc ) ;
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

