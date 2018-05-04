/*******************************************************************************


   Copyright (C) 2011-2018 SequoiaDB Ltd.

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

   http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.

   Source File Name = msgBuffer.cpp

   Descriptive Name =

   When/how to use: this program may be used on binary and text-formatted
   versions of PMD component. This file contains functions for agent processing.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          01/27/2015  LZ  Initial Draft

   Last Changed =

*******************************************************************************/
#include "msgBuffer.hpp"
#include "util.hpp"

INT32 _msgBuffer::alloc( const UINT32 size )
{
   INT32 rc = SDB_OK ;

   if( 0 ==  size )
   {
      rc = SDB_INVALIDARG ;
      goto error ;
   }

   _data = ( CHAR *) SDB_OSS_MALLOC( size ) ;
   if ( NULL == _data )
   {
      rc = SDB_OOM ;
      goto error ;
   }

   _capacity = size ;

done:
   return rc ;
error:
   goto done ;
}

INT32 _msgBuffer::realloc( const UINT32 size )
{
   INT32 rc = SDB_OK ;
   CHAR* ptr = NULL ;
   if ( 0 == size )
   {
      rc = SDB_INVALIDARG ;
      goto error ;
   }

   if ( size <= _capacity )
   {
      // do nothing
      goto done ;
   }

   ptr = ( CHAR * )SDB_OSS_REALLOC( _data, size ) ;
   if ( NULL == ptr )
   {
      rc = SDB_OOM ;
      goto error ;
   }

   _data = ptr ;
   _capacity = size ;
   ossMemset( _data + _size, 0, _capacity - _size ) ;

done:
   return rc ;
error:
   goto done ;
}

INT32 _msgBuffer::write( const CHAR *in, const UINT32 inLen,
                         BOOLEAN align , INT32 bytes )
{
   INT32 rc   = SDB_OK ;
   INT32 size = 0 ;        // new size to realloc
   INT32 num  = 0 ;        // number of memory block

   if ( NULL == in )
   {
      rc = SDB_INVALIDARG ;
      goto error ;
   }

   if( inLen > _capacity - _size )
   {
      // digit size of memory needed
      num = ( inLen + _size ) / MEMERY_BLOCK_SIZE + 1 ;
      size = num * MEMERY_BLOCK_SIZE ;

      rc = realloc( size ) ;
      if( SDB_OK != rc )
      {
         goto error ;
      }
   }

   ossMemcpy( _data + _size, in, inLen ) ;
   if ( align )
   {
      _size += ossRoundUpToMultipleX( inLen, bytes );
   }
   else
   {
      _size += inLen ;
   }

done:
   return rc ;
error:
   goto done ;
}

INT32 _msgBuffer::write( const bson::BSONObj &obj, BOOLEAN align, INT32 bytes )
{
   INT32 rc   = SDB_OK ;
   INT32 size = 0 ;        // new size to realloc
   INT32 num  = 0 ;        // number of memory block
   UINT32 objsize = obj.objsize() ;
   if( objsize > _capacity - _size )
   {
      // digit size of memory needed
      num = ( objsize + _size ) / MEMERY_BLOCK_SIZE + 1 ;
      size = num * MEMERY_BLOCK_SIZE ;

      rc = realloc( size ) ;
      if( SDB_OK != rc )
      {
         goto error ;
      }
   }

   ossMemcpy( _data + _size, obj.objdata(), objsize ) ;
   if ( align )
   {
      _size += ossRoundUpToMultipleX( objsize, bytes ) ;
   }
   else
   {
      _size += objsize ;
   }

done:
   return rc ;
error:
   goto done ;
}

INT32 _msgBuffer::advance( const UINT32 pos )
{
   INT32 rc = SDB_OK ;

   if ( pos > _capacity )
   {
      rc = SDB_INVALIDARG ;
      goto error ;
   }

   _size = pos ;

done:
   return rc ;
error:
   goto done ;
}

// int main( int argc, char** argv)
// {
//    std::string str = "abcdef" ;
//    msgBuffer buffer ;
//    for ( int i = 0; i < 10; ++i)
//    {
//       str += "eeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeee" ;
//       buffer.write( str.c_str(), str.length() ) ;
//    }
// 
//    std::cout << buffer.data() << std::endl ;
// 
//    return 0 ;
// }

