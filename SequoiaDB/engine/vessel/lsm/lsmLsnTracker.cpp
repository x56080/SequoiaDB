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

   Source File Name = lsmLsnTracker.cpp

   Descriptive Name = 

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          06/24/2022  LYC  Initial Draft

   Last Changed =

*******************************************************************************/
#include "vessel/lsm/lsmLsnTracker.h"

namespace engine
{
namespace vessel
{
   DPS_LSN_OFFSET lsmLsnTracker::beginToFlush()
   {
      SDB_ASSERT(DPS_INVALID_LSN_OFFSET == _minFlushLsn.load(std::memory_order_relaxed),
                 "must be invalid");
      DPS_LSN_OFFSET oldLsn = _minWriteLsn.exchange(DPS_INVALID_LSN_OFFSET);
      if (DPS_INVALID_LSN_OFFSET != oldLsn)
      {
         _minFlushLsn.store(oldLsn);
      }
      return oldLsn;
   }

   void lsmLsnTracker::endToFlush(BOOLEAN flushDone)
   {
      if (!flushDone)
      {
         setMinWriteLsn(_minFlushLsn.load(std::memory_order_relaxed));
      }
      _minFlushLsn.store(DPS_INVALID_LSN_OFFSET);
   }

   void lsmLsnTracker::setMinWriteLsn(DPS_LSN_OFFSET lsn)
   {
      DPS_LSN_OFFSET oldLsn = _minWriteLsn.load(std::memory_order_relaxed);
      do
      {
         if (lsn >= oldLsn)
         {
            break;
         }
         else if (_minWriteLsn.compare_exchange_weak(oldLsn, lsn))
         {
            break;
         }
      } while (TRUE);
   }

   DPS_LSN_OFFSET lsmLsnTracker::getMinDirtyLsn()const
   {
      DPS_LSN_OFFSET fl = _minFlushLsn.load(std::memory_order_relaxed);
      DPS_LSN_OFFSET wl = _minWriteLsn.load(std::memory_order_relaxed);
      return fl < wl ? fl : wl;
   }

} // namespace vessel
} // namespace engine