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

   Source File Name = utilStreamAllocator.hpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          11/20/2023  HGM Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef UTIL_STREAM_ALLOCATOR_HPP_
#define UTIL_STREAM_ALLOCATOR_HPP_

#include "ossMem.hpp"
#include "ossUtil.hpp"
#include "utilMemListPool.hpp"

#define UTIL_STREAM_ALLOCATOR_SIZE ( 512 )

namespace engine
{

   /*
      _utilStreamAllocatorBase define
    */
   class _utilStreamAllocatorBase : public SDBObject
   {
   public:
      _utilStreamAllocatorBase() = default ;
      virtual ~_utilStreamAllocatorBase() = default ;
      _utilStreamAllocatorBase( const _utilStreamAllocatorBase & ) = delete ;
      _utilStreamAllocatorBase &operator =( const _utilStreamAllocatorBase & ) = delete ;
   public:
      virtual void *malloc( size_t size ) = 0 ;
      virtual void free( void *p ) = 0 ;
      virtual void *realloc( void *p, size_t size ) = 0 ;
      virtual BOOLEAN isMovable() const = 0 ;
      virtual BOOLEAN isMovable( const void *p ) const = 0 ;
      virtual UINT32 getFastAllocSize() const { return 0 ; }
   } ;

   typedef class _utilStreamAllocatorBase utilStreamAllocator ;

   /*
      _utilStreamPoolAllocator define
    */
   class _utilStreamPoolAllocator : public _utilStreamAllocatorBase
   {
   public:
      _utilStreamPoolAllocator() = default ;
      virtual ~_utilStreamPoolAllocator() = default ;
      _utilStreamPoolAllocator( const _utilStreamPoolAllocator & ) = delete ;
      _utilStreamPoolAllocator &operator =( const _utilStreamPoolAllocator & ) = delete ;

      virtual void *malloc( size_t size ) override
      {
         return SDB_THREAD_ALLOC( size ) ;
      }

      virtual void *realloc( void *p, size_t size ) override
      {
         void *ptr = nullptr ;
         if ( nullptr == p )
         {
            ptr = _utilStreamPoolAllocator::malloc( size ) ;
         }
         else
         {
            ptr = SDB_THREAD_REALLOC( p, size ) ;
         }
         return ptr ;
      }

      virtual void free( void *p ) override
      {
         SDB_THREAD_FREE( p ) ;
      }

      virtual BOOLEAN isMovable() const override
      {
         return TRUE ;
      }

      virtual BOOLEAN isMovable( const void *p ) const override
      {
         return TRUE ;
      }
   } ;

   typedef class _utilStreamPoolAllocator utilStreamPoolAllocator ;

   /*
      _utilStreamStackAllocator define
    */
   template<UINT32 STACK_SIZE = 512>
   class _utilStreamStackAllocator : public _utilStreamAllocatorBase
   {
   public:
      _utilStreamStackAllocator() = default ;
      virtual ~_utilStreamStackAllocator() = default ;
      _utilStreamStackAllocator( const _utilStreamStackAllocator & ) = delete ;
      _utilStreamStackAllocator &operator =( const _utilStreamStackAllocator & ) = delete ;

      virtual void *malloc( size_t size ) override
      {
         void *buf = nullptr ;
         if ( 0 == _offset && size <= STACK_SIZE )
         {
            buf = _statckBuf ;
            _offset = size ;
         }
         else
         {
            buf = _poolAllocator.malloc( size ) ;
         }
         return buf ;
      }

      virtual void *realloc( void *p, size_t size ) override
      {
         void *buf = nullptr ;
         if ( nullptr == p )
         {
            buf = _utilStreamStackAllocator::malloc( size ) ;
         }
         else if ( isStackBuffer( p ) )
         {
            if ( size <= STACK_SIZE )
            {
               buf = _statckBuf ;
               _offset = size ;
            }
            else
            {
               buf = _poolAllocator.malloc( size ) ;
               if ( nullptr != buf )
               {
                  ossMemcpy( buf, _statckBuf, _offset ) ;
                  _offset = 0 ;
               }
            }
         }
         else
         {
            buf = _poolAllocator.realloc( p, size ) ;
         }
         return buf ;
      }

      virtual void free( void *p ) override
      {
         if ( nullptr != p )
         {
            if ( isStackBuffer( p ) )
            {
               _offset = 0 ;
            }
            else
            {
               _poolAllocator.free( p ) ;
            }
         }
      }

      BOOLEAN isStackBuffer( const void *p ) const
      {
         return _statckBuf == (const CHAR *)p ;
      }

      virtual BOOLEAN isMovable() const override
      {
         return FALSE ;
      }

      virtual BOOLEAN isMovable( const void *p ) const override
      {
         return !isStackBuffer( p ) ;
      }

      UINT32 getMaxStackBufSize() const
      {
         return STACK_SIZE ;
      }

      virtual UINT32 getFastAllocSize() const override
      {
         return STACK_SIZE ;
      }

   private:
      CHAR _statckBuf[ STACK_SIZE ] = {} ;
      UINT32 _offset = 0 ;
      utilStreamPoolAllocator _poolAllocator ;
   } ;

   typedef class _utilStreamStackAllocator<> utilStreamStackAllocator ;

}

#endif // UTIL_STREAM_ALLOCATOR_HPP_