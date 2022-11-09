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

   Source File Name = dpsOplistNode.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#include "dpsOplistNode.hpp"
#include "pdTrace.hpp"
#include "dpsLogRecordDef.hpp"

namespace engine
{
   _dpsOplistNode::_dpsOplistNode( utilUniqueBuffer &&b ):
   _buf( std::move( b ) )
   {
      if ( !_init( _buf ) )
      {
         reset();
         SDB_ASSERT( FALSE, "invalid data buffer" ) ;
      }
   } 

   _dpsOplistNode::_dpsOplistNode( _dpsOplistNode &&o ) noexcept:
   _rh( o._rh ),
   _rb( std::move( o._rb ) ),
   _buf( std::move( o._buf ) )
   {
      o.reset() ;
   }

   _dpsOplistNode &_dpsOplistNode::operator=( _dpsOplistNode &&o ) noexcept
   {
      _rh = o._rh ;
      _rb = std::move( o._rb ) ;
      _buf = std::move( o._buf ) ;
      o.reset() ;
      return *this ;
   }

   DPS_OPL_NODE_TYPE _dpsOplistNode::getType() const
   {
      SDB_ASSERT( isValid(), "can not be invalid" ) ;
      return isValid() ?
             dpsGetOplNodeType( _rh->_flags ) :
             DPS_OPL_NODE_TYPE::NONE ;
   }

   BOOLEAN _dpsOplistNode::_init( const utilUniqueBuffer &buf )
   {
      BOOLEAN r = FALSE ;
      if ( DPS_LOG_HEAD_SIZE <= buf.getSize() )
      {
         utilSlice s = buf.getSlice() ;
         const dpsLogRecordHeader *h = s.castTo<dpsLogRecordHeader>() ;
         if ( DPS_INVALID_LSN_OFFSET != h->_lsn )
         {
            _rh = h ;
            utilSlice bodySlice = s.getSliceFromOffsetToEnd( DPS_LOG_HEAD_SIZE ) ;
            _rb = std::move( dpsRecordElements( bodySlice.getData(), bodySlice.getSize() ) ) ;
            r = TRUE ;
         }
      }

      return r ;
   }

   BOOLEAN _dpsOplistNode::hasOplNodeInfo() const
   {
      return isValid() && _rb.contains( DPS_LOG_PUBLIC_OPL_NODE ) ;
   }

   BOOLEAN _dpsOplistNode::hasOplRollbackInfo() const
   {
      return isValid() && _rb.contains( DPS_LOG_PUBLIC_OPL_ROLLBACK_INFO ) ;
   }

   const dpsOplNodeEle *_dpsOplistNode::getOplNodeInfo() const
   {
      const dpsOplNodeEle *res = nullptr ;
      dpsRecordElements::iterator itr = _rb.seek( DPS_LOG_PUBLIC_OPL_NODE ) ;
      if ( itr.isValid() )
      {
         res = itr.getValue().castTo<dpsOplNodeEle>() ;
      }

      return res ;
   }

   const dpsOplRollbackInfoEle *_dpsOplistNode::getOplRollbackInfo() const
   {
      const dpsOplRollbackInfoEle *res = nullptr ;
      dpsRecordElements::iterator itr = _rb.seek( DPS_LOG_PUBLIC_OPL_ROLLBACK_INFO ) ;
      if ( itr.isValid() )
      {
         res = itr.getValue().castTo<dpsOplRollbackInfoEle>() ;
      }

      return res ;
   }
} // namespace engine
