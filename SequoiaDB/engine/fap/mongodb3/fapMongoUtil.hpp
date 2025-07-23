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

   Source File Name = msgBuffer.hpp

   Descriptive Name =

   When/how to use: this program may be used on binary and text-formatted
   versions of PMD component. This file contains functions for agent processing.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who         Description
   ====== =========== =========== ========================================
          11/04/2021  fangjiabin  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef _SDB_MSG_BUFFER_HPP_
#define _SDB_MSG_BUFFER_HPP_

#include "oss.hpp"
#include "ossUtil.hpp"
#include "ossMem.hpp"
#include "../../bson/bson.hpp"
#include "utilCommon.hpp"
#include "fapMongoMessageDef.hpp"
#include "fapMongoMessage.hpp"

#define MEMERY_BLOCK_SIZE 4096

namespace fap
{
class _msgBuffer : public SDBObject
{
public:
   _msgBuffer() : _data( NULL ), _size( 0 ), _capacity( 0 )
   {
      alloc( MEMERY_BLOCK_SIZE ) ;
   }

   ~_msgBuffer()
   {
      if ( NULL != _data )
      {
         SDB_OSS_FREE( _data ) ;
         _data = NULL ;
      }

      //_size = 0 ;
      //_capacity = 0 ;
   }

   BOOLEAN empty() const
   {
      return 0 == _size ;
   }

   INT32 write( const CHAR *in, const UINT32 inLen,
                BOOLEAN align = FALSE, INT32 bytes = 4 ) ;

   INT32 write( const BSONObj &obj,
                BOOLEAN align = FALSE, INT32 bytes = 4 ) ;

   INT32 read( CHAR* in, const UINT32 len ) ;

   INT32 advance( const UINT32 pos ) ;

   void zero()
   {
      ossMemset( _data, 0, _capacity ) ;
      _size = 0 ;
   }

   CHAR *data() const
   {
      return _data ;
   }

   const UINT32 size() const
   {
      return _size ;
   }

   const UINT32 capacity() const
   {
      return _capacity ;
   }

   void reserve( const UINT32 size )
   {
      if ( size < _capacity )
      {
         return ;
      }

      realloc( size ) ;
   }

   void doneLen()
   {
      *(SINT32 *)_data = _size ;
   }

private:
   INT32 alloc( const UINT32 size ) ;
   INT32 realloc( const UINT32 size ) ;

private:
   CHAR  *_data ;
   UINT32 _size ;
   UINT32 _capacity ;
} ;
typedef _msgBuffer msgBuffer ;

class _fapMongoErrorObjAssit : public SDBObject
{
public:
   _fapMongoErrorObjAssit() ;

   ~_fapMongoErrorObjAssit(){}

   BSONObj getErrorObj( INT32 errorCode )
   {
      return _errorObjsArray[ SDB_MAX_ERROR + errorCode ] ;
   }

private:
   BSONObj _errorObjsArray[ SDB_MAX_ERROR + SDB_MAX_WARNING + 1 ] ;
};
typedef _fapMongoErrorObjAssit fapMongoErrorObjAssit ;

INT32 fapMongoGenerateNewRecord( const BSONObj &matcher,
                                 const BSONObj &updatorObj,
                                 BSONObj &target ) ;

BSONObj fapMongoGetErrorBson( INT32 errorCode ) ;

CHAR* fapMongoGetOOMErrResHeader() ;

BOOLEAN fapMongoCheckBigEndian() ;

}
#endif
