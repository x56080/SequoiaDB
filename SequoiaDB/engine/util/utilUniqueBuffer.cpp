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

   Source File Name = utilUniqueBuffer.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#include "utilUniqueBuffer.hpp"
#include "utilMemListPool.hpp"
#include "pdTrace.hpp"
#include "ossLikely.hpp"

namespace engine
{
   _utilUniqueBuffer::~_utilUniqueBuffer()
   {
      if ( nullptr != _buffer )
      {
         SDB_THREAD_FREE( _buffer ) ;
      }
   }

   _utilUniqueBuffer::_utilUniqueBuffer( _utilUniqueBuffer &&o ) noexcept :
   _buffer( o._buffer ),
   _size( o._size )
   {
      o._buffer = nullptr ;
      o._size = 0;
   }

   _utilUniqueBuffer &_utilUniqueBuffer::operator=( _utilUniqueBuffer &&o ) noexcept
   {
      reset() ;
      _buffer = o._buffer ;
      _size = o._size ;
      o._buffer = nullptr ;
      o._size = 0 ;
      return *this ;
   }

   _utilUniqueBuffer::_utilUniqueBuffer(UINT32 size, CHAR *buf) noexcept :
   _buffer( buf ),
   _size( size )
   {
      
   }

   void _utilUniqueBuffer::reset()
   {
      if ( nullptr != _buffer )
      {
         SDB_THREAD_FREE( _buffer ) ;
         _buffer = nullptr ;
         _size = 0;
      }
      return ;
   }

   CHAR *_utilUniqueBuffer::release()
   {
      CHAR *buf = _buffer ;
      _buffer = nullptr ;
      _size = 0 ;
      return buf ;
   }

   _utilUniqueBuffer _utilUniqueBuffer::allocate( UINT32 size )
   {
      SDB_ASSERT( 0 < size, "can not be invalid" ) ;
      CHAR *buf = (CHAR *)SDB_THREAD_ALLOC( size ) ;
      if ( OSS_UNLIKELY( nullptr == buf ) )
      {
         PD_LOG(PDERROR, "failed to allocate mem.") ;
         return std::move( _utilUniqueBuffer() ) ;
      }

      return std::move( _utilUniqueBuffer( size, buf ) ) ;
   }

   INT32 _utilUniqueBuffer::realloc( UINT32 size )
   {
      INT32 rc = SDB_OK ;
      SDB_ASSERT( 0 < size, "can not be invalid" ) ;

      if ( OSS_UNLIKELY(nullptr == _buffer) )
      {
         _buffer = (CHAR *)SDB_THREAD_ALLOC( size ) ;
         if ( nullptr == _buffer )
         {
            PD_LOG(PDERROR, "failed to allocate mem." ) ;
            rc = SDB_OOM ;
            goto error;
         }
         _size = size ;
      }
      else
      {
         CHAR *buf = (CHAR *)SDB_THREAD_REALLOC( _buffer, size ) ;
         if ( OSS_UNLIKELY(nullptr == buf) )
         {
            PD_LOG(PDERROR, "failed to allocate mem." ) ;
            rc = SDB_OOM ;
            goto error;
         }

         _buffer = buf ;
         _size = size ;
      }
   done:
      return rc ;
   error: 
      goto done ;
   }

   void _utilUniqueBuffer::swap( _utilUniqueBuffer &o )
   {
      CHAR *buf = _buffer ;
      UINT32 size = _size ;
      _buffer = o._buffer ;
      _size = o._size ;
      o._buffer = buf ;
      o._size = size ;
      return ;
   }

   CHAR *_utilUniqueBuffer::getByOffset( UINT32 offset )
   {
      SDB_ASSERT( offset < _size, "out of bound" ) ;
      return offset < _size ? _buffer + offset : nullptr ;
   }

   void _utilUniqueBuffer::setBuffer(CHAR v)
   {
      if (0 < _size)
      {
         ossMemset(_buffer, v, _size);
      }
      return;
   }
} // namespace engine
