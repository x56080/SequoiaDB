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
      else if ( !pmdGetStartup().isOK() && !_info()->isAllNodeAbnormal( 0 ) )
      {
         PD_LOG ( PDWARNING, "Start type isn't normal, can't initial voting "
                  "until all nodes had been started" ) ;
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
         DPS_LSN lsn = _logger->getCurrentLsn() ;
         _MsgClsElectionBallot msg ;
         msg.weights = lsn ;
         msg.identity = _groupInfo->local ;
         msg.round = round ;
         map<UINT64, _clsSharingStatus *>::iterator itr=
                                       _groupInfo->alives.begin() ;
         for ( ; itr != _groupInfo->alives.end(); itr++ )
         {
            // if my bs is ok, but peer is not ok, skip
            if ( SERVICE_ABNORMAL == itr->second->beat.serviceStatus &&
                 pmdGetStartup().isOK() )
            {
               continue ;
            }
            if ( 0 > lsn.compare(itr->second->beat.endLsn ) )
            {
               PD_LOG ( PDDEBUG, "DSP lsn is not max, can't initial voting" ) ;
               rc = SDB_CLS_VOTE_FAILED ;
               goto error ;
            }
         }
         _broadcastAlives( &msg ) ;
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

      itrInfo = _groupInfo->info.find( id.value ) ;
      /// unknown member
      if ( _groupInfo->info.end() == itrInfo )
      {
         PD_LOG( PDWARNING, "unknown member [group:%d] [node:%d]",
                         id.columns.groupID, id.columns.nodeID ) ;
         goto error ;
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
      {
         map<UINT64, _clsSharingStatus *>::iterator itr =
                                    _groupInfo->alives.begin() ;
         for ( ; itr != _groupInfo->alives.end(); itr++ )
         {
            if ( SERVICE_NORMAL == itrInfo->second.beat.serviceStatus &&
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
      if ( pmdGetStartup().isOK() || _info()->isAllNodeAbnormal( 0 ) )
      {
         DPS_LSN local = _logger->getCurrentLsn() ;
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
      PD_LOG( PDDEBUG, "vote accept [node:%d] [lsn:%lld,%d] [round:%d]",
                        id.columns.nodeID, lsn.offset, lsn.version, round ) ;
      msg.header.res = SDB_OK ;
      _agent->syncSend( id, &msg ) ;
   done:
      PD_TRACE_EXITRC ( SDB__CLSVTSTUS__LAU1, rc ) ;
      return rc ;
   error:
      /// reuse err code
      rc = SDB_CLS_VOTE_FAILED ;
      goto done ;
   accepterr:
      PD_LOG( PDDEBUG, "vote refuse [node:%d] [lsn:%lld,%d] [round:%d]",
                       id.columns.nodeID, lsn.offset, lsn.version, round ) ;
      msg.header.res = SDB_CLS_VOTE_FAILED ;
      _agent->syncSend( id, &msg ) ;
      goto error ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSVTSTUS__BCALIVES, "_clsVoteStatus::_broadcastAlives" )
   void _clsVoteStatus::_broadcastAlives( void *msg )
   {
      PD_TRACE_ENTRY ( SDB__CLSVTSTUS__BCALIVES ) ;
      map<UINT64, _clsSharingStatus *>::iterator itr=
                                    _groupInfo->alives.begin() ;
      for ( ; itr != _groupInfo->alives.end(); itr++ )
      {
         _agent->syncSend( itr->second->beat.identity,
                           msg ) ;
      }
      PD_TRACE_EXIT ( SDB__CLSVTSTUS__BCALIVES ) ;
   }

}
