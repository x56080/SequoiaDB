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

   Source File Name = hitRateLimiter.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          10/10/2022  LYC  Initial Draft

   Last Changed =

******************************************************************************/

#ifndef VESSEL_HIT_RATE_LIMITER_H_
#define VESSEL_HIT_RATE_LIMITER_H_

#include "oss.hpp"
#include "vessel/hitRateLimitOptions.h"
#include <atomic>

namespace engine
{
namespace vessel
{
   class hitRateLimiter : public SDBObject
   {
      public:
         hitRateLimiter() = default;
         ~hitRateLimiter() = default;
         hitRateLimiter(const hitRateLimiter &) = delete;
         hitRateLimiter &operator=(const hitRateLimiter &) = delete;
      
      public:
         INT32 set(const hitRateLimitOptions &o);
         void reset();

      public:
         void limitRate() const;
         void adjustSleepTime(UINT32 curSstCount);
         UINT32 getSleepTime() const;
         BOOLEAN isLimited() const;

      private:
         hitRateLimitOptions _o;
         std::atomic_uint _sleepTime = {0};
         UINT32 _limitRange = _o.sstCountToKeepMaxSleepTime - _o.sstCountToTriggerRateLimit;

   }; // class hitRateLimiter

} // namespace vessel
} // namespace engine

#endif // VESSEL_HIT_RATE_LIMITER_H_
