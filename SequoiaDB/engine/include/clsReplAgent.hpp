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

   Source File Name = clsReplAgent.hpp

   Descriptive Name =

   When/how to use: this program may be used on binary and text-formatted
   versions of Replication component. This file contains structure for
   vote mechanism.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/01/2019  HGM Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef CLS_REPL_AGENT_HPP_
#define CLS_REPL_AGENT_HPP_

#include "core.hpp"
#include "oss.hpp"
#include "msg.hpp"
#include "ossMemPool.hpp"
#include "netRouteAgent.hpp"
#include "clsReplDef.hpp"
#include "dpsDef.hpp"
#include "clsVoteMachine.hpp"
#include "clsSyncManager.hpp"
#include "clsReelection.hpp"

namespace engine
{

   #define CLS_FORMART_STR_128   (128)

   /*
      _ICLSReplAgent define
    */
   class _ICLSReplAgent
   {
   public:
      _ICLSReplAgent( netRouteAgent *netAgent ) ;
      virtual ~_ICLSReplAgent() ;

   public:
      /// check status before launch vote
      virtual BOOLEAN checkVoteLaunch() = 0 ;
      /// get expect LSN from local
      virtual DPS_LSN getLocalExpectLSN() = 0 ;
      /// get current LSN from local
      virtual DPS_LSN getLocalCurrentLSN() = 0 ;
      /// get LSN window from local
      virtual void getLSNWindow( DPS_LSN &fileBeginLSN,
                                 DPS_LSN &memBeginLSN,
                                 DPS_LSN &endLSN,
                                 DPS_LSN &expectLSN ) = 0 ;
      /// indicate if local is OK
      virtual BOOLEAN isLocalOK() = 0 ;
      /// indicate if local is spare
      virtual BOOLEAN isLocalSpare() = 0 ;
      /// get vote weight
      virtual UINT8 getVoteWeight() = 0 ;
      /// get sharing break time
      virtual UINT32 getSharingBreakTime() = 0 ;
      /// get synchronize strategy
      virtual INT32 getSyncStrategy() = 0 ;
      /// get detectdisk
      virtual BOOLEAN getDetectDisk() = 0 ;
      /// handle case where local is not found in replica group
      virtual INT32 onLocalNotFoundInGroup() = 0 ;
      /// handle case before primary active
      virtual void beforePrimaryActive() = 0 ;
      /// handle case on primary active
      virtual void onPrimaryActive( const MsgRouteID &newPrimaryRID,
                                    const MsgRouteID &oldPrimaryRID ) = 0 ;
      /// handle case after primary active
      virtual void afterPrimaryActive( const MsgRouteID &newPrimaryRID,
                                       const MsgRouteID &oldPrimaryRID ) = 0 ;
      /// handle case before primary deactive
      virtual void beforePrimaryDeactive() = 0 ;
      /// handle case on primary deactive
      virtual void onPrimaryDeactive( const MsgRouteID &newPrimaryRID,
                                      const MsgRouteID &oldPrimaryRID ) = 0 ;
      /// handle case after primary deactive
      virtual void afterPrimaryDeactive( const MsgRouteID &newPrimaryRID,
                                         const MsgRouteID &oldPrimaryRID ) = 0 ;
      /// handle case when information of local group is expired
      virtual void onLocalGroupExpired() = 0 ;
      /// handle case when is notified that primary is changed
      /// before event
      virtual void beforeFoundNewPrimary() = 0 ;
      /// handle case when is notified that primary is changed
      /// after event
      virtual void afterFoundNewPrimary( const MsgRouteID &newPrimaryRID ) = 0 ;
      // process LSN reported by beat
      virtual void processBeatLSN( const MsgRouteID &remote,
                                   const DPS_LSN &lsn ) = 0 ;

   public:
      OSS_INLINE EDUID getMainEDUID()
      {
         return _mainEDUID ;
      }

      OSS_INLINE void setMainEDUID( EDUID eduID )
      {
         _mainEDUID = eduID ;
      }

      OSS_INLINE BOOLEAN isLocalAbnormal()
      {
         return !( isLocalOK() ) ;
      }

      OSS_INLINE BOOLEAN primaryIsMe()
      {
         return _vote.primaryIsMe() ;
      }

      OSS_INLINE void setLocalID( const MsgRouteID &id )
      {
         ossScopedRWLock lock( _getRuntimeLock(), EXCLUSIVE ) ;
         _info.local = id ;
      }

      OSS_INLINE UINT32 ailves()
      {
         ossScopedRWLock lock( _getRuntimeLock(), SHARED ) ;
         return _info.aliveSize() ;
      }

      OSS_INLINE UINT32 groupSize()
      {
         ossScopedRWLock lock( _getRuntimeLock(), SHARED ) ;
         return _info.groupSize() ;
      }

      OSS_INLINE void getBoth( UINT32 &nodeCnt, UINT32 &aliveCnt )
      {
         ossScopedRWLock lock( _getRuntimeLock(), SHARED ) ;
         nodeCnt = _info.groupSize() ;
         aliveCnt = _info.aliveSize() ;
      }

      // timeout: ms
      OSS_INLINE UINT32 getAlivesByTimeout( UINT32 timeout =
                                            CLS_NODE_KEEPALIVE_TIMEOUT )
      {
         ossScopedRWLock lock( _getRuntimeLock(), SHARED ) ;
         return _info.getAlivesByTimeout( timeout ) ;
      }

      OSS_INLINE BOOLEAN isAlive( const MsgRouteID &node )
      {
         ossScopedRWLock lock( _getRuntimeLock(), SHARED ) ;
         return ( _info.alives.find ( node.value ) != _info.alives.end() ) ;
      }

      OSS_INLINE BOOLEAN isSendNormal( UINT64 nodeID )
      {
         ossScopedRWLock lock( _getRuntimeLock(), SHARED ) ;
         if ( _info.local.value == nodeID )
         {
            return TRUE ;
         }
         return ( _info.getNodeSendFailedTimes( nodeID ) == 0 ) ;
      }

      OSS_INLINE MsgRouteID getPrimary()
      {
         ossScopedRWLock lock( _getRuntimeLock(), SHARED ) ;
         return _info.primary ;
      }

      INT32 aliveNode( const MsgRouteID &id ) ;

      BOOLEAN getPrimaryInfo( clsSharingStatus &primaryInfo ) ;

      INT32 reelect( CLS_REELECTION_LEVEL lvl,
                     UINT32 seconds,
                     pmdEDUCB *cb,
                     const MsgRouteID &destRID ) ;

      void reelectionDone() ;

      INT32 primaryCheck( pmdEDUCB *cb ) ;

      /// this func is used to support command "forceStepUp".
      INT32 stepUp( UINT32 seconds, pmdEDUCB *cb ) ;

      OSS_INLINE BOOLEAN isActive()
      {
         return _active ;
      }

      OSS_INLINE INT32 getStartShiftTime()
      {
         return _startShiftTime ;
      }

      OSS_INLINE void setStartShiftTime( INT32 startShiftTime )
      {
         _startShiftTime = startShiftTime ;
      }

   public:
      OSS_INLINE netRouteAgent *getNetAgent()
      {
         return _agent ;
      }

      OSS_INLINE clsGroupInfo *getGroupInfo()
      {
         return ( &_info ) ;
      }

      OSS_INLINE clsVoteMachine *getVoteMachine()
      {
         return &_vote ;
      }

      OSS_INLINE clsSyncManager *getSyncManager()
      {
         return &_sync ;
      }

   protected:
      INT32 _setGroupSet( const CLS_GROUP_VERSION &version,
                          NET_ROUTE_MAP &nodes,
                          UINT32 groupHashCode,
                          BOOLEAN updateRoute,
                          BOOLEAN &changeStatus ) ;
      BOOLEAN _isUDPHandle( NET_HANDLE handle ) ;
      INT32 _alive( const MsgRouteID &id, BOOLEAN fromUDP ) ;
      void  _checkBreak( const UINT32 &millisec ) ;
      void  _sharingBeat() ;
      INT32 _sendSharingBeat( clsSharingStatus &status, MsgClsBeat *message ) ;
      INT32 _handleMsg( NET_HANDLE handle, MsgHeader *msg ) ;
      INT32 _handleSharingBeat( NET_HANDLE handle, const MsgClsBeat *msg ) ;
      INT32 _handleSharingBeatRes( NET_HANDLE handle,
                                   const MsgClsBeatRes *msg ) ;
      void  _handleTimeout( UINT32 interval ) ;
      INT32 _handleEvent( pmdEDUEvent *event ) ;
      INT32 _handleStepDown() ;
      INT32 _handleStepUp( UINT32 seconds ) ;

      void _activate() ;
      void _deactivate() ;

      OSS_INLINE BOOLEAN _isMainEDUCB()
      {
         return ( _mainEDUID == pmdGetThreadEDUCB()->getID() ) ;
      }

      OSS_INLINE ossRWMutex *_getRuntimeLock()
      {
         // if it is main EDU, no need to acquire read lock
         return _isMainEDUCB() ? NULL : &( _info.mtx ) ;
      }

   protected:
      OSS_INLINE virtual UINT32 _getConfirmedStat() const
      {
         return 0 ;
      }

      OSS_INLINE virtual INT32 _getIndoubtErr() const
      {
         return SDB_OK ;
      }

      OSS_INLINE virtual BOOLEAN _isStop()
      {
         return FALSE ;
      }

      OSS_INLINE virtual BOOLEAN _isCatchup()
      {
         return FALSE ;
      }

      OSS_INLINE virtual BOOLEAN _isFTWhole()
      {
         return FALSE ;
      }

   protected:
      netRouteAgent *   _agent ;
      clsGroupInfo      _info ;
      clsVoteMachine    _vote ;
      clsSyncManager    _sync ;
      clsReelection     _reelection ;
      EDUID             _mainEDUID ;
      UINT32            _beatTime ;
      INT32             _startShiftTime ;
      BOOLEAN           _active ;
      BOOLEAN           _isAllNodeFatal ;
      ossEvent          _heartbeatEvent ;
   } ;

}

#endif // CLS_REPL_AGENT_HPP_
