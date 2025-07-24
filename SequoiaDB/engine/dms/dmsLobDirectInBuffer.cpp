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

   Source File Name = dmsLobDirectInBuffer.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          07/31/2014  YW  Initial Draft

   Last Changed =

*******************************************************************************/
#include "dmsLobDirectInBuffer.hpp"
#include "pmdEDU.hpp"
#include "dmsTrace.hpp"
#include "pd.hpp"

namespace engine
{
   /*
      _dmsLobDirectInBuffer implement
   */
   _dmsLobDirectInBuffer::_dmsLobDirectInBuffer( void *usrBuf,
                                                 UINT32 size,
                                                 UINT32 offset,
                                                 IExecutor *cb )
   :_dmsLobDirectBuffer( cb ),
    _usrBuf( usrBuf ),
    _usrSize( size ),
    _usrOffset( offset )
   {
      SDB_ASSERT( NULL != _usrBuf && 0 < _usrSize, "impossible" ) ;
   }

   _dmsLobDirectInBuffer::~_dmsLobDirectInBuffer()
   {
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMS_LOBDIRECTINBUF_GETALIGNEDTUPLE, "_dmsLobDirectInBuffer::getAlignedTuple" )
   INT32 _dmsLobDirectInBuffer::getAlignedTuple( tuple &t )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__DMS_LOBDIRECTINBUF_GETALIGNEDTUPLE ) ;
      UINT32 newSize = _usrSize ;
      UINT32 newOffset = ossRoundUpToMultipleX( _usrOffset,
                                                OSS_FILE_DIRECT_IO_ALIGNMENT ) ;
      if ( _usrOffset != newOffset )
      {
         newOffset -= OSS_FILE_DIRECT_IO_ALIGNMENT ;
      }

      SDB_ASSERT( newOffset <= _usrOffset, "impossible" ) ;
      newSize += ( _usrOffset - newOffset ) ;
      newSize = ossRoundUpToMultipleX( newSize,
                                       OSS_FILE_DIRECT_IO_ALIGNMENT ) ;
      if ( _bufSize < newSize )
      {
         rc = _extendBuf( newSize ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "Failed to extend buf:%d", rc ) ;
            goto error ;
         }
      }

      t.buf = _buf ;
      t.size = newSize ;
      t.offset = newOffset ;

   done:
      PD_TRACE_EXITRC( SDB__DMS_LOBDIRECTINBUF_GETALIGNEDTUPLE, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMS_LOBDIRECTINBUF_CP2USRBUF, "_dmsLobDirectInBuffer::copy2UsrBuf" )
   void _dmsLobDirectInBuffer::copy2UsrBuf( const tuple &t )
   {
      PD_TRACE_ENTRY( SDB__DMS_LOBDIRECTINBUF_CP2USRBUF ) ;
      SDB_ASSERT( t.offset <= _usrOffset &&
                  _usrSize <= t.size - ( _usrOffset - t.offset ) &&
                  NULL != t.buf, "impossible" ) ;
      ossMemcpy( _usrBuf,
                 ( const CHAR * )_buf + ( _usrOffset - t.offset ),
                 _usrSize ) ;

      PD_TRACE_EXIT( SDB__DMS_LOBDIRECTINBUF_CP2USRBUF ) ;
   }
}

