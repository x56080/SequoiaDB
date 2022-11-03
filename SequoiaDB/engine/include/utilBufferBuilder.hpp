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

   Source File Name = utilBufferBuilder.hpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#ifndef UTIL_BUFFER_BUILDER_HPP__
#define UTIL_BUFFER_BUILDER_HPP__

#include "utilUniqueBuffer.hpp"
#include "ossLikely.hpp"

#include <limits>
#include <type_traits>

namespace engine
{
   class _utilBufferBuilder : public SDBObject
   {
      public:
         struct options
         {
            options() = default ;
            options( UINT32 dbs ):
            defaultBufSize( dbs ) {}
            
            UINT32 defaultBufSize = 1024 ;
         };

      public:
         _utilBufferBuilder() = default ;
         _utilBufferBuilder( const options & ) noexcept ;
         ~_utilBufferBuilder() = default ;

      public:
         void reset() ;
         void restart() ;
         void resetBuf() ;
         utilUniqueBuffer release( UINT32 *size=nullptr ) ;
         OSS_INLINE UINT32 getSize() const { return _size ; }

         ///WARNING: do not cache any buf ptr if it is still building,
         /// the buffer may be reallocated.
         OSS_INLINE const utilUniqueBuffer &getBuf() const { return _buf ; }
         OSS_INLINE utilUniqueBuffer &getBuf() { return _buf ; }

      public:
         INT32 reserve( UINT32 size ) ;

         INT32 skip( UINT32 size ) ;

         void shrink( UINT32 size ) ;

         CHAR *getSlice( UINT32 offset, UINT32 size ) ;

      public:
         template<typename T>
         INT32 appendNumeric( T value ) ;

         INT32 appendUint8( UINT8 value ) { return appendNumeric(value) ; }

         INT32 appendUint16( UINT16 value ) { return appendNumeric(value) ; }

         INT32 appendUint32( UINT32 value ) { return appendNumeric(value) ; }

         INT32 appendUint64( UINT64 value ) { return appendNumeric(value) ; }

         INT32 appendInt8( INT8 value ) { return appendNumeric(value) ; }

         INT32 appendInt16( INT16 value ) { return appendNumeric(value) ; }

         INT32 appendInt32( INT32 value ) { return appendNumeric(value) ; }

         INT32 appendInt64( INT64 value ) { return appendNumeric(value) ; }

         template<class T>
         INT32 appendObj( const T &obj ) ;

         INT32 append( UINT32 size, const void *data ) ;
      
      private:
         CHAR *_ensureBuf( UINT32 size ) ;

         OSS_INLINE UINT32 _getFreeBufSize() const { return _buf.getSize() - _size ; }
         OSS_INLINE CHAR *_getBufToWrite() { return _buf.get() + _size ; }
         OSS_INLINE void _incSize( UINT32 size ) { _size += size ; }
      private:
         options _o ;
         utilUniqueBuffer _buf ;
         UINT32 _size = 0 ;
   } ;
   using utilBufferBuilder = class _utilBufferBuilder ;

   template<typename T>
   INT32 _utilBufferBuilder::appendNumeric(T value)
   {
      INT32 rc = SDB_OK;
      static_assert( std::numeric_limits<T>::is_specialized, "must be numeric" );
      CHAR *buf = _ensureBuf( sizeof(T) ) ;
      if ( OSS_UNLIKELY(nullptr == buf) )
      {
         rc = SDB_OOM ;
         goto error ;
      }

      *reinterpret_cast<T *>(buf) = value ;
      _incSize( sizeof(T) ) ;

   done:
      return rc ;
   error:
      goto done ; 
   }

   template<class T>
   INT32 _utilBufferBuilder::appendObj(const T &obj)
   {
      INT32 rc = SDB_OK;
      static_assert( std::is_standard_layout<T>::value, "must be standard layout" );
      CHAR *buf = _ensureBuf( sizeof(T) ) ;
      if ( OSS_UNLIKELY(nullptr == buf) )
      {
         rc = SDB_OOM ;
         goto error ;
      }

      ossMemcpy( buf, &obj, sizeof(T) ) ;
      _incSize( sizeof(T) ) ;

   done:
      return rc ;
   error:
      goto done ; 
   }
} // namespace engine


#endif//UTIL_BUFFER_BUILDER_HPP__