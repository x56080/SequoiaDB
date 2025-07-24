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

   Source File Name = hitRateLimiter.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          10/10/2022  LYC  Initial Draft

   Last Changed =

*******************************************************************************/
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
