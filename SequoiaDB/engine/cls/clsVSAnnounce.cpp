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

   Source File Name = clsVSAnnounce.cpp

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
#include "clsVSAnnounce.hpp"
#include "pdTrace.hpp"
#include "clsTrace.hpp"

namespace engine
{
   _clsVSAnnounce::_clsVSAnnounce( _clsGroupInfo *info, _netRouteAgent *agent )
   :_clsVoteStatus( info, agent, CLS_ELECTION_STATUS_ANNOUNCE )
   {

   }

   _clsVSAnnounce::~_clsVSAnnounce()
   {

   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSVSANN_HDINPUT, "_clsVSAnnounce::handleInput" )
   INT32 _clsVSAnnounce::handleInput( const MsgHeader *header,
                                      INT32 &next )
   {
      PD_TRACE_ENTRY ( SDB__CLSVSANN_HDINPUT ) ;
      SDB_ASSERT( NULL != header, "header should not be NULL" ) ;

      /// do not accept any ballot
      if ( MSG_CLS_BALLOT == header->opCode )
      {
         next = id() ;
      }
      else if ( MSG_CLS_BALLOT_RES == header->opCode )
      {
         const _clsSharingStatus *pStatus = NULL ;
         const _MsgClsElectionRes *msg = ( const _MsgClsElectionRes * ) header ;

         map<UINT64, _clsSharingStatus>::iterator itr = _info()->info.find( msg->identity.value ) ;
         if ( itr != _info()->info.end() )
         {
            pStatus = &( itr->second ) ;
         }

         if ( CLS_ELECTION_ROUND_STAGE_ONE == msg->round || !pStatus )
         {
            next = id() ;
         }
         else
         {
            if ( SDB_OK == msg->header.res )
            {
               // If this accepted msg is from critical node, record it
               if ( pStatus->isInCriticalMode() )
               {
                  ++_criticalAccepted() ;
               }

               // If this accepted msg is from maintenance node, ignore it
               if ( ! pStatus->isInMaintenanceMode() )
               {
                  ++_accepted() ;
               }

               // Node in normal mode, not critical mode
               if ( _info()->groupSize() <= ( _accepted() + 1 ) )
               {
                  next =  CLS_ELECTION_STATUS_PRIMARY;
                  PD_LOG( PDEVENT, "%s Vote: change to primary by all accept", getScopeName() ) ;
               }
               else
               {
                  next = id() ;
               }
            }
            // If is in enforced critical mode, ignore the reject result, wait until timeout
            else if ( SDB_CLS_PRIMARY_ALREADY_EXIST != msg->header.res &&
                      !isLocation() &&
                      _info()->grpMode.enforced &&
                      CLS_GROUP_MODE_CRITICAL == _info()->localGrpMode &&
                      !pStatus->isInCriticalMode() )
            {
               next = id() ;
            }
            else
            {
               next = CLS_ELECTION_STATUS_SILENCE ;
            }
         }
      }
      else
      {
         /// error msg
         next = id() ;
      }
      PD_TRACE_EXIT ( SDB__CLSVSANN_HDINPUT ) ;
      return SDB_OK ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSVSANN_HDTMOUT, "_clsVSAnnounce::handleTimeout" )
   void _clsVSAnnounce::handleTimeout( const UINT32 &millisec,
                                       INT32 &next )
   {
      PD_TRACE_ENTRY ( SDB__CLSVSANN_HDTMOUT ) ;
      _timeout() += millisec ;
      if ( CLS_VOTE_CS_TIME <= _timeout() )
      {
         if ( _isAccepted() )
         {
            next = CLS_ELECTION_STATUS_PRIMARY ;

            if ( CLS_GROUP_MODE_CRITICAL == _info()->localGrpMode && ! isLocation() )
            {
               if ( _info()->grpMode.enforced )
               {
                  PD_LOG( PDEVENT, "%s Vote: change to primary by timeout "
                          "in enforced critical mode", getScopeName() ) ;
               }
               else
               {
                  PD_LOG( PDEVENT, "%s Vote: change to primary by timeout "
                          "in critical mode", getScopeName() ) ;
               }
            }
            else
            {
               PD_LOG( PDEVENT, "%s Vote: change to primary by timeout", getScopeName() ) ;
            }
         }
         else
         {
            next = CLS_ELECTION_STATUS_SILENCE ;
         }
      }
      else
      {
         next = id() ;
      }
      PD_TRACE_EXIT ( SDB__CLSVSANN_HDTMOUT ) ;
      return ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSVSANN_ACTIVE, "_clsVSAnnounce::active" )
   void _clsVSAnnounce::active( INT32 &next )
   {
      PD_TRACE_ENTRY ( SDB__CLSVSANN_ACTIVE ) ;
      _timeout() = 0 ;
      _accepted() = 0 ;
      _criticalAccepted() = 0 ;

      if ( _info()->groupSize() == 1 && 0 == _info()->maintenanceSize() )
      {
         next = CLS_ELECTION_STATUS_PRIMARY ;
      }
      else
      {
         next = id() ;
         if ( SDB_OK != _announce() )
         {
            next = CLS_ELECTION_STATUS_SILENCE ;
         }
         else if ( _info()->groupSize() == 1 &&
                   CLS_GROUP_MODE_MAINTENANCE != _info()->localGrpMode )
         {
            next = CLS_ELECTION_STATUS_PRIMARY ;
         }
      }
      PD_TRACE_EXIT ( SDB__CLSVSANN_ACTIVE ) ;
      return ;
   }
}
