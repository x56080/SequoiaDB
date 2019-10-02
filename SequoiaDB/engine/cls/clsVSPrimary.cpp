/*******************************************************************************

   Copyright (C) 2011-2018 SequoiaDB Ltd.

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU Affero General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU Affero General Public License for more details.

   You should have received a copy of the GNU Affero General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.

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
