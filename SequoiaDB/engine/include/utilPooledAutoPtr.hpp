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

   Source File Name = utilPooledAutoPtr.hpp

   Descriptive Name = Operating System Services Header

   When/how to use: this program may be used on binary and text-formatted
   versions of OSS component. This file contains functions for OSS operations.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          04/13/2019  XJH  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef UTIL_POOLED_AUTO_PTR_HPP__
#define UTIL_POOLED_AUTO_PTR_HPP__

#include "ossTypes.hpp"
#include "ossAtomicBase.hpp"
#include "utilMemListPool.hpp"

namespace engine
{

   /*
      UTIL_ALLOC_TYPE define
   */
   enum UTIL_ALLOC_TYPE
   {
      ALLOC_OSS = 0,
      ALLOC_POOL,
      ALLOC_TC
   } ;

   /*
      _utilPooledAutoPtr define
   */
   class _utilPooledAutoPtr
   {
      public:
         _utilPooledAutoPtr() ;
         _utilPooledAutoPtr( const _utilPooledAutoPtr &rhs ) ;
         ~_utilPooledAutoPtr() ;

         _utilPooledAutoPtr& operator= ( const _utilPooledAutoPtr &rhs ) ;
         bool operator! () const { return get() ? false : true ; }

         operator bool () { return get() ? true : false ; }
         operator CHAR* () { return get () ; }
         operator BOOLEAN () { return get() ? TRUE : FALSE ; }
         operator const CHAR* () { return get() ; }

      public:
         static _utilPooledAutoPtr alloc( UINT32 size,
                                          const CHAR *pFile,
                                          UINT32 line,
                                          UTIL_ALLOC_TYPE type = ALLOC_TC ) ;

         static _utilPooledAutoPtr alloc( UINT32 size,
                                          UTIL_ALLOC_TYPE type = ALLOC_TC ) ;

         static _utilPooledAutoPtr make( CHAR *ptr,
                                         UTIL_ALLOC_TYPE type = ALLOC_TC ) ;

      private:
         _utilPooledAutoPtr( CHAR *ptr, UTIL_ALLOC_TYPE type = ALLOC_TC ) ;

      public:
         CHAR*       get() ;
         const CHAR* get() const ;
         INT32       refCount() const ;
         void        release() ;

      private:
         CHAR                 *_ptr ;
         INT32                *_pRef ;
         UTIL_ALLOC_TYPE      _allocType ;
   } ;
   typedef _utilPooledAutoPtr utilPooledAutoPtr ;

   /*
      utilSharePtr define
   */
   template < typename T >
   class utilSharePtr
   {
      public:
         utilSharePtr() ;
         utilSharePtr( const utilSharePtr &rhs ) ;
         ~utilSharePtr() ;

         utilSharePtr& operator= ( const utilSharePtr &rhs ) ;
         bool operator! () const { return get() ? false : true ; }
         T* operator->() { return get() ; }
         const T* operator->() const { return get() ; }

         operator bool () { return get() ? true : false ; }
         operator T* () { return get () ; }
         operator BOOLEAN () { return get() ? TRUE : FALSE ; }
         operator const T* () { return get() ; }

      public:
         static utilSharePtr alloc( const CHAR *pFile,
                                    UINT32 line,
                                    UTIL_ALLOC_TYPE type = ALLOC_TC ) ;

         static utilSharePtr alloc( UTIL_ALLOC_TYPE type = ALLOC_TC ) ;

         static utilSharePtr make( T *ptr,
                                   UTIL_ALLOC_TYPE type = ALLOC_TC ) ;

      public:
         T*          get() { return _ptr ; }
         const T*    get() const { return _ptr ; }
         INT32       refCount() const { return _pRef ? *_pRef : 0 ; }
         void        release() ;

      private:
         utilSharePtr( T *ptr, UTIL_ALLOC_TYPE type = ALLOC_TC ) ;

      private:
         T                    *_ptr ;
         INT32                *_pRef ;
         UTIL_ALLOC_TYPE      _allocType ;

   } ;

   template< typename T >
   utilSharePtr<T>::utilSharePtr()
   {
      _ptr = NULL ;
      _pRef = NULL ;
      _allocType = ALLOC_TC ;
   }

   template< typename T >
   utilSharePtr<T>::utilSharePtr( const utilSharePtr &rhs )
   {
      _ptr = rhs._ptr ;
      _pRef = rhs._pRef ;
      _allocType = rhs._allocType ;
      if ( _pRef )
      {
         INT32 orgRef = ossFetchAndIncrement32( _pRef ) ;
         SDB_ASSERT( orgRef >= 0, "Ref is invlaid" ) ;
         SDB_UNUSED( orgRef ) ;
      }
   }

   template< typename T >
   utilSharePtr<T>::utilSharePtr( T *ptr, UTIL_ALLOC_TYPE type )
   {
      _ptr = NULL ;
      _pRef = NULL ;
      _allocType = type ;
      if ( ptr )
      {
         /// create ref
         if ( ALLOC_OSS == _allocType )
         {
            _pRef = ( INT32* )SDB_OSS_MALLOC( sizeof( INT32 ) ) ;
         }
         else if ( ALLOC_POOL == _allocType )
         {
            _pRef = ( INT32* )SDB_POOL_ALLOC( sizeof( INT32 ) ) ;
         }
         else
         {
            _pRef = ( INT32* )SDB_THREAD_ALLOC( sizeof( INT32 ) ) ;
         }

         if ( !_pRef )
         {
            throw std::bad_alloc() ;
         }

         *_pRef = 1 ;
         _ptr = ptr ;
      }
   }

   template< typename T >
   utilSharePtr<T>::~utilSharePtr()
   {
      release() ;
   }

   template< typename T >
   utilSharePtr<T>& utilSharePtr<T>::operator= ( const utilSharePtr &rhs )
   {
      release() ;
      _ptr = rhs._ptr ;
      _pRef = rhs._pRef ;
      _allocType = rhs._allocType ;
      if ( _pRef )
      {
         INT32 orgRef = ossFetchAndIncrement32( _pRef ) ;
         SDB_ASSERT( orgRef >= 0, "Ref is invlaid" ) ;
         SDB_UNUSED( orgRef ) ;
      }
      return *this ;
   }

   template< typename T >
   utilSharePtr<T> utilSharePtr<T>::alloc( const CHAR *pFile,
                                           UINT32 line,
                                           UTIL_ALLOC_TYPE type )
   {
      utilSharePtr<T> recordPtr ;
      UINT32 realSZ = sizeof( T ) + sizeof( INT32 ) ;
      CHAR *ptr = NULL ;

      if ( ALLOC_OSS == type )
      {
         ptr = ( CHAR* )ossMemAlloc( realSZ, pFile, line ) ;
      }
      else if ( ALLOC_POOL == type )
      {
         ptr = ( CHAR* )utilPoolAlloc( realSZ, pFile, line ) ;
      }
      else
      {
         ptr = ( CHAR* )utilThreadAlloc( realSZ, pFile, line ) ;
      }

      if ( ptr )
      {
         *(INT32*)ptr = 1 ;
         recordPtr._pRef = (INT32*)ptr ;
         recordPtr._ptr = (T*)( ptr + sizeof( INT32 ) ) ;
         recordPtr._allocType = type ;
         new ( recordPtr._ptr ) T () ;
      }
      return recordPtr ;
   }

   template< typename T >
   utilSharePtr<T> utilSharePtr<T>::alloc( UTIL_ALLOC_TYPE type )
   {
      return alloc( __FILE__, __LINE__, type ) ;
   }

   template< typename T >
   utilSharePtr<T> utilSharePtr<T>::make( T *ptr,
                                          UTIL_ALLOC_TYPE type )
   {
      utilSharePtr<T> recordPtr ;

      if ( ptr )
      {
         /// create ref
         if ( ALLOC_OSS == type )
         {
            recordPtr._pRef = ( INT32* )SDB_OSS_MALLOC( sizeof( INT32 ) ) ;
         }
         else if ( ALLOC_POOL == type )
         {
            recordPtr._pRef = ( INT32* )SDB_POOL_ALLOC( sizeof( INT32 ) ) ;
         }
         else
         {
            recordPtr._pRef = ( INT32* )SDB_THREAD_ALLOC( sizeof( INT32 ) ) ;
         }

         if ( recordPtr._pRef )
         {
            *(recordPtr._pRef) = 1 ;
            recordPtr._ptr = ptr ;
            recordPtr._allocType = type ;
         }
      }
      return recordPtr ;
   }

   template< typename T >
   void utilSharePtr<T>::release()
   {
      if ( _pRef )
      {
         INT32 orgRef = ossFetchAndDecrement32( _pRef ) ;
         SDB_ASSERT( orgRef >= 1, "Ref is invlaid" ) ;
         if ( 1 == orgRef )
         {
            if ( (CHAR*)_ptr - sizeof( INT32 ) == ( CHAR* )_pRef )
            {
               _ptr->~T() ;
            }
            else
            {
               SDB_OSS_DEL( _ptr ) ;
            }
            _ptr = NULL ;

            if ( ALLOC_OSS == _allocType )
            {
               SDB_OSS_FREE( _pRef ) ;
            }
            else if ( ALLOC_POOL == _allocType )
            {
               SDB_POOL_FREE( _pRef ) ;
            }
            else
            {
               SDB_THREAD_FREE( _pRef ) ;
            }
            _pRef = NULL ;
         }
      }
   }

}

#endif // UTIL_POOLED_AUTO_PTR_HPP__

