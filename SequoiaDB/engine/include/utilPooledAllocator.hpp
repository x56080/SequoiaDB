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

   Source File Name = utilPooledAllocator.hpp

   Descriptive Name = Operating System Services Header

   When/how to use: this program may be used on binary and text-formatted
   versions of OSS component. This file contains functions for OSS operations.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          05/24/2019  XJH  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef UTIL_POOLED_ALLOCATOR_HPP__
#define UTIL_POOLED_ALLOCATOR_HPP__

#include "ossTypes.hpp"
#include "utilBitmap.hpp"
#include "utilMemBlockPool.hpp"
#include "ossMem.hpp"
#include "pd.hpp"

#pragma warning( disable: 4200 )

namespace engine
{

   #define UTIL_ALLOCATE_DFT_CACHE_SIZE         ( 64 )
   #define UTIL_ALLOCATE_DFT_CACHE_NUM          ( 4 )

   #define UTIL_ALLOCATE_MAX_CACHE_SIZE         ( 512 )
   #define UTIL_ALLOCATE_MAX_CACHE_NUM          ( 16 )

   /*
      _utilTrunkAllocator define
   */
   template < typename T,
              UINT32 cacheSize = UTIL_ALLOCATE_DFT_CACHE_SIZE >
   class _utilTrunkAllocator
   {
      public:

         struct _innerT
         {
            T        _t ;
         } ;

         typedef _utilStackBitmap<cacheSize>       myBitmap ;
         typedef _innerT                           value_type ;
         typedef _innerT*                          pointer ;
         typedef const _innerT*                    const_pointer ;
         typedef UINT32                            size_type ;

      public:
         _utilTrunkAllocator()
         {
            SDB_ASSERT( cacheSize <= UTIL_ALLOCATE_MAX_CACHE_SIZE,
                        "Invalid cacheSize" ) ;
            _ptr = NULL ;
         }

         _utilTrunkAllocator( const _utilTrunkAllocator &rhs )
         {
         }

         template < typename _T, UINT32 _cacheSize >
         _utilTrunkAllocator( const _utilTrunkAllocator< _T, _cacheSize > &rhs )
         {
         }

         ~_utilTrunkAllocator()
         {
            if ( _ptr )
            {
               utilGetGlobalMemPool() ?
                  utilGetGlobalMemPool()->release( (void*&)_ptr ) :
                  SDB_OSS_FREE( (void*)_ptr ) ;
            }
         }

         pointer allocate( size_type count )
         {
            INT32 pos = -1 ;
            pointer ptr = NULL ;

            if ( 1 == count )
            {
               if ( !_ptr )
               {
                  UINT32 size = cacheSize * sizeof( value_type ) ;
                  _ptr = utilGetGlobalMemPool() ?
                           (pointer)utilGetGlobalMemPool()->alloc( size ) :
                           (pointer)SDB_OSS_MALLOC( size ) ;
               }
               if ( _ptr && ( -1 != ( pos = _bitmap.nextFreeBitPos( 0 ) ) ) )
               {
#ifdef _DEBUG
                  SDB_ASSERT( !_bitmap.testBit( pos ), "Invalid bit" ) ;
#endif //_DEBUG
                  _bitmap.setBit( pos ) ;
                  ptr = _ptr + pos ;
               }
            }

            return ptr ;
         }

         void deallocate( pointer ptr, size_type count )
         {
            UINT32 pos = _calcPos( ptr ) ;

#ifdef _DEBUG
            SDB_ASSERT( in( ptr ), "Not in self" ) ;
            SDB_ASSERT( _bitmap.testBit( pos ), "Invalid bit" ) ;
#endif // _DEBUG
            for ( UINT32 i = 0 ; i < count ; ++i )
            {
               _bitmap.clearBit( pos + i ) ;
            }
         }

         BOOLEAN in( const_pointer ptr ) const
         {
            if ( _ptr && ptr >= _ptr && ptr < _ptr + cacheSize )
            {
#ifdef _DEBUG
               SDB_ASSERT( 0 == ( ( (CHAR*)ptr - (CHAR*)_ptr ) %
                                  sizeof(value_type) ),
                           "Invalid ptr" ) ;
#endif // _DEBUG
               return TRUE ;
            }
            return FALSE ;
         }

         BOOLEAN freeSize() const
         {
            return _bitmap.freeSize() ;
         }

         BOOLEAN isFull() const
         {
            return _bitmap.isFull() ;
         }

         BOOLEAN isEmpty() const
         {
            return _bitmap.isEmpty() ;
         }

      protected:
         UINT32 _calcPos( const_pointer ptr ) const
         {
            return ( ptr - _ptr ) ;
         }

      private:
         pointer                    _ptr ;
         myBitmap                   _bitmap ;
   } ;

   /*
      _utilPooledAllocator define
   */
   template < typename T,
              UINT32 cacheSize = UTIL_ALLOCATE_DFT_CACHE_SIZE,
              UINT32 cacheNum = UTIL_ALLOCATE_DFT_CACHE_NUM >
   class _utilPooledAllocator : public std::allocator<T>
   {
      public:
         typedef _utilTrunkAllocator< T, cacheSize >           myCache ;
         typedef UINT32                                        size_type ;
         typedef typename std::allocator<T>::pointer           pointer ;
         typedef typename std::allocator<T>::value_type        value_type ;
         typedef typename std::allocator<T>::const_pointer     const_pointer ;
         typedef typename std::allocator<T>::reference         reference ;
         typedef typename std::allocator<T>::const_reference   const_reference ;

      public:
         _utilPooledAllocator()
         {
            SDB_ASSERT( cacheSize <= UTIL_ALLOCATE_MAX_CACHE_SIZE,
                        "Invalid cacheSize" ) ;
            SDB_ASSERT( cacheNum <= UTIL_ALLOCATE_MAX_CACHE_NUM,
                        "Invalid cacheNum" ) ;
         }

         _utilPooledAllocator( const _utilPooledAllocator &rhs )
         {
         }

         template < typename _T,
                    UINT32 _cacheSize,
                    UINT32 _cacheNum >
         _utilPooledAllocator( const _utilPooledAllocator<_T, _cacheSize, _cacheNum> &rhs )
         {
         }

         ~_utilPooledAllocator()
         {
         }

         pointer allocate( size_type count, const void* pHint = NULL )
         {
            pointer ptr = NULL ;

            if ( 1 == count )
            {
               for ( UINT32 i = 0 ; i < cacheNum ; ++i )
               {
                  if ( NULL != ( ptr = (pointer)_cache[i].allocate( count ) ) )
                  {
                     return ptr ;
                  }
               }
            }

            size_type sz = count * sizeof( value_type ) ;
            ptr = utilGetGlobalMemPool() ?
                     (pointer)utilGetGlobalMemPool()->alloc( sz ) :
                     (pointer)SDB_OSS_MALLOC( sz ) ;

            return ptr ;
         }

         void deallocate( pointer ptr, size_type count )
         {
            for ( UINT32 i = 0 ; i < cacheNum ; ++i )
            {
               if ( _cache[i].in( (typename myCache::const_pointer)ptr ) )
               {
                  _cache[i].deallocate( (typename myCache::pointer)ptr,
                                        count ) ;
                  return ;
               }
            }

            if ( ptr )
            {
               utilGetGlobalMemPool() ?
                  utilGetGlobalMemPool()->release( (void*&)ptr ) :
                  SDB_OSS_FREE( (void*)ptr ) ;
            }
         }

         template < typename _Other,
                    UINT32 _cacheSize = cacheSize,
                    UINT32 _cacheNum = cacheNum >
         struct rebind
         {
            // convert this type to allocator<_Other>
            typedef _utilPooledAllocator<_Other, _cacheSize, _cacheNum> other ;
         } ;

      private:
         myCache                 _cache[ cacheNum ] ;

   } ;

}

#endif // UTIL_POOLED_ALLOCATOR_HPP__

