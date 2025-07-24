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
#include "clsReelection.hpp"
#include <vector>

using namespace std ;

namespace engine
{
   class _netRouteAgent ;
   class _clsMgr ;
   class _clsDataSrcBaseSession ;
   class _pmdEDUCB ;

   #define CLS_SYNCCTRL_THRESHOLD_SIZE          (10)
   #define CLS_SYNC_DFT_TIMEOUT                 ( 3600 * OSS_ONE_SEC )

   /*
      _clsReplicateSet define
   */
   class _clsReplicateSet : public _pmdObjBase, public _dpsEventHandler
   {
      DECLARE_OBJ_MSG_MAP()

      public:
         _clsReplicateSet( _netRouteAgent *agent ) ;
         virtual ~_clsReplicateSet() ;

      public:
         OSS_INLINE BOOLEAN primaryIsMe()
         {
            return _vote.primaryIsMe() ;
         }

         OSS_INLINE clsBucket* getBucket ()
         {
            return &_replBucket ;
         }

         OSS_INLINE void setLocalID( const MsgRouteID &id )
         {
            _info.local = id ;
            /// _agent was set by clsMgr.
         }

         OSS_INLINE const UINT32 ailves()
         {
            UINT32 num = 0 ;
            _info.mtx.lock_r() ;
            num = _info.aliveSize() ;
            _info.mtx.release_r() ;
            return num ;
         }

         OSS_INLINE UINT32 groupSize ()
         {
            UINT32 num = 0 ;
            _info.mtx.lock_r () ;
            num = _info.groupSize() ;
            _info.mtx.release_r  () ;
            return num ;
         }

         OSS_INLINE void getBoth( UINT32 &nodeCnt,
                                  UINT32 &aliveCnt )
         {
            _info.mtx.lock_r() ;
            nodeCnt = _info.groupSize() ;
            aliveCnt = _info.aliveSize() ;
            _info.mtx.release_r() ;
            return ;
         }

         // timeout: ms
         OSS_INLINE UINT32 getAlivesByTimeout( UINT32 timeout =
                                               CLS_NODE_KEEPALIVE_TIMEOUT )
         {
            UINT32 num = 0 ;
            _info.mtx.lock_r () ;
            num = _info.getAlivesByTimeout( timeout ) ;
            _info.mtx.release_r  () ;
            return num ;
         }

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

         OSS_INLINE _clsSyncManager *syncMgr()
         {
            return &_sync ;
         }

         OSS_INLINE _clsVoteMachine* voteMachine()
         {
            return &_vote ;
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
            session.endLsn = offset ;
            session.eduCB = eduCB ;
            eduCB->getEvent().reset() ;

            if ( w > CLS_REPLSET_MAX_NODE_SIZE )
            {
               w = CLS_REPLSET_MAX_NODE_SIZE ;
            }

            return _sync.sync( session, w, timeout ) ;
         }

         OSS_INLINE UINT32 getNtySessionNum ()
         {
            return _srcSessionNum ;
         }

         OSS_INLINE BOOLEAN isInStepUp() const
         {
            return _vote.isInStepUp() ;
         }

         ossQueue< clsLSNNtyInfo >* getNtyQue() { return &_ntyQue ; }
         DPS_LSN_OFFSET getNtyLastOffset() const { return _ntyLastOffset ; }
         DPS_LSN_OFFSET getNtyProcessedOffset() const { return _ntyProcessedOffset ; }

         void notify2Session( UINT32 suLID, UINT32 clLID, dmsExtentID extLID,
                              const DPS_LSN_OFFSET &offset ) ;

         virtual void onWriteLog( DPS_LSN_OFFSET offset ) ;

         virtual void onPrepareLog( UINT32 csLID, UINT32 clLID,
                                    INT32 extLID, DPS_LSN_OFFSET offset ) ;

         virtual INT32 canAssignLogPage( UINT32 reqLen, pmdEDUCB *cb ) ;

         virtual INT32 onCompleteOpr( _pmdEDUCB *cb, INT32 w )
         {
            return sync( cb->getEndLsn(), cb, w ) ;
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

         void getGroupInfo( _MsgRouteID &primary,
                            vector<_netRouteNode > &group ) ;

         MsgRouteID     getPrimary () ;
         BOOLEAN        isSendNormal( UINT64 nodeID ) ;

         ossEvent*      getFaultEvent() ;
         ossEvent*      getSyncEmptyEvent() ;

         INT64 netIn() ;
         INT64 netOut() ;
         void resetMon() ;

         INT32 reelect( CLS_REELECTION_LEVEL lvl,
                        UINT32 seconds,
                        pmdEDUCB *cb ) ;

         void reelectionDone() ;

         /// this func is used to support command "forceStepUp".
         INT32 stepUp( UINT32 seconds,
                       pmdEDUCB *cb ) ;

         INT32 primaryCheck( pmdEDUCB *cb ) ;

         INT32 aliveNode( const MsgRouteID &id ) ;

         UINT64   getLastConsultTick() const ;
         void     setLastConsultTick( UINT64 tick ) ;

      private:
         INT32 _setGroupSet( const CLS_GROUP_VERSION &version,
                             map<UINT64, _netRouteNode> &nodes,
                             BOOLEAN &changeStatus ) ;

         INT32 _alive( const _MsgRouteID &id ) ;

         INT32 _handleSharingBeat( const _MsgClsBeat *msg ) ;

         INT32 _handleSharingBeatRes( const _MsgClsBeatRes *msg ) ;

         INT32 _handleGroupRes( const MsgCatGroupRes *msg ) ;

         void _sharingBeat() ;

         void _checkBreak( const UINT32 &millisec ) ;

         UINT32 _getThresholdTime( UINT64 diffSize ) ;

         INT32 _handleStepDown() ;

         INT32 _handleStepUp( UINT32 seconds ) ;

      private:
         _netRouteAgent          *_agent ;
         _clsGroupInfo           _info ;
         _clsVoteMachine         _vote ;
         _dpsLogWrapper          *_logger ;
         _clsSyncManager         _sync ;
         _clsCatalogCaller       _cata ;
         _clsReelection          _reelection ;
         clsBucket               _replBucket ;
         _clsMgr                 *_clsCB ;
         UINT64                  _timerID ;
         UINT32                  _beatTime ;
         BOOLEAN                 _active ;
         UINT64                  _lastTimerTick ;

         UINT64                  _lastConsultTick ;

         UINT32                  _srcSessionNum ;
         ossRWMutex              _vecLatch ;
         std::vector<_clsDataSrcBaseSession*> _vecSrcSessions ;

         // notify queue
         ossQueue< clsLSNNtyInfo >  _ntyQue ;
         DPS_LSN_OFFSET             _ntyLastOffset ;
         DPS_LSN_OFFSET             _ntyProcessedOffset ;

         // sync control param
         UINT64                  _totalLogSize ;
         UINT64                  _sizethreshold[ CLS_SYNCCTRL_THRESHOLD_SIZE ] ;
         UINT32                  _timeThreshold[ CLS_SYNCCTRL_THRESHOLD_SIZE ] ;
         BOOLEAN                 _inSyncCtrl ;

         ossEvent                _faultEvent ;
         ossEvent                _syncEmptyEvent ;
   } ;

   typedef class _clsReplicateSet clsReplicateSet ;
   typedef _clsReplicateSet replCB ;
}

#endif

