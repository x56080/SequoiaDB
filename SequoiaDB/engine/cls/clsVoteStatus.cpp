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
#include "clsReplAgent.hpp"
#include "pd.hpp"
#include "pdTrace.hpp"
#include "clsTrace.hpp"

namespace engine
{
   _clsVoteStatus::_clsVoteStatus( ICLSReplAgent *replAgent,
                                   INT32 id )
   : _replAgent( replAgent ),
     _groupInfo( replAgent->getGroupInfo() ),
     _agent( replAgent->getNetAgent() ),
     _id( id ),
     _time( 0 ),
     _acceptedNum( 0 )
   {
      SDB_ASSERT( CLS_INVALID_VOTE_ID != _id,
                  "id should not be invalid" ) ;
   }

   _clsVoteStatus::~_clsVoteStatus()
   {

   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSVTSTUS__LAU, "_clsVoteStatus::_launch" )
   INT32 _clsVoteStatus::_launch( const CLS_ELECTION_ROUND &round )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB__CLSVTSTUS__LAU ) ;

      if ( 0 != _groupInfo->primary.value )
      {
         PD_LOG ( PDDEBUG, "%s Vote: primary[%u] already exist, can't initial voting",
                  getScopeName(), _groupInfo->primary.columns.nodeID ) ;
         rc = SDB_CLS_VOTE_FAILED ;
         goto error ;
      }
      else if ( CLS_GROUP_MODE_MAINTENANCE == _groupInfo->localGrpMode )
      {
         PD_LOG( PDINFO, "%s Vote: node is in maintenance mode, "
                 "can't initial voting", getScopeName() ) ;
         goto done ;
      }
      /* 
         1. For group in normal mode, check if majority nodes are alive.
         2. For group in critical mode, check if local node is in critical mode and
            if majority critical nodes are alive. Only replica group's election is effective.
       */
      else if ( ! CLS_IS_MAJORITY( _groupInfo->aliveSize(), _groupInfo->groupSize() ) &&
                ! ( ! isLocation() &&
                    CLS_GROUP_MODE_CRITICAL == _groupInfo->localGrpMode &&
                    CLS_IS_MAJORITY( _groupInfo->criticalAliveSize(), _groupInfo->criticalSize() ) ) )
      {
         PD_LOG ( PDINFO, "%s Vote: alive nodes is not major, can't initial voting, "
                  "alive size = %d, group size = %d",
                  getScopeName(), _groupInfo->aliveSize() , _groupInfo->groupSize() ) ;
         rc = SDB_CLS_VOTE_FAILED ;
         goto error ;
      }
      else if ( _replAgent->isLocalAbnormal() &&
                !_info()->isAllNodeAbnormal( 0 ) )
      {
         PD_LOG ( PDWARNING, "Start type isn't normal, can't initial voting "
                  "until all nodes had been started" ) ;
         rc = SDB_CLS_VOTE_FAILED ;
         goto error ;
      }
<<<<<<< HEAD
      else if ( SDB_OK != sdbGetReplCB()->getSyncEmptyEvent()->wait( 0 ) )
      {
         PD_LOG( PDWARNING, "Repl sync log is running, "
                 "can't initial voting" ) ;
         rc = SDB_CLS_VOTE_FAILED ;
         goto error ;
      }
      else if ( !sdbGetReplCB()->getBucket()->isEmpty() )
=======
      else if ( !_replAgent->checkVoteLaunch() )
>>>>>>> c4064a6f2c2dfdf2b1bf049c2f904b74db0494b2
      {
         PD_LOG( PDWARNING, "Failed to check vote launch" ) ;
         rc = SDB_CLS_VOTE_FAILED ;
         goto error ;
      }

      // launch
      {
         DPS_LSN lsn = _replAgent->getLocalExpectLSN() ;
         _MsgClsElectionBallot msg ;
         msg.weights = lsn ;
         msg.identity = _groupInfo->local ;
         msg.round = round ;
<<<<<<< HEAD
         if ( isLocation() )
         {
            // Add locationID to ballot msg
            msg.locationID = _groupInfo->localLocationID ;
         }

         map<UINT64, _clsSharingStatus *>::const_iterator itr = _groupInfo->alives.begin() ;

         // Local node is in enforced critical mode, only replica group's election is effective
         if ( ! isLocation() &&
              CLS_GROUP_MODE_CRITICAL == _groupInfo->localGrpMode &&
              CLS_IS_MAJORITY( _groupInfo->criticalAliveSize(), _groupInfo->criticalSize() ) )
         {
            while ( _groupInfo->alives.end() != itr )
=======
         CLS_ALIVE_MAP::iterator itr = _groupInfo->alives.begin() ;
         for ( ; itr != _groupInfo->alives.end(); itr++ )
         {
            // if my bs is ok, but peer is not ok, skip
            if ( SERVICE_ABNORMAL == itr->second->beat.serviceStatus &&
                 _replAgent->isLocalOK() )
>>>>>>> c4064a6f2c2dfdf2b1bf049c2f904b74db0494b2
            {
               const _clsSharingStatus* status = ( itr++ )->second ;
               // If local is normal but iterator node is abnormal, continue
               if ( SERVICE_ABNORMAL == status->beat.serviceStatus && pmdGetStartup().isOK() )
               {
                  continue ;
               }
               // If iterator node is not in critical node, ignore
               // This condition helps node in enforced critical mode to start a new election which
               // breaks the principle of lsn first.
               else if ( ! status->isInCriticalMode() )
               {
                  continue ;
               }
               // If iterator node is in critical node and lsn is greater than local's, stop initializing vote
               else if ( 0 > lsn.compare( status->beat.endLsn ) )
               {
                  PD_LOG ( PDDEBUG, "%s Vote: DSP lsn is not max, can't initial voting",
                           getScopeName() ) ;
                  rc = SDB_CLS_VOTE_FAILED ;
                  goto error ;
               }
            }
         }
         else
         {
            for ( ; itr != _groupInfo->alives.end(); itr++ )
            {
               // if my bs is ok, but peer is not ok, skip
               if ( SERVICE_ABNORMAL == itr->second->beat.serviceStatus && pmdGetStartup().isOK() )
               {
                  continue ;
               }
               if ( 0 > lsn.compare( itr->second->beat.endLsn ) )
               {
                  PD_LOG ( PDDEBUG, "%s Vote: DSP lsn is not max, can't initial voting",
                           getScopeName() ) ;
                  rc = SDB_CLS_VOTE_FAILED ;
                  goto error ;
               }
            }
         }

         _broadcastAlives( &msg ) ;
         PD_LOG( PDEVENT, "%s Vote: broadcast vote[round:%d] to all alive nodes",
                 getScopeName(), round ) ;
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
      CLS_NODE_MAP::iterator itrInfo ;
      BOOLEAN peerAbnormal = FALSE ;
      BOOLEAN localAbnormal = FALSE ;
      DPS_LSN local ;

      // Add locationID to ballot msg
      if ( isLocation() )
      {
         msg.locationID = _groupInfo->localLocationID ;
      }

      itrInfo = _groupInfo->info.find( id.value ) ;
      /// unknown member
      if ( _groupInfo->info.end() == itrInfo )
      {
         PD_LOG( PDWARNING, "unknown member [group:%u] [node:%u]",
                 id.columns.groupID, id.columns.nodeID ) ;
         goto error ;
      }
      if ( SERVICE_NORMAL != itrInfo->second.beat.serviceStatus )
      {
         peerAbnormal = TRUE ;
      }
      if ( _replAgent->isLocalAbnormal() )
      {
         localAbnormal = TRUE ;
      }

      /// primary is exist. refuse
      if ( MSG_INVALID_ROUTEID !=_groupInfo->primary.value )
      {
         PD_LOG( PDDEBUG, "%s Vote: the primary still exist [group:%u] [node:%u]",
                 getScopeName(),
                 _groupInfo->primary.columns.groupID,
                 _groupInfo->primary.columns.nodeID ) ;
         goto accepterr ;
      }

      // If node is in maintenance node, don't vote
      if ( CLS_GROUP_MODE_MAINTENANCE == _groupInfo->localGrpMode )
      {
         PD_LOG( PDINFO, "%s Vote: node is in maintenance mode, can't vote", getScopeName() ) ;
         goto done ;
      }

      // Check if majority nodes are alive or peer node is in critical mode,
      // if not, do not response.
      if ( ! CLS_IS_MAJORITY( _groupInfo->aliveSize(), _groupInfo->groupSize() ) &&
           ! itrInfo->second.isInCriticalMode() )
      {
         PD_LOG( PDDEBUG, "%s Vote: sharing break whih majority", getScopeName() ) ;
         goto error ;
      }
<<<<<<< HEAD

      if ( NULL == _logger )
      {
         _logger = pmdGetKRCB()->getDPSCB() ;
         SDB_ASSERT( NULL != _logger, "logger should not be NULL" ) ;
      }
      local = _logger->expectLsn() ;
=======
      local = _replAgent->getLocalExpectLSN() ;
>>>>>>> c4064a6f2c2dfdf2b1bf049c2f904b74db0494b2

      // Local node is in enforced critical mode
      if ( ! isLocation() &&
           CLS_GROUP_MODE_CRITICAL == _groupInfo->localGrpMode &&
           _groupInfo->enforcedGrpMode )
      {
         map<UINT64, _clsSharingStatus *>::const_iterator itr = _groupInfo->alives.begin() ;
         while ( _groupInfo->alives.end() != itr )
         {
            const _clsSharingStatus* status = ( itr++ )->second ;
            // If peer is normal but iterator node is abnormal, continue
            if ( ! peerAbnormal && SERVICE_ABNORMAL == status->beat.serviceStatus )
            {
               continue ;
            }
            // If iterator node is not in critical node, ignore
            else if ( ! status->isInCriticalMode() )
            {
               continue ;
            }
            // If iterator node is in critical node and lsn is greater than peer's, accept error
            else if ( 0 > lsn.compare( status->beat.endLsn ) )
            {
               goto accepterr ;
            }
         }
      }
      // Node is not in enforced critical mode
      else
      {
<<<<<<< HEAD
         map<UINT64, _clsSharingStatus *>::const_iterator itr = _groupInfo->alives.begin() ;
=======
         CLS_ALIVE_MAP::iterator itr = _groupInfo->alives.begin() ;
>>>>>>> c4064a6f2c2dfdf2b1bf049c2f904b74db0494b2
         for ( ; itr != _groupInfo->alives.end(); itr++ )
         {
            if ( !peerAbnormal &&
                 SERVICE_ABNORMAL == itr->second->beat.serviceStatus )
            {
               continue ;
            }
            /// find anyone's lsn > request's lsn. refuse.
            else if ( 0 > lsn.compare( itr->second->beat.endLsn ) )
            {
               goto accepterr ;
            }
         }
      }

      /* when 1) self is business ok
              2) peer node is abnormal
         need to judge self lsn */
      if ( !localAbnormal || peerAbnormal )
      {
         INT32 cRc = local.compare( lsn ) ;
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
<<<<<<< HEAD
            UINT8 shadowWeight = 0 ;
            UINT8 peerShadowWeight = 0 ;
            _clsVoteMachine* vote = sdbGetReplCB()->voteMachine( isLocation() ) ;

            // Get and compare electionWeight only if the peer beat version is greater than 1
            // Here we just comapre electionWeight, if the twos are equal, compare the _shadowWeight
            if ( ! isLocation() && CLS_BEAT_VERSION_2 <= itrInfo->second.beat.beatVersion )
            {
               UINT8 localElectionWeight = vote->getElectionWeight() ;
               UINT8 peerElectionWeight = itrInfo->second.beat.getElectionWeight() ;

               if ( localElectionWeight > peerElectionWeight )
               {
                  goto accepterr ;
               }
               else if ( localElectionWeight < peerElectionWeight )
               {
                  goto accept ;
               }
            }

            // Get weight
            shadowWeight = CLS_GET_WEIGHT( pmdGetOptionCB()->weight(), vote->getShadowWeight() ) ;
            if ( isLocation() )
            {
               peerShadowWeight = itrInfo->second.beat.getLocationWeight() ;
            }
            else
            {
               peerShadowWeight = itrInfo->second.beat.weight ;
            }

            // Compare weight
            if ( shadowWeight < peerShadowWeight )
=======
            UINT8 voteWeight = _replAgent->getVoteWeight() ;
            UINT8 shadowWeight = _replAgent->getVoteMachine()->getShadowWeight() ;
            UINT8 weight = CLS_GET_WEIGHT( voteWeight, shadowWeight ) ;
            const UINT8 remoteWeight = itrInfo->second.beat.weight ;
            if ( weight < remoteWeight )
>>>>>>> c4064a6f2c2dfdf2b1bf049c2f904b74db0494b2
            {
               goto accept ;
            }
            else if ( peerShadowWeight < shadowWeight )
            {
               goto accepterr ;
            }
<<<<<<< HEAD
            else if ( peerShadowWeight < pmdGetOptionCB()->weight() )
=======
            else if ( remoteWeight < voteWeight )
>>>>>>> c4064a6f2c2dfdf2b1bf049c2f904b74db0494b2
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
      PD_LOG( PDEVENT, "%s Vote: accept node[id:%u, lsn:%u.%lld, round:%d, "
              "abnormal: %s], local[lsn:%u.%lld, abnormal:%s]", getScopeName(),
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
      PD_LOG( PDDEBUG, "%s Vote: refuse node[id:%u, lsn:%u.%lld, round:%d, "
              "abnormal: %s], local[lsn:%u.%lld, abnormal:%s]", getScopeName(),
              id.columns.nodeID, lsn.version, lsn.offset, round,
              (peerAbnormal ? "TRUE":"FALSE"),
              local.version, local.offset,
              (localAbnormal ? "TRUE":"FALSE") ) ;
      msg.header.res = SDB_CLS_VOTE_FAILED ;
      _agent->syncSend( id, (MsgHeader *)&msg ) ;
      goto error ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSVTSTUS__BCALIVES, "_clsVoteStatus::_broadcastAlives" )
   void _clsVoteStatus::_broadcastAlives( void *msg )
   {
      PD_TRACE_ENTRY ( SDB__CLSVTSTUS__BCALIVES ) ;
<<<<<<< HEAD
      map<UINT64, _clsSharingStatus *>::iterator itr = _groupInfo->alives.begin() ;
=======
      CLS_ALIVE_MAP::iterator itr = _groupInfo->alives.begin() ;
>>>>>>> c4064a6f2c2dfdf2b1bf049c2f904b74db0494b2
      for ( ; itr != _groupInfo->alives.end(); itr++ )
      {
         _agent->syncSend( itr->second->beat.identity, (MsgHeader *)msg ) ;
      }
      PD_TRACE_EXIT ( SDB__CLSVTSTUS__BCALIVES ) ;
   }

   BOOLEAN _clsVoteStatus::_isAccepted()
   {
      BOOLEAN isAccept = FALSE ;

      // Node is in normal mode
      if ( CLS_IS_MAJORITY( _acceptedNum + 1, _groupInfo->groupSize() ) )
      {
         isAccept = TRUE ;
      }
      // Node in critical mode which is only effective in group election( not in location election )
      else if ( ! isLocation() && CLS_GROUP_MODE_CRITICAL == _groupInfo->localGrpMode )
      {
         if ( CLS_IS_MAJORITY( _criticalAcceptedNum + 1, _groupInfo->criticalSize() ) )
         {
            // Node is in enforced mode, need to check if majority critical nodes agree
            if ( _groupInfo->enforcedGrpMode )
            {
               isAccept = TRUE ;
            }
            // Node is not in enforced mode, need to check if majority nodes
            // and majotiry critical nodes agree
            else if ( CLS_IS_MAJORITY( _acceptedNum + 1, _groupInfo->aliveSize() ) )
            {
               isAccept = TRUE ;
            }
         }
      }

      return isAccept ;
   }

}
