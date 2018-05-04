/*******************************************************************************

   Copyright (C) 2011-2018 SequoiaDB Ltd.

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

   http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.

   Source File Name = clsVSPrimary.cpp

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

#include "clsVSPrimary.hpp"
#include "pmd.hpp"
#include "pmdCB.hpp"
#include "pdTrace.hpp"
#include "clsTrace.hpp"

namespace engine
{
   #define CLS_PRIMARY_UP_NOTIFY_TIMES          ( 60 )

   _clsVSPrimary::_clsVSPrimary( _clsGroupInfo *info,
                                  _netRouteAgent *agent ):
                                 _clsVoteStatus( info, agent,
                                               CLS_ELECTION_STATUS_PRIMARY)
   {

   }

   _clsVSPrimary::~_clsVSPrimary()
   {

   }

   INT32 _clsVSPrimary::handleInput( const MsgHeader *header,
                                     INT32 &next )
   {
      /// primary do not accept any request
      next = id() ;
      return SDB_OK ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSVSPMY_HDTMOUT, "_clsVSPrimary::handleTimeout" )
   void _clsVSPrimary::handleTimeout( const UINT32 &millisec,
                                      INT32 &next )
   {
      PD_TRACE_ENTRY ( SDB__CLSVSPMY_HDTMOUT ) ;
      _timeout() += millisec ;
      if ( CLS_VOTE_CS_TIME <= _timeout() )
      {
         if ( CLS_IS_MAJORITY( _info()->alives.size() + 1,
                               _info()->groupSize() ) )
         {
            _timeout() = 0 ;
            next = id() ;
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
      PD_TRACE_EXIT ( SDB__CLSVSPMY_HDTMOUT ) ;
      return ;
   }

   void _clsVSPrimary::deactive ()
   {
      _MsgCatPrimaryChange msg ;

      // primary change before
      sdbGetClsCB()->ntyPrimaryChange( FALSE, SDB_EVT_OCCUR_BEFORE ) ;

      _info()->mtx.lock_w() ;
      if ( _info()->local.value == _info()->primary.value )
      {
         _info()->primary.value = MSG_INVALID_ROUTEID ;
      }
      pmdSetPrimary( FALSE ) ; // set global primary
      msg.newPrimary = _info()->primary ;
      msg.oldPrimary = _info()->local ;
      _info()->mtx.release_w() ;

      // primary change after
      sdbGetClsCB()->ntyPrimaryChange( FALSE, SDB_EVT_OCCUR_AFTER ) ;

      sdbGetReplCB()->callCatalog( (MsgHeader *)&msg ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSVSPMY_ACTIVE, "_clsVSPrimary::active" )
   void _clsVSPrimary::active( INT32 &next )
   {
      PD_TRACE_ENTRY ( SDB__CLSVSPMY_ACTIVE ) ;
      _timeout() = 0 ;
      next = id() ;
      _MsgCatPrimaryChange msg ;

      // before primary
      sdbGetClsCB()->ntyPrimaryChange( TRUE, SDB_EVT_OCCUR_BEFORE ) ;

      _info()->mtx.lock_w() ;
      msg.newPrimary = _info()->local ;
      msg.oldPrimary = _info()->primary ;
      _info()->primary = _info()->local ;
      pmdSetPrimary( TRUE ) ; // set global primary
      _info()->mtx.release_w() ;

      sdbGetReplCB()->reelectionDone() ;

      PD_LOG ( PDEVENT, "Change to Primary" ) ;

      // after primary
      sdbGetClsCB()->ntyPrimaryChange( TRUE, SDB_EVT_OCCUR_AFTER ) ;

      sdbGetReplCB()->callCatalog( (MsgHeader *)&msg,
                                   CLS_PRIMARY_UP_NOTIFY_TIMES ) ;

      PD_TRACE_EXIT ( SDB__CLSVSPMY_ACTIVE ) ;
      return ;
   }

}
