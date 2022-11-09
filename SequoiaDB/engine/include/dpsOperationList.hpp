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

   Source File Name = dpsOperationList.hpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#ifndef DPS_OPLIST_HPP__
#define DPS_OPLIST_HPP__

#include "dpsOplistDef.hpp"
#include "dpsOplistNode.hpp"
#include "ossMemPool.hpp"

namespace engine
{
   class _dpsOperationList : public SDBObject
   {
      public:
         _dpsOperationList() = default ;
         ~_dpsOperationList() = default ;
         _dpsOperationList( const _dpsOperationList & ) = delete ;
         _dpsOperationList &operator=( const _dpsOperationList & ) = delete ;
         _dpsOperationList( _dpsOperationList && ) noexcept ;
         _dpsOperationList &operator=( _dpsOperationList && ) noexcept ;

      public:
         OSS_INLINE DPS_OPLIST_STATUS getStatus() const { return _status ; }

         OSS_INLINE BOOLEAN isStart() const { return DPS_OPLIST_STATUS::START == _status ; }

         OSS_INLINE BOOLEAN isBuilding() const { return DPS_OPLIST_STATUS::BUILDING == _status ; }
         
         OSS_INLINE BOOLEAN isCompleted() const { return DPS_OPLIST_STATUS::COMPLETED == _status ; }

         OSS_INLINE BOOLEAN isRollingBack() const
         {
            return DPS_OPLIST_STATUS::ROLLING_BACK == _status ;
         }

         OSS_INLINE UINT32 getSize() const { return _nodes.size() ; }

         OSS_INLINE DPS_LSN_OFFSET getLsnOffset() const
         {
            return _nodes.empty() ? DPS_INVALID_LSN_OFFSET : _nodes.front().getLsnOffset() ;
         }

         OSS_INLINE DPS_LSN getLSN() const
         {
            return _nodes.empty() ? DPS_LSN() : _nodes.front().getLSN() ;
         }

         OSS_INLINE DPS_LSN_OFFSET getCurrentTailLsnOffset() const
         {
            return _nodes.empty() ? DPS_INVALID_LSN_OFFSET : _nodes.back().getLsnOffset() ;
         }

         OSS_INLINE DPS_LSN getCurrentTailLSN() const
         {
            return _nodes.empty() ? DPS_LSN() : _nodes.back().getLSN() ;
         }

         OSS_INLINE void reset()
         {
            _status = DPS_OPLIST_STATUS::START ;
            _nodes.clear() ;
         }

      public:
         INT32 initFromRecords( ossPoolList<utilUniqueBuffer> &&records ) ;

         INT32 append( utilUniqueBuffer &&buffer ) ;

         /// seek unit by node lsn.
         const dpsOplistNode *seek( const DPS_LSN &lsn ) const ;

         BOOLEAN contains( const DPS_LSN &lsn ) const ;

      private:
         using _NODE_CONTAINER = ossPoolList<dpsOplistNode> ;

      public:
         using iterator = _NODE_CONTAINER::const_iterator ;
         iterator begin() const { return _nodes.cbegin() ; }
         iterator end() const { return _nodes.cend() ; }

      private:
         INT32 _appendWhenStart( dpsOplistNode &&unit ) ;
         INT32 _appendWhenBuilding( dpsOplistNode &&unit ) ;
         INT32 _appendWhenRollingBack( dpsOplistNode &&unit ) ;
      
         INT32 _append( dpsOplistNode &&unit ) ;

         INT32 _checkRollbackNode( const dpsOplistNode &unit ) const ;
         INT32 _checkNonheadNode( const dpsOplistNode &unit ) const;

      private:
         DPS_OPLIST_STATUS _status = DPS_OPLIST_STATUS::START ;
         _NODE_CONTAINER _nodes ;
   } ;//class _dpsOperationList

   using dpsOperationList = class _dpsOperationList ;
} // namespace engine


#endif//DPS_OPLIST_HPP__