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
#include "clsReplAgent.hpp"
#include "pdTrace.hpp"
#include "clsTrace.hpp"

namespace engine
{

   _clsVSPrimary::_clsVSPrimary( ICLSReplAgent *replAgent )
   : _clsVoteStatus( replAgent, CLS_ELECTION_STATUS_PRIMARY )
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
      MsgRouteID newPrimaryRID, oldPrimaryRID ;

      _replAgent->beforePrimaryDeactive() ;

      _info()->mtx.lock_w() ;

      oldPrimaryRID = _info()->primary ;
      if ( _info()->local.value == _info()->primary.value )
      {
         _info()->primary.value = MSG_INVALID_ROUTEID ;
      }
      newPrimaryRID = _info()->primary ;

      _replAgent->onPrimaryDeactive( newPrimaryRID, oldPrimaryRID ) ;

      _info()->mtx.release_w() ;

      _replAgent->afterPrimaryDeactive( newPrimaryRID, oldPrimaryRID ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSVSPMY_ACTIVE, "_clsVSPrimary::active" )
   void _clsVSPrimary::active( INT32 &next )
   {
      PD_TRACE_ENTRY ( SDB__CLSVSPMY_ACTIVE ) ;

      MsgRouteID newPrimaryRID, oldPrimaryRID ;

      _timeout() = 0 ;
      next = id() ;

      _replAgent->beforePrimaryActive() ;

      _info()->mtx.lock_w() ;

      oldPrimaryRID = _info()->primary ;
      _info()->primary = _info()->local ;
      newPrimaryRID = _info()->primary ;

      _replAgent->onPrimaryActive( newPrimaryRID, oldPrimaryRID ) ;

      _info()->mtx.release_w() ;

      _replAgent->reelectionDone() ;

      PD_LOG ( PDEVENT, "Change to Primary" ) ;

      _replAgent->afterPrimaryActive( newPrimaryRID, oldPrimaryRID ) ;

      PD_TRACE_EXIT ( SDB__CLSVSPMY_ACTIVE ) ;
   }

}
