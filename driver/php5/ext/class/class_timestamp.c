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

#include "class_timestamp.h"
#include "timestamp.h"

#define TIMESTAMP_FORMAT "%d-%d-%d-%d.%d.%d.%d"

#define TIMESTAMP_MAX_INC 1000000

extern INT32 timestampDesc ;

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

PHP_METHOD( SequoiaTimestamp, __construct )
{
   INT32 rc      = SDB_OK ;
   INT32 year    = 0 ;
   INT32 month   = 0 ;
   INT32 day     = 0 ;
   INT32 hour    = 0 ;
   INT32 minute  = 0 ;
   INT32 second  = 0 ;
   INT32 micros  = 0 ;
   time_t timep  = 0 ;
   zval *pTimestamp  = NULL ;
   zval *pThisObj = getThis() ;
   sdbTimestamp sdbTime ;
   struct tm tmTime ;
   struct phpTimestamp *pDriverTimestamp = (struct phpTimestamp *)\
            emalloc( sizeof( struct phpTimestamp ) ) ;
   if( !pDriverTimestamp )
   {
      goto error ;
   }
   pDriverTimestamp->second = 0 ;
   pDriverTimestamp->micros = 0 ;
   if( PHP_GET_PARAMETERS( "|z", &pTimestamp ) == FAILURE )
   {
      goto error ;
   }
   if( !pTimestamp )
   {
      goto error ;
   }
   if( Z_TYPE_P( pTimestamp ) == IS_STRING )
   {
      if( ossStrchr( Z_STRVAL_P( pTimestamp ), 't' ) ||
          ossStrchr( Z_STRVAL_P( pTimestamp ), 'T' ) )
      {
         rc = timestampParse( Z_STRVAL_P( pTimestamp ),
                              Z_STRLEN_P( pTimestamp ),
                              &sdbTime ) ;
         if( rc )
         {
            goto error ;
         }
         timep = (time_t)sdbTime.sec ;
         micros = sdbTime.nsec / 1000 ;
      }
      else
      {
         ossMemset( &tmTime, 0, sizeof( tmTime ) ) ;
         if( !sscanf( Z_STRVAL_P( pTimestamp ),
                      TIMESTAMP_FORMAT,
                      &year,
                      &month,
                      &day,
                      &hour,
                      &minute,
                      &second,
                      &micros ) )
         {
            goto error ;
         }
         tmTime.tm_year = year - 1900 ;
         tmTime.tm_mon  = month - 1 ;
         tmTime.tm_mday = day ;
         tmTime.tm_hour = hour ;
         tmTime.tm_min  = minute ;
         tmTime.tm_sec  = second ;
         timep = mktime( &tmTime ) ;
      }
      pDriverTimestamp->second = (INT32)timep ;
      pDriverTimestamp->micros = micros ;
   }

   if ( pDriverTimestamp->micros < 0 ||
        pDriverTimestamp->micros >= TIMESTAMP_MAX_INC )
   {
      INT32 sec = pDriverTimestamp->micros / TIMESTAMP_MAX_INC ;
      INT32 us  = pDriverTimestamp->micros % TIMESTAMP_MAX_INC ;

      if ( us < 0 )
      {
         sec -= 1;
         us += TIMESTAMP_MAX_INC;
      }

      pDriverTimestamp->second += sec ;
      pDriverTimestamp->micros = us ;
   }

done:
   PHP_SAVE_RESOURCE( pThisObj,
                      "$timestamp",
                      pDriverTimestamp,
                      timestampDesc ) ;
   return ;
error:
   pDriverTimestamp = NULL ;
   goto done ;
}

PHP_METHOD( SequoiaTimestamp, __toString )
{
   time_t second = 0 ;
   INT32 micros = 0 ;
   struct phpTimestamp *pDriverTimestamp = NULL ;
   zval *pThisObj = getThis() ;
   CHAR pDateStr[64] ;
   struct tm tmTime ;
   PHP_READ_RESOURCE( pThisObj,
                      "$timestamp",
                      pDriverTimestamp,
                      struct phpTimestamp*,
                      SDB_TIMESTAMP_HANDLE_NAME,
                      timestampDesc ) ;
   if( !pDriverTimestamp )
   {
      goto error ;
   }
   second = (time_t)pDriverTimestamp->second ;
   micros = pDriverTimestamp->micros ;
   ossMemset( pDateStr, 0, 64 ) ;
   local_time( &second, &tmTime ) ;
   ossSnprintf( pDateStr,
                64,
                "%04d-%02d-%02d-%02d.%02d.%02d.%06d",
                tmTime.tm_year + 1900,
                tmTime.tm_mon + 1,
                tmTime.tm_mday,
                tmTime.tm_hour,
                tmTime.tm_min,
                tmTime.tm_sec,
                micros ) ;
   RETVAL_STRING( pDateStr, 1 ) ;
done:
   return ;
error:
   RETVAL_EMPTY_STRING() ;
   goto done ;
}