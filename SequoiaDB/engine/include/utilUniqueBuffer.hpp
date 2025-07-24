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

   Source File Name = utilUniqueBuffer.hpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

*******************************************************************************/
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
