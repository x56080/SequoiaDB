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

   Source File Name = dmsLobDirectOutBuffer.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          07/31/2014  YW  Initial Draft

   Last Changed =

*******************************************************************************/
#include "dmsLobDirectOutBuffer.hpp"
#include "pmdEDU.hpp"
#include "pd.hpp"
#include "dmsTrace.hpp"

namespace engine
{
   /*
      _dmsLobDirectOutBuffer implement
   */
   _dmsLobDirectOutBuffer::_dmsLobDirectOutBuffer( const void *buf,
                                                   UINT32 size,
                                                   IExecutor *cb )
   :_dmsLobDirectBuffer( cb ),
    _usrBuf( buf ),
    _size( size )
   {
      SDB_ASSERT( NULL != _usrBuf && 0 < size, "impossible" ) ;
   }

   _dmsLobDirectOutBuffer::~_dmsLobDirectOutBuffer()
   {
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMS_LOBDIRECTOUTBUF_GETALIGNEDTUPLE, "_dmsLobDirectOutBuffer::getAlignedTuple" )
   INT32 _dmsLobDirectOutBuffer::getAlignedTuple( tuple &t )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__DMS_LOBDIRECTOUTBUF_GETALIGNEDTUPLE ) ;
      UINT32 aligned = ossRoundUpToMultipleX( _size,
                                              OSS_FILE_DIRECT_IO_ALIGNMENT ) ;
      if ( _bufSize < aligned )
      {
         rc = _extendBuf( aligned ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "Failed to extend buf:%d", rc ) ;
            goto error ;
         }
      }

      ossMemcpy( _buf, _usrBuf, _size ) ;
      if ( _size < aligned )
      {
         ossMemset( ( CHAR * )_buf + _size, '\0',
                    aligned - _size ) ;
      }

      t.buf = _buf ;
      t.size = aligned ;

   done:
      PD_TRACE_EXITRC( SDB__DMS_LOBDIRECTOUTBUF_GETALIGNEDTUPLE, rc ) ;
      return rc ;
   error:
      goto done ;
   }

}

