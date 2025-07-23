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

   Source File Name = utilSHMBuffer.hpp

   Descriptive Name = share memory

   When/how to use: this program may be used on binary and text-formatted
   versions of PMD component.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          07/30/2019  HGM Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef UTIL_SHM_BUFFER_HPP__
#define UTIL_SHM_BUFFER_HPP__

#include "oss.hpp"
#include "ossUtil.hpp"
#include "ossShMem.hpp"
#include "ossMemPool.hpp"

class utilSHMBuffer
{
public:
   utilSHMBuffer()
   : _buffer( NULL ),
     _size( 0 ),
     _attachOnly( FALSE ),
#if defined (_LINUX)
     _shmID( 0 ),
     _shmKey( 0 )
#elif defined (_WINDOWS)
     _shmID( NULL ),
     _shmKey( NULL )
#endif
   {
   }

   ~utilSHMBuffer()
   {
      release() ;
   }

   CHAR *getBuffer()
   {
      return _buffer ;
   }

   UINT32 getSize()
   {
      return _size ;
   }

   const CHAR *getKeyString() const
   {
      return _shmKeyString.c_str() ;
   }

   ossSHMKey getKey()
   {
      return _shmKey ;
   }

   ossSHMMid getID()
   {
      return _shmID ;
   }

   // allocate will create if shared memory does not exists
   BOOLEAN allocate( const CHAR *key, UINT32 size )
   {
      return _acquire( key, size, FALSE ) ;
   }

   // attach will not create if shared memory does not exists
   BOOLEAN attach( const CHAR *key, UINT32 size )
   {
      return _acquire( key, size, TRUE ) ;
   }

   void release()
   {
      if ( NULL == _buffer )
      {
         return ;
      }
      else if ( _attachOnly )
      {
         ossSHMDetach( _shmID, &_buffer ) ;
      }
      else
      {
         ossSHMFree( _shmID, &_buffer ) ;
      }
      _clear() ;
   }

protected:
   BOOLEAN _acquire( const CHAR *key, UINT32 size, BOOLEAN attach )
   {
      // check if acquiring the same buffer
      if ( _isSameBuffer( key, size ) )
      {
         return TRUE ;
      }

      // prepare key and size
      if ( !_prepareAcquire( key, size ) )
      {
         return FALSE ;
      }

      // try release old buffer
      release () ;

      // acquire buffer
      if ( attach )
      {
         _buffer = ossSHMAttach( _shmKey, _size, _shmID ) ;
      }
      else
      {
         _buffer = ossSHMAttach( _shmKey, _size, _shmID ) ;
         if ( NULL == _buffer )
         {
            _buffer = ossSHMAlloc( _shmKey, _size, OSS_SHM_CREATE, _shmID ) ;
         }
      }

      // check if succeed
      if ( NULL == _buffer )
      {
         _clear() ;
         return FALSE ;
      }

      _attachOnly = attach ;

      return TRUE ;
   }

   void _clear()
   {
      _buffer = NULL ;
      _size = 0 ;
      _attachOnly = FALSE ;
#if defined (_LINUX)
      _shmID = 0 ;
      _shmKey = 0 ;
#elif defined (_WINDOWS)
      _shmID = NULL ;
      _shmKey = NULL ;
#endif
      _shmKeyString.clear() ;
   }

   BOOLEAN _prepareAcquire( const CHAR *key, UINT32 size )
   {
      // validate key and size
      if ( NULL == key || 0 == size )
      {
         return FALSE ;
      }

      _shmKeyString = key ;
#if defined (_LINUX)
      _shmKey = ossAtoi( _shmKeyString.c_str() ) ;
#elif defined (_WINDOWS)
      _shmKey = _shmKeyString.c_str() ;
#endif
      _size = size ;

      return TRUE ;
   }

   BOOLEAN _isSameBuffer( const CHAR *key, UINT32 size )
   {
      // buffer should be valid, key and size should be the same
      return ( NULL != _buffer &&
               NULL != key && 0 == ossStrcmp( _shmKeyString.c_str(), key ) &&
               0 != size && _size == size ) ;
   }

   BOOLEAN _isValidBuffer()
   {
#if defined (_LINUX)
      return ( NULL != _buffer && _shmID >= 0 ) ;
#elif defined (_WINDOWS)
      return ( NULL != _buffer && _shmID != NULL ) ;
#endif
   }

protected:
   CHAR *         _buffer ;
   UINT32         _size ;
   BOOLEAN        _attachOnly ;
   ossSHMMid      _shmID ;
   ossSHMKey      _shmKey ;
   ossPoolString  _shmKeyString ;
} ;

#endif // UTIL_SHM_BUFFER_HPP__
