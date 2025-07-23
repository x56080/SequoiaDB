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

   Source File Name = clsReplAgent.cpp

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
#include "clsReplAgent.hpp"
#include "pdTrace.hpp"
#include "clsTrace.hpp"
#include "clsUtil.hpp"
#include "msgMessageFormat.hpp"
#include "pmd.hpp"

namespace engine
{

   #define CLS_FORMART_STR_128                  (128)

   /*
      ICLSReplAgent implement
    */
   _ICLSReplAgent::_ICLSReplAgent( netRouteAgent *netAgent )
   : _agent( netAgent ),
     _vote( this ),
     _sync( this ),
     _reelection( this ),
     _mainEDUID( PMD_INVALID_EDUID ),
     _beatTime( 0 ),
     _startShiftTime( 0 ),
     _active( FALSE ),
     _isAllNodeFatal( FALSE )
   {
   }

   _ICLSReplAgent::~_ICLSReplAgent()
   {
   }

   // PD_TRACE_DECLARE_FUNCTION (SDB__ICLSREPLAGENT_ALIVENODE, "_ICLSReplAgent::aliveNode" )
   INT32 _ICLSReplAgent::aliveNode( const MsgRouteID &id )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__ICLSREPLAGENT_ALIVENODE ) ;

      /// wait for 100 milliseconds
      rc = _info.mtx.lock_r( 100 ) ;
      if ( SDB_OK == rc )
      {
         CLS_ALIVE_MAP::iterator itr = _info.alives.find( id.value ) ;
         if ( itr != _info.alives.end() )
         {
            itr->second->timeout = 0 ;
            itr->second->breakTime = 0 ;
            itr->second->deadtime = 0 ;
            itr->second->sendFailedTimes = 0 ;
         }
         else
         {
            rc = SDB_CLS_NODE_BSFAULT ;
         }
         _info.mtx.release_r() ;
      }

      PD_TRACE_EXITRC( SDB__ICLSREPLAGENT_ALIVENODE, rc ) ;

      return rc ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__ICLSREPLAGENT_GETPRIMARYINFO, "_ICLSReplAgent::getPrimaryInfo" )
   BOOLEAN _ICLSReplAgent::getPrimaryInfo( clsSharingStatus &primaryInfo )
   {
      BOOLEAN isOk = FALSE ;

      PD_TRACE_ENTRY( SDB__ICLSREPLAGENT_GETPRIMARYINFO ) ;

      ossScopedRWLock lock( &_info.mtx, SHARED ) ;

      CLS_NODE_MAP::iterator itr = _info.info.find( _info.primary.value ) ;
      if ( itr != _info.info.end() )
      {
         primaryInfo = itr->second ;
         isOk = TRUE ;
      }

      PD_TRACE_EXIT( SDB__ICLSREPLAGENT_GETPRIMARYINFO ) ;

      return isOk ;
   }

   // PD_TRACE_DECLARE_FUNCTION (SDB__ICLSREPLAGENT_REELECT, "_ICLSReplAgent::reelect" )
   INT32 _ICLSReplAgent::reelect( CLS_REELECTION_LEVEL lvl,
                                  UINT32 seconds,
                                  pmdEDUCB *cb,
                                  UINT16 destID )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__ICLSREPLAGENT_REELECT ) ;

      if ( 1 == groupSize() )
      {
         goto done ;
      }

      rc = _reelection.run( lvl, seconds, cb, destID ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "Failed to reelect, rc: %d", rc ) ;
         goto error ;
      }

   done:
      PD_TRACE_EXITRC( SDB__ICLSREPLAGENT_REELECT, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   void _ICLSReplAgent::reelectionDone()
   {
      _vote.setShadowWeight( CLS_ELECTION_WEIGHT_USR_MIN ) ;
      _reelection.signal() ;
   }

   // PD_TRACE_DECLARE_FUNCTION (SDB__ICLSREPLAGENT_PRIMARYCHECK, "_ICLSReplAgent::primaryCheck" )
   INT32 _ICLSReplAgent::primaryCheck( pmdEDUCB *cb )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__ICLSREPLAGENT_PRIMARYCHECK ) ;

      rc = _reelection.wait( cb ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to wait:%d", rc ) ;
         goto error ;
      }
      else if ( !primaryIsMe() )
      {
         rc = SDB_CLS_NOT_PRIMARY ;
         goto error ;
      }
      else
      {
         /// do nothing.
      }

   done:
      PD_TRACE_EXITRC( SDB__ICLSREPLAGENT_PRIMARYCHECK, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION (SDB__ICLSREPLAGENT__STEPUP, "_ICLSReplAgent::stepUp" )
   INT32 _ICLSReplAgent::stepUp( UINT32 seconds, pmdEDUCB *cb )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__ICLSREPLAGENT__STEPUP ) ;

      DPS_LSN lsn ;
      pmdEDUMgr *eduMgr = pmdGetKRCB()->getEDUMgr() ;

      if ( MSG_INVALID_ROUTEID != getPrimary().value )
      {
         PD_LOG( PDERROR, "can not step up when primary node"
                 " exists" ) ;
         rc = SDB_CLS_CAN_NOT_STEP_UP ;
         goto error ;
      }
      else if ( !isActive() )
      {
         rc = SDB_CLS_NODE_INFO_EXPIRED ;
         PD_LOG( PDERROR, "can not step up before local's node download "
                 "group info" ) ;
         goto error ;
      }

      lsn = getLocalExpectLSN() ;
      if ( _sync.atLeastOne( lsn.offset ) )
      {
         PD_LOG( PDERROR, "can not step up when other nodes' lsn"
                 " bigger than local's" ) ;
         rc = SDB_CLS_CAN_NOT_STEP_UP ;
         goto error ;
      }

      rc = eduMgr->postEDUPost( getMainEDUID(),
                                PMD_EDU_EVENT_STEP_UP,
                                PMD_EDU_MEM_NONE,
                                NULL, seconds ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to post event to repl cb, rc: %d", rc ) ;
         goto error ;
      }

   done:
      PD_TRACE_EXITRC( SDB__ICLSREPLAGENT__STEPUP, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__ICLSREPLAGENT__SETGPSET, "_ICLSReplAgent::_setGroupSet" )
   INT32 _ICLSReplAgent::_setGroupSet( const CLS_GROUP_VERSION &version,
                                       NET_ROUTE_MAP &nodes,
                                       UINT32 groupHashCode,
                                       BOOLEAN updateRoute,
                                       BOOLEAN &changeStatus )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__ICLSREPLAGENT__SETGPSET ) ;

      BOOLEAN hasLocal = FALSE ;
      NET_ROUTE_MAP::iterator itr ;
      CLS_NODE_MAP::iterator itr2 ;
      changeStatus = FALSE ;

      if ( version <= _info.version )
      {
         rc = SDB_REPL_REMOTE_G_V_EXPIRED ;
         goto error ;
      }

      if ( CLS_REPLSET_MAX_NODE_SIZE < nodes.size() )
      {
         rc = SDB_CLS_INVALID_GROUP_NUM ;
         PD_LOG( PDWARNING, "invalid group size : %d",
                 nodes.size() ) ;
         goto error ;
      }

      _info.version = version ;

      /// update new nodes, include the node with
      /// same id but different address
      if ( isLocalSpare() )
      {
         hasLocal = TRUE ;
         nodes.clear() ;
      }

      itr = nodes.begin() ;
      for ( ; itr != nodes.end(); itr++ )
      {
         if ( itr->first == _info.local.value )
         {
            hasLocal = TRUE ;
            continue ;
         }
         else if ( !itr->second._isActive )
         {
            if ( getStartShiftTime() < 0 )
            {
               /// when has overed the start shift time, need ignore
               /// the nodes there are not actived
               continue ;
            }
            itr->second._isActive = TRUE ;
            changeStatus = TRUE ;
         }
         if ( updateRoute )
         {
            if ( SDB_OK == _agent->updateRoute( itr->second._id,
                                                itr->second ) )
            {
               _info.mtx.lock_w() ;
               _clsGroupBeat &beat = (_info.info[itr->first]).beat ;
               _info.mtx.release_w() ;
               beat.identity = itr->second._id ;
               beat.beatID = 0 ;
               /// we alive the changed node here. if it is unnormal,
               /// break it out later.
               _alive( itr->second._id, FALSE ) ;
               PD_LOG( PDEVENT, "add node [%s:%s]",
                       itr->second._host, itr->second._service[0].c_str() ) ;
            }
         }
         else
         {
            BOOLEAN newAdded = FALSE ;
            _info.mtx.lock_w() ;
            if ( _info.info.end() == _info.info.find( itr->first ) )
            {
               clsGroupBeat &beat = ( _info.info[ itr->first ] ).beat ;
               beat.identity = itr->second._id ;
               beat.beatID = 0 ;
               newAdded = TRUE ;
            }
            _info.mtx.release_w() ;

            if ( newAdded )
            {
               /// we alive the changed node here. if it is unnormal,
               /// break it out later.
               _alive( itr->second._id, FALSE ) ;
               PD_LOG( PDEVENT, "add node [%s:%s]",
                       itr->second._host, itr->second._service[0].c_str() ) ;
            }
         }
      } // for ( ; itr != nodes.end(); itr++ )

      if ( !hasLocal )
      {
         PD_LOG( PDERROR, "local node is not in the cluster!" ) ;
         rc = onLocalNotFoundInGroup() ;
         goto done ;
      }

      /// remove deleted nodes
      itr2 = _info.info.begin() ;
      for ( ; itr2 != _info.info.end(); )
      {
         itr = nodes.find( itr2->first ) ;
         if ( nodes.end() == itr || FALSE == itr->second._isActive )
         {
            /// if primary is deleted, set primary invalid
            if ( itr2->first == _info.primary.value )
            {
               _info.primary.value = 0 ;
            }
            MsgRouteID tmp ;
            tmp.value = itr2->first ;
            PD_LOG( PDEVENT, "erase node[%d,%d]",
                    tmp.columns.groupID, tmp.columns.nodeID ) ;
            _info.mtx.lock_w() ;
            _info.alives.erase( itr2->first ) ;
            _info.info.erase( itr2++ ) ;
            _info.mtx.release_w() ;
         }
         else
         {
            ++itr2 ;
         }
      } // for ( ; itr2 != _info.info.end(); itr2++ )

      _sync.updateNotifyList( TRUE ) ;

   done:
      if ( SDB_OK == rc || SDB_REPL_REMOTE_G_V_EXPIRED == rc )
      {
         /// set hash code
         _info.setHashCode( groupHashCode ) ;
      }
      PD_TRACE_EXITRC( SDB__ICLSREPLAGENT__SETGPSET, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   BOOLEAN _ICLSReplAgent::_isUDPHandle( NET_HANDLE handle )
   {
      return ( NET_EVENT_HANDLER_UDP ==
                           _agent->getFrame()->getEventHandleType( handle ) ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION (SDB__ICLSREPLAGENT__ALIVE, "_ICLSReplAgent::_alive" )
   INT32 _ICLSReplAgent::_alive( const MsgRouteID &id, BOOLEAN fromUDP )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__ICLSREPLAGENT__ALIVE ) ;

      CLS_NODE_MAP::iterator itr = _info.info.find( id.value ) ;
      if ( _info.info.end() == itr )
      {
         rc = SDB_REPL_INVALID_GROUP_MEMBER ;
         goto error ;
      }
      if ( _info.alives.end() == _info.alives.find( itr->first ) )
      {
         _clsSharingStatus &status = itr->second ;
         _info.mtx.lock_w() ;
         _info.alives.insert( make_pair( itr->first, &status ) ) ;
         _sync.updateNodeStatus( status.beat.identity, TRUE ) ;
         _info.mtx.release_w() ;

         PD_LOG( PDEVENT, "vote: [node:%d] aliving from %s",
                 status.beat.identity.columns.nodeID,
                 ( CLS_NODE_STOP == status.beat.nodeRunStat ?
                   "shutdown" : "break" ) ) ;
      }
      itr->second.timeout = 0 ;
      itr->second.breakTime = 0 ;
      itr->second.deadtime = 0 ;
      itr->second.sendFailedTimes = 0 ;

      if ( fromUDP )
      {
         itr->second.setUDPSupported() ;
      }

   done:
      PD_TRACE_EXITRC( SDB__ICLSREPLAGENT__ALIVE, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__ICLSREPLAGENT__CHKBRK, "_ICLSReplAgent::_checkBreak" )
   void _ICLSReplAgent::_checkBreak( const UINT32 &millisec )
   {
      /// avoid the use of w lock. only find item need to be
      /// erase, we lock w. here we think that no need to lock
      /// w when change value
      PD_TRACE_ENTRY( SDB__ICLSREPLAGENT__CHKBRK ) ;

      BOOLEAN needErase = FALSE ;
      CLS_ALIVE_MAP::iterator itr ;
      CLS_NODE_MAP::iterator itrInfo ;
      _clsSharingStatus *pStatus = NULL ;
      BOOLEAN isAllNodeFatal = TRUE ;

      for ( itr = _info.alives.begin() ; itr != _info.alives.end() ; itr++ )
      {
         pStatus = itr->second ;
         pStatus->timeout += millisec ;

         if ( isAllNodeFatal &&
              !PMD_FT_IS_FATAL_FAULT( pStatus->beat.ftConfirmStat ) )
         {
            isAllNodeFatal = FALSE ;
         }

         if ( getSharingBreakTime() <= pStatus->timeout )
         {
            needErase = TRUE ;
         }
      }

      /// update _isAllNodeFatal
      _isAllNodeFatal = isAllNodeFatal ;

      // increase break node's break time
      for ( itrInfo = _info.info.begin() ; itrInfo != _info.info.end() ;
            ++itrInfo )
      {
         if ( _info.alives.find( itrInfo->first ) != _info.alives.end() )
         {
            continue ;
         }
         itrInfo->second.breakTime += millisec ;
      }

      if ( !needErase )
      {
         goto done ;
      }

      _info.mtx.lock_w() ;
      itr = _info.alives.begin() ;
      for ( ; itr != _info.alives.end(); )
      {
         pStatus = itr->second ;
         if ( getSharingBreakTime() <= pStatus->timeout )
         {
            if ( itr->first == _info.primary.value )
            {
               PD_LOG( PDERROR, "vote: primary [node:%d] alive break(%s)",
                       _info.primary.columns.nodeID,
                       ( CLS_NODE_STOP == pStatus->beat.nodeRunStat ?
                         "shutdown" : "unknown" ) ) ;
               _info.primary.value = MSG_INVALID_ROUTEID ;
            }
            else
            {
               PD_LOG( PDERROR, "vote: [node:%d] alive break(%s)",
                       pStatus->beat.identity.columns.nodeID,
                       ( CLS_NODE_STOP == pStatus->beat.nodeRunStat ?
                         "shutdown" : "unknown" ) ) ;
            }
            pStatus->beat.beatID = CLS_BEATID_INVALID ;
            pStatus->beat.serviceStatus = SERVICE_UNKNOWN ;
            pStatus->beat.ftConfirmStat = 0 ;
            pStatus->beat.indoubtErr = SDB_OK ;

            // alive break, reset UDP support
            pStatus->resetUDP() ;

            _sync.updateNodeStatus( pStatus->beat.identity, FALSE ) ;

            _info.alives.erase( itr++ ) ;
         }
         else
         {
            ++itr ;
         }
      }
      _info.mtx.release_w() ;
      /// cutting when down to secondary is in _clsVSPrimary.
      if ( _vote.primaryIsMe() )
      {
         _sync.cut( _info.alives.size(), _isFTWhole() ) ;
      }

   done:
      PD_TRACE_EXIT( SDB__ICLSREPLAGENT__CHKBRK ) ;
   }

   // The function is called by cls mgr thread with the same change thread,
   // so don't need to use lock
   // PD_TRACE_DECLARE_FUNCTION ( SDB__ICLSREPLAGENT__SHRBEAT, "_ICLSReplAgent::_sharingBeat" )
   void _ICLSReplAgent::_sharingBeat()
   {
      PD_TRACE_ENTRY( SDB__ICLSREPLAGENT__SHRBEAT ) ;

      if ( _info.info.empty() )
      {
         goto done ;
      }
      else
      {
         INT32 rc = SDB_OK ;
         DPS_LSN fBegin ;
         DPS_LSN mBegin ;
         DPS_LSN end ;
         DPS_LSN expectLSN ;
         getLSNWindow( fBegin, mBegin, end, expectLSN ) ;
         MsgClsBeat msg ;
         msg.beat.identity = _info.local ;
         msg.beat.endLsn = expectLSN ;
         msg.beat.version = _info.version ;
         *(UINT32*)msg.beat.hashCode = _info.getHashCode() ;
         msg.beat.role = _vote.primaryIsMe() ?
                         CLS_GROUP_ROLE_PRIMARY : CLS_GROUP_ROLE_SECONDARY ;
         msg.beat.beatID = _info.nextBeatID() ;
         msg.header.requestID = msg.beat.beatID ;
         msg.beat.serviceStatus = isLocalOK() ?
                                  SERVICE_NORMAL : SERVICE_ABNORMAL ;
         UINT8 weight = getVoteWeight() ;
         UINT8 shadowWeight = _vote.getShadowWeight() ;
         msg.beat.weight = CLS_GET_WEIGHT( weight, shadowWeight ) ;
         msg.beat.ftConfirmStat = _getConfirmedStat() ;
         msg.beat.indoubtErr = _getIndoubtErr() ;
         if ( _isStop() )
         {
            msg.beat.nodeRunStat = (UINT8)CLS_NODE_STOP ;
         }
         else if ( _isCatchup() )
         {
            msg.beat.nodeRunStat = (UINT8)CLS_NODE_CATCHUP ;
         }

         CLS_NODE_MAP::iterator itr = _info.info.begin() ;
         for ( ; itr != _info.info.end(); itr++ )
         {
            _clsSharingStatus &status = itr->second ;

            /// decrease dead time for heartbeat
            if ( status.deadtime >= getSharingBreakTime() &&
                 status.deadtime >= _beatTime )
            {
               status.deadtime -= _beatTime ;
               continue ;
            }
            msg.beat.syncStatus = clsSyncWindow( status.beat.endLsn,
                                                 fBegin, mBegin, expectLSN ) ;

            rc = _sendSharingBeat( status, &msg ) ;
            if ( SDB_OK == rc )
            {
               status.sendFailedTimes = 0 ;
            }
            else
            {
               INT32 sysErr = SOCKET_GETLASTERROR ;

               if ( sysErr == CLS_CONNREFUSED )
               {
                  ++( status.sendFailedTimes ) ;
               }

               /// if send heartbeat msg failed, and the node is not in active,
               /// nead to reset dead time to decrease heartbeat msg
               if ( _info.alives.find( itr->first ) == _info.alives.end() )
               {
                  UINT32 resetTimeout = 0 ;
                  status.deadtime = getSharingBreakTime() - 1 ;
                  if ( sysErr == CLS_CONNREFUSED )
                  {
                     resetTimeout = 1800 * OSS_ONE_SEC ;
                  }
                  else
                  {
                     resetTimeout = 120 * OSS_ONE_SEC ;
                  }
                  status.deadtime += resetTimeout ;

                  PD_LOG( PDEVENT, "Reset node[%d] sharing-beat time to %u(sec)",
                          status.beat.identity.columns.nodeID,
                          resetTimeout / OSS_ONE_SEC ) ;
               }
               /// When the node is alive, but run stat is CLS_NODE_STOP, and
               /// send heart-beat failed, should set timeout
               else if ( CLS_NODE_STOP == status.beat.nodeRunStat )
               {
                  status.timeout = getSharingBreakTime() ;
               }
            }
         }
      }

   done:
      _heartbeatEvent.signalAll() ;
      PD_TRACE_EXIT( SDB__ICLSREPLAGENT__SHRBEAT ) ;
      return ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__ICLSREPLAGENT__SENDSHARINGBEAT, "_ICLSReplAgent::_sendSharingBeat" )
   INT32 _ICLSReplAgent::_sendSharingBeat( clsSharingStatus &status,
                                           MsgClsBeat *message )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__ICLSREPLAGENT__SENDSHARINGBEAT ) ;

      /// use UDP to send message, but we need to test whether remote
      /// supports UDP for backwards compatibility
      if ( status.isUDPSupported() )
      {
         // UDP is marked supported
         rc = _agent->syncSendUDP( status.beat.identity, message ) ;
      }
      else if ( status.isUDPUnavailable() )
      {
         // UDP is marked unavailable, use TCP directly
         rc = _agent->syncSend( status.beat.identity, message ) ;
      }
      else
      {
         // UDP status is unknown, test UDP first, and then send with TCP
         INT32 tmpRC = _agent->syncSendUDP( status.beat.identity, message ) ;
         if ( SDB_OK != tmpRC )
         {
            status.setUDPUnavailable() ;
         }
         else
         {
            status.increaseUDPTest() ;
         }

         rc = _agent->syncSend( status.beat.identity, message ) ;
      }

      PD_TRACE_EXITRC( SDB__ICLSREPLAGENT__SENDSHARINGBEAT, rc ) ;

      return rc ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__ICLSREPLAGENT__HNDMSG, "_ICLSReplAgent::_handleMsg" )
   INT32 _ICLSReplAgent::_handleMsg( NET_HANDLE handle, MsgHeader *msg )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__ICLSREPLAGENT__HNDMSG ) ;

      SDB_ASSERT( NULL != msg, "message is invalid" ) ;

      if ( !_active )
      {
         rc = SDB_REPL_GROUP_NOT_ACTIVE ;
         goto error ;
      }

      switch ( msg->opCode )
      {
         case MSG_CLS_BEAT :
         {
            rc = _handleSharingBeat( handle, (const MsgClsBeat *)msg ) ;
            break ;
         }
         case MSG_CLS_BEAT_RES :
         {
            rc = _handleSharingBeatRes( handle, (const MsgClsBeatRes *)msg ) ;
            break ;
         }
         case MSG_CLS_BALLOT :
         {
            rc = _vote.handleInput( msg ) ;
            break ;
         }
         case MSG_CLS_BALLOT_RES :
         {
            rc = _vote.handleInput( msg ) ;
            break ;
         }
         default :
         {
            PD_LOG( PDWARNING, "unknown msg: %s", msg2String( msg ).c_str() ) ;
            rc = SDB_CLS_UNKNOW_MSG ;
            break ;
         }
      }

   done:
      PD_TRACE_EXITRC( SDB__ICLSREPLAGENT__HNDMSG, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__ICLSREPLAGENT__HNDSHRBEAT, "_ICLSReplAgent::_handleSharingBeat" )
   INT32 _ICLSReplAgent::_handleSharingBeat( NET_HANDLE handle,
                                             const MsgClsBeat *msg )
   {

      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__ICLSREPLAGENT__HNDSHRBEAT ) ;

      SDB_ASSERT( NULL != msg, "msg should not be NULL" ) ;

      const clsGroupBeat &beat = msg->beat ;
      CLS_NODE_MAP::iterator itr = _info.info.find( beat.identity.value ) ;
      if ( *(UINT32*)beat.hashCode != _info.getHashCode() ||
          ( _info.info.end() == itr && beat.version <= _info.version ) )
      {
         PD_LOG( PDINFO, "Beat hashCode[%u] is not the same with self[%u] or "
                 "node[%s] is not found in group information",
                 *(UINT32*)beat.hashCode, _info.getHashCode(),
                 routeID2String( beat.identity ).c_str() ) ;
         rc = SDB_REPL_INVALID_GROUP_MEMBER ;
         goto error ;
      }

      if ( beat.version > _info.version )
      {
         rc = SDB_REPL_LOCAL_G_V_EXPIRED ;
         onLocalGroupExpired() ;
      }
      else if ( itr != _info.info.end() )
      {
         _clsSharingStatus &statusItem = itr->second ;

         /// FT confirm stat changed
         if ( statusItem.beat.ftConfirmStat != beat.getFTConfirmStat() )
         {
            CHAR oldStatStr[ CLS_FORMART_STR_128 + 1 ] = { 0 } ;
            CHAR newStatStr[ CLS_FORMART_STR_128 + 1 ] = { 0 } ;

            utilFTMaskToStr( statusItem.beat.ftConfirmStat,
                             oldStatStr, CLS_FORMART_STR_128 ) ;
            utilFTMaskToStr( beat.getFTConfirmStat(),
                             newStatStr, CLS_FORMART_STR_128 ) ;
            PD_LOG( PDEVENT, "Node[%d]'s fault-tolerance confirm stat "
                    "changed: 0x%08x(%s) => 0x%08x(%s), indoubt error: %d",
                    beat.identity.columns.nodeID,
                    statusItem.beat.ftConfirmStat,
                    oldStatStr,
                    beat.getFTConfirmStat(),
                    newStatStr,
                    beat.getIndoubtErr() ) ;
         }
         /// Node start/stop changed
         if ( statusItem.beat.nodeRunStat != beat.nodeRunStat )
         {
            PD_LOG( PDEVENT, "Node[%d]'s run stat changed: %d(%s) => %d(%s)",
                    beat.identity.columns.nodeID,
                    statusItem.beat.nodeRunStat,
                    clsNodeRunStat2String( statusItem.beat.nodeRunStat ),
                    beat.nodeRunStat,
                    clsNodeRunStat2String( beat.nodeRunStat ) ) ;
         }

         statusItem.beat = beat ;

         if ( CLS_GROUP_ROLE_PRIMARY == beat.role )
         {
            setStartShiftTime( -1 ) ; // have primary node

            if ( _vote.primaryIsMe() )
            {
               DPS_LSN lsn  = getLocalExpectLSN() ;
               if ( 0 >= lsn.compare( beat.endLsn ) )
               {
                  _info.mtx.lock_w() ;
                  _info.primary = beat.identity ;
                  _info.mtx.release_w() ;
                  _vote.force( CLS_ELECTION_STATUS_SILENCE ) ;
                  PD_LOG( PDEVENT, "vote:remote lsn[%d:%lld]"
                          " higher(or equal) than local lsn[%d:%lld],"
                          " we change to secondary.",
                          beat.endLsn.version, beat.endLsn.offset,
                          lsn.version, lsn.offset ) ;
               }
            }
            else if ( _info.primary.value != beat.identity.value )
            {
               PD_LOG( PDEVENT, "vote: the discovery of new primary[%d]",
                       beat.identity.columns.nodeID ) ;
               beforeFoundNewPrimary() ;
               _vote.force( CLS_ELECTION_STATUS_SILENCE ) ;
               _info.mtx.lock_w() ;
               _info.primary = beat.identity ;
               _info.mtx.release_w() ;
               afterFoundNewPrimary( getPrimary() ) ;

               /// when self is in slice, force to secondary
               if ( _vote.isStatus( CLS_ELECTION_STATUS_SILENCE ) )
               {
                  _vote.force( CLS_ELECTION_STATUS_SEC ) ;
               }
            }

            // if find new primary node, should to wake up reelection
            if ( CLS_ELECTION_WEIGHT_USR_MIN != _vote.getShadowWeight() &&
                 _vote.isShadowTimeout() )
            {
               reelectionDone() ;
            }
         }
         else
         {
            if ( _info.primary.value == beat.identity.value )
            {
               PD_LOG( PDEVENT, "vote: primary node[%d] is down",
                       beat.identity.columns.nodeID ) ;
               beforeFoundNewPrimary() ;
               _info.mtx.lock_w() ;
               _info.primary.value = MSG_INVALID_ROUTEID ;
               _info.mtx.release_w() ;
               afterFoundNewPrimary( getPrimary() ) ;
            }
         }
      }
      {
         _alive( beat.identity, _isUDPHandle( handle ) ) ;
         MsgClsBeatRes res ;
         res.header.header.requestID = msg->header.requestID ;
         res.identity = _info.local ;
         _agent->syncSend( handle, &res ) ;
      }

   done:
      PD_TRACE_EXITRC( SDB__ICLSREPLAGENT__HNDSHRBEAT, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   INT32 _ICLSReplAgent::_handleSharingBeatRes( NET_HANDLE handle,
                                                const MsgClsBeatRes *msg )
   {
      SDB_ASSERT( NULL != msg, "msg should not be NULL" ) ;
      return _alive( msg->identity, _isUDPHandle( handle ) ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__ICLSREPLAGENT__HANDLETIMEOUT, "_ICLSReplAgent::_handleTimeout" )
   void _ICLSReplAgent::_handleTimeout( UINT32 interval )
   {
      PD_TRACE_ENTRY( SDB__ICLSREPLAGENT__HANDLETIMEOUT ) ;

      if ( !_active )
      {
         goto done ;
      }
      _beatTime += interval ;
      if ( CLS_SHARING_BETA_INTERVAL <= _beatTime )
      {
         _sharingBeat() ;
         _beatTime = 0 ;
      }

      _checkBreak( interval ) ;

      _vote.handleTimeout( interval ) ;
      _sync.handleTimeout( interval ) ;

   done:
      PD_TRACE_EXIT( SDB__ICLSREPLAGENT__HANDLETIMEOUT ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__ICLSREPLAGENT__HNDEVENT, "_ICLSReplAgent::_handleEvent" )
   INT32 _ICLSReplAgent::_handleEvent( pmdEDUEvent *event )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__ICLSREPLAGENT__HNDEVENT ) ;

      if ( PMD_EDU_EVENT_STEP_DOWN == event->_eventType )
      {
         rc = _handleStepDown() ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "failed to step down:%d", rc ) ;
            goto error ;
         }
      }
      else if ( PMD_EDU_EVENT_STEP_UP == event->_eventType )
      {
         rc = _handleStepUp( event->_userData ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "failed to step up:%d", rc ) ;
            goto error ;
         }
      }
      else
      {
         PD_LOG( PDERROR, "unknown event type:%d", event->_eventType ) ;
         rc = SDB_SYS ;
         goto error ;
      }

   done:
      PD_TRACE_EXITRC( SDB__ICLSREPLAGENT__HNDEVENT, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION (SDB__ICLSREPLAGENT__HANDLESTEPDOWN, "_ICLSReplAgent::_handleStepDown" )
   INT32 _ICLSReplAgent::_handleStepDown()
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__ICLSREPLAGENT__HANDLESTEPDOWN ) ;
      _vote.setShadowWeight( CLS_ELECTION_WEIGHT_MIN ) ;
      _vote.force( CLS_ELECTION_STATUS_SEC ) ;
      PD_TRACE_EXITRC( SDB__ICLSREPLAGENT__HANDLESTEPDOWN, rc ) ;
      return rc ;
   }

   // PD_TRACE_DECLARE_FUNCTION (SDB__ICLSREPLAGENT__HANDLESTEPUP, "_ICLSReplAgent::_handleStepUp" )
   INT32 _ICLSReplAgent::_handleStepUp( UINT32 seconds )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__ICLSREPLAGENT__HANDLESTEPUP ) ;
      PD_LOG(PDEVENT, "force to step up, seconds:%d", seconds ) ;
      _vote.force( CLS_ELECTION_STATUS_PRIMARY, seconds * 1000 ) ;
      PD_TRACE_EXITRC( SDB__ICLSREPLAGENT__HANDLESTEPUP, rc ) ;
      return rc ;
   }

   void _ICLSReplAgent::_activate()
   {
      if ( !_active )
      {
         _active = TRUE ;
         _vote.init() ;
      }
   }

   void _ICLSReplAgent::_deactivate()
   {
      if ( _active )
      {
         _vote.force( CLS_ELECTION_STATUS_SEC, OSS_SINT32_MAX ) ;
      }
   }

}
