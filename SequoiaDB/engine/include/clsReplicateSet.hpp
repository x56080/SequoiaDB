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
#include "clsReplAgent.hpp"
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
   class _pmdFTMgr ;

   #define CLS_SYNCCTRL_THRESHOLD_SIZE          (10)
   #define CLS_SYNC_DFT_TIMEOUT                 ( 600 * OSS_ONE_SEC )

   #define CLS_SYNCWAIT_FIX_TIME_SLICE          ( 10 * OSS_ONE_SEC )

   /*
      _clsReplicateSet define
   */
   class _clsReplicateSet : public _pmdObjBase,
                            public _dpsEventHandler,
                            public _ICluster,
                            public ICLSReplAgent
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

         OSS_INLINE clsBucket* getBucket ()
         {
            return &_replBucket ;
         }

         OSS_INLINE void getDetailInfo( UINT32 &nodeCnt, UINT32 &aliveCnt,
                                        UINT32 &falutCnt, UINT32 &ssCnt,
                                        INT32 &indoubtErr,
                                        UINT16 &indoubtNodeID )
         {
            map<UINT64, _clsSharingStatus *>::iterator it ;
            _clsSharingStatus *pStatus = NULL ;

            nodeCnt = 0 ;
            aliveCnt = 0 ;
            falutCnt = 0 ;
            ssCnt = 0 ;
            indoubtErr = SDB_OK ;

            ossScopedRWLock lock( &_info.mtx, SHARED ) ;

            nodeCnt = _info.groupSize() ;
            aliveCnt = _info.aliveSize() ;

            it = _info.alives.begin() ;
            while( it != _info.alives.end() )
            {
               pStatus = it->second ;
               ++it ;

               if ( CLS_NODE_STOP == pStatus->beat.nodeRunStat )
               {
                  --aliveCnt ;
               }
               else if ( 0 != pStatus->beat.ftConfirmStat )
               {
                  ++falutCnt ;
                  if ( SDB_OK == indoubtErr )
                  {
                     indoubtErr = pStatus->beat.indoubtErr ;
                     indoubtNodeID = pStatus->beat.identity.columns.nodeID ;
                  }
               }
               else if ( CLS_NODE_RUNNING != pStatus->beat.nodeRunStat )
               {
                  ++ssCnt ;
               }
            }
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
            INT32 rc = SDB_OK ;
            UINT32 timeout = 0 ;
            UINT32 onceTimeout = 0 ;
            BOOLEAN replCheckRC = SDB_OK ;

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

         virtual void  onMoveLog( DPS_LSN_OFFSET moveToOffset,
                                  DPS_LSN_VER moveToVersion,
                                  DPS_LSN_OFFSET expectOffset,
                                  DPS_LSN_VER expectVersion,
                                  DPS_MOMENT moment,
                                  INT32 errcode )
         {
            return ;
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

         ossEvent*      getFaultEvent() ;
         ossEvent*      getSyncEmptyEvent() ;

         INT64 netIn() ;
         INT64 netOut() ;

         INT32 replSizeCheck( INT16 w, INT16 &finalW, _pmdEDUCB *cb,
                              BOOLEAN isAfterData = FALSE ) ;

         UINT64   getLastConsultTick() const ;
         void     setLastConsultTick( UINT64 tick ) ;

      private:
         INT32 _handleGroupRes( const MsgCatGroupRes *msg ) ;

         UINT32 _getThresholdTime( UINT64 diffSize ) ;

      public:
         // vote agent implements
         virtual BOOLEAN checkVoteLaunch() ;
         virtual DPS_LSN getLocalExpectLSN() ;
         virtual DPS_LSN getLocalCurrentLSN() ;
         virtual void getLSNWindow( DPS_LSN &fileBeginLSN,
                                    DPS_LSN &memBeginLSN,
                                    DPS_LSN &endLSN,
                                    DPS_LSN &expectLSN ) ;
         virtual BOOLEAN isLocalOK() ;
         virtual BOOLEAN isLocalSpare() ;
         virtual UINT8 getVoteWeight() ;
         virtual UINT32 getSharingBreakTime() ;
         virtual BOOLEAN getDetectDisk() ;
         virtual INT32 getSyncStrategy() ;
         virtual INT32 onLocalNotFoundInGroup() ;
         virtual void beforePrimaryActive() ;
         virtual void onPrimaryActive( const MsgRouteID &newPrimaryRID,
                                       const MsgRouteID &oldPrimaryRID ) ;
         virtual void afterPrimaryActive( const MsgRouteID &newPrimaryRID,
                                          const MsgRouteID &oldPrimaryRID ) ;
         virtual void beforePrimaryDeactive() ;
         virtual void onPrimaryDeactive( const MsgRouteID &newPrimaryRID,
                                         const MsgRouteID &oldPrimaryRID ) ;
         virtual void afterPrimaryDeactive( const MsgRouteID &newPrimaryRID,
                                            const MsgRouteID &oldPrimaryRID ) ;
         virtual void onLocalGroupExpired() ;
         virtual void beforeFoundNewPrimary() ;
         virtual void afterFoundNewPrimary( const MsgRouteID &newPrimaryRID ) ;
         virtual void processBeatLSN( const MsgRouteID &remote,
                                      const DPS_LSN &lsn ) ;

      protected:
         OSS_INLINE virtual UINT32 _getConfirmedStat() const
         {
            return _pFTMgr->getConfirmedStat() ;
         }

         OSS_INLINE virtual INT32 _getIndoubtErr() const
         {
            return _pFTMgr->getIndoubtErr() ;
         }

         OSS_INLINE virtual BOOLEAN _isStop()
         {
            return _pFTMgr->isStop() ;
         }

         OSS_INLINE virtual BOOLEAN _isCatchup()
         {
            return _pFTMgr->isCatchup() ;
         }

         OSS_INLINE virtual BOOLEAN _isFTWhole()
         {
            return ( ( FT_LEVEL_WHOLE == _pFTMgr->getFTLevel() ) ?
                     TRUE : FALSE ) ;
         }

      private:
         _dpsLogWrapper          *_logger ;
         _pmdFTMgr               *_pFTMgr ;
         _clsCatalogCaller       _cata ;
         clsBucket               _replBucket ;
         _clsMgr                 *_clsCB ;
         UINT64                  _timerID ;
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

         UINT32                  _syncwaitTimeout ;
         UINT32                  _shutdownWaitTimeout ;
         UINT32                  _fusingTimeout ;
   } ;

   typedef class _clsReplicateSet clsReplicateSet ;
   typedef _clsReplicateSet replCB ;
}

#endif

