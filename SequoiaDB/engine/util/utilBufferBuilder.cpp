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

   Source File Name = utilBufferBuilder.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#include "utilBufferBuilder.hpp"
#include "pdTrace.hpp"
#include "ossLikely.hpp"

namespace engine
{
   _utilBufferBuilder::_utilBufferBuilder( const options &o ) noexcept :
   _o(o)
   {

   }

   void _utilBufferBuilder::reset()
   {
      _o = options() ;
      _size = 0 ;
      _buf.reset() ;
      return ;
   }

   void _utilBufferBuilder::restart()
   {
      _size = 0;
      return ;
   }

   void _utilBufferBuilder::resetBuf()
   {
      _size = 0 ;
      _buf.reset() ;
      return ;
   }

   utilUniqueBuffer _utilBufferBuilder::release( UINT32 *size )
   {
      if ( nullptr != size )
      {
         *size = _size ;
      }
      _size = 0 ;
      return std::move( _buf ) ;
   }

   INT32 _utilBufferBuilder::append( UINT32 size, const void *data )
   {
      INT32 rc = SDB_OK ;
      if ( OSS_UNLIKELY( 0 == size || nullptr == data ) )
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }
      else
      {
         CHAR *buf = _ensureBuf( size ) ;
         if ( OSS_UNLIKELY(nullptr == buf) )
         {
            rc = SDB_OOM ;
            goto error ;
         }

         ossMemcpy( buf, data, size ) ;
         _incSize( size ) ;
      }
   done:
      return rc ;
   error:
      goto done ;
   }

   CHAR *_utilBufferBuilder::_ensureBuf( UINT32 size )
   {
      SDB_ASSERT( 0 < size, "can not be invalid" ) ;
      CHAR *buf = nullptr ;

      if ( _getFreeBufSize() < size )
      {
         UINT32 bufSize = 0 == _buf.getSize() ? _o.defaultBufSize : _buf.getSize() << 1 ;
         if ( bufSize < ( _size + size ) )
         {
            constexpr UINT32 _EXTRA_BUF_SIZE = 128 ;
            bufSize = _size + size + _EXTRA_BUF_SIZE ;
         }

         INT32 rc = _buf.realloc( bufSize ) ;
         if ( OSS_UNLIKELY( SDB_OK != rc ) )
         {
            PD_LOG( PDERROR, "failed to reallocate buf:%d", rc ) ;
            goto done ;
         }
      }

      buf = _getBufToWrite() ;
   done:
      return buf ;
   }

   INT32 _utilBufferBuilder::reserve( UINT32 size )
   {
      INT32 rc = SDB_OK ;
      if ( OSS_UNLIKELY(0 == size) )
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }
      else
      {
         CHAR *buf = _ensureBuf( size ) ;
         if ( OSS_UNLIKELY(nullptr == buf) )
         {
            rc = SDB_OOM ;
            goto error;
         }
      }
   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _utilBufferBuilder::skip( UINT32 size )
   {
      INT32 rc = SDB_OK ;
      if ( OSS_UNLIKELY(0 == size) )
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }
      else
      {
         CHAR *buf = _ensureBuf( size ) ;
         if ( OSS_UNLIKELY(nullptr == buf) )
         {
            rc = SDB_OOM ;
            goto error;
         }

         _incSize( size ) ;
      }
   done:
      return rc ;
   error:
      goto done ;
   }

   void _utilBufferBuilder::shrink( UINT32 size )
   {
      SDB_ASSERT( size <= _size, "out of bound" ) ;
      _size = size ;
   }

   CHAR *_utilBufferBuilder::getSlice( UINT32 offset, UINT32 size )
   {
      UINT32 maxOffset = offset + size ;
      SDB_ASSERT( maxOffset <= _size, "out of bound" ) ;
      return _buf.getByOffset( offset ) ;
   }
} // namespace engine
