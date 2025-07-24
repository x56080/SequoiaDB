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

   Source File Name = utilString.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          15/01/2015  YW  Initial Draft

   Last Changed =

*******************************************************************************/
#include "utilString.hpp"
#include "ossUtil.hpp"
#include "pd.hpp"
#include "ossMem.hpp"

namespace engine
{
   _utilString::_utilString()
   :_dynamic( NULL ),
    _buf( _static ),
    _bufLen( UTIL_STRING_STAITC_LEN ),
    _len( 0 )
   {
      _buf[0] = '\0' ;
   }

   _utilString::~_utilString()
   {
      SAFE_OSS_FREE( _dynamic ) ;
   }

   INT32 _utilString::append( const CHAR *str, UINT32 len )
   {
      INT32 rc = SDB_OK ;
      SDB_ASSERT( NULL != str, "can not be null" ) ;

      if ( NULL == str || 0 == len )
      {
         goto done ;
      }

      if ( !enough( len ) )
      {
         UINT32 dLen = _bufLen * 2 ;
         UINT32 minSize = _len + len + 1 ;
         rc = resize( dLen <= minSize ?
                      minSize : dLen ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "failed to extend buf:%d", rc ) ;
            goto error ;
         }
      }

      ossMemcpy( _buf + _len, str, len ) ;
      _len += len ;
      _buf[_len] = '\0' ;
   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _utilString::append( CHAR c )
   {
      return append( &c, 1 ) ;
   }

   INT32 _utilString::resize( UINT32 size )
   {
      INT32 rc = SDB_OK ;
      if ( UTIL_STRING_STAITC_LEN < _bufLen &&
           _bufLen < size )
      {
         CHAR *p = ( CHAR * )SDB_OSS_REALLOC( _dynamic, size ) ;
         if ( NULL == p )
         {
            PD_LOG( PDERROR, "failed to allocate mem." ) ;
            rc = SDB_OOM ;
            goto error ;
         }
         _dynamic = p ;
         _bufLen = size ;
         _buf = _dynamic ;
      }
      else if ( UTIL_STRING_STAITC_LEN == _bufLen &&
                _bufLen < size )
      {
         _dynamic = ( CHAR * )SDB_OSS_MALLOC( size ) ;
         if ( NULL == _dynamic )
         {
            PD_LOG( PDERROR, "failed to allocate mem." ) ;
            rc = SDB_OOM ;
            goto error ;
         }
         _bufLen = size ;
         if ( 0 < _len )
         {
            ossMemcpy( _dynamic, _static, _len ) ;
         }
         _dynamic[_len] = '\0' ; 
         _buf = _dynamic ;
      }
   done:
      return rc ;
   error:
      goto done ;
   }

}

