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

   Source File Name = utilStrictBuffer.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

*******************************************************************************/
#include "utilStrictBuffer.hpp"
#include "ossLikely.hpp"

namespace engine
{

   /*
      _utilStrictBuffer implement
    */
   void _utilStrictBuffer::setBuffer( CHAR v )
   {
      SDB_ASSERT( isWritable(), "must be writable" ) ;
      ossMemset( _wptr, v, _size ) ;
   }

   void _utilStrictBuffer::setBuffer(UINT32 offset, UINT32 size, CHAR v)
   {
      SDB_ASSERT( isWritable(), "must be writable" ) ;
      ossMemset( _wptr + offset, v, size ) ;
   }

   INT32 _utilStrictBuffer::write( UINT32 offset, UINT32 size, const void *data )
   {
      INT32 rc = SDB_OK ;

      if ( OSS_UNLIKELY( !isWritable() ) )
      {
         rc = SDB_OPERATION_CONFLICT ;
         goto error;
      }
      else if ( OSS_UNLIKELY( 0 == size || NULL == data ) )
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }
      else if ( !_isValidAccessing( offset, size ) )
      {
         rc = SDB_OUT_OF_BOUND ;
         goto error ;
      }

      ossMemcpy( _wptr + offset, data, size ) ;

   done:
      return rc ;

   error:
      goto done ;
   }

   INT32 _utilStrictBuffer::read( UINT32 offset, UINT32 size, void *data ) const
   {
      INT32 rc = SDB_OK ;

      if ( OSS_UNLIKELY( !isValid() ) )
      {
         rc = SDB_OPERATION_CONFLICT ;
         goto error ;
      }
      else if ( OSS_UNLIKELY( 0 == size || NULL == data ) )
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }
      else if ( !_isValidAccessing( offset, size ) )
      {
         rc = SDB_OUT_OF_BOUND ;
         goto error ;
      }

      ossMemcpy( data, _rptr + offset, size ) ;

   done:
      return rc ;

   error:
      goto done ;
   }

   utilSlice _utilStrictBuffer::getSlice() const
   {
      SDB_ASSERT( isValid(), "can not be invalid" ) ;
      return utilSlice( _size, _rptr ) ;
   }

   utilSlice _utilStrictBuffer::getSlice( UINT32 offset, UINT32 size ) const
   {
      SDB_ASSERT( isValid(), "can not be invalid" ) ;
      return _isValidAccessing( offset, size ) ? utilSlice( size, _rptr + offset ) : utilSlice() ;
   }

   _utilStrictBuffer _utilStrictBuffer::getReadableBuffer () const
   {
      SDB_ASSERT(isValid(), "can not be invalid");
      return _utilStrictBuffer( _size, _rptr ) ;
   }

   _utilStrictBuffer _utilStrictBuffer::getReadableBuffer( UINT32 size, UINT32 offset ) const
   {
      SDB_ASSERT( isValid(), "can not be invalid" ) ;
      return  _isValidAccessing( offset, size ) ? _utilStrictBuffer( size, _rptr + offset ) : _utilStrictBuffer() ;
   }

   _utilStrictBuffer _utilStrictBuffer::getWritableBuffer( UINT32 size, UINT32 offset )
   {
      SDB_ASSERT( isValid(), "can not be invalid" ) ;
      _utilStrictBuffer buffer ;
      if ( isWritable() && _isValidAccessing( offset, size ) )
      {
         buffer.makeWritable( size, _wptr + offset ) ;
      }
      return buffer ;
   }

}
