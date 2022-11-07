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

   Source File Name = hitRateLimiter.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          10/10/2022  LYC  Initial Draft

   Last Changed =

******************************************************************************/

#include "vessel/hitRateLimiter.h"
#include "pd.hpp"
#include <thread>

namespace engine
{
namespace vessel
{
   INT32 hitRateLimiter::set(const hitRateLimitOptions &o)
   {
      INT32 rc = SDB_OK;
      reset();

      if (0 == o.sstCountToKeepMaxSleepTime)
      {
         // 'sstCountToKeepMaxSleepTime = 0' means no rate limit.
         _o.sstCountToKeepMaxSleepTime = 0;
         goto done;
      }
      else if (o.sstCountToKeepMaxSleepTime < o.sstCountToTriggerRateLimit ||
               o.sstCountToKeepMaxSleepTime > hitRateLimitOptions::LIMIT_SST_COUNT_MAX)
      {
         // Invalid sst count options.
         rc = SDB_INVALIDARG;
         goto error;
      }
      else if (o.maxSleepMicroseconds < o.minSleepMicroseconds ||
               o.minSleepMicroseconds < hitRateLimitOptions::SLEEP_MICRO_SECONDS_MIN ||
               o.maxSleepMicroseconds > hitRateLimitOptions::SLEEP_MICRO_SECONDS_MAX)
      {
         // Invalid sleep time options.
         rc = SDB_INVALIDARG;
         goto error;
      }

      _o = o;
      _limitRange = _o.sstCountToKeepMaxSleepTime - _o.sstCountToTriggerRateLimit;

   done:
      return rc;
   error:
      reset();
      goto done;
   }

   void hitRateLimiter::reset()
   {
      _o = hitRateLimitOptions();
      _sleepTime.store(0, std::memory_order_relaxed);
      _limitRange = _o.sstCountToKeepMaxSleepTime - _o.sstCountToTriggerRateLimit;
   }

   void hitRateLimiter::limitRate() const
   {
      if (0 == _o.sstCountToKeepMaxSleepTime)
      {
         return;
      }

      UINT32 time = _sleepTime.load(std::memory_order_relaxed);
      if (time != 0)
      {
         std::this_thread::sleep_for(std::chrono::microseconds(time));
      }
      return;
   }

   void hitRateLimiter::adjustSleepTime(UINT32 curSstCount)
   {
      if (0 == _o.sstCountToKeepMaxSleepTime)
      {
         return;
      }
  
      // Calculate the sleep time in proportion to the current sst count,
      // more sst files means longer sleep time until max sleep time is reached.
      if (curSstCount < _o.sstCountToTriggerRateLimit)
      {
         // Fewer files, no rate limit required, sleep time is 0.
         _sleepTime.store(0, std::memory_order_relaxed);
      }
      else if (curSstCount >= _o.sstCountToKeepMaxSleepTime)
      {
         // If the current sst count exceeds 'sstCountToKeepMaxSleepTime'
         // or 'sstCountToKeepMaxSleepTime' is equal to 'sstCountToTriggerRateLimit',
         // sleep time is equal to max sleep microseconds.
         _sleepTime.store(_o.maxSleepMicroseconds, std::memory_order_relaxed);
      }
      else
      {
         // When the current file count is within the limit range, we need to
         // calculate the propotion of the current sst count in the limit range.
         // Larger proportion means longer sleep time.
         // The formula for calculating sleep time: 'MaxSleepTime * propotion * propotion'.
         UINT32 overCount = curSstCount - _o.sstCountToTriggerRateLimit + 1;
         FLOAT32 proportion = (FLOAT32)overCount / _limitRange;
         UINT32 time = proportion * proportion * _o.maxSleepMicroseconds;

         // sleep time must be greater than min sleep microseconds.
         if (time < _o.minSleepMicroseconds)
         {
            _sleepTime.store(_o.minSleepMicroseconds, std::memory_order_relaxed);
         }
         else
         {
            _sleepTime.store(time, std::memory_order_relaxed);
         }
      }
      return;
   }

   UINT32 hitRateLimiter::getSleepTime() const 
   {
      return _sleepTime.load(std::memory_order_relaxed);
   }

   BOOLEAN hitRateLimiter::isLimited() const
   {
      return 0 != _sleepTime.load(std::memory_order_relaxed);
   }

} // namespace vessel
} // namespace engine
