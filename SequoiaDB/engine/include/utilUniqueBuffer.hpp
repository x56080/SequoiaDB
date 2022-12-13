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

   Source File Name = utilUniqueBuffer.hpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#ifndef UTIL_UNIQUE_BUFFER_HPP__
#define UTIL_UNIQUE_BUFFER_HPP__

#include "utilSlice.hpp"

namespace engine
{
   class _utilUniqueBuffer : public SDBObject
   {
      public:
         _utilUniqueBuffer() = default ;
         ~_utilUniqueBuffer() ;
         _utilUniqueBuffer( _utilUniqueBuffer && ) noexcept ;
         _utilUniqueBuffer &operator=( _utilUniqueBuffer && ) noexcept ;
         _utilUniqueBuffer( const _utilUniqueBuffer & ) = delete ;
         _utilUniqueBuffer &operator=( const _utilUniqueBuffer & ) = delete ;
         explicit operator bool() const noexcept
         {
            return nullptr != _buffer ;
         }

      private:
         explicit _utilUniqueBuffer(UINT32 size, CHAR *buf) noexcept ;

      public:
         static _utilUniqueBuffer allocate( UINT32 size ) ;    

      public:

         OSS_INLINE BOOLEAN isValid() const { return nullptr != _buffer ; }
         void reset() ;

         /// detach buffer from buffer obj!
         CHAR *release() ;

         INT32 realloc( UINT32 size ) ;

         void swap( _utilUniqueBuffer &buf ) ;

         CHAR *getByOffset( UINT32 offset ) ;

         void setBuffer(CHAR v=0x0);

         OSS_INLINE CHAR *get() { return _buffer ; }
         OSS_INLINE const CHAR *get() const { return _buffer ; }
         OSS_INLINE UINT32 getSize() const { return _size ; }
         OSS_INLINE utilSlice getSlice() const { return utilSlice(_size, _buffer) ;}

      private:
         CHAR *_buffer = nullptr ;
         UINT32 _size = 0 ;
   };//class _utilUniqueBuffer
   using utilUniqueBuffer = class _utilUniqueBuffer ;
} // namespace engine


#endif//UTIL_UNIQUE_BUFFER_HPP__
