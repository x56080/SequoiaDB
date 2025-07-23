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

#include "utilMemListPool.hpp"
#include <typeinfo>
#include "pd.hpp"

#pragma warning( disable: 4200 )

extern BOOLEAN ossMemDebugEnabled ;

namespace engine
{

   /*
      _utilPooledAllocator define
   */
   template < typename T >
   class _utilPooledAllocator : public std::allocator<T>
   {
      public:
         typedef typename std::allocator<T>::size_type         size_type ;
         typedef typename std::allocator<T>::pointer           pointer ;
         typedef typename std::allocator<T>::value_type        value_type ;
         typedef typename std::allocator<T>::const_pointer     const_pointer ;
         typedef typename std::allocator<T>::reference         reference ;
         typedef typename std::allocator<T>::const_reference   const_reference ;

      public:
         _utilPooledAllocator()
         {
         }

         _utilPooledAllocator( const _utilPooledAllocator &rhs )
         {
         }

         template < typename _T >
         _utilPooledAllocator( const _utilPooledAllocator<_T > &rhs )
         {
         }

         ~_utilPooledAllocator()
         {
         }

         pointer allocate( size_type count, const void* pHint = NULL )
         {
            if ( !ossMemDebugEnabled )
            {
               pointer p = (pointer)SDB_THREAD_ALLOC( count *
                                                      sizeof( value_type ) ) ;
               if ( !p )
               {
                  throw std::bad_alloc() ;
               }
               return p ;
            }
            else
            {
               UINT16 hash = 0 ;
               static const CHAR *pCharName = typeid( char ).name() ;
               const CHAR *pIDName = typeid( value_type ).name() ;

               if ( pIDName == pCharName )
               {
                  pIDName = NULL ;
               }
               else
               {
                  hash = (UINT16)ossHashFileName( pIDName ) ;
               }

               pointer p = (pointer)utilThreadAlloc( count *
                                                     sizeof( value_type ),
                                                     __FILE__,
                                                     pIDName ? hash : __LINE__,
                                                     NULL,
                                                     pIDName ) ;
               if ( !p )
               {
                  throw std::bad_alloc() ;
               }
               return p ;
            }
         }

         void deallocate( pointer ptr, size_type count )
         {
            SDB_THREAD_FREE( ptr ) ;
         }

         template < typename _Other >
         struct rebind
         {
            // convert this type to allocator<_Other>
            typedef _utilPooledAllocator<_Other> other ;
         } ;

   } ;

}

#endif // UTIL_POOLED_ALLOCATOR_HPP__

