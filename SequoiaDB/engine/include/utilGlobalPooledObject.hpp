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

   Source File Name = utilGlobalPooledObject.hpp

   Descriptive Name = 

   When/how to use: this program may be used on binary and text-formatted
   versions of OSS component. This file contains functions for OSS operations.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef UTIL_GLOBAL_POOLED_OBJECT_HPP_
#define UTIL_GLOBAL_POOLED_OBJECT_HPP_

#pragma warning( disable: 4290 )
#include "utilMemListPool.hpp"
#include "ossMem.hpp"

namespace engine
{
// we should NEVER AND EVER put any virtual function in this object
   // and we should never cast an object to _utilGlobalPooledObject and delete.
   class _utilGlobalPooledObject
   {
   protected :
      // never instantiate SDBObject
      _utilGlobalPooledObject () {}
   public :
   // do NOT put virtual destructor because it will change the inherited object
   // size

      // regular new
      void * operator new ( size_t size ) throw ( const char * )
      {
         void *p = SDB_POOL_ALLOC( size ) ;
         if ( !p ) throw "allocation failure" ;
         return p ;
      }

      void * operator new[] ( size_t size ) throw ( const char * )
      {
         void *p = SDB_POOL_ALLOC( size ) ;
         if ( !p ) throw "allocation failure" ;
         return p ;
      }

      // placement new
      void * operator new ( size_t size, void* p ) throw ( const char * )
      {
         if ( !p ) throw "allocation failure" ;
         return p;
      }

      void * operator new[] ( size_t size, void* p ) throw ( const char * )
      {
         if ( !p ) throw "allocation failure" ;
         return p;
      }

      void operator delete ( void *p )
      {
         SDB_POOL_FREE( p ) ;
      }

      void operator delete[] ( void *p )
      {
         SDB_POOL_FREE( p ) ;
      }

      // placement delete (no-op)
      void operator delete ( void* p , void* p2) throw ()
      {
      }

      void operator delete[] ( void* p, void* p2 ) throw ()
      {
      }

      // new with file/line number
      void * operator new ( size_t size, const CHAR *pFile, UINT32 line )
            throw ( const char * )
      {
         void *p = utilPoolAlloc( size, pFile, line ) ;
         if ( !p ) throw "allocation failure" ;
         return p ;
      }

      void * operator new[] ( size_t size, const CHAR *pFile, UINT32 line )
            throw ( const char * )
      {
         void *p = utilPoolAlloc( size, pFile, line ) ;
         if ( !p ) throw "allocation failure" ;
         return p ;
      }

      void operator delete ( void *p, const CHAR *pFile, UINT32 line )
      {
         SDB_POOL_FREE( p ) ;
      }

      void operator delete[] ( void *p, const CHAR *pFile, UINT32 line )
      {
         SDB_POOL_FREE( p ) ;
      }

      // no throw
      void * operator new ( size_t size, const std::nothrow_t & )
      {
         return SDB_POOL_ALLOC( size ) ;
      }

      void * operator new[] ( size_t size, const std::nothrow_t & )
      {
         return SDB_POOL_ALLOC( size ) ;
      }

      void operator delete ( void *p, const std::nothrow_t & )
      {
         SDB_POOL_FREE(p) ;
      }

      void operator delete[] ( void *p, const std::nothrow_t & )
      {
         SDB_POOL_FREE(p) ;
      }

      // no throw with line number
      void * operator new ( size_t size, const CHAR *pFile,
                            UINT32 line, const std::nothrow_t & )
      {
         return utilPoolAlloc( size, pFile, line ) ;
      }

      void * operator new[] ( size_t size, const CHAR *pFile,
                              UINT32 line, const std::nothrow_t & )
      {
         return utilPoolAlloc( size, pFile, line ) ;
      }

      void operator delete ( void *p, const CHAR *pFile,
                             UINT32 line, const std::nothrow_t & )
      {
         SDB_POOL_FREE(p) ;
      }

      void operator delete[] ( void *p, const CHAR *pFile,
                               UINT32 line, const std::nothrow_t & )
      {
         SDB_POOL_FREE(p) ;
      }
   } ;
   typedef class _utilGlobalPooledObject utilGlobalPooledObject ;
}

#endif//UTIL_GLOBAL_POOLED_OBJECT_HPP_