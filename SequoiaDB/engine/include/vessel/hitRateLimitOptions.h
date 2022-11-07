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