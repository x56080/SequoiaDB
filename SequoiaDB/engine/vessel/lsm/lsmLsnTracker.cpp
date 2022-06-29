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