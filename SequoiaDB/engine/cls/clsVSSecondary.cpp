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

   Source File Name = clsVSSecondary.cpp

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
#include "clsVSSecondary.hpp"
#include "pdTrace.hpp"
#include "clsTrace.hpp"

namespace engine
{

   INT32 g_startShiftTime = 0 ;

   _clsVSSecondary::_clsVSSecondary( _clsGroupInfo *info,
                                     _netRouteAgent *agent )
   :_clsVoteStatus( info, agent, CLS_ELECTION_STATUS_SEC )
   {
      _hasPrint = FALSE ;
   }

   _clsVSSecondary::~_clsVSSecondary()
   {

   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSVSSD_HDINPUT, "_clsVSSecondary::handleInput" ) 
   INT32 _clsVSSecondary::handleInput( const MsgHeader *header,
                                       INT32 &next )
   {
      PD_TRACE_ENTRY ( SDB__CLSVSSD_HDINPUT ) ;
      SDB_ASSERT( NULL != header, "header should not be NULL" ) ;
      if ( MSG_CLS_BALLOT_RES == header->opCode )
      {
         next = id() ;
         goto done ;
      }
      else if ( MSG_CLS_BALLOT == header->opCode )
      {
         if ( !isLocation() && g_startShiftTime > 0 )
         {
            g_startShiftTime = -1 ; // some node begin vote
            PD_LOG( PDEVENT, "%s Vote: Has recieved vote require from node(%u), "
                    "disable startup grace period",
                    getScopeName(), header->routeID.columns.nodeID ) ;
         }

         const _MsgClsElectionBallot *msg = ( const _MsgClsElectionBallot * ) header ;
         if ( CLS_ELECTION_ROUND_STAGE_ONE == msg->round )
         {
            _promise( msg ) ;
            next = id() ;
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
      else
      {
         /// error msg
         next = id() ;
      }
   done:
      PD_TRACE_EXIT ( SDB__CLSVSSD_HDINPUT ) ;
      return SDB_OK ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSVSSD_HDTMOUT, "_clsVSSecondary::handleTimeout" )
   void _clsVSSecondary::handleTimeout( const UINT32 &millisec,
                                        INT32 &next )
   {
      INT32 startShiftTime = g_startShiftTime ;
      PD_TRACE_ENTRY ( SDB__CLSVSSD_HDTMOUT ) ;
      _timeout() += millisec ;

      if ( _info()->isAllNodeBeat() ||
          ( CLS_GROUP_MODE_CRITICAL == _info()->localGrpMode &&
            _info()->grpMode.enforced &&
            _info()->isAllCriticalNodeBeat() ) )
      {
         startShiftTime = -1 ;

         if ( !isLocation() && g_startShiftTime > 0 )
         {
            g_startShiftTime = -1 ; // recieve all node sharing-beat
            PD_LOG( PDEVENT, "%s Vote: Has recieved all nodes beat, disable startup grace period",
                    getScopeName() ) ;
         }
      }

      if ( ( startShiftTime < 0 || startShiftTime <= (INT32)_timeout() ) &&
           CLS_VOTE_CS_TIME <= _timeout() )
      {
         if ( _hasPrint )
         {
            PD_LOG( PDEVENT, "%s Vote: begin to vote...", getScopeName() ) ;
         }

         startShiftTime = -1 ;

         if ( !isLocation() && g_startShiftTime > 0 )
         {
            PD_LOG( PDEVENT, "%s Vote: The startup grace period(%u secs) has timeout, disable it",
                    getScopeName(), g_startShiftTime / OSS_ONE_SEC ) ;
            g_startShiftTime = -1 ;
         }
         next = CLS_ELECTION_STATUS_VOTE ;
      }
      else
      {
         if ( !_hasPrint && CLS_VOTE_CS_TIME <= _timeout() )
         {
            _hasPrint = TRUE ;
            PD_LOG( PDEVENT, "%s Vote: Wait for up to %u seconds or until all nodes started, "
                    "then begin to vote. You can change the wait time by modifying the "
                    "parameter 'startshifttime'", getScopeName(),
                    ( startShiftTime - (INT32)_timeout() ) / OSS_ONE_SEC ) ;
         }
         next = id() ;
      }
      PD_TRACE_EXIT ( SDB__CLSVSSD_HDTMOUT ) ;
      return ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSVSSD_ACTIVE, "_clsVSSecondary::active" )
   void _clsVSSecondary::active( INT32 &next )
   {
      PD_TRACE_ENTRY ( SDB__CLSVSSD_ACTIVE ) ;
      _timeout() = 0 ;
      _hasPrint = FALSE ;

      if ( _info()->groupSize() == 1 && 0 == _info()->maintenanceSize() )
      {
         if ( !isLocation() && g_startShiftTime > 0 )
         {
            g_startShiftTime = -1 ;
            PD_LOG( PDEVENT, "%s Vote: Group has only 1 node, disable startup grace period",
                    getScopeName() ) ;
         }
         next = CLS_ELECTION_STATUS_VOTE ;
      }
      else
      {
         next = id() ;
      }
      PD_TRACE_EXIT ( SDB__CLSVSSD_ACTIVE ) ;
      return ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSVSSD_FORCEACTIVE, "_clsVSSecondary::forceActive" )
   void _clsVSSecondary::forceActive ( INT32 & next )
   {
      PD_TRACE_ENTRY ( SDB__CLSVSSD_FORCEACTIVE ) ;
      _timeout() = 0 ;
      _hasPrint = FALSE ;
      next = id() ;
      PD_TRACE_EXIT ( SDB__CLSVSSD_FORCEACTIVE ) ;
      return ;
   }

}
