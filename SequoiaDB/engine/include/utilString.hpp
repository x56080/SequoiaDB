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

   Source File Name = utilString.hpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          15/01/2015  YW  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef UTIL_STRING_HPP_
#define UTIL_STRING_HPP_

#include "oss.hpp"
#include "core.hpp"
#include "ossUtil.hpp"
#include "pd.hpp"
#include "ossMem.hpp"
#include <boost/noncopyable.hpp>
#include "../bson/bson.hpp"

#ifdef SDB_ENGINE
#include "utilMemListPool.hpp"
#endif // SDB_ENGINE

#define UTIL_STRING_STAITC_LEN     256
#define UTIL_STRING_INT_LEN        11
#define UTIL_STRING_INT64_LEN      22
#define UTIL_STRING_DOUBLE_LEN     25
#define UTIL_STRING_DATE_LEN       64
#define UTIL_STRING_TIMESTAMP_LEN  128

using namespace bson ;

namespace engine
{
   template < UINT32 BUFFERSIZE = UTIL_STRING_STAITC_LEN > 
   class _utilString : public SDBObject, boost::noncopyable
   {
   public:
      _utilString() :_dynamic( NULL ), _buf( _static ), 
                     _bufLen( BUFFERSIZE ), _len( 0 )
      {
         _buf[0] = '\0' ;
      }

      ~_utilString()
      {
         #ifdef SDB_ENGINE
            if ( NULL != _dynamic )
            {
               SDB_THREAD_FREE( _dynamic ) ;
               _dynamic = NULL ;
            }
         #else
            SAFE_OSS_FREE( _dynamic ) ;
         #endif
      }

   public:
      void  clear()
      {
         _buf[0] = '\0' ;
         _len = 0 ;
      }

      INT32 assign( const CHAR *str )
      {
         clear() ;
         return append( str ) ;
      }

      INT32 append( const CHAR *str )
      {
         if ( str && *str )
         {
            return append( str, ossStrlen( str ) ) ;
         }
         return SDB_OK ;
      }

      INT32 append( const CHAR *str, UINT32 len )
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

      INT32 append( CHAR c )
      {
         return append( &c, 1 ) ;
      }

      INT32 resize( UINT32 size )
      {
         INT32 rc = SDB_OK ;
         if ( BUFFERSIZE < _bufLen && _bufLen < size )
         {
            #ifdef SDB_ENGINE
               CHAR *p = ( CHAR * )SDB_THREAD_REALLOC( _dynamic, size ) ;
            #else
               CHAR *p = ( CHAR * )SDB_OSS_REALLOC( _dynamic, size ) ;
            #endif
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
         else if ( BUFFERSIZE == _bufLen && _bufLen < size )
         {
            #ifdef SDB_ENGINE
               _dynamic = ( CHAR * )SDB_THREAD_ALLOC( size ) ;
            #else
               _dynamic = ( CHAR * )SDB_OSS_MALLOC( size ) ;
            #endif
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

      OSS_INLINE const CHAR *str() const
      {
         return _buf ;
      }

      /// does not include terminating character
      OSS_INLINE UINT32 len() const
      {
         return _len ;
      }

      OSS_INLINE BOOLEAN enough( UINT32 len ) const
      {
         return _len + len + 1 <= _bufLen ;
      }

      INT32 appendINT32( INT32 v )
      {
         return appendNumber( v, UTIL_STRING_INT_LEN, "%d" ) ;
      }

      INT32 appendDouble( FLOAT64 v )
      {
         return appendNumber( v, UTIL_STRING_DOUBLE_LEN, "%g" ) ;
      }

      INT32 appendINT64( INT64 v )
      {
         return appendNumber( v, UTIL_STRING_INT64_LEN, "%lld" ) ;
      }

      INT32 appendDecimal( const bsonDecimal &decimal )
      {
         INT32 rc = SDB_OK ;
         string value ;
         rc = decimal.toStringChecked( value ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "Failed toStringChecked,rc=%d",rc ) ;
            goto error ;
         }

         rc = append( value.c_str(), value.length() ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "failed to append String:%d", rc ) ;
            goto error ;
         }
      done:
         return rc ;
      error:
         goto done ;
      }

      INT32 appendDate( Date_t date )
      {
         INT32 rc = SDB_OK ;
         time_t timer ;
         struct tm psr ;
         if ( !enough( UTIL_STRING_DATE_LEN ) )
         {
            rc = resize( _bufLen + UTIL_STRING_DATE_LEN + 1 ) ;
            if ( SDB_OK != rc )
            {
               goto error ;
            }
         }
         timer = (time_t)( ( INT64 )( date ) / 1000 ) ;
         local_time ( &timer, &psr ) ;
         rc = sprintf( &(_buf[_len]),
                       "%04d-%02d-%02d",
                       psr.tm_year + 1900,
                       psr.tm_mon + 1,
                       psr.tm_mday ) ;
         if ( rc < SDB_OK )
         {
            rc = SDB_SYS ;
            goto error ;
         }

         _len += rc ;
         rc = SDB_OK ;
         _buf[_len] = '\0';
      done:
         return rc ;
      error:
         goto done ;
      }

      INT32 appendTimestamp( Date_t date, UINT32 inc )
      {
         INT32 rc = SDB_OK ;
         time_t timer ;
         struct tm psr ;
         if ( !enough( UTIL_STRING_TIMESTAMP_LEN ) )
         {
            rc = resize( _bufLen + UTIL_STRING_TIMESTAMP_LEN + 1 ) ;
            if ( SDB_OK != rc )
            {
               goto error ;
            }
         }
         timer = (time_t)( ( INT64 )( date.millis ) / 1000 ) ;
         local_time ( &timer, &psr ) ;
         rc = sprintf ( &(_buf[_len]),
                        "%04d-%02d-%02d-%02d.%02d.%02d.%06d",
                        psr.tm_year + 1900,
                        psr.tm_mon + 1,
                        psr.tm_mday,
                        psr.tm_hour,
                        psr.tm_min,
                        psr.tm_sec,
                        inc ) ;
         if ( rc < SDB_OK )
         {
            rc = SDB_SYS ;
            goto error ;
         }

         _len += rc ;
         rc = SDB_OK ;
         _buf[_len] = '\0';
      done:
         return rc ;
      error:
         goto done ;
   }

      INT32 appendOID( const bson::OID &oid )
      {
         return append( oid.str().c_str() ) ;
      }

      INT32 appendBool( BOOLEAN boolValue )
      {
         return append( boolValue ? "true" : "false" ) ;
      }

      template <typename T>
      INT32 appendNumber( T v, UINT32 size, const CHAR *macro )
      {
         INT32 rc = SDB_OK ;
         if ( !enough( size ) )
         {
            rc = resize( size + _bufLen + 1 ) ;
            if ( SDB_OK != rc )
            {
               goto error ;
            }
         }

         rc = sprintf( &(_buf[_len]), macro, v ) ;
         if ( rc < SDB_OK )
         {
            rc = SDB_SYS ;
            goto error ;
         }

         _len += rc ;
         rc = SDB_OK ;
         _buf[_len] = '\0';
      done:
         return rc ;
      error:
         goto done ;
      }

   private:
      CHAR _static[ BUFFERSIZE ] ;
      CHAR *_dynamic ;
      CHAR *_buf ;
      UINT32 _bufLen ;
      UINT32 _len ; 
   } ;
}

#endif //UTIL_STRING_HPP_

