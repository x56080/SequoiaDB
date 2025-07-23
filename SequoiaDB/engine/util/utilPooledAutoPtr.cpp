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

   Source File Name = utilPooledAutoPtr.cpp

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
#include "utilPooledAutoPtr.hpp"
#include "utilMemBlockPool.hpp"
#include "ossAtomicBase.hpp"
#include "ossMem.hpp"

namespace engine
{

   /*
      _utilPooledAutoPtr implement
   */
   _utilPooledAutoPtr::_utilPooledAutoPtr()
   {
      _ptr = NULL ;
   }

   _utilPooledAutoPtr::_utilPooledAutoPtr( CHAR *ptr )
   {
      _ptr = ptr ;
      if ( _ptr )
      {
         INT32 orgRef = ossFetchAndIncrement32( _refPtr() ) ;
         SDB_ASSERT( orgRef >= 0, "Ref is invlaid" ) ;
         SDB_UNUSED( orgRef ) ;
      }
   }

   _utilPooledAutoPtr::_utilPooledAutoPtr( const _utilPooledAutoPtr &rhs )
   {
      _ptr = rhs._ptr ;
      if ( _ptr )
      {
         INT32 orgRef = ossFetchAndIncrement32( _refPtr() ) ;
         SDB_ASSERT( orgRef >= 0, "Ref is invlaid" ) ;
         SDB_UNUSED( orgRef ) ;
      }
   }

   _utilPooledAutoPtr::~_utilPooledAutoPtr()
   {
      release() ;
   }

   _utilPooledAutoPtr& _utilPooledAutoPtr::operator= ( const _utilPooledAutoPtr &rhs )
   {
      release() ;
      _ptr = rhs._ptr ;
      if ( _ptr )
      {
         INT32 orgRef = ossFetchAndIncrement32( _refPtr() ) ;
         SDB_ASSERT( orgRef >= 0, "Ref is invlaid" ) ;
         SDB_UNUSED( orgRef ) ;
      }
      return *this ;
   }

   _utilPooledAutoPtr _utilPooledAutoPtr::alloc( UINT32 size )
   {
      _utilPooledAutoPtr recordPtr ;
      if ( size > 0 )
      {
         UINT32 realSZ = size + sizeof( INT32 ) ;
         CHAR *ptr = NULL ;
      
         ptr = ( CHAR* )( utilGetGlobalMemPool() ?
                              utilGetGlobalMemPool()->alloc( realSZ ) :
                              SDB_OSS_MALLOC( realSZ ) ) ;
         if ( ptr )
         {
            *(INT32*)ptr = 1 ;
            recordPtr._ptr = ptr ;
         }
      }
      return recordPtr ;
   }

   CHAR* _utilPooledAutoPtr::get()
   {
      return _ptr ? _ptr + sizeof( INT32 ) : NULL ;
   }

   const CHAR* _utilPooledAutoPtr::get() const
   {
      return _ptr ? _ptr + sizeof( INT32 ) : NULL ;
   }

   INT32 _utilPooledAutoPtr::refCount() const
   {
      return _ptr ? *((INT32*)_ptr) : 0 ;
   }

   INT32* _utilPooledAutoPtr::_refPtr()
   {
      return _ptr ? (INT32*)_ptr : NULL ;
   }

   void _utilPooledAutoPtr::release()
   {
      if ( _ptr )
      {
         INT32 orgRef = ossFetchAndDecrement32( _refPtr() ) ;
         SDB_ASSERT( orgRef >= 1, "Ref is invlaid" ) ;
         if ( 1 == orgRef )
         {
            utilGetGlobalMemPool() ? 
               utilGetGlobalMemPool()->release( (void*&)_ptr ) :
               SDB_OSS_FREE( (void*)_ptr ) ;
            _ptr = NULL ;
         }
      }
   }

}

