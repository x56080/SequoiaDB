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

#include "clsVSSilence.hpp"
#include "pd.hpp"
#include "pdTrace.hpp"
#include "clsTrace.hpp"
#include "pmd.hpp"

namespace engine
{

   _clsVSSilence::_clsVSSilence( _clsGroupInfo *info,
                                 _netRouteAgent *agent )
   :_clsVoteStatus( info, agent, CLS_ELECTION_STATUS_SILENCE )
   {

   }

   _clsVSSilence::~_clsVSSilence()
   {

   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSVSSL_HDINPUT, "_clsVSSilence::handleInput" )
   INT32 _clsVSSilence::handleInput( const MsgHeader *header,
                                     INT32 &next )
   {
      PD_TRACE_ENTRY ( SDB__CLSVSSL_HDINPUT ) ;
      PD_LOG( PDDEBUG, "vote: silence status, ignore msg" ) ;
      next = id() ;
      PD_TRACE_EXIT ( SDB__CLSVSSL_HDINPUT ) ;
      return SDB_OK ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSVSSL_HDTMOUT, "_clsVSSilence::handleTimeout" )
   void _clsVSSilence::handleTimeout( const UINT32 &millisec,
                                      INT32 &next )
   {
      PD_TRACE_ENTRY ( SDB__CLSVSSL_HDTMOUT ) ;
      _timeout() += millisec ;

      const static UINT32 maxSliceTime = 30 * OSS_ONE_SEC ;
      /// silence time must be higher than brk time.
      if ( pmdGetOptionCB()->sharingBreakTime() + 1000 <= _timeout() ||
           maxSliceTime <= _timeout() )
      {
         next = CLS_ELECTION_STATUS_SEC ;
         goto done ;
      }
      next = id() ;
   done:
      PD_TRACE_EXIT ( SDB__CLSVSSL_HDTMOUT ) ;
      return ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSVSSL_ACTIVE, "_clsVSSilence::active" )
   void _clsVSSilence::active( INT32 &next )
   {
      PD_TRACE_ENTRY ( SDB__CLSVSSL_ACTIVE ) ;
      _timeout() = 0 ;

      if ( _info()->groupSize() == 1 )
      {
         next = CLS_ELECTION_STATUS_SEC ;
      }
      else
      {
         next = id() ;
      }

      PD_TRACE_EXIT ( SDB__CLSVSSL_ACTIVE ) ;
      return ;
   }
}
