/*******************************************************************************

   Copyright (C) 2011-2018 SequoiaDB Ltd.

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU Affero General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU Affero General Public License for more details.

   You should have received a copy of the GNU Affero General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.

   Source File Name = stpCommon.hpp

   Descriptive Name = Serial Time Protocol common defines

   When/how to use: this program may be used on binary and text-formatted
   versions of STP component. This file contains structure for Serial Time
   Protocol.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          07/30/2019  HGM Initial Draft

   Last Changed =

*******************************************************************************/

#ifndef STP_COMMON_HPP__
#define STP_COMMON_HPP__

namespace engine
{

   // name for pipe for STP
   // NOTE: 2 pipes will be created for STP
   //       one named in service name ( port ) is system pipe
   //       another named in stp is used for STP only
   #define STP_PIPE_SERVICE_NAME       "stp"

   // version of STP ( current is 1 )
   #define STP_VERSION_V1              ( 1 )
   #define STP_VERSION                 ( STP_VERSION_V1 )

   // time unit converters
   // convert seconds into milliseconds
   #define STP_SEC_TO_MILLISEC( x )          ( ( x ) * OSS_ONE_SEC )
   // convert seconds into microseconds
   #define STP_SEC_TO_MICROSEC( x )          ( (UINT64)( x ) * 1000000LL )
   // convert seconds into nanoseconds
   #define STP_SEC_TO_NANOSEC( x )           ( (UINT64)( x ) * 1000000000LL )

   // convert microseconds to nanoseconds
   #define STP_MICROSEC_TO_NANOSEC( x )      ( (UINT64)( x ) * 1000LL )
   // convert milliseconds to nanoseconds
   #define STP_MILLISEC_TO_NANOSEC( x )      ( (UINT64)( x ) * 1000000LL )

   // convert nanoseconds to microseconds
   #define STP_NANOSEC_TO_MICROSEC( x )      ( (UINT64)( x ) / 1000LL )
   // convert microseconds to millicseconds
   #define STP_MICROSEC_TO_MILLISEC( x )     ( (UINT32)( x ) / 1000 )
   // convert milliseconds to seconds
   #define STP_MILLISEC_TO_SEC( x )          ( ( x ) / 1000 )

   // STP synchronize interval defines ( in seconds )
   // minimum synchronize interval is 10 seconds
   #define STP_MIN_SYNC_INTERVAL       ( 10 )
   // maximum synchronize interval is 600 seconds
   #define STP_MAX_SYNC_INTERVAL       ( 600 )
   // default synchronize interval is 60 seconds
   #define STP_DEF_SYNC_INTERVAL       ( 60 )

   // STP time error defines ( in microseconds )
   // minimum time error is 1000 microseconds
   #define STP_MIN_TIME_ERROR_US       ( 1000 )
   // maximum time error is 50000 microseconds
   #define STP_MAX_TIME_ERROR_US       ( 50000 )
   // default time error is 1000 microseconds ( the same as the minimum one )
   #define STP_DEF_TIME_ERROR_US       ( STP_MIN_TIME_ERROR_US )

   // STP time error defines ( in nanoseconds )
   #define STP_MIN_TIME_ERROR ( STP_MICROSEC_TO_NANOSEC( STP_MIN_TIME_ERROR_US ) )
   #define STP_MAX_TIME_ERROR ( STP_MICROSEC_TO_NANOSEC( STP_MAX_TIME_ERROR_US ) )
   #define STP_DEF_TIME_ERROR ( STP_MICROSEC_TO_NANOSEC( STP_DEF_TIME_ERROR_US ) )

   // step to adjust time error ( adjust time error by 10% each time )
   #define STP_TIME_ERROR_ADJUST_STEP  ( 0.1 )

   // STP default slew rate for CPU ticks
   #define STP_DEF_SLEWRATE   ( 10000LL )

}

#endif // STP_COMMON_HPP__
