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

   Source File Name = ossAllocator.hpp

   Descriptive Name = Operating System Services Header

   When/how to use: this program may be used on binary and text-formatted
   versions of OSS component. This file contains functions for OSS operations.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/09/2019  XJH  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef OSS_ALLOCATOR_HPP__
#define OSS_ALLOCATOR_HPP__

#include "ossUtil.hpp"

#pragma warning( disable: 4200 )

namespace engine
{

   /*
      _utilPooledAllocator define
   */
   template < typename T >
   class _ossAllocator : public std::allocator<T>
   {
      public:
         typedef typename std::allocator<T>::size_type         size_type ;
         typedef typename std::allocator<T>::pointer           pointer ;
         typedef typename std::allocator<T>::value_type        value_type ;
         typedef typename std::allocator<T>::const_pointer     const_pointer ;
         typedef typename std::allocator<T>::reference         reference ;
         typedef typename std::allocator<T>::const_reference   const_reference ;

      public:
         _ossAllocator()
         {
         }

         _ossAllocator( const _ossAllocator &rhs )
         {
         }

         template < typename _T >
         _ossAllocator( const _ossAllocator<_T > &rhs )
         {
         }

         ~_ossAllocator()
         {
         }

         pointer allocate( size_type count, const void* pHint = NULL )
         {
            ossSignalShield shield ;
            shield.doNothing() ;
            return (pointer)malloc( count * sizeof( value_type ) ) ;
         }

         void deallocate( pointer ptr, size_type count )
         {
            ossSignalShield shield ;
            shield.doNothing() ;
            free( ptr ) ;
         }

         template < typename _Other >
         struct rebind
         {
            // convert this type to allocator<_Other>
            typedef _ossAllocator<_Other> other ;
         } ;

   } ;

}

#endif // OSS_ALLOCATOR_HPP__

