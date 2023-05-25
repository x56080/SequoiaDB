/*******************************************************************************

   Copyright (C) 2011-2023 SequoiaDB Ltd.

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

   Source File Name = clsGroupModeJob.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          03/21/2023  LCX Initial Draft

   Last Changed =

*******************************************************************************/

#include "clsGroupModeJob.hpp"
#include "clsReplicateSet.hpp"
#include "pdTrace.hpp"
#include "clsTrace.hpp"
#include "clsRemoteOperator.hpp"
#include "rtn.hpp"
#include "catCommon.hpp"
#include "pmdDummySession.hpp"
#include "clsMgr.hpp"

namespace engine
{
   #define CLS_CRITICAL_RESTORE_THRESHOLD 0.125

   /* 
      _clsCriticalModeMonitorJob Implement
    */
   _clsCriticalModeMonitorJob::_clsCriticalModeMonitorJob( _clsGroupInfo *info )
   : _grpModeItem( info->grpMode.grpModeInfo[0] ),
     _localVersion( version.inc() + 1 ),
     _info( info )
   {
      SDB_ASSERT( NULL != _info, "can not be null" ) ;
   }

   _clsCriticalModeMonitorJob::~_clsCriticalModeMonitorJob()
   {
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLS_CRITICALMODE_MONITOR_DOIT, "_clsCriticalModeMonitorJob::doit" )
   INT32 _clsCriticalModeMonitorJob::doit( IExecutor *pExe,
                                           UTIL_LJOB_DO_RESULT &result,
                                           UINT64 &sleepTime )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__CLS_CRITICALMODE_MONITOR_DOIT ) ;

      pmdEDUCB *cb = dynamic_cast<pmdEDUCB*>( pExe ) ;
      result = UTIL_LJOB_DO_CONT ;

      const clsGroupMode grpMode = _info->grpMode ;

      if ( _localVersion < version.fetch() )
      {
         PD_LOG( PDDEBUG, "There is a new job executed by other thread, quit this job" ) ;
         result = UTIL_LJOB_DO_FINISH ;
      }
      else if ( PMD_IS_DB_DOWN() )
      {
         PD_LOG( PDDEBUG, "DB is down, stop critical mode monitor" ) ;
         result = UTIL_LJOB_DO_FINISH ;
         rc = SDB_APP_INTERRUPT ;
      }
      // Only primary need to execute this job
      else if ( ! pmdIsPrimary() )
      {
         PD_LOG( PDDEBUG, "Primary changed, stop critical mode monitor" ) ;
         result = UTIL_LJOB_DO_FINISH ;
      }
      // The grpMode info in _vote has been updated, quit this job
      else if ( CLS_GROUP_MODE_CRITICAL != grpMode.mode )
      {
         PD_LOG( PDDEBUG, "Group mode changed, stop critical mode monitor" ) ;
         result = UTIL_LJOB_DO_FINISH ;
      }
      else
      {
         const _clsGrpModeItem &grpModeItem = grpMode.grpModeInfo[0] ;

         // Check if grpMode in clsReplicaSet is valid,
         // if effective node in critical is not primary, stop critical mode
         if ( ( INVALID_NODEID == _info->local.columns.nodeID ||
               _info->local.columns.nodeID != grpMode.grpModeInfo[0].nodeID ) &&
              ( MSG_INVALID_LOCATIONID == grpModeItem.locationID ||
                pmdGetLocationID() != grpModeItem.locationID ) )
         {
            rc = _stopCriticalMode( cb ) ;

            // Check rc to make sure the critical mode is stop
            if ( SDB_OK != rc )
            {
               sleepTime = CLS_CRITICALMODE_CHECK_INTERVAL ;
               result = UTIL_LJOB_DO_CONT ;
            }
            else
            {
               result = UTIL_LJOB_DO_FINISH ;
               PD_LOG( PDEVENT, "Stop critical mode: primary[nodeID: %u, Location: %s] "
                       "is not in effective nodes[nodeID: %u, Location: %s]",
                       _info->local.columns.nodeID, pmdGetLocation(),
                       grpMode.grpModeInfo[0].nodeID, grpMode.grpModeInfo[0].location.c_str() ) ;
            }
         }
         // Check if this job is expired
         else if ( grpModeItem.locationID == _grpModeItem.locationID &&
                   grpModeItem.nodeID == _grpModeItem.nodeID &&
                   grpModeItem.updateTime.time == _grpModeItem.updateTime.time )
         {
            rc = _checkCriticalMode( cb, result, sleepTime ) ;
         }
         else
         {
            result = UTIL_LJOB_DO_FINISH ;
         }
      }

      PD_TRACE_EXITRC( SDB__CLS_CRITICALMODE_MONITOR_DOIT, rc ) ;
      return rc ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLS_CRITICALMODE_MONITOR__CHECKCRITICALMODE, "_clsCriticalModeMonitorJob::_checkCriticalMode" )
   INT32 _clsCriticalModeMonitorJob::_checkCriticalMode( pmdEDUCB *cb,
                                                         UTIL_LJOB_DO_RESULT &result,
                                                         UINT64 &sleepTime )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__CLS_CRITICALMODE_MONITOR__CHECKCRITICALMODE ) ;

      ossTimestamp curTime ;
      ossGetCurrentTime( curTime ) ;

      // UpdateTime < curTime < MinKeepTime
      if ( curTime.time < _grpModeItem.minKeepTime.time )
      {
         sleepTime = ( _grpModeItem.minKeepTime.time - curTime.time ) * OSS_ONE_SEC ;
         result = UTIL_LJOB_DO_CONT ;
      }
      // MinKeepTime <= curTime < MaxKeepTime
      else if ( curTime.time < _grpModeItem.maxKeepTime.time )
      {
         ossScopedRWLock( &_info->mtx, SHARED ) ;

         UINT32 aliveNum = _info->aliveSize() ;
         UINT32 nodeNum = _info->groupSize() ;

         // Check if majority nodes are alive
         if ( ( nodeNum / 2 ) < aliveNum )
         {
            UINT64 totalLogSize = pmdGetOptionCB()->getTotalLogSpace() ;
            DPS_LSN expectLsn = pmdGetKRCB()->getDPSCB()->expectLsn() ;

            // If the difference of primary and slave node's lsn is less or equal
            // than 0.125 * totalLogSize, we can assume that the slave node is in normal state
            const UINT64 diffOffset = CLS_CRITICAL_RESTORE_THRESHOLD * totalLogSize ;
            UINT64 minLsnOffset = expectLsn.offset > diffOffset ? expectLsn.offset - diffOffset : 0 ;
            UINT32 sucNum = _info->getAlivesByLsn( minLsnOffset ) ;

            // Check if majority nodes achieve a threshold lsn
            if ( ( nodeNum / 2 ) < sucNum )
            {
               rc = _stopCriticalMode( cb ) ;

               // Check rc to make sure the critical mode is stop
               if ( SDB_OK != rc )
               {
                  sleepTime = CLS_CRITICALMODE_CHECK_INTERVAL ;
                  result = UTIL_LJOB_DO_CONT ;
               }
               else
               {
                  result = UTIL_LJOB_DO_FINISH ;
                  PD_LOG( PDEVENT, "Stop critical mode: majority "
                          "of group nodes are in normal status" ) ;
               }
            }
         }
         else
         {
            sleepTime = CLS_CRITICALMODE_CHECK_INTERVAL ;
            result = UTIL_LJOB_DO_CONT ;
         }
      }
      // MaxKeepTime <= curTime
      else if ( _grpModeItem.maxKeepTime.time <= curTime.time )
      {
         rc = _stopCriticalMode( cb ) ;

         // Check rc to make sure the critical mode is stop
         if ( SDB_OK != rc )
         {
            sleepTime = CLS_CRITICALMODE_CHECK_INTERVAL ;
            result = UTIL_LJOB_DO_CONT ;
         }
         else
         {
            result = UTIL_LJOB_DO_FINISH ;

            CHAR maxTimeStr[ OSS_TIMESTAMP_STRING_LEN + 1 ] = { 0 } ;
            ossTimestampToString( const_cast< ossTimestamp& >( _grpModeItem.maxKeepTime ),
                                  maxTimeStr ) ;
            PD_LOG( PDEVENT, "Stop critical mode: current time reach the MaxKeepTime[%s]",
                    maxTimeStr ) ;
         }
      }

      PD_TRACE_EXITRC( SDB__CLS_CRITICALMODE_MONITOR__CHECKCRITICALMODE, rc ) ;
      return rc ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLS_CRITICALMODE_MONITOR__STOPCRITICALMODE, "_clsCriticalModeMonitorJob::_stopCriticalMode" )
   INT32 _clsCriticalModeMonitorJob::_stopCriticalMode( pmdEDUCB *cb )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__CLS_CRITICALMODE_MONITOR__STOPCRITICALMODE ) ;

      IRemoteOperator *pRemoteOpr = NULL ;
      BOOLEAN attachedDummySession = FALSE ;
      pmdDummySession session ;

      if ( NULL == cb->getSession() )
      {
         session.attachCB( cb ) ;
         attachedDummySession = TRUE ;
      }

      rc = cb->getOrCreateRemoteOperator( &pRemoteOpr ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to get remote operator, rc: %d", rc ) ;

      rc = pRemoteOpr->stopCriticalMode( _info->local.columns.groupID ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to stop critical mode, rc: %d", rc ) ;

   done:
      if ( attachedDummySession )
      {
         session.detachCB() ;
      }
      PD_TRACE_EXITRC( SDB__CLS_CRITICALMODE_MONITOR__STOPCRITICALMODE, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   ossAtomic32 _clsCriticalModeMonitorJob::version( 0 ) ;

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLS_STARTCRITICALMODE_MONITOR, "clsStartCriticalModeMonitor" )
   INT32 clsStartCriticalModeMonitor( _clsGroupInfo *info )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__CLS_STARTCRITICALMODE_MONITOR ) ;

      clsCriticalModeMonitorJob *pJob = NULL ;

      pJob = SDB_OSS_NEW clsCriticalModeMonitorJob( info ) ;
      if ( NULL == pJob )
      {
         rc = SDB_OOM ;
         PD_LOG( PDERROR, "Failed to allocate CriticalModeMonitor job, rc: %d", rc ) ;
         goto error ;
      }

      // Set takeover to true, so we don't need to free this job
      rc = pJob->submit( TRUE ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "Failed to submit CriticalModeMonitor job, rc: %d", rc ) ;
         goto error ;
      }

   done:
      PD_TRACE_EXITRC( SDB__CLS_STARTCRITICALMODE_MONITOR, rc ) ;
      return rc ;

   error:
      goto done ;
   }


   /* 
      _clsGroupModeReqJob Implement
    */
   _clsGroupModeReqJob::_clsGroupModeReqJob( _clsGroupInfo *info, clsVoteMachine *vote )
   : _info( info ),
     _vote( vote )
   {
   }

   _clsGroupModeReqJob::~_clsGroupModeReqJob()
   {
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLS_GRPMODEREQ_DOIT, "_clsGroupModeReqJob::doit" )
   INT32 _clsGroupModeReqJob::doit( IExecutor *pExe,
                                    UTIL_LJOB_DO_RESULT &result,
                                    UINT64 &sleepTime )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__CLS_GRPMODEREQ_DOIT ) ;

      sleepTime = CLS_CRITICALMODE_CHECK_INTERVAL ;
      result = UTIL_LJOB_DO_CONT ;
      IRemoteOperator *pRemoteOpr = NULL ;
      pmdEDUCB *cb = dynamic_cast<pmdEDUCB*>( pExe ) ;
      SDB_RTNCB *rtnCB = pmdGetKRCB()->getRTNCB() ;

      INT64 contextID = -1 ;
      const CHAR *pCommand = CMD_ADMIN_PREFIX CMD_NAME_LIST_GROUPMODES ;
      BSONObj matcher, dummyObj ;
      vector< BSONObj > objList ;
      BOOLEAN attachedDummySession = FALSE ;
      pmdDummySession session ;

      if ( NULL == cb->getSession() )
      {
         session.attachCB( cb ) ;
         attachedDummySession = TRUE ;
      }

      try
      {
         BSONObjBuilder matcherBuilder ;
         matcherBuilder.append( FIELD_NAME_GROUPID, _info->local.columns.groupID ) ;
         matcherBuilder.doneFast() ;
         matcher = matcherBuilder.obj() ;
      }
      catch ( std::exception &e )
      {
         rc = ossException2RC( &e ) ;
         PD_LOG( PDERROR, "Failed to create BSON object: %s", e.what() ) ;
         goto error ;
      }

      rc = cb->getOrCreateRemoteOperator( &pRemoteOpr ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to get remote operator, rc: %d", rc ) ;

      rc = pRemoteOpr->list( contextID, pCommand, matcher, dummyObj, dummyObj, dummyObj ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to list group modes, rc: %d", rc ) ;

      // Get return data
      while ( -1 != contextID )
      {
         rtnContextBuf buf ;
         rc = rtnGetMore( contextID, -1, buf, cb, rtnCB ) ;
         if ( SDB_DMS_EOC == rc )
         {
            contextID = -1 ;
            rc = SDB_OK ;
            break ;
         }
         PD_RC_CHECK( rc, PDERROR, "Failed to call get more, rc: %d", rc ) ;

         while ( !buf.eof() )
         {
            BSONObj obj ;
            try
            {
               rc = buf.nextObj( obj ) ;
               PD_RC_CHECK( rc, PDERROR, "Failed to get object from result, "
                            "rc: %d", rc ) ;
               objList.push_back( obj.getOwned() ) ;
            }
            catch ( exception &e )
            {
               PD_LOG( PDERROR, "Failed to get object from result, "
                       "occur exception %s", e.what() ) ;
               rc = ossException2RC( &e ) ;
               goto error ;
            }
         }
      }

      if ( objList.empty() )
      {
         rc = SDB_DMS_NOTEXIST ;
         PD_LOG( PDERROR, "Failed to list group modes, rc: %d", rc ) ;
         goto error ;
      }

      rc = _handleGroupModeRes( objList.front() ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to check group mode info, rc: %s", rc ) ;

      // If set grpMode successfully, quit the job
      result = UTIL_LJOB_DO_FINISH ;

   done:
      if ( -1 != contextID )
      {
         rtnKillContexts( 1, &contextID, cb, rtnCB ) ;
         contextID = -1 ;
      }
      if ( attachedDummySession )
      {
         session.detachCB() ;
      }
      PD_TRACE_EXITRC( SDB__CLS_GRPMODEREQ_DOIT, rc ) ;
      return rc ;

   error:
      goto done ;
   }


   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLS_GRPMODEREQ_HANDLEGRPPMODERES, "_clsGroupModeReqJob::_handleGroupModeRes" )
   INT32 _clsGroupModeReqJob::_handleGroupModeRes( const BSONObj &grpModeObj )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB__CLS_GRPMODEREQ_HANDLEGRPPMODERES );

      clsGroupMode grpMode ;
      const CLS_LOC_INFO_MAP &locMap = _info->locationInfoMap ;

      // Parse group mode info
      rc = catParseGrpModeObj( grpModeObj, locMap, grpMode ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDWARNING, "Parse group mode object failed, rc = %d", rc ) ;
         goto error ;
      }

      // Update group mode to local
      if ( CLS_GROUP_MODE_CRITICAL == grpMode.mode )
      {
         if ( ( INVALID_NODEID != _info->local.columns.nodeID &&
                _info->local.columns.nodeID == grpMode.grpModeInfo[0].nodeID ) ||
              ( CLS_INVALID_LOCATIONID != grpMode.grpModeInfo[0].locationID &&
                pmdGetLocationID() == grpMode.grpModeInfo[0].locationID ) )
         {
            // Set shadowTime = -1, which means this node is in critical mode
            rc = _vote->setGrpMode( grpMode, -1 ) ;
            PD_RC_CHECK( rc, PDERROR, "Failed to set critical mode, rc: %d", rc ) ;
         }
         else
         {
            // Set shadowTime = 0, which means this node is in normal mode
            rc = _vote->setGrpMode( grpMode, 0 ) ;
            PD_RC_CHECK( rc, PDERROR, "Failed to set critical mode, rc: %d", rc ) ;
         }
      }

   done :
      PD_TRACE_EXITRC ( SDB__CLS_GRPMODEREQ_HANDLEGRPPMODERES, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLS_STARTGRPMODEREQ, "clsStartGroupModeReqJob" )
   INT32 clsStartGroupModeReqJob( _clsGroupInfo *info, _clsVoteMachine *vote )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__CLS_STARTGRPMODEREQ ) ;

      _clsGroupModeReqJob *pJob = NULL ;

      pJob = SDB_OSS_NEW _clsGroupModeReqJob( info, vote ) ;
      if ( NULL == pJob )
      {
         rc = SDB_OOM ;
         PD_LOG( PDERROR, "Failed to allocate GroupModeReuire job, rc: %d", rc ) ;
         goto error ;
      }

      // Set takeover to true, so we don't need to free this job
      rc = pJob->submit( TRUE ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "Failed to submit GroupModeReuire job, rc: %d", rc ) ;
         goto error ;
      }

   done:
      PD_TRACE_EXITRC( SDB__CLS_STARTGRPMODEREQ, rc ) ;
      return rc ;

   error:
      goto done ;
   }
}