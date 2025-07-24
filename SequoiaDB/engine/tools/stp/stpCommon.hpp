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

#include "oss.hpp"
#include "math.h"

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
   // convert nanoseconds to milliseconds
   #define STP_NANOSEC_TO_MILLISEC( x )      ( (UINT64)( x ) / 1000000LL )
   // convert microseconds to milliseconds
   #define STP_MICROSEC_TO_MILLISEC( x )     ( (UINT32)( x ) / 1000 )
   #define STP_MICROSEC_TO_MILLISEC_CEIL( x ) \
                              ( (UINT32)( ceil( (FLOAT64)( x ) / 1000.0 ) ) )
   // convert milliseconds to microseconds
   #define STP_MILLISEC_TO_MICROSEC( x )     ( (UINT64)( x ) * 1000LL )
   // convert milliseconds to seconds
   #define STP_MILLISEC_TO_SEC( x )          ( ( x ) / 1000 )

   // STP synchronize interval defines ( in seconds )
   // minimum synchronize interval is 10 seconds
   #define STP_MIN_SYNC_INTERVAL       ( 10 )
   // maximum synchronize interval is 600 seconds
   #define STP_MAX_SYNC_INTERVAL       ( 600 )
   // default synchronize interval is 60 seconds
   #define STP_DEF_SYNC_INTERVAL       ( 60 )
   // step for increase / decrease synchronize interval in runtime
   #define STP_SYNC_INTERVAL_STEP      ( STP_MIN_SYNC_INTERVAL )
   // calculate steps from synchronize interval
   #define STP_SYNC_INT_TO_STEP( syncIntSec ) \
                           ( ( syncIntSec ) >= 1 ? \
                             ( ( ( syncIntSec ) + STP_SYNC_INTERVAL_STEP - 1 ) / \
                               STP_SYNC_INTERVAL_STEP - 1 ) : \
                             ( 0 ) )
   // calculate synchronize interval from steps
   #define STP_SYNC_STEP_TO_INT( syncIntSteps ) \
                           ( STP_MIN_SYNC_INTERVAL + \
                             STP_SYNC_INTERVAL_STEP * ( syncIntSteps ) )

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

   // synchronize record with offset in valid range means the offset is too
   // trivial to adjust slew rate
   // NOTE:
   // - the offset of each synchronize can not be larger than the minimum
   //   time error ( 1ms )
   // - we take 1/10 ( 0.1ms ) of it as the limit to adjust the slew rate for
   //   CPU ticks
   // maximum valid offset to slew rate check
   #define STP_SLEWRATE_OFFSET_MAX_LIMIT     ( 100000L )
   // minimum valid offset to slew rate check
   #define STP_SLEWRATE_OFFSET_MIN_LIMIT     ( -100000L )

   // STP default slew rate for CPU ticks
   // NOTE:
   // for the given minimum and maximum limits and interval of slew rate
   // check interval ( 10s ), we need 10s / 0.1ms in precision to tell difference
   #define STP_DEF_SLEWRATE   ( 100000LL )

   // STP to keep history synchronize records
   #define STP_DEF_SYNC_HIST_SIZE ( 20 )
}

#endif // STP_COMMON_HPP__
