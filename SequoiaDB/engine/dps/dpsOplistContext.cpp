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

   Source File Name = dpsOplistContext.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#include "dpsOplistContext.hpp"
#include "pdTrace.hpp"
#include "ossLikely.hpp"
#include "dpsTrace.hpp"

namespace engine
{

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DPSOPLCTX_STARTNEWOPL, "_dpsOplistContext::startNewOpl" )
   INT32 _dpsOplistContext::startNewOpl()
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__DPSOPLCTX_STARTNEWOPL ) ;

      if ( OSS_UNLIKELY(_MAX_BUILDING_OPL_NUM == _size) )
      {
         PD_LOG( PDERROR, "no more opl can be started") ;
         rc = SDB_INVALID_OPERATION ;
         goto error ;
      }

      ++_size ;
      _resetCurrentCtx() ;
   done:
      PD_TRACE_EXITRC( SDB__DPSOPLCTX_STARTNEWOPL, rc ) ;
      return rc;
   error:
      goto done ;
   }

   void _dpsOplistContext::_resetCurrentCtx()
   {
      _current = 0 == _size ? nullptr : _bcc.data() + ( _size - 1 ) ;
   }

   BOOLEAN _dpsOplistContext::isFreshOpl() const
   {
      SDB_ASSERT( nullptr != _current, "can not be invalid" ) ;
      return 0 == _current->getSize() ;
   }

   DPS_LSN _dpsOplistContext::getOplLSN() const
   {
      return nullptr == _current ?
             DPS_LSN() : _current->getLSN() ;
   }

   DPS_LSN _dpsOplistContext::getPreNodeLSN() const
   {
      return nullptr == _current ?
             DPS_LSN() : _current->getCurrentTailLSN() ;
   }

   BOOLEAN _dpsOplistContext::isOplRollingBack() const
   {
      SDB_ASSERT( nullptr != _current, "can not be invalid" ) ;
      return DPS_OPLIST_STATUS::ROLLING_BACK == _current->getStatus() ;
   }

   void _dpsOplistContext::setOplRollingBack()
   {
      SDB_ASSERT( nullptr != _current, "can not be invalid" ) ;
      _current->beginToRollBack() ;
   }

   void _dpsOplistContext::push( const DPS_LSN &lsn )
   {
      SDB_ASSERT( nullptr != _current, "start opl first" ) ;
      SDB_ASSERT( lsn.isValid(), "can not be invalid" ) ;
      _current->push( lsn ) ;
      return ;
   }

   void _dpsOplistContext::completeOpl( dpsOplBuildingCtx *result )
   {
      SDB_ASSERT( nullptr != _current, "start opl first" ) ;
      _current->setCompleted() ;
      if ( nullptr != result )
      {
         *result = *_current ;
      }
      _current->reset() ;
      --_size ;
      _resetCurrentCtx() ;
      return ;
   }

   void _dpsOplistContext::terminateFreshOpl()
   {
      SDB_ASSERT( nullptr != _current, "start opl first" ) ;
      SDB_ASSERT( DPS_OPLIST_STATUS::START == _current->getStatus(), "unexpected status") ;
      _current->reset() ;
      --_size ;
      _resetCurrentCtx() ;
      return ;
   }

   void _dpsOplistContext::reset()
   {
      _current = nullptr ;
      for ( UINT32 i = 0; i < _size; ++i )
      {
         _bcc[i].reset() ;
      }
      _size = 0 ;
      return ;
   }
} // namespace engine
