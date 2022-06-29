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