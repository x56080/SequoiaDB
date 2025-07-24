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

   Source File Name = hitRateLimitOptions.h

   Descriptive Name = 

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          10/10/2022  LYC  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef VESSEL_HIT_RATE_LIMIT_OPTIONS_H_
#define VESSEL_HIT_RATE_LIMIT_OPTIONS_H_

#include "oss.hpp"

namespace engine
{
namespace vessel
{
   struct hitRateLimitOptions : public SDBObject
   {
      static constexpr UINT32 LIMIT_SST_COUNT_MAX = 1024;

      // Ranges: [0, LIMIT_SST_COUNT_MAX]
      UINT32 sstCountToTriggerRateLimit = 128;
      
      // 'sstCountToKeepMaxSleepTime == 0' means no rate limit.
      // If 'sstCountToKeepMaxSleepTime' is not 0, it 
      // must be greater than or equal to 'sstCountToTriggerRateLimit'.
      // Ranges: [0, LIMIT_SST_COUNT_MAX]
      UINT32 sstCountToKeepMaxSleepTime = 256;

      static constexpr UINT32 SLEEP_MICRO_SECONDS_MIN = 1;
      static constexpr UINT32 SLEEP_MICRO_SECONDS_MAX = 1000 * 1000;

      // Ranges: [SLEEP_MICRO_SECONDS_MIN, SLEEP_MICRO_SECONDS_MAX]
      UINT32 minSleepMicroseconds = 10;

      // 'maxSleepMicroseconds' must be greater than or equal to 'minSleepMicroseconds'.
      // Ranges: [SLEEP_MICRO_SECONDS_MIN, SLEEP_MICRO_SECONDS_MAX]
      UINT32 maxSleepMicroseconds = 10000;

   }; // struct hitRateLimitOptions

} // namespace vessel
} // namespace engine

#endif//VESSEL_HIT_RATE_LIMIT_OPTIONS_H_