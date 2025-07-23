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

   Source File Name = utilBuffBlock.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          20/11/2018  YSD Initial Draft

   Last Changed =

*******************************************************************************/
#include "utilBuffBlock.hpp"
#include "utilTrace.hpp"
#include "pdTrace.hpp"

namespace engine
{
   _utilBuffBlock::_utilBuffBlock( UINT64 size )
   : _buff( NULL ),
     _size( size ),
     _writePos( 0 )
   {
   }

   _utilBuffBlock::~_utilBuffBlock()
   {
      SAFE_OSS_FREE( _buff ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__UTILBUFFBLOCK_INIT, "_utilBuffBlock::init" )
   INT32 _utilBuffBlock::init()
   {
      INT32 rc = SDB_OK;
      PD_TRACE_ENTRY( SDB__UTILBUFFBLOCK_INIT ) ;

      _buff = ( CHAR * )SDB_OSS_MALLOC( _size ) ;
      if ( !_buff )
      {
         rc = SDB_OOM ;
         PD_LOG( PDERROR, "Allocate memory for sort area block failed. "
                          "Requested size: %zd", _size ) ;
         goto error ;
      }

   done:
      PD_TRACE_EXITRC( SDB__UTILBUFFBLOCK_INIT, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__UTILBUFFBLOCK_APPEND, "_utilBuffBlock::append" )
   INT32 _utilBuffBlock::append( const CHAR *data, UINT32 len )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__UTILBUFFBLOCK_APPEND ) ;

      SDB_ASSERT( data, "Data is NULL" ) ;

      if ( !_buff )
      {
         rc = SDB_SYS ;
         PD_LOG( PDERROR, "Sort area block not initialized yet" ) ;
         goto error ;
      }

      if ( freeSize() < len )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG( PDERROR, "Data size too big for the block" ) ;
         goto error ;
      }

      ossMemcpy( _buff + _writePos, data, len ) ;
      _writePos += len ;

   done:
      PD_TRACE_EXITRC( SDB__UTILBUFFBLOCK_APPEND, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__UTILBUFFBLOCK_RESIZE, "_utilBuffBlock::resize" )
   INT32 _utilBuffBlock::resize( UINT64 newSize )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__UTILBUFFBLOCK_RESIZE ) ;
      CHAR *newBuf = NULL ;

      newBuf = ( CHAR * )SDB_OSS_REALLOC( _buff, newSize ) ;
      if ( !newBuf )
      {
         rc = SDB_OOM ;
         PD_LOG( PDERROR, "Reallocate block size from %zd to %zd failed",
                 _size, newSize ) ;
         goto error ;
      }

      _buff = newBuf ;
      _size = newSize ;

   done:
      PD_TRACE_EXITRC( SDB__UTILBUFFBLOCK_RESIZE, rc ) ;
      return rc ;
   error:
      goto done ;
   }
}
