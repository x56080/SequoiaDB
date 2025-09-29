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

   Source File Name = utilAllocator.hpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          18/04/2017  HGM Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef UTIL_ALLOCATOR_HPP_
#define UTIL_ALLOCATOR_HPP_

#include "core.hpp"
#include "oss.hpp"
#include "ossMem.hpp"
#include "ossUtil.hpp"

using namespace std ;

#define UTIL_ALLOCATOR_SIZE   265

namespace engine
{
   template < UINT32 stackSize = UTIL_ALLOCATOR_SIZE >
   class _utilAllocator
   {
      public :
         _utilAllocator()
         {
            _offset = 0 ;
         }

         virtual ~_utilAllocator()
         {
            _offset = 0 ;
         }

         void* allocate ( size_t size )
         {
            void *p = NULL ;
            if ( _offset + size <= stackSize )
            {
               p = _mem + _offset ;
               _offset += size ;
            }

            return p ;
         }

         BOOLEAN isAllocatedByme ( void *p )
         {
            if ( p >= _mem && p < _mem + stackSize )
            {
               return TRUE ;
            }

            return FALSE ;
         }

      protected :
         char _mem[ stackSize ] ;
         INT32 _offset ;
   } ;
}

#endif // UTIL_ALLOCATOR_HPP_

