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

   Source File Name = lsmLsnTracker.h

   Descriptive Name = 

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          06/24/2022  LYC  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef VESSEL_LSM_LSN_TRACKER_H_
#define VESSEL_LSM_LSN_TRACKER_H_

#include "oss.hpp"
#include "dpsDef.hpp"
#include <atomic>

namespace engine
{
namespace vessel
{
   class lsmLsnTracker : public SDBObject
   {
      public:
         lsmLsnTracker() = default;
         ~lsmLsnTracker() = default;
         lsmLsnTracker(const lsmLsnTracker &) = delete;
         lsmLsnTracker &operator= (const lsmLsnTracker &) = delete;

      public:
         DPS_LSN_OFFSET beginToFlush();

         void endToFlush(BOOLEAN flushDone); 

         void setMinWriteLsn(DPS_LSN_OFFSET lsn);

         DPS_LSN_OFFSET getMinDirtyLsn() const;

      private:
         std::atomic_ullong _minWriteLsn = {DPS_INVALID_LSN_OFFSET};
         std::atomic_ullong _minFlushLsn = {DPS_INVALID_LSN_OFFSET};
   }; // class lsmLsnTracker
 
} // namespace vessel
} // namespace engine

#endif // VESSEL_LSM_LSN_TRACKER_H_