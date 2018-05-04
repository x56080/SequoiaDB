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

   Source File Name = utilStream.cpp

   Descriptive Name = I/O stream API

   When/how to use: this program may be used on binary and text-formatted
   versions of DPS component. This file contains code logic for log page
   operations

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          8/13/2016  David Li  Initial Draft

   Last Changed =

*******************************************************************************/
#include "utilStream.hpp"
#include "ossMem.hpp"
#include "pd.hpp"

namespace engine
{
   utilStream::utilStream()
      : _buf( NULL ),
        _bufSize( 0 )
   {
   }

   utilStream::~utilStream()
   {
      SAFE_OSS_FREE( _buf ) ;
      _bufSize = 0 ;
   }

   INT32 utilStream::init( INT32 bufSize )
   {
      INT32 rc = SDB_OK;
      
      SDB_ASSERT( bufSize > 0, "bufSize <= 0" ) ;

      _bufSize = bufSize ;

      _buf = (CHAR*)SDB_OSS_MALLOC( _bufSize );
      if (NULL == _buf)
      {
         PD_LOG( PDERROR, "Failed to malloc buffer for stream" ) ;
         rc = SDB_OOM;
         goto error;
      }

   done:
      return rc;
   error:
      goto done;
   }

   INT32 utilStream::copy( utilInStream& in, utilOutStream& out,
                           INT64* streamSize, utilStreamInterrupt* si )
   {
      INT32 rc = SDB_OK ;
      INT64 readSize = 0 ;
      INT64 totalSize = 0 ;

      SDB_ASSERT( NULL != _buf, "_buf can't be NULL" ) ;

      for ( ;; )
      {
         if ( NULL != si && si->isInterrupted() )
         {
            rc = SDB_INTERRUPT ;
            goto error ;
         }

         rc = in.read( _buf, _bufSize, readSize ) ;
         if ( SDB_EOF == rc )
         {
            rc = SDB_OK ;
            break ;
         }
         else if ( SDB_OK == rc )
         {
            totalSize += readSize ;

            rc = out.write( _buf, readSize ) ;
            if ( SDB_OK != rc )
            {
               PD_LOG( PDERROR, "Failed to write to outstream, rc=%d", rc ) ;
               goto error ;
            }
         }
         else
         {
            PD_LOG( PDERROR, "Failed to read from instream, rc=%d", rc ) ;
            goto error ;
         }
      }

      if ( NULL != streamSize )
      {
         *streamSize = totalSize ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 utilStream::copy( utilInStream& in, utilOutStream&out, INT64 size,
                           INT64* streamSize, utilStreamInterrupt* si )
   {
      INT32 rc = SDB_OK ;
      INT64 readSize = 0 ;
      INT64 totalSize = 0 ;

      SDB_ASSERT( NULL != _buf, "_buf can't be NULL" ) ;
      SDB_ASSERT( size > 0, "size <= 0" ) ;

      while ( totalSize < size )
      {
         if ( NULL != si && si->isInterrupted() )
         {
            rc = SDB_INTERRUPT ;
            goto error ;
         }
         
         rc = in.read( _buf, OSS_MIN( size - totalSize, _bufSize ), readSize ) ;
         if ( SDB_EOF == rc )
         {
            rc = SDB_OK ;
            break ;
         }
         else if ( SDB_OK == rc )
         {
            totalSize += readSize ;
            
            rc = out.write( _buf, readSize ) ;
            if ( SDB_OK != rc )
            {
               PD_LOG( PDERROR, "Failed to write to outstream, rc=%d", rc ) ;
               goto error ;
            }
         }
         else
         {
            PD_LOG( PDERROR, "Failed to read from instream, rc=%d", rc ) ;
            goto error ;
         }
      }

      if ( NULL != streamSize )
      {
         *streamSize = totalSize ;
      }
      
   done:
      return rc ;
   error:
      goto done ;
   }
}
