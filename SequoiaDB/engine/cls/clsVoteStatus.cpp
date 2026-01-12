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

   Source File Name = clsVoteStatus.cpp

   Descriptive Name =

   When/how to use:

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          11/28/2012  YW  Initial Draft

   Last Changed =

*******************************************************************************/
#include "clsVoteStatus.hpp"
#include "pmd.hpp"
#include "pmdCB.hpp"
#include "dpsLogWrapper.hpp"
#include "pd.hpp"
#include "pdTrace.hpp"
#include "clsTrace.hpp"
#include "pmdStartup.hpp"

namespace engine
{

   static INT32 _compareLsnWithStatus( const DPS_LSN &left, BOOLEAN leftNormal,
                                       const DPS_LSN &right, BOOLEAN rightNormal,
                                       DPS_LSN_OFFSET abnormalThreshold )
   {
      INT32 cmp = 0 ;

      if ( leftNormal && !rightNormal )
      {
         DPS_LSN tmpLsn = right ;
         cmp = left.compareVersion( tmpLsn.version ) ;
         if ( cmp <= 0 )
         {
            if ( DPS_INVALID_LSN_OFFSET != tmpLsn.offset )
            {
               if ( tmpLsn.offset >= abnormalThreshold )
               {
                  tmpLsn.offset -= abnormalThreshold ;
               }
               else
               {
                  tmpLsn.offset = DPS_INVALID_LSN_OFFSET ;
               }
            }
            cmp = left.compareOffset( tmpLsn ) ;
         }
      }
      else if ( !leftNormal && rightNormal )
      {
         DPS_LSN tmpLsn = left ;
         cmp = tmpLsn.compareVersion( right.version ) ;
         if ( cmp >= 0 )
         {
            if ( DPS_INVALID_LSN_OFFSET != tmpLsn.offset )
            {
               if ( tmpLsn.offset >= abnormalThreshold )
               {
                  tmpLsn.offset -= abnormalThreshold ;
               }
               else
               {
                  tmpLsn.offset = DPS_INVALID_LSN_OFFSET ;
               }
            }
            cmp = tmpLsn.compareOffset( right ) ;
         }
      }
      else
      {
         cmp = left.compare( right ) ;
      }

      return cmp ;
   }

   /*
      _clsVoteStatus implement
   */
   _clsVoteStatus::_clsVoteStatus( _clsGroupInfo *info,
                                   _netRouteAgent *agent,
                                   INT32 id ):
                                   _groupInfo( info ),
                                   _agent( agent ),
                                   _logger( NULL ),
                                   _id( id )
   {
      SDB_ASSERT( CLS_INVALID_VOTE_ID != _id,
                  "id should not be invalid" ) ;
      _time = 0 ;
      _acceptedNum = 0 ;
   }

   _clsVoteStatus::~_clsVoteStatus()
   {

   }

   void _clsVoteStatus::setImmediatelyTime()
   {
      _timeout() = CLS_VOTE_CS_TIME ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSVTSTUS__LAU, "_clsVoteStatus::_launch" )
   INT32 _clsVoteStatus::_launch( const CLS_ELECTION_ROUND &round )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB__CLSVTSTUS__LAU ) ;

      if ( 0 != _groupInfo->primary.value )
      {
         PD_LOG ( PDDEBUG, "Primary[%d] already exist, can't initial voting",
                  _groupInfo->primary.columns.nodeID ) ;
         rc = SDB_CLS_VOTE_FAILED ;
         goto error ;
      }
      else if ( !CLS_IS_MAJORITY( _groupInfo->aliveSize() ,
                                  _groupInfo->groupSize() ) )
      {
         PD_LOG ( PDINFO, "Alive nodes is not major, can't initial voting, "
                  "alive size = %d, group size = %d",
                  _groupInfo->aliveSize() , _groupInfo->groupSize() ) ;
         rc = SDB_CLS_VOTE_FAILED ;
         goto error ;
      }
      else if ( SDB_OK != sdbGetReplCB()->getSyncEmptyEvent()->wait( 0 ) )
      {
         PD_LOG( PDWARNING, "Repl sync log is running, "
                 "can't initial voting" ) ;
         rc = SDB_CLS_VOTE_FAILED ;
         goto error ;
      }
      else if ( !sdbGetReplCB()->getBucket()->isEmpty() )
      {
         PD_LOG( PDWARNING, "Repl log is not empty, can't initial voting, "
                 "repl bucket size: %d",
                 sdbGetReplCB()->getBucket()->size() ) ;
         rc = SDB_CLS_VOTE_FAILED ;
         goto error ;
      }
      else if ( sdbGetTransCB()->isNeedSyncTrans() &&
                pmdGetStartup().isOK() )
      {
         PD_LOG( PDWARNING, "Trans info is not sync, can't initial voting" ) ;
         rc = SDB_CLS_VOTE_FAILED ;
         goto error ;
      }

      if ( NULL == _logger )
      {
         _logger = pmdGetKRCB()->getDPSCB() ;
         SDB_ASSERT( NULL != _logger, "logger should not be NULL" ) ;
      }

      // launch
      {
         UINT32 broadCount = 0 ;
         BOOLEAN localAbnormal = FALSE ;
         INT64 abnormalThreshold = _logger->electionLsnAdvantageThreshold() ;
         DPS_LSN lsn = _logger->expectLsn() ;
         _MsgClsElectionBallot msg ;
         msg.weights = lsn ;
         msg.identity = _groupInfo->local ;
         msg.round = round ;

         map<UINT64, _clsSharingStatus *>::const_iterator itr ;
         if ( !pmdGetStartup().isOK() )
         {
            localAbnormal = TRUE ;

            if ( _logger->getCurrentLsn().invalid() && !_groupInfo->isAllNodeBeat() )
            {
               PD_LOG( PDWARNING, "Start type isn't normal and LSN is invalid"
                       "(maybe not complete fullsync), can't inital voting "
                       "until all nodes had been started") ;
               rc = SDB_CLS_VOTE_FAILED ;
               goto error ;
            }
         }

         itr = _groupInfo->alives.begin() ;
         for ( ; itr != _groupInfo->alives.end(); itr++ )
         {
            const _clsSharingStatus* status = itr->second ;
            // If local is normal but iterator node is abnormal, continue
            if ( SERVICE_ABNORMAL == status->beat.serviceStatus && !localAbnormal &&
                  abnormalThreshold < 0 )
            {
               continue ;
            }
            // If iterator node is in critical node and lsn is greater than local's, stop initializing vote
            else if ( _compareLsnWithStatus( lsn, !localAbnormal, status->beat.endLsn,
                                             SERVICE_NORMAL == status->beat.serviceStatus,
                                             abnormalThreshold ) < 0 )
            {
               PD_LOG ( PDDEBUG, "Vote: DSP lsn is not max, can't initial voting" ) ;
               rc = SDB_CLS_VOTE_FAILED ;
               goto error ;
            }
         }

         broadCount = _broadcastAlives( &msg ) ;
         PD_LOG( PDEVENT, "Vote: broadcast vote[round:%d] to all alive nodes, succeed:%u",
                 round, broadCount ) ;
      }

   done:
      PD_TRACE_EXITRC ( SDB__CLSVTSTUS__LAU, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSVTSTUS__LAU1, "_clsVoteStatus::_launch" )
   INT32 _clsVoteStatus::_launch( const DPS_LSN &lsn,
                                  const _MsgRouteID &id,
                                  const CLS_ELECTION_ROUND &round )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB__CLSVTSTUS__LAU1 ) ;
      _MsgClsElectionRes msg ;
      msg.identity = _groupInfo->local ;
      msg.round = round ;
      map<UINT64, _clsSharingStatus >::iterator itrInfo ;
      BOOLEAN peerAbnormal = FALSE ;
      BOOLEAN localAbnormal = FALSE ;
      INT64 abnormalThreshold = 0 ;
      DPS_LSN local ;

      itrInfo = _groupInfo->info.find( id.value ) ;
      /// unknown member
      if ( _groupInfo->info.end() == itrInfo )
      {
         PD_LOG( PDWARNING, "unknown member [group:%d] [node:%d]",
                 id.columns.groupID, id.columns.nodeID ) ;
         goto error ;
      }
      if ( SERVICE_NORMAL != itrInfo->second.beat.serviceStatus )
      {
         peerAbnormal = TRUE ;
      }
      if ( !pmdGetStartup().isOK() )
      {
         localAbnormal = TRUE ;
      }

      /// primary is exist. refuse
      if ( MSG_INVALID_ROUTEID !=_groupInfo->primary.value )
      {
         PD_LOG( PDDEBUG, "vote:the primary still exist [group:%d] [node:%d]",
                 _groupInfo->primary.columns.groupID,
                 _groupInfo->primary.columns.nodeID ) ;
         goto accepterr ;
      }
      /// majority members' status are unknown.do not response
      if ( !CLS_IS_MAJORITY( _groupInfo->aliveSize() ,
                             _groupInfo->groupSize() ) )
      {
         PD_LOG( PDDEBUG, "vote: sharing break whih majority" ) ;
         goto error ;
      }
      if ( NULL == _logger )
      {
         _logger = pmdGetKRCB()->getDPSCB() ;
         SDB_ASSERT( NULL != _logger, "logger should not be NULL" ) ;
      }
      local = _logger->expectLsn() ;
      abnormalThreshold = _logger->electionLsnAdvantageThreshold() ;

      {
         map<UINT64, _clsSharingStatus *>::iterator itr =
                                    _groupInfo->alives.begin() ;
         for ( ; itr != _groupInfo->alives.end(); itr++ )
         {
            const _clsSharingStatus* status = itr->second ;
            // If peer is normal but iterator node is abnormal, continue
            if ( ! peerAbnormal && SERVICE_ABNORMAL == status->beat.serviceStatus &&
                 abnormalThreshold < 0 )
            {
               continue ;
            }
            // If node lsn is greater than peer's, accept error
            else if ( _compareLsnWithStatus( lsn, !peerAbnormal, status->beat.endLsn,
                                             SERVICE_NORMAL == status->beat.serviceStatus,
                                             abnormalThreshold ) < 0 )
            {
               goto accepterr ;
            }
         }
      }

      /* when 1) self is business ok or
              2) peer node is abnormal or
              3) abnormalThreshold >= 0
         need to judge self lsn */
      if ( !localAbnormal || peerAbnormal || abnormalThreshold >= 0 )
      {
         INT32 cRc = _compareLsnWithStatus( local, !localAbnormal, lsn,
                                            !peerAbnormal, abnormalThreshold ) ;
         /// local < lsn. accept
         if ( 0 > cRc )
         {
            goto accept ;
         }
         /// local > lsn. refuse
         else if ( 0 < cRc )
         {
            goto accepterr ;
         }
         /// the same, judge weight.
         else
         {
            UINT8 weight = pmdGetOptionCB()->weight() ;
            UINT8 shadowWeight = sdbGetReplCB()->voteMachine()->getShadowWeight() ;
            weight = CLS_GET_WEIGHT( weight, shadowWeight ) ;
            const UINT8 remoteWeight = itrInfo->second.beat.weight ;
            if ( weight < remoteWeight )
            {
               goto accept ;
            }
            else if ( remoteWeight < weight )
            {
               goto accepterr ;
            }
            else if ( itrInfo->second.beat.weight < pmdGetOptionCB()->weight() )
            {
               goto accepterr ;
            }
            /// judge id
            else if ( id.value < _groupInfo->local.value )
            {
               goto accepterr ;
            }
            else
            {
               goto accept ;
            }
         }
      }

   accept:
      PD_LOG( PDEVENT, "vote: Accept node[id:%d, lsn:%u.%lld, round:%d, "
              "abnormal: %s], local[lsn:%u.%lld, abnormal:%s]",
              id.columns.nodeID, lsn.version, lsn.offset, round,
              (peerAbnormal ? "TRUE":"FALSE"),
              local.version, local.offset,
              (localAbnormal ? "TRUE":"FALSE") ) ;
      msg.header.res = SDB_OK ;
      _agent->syncSend( id, (MsgHeader *)&msg ) ;
   done:
      PD_TRACE_EXITRC ( SDB__CLSVTSTUS__LAU1, rc ) ;
      return rc ;
   error:
      /// reuse err code
      rc = SDB_CLS_VOTE_FAILED ;
      goto done ;
   accepterr:
      PD_LOG( PDDEBUG, "vote: Refuse node[id:%d, lsn:%u.%lld, round:%d, "
              "abnormal: %s], local[lsn:%u.%lld, abnormal:%s]",
              id.columns.nodeID, lsn.version, lsn.offset, round,
              (peerAbnormal ? "TRUE":"FALSE"),
              local.version, local.offset,
              (localAbnormal ? "TRUE":"FALSE") ) ;
      msg.header.res = SDB_CLS_VOTE_FAILED ;
      _agent->syncSend( id, (MsgHeader *)&msg ) ;
      goto error ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSVTSTUS__BCALIVES, "_clsVoteStatus::_broadcastAlives" )
   UINT32 _clsVoteStatus::_broadcastAlives( void *msg )
   {
      UINT32 count = 0 ;
      PD_TRACE_ENTRY ( SDB__CLSVTSTUS__BCALIVES ) ;
      map<UINT64, _clsSharingStatus *>::iterator itr=
                                    _groupInfo->alives.begin() ;
      for ( ; itr != _groupInfo->alives.end(); itr++ )
      {
         if ( SDB_OK == _agent->syncSend( itr->second->beat.identity, (MsgHeader *)msg ) )
         {
            ++count ;
         }
      }
      PD_TRACE_EXIT ( SDB__CLSVTSTUS__BCALIVES ) ;
      return count ;
   }

}
