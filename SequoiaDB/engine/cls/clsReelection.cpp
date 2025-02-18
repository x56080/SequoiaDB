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
                                   _netRouteAgent *pAgent )
   :_vote( vote ),
    _syncMgr( syncMgr ),
    _pAgent( pAgent ),
    _level( CLS_REELECTION_LEVEL_NONE ),
    _step( CLS_REELECTION_STEP_NONE ),
    _waitMS( 0 ),
    _destID( 0 )
   {
      SDB_ASSERT( NULL != _vote &&
                  NULL != _syncMgr, "can not be null" ) ;
      _event.signalAll() ;
   }

   _clsReelection::~_clsReelection()
   {

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
                              UINT32 seconds,
                              pmdEDUCB *cb,
                              UINT16 destID )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__CLSREELECTION_RUN ) ;
      UINT32 timePassed = 0 ;
      BOOLEAN resetEvent = FALSE ;
      BOOLEAN needNtyEnd = FALSE ;
      MsgClsReelectNotify notifyMsg ;

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
         PD_LOG( PDERROR, "seconds of reelection should over 10" ) ;
         goto error ;
      }

      if ( !_vote->primaryIsMe() )
      {
         rc = SDB_CLS_NOT_PRIMARY ;
         PD_LOG( PDERROR, "only primary node can reelect" ) ;
         goto error ;
      }
      /// is self
      else if ( 0 != destID && destID == pmdGetNodeID().columns.nodeID )
      {
         // restore
         _vote->setShadowWeight( CLS_ELECTION_WEIGHT_USR_MIN ) ;
         goto done ;
      }

      if ( !ossCompareAndSwap32( &_level, CLS_REELECTION_LEVEL_NONE, lvl ) )
      {
         PD_LOG_MSG( PDERROR, "Can not do reelection when last reelection is not done" ) ;
         rc = SDB_OPERATION_CONFLICT ;
         goto error ;
      }

      PD_LOG( PDEVENT, "Run reelect(Level:%d, Seconds:%d, DestID: %u)",
              (INT32)lvl, seconds, destID ) ;

      _event.reset() ;
      resetEvent = TRUE ;
      needNtyEnd = TRUE ;

      rc = _wait4AllWriteDone( timePassed, seconds, lvl, cb ) ;
      if ( rc )
      {
         goto error ;
      }

      /// we need at least one replication done.
      /// otherwise this node will still be the primary.
      rc = _wait4Replica( timePassed, seconds, cb, destID ) ;
      if ( rc )
      {
         goto error ;   
      }

      if ( 0 != destID )
      {
         MsgRouteID routeID = pmdGetNodeID() ;

         /// notify dest node reelect begin
         notifyMsg.isLocation = 0 ;
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

      rc = _stepDown( timePassed, seconds, cb ) ;
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
         notifyMsg.isLocation = 0 ;
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
         _vote->setShadowWeight( CLS_ELECTION_WEIGHT_USR_MIN ) ;
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

   // PD_TRACE_DECLARE_FUNCTION (SDB__CLSREELECTION__STEPDOWN, "_clsReelection::_stepDown" )
   INT32 _clsReelection::_stepDown( UINT32 &timePassed,
                                    UINT32 timeout,
                                    pmdEDUCB *cb )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__CLSREELECTION__STEPDOWN ) ;
      pmdEDUMgr *eduMgr = pmdGetKRCB()->getEDUMgr() ;
      EDUID eduID = eduMgr->getSystemEDU( EDU_TYPE_CLUSTER ) ;
      rc = eduMgr->postEDUPost( eduID, PMD_EDU_EVENT_STEP_DOWN ) ;
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
}

