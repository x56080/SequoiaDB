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

   Source File Name = dpsOplBuildingCtx.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

*******************************************************************************/
#include "dpsOplBuildingCtx.hpp"
#include "pdTrace.hpp"
#include "ossLikely.hpp"

namespace engine
{
   void _dpsOplBuildingCtx::reset()
   {
      _lsn.reset() ;
      _currentTail.reset() ;
      _size = 0 ;
      _status = DPS_OPLIST_STATUS::START ;
      return ;
   }

   void _dpsOplBuildingCtx::push( const DPS_LSN &node )
   {
      SDB_ASSERT( !node.invalid(), "can not be invalid" ) ;
      if ( OSS_UNLIKELY(DPS_OPLIST_STATUS::COMPLETED == _status) )
      {
         SDB_ASSERT( FALSE, "invalid operation" ) ;
      }
      else if ( DPS_OPLIST_STATUS::START == _status )
      {
         _lsn = node ;
         _currentTail = node ;
         _status = DPS_OPLIST_STATUS::BUILDING ;
         ++_size ;
      }
      else
      {
         SDB_ASSERT( _currentTail.compareOffset( node ) < 0,
                     "the new lsn should be over current tail lsn" ) ;
         _currentTail = node ;
         ++_size ;
      }

      return ;
   }

   void _dpsOplBuildingCtx::beginToRollBack()
   {
      SDB_ASSERT( DPS_OPLIST_STATUS::BUILDING == _status, "invalid status" ) ;
      _status = DPS_OPLIST_STATUS::ROLLING_BACK ;
   }

   void _dpsOplBuildingCtx::setCompleted()
   {
      SDB_ASSERT( DPS_OPLIST_STATUS::BUILDING == _status ||
                  DPS_OPLIST_STATUS::ROLLING_BACK == _status, "invalid status" ) ;
      _status = DPS_OPLIST_STATUS::COMPLETED ;
   }
} // namespace engine
