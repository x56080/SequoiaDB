/*******************************************************************************
   Copyright (C) 2012-2014 SequoiaDB Ltd.

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

   http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.
*******************************************************************************/

#include "class_date.h"
#include "timestamp.h"

#define DATE_FORMAT "%d-%d-%d"

extern INT32 dateDesc ;

static void local_time ( time_t *Time, struct tm *TM )
{
   if ( !Time || !TM )
      return ;
#if defined (__linux__ ) || defined (_AIX)
   localtime_r( Time, TM ) ;
#elif defined (_WIN32)
   // The Time represents the seconds elapsed since midnight (00:00:00),
   // January 1, 1970, UTC. This value is usually obtained from the time
   // function.
   localtime_s( TM, Time ) ;
#else
#error "unimplemented local_time()"
#endif
}

PHP_METHOD( SequoiaDate, __construct )
{
   INT32 rc     = SDB_OK ;
   INT32 year   = 0 ;
   INT32 month  = 0 ;
   INT32 day    = 0 ;
   INT32 micros = 0 ;
   INT32 numType = PHP_NOT_NUMBER ;
   INT32 numInt  = 0 ;
   INT64 numLong = 0 ;
   double numDouble = 0 ;
   time_t timep = 0 ;
   zval *pDate  = NULL ;
   zval *pThisObj = getThis() ;
   sdbTimestamp sdbTime ;
   struct tm tmTime ;
   struct phpDate *pDriverDate = (struct phpDate *)\
               emalloc( sizeof( struct phpDate ) ) ;
   if( !pDriverDate )
   {
      goto error ;
   }
   pDriverDate->milli = 0 ;
   if( PHP_GET_PARAMETERS( "|z", &pDate ) == FAILURE )
   {
      goto error ;
   }
   if( !pDate )
   {
      goto error ;
   }
   if( Z_TYPE_P( pDate ) == IS_STRING )
   {
      if( ossStrchr( Z_STRVAL_P( pDate ), 't' ) ||
          ossStrchr( Z_STRVAL_P( pDate ), 'T' ) )
      {
         rc = timestampParse( Z_STRVAL_P( pDate ),
                              Z_STRLEN_P( pDate ),
                              &sdbTime ) ;
         if( rc )
         {
            goto error ;
         }
         timep = (time_t)sdbTime.sec ;
         micros = sdbTime.nsec / 1000 ;
         pDriverDate->milli = (INT64)timep * 1000LL + micros ;
      }
      else
      {
         php_parseNumber( Z_STRVAL_P( pDate ),
                          Z_STRLEN_P( pDate ),
                          &numType,
                          &numInt,
                          &numLong,
                          &numDouble TSRMLS_CC ) ;
         if( numType == PHP_NOT_NUMBER )
         {
            ossMemset( &tmTime, 0, sizeof( tmTime ) ) ;
            if( !sscanf( Z_STRVAL_P( pDate ),
                         DATE_FORMAT,
                         &year,
                         &month,
                         &day ) )
            {
               goto error ;
            }
            tmTime.tm_year = year - 1900 ;
            tmTime.tm_mon  = month - 1 ;
            tmTime.tm_mday = day ;
            tmTime.tm_hour = 0 ;
            tmTime.tm_min  = 0 ;
            tmTime.tm_sec  = 0 ;
            timep = mktime( &tmTime ) ;
            pDriverDate->milli = (INT64)timep * 1000LL ;
         }
         else
         {
            if( numType == PHP_IS_INT32 )
            {
               pDriverDate->milli = (INT64)numInt ;
            }
            else if( numType == PHP_IS_INT64 )
            {
               pDriverDate->milli = numLong ;
            }
         }
      }
   }
   else if( Z_TYPE_P( pDate ) == IS_LONG )
   {
      pDriverDate->milli = (INT64)(Z_LVAL_P( pDate )) ;
   }
done:
   PHP_SAVE_RESOURCE( pThisObj, "$date", pDriverDate, dateDesc ) ;
   return ;
error:
   pDriverDate = NULL ;
   goto done ;
}

PHP_METHOD( SequoiaDate, __toString )
{
   time_t timep = 0 ;
   struct phpDate *pDriverDate = NULL ;
   zval *pThisObj = getThis() ;
   CHAR pDateStr[64] ;
   struct tm tmTime ;
   PHP_READ_RESOURCE( pThisObj,
                      "$date",
                      pDriverDate,
                      struct phpDate*,
                      SDB_DATE_HANDLE_NAME,
                      dateDesc ) ;
   if( !pDriverDate )
   {
      goto error ;
   }
   timep = (time_t)( pDriverDate->milli / 1000 ) ;
   ossMemset( pDateStr, 0, 64 ) ;
   local_time( &timep, &tmTime ) ;
   if( tmTime.tm_year + 1900 >= 0 &&
       tmTime.tm_year + 1900 <= 9999 )
   {
      ossSnprintf( pDateStr,
                   64,
                   "%04d-%02d-%02d",
                   tmTime.tm_year + 1900,
                   tmTime.tm_mon + 1,
                   tmTime.tm_mday ) ;
      PHP_RETVAL_STRING( pDateStr, 1 ) ;
   }
   else
   {
      ossMemset( pDateStr, 0, 64 ) ;
      ossSnprintf( pDateStr,
                   64,
                   "%lld",
                   (UINT64)pDriverDate->milli ) ;
      PHP_RETVAL_STRING( pDateStr, 1 ) ;
   }
done:
   return ;
error:
   RETVAL_EMPTY_STRING() ;
   goto done ;
}