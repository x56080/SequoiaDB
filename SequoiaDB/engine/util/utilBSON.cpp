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

   Source File Name = utilBSON.cpp

   Descriptive Name = N/A

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who         Description
   ====== =========== =========== =============================================
          23/07/2025  fangjiabin  Initial Draft

   Last Changed =

*******************************************************************************/
#include "utilBSON.hpp"
#include "../bson/bson.hpp"

namespace engine
{
   _utilBSONRawBuilder::_utilBSONRawBuilder()
   : _buffer( NULL ),
     _writePtr( NULL ),
     _buffSize( 0 ),
     _remainSize( 0 ),
     _done( FALSE ),
     _isOutOfBuff( FALSE )
   {
   }

   _utilBSONRawBuilder::~_utilBSONRawBuilder()
   {
   }

   INT32 _utilBSONRawBuilder::start( CHAR *buffer, INT32 buffSize, const BSONObj *baseObj )
   {
      INT32 rc = SDB_OK ;
      SDB_ASSERT( buffer && (buffSize > 5 ), "Buffer not valid " ) ;

      _buffer = buffer ;
      _writePtr = buffer ;
      _buffSize = buffSize ;
      _remainSize = buffSize ;
      _done = FALSE ;
      _isOutOfBuff = FALSE ;

      if ( baseObj )
      {
         INT32 copySize = baseObj->objsize() - 1 ;    // Leave the EOO, it will be added in done.
         if ( buffSize < baseObj->objsize() )
         {
            rc = SDB_SYS ;
            _isOutOfBuff = TRUE ;
            goto error ;
         }

         ossMemcpy( _buffer, baseObj->objdata(), copySize ) ;
         _writePtr = _buffer + copySize ;
         _remainSize = buffSize - copySize ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _utilBSONRawBuilder::appendElement( BSONType type, const CHAR *name, INT32 nameLen,
                                             const CHAR *value, INT32 valueLen )
   {
      INT32 rc = SDB_OK ;
      INT32 len = 1 + nameLen + 1 + valueLen ;  // 1 byte for the null terminating, 1 byte for the type.

      if ( _done )
      {
         SDB_ASSERT( FALSE, "The building has been done" ) ;
         rc = SDB_SYS ;
         goto error ;
      }

      if ( _writePtr == _buffer )
      {
         // Just start, reserve the first 4 bytes for the length of the object.
         _writePtr += sizeof(INT32) ;
         _remainSize -= sizeof(INT32) ;
      }

      if ( len > _remainSize )
      {
         SDB_ASSERT( FALSE, "Buffer size too small" ) ;
         rc = SDB_SYS ;
         _isOutOfBuff = TRUE ;
         PD_LOG( PDWARNING, "Buffer for building the record is too small, rc: %d", rc ) ;
         goto error ;
      }

      // Write the type.
      *_writePtr = (CHAR)type ;
      ++_writePtr ;

      // Write the field name.
      ossMemcpy( _writePtr, name, nameLen ) ;
      _writePtr += nameLen ;
      *_writePtr = '\0' ;
      ++_writePtr ;

      // Write the value.
      if ( value && valueLen > 0 )
      {
         ossMemcpy( _writePtr, value, valueLen ) ;
         _writePtr += valueLen ;
      }

      _remainSize -= len ;

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _utilBSONRawBuilder::done( BOOLEAN &isEmpty )
   {
      INT32 rc = SDB_OK ;

      if ( _writePtr == _buffer )
      {
         isEmpty = TRUE ;
         goto done ;
      }

      // Append the EOO.
      if ( _remainSize <= 0 )
      {
         rc = SDB_SYS ;
         _isOutOfBuff = TRUE ;
         PD_LOG( PDWARNING, "Buffer for building the record is too small, rc: %d", rc ) ;
         goto error ;
      }

      *_writePtr = (CHAR)EOO ;
      ++_writePtr ;
      *(INT32 *)_buffer = _writePtr - _buffer ;

#ifdef _DEBUG
      // Check if the generated record is valid.
      if ( !_validate() )
      {
         SDB_ASSERT( FALSE, "The record is invalid" ) ;
      }
#endif /* _DEBUG */

      _done = TRUE ;
      isEmpty = FALSE ;

   done:
      return rc ;
   error:
      goto done ;
   }

   void _utilBSONRawBuilder::reset()
   {
      _buffer = NULL ;
      _writePtr = NULL ;
      _buffSize = 0 ;
      _remainSize = 0 ;
      _done = FALSE ;
      _isOutOfBuff = FALSE ;
   }

   INT32 _utilBSONRawBuilder::dataSize() const
   {
      return (INT32)( _writePtr - _buffer ) ;
   }

   BOOLEAN _utilBSONRawBuilder::isOutOfBuff() const
   {
      return _isOutOfBuff ;
   }

   BOOLEAN _utilBSONRawBuilder::_validate() const
   {
      BOOLEAN result = TRUE ;
      try
      {
         BSONObj record( _buffer ) ;
         if ( !record.valid() )
         {
            result = FALSE ;
         }
      }
      catch ( std::exception &e )
      {
         result = FALSE ;
      }

      return result ;
   }
}
