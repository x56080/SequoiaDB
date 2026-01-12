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

   Source File Name = clsVoteMachine.hpp

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
#include "clsVSVote.hpp"
#include "pdTrace.hpp"
#include "clsTrace.hpp"
#include "pmdStartup.hpp"

namespace engine
{
   _clsVSVote::_clsVSVote( _clsGroupInfo *info, _netRouteAgent *agent ):
   _clsVoteStatus( info, agent, CLS_ELECTION_STATUS_VOTE )
   {

   }

   _clsVSVote::~_clsVSVote()
   {

   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSVSVT_HDINPUT, "_clsVSVote::handleInput" )
   INT32 _clsVSVote::handleInput( const MsgHeader *header,
                                  INT32 &next )
   {
      PD_TRACE_ENTRY ( SDB__CLSVSVT_HDINPUT ) ;
      SDB_ASSERT( NULL != header, "header should not be NULL" ) ;

      if ( MSG_CLS_BALLOT == header->opCode )
      {
         const _MsgClsElectionBallot *msg = ( const _MsgClsElectionBallot * ) header ;
         if ( CLS_ELECTION_ROUND_STAGE_ONE == msg->round )
         {
            if ( SDB_OK == _promise( msg ) )
            {
               next = CLS_ELECTION_STATUS_SEC;
            }
            else
            {
               next = id() ;
            }
         }
         else
         {
            if ( SDB_OK == _accept( msg ) )
            {
               next = CLS_ELECTION_STATUS_SILENCE ;
            }
            else
            {
               next = id() ;
            }
         }
      }
      else if ( MSG_CLS_BALLOT_RES == header->opCode )
      {
         const _MsgClsElectionRes *msg = ( const _MsgClsElectionRes * ) header ;
         if ( CLS_ELECTION_ROUND_STAGE_ONE == msg->round )
         {
            if ( SDB_OK == msg->header.res )
            {
               const _clsSharingStatus &status = _info()->info.find( msg->identity.value )->second ;

               // If this accepted msg is from critical node, record it
               if ( status.isInCriticalMode() )
               {
                  ++_criticalAccepted() ;
               }

               // If this accepted msg is from maintenance node, ignore it
               if ( ! status.isInMaintenanceMode() )
               {
                  ++_accepted() ;
               }

               // Node in normal mode, not critical mode
               if ( _info()->groupSize() <= ( _accepted() + 1 ) )
               {
                  next = CLS_ELECTION_STATUS_ANNOUNCE ;
                  PD_LOG( PDEVENT, "%s Vote: change to announce by all accept", getScopeName() ) ;
               }
               else
               {
                  next = id() ;
               }
            }
            // If is in enforced critical mode, ignore the reject result, wait until timeout
            else if ( ! isLocation() && _info()->enforcedGrpMode &&
                      CLS_GROUP_MODE_CRITICAL == _info()->localGrpMode )
            {
               next = id() ;
            }
            else
            {
               next = CLS_ELECTION_STATUS_SEC ;
            }
         }
         else
         {
            next = id() ;
         }
      }
      else
      {
         next = id() ;
      }

      PD_TRACE_EXIT ( SDB__CLSVSVT_HDINPUT ) ;
      return SDB_OK ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSVSVT_HDTMOUT, "_clsVSVote::handleTimeout" )
   void _clsVSVote::handleTimeout( const UINT32 &millisec,
                                   INT32 &next )
   {
      PD_TRACE_ENTRY ( SDB__CLSVSVT_HDTMOUT ) ;
      _timeout() += millisec ;
      if ( CLS_VOTE_CS_TIME <= _timeout() )
      {
         if ( _isAccepted() )
         {
            next = CLS_ELECTION_STATUS_ANNOUNCE ;

            if ( CLS_GROUP_MODE_CRITICAL == _info()->localGrpMode && ! isLocation() )
            {
               if ( _info()->enforcedGrpMode )
               {
                  PD_LOG( PDEVENT, "%s Vote: change to announce by timeout "
                          "in enforced critical mode", getScopeName() ) ;
               }
               else
               {
                  PD_LOG( PDEVENT, "%s Vote: change to announce by timeout "
                          "in critical mode", getScopeName() ) ;
               }
            }
            else
            {
               PD_LOG( PDEVENT, "%s Vote: change to announce by timeout", getScopeName() ) ;
            }
         }
         else
         {
            next = CLS_ELECTION_STATUS_SEC ;
         }
      }
      else
      {
         next = id() ;
      }
      PD_TRACE_EXIT ( SDB__CLSVSVT_HDTMOUT ) ;
      return ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSVSVT_ACTIVE, "_clsVSVote::active" )
   void _clsVSVote::active( INT32 &next )
   {
      PD_TRACE_ENTRY ( SDB__CLSVSVT_ACTIVE ) ;
      _timeout() = 0 ;
      _accepted() = 0 ;
      _criticalAccepted() = 0 ;

      if ( _info()->groupSize() == 1 )
      {
         next = CLS_ELECTION_STATUS_ANNOUNCE ;
      }
      else
      {
         next = id() ;
         _vote() ;
      }
      PD_TRACE_EXIT ( SDB__CLSVSVT_ACTIVE ) ;
      return ;
   }
}
