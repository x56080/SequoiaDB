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

   Source File Name = rplUtil.hpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          22/9/2016  David Li  Initial Draft

   Last Changed =

*******************************************************************************/
#include "rplUtil.hpp"
#include "ossUtil.hpp"
#include "dpsDef.hpp"
#include "pd.hpp"

using namespace std ;

namespace replay
{
   void getCurrentTime( string &timeStr )
   {
      ossTimestamp Tm ;
      CHAR szFormat[] = "%04d%02d%02d%02d%02d%02d" ;
      CHAR szTimestmpStr[ OSS_TIMESTAMP_STRING_LEN + 1 ] = { 0 } ;
      struct tm tmpTm ;

      ossGetCurrentTime( Tm ) ;
      ossLocalTime( Tm.time, tmpTm ) ;

      if ( Tm.microtm >= OSS_ONE_MILLION )
      {
         tmpTm.tm_sec ++ ;
         Tm.microtm %= OSS_ONE_MILLION ;
      }

      ossSnprintf ( szTimestmpStr, sizeof( szTimestmpStr ),
                    szFormat,
                    tmpTm.tm_year + 1900,
                    tmpTm.tm_mon + 1,
                    tmpTm.tm_mday,
                    tmpTm.tm_hour,
                    tmpTm.tm_min,
                    tmpTm.tm_sec ) ;

      timeStr = szTimestmpStr ;
   }

   void getCurrentDate( string &dateStr )
   {
      ossTimestamp Tm ;
      CHAR szFormat[] = "%04d%02d%02d%02d%02d" ;
      CHAR szDatetmpStr[ OSS_TIMESTAMP_STRING_LEN + 1 ] = { 0 } ;
      struct tm tmpTm ;

      ossGetCurrentTime( Tm ) ;
      ossLocalTime( Tm.time, tmpTm ) ;

      if ( Tm.microtm >= OSS_ONE_MILLION )
      {
         tmpTm.tm_sec ++ ;
         Tm.microtm %= OSS_ONE_MILLION ;
      }

      ossSnprintf ( szDatetmpStr, sizeof( szDatetmpStr ),
                    szFormat,
                    tmpTm.tm_year + 1900,
                    tmpTm.tm_mon + 1,
                    tmpTm.tm_mday,
                    tmpTm.tm_hour,
                    tmpTm.tm_min) ;

      dateStr = szDatetmpStr ;
   }

   BOOLEAN isSameDay( UINT64 microSecondLeft, UINT64 microSecondRight )
   {
      struct tm tmLeft ;
      ossTimestamp ossTMLeft = ossMicrosecondsToTimestamp( microSecondLeft ) ;
      ossLocalTime( ossTMLeft.time, tmLeft ) ;

      struct tm tmRight ;
      ossTimestamp ossTMRight = ossMicrosecondsToTimestamp( microSecondRight ) ;
      ossLocalTime( ossTMRight.time, tmRight ) ;

      if ( tmLeft.tm_year == tmRight.tm_year
           && tmLeft.tm_mon == tmRight.tm_mon
           && tmLeft.tm_mday == tmRight.tm_mday )
      {
         return TRUE ;
      }

      return FALSE ;
   }

   UINT64 replaceAndGetTime( UINT64 currentTime, INT32 newHour, INT32 newMinite,
                             INT32 newSecond )
   {
      struct tm tm ;
      ossTimestamp ossTM = ossMicrosecondsToTimestamp( currentTime ) ;
      ossLocalTime( ossTM.time, tm ) ;

      tm.tm_hour = newHour ;
      tm.tm_min = newMinite ;
      tm.tm_sec = newSecond ;

      ossTM.time = ossMkTime( &tm ) ;

      return ossTimestampToMicroseconds( ossTM ) ;
   }

   void rplTimestampToString( ossTimestamp &timestamp, string &timeStr )
   {
      CHAR szFormat[] = "%04d-%02d-%02d %02d.%02d.%02d.%06d" ;
      CHAR szTimestmpStr[ OSS_TIMESTAMP_STRING_LEN + 1 ] = { 0 } ;
      struct tm tmpTm ;

      ossLocalTime( timestamp.time, tmpTm ) ;

      if ( timestamp.microtm >= OSS_ONE_MILLION )
      {
         tmpTm.tm_sec ++ ;
         timestamp.microtm %= OSS_ONE_MILLION ;
      }

      ossSnprintf ( szTimestmpStr, sizeof( szTimestmpStr ),
                    szFormat,
                    tmpTm.tm_year + 1900,
                    tmpTm.tm_mon + 1,
                    tmpTm.tm_mday,
                    tmpTm.tm_hour,
                    tmpTm.tm_min,
                    tmpTm.tm_sec,
                    timestamp.microtm ) ;

      timeStr = szTimestmpStr ;
   }

   void rplTimeIncToString( time_t &timer, UINT32 inc, string &timeStr )
   {
      CHAR szFormat[] = "%04d-%02d-%02d %02d.%02d.%02d.%06d" ;
      CHAR szTimestmpStr[ OSS_TIMESTAMP_STRING_LEN + 1 ] = { 0 } ;
      struct tm tmpTm ;

      ossLocalTime( timer, tmpTm ) ;
      ossSnprintf ( szTimestmpStr, sizeof( szTimestmpStr ),
                    szFormat,
                    tmpTm.tm_year + 1900,
                    tmpTm.tm_mon + 1,
                    tmpTm.tm_mday,
                    tmpTm.tm_hour,
                    tmpTm.tm_min,
                    tmpTm.tm_sec,
                    inc ) ;

      timeStr = szTimestmpStr ;
   }

}

