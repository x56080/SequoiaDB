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

   Source File Name = dpsOplBuildingCtx.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

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
