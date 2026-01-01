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
#ifndef CLSREPLICATESET_HPP_
#define CLSREPLICATESET_HPP_

#include "netRouteAgent.hpp"
#include "msgReplicator.hpp"
#include "msgCatalog.hpp"
#include "clsVoteMachine.hpp"
#include "msg.hpp"
#include "pmdObjBase.hpp"
#include "clsCatalogCaller.hpp"
#include "clsSyncManager.hpp"
#include "dms.hpp"
#include "clsReplBucket.hpp"
#include "dpsDef.hpp"
#include "ossQueue.hpp"
#include "utilCircularQueue.hpp"
#include "clsReelection.hpp"
#include "utilReplSizePlan.hpp"
#include "utilBitmap.hpp"
#include <vector>

using namespace std ;

namespace engine
{
   class _netRouteAgent ;
   class _clsMgr ;
   class _clsDataSrcBaseSession ;
   class _pmdEDUCB ;
   class _pmdFTMgr ;

   #define CLS_SYNCCTRL_THRESHOLD_SIZE          (10)
   #define CLS_SYNC_DFT_TIMEOUT                 ( 600 * OSS_ONE_SEC )

   #define CLS_SYNCWAIT_FIX_TIME_SLICE          ( 10 * OSS_ONE_SEC )
   #define CLS_DISABLE_SRC_INTERVAL             ( 3600 * OSS_ONE_SEC )

   #define CLS_SYNC_NOTIFY_CAPACITY             ( 32 )

   typedef _utilCircularStackBuffer< CLS_NODE_ARRAY, CLS_SYNC_NOTIFY_CAPACITY > CLS_SYNCNTY_QUEUE_BUFFER ;
   typedef _utilCircularQueue< CLS_NODE_ARRAY >                                 CLS_SYNCNTY_QUEUE_CONTAINER ;

   /*
      _clsSyncNotifyQueue define
   */
   class _clsSyncNotifyQueue : public ossQueue< CLS_NODE_ARRAY, CLS_SYNCNTY_QUEUE_CONTAINER >
   {
   protected:
      typedef ossQueue< CLS_NODE_ARRAY, CLS_SYNCNTY_QUEUE_CONTAINER > _BASE ;

   public:
      _clsSyncNotifyQueue()
      : _BASE( CLS_SYNCNTY_QUEUE_CONTAINER( &_buffer ) )
      {
      }

      ~_clsSyncNotifyQueue()
      {
      }

   protected:
      CLS_SYNCNTY_QUEUE_BUFFER _buffer ;
   } ;
   typedef _clsSyncNotifyQueue clsSyncNotifyQueue ;

   /*
      _clsReplicateSet define
   */
   class _clsReplicateSet : public _pmdObjBase, public _dpsEventHandler,
                            public _ICluster, public _clsReplayEventHandler
   {
      DECLARE_OBJ_MSG_MAP()

      public:
         _clsReplicateSet( _netRouteAgent *agent ) ;
         virtual ~_clsReplicateSet() ;

      public:
         /*
            When is SDB_DPS_INVALID_LSN_OFFSET means keep the same with
            ExpectLSN(in dps)
         */
         virtual UINT64    completeLsn( BOOLEAN doFast = TRUE,
                                        UINT32 *pVer = NULL ) ;
         virtual UINT32    lsnQueSize() ;
         /*
            When is SDB_DPS_INVALID_LSN_OFFSET means keep the same with
            ExpectLSN(in dps)
         */
         virtual BOOLEAN   primaryLsn( UINT64 &lsn, UINT32 *pVer = NULL ) ;

      public:
         OSS_INLINE BOOLEAN primaryIsMe()
         {
            return _vote.primaryIsMe() ;
         }

         OSS_INLINE BOOLEAN locationPrimaryIsMe()
         {
            return _locationVote.primaryIsMe() ;
         }

         OSS_INLINE BOOLEAN isActiveLocation() const
         {
            return _vote.hasElectionWeight( CLS_ELECTION_WEIGHT_ACTIVE_LOCATION ) ;
         }

         OSS_INLINE BOOLEAN isAffinitiveLocation() const
         {
            return _vote.hasElectionWeight( CLS_ELECTION_WEIGHT_AFFINITIVE_LOCATION ) ;
         }

         OSS_INLINE UINT8 getElectionWeight() const
         {
            return _vote.getElectionWeight() ;
         }

         OSS_INLINE clsBucket* getBucket ()
         {
            return &_replBucket ;
         }

         OSS_INLINE void setLocalID( const MsgRouteID &id )
         {
            _info.local = id ;
            _locationInfo.local = id ;
            /// _agent was set by clsMgr.
         }

         OSS_INLINE const UINT32 ailves( BOOLEAN isLocation = FALSE )
         {
            _clsGroupInfo &info = isLocation ? _locationInfo : _info ;
            ossScopedRWLock lock( &info.mtx, SHARED ) ;
            return info.aliveSize() ;
         }

         OSS_INLINE UINT32 groupSize( BOOLEAN isLocation = FALSE )
         {
            _clsGroupInfo &info = isLocation ? _locationInfo : _info ;
            ossScopedRWLock lock( &info.mtx, SHARED ) ;
            return info.groupSize() ;
         }

         OSS_INLINE UINT32 criticalSize()
         {
            ossScopedRWLock lock( &_info.mtx, SHARED ) ;
            return _info.criticalSize() ;
         }

         OSS_INLINE UINT32 maintenanceSize()
         {
            ossScopedRWLock lock( &_info.mtx, SHARED ) ;
            return _info.maintenanceSize() ;
         }

         OSS_INLINE UINT32 criticalAliveSize()
         {
            ossScopedRWLock lock( &_info.mtx, SHARED ) ;
            return _info.criticalAliveSize() ;
         }

         OSS_INLINE INT16 majoritySize()
         {
            INT16 w = 0 ;

            ossScopedRWLock lock( &_info.mtx, SHARED ) ;

            if ( CLS_GROUP_MODE_CRITICAL == _info.localGrpMode )
            {
               // If group is in critical mode, use critical size to calculate majority size
               w = (INT16)( _info.criticalSize() / 2 + 1 ) ;
            }
            else
            {
               w = (INT16)( _info.groupSize() / 2 + 1 ) ;
            }

            return w ;
         }

         OSS_INLINE CLS_GROUP_MODE getGrpMode()
         {
            ossScopedRWLock lock( &_info.mtx, SHARED ) ;
            return _info.grpMode.mode ;
         }

         void getDetailInfo( UINT32 &nodeCnt, UINT32 &aliveCnt,
                             UINT32 &falutCnt, UINT32 &ssCnt,
                             INT32 &indoubtErr,
                             UINT16 &indoubtNodeID,
                             utilLocationInfo *locationInfo = NULL,
                             const SDB_CONSISTENCY_STRATEGY strategy = SDB_CONSISTENCY_NODE,
                             BOOLEAN *pIsCriticalNodeMode = NULL ) ;

         OSS_INLINE BOOLEAN isAlive ( NodeID node )
         {
            BOOLEAN bAlive = FALSE ;
            _info.mtx.lock_r() ;
            map<UINT64, _clsSharingStatus *>::iterator it =
               _info.alives.find ( node.value ) ;
            if ( it != _info.alives.end() )
            {
               bAlive = TRUE ;
            }
            _info.mtx.release_r() ;

            return bAlive ;
         }

         OSS_INLINE BOOLEAN atLeastOneBySharingBeat( const DPS_LSN_OFFSET &offset,
                                                     UINT16 ensureNodeID = 0,
                                                     BOOLEAN onlyInAlive = TRUE )
         {
            ossScopedRWLock lock( &_info.mtx, SHARED ) ;
            return _info.atLeastOne( offset, ensureNodeID, onlyInAlive ) ;
         }

         OSS_INLINE DPS_LSN getMaxLsnBySharingBeat( UINT16 *pNodeID,
                                                    BOOLEAN onlyInAlive = TRUE )
         {
            ossScopedRWLock lock( &_info.mtx, SHARED ) ;
            return _info.getMaxLsn( pNodeID, onlyInAlive ) ;
         }

         OSS_INLINE _clsSyncManager *syncMgr()
         {
            return &_sync ;
         }

         OSS_INLINE _clsVoteMachine* voteMachine( BOOLEAN isLocation = FALSE )
         {
            return isLocation ? &_locationVote : &_vote ;
         }

         OSS_INLINE INT32 sync( const DPS_LSN_OFFSET &offset,
                                _pmdEDUCB *eduCB,
                                UINT32 w = 1,
                                INT64 timeout = CLS_SYNC_DFT_TIMEOUT )
         {
            if ( DPS_INVALID_LSN_OFFSET == offset || 1 >= w )
            {
               return SDB_OK ;
            }

            _clsSyncSession session ;
            session.eduCB = eduCB ;
            eduCB->getEvent().reset() ;

            if ( w > CLS_REPLSET_MAX_NODE_SIZE )
            {
               w = CLS_REPLSET_MAX_NODE_SIZE ;
            }

            session.waitPlan = eduCB->getOperator()->getWaitplan() ;
            session.waitPlan.offset = offset ;
            return _sync.sync( session, w, timeout,
                               FT_LEVEL_WHOLE == _pFTMgr->getFTLevel() ?
                                                             TRUE : FALSE ) ;
         }

         OSS_INLINE UINT32 getNtySessionNum ()
         {
            return _srcSessionNum ;
         }

         OSS_INLINE BOOLEAN isInStepUp() const
         {
            return _vote.isInStepUp() ;
         }

         OSS_INLINE BOOLEAN isInCriticalMode()
         {
            return CLS_GROUP_MODE_CRITICAL == _info.localGrpMode ;
         }

         OSS_INLINE BOOLEAN isInMaintenanceMode()
         {
            return CLS_GROUP_MODE_MAINTENANCE == _info.localGrpMode ;
         }

         OSS_INLINE BOOLEAN isInEnforcedGrpMode()
         {
            return _info.enforcedGrpMode ;
         }

         OSS_INLINE BOOLEAN isReadyForSrc( UINT64 curTick )
         {
            UINT64 lastLogMoveTick = _lastLogMoveTick.fetch() ;
            UINT64 curTimeSpan = pmdDBTickSpan2Time( curTick - lastLogMoveTick ) ;
            /// source is ready in the following two cases
            /// 1. During this full sync, the secondary replay log has not failed
            /// 2. The downtime of the last full sync has passed(curTimeSpan > 1h)
            return 0 == lastLogMoveTick || curTimeSpan > CLS_DISABLE_SRC_INTERVAL ;
         }

         OSS_INLINE void onReplayLogError()
         {
            _lastLogMoveTick.swap( pmdGetDBTick() ) ;
         }

         ossQueue< clsLSNNtyInfo >* getNtyQue() { return &_ntyQue ; }
         DPS_LSN_OFFSET getNtyLastOffset() const { return _ntyLastOffset ; }
         DPS_LSN_OFFSET getNtyProcessedOffset() const { return _ntyProcessedOffset ; }

         DPS_LSN_OFFSET getNtyReplayOffset() const { return _ntyReplayOffset ; }
         void updateNtyReplayOffset( DPS_LSN_OFFSET offset )
         {
            if ( offset > _ntyReplayOffset )
            {
               _ntyReplayOffset = offset ;
            }
         }
         void resetNtyReplayOffset( DPS_LSN_OFFSET offset )
         {
            _ntyReplayOffset = offset ;
         }

         void notify2Session( UINT32 suLID, UINT32 clLID, dmsExtentID extLID,
                              const DPS_LSN_OFFSET &offset ) ;

         virtual void onWriteLog( DPS_LSN_OFFSET offset ) ;

         virtual void onPrepareLog( UINT32 csLID, UINT32 clLID,
                                    INT32 extLID, DPS_LSN_OFFSET offset ) ;

         virtual void onMoveLog( DPS_LSN_OFFSET moveToOffset,
                                 DPS_LSN_VER moveToVersion,
                                 DPS_LSN_OFFSET expectOffset,
                                 DPS_LSN_VER expectVersion,
                                 DPS_MOMENT moment,
                                 INT32 errcode ) ;

         virtual void onReplayLog( UINT32 csLID, UINT32 clLID,
                                   INT32 extLID, DPS_LSN_OFFSET offset ) ;

         virtual INT32 canAssignLogPage( UINT32 reqLen, pmdEDUCB *cb ) ;

         virtual INT32 onCompleteOpr( _pmdEDUCB *cb, INT32 w )
         {
            INT32 rc = SDB_OK ;
            UINT32 timeout = 0 ;
            UINT32 onceTimeout = 0 ;
            BOOLEAN replCheckRC = SDB_OK ;
            monClassQuery *monQuery = w > 1 ? cb->getMonQueryCB() : NULL ;

            if ( monQuery )
            {
               monQuery->startBlockTimer( EDU_BLOCK_SYNCWAIT ) ;
               monQuery->syncWaitCount++ ;
            }

            while ( TRUE )
            {
               UINT32 tmpSyncWaitTimeout = SDB_OK == replCheckRC ?
                                           _syncwaitTimeout :
                                           _fusingTimeout ;

               if ( tmpSyncWaitTimeout <= timeout )
               {
                  onceTimeout = 1 ;
               }
               else if ( tmpSyncWaitTimeout < timeout +
                                              CLS_SYNCWAIT_FIX_TIME_SLICE )
               {
                  onceTimeout = tmpSyncWaitTimeout - timeout ;
               }
               else
               {
                  onceTimeout = CLS_SYNCWAIT_FIX_TIME_SLICE ;
               }

               rc = sync( cb->getEndLsn(), cb, w, onceTimeout ) ;
               if ( SDB_TIMEOUT == rc || SDB_DATABASE_DOWN == rc )
               {
                  if ( SDB_DATABASE_DOWN == rc ||
                       SDB_CLS_WAIT_SYNC_FAILED == replCheckRC )
                  {
                     rc = SDB_CLS_WAIT_SYNC_FAILED ;
                  }
                  timeout += onceTimeout ;
                  if ( timeout >= tmpSyncWaitTimeout )
                  {
                     PD_LOG( PDWARNING, "Wait sync timeout(HasWaitTime: %u, WaitTime: %u), rc: %d",
                             timeout, tmpSyncWaitTimeout, rc ) ;
                     break ;
                  }

                  INT16 tmpW = 0 ;
                  replCheckRC = replSizeCheck( cb->getOrgReplSize(),
                                               tmpW, cb, TRUE ) ;
                  /// check replsize again
                  if ( SDB_OK == replCheckRC )
                  {
                     w = tmpW ;
                  }
                  continue ;
               }

               break ;
            }

            if ( monQuery )
            {
               monQuery->stopBlockTimer() ;
            }

            /// clean saved org repl size
            cb->setOrgReplSize( 1 ) ;
            return rc ;
         }

         virtual void  onSwitchLogFile( UINT32 preLogicalFileId,
                                        UINT32 preFileId,
                                        UINT32 curLogicalFileId,
                                        UINT32 curFileId )
         {
            return ;
         }

         BOOLEAN isMajorityAlive()
         {
            ossScopedRWLock lock( &_info.mtx, SHARED ) ;

            return CLS_IS_MAJORITY( _info.getAlivesByTimeout( CLS_NODE_KEEPALIVE_TIMEOUT ),
                                    _info.groupSize() ) ||
                   ( CLS_GROUP_MODE_CRITICAL == _info.localGrpMode &&
                     CLS_IS_MAJORITY( _info.getCriticalAlivesByTimeout( CLS_NODE_KEEPALIVE_TIMEOUT ),
                                      _info.criticalSize() ) ) ;
         }

         virtual INT32 canAssignLogPageOnSecondary( UINT32 reqLen, _pmdEDUCB *cb )
         {
            return SDB_OK ;
         }

      public:
         void  regSession ( _clsDataSrcBaseSession *pSession ) ;
         void  unregSession ( _clsDataSrcBaseSession *pSession ) ;

      public:
         INT32 initialize() ;
         INT32 active() ;
         INT32 deactive () ;
         INT32 final() ;
         void  onConfigChange() ;
         void  ntyPrimaryChange( BOOLEAN primary,
                                 SDB_EVENT_OCCUR_TYPE type ) ;

         virtual void  onTimer ( UINT64 timerID, UINT32 interval ) ;

         INT32 handleMsg( NET_HANDLE handle, MsgHeader* msg ) ;

         INT32 handleEvent( pmdEDUEvent *event ) ;

         INT32 callCatalog( MsgHeader *header, UINT32 times = 1 ) ;

         const _clsCataCallerMeta* getCataCallerMeta( UINT32 key ) ;

         BOOLEAN getPrimaryInfo( _clsSharingStatus &primaryInfo ) ;

         void getGroupInfo( _MsgRouteID &primary,
                            vector<_netRouteNode > &group ) ;

         CLS_LOC_INFO_MAP getLocInfoMap()
         {
            ossScopedRWLock lock( &_info.mtx, SHARED ) ;
            return _info.locationInfoMap ;
         }

         INT32          postGroupModeInfo( const clsGroupMode &grpMode,
                                           INT32 shadowTime,
                                           BOOLEAN isLocalMode,
                                           BOOLEAN enforced = FALSE ) ;

         INT32          startGrpModeJob( UINT64 delayMS = 0 ) ;

         MsgRouteID     getPrimary () ;
         MsgRouteID     getLocationPrimary () ;
         BOOLEAN        isSendNormal( UINT64 nodeID ) ;

         ossEvent*      getFaultEvent() ;
         ossEvent*      getSyncEmptyEvent() ;

         INT64 netIn() ;
         INT64 netOut() ;

         INT32 reelect( CLS_REELECTION_LEVEL lvl,
                        INT32 seconds,
                        pmdEDUCB *cb,
                        const SET_UINT16 &setDestID ) ;

         void reelectionDone( BOOLEAN change2Primary = FALSE ) ;

         INT32 locationReelect( CLS_REELECTION_LEVEL lvl,
                                INT32 seconds,
                                pmdEDUCB *cb,
                                const SET_UINT16 &setDestID ) ;

         void locationReelectionDone( BOOLEAN change2Primary = FALSE ) ;

         /// this func is used to support command "forceStepUp".
         INT32 stepUp( UINT32 keepSeconds, pmdEDUCB *cb,
                       UINT32 waitSeconds = 0,
                       BOOLEAN enforced = FALSE ) ;

         INT32 waitReelect( pmdEDUCB *cb, UINT32 timeout ) ;

         INT32 primaryCheck( pmdEDUCB *cb, INT32 waitReelectSec ) ;
         INT32 replSizeCheck( INT16 w, INT16 &finalW, _pmdEDUCB *cb,
                              BOOLEAN isAfterData = FALSE ) ;

         INT32 aliveNode( const MsgRouteID &id ) ;

         UINT64   getLastConsultTick() const ;
         void     setLastConsultTick( UINT64 tick ) ;

         clsSyncNotifyQueue* getSyncNotifyQue() { return &_syncNotifyQue ; }

      private:
         INT32 _checkGroupInfo( const CLS_GROUP_VERSION &version,
                                const map<UINT64, _netRouteNode> &nodes ) ;

         INT32 _checkGrpModeInfo( CLS_GROUP_MODE grpMode ) ;

         INT32 _setGroupSet( const CLS_GROUP_VERSION &version,
                             const CLS_LOC_INFO_MAP &locationInfoMap,
                             map<UINT64, _netRouteNode> &nodes,
                             BOOLEAN &changeStatus ) ;

         INT32 _setLocationSet( const map<UINT64, _netRouteNode> &nodes ) ;
         INT32 _setLocationInfo( const map<UINT64, _netRouteNode> &nodes,
                                 CLS_LOC_INFO_MAP &locationInfoMap,
                                 const ossPoolString &activeLocation ) ;

         /*
            only call in CLUSTER Thread
         */
         void _setElectionWeight( const ossPoolString &activeLocation ) ;

         BOOLEAN _isUDPHandle( NET_HANDLE handle ) ;

         INT32 _alive( const _MsgRouteID &id,
                       BOOLEAN fromUDP,
                       BOOLEAN isLocation = FALSE ) ;

         INT32 _handleSharingBeat( NET_HANDLE handle, const _MsgClsBeat *msg ) ;

         INT32 _handleSharingBeatRes( NET_HANDLE handle,
                                      const _MsgClsBeatRes *msg ) ;

         INT32 _handleGroupRes( const MsgCatGroupRes *msg ) ;

         void _sharingBeat() ;

         INT32 _sendSharingBeat( _clsSharingStatus &status,
                                 MsgClsBeat *message ) ;

         void _checkBreak( const UINT32 &millisec ) ;

         UINT32 _getThresholdTime( UINT64 diffSize ) ;

         INT32 _handleStepDown( BOOLEAN isLocation ) ;

         INT32 _handleStepUp( UINT32 seconds ) ;

         INT32 _handleGroupModeUpdate( const clsGroupMode &grpMode,
                                       INT32 shadowTime,
                                       BOOLEAN isLocalMode,
                                       BOOLEAN enforced ) ;

         void _notifySrcSessions( UINT32 csLID, UINT32 clLID,
                                  INT32 extLID, DPS_LSN_OFFSET offset ) ;

         void _forceSrcSessions() ;

         void _calLocationAffinity( const map<UINT64, _netRouteNode> &nodes,
                                    CLS_LOC_INFO_MAP &locationInfoMap ) ;

         INT32 _handleBallot( const MsgHeader *header ) ;

         INT32 _handleBallotRes( const MsgHeader *header ) ;

      private:
         _netRouteAgent          *_agent ;
         _clsGroupInfo           _info ;
         _clsGroupInfo           _locationInfo ;
         _clsVoteMachine         _vote ;
         _clsVoteMachine         _locationVote ;
         _dpsLogWrapper          *_logger ;
         _pmdFTMgr               *_pFTMgr ;
         _clsSyncManager         _sync ;
         _clsCatalogCaller       _cata ;
         _clsReelection          _reelection ;
         _clsReelection          _locationReelection ;
         clsBucket               _replBucket ;
         _clsMgr                 *_clsCB ;
         UINT64                  _timerID ;
         UINT32                  _beatTime ;
         BOOLEAN                 _active ;
         BOOLEAN                 _locationActive ;
         UINT64                  _lastTimerTick ;

         UINT64                  _lastConsultTick ;

         UINT32                  _srcSessionNum ;
         ossRWMutex              _vecLatch ;
         std::vector<_clsDataSrcBaseSession*> _vecSrcSessions ;

         // notify queue
         ossQueue< clsLSNNtyInfo >  _ntyQue ;
         DPS_LSN_OFFSET             _ntyLastOffset ;
         DPS_LSN_OFFSET             _ntyProcessedOffset ;
         DPS_LSN_OFFSET             _ntyReplayOffset ;

         // sync control param
         UINT64                  _totalLogSize ;
         UINT64                  _sizethreshold[ CLS_SYNCCTRL_THRESHOLD_SIZE ] ;
         UINT32                  _timeThreshold[ CLS_SYNCCTRL_THRESHOLD_SIZE ] ;
         BOOLEAN                 _inSyncCtrl ;

         ossEvent                _faultEvent ;
         ossEvent                _syncEmptyEvent ;

         UINT32                  _syncwaitTimeout ;
         UINT32                  _shutdownWaitTimeout ;
         UINT32                  _fusingTimeout ;

         BOOLEAN                 _isAllNodeFatal ;
         ossEvent                _heartbeatEvent ;

         ossAtomic64             _lastLogMoveTick ;
         BOOLEAN                 _remoteLocationConsistency ;

         clsSyncNotifyQueue      _syncNotifyQue ;
   } ;

   typedef class _clsReplicateSet clsReplicateSet ;
   typedef _clsReplicateSet replCB ;
}

#endif

