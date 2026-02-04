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

   Source File Name = clsSyncManager.hpp

   Descriptive Name =

   When/how to use:

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/14/2012  YW  Initial Draft

   Last Changed =

*******************************************************************************/
#include "clsSyncManager.hpp"
#include "pmdEDU.hpp"
#include "dpsLogWrapper.hpp"
#include "netRouteAgent.hpp"
#include "clsBase.hpp"
#include <map>
#include "pdTrace.hpp"
#include "clsTrace.hpp"
#include "pmd.hpp"
#include "utilReplSizePlan.hpp"
#include "utilBitmap.hpp"

using namespace std ;

namespace engine
{
   const UINT32 CLS_REPLSE_WRITE_ONE = 1 ;
   const UINT32 CLS_SYNC_SET_NUM = CLS_REPLSET_MAX_NODE_SIZE - 1;

   #define CLS_W_2_SUB( num ) ( (num) - 2 )
   #define CLS_SUB_2_W( sub ) ( (sub) + 2 )

   #define CLS_WAKE_W_TIMEOUT             ( 2000 )

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSSYNCMAG__CLSSYNCMAG, "_clsSyncManager::_clsSyncManager" )
   _clsSyncManager::_clsSyncManager( _netRouteAgent *agent,
                                     _clsGroupInfo *info,
                                     _clsGroupInfo *locationInfo ):
                                     _agent( agent ),
                                     _info( info ),
                                     _locationInfo( locationInfo ),
                                     _validSync( 0 ),
                                     _timeout( 0 ),
                                     _aliveCount( 0 ),
                                     _blockSync( 0 )
   {
      PD_TRACE_ENTRY ( SDB__CLSSYNCMAG__CLSSYNCMAG ) ;
      _syncSrc.value = MSG_INVALID_ROUTEID ;
      _wakeTimeout = 0 ;
      _syncWaitSize = 0 ;

      PD_TRACE_EXIT ( SDB__CLSSYNCMAG__CLSSYNCMAG ) ;
   }

   _clsSyncManager::~_clsSyncManager()
   {
      SDB_ASSERT( 0 == _blockSync.peek(), "block sync should be 0" ) ;
   }

   // The function is called already in _info->mtx.lock_w(),
   // so can't lock any way
   void _clsSyncManager::updateNodeStatus( const MsgRouteID & id,
                                           BOOLEAN valid )
   {
      for ( UINT32 i = 0 ; i < _validSync ; ++i )
      {
         // find
         if ( _notifyList[i].id.value == id.value )
         {
            _notifyList[i].valid = valid ;
            if ( valid )
            {
               ++_aliveCount ;
            }
            else
            {
               --_aliveCount ;
            }
            break ;
         }
      }
   }

   // The function is called by the full sync(source) thread, so need to
   // use lock
   void _clsSyncManager::notifyFullSync( const MsgRouteID & id )
   {
      _info->mtx.lock_r() ;

      for ( UINT32 i = 0 ; i < _validSync ; ++i )
      {
         // find
         if ( _notifyList[i].id.value == id.value )
         {
            _notifyList[i].setInFullSync() ;
            break ;
         }
      }

      _info->mtx.release_r() ;
   }

   // The function is called already in _info->mtx.lock_w(),
   // so can't lock any way
   BOOLEAN _clsSyncManager::updateNodeGrpMode( const MsgRouteID &id, CLS_GROUP_MODE grpMode )
   {
      for ( UINT32 i = 0 ; i < _validSync ; ++i )
      {
         // find
         if ( _notifyList[i].id.value == id.value )
         {
            _notifyList[i].grpMode = grpMode ;
            return TRUE ;
         }
      }
      return FALSE ;
   }

   // The function is called by shard session thread, so need to use lock
   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSSYNCMAG_GETARBITLSN, "_clsSyncManager::getSyncCtrlArbitLSN" )
   DPS_LSN_OFFSET _clsSyncManager::getSyncCtrlArbitLSN()
   {
      DPS_LSN_OFFSET offset = DPS_INVALID_LSN_OFFSET ;
      INT32 syncSty = pmdGetOptionCB()->syncStrategy() ;

      PD_TRACE_ENTRY ( SDB__CLSSYNCMAG_GETARBITLSN ) ;

      if ( 0 == _validSync || CLS_SYNC_NONE == syncSty )
      {
         goto done ;
      }
      else
      {
         ossScopedRWLock lock( &_info->mtx, SHARED ) ;

         for ( UINT32 i = 0 ; i < _validSync ; ++i )
         {
            /// fullsync or maintenance node
            if ( _notifyList[i].isInFullSync() || _notifyList[i].isInMaintenance() )
            {
               continue ;
            }
            /// invalid node
            else if ( CLS_SYNC_KEEPNORMAL == syncSty &&
                      FALSE == _notifyList[i].isValid() )
            {
               continue ;
            }
            /// cirtical mode
            else if ( CLS_GROUP_MODE_CRITICAL == _info->localGrpMode )
            {
               if ( ( _info->isLocationCritical() && !_notifyList[i].isInCriticalMode() ) ||
                    ( _info->isNodeCritical() && !_notifyList[i].isValid() ) )
               {
                  continue ;
               }
            }

            /// calc lsn
            if ( DPS_INVALID_LSN_OFFSET == offset ||
                 _notifyList[i].offset < offset )
            {
               offset = _notifyList[i].offset ;
            }
         }
      }

   done:
      PD_TRACE_EXIT ( SDB__CLSSYNCMAG_GETARBITLSN ) ;
      return offset ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSSYNCMAG_UPNFYLIST, "_clsSyncManager::updateNotifyList" )
   INT32 _clsSyncManager::updateNotifyList( BOOLEAN newNodeValid )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB__CLSSYNCMAG_UPNFYLIST ) ;

      /// info's changing is handled in one thread.
      /// no need to require lock.
      map<UINT64, _clsSharingStatus> &group = _info->info ;
      UINT32 removed = 0 ;
      UINT32 prevAlives = 0 ;
      UINT32 aliveRemoved = 0 ;
      _clsSyncStatus status[CLS_REPLSET_MAX_NODE_SIZE - 1] ;
      UINT32 valid = 0 ;

      ossScopedRWLock lock( &_info->mtx, EXCLUSIVE ) ;

      _aliveCount = _info->alives.size() ;

      /// find removed nodes
      for ( UINT32 i = 0; i < _validSync ; i++ )
      {
         if ( _notifyList[i].valid )
         {
            ++prevAlives ;
         }

         if ( group.end() ==
              group.find( _notifyList[i].id.value ) )
         {
            if ( _notifyList[i].valid )
            {
               ++aliveRemoved ;
            }
            ++removed ;
         }
         else
         {
            status[valid] = _notifyList[i] ;
            ++valid ;
         }
      }

      /// clear synclist
      if ( 0 != removed )
      {
         _clearSyncList( removed, aliveRemoved, prevAlives,
                         _validSync, status ) ;
      }

      UINT32 merge = valid ;
      map<UINT64, _clsSharingStatus>::const_iterator itr = group.begin() ;
      /// add new nodes
      for ( ; itr != group.end(); itr++ )
      {
         BOOLEAN has = FALSE ;
         for ( UINT32 j = 0; j < valid; j++ )
         {
            if ( itr->first == status[j].id.value )
            {
               has = TRUE ;
               status[j].locationID = itr->second.locationID ;
               status[j].affinitive = itr->second.isAffinitiveLocation ;
               status[j].locationIndex = itr->second.locationIndex ;
               break ;
            }
         }
         if ( !has )
         {
            status[merge].offset = 0 ;
            status[merge].syncOffset = 0 ;
            status[merge].id.value = itr->first ;
            status[merge].valid = newNodeValid ;
            status[merge].locationID = itr->second.locationID ;
            status[merge].affinitive = itr->second.isAffinitiveLocation ;
            status[merge].locationIndex = itr->second.locationIndex ;
            ++merge ;
         }
      }

      ossMemcpy( _notifyList, status, merge * sizeof( _clsSyncStatus ) ) ;
      _validSync = merge ;

      PD_TRACE_EXITRC ( SDB__CLSSYNCMAG_UPNFYLIST, rc ) ;
      return rc ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSSYNCMAG_SYNC, "_clsSyncManager::sync" )
   INT32 _clsSyncManager::sync( _clsSyncSession &session,
                                const UINT32 &w,
                                INT64 timeout,
                                BOOLEAN isFTWhole )
   {
      PD_TRACE_ENTRY ( SDB__CLSSYNCMAG_SYNC ) ;
      SDB_ASSERT( w <= CLS_REPLSET_MAX_NODE_SIZE &&
                  CLS_REPLSE_WRITE_ONE <= w,
                 "1 <= sync num <= CLS_REPLSET_MAX_NODE_SIZE" ) ;
      SDB_ASSERT( NULL != session.eduCB, "educb should not be NULL" ) ;
      SDB_ASSERT( DPS_INVALID_LSN_OFFSET != session.waitPlan.offset,
                  "end lsn should not be valid" ) ;
      INT32 rc = SDB_OK ;
      UINT32 sub = 0;
      BOOLEAN needWait = TRUE ;

      /// if w <= 1, return
      if ( CLS_REPLSE_WRITE_ONE >= w )
      {
         goto done ;
      }

      _info->mtx.lock_r() ;

      if ( 0 == _validSync )
      {
         _info->mtx.release_r() ;
         goto done ;
      }
      else if ( MSG_INVALID_ROUTEID == _info->primary.value ||
                _info->primary.value != _info->local.value )
      {
         /// has change to secondary. Why need to check primary again,
         /// because change primary will cut(0), but this thread has not push
         /// to wait queue, so, need to check again
         rc = SDB_CLS_WAIT_SYNC_FAILED ;
         _info->mtx.release_r() ;
         goto error ;
      }
      else if ( _aliveCount < _validSync && w > _aliveCount + 1 )
      {
         // if ReplSize is -1, or ReplSize is valid with FT whole mode,
         // we can degrade the ReplSize for wait sync, report node is down
         // to caller, who can adjust ReplSize if needed
         if ( session.waitPlan.isCriticalNodeMode ||
              ( w > CLS_REPLSIZE_CONSISTENCE_MIN &&
                ( session.canReCheck ||
                  ( session.eduCB->getOrgReplSize() >= CLS_REPLSIZE_SPECIAL_MIN &&
                    session.eduCB->getOrgReplSize() <= CLS_REPLSIZE_SPECIAL_MAX ) ||
                  ( CLS_REPLSIZE_ONE != session.eduCB->getOrgReplSize() &&
                    isFTWhole ) ) ) )
         {
            rc = SDB_DATABASE_DOWN ;
         }
         else
         {
            rc = SDB_CLS_WAIT_SYNC_FAILED ;
         }
         _info->mtx.release_r() ;
         goto error ;
      }
      else if ( w > _validSync + 1 )
      {
         sub = CLS_W_2_SUB( _validSync + 1 ) ;
      }
      else
      {
         sub = CLS_W_2_SUB( w ) ;
      }

      _mtxs[sub].get() ;
      if ( DPS_INVALID_LSN_OFFSET != _checkList[sub].offset &&
           session.waitPlan.isPassed( _checkList[sub] ) )
      {
         needWait = FALSE ;
         _mtxs[sub].release() ;
      }
      else if ( DPS_INVALID_LSN_OFFSET != _checkList[sub].offset &&
                session.waitPlan.offset < _checkList[sub].offset )
      {
         _mtxs[sub].release() ;
         if ( _validSync - 1 == sub )
         {
            // if it's last check list, session will not need to sync
            needWait = FALSE ;
         }
         else
         {
            rc = _jump( session, sub, needWait ) ;
         }
      }
      else
      {
         rc = _syncList[sub].push( session ) ;
         _mtxs[sub].release() ;
      }
      _info->mtx.release_r() ;

      if ( SDB_OK != rc )
      {
         goto error ;
      }
      else if ( needWait )
      {
         rc = _wait( session, sub, timeout ) ;
      }

   done:
      PD_TRACE_EXITRC ( SDB__CLSSYNCMAG_SYNC, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSSYNCMAG_COMPLETE, "_clsSyncManager::complete" )
   void _clsSyncManager::complete( const MsgRouteID &id,
                                   const DPS_LSN &lsn,
                                   UINT32 TID,
                                   const DPS_LSN &syncLsn )
   {
      PD_TRACE_ENTRY ( SDB__CLSSYNCMAG_COMPLETE ) ;
      if ( lsn.invalid() || MSG_INVALID_ROUTEID == id.value )
      {
         PD_LOG( PDDEBUG, "sync: invalid complete."
                 "[nodeid:%d] [lsn:%d, %lld]",
                 id.columns.nodeID, lsn.version, lsn.offset ) ;
         goto done ;
      }
      {
      _info->mtx.lock_r() ;
      _MsgRouteID primary = _info->primary ;
      _info->mtx.release_r() ;
      if ( primary.value == _info->local.value &&
           MSG_INVALID_ROUTEID != primary.value )
      {
         _complete( id, lsn.offset, syncLsn.offset ) ;
      }
      else if ( MSG_INVALID_ROUTEID != primary.value )
      {
         _MsgReplVirSyncReq msg ;
         msg.complete = lsn ;
         msg.from = id ;
         msg.next = syncLsn ;
         msg.header.TID = TID ;
         _agent->syncSend( primary, (MsgHeader *)&msg ) ;

         /// without plan
         _complete( id, lsn.offset, syncLsn.offset, TRUE ) ;
      }
      else
      {
         /// do noting
      }
      }
   done:
      PD_TRACE_EXIT ( SDB__CLSSYNCMAG_COMPLETE ) ;
      return ;
   }

   void _clsSyncManager::handleTimeout( const UINT32 &interval )
   {
      _wakeTimeout += interval ;

      if ( _wakeTimeout > CLS_WAKE_W_TIMEOUT )
      {
         ossScopedRWLock lock( &_info->mtx, SHARED ) ;
         CLS_WAKE_PLAN plan ;
         _createWakePlan( plan ) ;
         _wake( plan ) ;
      }
   }

   // The function is called by repl session(src), so need use lock
   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSSYNCMAG_GETNOTIFYNOES, "_clsSyncManager::getNotifyNodes" )
   UINT32 _clsSyncManager::getNotifyNodes( const DPS_LSN_OFFSET &offset, CLS_NODE_ARRAY &nodes )
   {
      UINT32 nodeCnt = 0 ;

      PD_TRACE_ENTRY ( SDB__CLSSYNCMAG_GETNOTIFYNOES ) ;
      SDB_ASSERT( DPS_INVALID_LSN_OFFSET != offset,
                  "offset should not be invalid" ) ;

      ossScopedRWLock lock( &_info->mtx, SHARED ) ;

      for ( UINT32 i = 0; i < _validSync ; i++ )
      {
         if ( 0 == _notifyList[i].id.value )
         {
            SDB_ASSERT( FALSE, "impossible" ) ;
         }
         /// compare the offset of lsn.
         /// the node which request the latest lsn
         /// will be nofitied.
         else if ( offset == _notifyList[i].offset )
         {
            if ( nodes.addNode( _notifyList[i].id.value ) )
            {
               ++nodeCnt ;
            }
         }
      }

      PD_TRACE_EXIT ( SDB__CLSSYNCMAG_GETNOTIFYNOES ) ;
      return nodeCnt ;
   }

   // The function is called by repl session(src), so need use lock
   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSSYNCMAG_NOTIFY, "_clsSyncManager::notify" )
   void _clsSyncManager::notify( const DPS_LSN_OFFSET &offset )
   {
      PD_TRACE_ENTRY ( SDB__CLSSYNCMAG_NOTIFY ) ;
      SDB_ASSERT( DPS_INVALID_LSN_OFFSET != offset,
                  "offset should not be invalid" ) ;
      _MsgSyncNotify msg ;
      msg.header.TID = CLS_TID_REPL_SYC ;

      ossScopedRWLock lock( &_info->mtx, SHARED ) ;

      for ( UINT32 i = 0; i < _validSync ; i++ )
      {
         if ( 0 == _notifyList[i].id.value )
         {
            SDB_ASSERT( FALSE, "impossible" ) ;
         }
         /// compare the offset of lsn.
         /// the node which request the latest lsn
         /// will be nofitied.
         else if ( offset == _notifyList[i].offset )
         {
            msg.header.routeID = _notifyList[i].id ;
            msg.header.routeID.columns.serviceID = MSG_ROUTE_REPL_SERVICE_CTRL ;
            _agent->syncSend( msg.header.routeID, (MsgHeader *)&msg ) ;
         }
         else
         {
            /// do nothing
         }
      }

      PD_TRACE_EXIT ( SDB__CLSSYNCMAG_NOTIFY ) ;
      return ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSSYNCMAG_NOTIFYNODES, "_clsSyncManager::notifyNodes" )
   void _clsSyncManager::notifyNodes( const CLS_NODE_ARRAY &nodes )
   {
      PD_TRACE_ENTRY ( SDB__CLSSYNCMAG_NOTIFYNODES ) ;

      _MsgSyncNotify msg ;
      msg.header.TID = CLS_TID_REPL_SYC ;

      for ( UINT32 i = 0 ; i < nodes.getLength() ; ++i )
      {
         msg.header.routeID.value = nodes.getNode( i ) ;
         msg.header.routeID.columns.serviceID = MSG_ROUTE_REPL_SERVICE_CTRL ;
         _agent->syncSend( msg.header.routeID, (MsgHeader *)&msg ) ;
      }

      PD_TRACE_EXIT ( SDB__CLSSYNCMAG_NOTIFYNODES ) ;
      return ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSSYNCMAG_GETSYNCSRC, "_clsSyncManager::getSyncSrc" )
   MsgRouteID _clsSyncManager::getSyncSrc( const set<UINT64> &blacklist,
                                           CLS_GROUP_VERSION &version,
                                           BOOLEAN useLocaction )
   {
      PD_TRACE_ENTRY ( SDB__CLSSYNCMAG_GETSYNCSRC ) ;
      MsgRouteID res ;
      UINT32 localLocationID = pmdGetLocationID() ;
      CLS_SELECT_RANGE selectType = CLS_SELECT_GROUP ;

      res.value = MSG_INVALID_ROUTEID ;

      if ( useLocaction && MSG_INVALID_LOCATIONID != localLocationID && !pmdIsLocationPrimary() )
      {
         selectType = CLS_SELECT_LOCATION ;
      }

      ossScopedRWLock lock( &_info->mtx, SHARED ) ;

      // update group info version
      version = _info->version ;

      res = _getSyncSrc( blacklist, selectType ) ;
      if ( MSG_INVALID_ROUTEID == res.value && CLS_SELECT_LOCATION == selectType )
      {
         selectType = CLS_SELECT_AFFINITY_LOCATION ;
         res = _getSyncSrc( blacklist, selectType ) ;

         if ( MSG_INVALID_ROUTEID == res.value )
         {
            selectType = CLS_SELECT_GROUP ;
            res = _getSyncSrc( blacklist, selectType ) ;
         }
      }

      if ( MSG_INVALID_ROUTEID == res.value )
      {
         res.value = _info->primary.value ;
      }

      PD_TRACE_EXIT ( SDB__CLSSYNCMAG_GETSYNCSRC ) ;
      return res ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSSYNCMAG__GETSYNCSRC, "_clsSyncManager::_getSyncSrc" )
   MsgRouteID _clsSyncManager::_getSyncSrc( const set<UINT64> &blacklist,
                                            CLS_SELECT_RANGE rangeType )
   {
      PD_TRACE_ENTRY ( SDB__CLSSYNCMAG__GETSYNCSRC ) ;
      MsgRouteID res ;
      res.value = MSG_INVALID_ROUTEID ;

      MsgRouteID priIds[ CLS_SYNC_SET_NUM ] ;
      MsgRouteID secIds[ CLS_SYNC_SET_NUM ] ;
      UINT32 priSub = 0, secSub = 0 ;

      UINT32 localLocationID = pmdGetLocationID() ;
      BOOLEAN isLocationPreferred = FALSE ;
      map<UINT64, _clsSharingStatus *>::iterator itr ;

      if ( CLS_SELECT_GROUP != rangeType && MSG_INVALID_LOCATIONID != localLocationID )
      {
         isLocationPreferred = TRUE ;
      }

      /// first select PEER Node, primary priority
      /// then select RC Node, secondary priority
      for ( itr = _info->alives.begin() ; itr != _info->alives.end() ; itr++ )
      {
         _clsSharingStatus *pStatus = itr->second ;

         if ( CLS_SYNC_STATUS_PEER == pStatus->beat.syncStatus &&
              0 == blacklist.count( itr->first ) )
         {
            if ( isLocationPreferred &&
                 ( ( CLS_SELECT_LOCATION == rangeType &&
                     pStatus->locationID != localLocationID ) ||
                   ( CLS_SELECT_AFFINITY_LOCATION == rangeType &&
                     !pStatus->isAffinitiveLocation ) ) )
            {
               continue ;
            }

            if ( pStatus->isPrimary( isLocationPreferred ) )
            {
               priIds[ priSub++ ].value = itr->first ;
            }
            else
            {
               secIds[ secSub++ ].value = itr->first ;
            }
         }
      }

      if ( 0 != priSub )
      {
         res.value = priIds[ ossRand() % priSub ].value ;
      }
      else if ( 0 != secSub )
      {
         res.value = secIds[ ossRand() % secSub ].value ;
      }
      else
      {
         priSub = 0 ;
         secSub = 0 ;

         for ( itr = _info->alives.begin() ; itr != _info->alives.end() ; itr++ )
         {
            _clsSharingStatus *pStatus = itr->second ;

            if ( 0 == blacklist.count( itr->first ) )
            {
               if ( isLocationPreferred &&
                    ( ( CLS_SELECT_LOCATION == rangeType &&
                        pStatus->locationID != localLocationID ) ||
                      ( CLS_SELECT_AFFINITY_LOCATION == rangeType &&
                        !pStatus->isAffinitiveLocation ) ) )
               {
                  continue ;
               }

               if ( pStatus->isPrimary( isLocationPreferred ) )
               {
                  priIds[ priSub++ ].value = itr->first ;
               }
               else
               {
                  secIds[ secSub++ ].value = itr->first ;
               }
            }

            /// when is location, location primary priority
            MsgRouteID *pPrioIds = isLocationPreferred ? priIds : secIds ;
            MsgRouteID *pOtherIds = isLocationPreferred ? secIds : priIds ;
            UINT32 prioSub = isLocationPreferred ? priSub : secSub ;
            UINT32 otherSub = isLocationPreferred ? secSub : priSub ;

            if ( 0 != prioSub )
            {
               res.value = pPrioIds[ ossRand() % prioSub ].value ;
            }
            else if ( 0 != otherSub )
            {
               res.value = pOtherIds[ ossRand() % otherSub ].value ;
            }
         }
      }

      PD_TRACE_EXIT ( SDB__CLSSYNCMAG__GETSYNCSRC ) ;
      return res ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSSYNCMAG_GETFULLSRC, "_clsSyncManager::getFullSrc" )
   MsgRouteID _clsSyncManager::getFullSrc( const set<UINT64> &blacklist,
                                           CLS_GROUP_VERSION &version,
                                           BOOLEAN useLocaction )
   {
      PD_TRACE_ENTRY ( SDB__CLSSYNCMAG_GETFULLSRC ) ;
      MsgRouteID id ;
      UINT32 localLocationID = pmdGetLocationID() ;
      CLS_SELECT_RANGE selectType = CLS_SELECT_GROUP ;

      id.value = MSG_INVALID_ROUTEID ;

      if ( useLocaction && MSG_INVALID_LOCATIONID != localLocationID )
      {
         selectType = CLS_SELECT_LOCATION ;
      }

      ossScopedRWLock lock( &_info->mtx, SHARED ) ;

      // update group info version
      version = _info->version ;

      id = _getFullSrc( blacklist, selectType ) ;
      if ( MSG_INVALID_ROUTEID == id.value && CLS_SELECT_LOCATION == selectType )
      {
         selectType = CLS_SELECT_AFFINITY_LOCATION ;
         id = _getFullSrc( blacklist, selectType ) ;

         if ( MSG_INVALID_ROUTEID == id.value )
         {
            selectType = CLS_SELECT_GROUP ;
            id = _getFullSrc( blacklist, selectType ) ;
         }
      }

      if ( MSG_INVALID_ROUTEID == id.value &&
           _info->primary.columns.nodeID != _info->local.columns.nodeID )
      {
         id.value = _info->primary.value ;
      }

      PD_TRACE_EXIT ( SDB__CLSSYNCMAG_GETFULLSRC ) ;
      return id ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSSYNCMAG__GETFULLSRC, "_clsSyncManager::_getFullSrc" )
   MsgRouteID _clsSyncManager::_getFullSrc( const set<UINT64> &blacklist,
                                            CLS_SELECT_RANGE rangeType )
   {
      PD_TRACE_ENTRY ( SDB__CLSSYNCMAG__GETFULLSRC ) ;
      MsgRouteID id ;

      id.value = MSG_INVALID_ROUTEID ;

      MsgRouteID secIds[ CLS_SYNC_SET_NUM ] ;
      MsgRouteID priIds[ CLS_SYNC_SET_NUM ] ;
      UINT32 secSub = 0, priSub = 0 ;

      UINT32 localLocationID = pmdGetLocationID() ;
      BOOLEAN isLocationPreferred = FALSE ;
      map<UINT64, _clsSharingStatus *>::iterator itr ;

      if ( CLS_SELECT_GROUP != rangeType && MSG_INVALID_LOCATIONID != localLocationID )
      {
         isLocationPreferred = TRUE ;
      }

      for ( itr = _info->alives.begin() ; itr != _info->alives.end(); itr++ )
      {
         _clsSharingStatus *pStatus = itr->second ;

         if ( 0 != blacklist.count( itr->first ) )
         {
            continue ;
         }
         else if ( 0 != pStatus->beat.getFTConfirmStat() ||
                   SERVICE_NORMAL != pStatus->beat.serviceStatus )
         {
            continue ;
         }
         else if ( isLocationPreferred &&
                   ( ( CLS_SELECT_LOCATION == rangeType &&
                       pStatus->locationID != localLocationID ) ||
                     ( CLS_SELECT_AFFINITY_LOCATION == rangeType &&
                       !pStatus->isAffinitiveLocation ) ) )
         {
            continue ;
         }
         else if ( pStatus->isPrimary( isLocationPreferred ) )
         {
            priIds[ priSub++ ].value = itr->first ;
         }
         else if ( 0 != pStatus->beat.endLsn.offset )
         {
            secIds[ secSub++ ].value = itr->first ;
         }
      }

      if ( 0 != secSub )
      {
         id = secIds[ ossRand() % secSub ] ;
      }
      else if ( 0 != priSub )
      {
         id = priIds[ ossRand() % priSub ] ;
      }

      PD_TRACE_EXIT ( SDB__CLSSYNCMAG__GETFULLSRC ) ;
      return id ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSSYNCMAG_CUT, "_clsSyncManager::cut" )
   void _clsSyncManager::cut( UINT32 alives, BOOLEAN isFTWhole )
   {
      PD_TRACE_ENTRY ( SDB__CLSSYNCMAG_CUT ) ;
      SDB_ASSERT( alives <= _validSync, "impossible" ) ;
      if ( _validSync < alives )
      {
         PD_LOG( PDWARNING, "sync: alives is bigger than valid sync."
                 "[alives:%d][valid:%d]", alives, _validSync ) ;
         goto done ;
      }
      {
         _clsSyncSession session ;
         for ( SINT32 i = (SINT32)_validSync - 1 ; i > (SINT32)alives - 1 ;
               --i )
         {
            _mtxs[i].get() ;
            while ( SDB_OK == _syncList[i].pop( session ) )
            {
               if ( session.waitPlan.isCriticalNodeMode ||
                    ( CLS_SUB_2_W( i ) > CLS_REPLSIZE_CONSISTENCE_MIN &&
                      ( session.canReCheck ||
                        ( session.eduCB->getOrgReplSize() >= CLS_REPLSIZE_SPECIAL_MIN &&
                          session.eduCB->getOrgReplSize() <= CLS_REPLSIZE_SPECIAL_MAX ) ||
                        ( CLS_REPLSIZE_ONE != session.eduCB->getOrgReplSize() &&
                          isFTWhole ) ) ) )
               {
                  session.eduCB->getEvent().signal( SDB_DATABASE_DOWN ) ;
               }
               else
               {
                  session.eduCB->getEvent().signal ( SDB_CLS_WAIT_SYNC_FAILED ) ;
               }
            }
            _mtxs[i].release() ;
         }
      }

   done:
      PD_TRACE_EXIT ( SDB__CLSSYNCMAG_CUT ) ;
      return ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSSYNCMAG_ATLEASTONE, "_clsSyncManager::atLeastOne" )
   BOOLEAN _clsSyncManager::atLeastOne( const DPS_LSN_OFFSET &offset,
                                        UINT16 ensureNodeID,
                                        BOOLEAN onlyInAlive )
   {
      BOOLEAN res = _validSync > 0 ? FALSE : TRUE ;
      PD_TRACE_ENTRY( SDB__CLSSYNCMAG_ATLEASTONE ) ;
      DPS_LSN lsn ;
      lsn.offset = offset ;

      ossScopedRWLock lock( &_info->mtx, SHARED ) ;

      for ( UINT32 i = 0; i < _validSync ; i++ )
      {
         if ( onlyInAlive && ( !_notifyList[i].valid || _notifyList[i].isInMaintenance() ) )
         {
            continue ;
         }

         /// Found ensureNodeID
         if ( 0 != ensureNodeID &&
              ensureNodeID == _notifyList[i].id.columns.nodeID )
         {
            if ( 0 >= lsn.compareOffset( _notifyList[i].offset ) )
            {
               res = TRUE ;
            }
            else
            {
               res = FALSE ;
            }
            break ;
         }
         else if ( 0 == ensureNodeID &&
                   0 >= lsn.compareOffset( _notifyList[i].offset ) )
         {
            res = TRUE ;
            break ;
         }
      }
      PD_TRACE_EXIT( SDB__CLSSYNCMAG_ATLEASTONE ) ;
      return res ;
   }

   // The function is called by repl session, so need to use lock
   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSSYNCMAG__COMPLETE, "_clsSyncManager::_complete" )
   void _clsSyncManager::_complete( const MsgRouteID &id,
                                    const DPS_LSN_OFFSET &offset,
                                    const DPS_LSN_OFFSET &syncOffset,
                                    BOOLEAN noPlan )
   {
      PD_TRACE_ENTRY ( SDB__CLSSYNCMAG__COMPLETE ) ;
      DPS_LSN lsn, syncLsn ;
      lsn.offset = offset ;
      syncLsn.offset = syncOffset ;

      /// update sync wait info
      _updateSyncWaitInfo( id.columns.nodeID, offset, syncOffset ) ;

      ossScopedRWLock lock( &_info->mtx, SHARED ) ;

      /// update notify list
      for ( UINT32 i = 0; i < _validSync ; i++ )
      {
         if ( _notifyList[i].id.value == id.value )
         {
            if ( lsn.compareOffset( _notifyList[i].offset ) > 0 )
            {
               _notifyList[i].offset = offset ;
            }
            if ( syncLsn.compareOffset( _notifyList[i].syncOffset ) > 0 )
            {
               _notifyList[i].syncOffset = syncOffset ;
            }
            break ;
         }
      }

      if ( !noPlan )
      {
         /// wake up agent thread which is waiting
         CLS_WAKE_PLAN plan ;
         _createWakePlan( plan ) ;
         _wake( plan, id.columns.nodeID, offset, syncOffset ) ;
      }
      PD_TRACE_EXIT ( SDB__CLSSYNCMAG__COMPLETE ) ;
      return ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSSYNCMAG__WAKE, "_clsSyncManager::_wake" )
   void _clsSyncManager::_wake( CLS_WAKE_PLAN &plan,
                                UINT16 wakeByNodeID,
                                DPS_LSN_OFFSET wakeByOffset,
                                DPS_LSN_OFFSET wakeSyncOffset )
   {
      /// eg: we got a plan : { 0, 5, 10 }
      /// begin from w = 2 sync list. we pop all nodes which
      /// lsn is lower than 10. then we erase 10 from plan.
      /// next, in the list which w = 3, we pop all nodes which
      /// lsn is lower than 5. then erase 5 from plan.
      /// at last, in the list which w = 4. we pop all nodes
      /// which lsn is lower than 0. pop 0 from plan.
      /// plan is empty. the waking is done.

      PD_TRACE_ENTRY ( SDB__CLSSYNCMAG__WAKE ) ;
      _wakeTimeout = 0 ;
      SDB_ASSERT( plan.size() <= CLS_REPLSET_MAX_NODE_SIZE - 1,
                  "plan size should <= CLS_REPLSET_MAX_NODE_SIZE - 1" ) ;

      UINT64 curTime = 0 ;
      _clsSyncSession session ;
      /// begin from w = 2.
      UINT32 sub = 0 ;
      CLS_WAKE_PLAN::reverse_iterator ritr = plan.rbegin();
      while ( ritr != plan.rend() )
      {
         /// get max elemenet
         DPS_LSN lsn ;
         const utilReplSizePlan &planItem = ritr->second ;
         lsn.offset = planItem.offset ;

         _mtxs[sub].get() ;
         _checkList[sub] = planItem ;
         while ( SDB_OK == _syncList[sub].root( session ) )
         {
            if ( 0 < lsn.compareOffset( session.waitPlan.offset ) )
            {
               monClassQuery *pMonQuery = session.eduCB->getMonQueryCB() ;

               if ( pMonQuery && pMonQuery->isBeginSyncWait() )
               {
                  curTime = ossGetCurrentMicroseconds() ;
               }

               if ( session.waitPlan.isPassed( _checkList[sub] ) )
               {
                  /// set sync wait info
                  if ( pMonQuery && pMonQuery->isBeginSyncWait() )
                  {
                     for ( UINT32 i = 0; i < _validSync ; i++ )
                     {
                        pMonQuery->setSyncWaitInfo( _notifyList[i].id.columns.nodeID,
                                                    _notifyList[i].offset,
                                                    _notifyList[i].syncOffset,
                                                    TRUE,
                                                    curTime ) ;
                     }
                  }

                  session.eduCB->getEvent().signal ( SDB_OK ) ;
                  _syncList[sub].pop( session ) ;
               }
               else
               {
                  PD_LOG( PDDEBUG, "Session[LSN:%llu, ID:%lld] need level up",
                          session.waitPlan.offset, session.eduCB->getID() ) ;
                  session.eduCB->getEvent().signal( SDB_OPERATION_RETRY ) ;
                  _syncList[sub].pop( session ) ;
               }
            }
            else
            {
               break ;
            }
         }
         _mtxs[sub].release() ;

         ++ritr ;
         ++sub ;
      }
      PD_TRACE_EXIT ( SDB__CLSSYNCMAG__WAKE ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSSYNCMAG__CTWAKEPLAN, "_clsSyncManager::_createWakePlan" )
   void _clsSyncManager::_createWakePlan( CLS_WAKE_PLAN &plan )
   {
      PD_TRACE_ENTRY ( SDB__CLSSYNCMAG__CTWAKEPLAN ) ;

      DPS_LSN_OFFSET offsetMin = DPS_INVALID_LSN_OFFSET - 1 ;
      utilReplSizePlan wakePlan ;
      UINT32 selfLocation = pmdGetLocationID() ;
      _utilStackBitmap< CLS_REPLSET_MAX_NODE_SIZE > isMarked ;

      for ( UINT32 i = 0; i < _validSync ; i++ )
      {
         if ( !_notifyList[ i ].isValid() )
         {
            continue ;
         }
         else if ( offsetMin > _notifyList[ i ].offset )
         {
            offsetMin = _notifyList[ i ].offset ;
         }
      }

      for ( UINT32 i = 0; i < _validSync ; i++ )
      {
         wakePlan.reset() ;
         if ( _notifyList[i].isInFullSync() )
         {
            /// DPS_INVALID_LSN_OFFSET is for full sync, so ignore the node.
            /// Save as min offset of other nodes
            wakePlan.offset = offsetMin ;
         }
         else if ( _notifyList[i].isInMaintenance() &&
                   _notifyList[i].offset > offsetMin )
         {
            /// ignored maintenance node
            wakePlan.offset = offsetMin ;
         }
         else
         {
            wakePlan.offset = _notifyList[i].offset ;
         }
         wakePlan.syncIndex = i ;

         if ( MSG_INVALID_LOCATIONID != selfLocation &&
              MSG_INVALID_LOCATIONID != _notifyList[i].locationID )
         {
            if ( selfLocation == _notifyList[i].locationID )
            {
               wakePlan.primaryLocationNodes = 1 ;
               wakePlan.affinitiveNodes = 1 ;
               isMarked.setBit( _notifyList[i].locationIndex ) ;
            }
            else
            {
               wakePlan.locations = 1 ;
               if ( _notifyList[i].affinitive )
               {
                  wakePlan.affinitiveLocations = 1 ;
                  wakePlan.affinitiveNodes = 1 ;
               }
            }
         }
         plan.insert( CLS_WAKE_PLAN::value_type( wakePlan.offset, wakePlan ) ) ;
      }

      if ( MSG_INVALID_LOCATIONID != selfLocation )
      {
         CLS_WAKE_PLAN::reverse_iterator ritr = plan.rbegin() ;
         wakePlan.reset() ;

         /// self info
         wakePlan.locations = 1 ;
         wakePlan.affinitiveLocations = 1 ;
         wakePlan.primaryLocationNodes = 1 ;
         wakePlan.affinitiveNodes = 1 ;

         while ( ritr != plan.rend() )
         {
            utilReplSizePlan &tmpPlan = ritr->second ;

            if ( !isMarked.testBit( _notifyList[tmpPlan.syncIndex].locationIndex ) )
            {
               wakePlan.locations += tmpPlan.locations ;
               wakePlan.affinitiveLocations += tmpPlan.affinitiveLocations ;
               isMarked.setBit( _notifyList[tmpPlan.syncIndex].locationIndex ) ;
            }

            wakePlan.primaryLocationNodes += tmpPlan.primaryLocationNodes ;
            wakePlan.affinitiveNodes += tmpPlan.affinitiveNodes ;

            tmpPlan.affinitiveLocations = wakePlan.affinitiveLocations ;
            tmpPlan.primaryLocationNodes = wakePlan.primaryLocationNodes ;
            tmpPlan.locations = wakePlan.locations ;
            tmpPlan.affinitiveNodes = wakePlan.affinitiveNodes ;

            ++ritr ;
         }
      }

      SDB_ASSERT( plan.size() == _validSync, "impossible") ;
      PD_TRACE_EXIT ( SDB__CLSSYNCMAG__CTWAKEPLAN ) ;
      return ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSSYNCMAG__WAIT, "_clsSyncManager::_wait" )
   INT32 _clsSyncManager::_wait( _clsSyncSession &session, UINT32 sub, INT64 timeout )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB__CLSSYNCMAG__WAIT ) ;
      PD_LOG( PDDEBUG, "sync: wait [w:%d]", CLS_SUB_2_W( sub ) ) ;
      pmdEDUEvent ev ;
      INT64 tmpTime = 0 ;
      BOOLEAN hasBlock = FALSE ;
      pmdEDUCB *cb = session.eduCB ;
      monClassQuery *pMonQuery = cb->getMonQueryCB() ;

      BOOLEAN monSyncInfo = FALSE ;
      UINT32 slowSyncThreshold = monGetSlowSyncThreshold() ;
      UINT32 syncCostTime = 0 ;  /// ms

      /// check is re-wait
      if ( pMonQuery && 0 != pMonQuery->syncWaitBeginTime &&
           pMonQuery->syncWaitLSN == session.waitPlan.offset )
      {
         if ( _regSyncWaitInfo( pMonQuery ) )
         {
            pMonQuery->reBeginSyncWait() ;
            monSyncInfo = TRUE ;
         }
      }

      while ( !cb->isInterrupted() )
      {
         tmpTime = timeout >= 0 ?
                   ( timeout > OSS_ONE_SEC ? OSS_ONE_SEC : timeout ) :
                   OSS_ONE_SEC ;

         /// fix tmp time
         if ( pMonQuery && !monSyncInfo && tmpTime + syncCostTime >= slowSyncThreshold )
         {
            if ( syncCostTime < slowSyncThreshold )
            {
               tmpTime = slowSyncThreshold - syncCostTime ;
            }
            else
            {
               tmpTime = 1 ;
            }
         }

         UINT64 curTime = 0 ;
         /// wait for responses from other nodes.
         INT32 rcTmp = cb->getEvent().wait ( tmpTime, &rc ) ;

         /// check wether to enable monitor sync wait
         if ( pMonQuery && !monSyncInfo && ( SDB_OK != rcTmp || SDB_OK != rc ) )
         {
            UINT64 syncBeginTime = 0 ; /// us
            BOOLEAN hasCalcCost = FALSE ;
            BOOLEAN isFirst = TRUE ;
            BOOLEAN enusreCapicity = TRUE ;

            if ( pMonQuery->isCurrentQueryTick( MON_TICK_SYNCWAIT, &syncBeginTime ) )
            {
               curTime = ossGetCurrentMicroseconds() ;
               if ( curTime > syncBeginTime )
               {
                  syncCostTime = ( curTime - syncBeginTime ) / 1000 ;
                  hasCalcCost = TRUE ;
               }
            }

            if ( !hasCalcCost )
            {
               syncCostTime += ( SDB_OK != rcTmp ? tmpTime : 1 ) ;
               hasCalcCost = TRUE ;
            }

            if ( syncCostTime >= slowSyncThreshold )
            {
               if ( 0 == curTime )
               {
                  curTime = ossGetCurrentMicroseconds() ;
               }
               pMonQuery->syncReplSize = CLS_SUB_2_W( sub ) ;
               pMonQuery->slowSyncCount += 1 ;
               if ( 0 != syncBeginTime )
               {
                  pMonQuery->syncWaitBeginTime = syncBeginTime ;
               }
               else if ( curTime > syncCostTime * 1000 )
               {
                  pMonQuery->syncWaitBeginTime = curTime - syncCostTime * 1000 ;
               }
               else
               {
                  pMonQuery->syncWaitBeginTime = curTime ;
               }
               pMonQuery->syncWaitLSN = session.waitPlan.offset ;

               /// lock and add nodes
               ossScopedRWLock lock( &_info->mtx, SHARED ) ;

               if ( !pMonQuery->syncInfos.empty() )
               {
                  isFirst = FALSE ;
               }
               else
               {
                  try
                  {
                     pMonQuery->reserveSyncInfo( _validSync ) ;
                  }
                  catch( std::exception &e )
                  {
                     PD_LOG( PDWARNING, "Occur exception: %s", e.what() ) ;
                     enusreCapicity = FALSE ;
                     /// ignore error
                  }
               }

               if ( enusreCapicity )
               {
                  for ( UINT32 i = 0; i < _validSync ; i++ )
                  {
                     monSyncWaitInfo waitInfo ;
                     waitInfo._nodeID = _notifyList[i].id.columns.nodeID ;
                     waitInfo._startPointNextOffset = _notifyList[i].syncOffset ;
                     waitInfo._startPointCompleteOffset = _notifyList[i].offset ;
                     waitInfo._endPointNextOffset = waitInfo._startPointNextOffset ;
                     waitInfo._endPointCompleteOffset = waitInfo._startPointCompleteOffset ;

                     /// update status
                     if ( (UINT64)~0 != waitInfo.getCompareMatchLsn() &&
                          waitInfo.getCompareMatchLsn() > pMonQuery->syncWaitLSN )
                     {
                        UINT64 costTime = 1 ;

                        if ( curTime > pMonQuery->syncWaitBeginTime )
                        {
                           costTime = curTime - pMonQuery->syncWaitBeginTime ;
                        }

                        waitInfo.setInfo( waitInfo._endPointCompleteOffset,
                                          waitInfo._endPointNextOffset,
                                          pMonQuery->syncWaitLSN, FALSE,
                                          costTime, curTime ) ;
                     }

                     if ( isFirst )
                     {
                        try
                        {
                           pMonQuery->addWaitInfo2SyncInfo( waitInfo ) ;
                        }
                        catch( std::exception &e )
                        {
                           PD_LOG( PDWARNING, "Occur exception: %s", e.what() ) ;
                           /// ignore error
                        }
                     }
                     else
                     {
                        pMonQuery->updateSyncWaitInfo( waitInfo ) ;
                     }
                  }

                  if ( _regSyncWaitInfo( pMonQuery ) )
                  {
                     /// update flag
                     monSyncInfo = TRUE ;
                     pMonQuery->beginSyncWait() ;
                  }
               }
            }
         }

         if ( SDB_OK != rcTmp )
         {
            if ( timeout >= 0 )
            {
               timeout -= tmpTime ;
               if ( timeout <= 0 )
               {
                  rc = SDB_TIMEOUT ;
                  break ;
               }
            }

            if ( !hasBlock )
            {
               cb->setBlock( EDU_BLOCK_SYNCWAIT, "Waiting replicas sync" ) ;
               hasBlock = TRUE ;
            }
            continue ;
         }
         else if ( SDB_OPERATION_RETRY == rc )
         {
            cb->getEvent().reset() ;

            if ( _validSync - 1 == sub )
            {
               // if it's last check list, session will not need to sync
               rc = SDB_OK ;
               goto done ;
            }

            BOOLEAN needWait = TRUE ;
            rc = _jump( session, sub, needWait ) ;
            if ( SDB_OK != rc )
            {
               goto done ;
            }
            else if ( !needWait )
            {
               goto done ;
            }
         }
         else
         {
            goto done ;
         }
      }

      if ( hasBlock )
      {
         cb->unsetBlock() ;
         hasBlock = FALSE ;
      }

      /// interrupted or timeout, clear info.
      {
         _mtxs[sub].get() ;
         _clsSyncMinHeap &heap = _syncList[sub] ;
         UINT32 i = 0 ;

         while ( i < heap.dataSize() )
         {
            if ( cb == heap[i].eduCB )
            {
               PD_LOG ( PDDEBUG, "Session[ID:%lld, LSN:%lld] interrupt,remove "
                        "from heap[sub:%d, index:%d]", heap[i].eduCB->getID(),
                        heap[i].waitPlan.offset, sub, i ) ;
               heap.erase( i ) ;
               break ;
            }
            ++i ;
         }
         _mtxs[sub].release() ;
         if ( SDB_OK == rc )
         {
            rc = SDB_APP_INTERRUPT ;
         }
      }

   done:
      if ( hasBlock )
      {
         cb->unsetBlock() ;
      }
      if ( monSyncInfo )
      {
         _unregSyncWaitInfo( pMonQuery ) ;
         pMonQuery->finishSyncWait() ;
      }
      PD_TRACE_EXITRC ( SDB__CLSSYNCMAG__WAIT, rc ) ;
      return rc ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSSYNCMAG__JUMP, "_clsSyncManager::_jump" )
   INT32 _clsSyncManager::_jump( _clsSyncSession &session,
                                 UINT32 &sub,
                                 BOOLEAN &needWait )
   {
      PD_TRACE_ENTRY ( SDB__CLSSYNCMAG__JUMP ) ;
      INT32 rc = SDB_OK ;

      needWait = FALSE ;

      while ( _validSync > sub )
      {
         _mtxs[++sub].get() ;
         if ( DPS_INVALID_LSN_OFFSET != _checkList[sub].offset &&
              session.waitPlan.isPassed( _checkList[sub] ) )
         {
            needWait = FALSE ;
            _mtxs[sub].release() ;
            break ;
         }
         else if ( DPS_INVALID_LSN_OFFSET != _checkList[sub].offset &&
                   session.waitPlan.offset < _checkList[sub].offset )
         {
            _mtxs[sub].release() ;
            // if it's last check list, session will not need to sync
            if ( _validSync - 1 == sub )
            {
               needWait = FALSE ;
               break ;
            }
         }
         else
         {
            PD_LOG( PDDEBUG, "Session[LSN:%llu, ID:%lld] has jumped to "
                    "check list[sub:%d]", session.waitPlan.offset,
                    session.eduCB->getID(), sub ) ;
            /// has jump, need set canReCheck
            session.canReCheck = TRUE ;
            rc = _syncList[sub].push( session ) ;
            _mtxs[sub].release() ;
            needWait = TRUE ;
            break ;
         }
      }

      PD_TRACE_EXITRC ( SDB__CLSSYNCMAG__JUMP, rc ) ;
      return rc ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSSYNCMAG__CRSYNCLIST, "_clsSyncManager::_clearSyncList" )
   void _clsSyncManager::_clearSyncList( UINT32 removed, UINT32 removedAlives,
                                         UINT32 preAlives, UINT32 preSyncNum,
                                         _clsSyncStatus *left )
   {
      PD_TRACE_ENTRY ( SDB__CLSSYNCMAG__CRSYNCLIST ) ;

      UINT32 leftNum = preSyncNum - removed ; // except self
      UINT32 endRemovedSub = preAlives - removedAlives ;

      /// loop every removed synclist
      UINT32 removedSub = preSyncNum - 1 ;
      for ( ; removedSub >= endRemovedSub ; --removedSub )
      {
         _mtxs[removedSub].get() ;
         if ( endRemovedSub > 0 )
         {
            _mtxs[endRemovedSub-1].get() ;
         }
         _clsSyncSession session ;

         while ( SDB_OK == _syncList[removedSub].pop( session ) )
         {
            UINT32 complete = 0 ;
            /// compute w's completion
            for ( UINT32 j = 0 ; j < leftNum ; ++j )
            {
               if ( session.waitPlan.offset < left[j].offset )
               {
                  ++complete ;
               }
            }
            if ( leftNum <= complete ||
                 removedSub + 1 <= complete )
            {
               session.eduCB->getEvent().signal ( SDB_OK ) ;
            }
            else if ( 0 == endRemovedSub ||
                      removedSub > endRemovedSub + removed - 1 )
            {
               session.eduCB->getEvent().signal ( SDB_CLS_WAIT_SYNC_FAILED ) ;
            }
            /// push the session which is not completed
            /// into the newlist.
            else
            {
               _syncList[endRemovedSub-1].push( session ) ;
            }
         }
         _mtxs[removedSub].release() ;
         if ( endRemovedSub > 0 )
         {
            _mtxs[endRemovedSub-1].release() ;
         }
         else if ( 0 == removedSub )
         {
            break ;
         }
      }
      PD_TRACE_EXIT ( SDB__CLSSYNCMAG__CRSYNCLIST ) ;
   }

   BOOLEAN _clsSyncManager::_regSyncWaitInfo( monClassQuery *pMonQuery )
   {
      BOOLEAN result = TRUE ;
      ossPoolVector<monSyncWaitInfo>::iterator it ;

      ossScopedLock lock( &_syncWaitLatch ) ;

      try
      {
         it = pMonQuery->syncInfos.begin() ;
         while ( it != pMonQuery->syncInfos.end() )
         {
            monSyncWaitInfo *pWaitInfo = &(*it) ;
            MAP_LSN_2_SYNCWAIT_INFO &mapSyncWaitInfo = _mapNode2SyncWaitInfo[ pWaitInfo->_nodeID ] ;
            if ( !mapSyncWaitInfo.insert( MAP_LSN_2_SYNCWAIT_INFO::value_type(
                                             pMonQuery->syncWaitLSN,
                                             clsSyncWaitItem( pMonQuery, pWaitInfo ) ) ).second )
            {
               result = FALSE ;
               _unregSyncWaitInfo_i( pMonQuery ) ;
               break ;
            }
            ++it ;
         }
      }
      catch( std::exception &e )
      {
         PD_LOG( PDERROR, "Occur exception: %s", e.what() ) ;
         result = FALSE ;
         _unregSyncWaitInfo_i( pMonQuery ) ;
      }

      _syncWaitSize = _mapNode2SyncWaitInfo.size() ;

      return result ;
   }

   void _clsSyncManager::_unregSyncWaitInfo_i( monClassQuery *pMonQuery, BOOLEAN canUseNtyList )
   {
      MAP_NODE_2_SYNCWAIT_INFO::iterator it = _mapNode2SyncWaitInfo.begin() ;
      while( it != _mapNode2SyncWaitInfo.end() )
      {
         MAP_LSN_2_SYNCWAIT_INFO &mapSyncWaitInfo = it->second ;

         MAP_LSN_2_SYNCWAIT_INFO::iterator itSync = mapSyncWaitInfo.find( pMonQuery->syncWaitLSN ) ;
         if ( itSync != mapSyncWaitInfo.end() )
         {
            clsSyncWaitItem &syncItem = itSync->second ;
            if ( canUseNtyList && 0 == syncItem._pWaitInfo->_finished )
            {
               /// should update the end point info
               for ( UINT32 i = 0; i < _validSync ; i++ )
               {
                  if ( _notifyList[i].id.columns.nodeID == syncItem._pWaitInfo->_nodeID )
                  {
                     syncItem._pWaitInfo->_endPointCompleteOffset = _notifyList[i].offset ;
                     syncItem._pWaitInfo->_endPointNextOffset = _notifyList[i].syncOffset ;
                     break ;
                  }
               }
            }

            /// erase
            mapSyncWaitInfo.erase( itSync ) ;
         }

         /// when map is empty, erase it
         if ( mapSyncWaitInfo.empty() )
         {
            _mapNode2SyncWaitInfo.erase( it++ ) ;
         }
         else
         {
            ++it ;
         }
      }
   }

   void _clsSyncManager::_unregSyncWaitInfo( monClassQuery *pMonQuery )
   {
      ossScopedRWLock lock( &_info->mtx, SHARED ) ;

      ossScopedLock lockWait( &_syncWaitLatch ) ;

      _unregSyncWaitInfo_i( pMonQuery, TRUE ) ;

      _syncWaitSize = _mapNode2SyncWaitInfo.size() ;
   }

   void _clsSyncManager::_updateSyncWaitInfo( UINT16 nodeID,
                                              const DPS_LSN_OFFSET &offset,
                                              const DPS_LSN_OFFSET &syncOffset )
   {
      /// fast check
      if ( _syncWaitSize > 0 )
      {
         UINT64 curTime = 0 ;
         UINT64 costTime = 0 ;

         ossScopedLock lock( &_syncWaitLatch ) ;

         MAP_NODE_2_SYNCWAIT_INFO::iterator it = _mapNode2SyncWaitInfo.find( nodeID ) ;
         if ( it != _mapNode2SyncWaitInfo.end() )
         {
            MAP_LSN_2_SYNCWAIT_INFO &mapSyncWaitInfo = it->second ;

            MAP_LSN_2_SYNCWAIT_INFO::iterator itSync = mapSyncWaitInfo.begin() ;
            while( itSync != mapSyncWaitInfo.end() )
            {
               clsSyncWaitItem &waitItem = itSync->second ;
               UINT64 compareLsn = waitItem._pWaitInfo->getCompareMatchLsn( offset, syncOffset ) ;

               if ( DPS_INVALID_LSN_OFFSET != compareLsn && compareLsn > itSync->first )
               {
                  if ( 0 == curTime )
                  {
                     curTime = ossGetCurrentMicroseconds() ;
                  }
                  
                  if ( curTime > waitItem._pMonQuery->syncWaitBeginTime )
                  {
                     costTime = curTime - waitItem._pMonQuery->syncWaitBeginTime ;
                  }
                  else
                  {
                     costTime = 1 ;
                  }

                  waitItem._pWaitInfo->setInfo( offset, syncOffset, itSync->first,
                                                FALSE, costTime, curTime ) ;

                  mapSyncWaitInfo.erase( itSync++ ) ;
               }
               else
               {
                  break ;
               }
            }

            if ( mapSyncWaitInfo.empty() )
            {
               _mapNode2SyncWaitInfo.erase( it ) ;
               _syncWaitSize = _mapNode2SyncWaitInfo.size() ;
            }
         }
      }
   }

}
