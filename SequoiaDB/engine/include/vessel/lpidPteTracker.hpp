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

   Source File Name = lpidPteTracker.hpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#ifndef VESSEL_LPID_PTE_TRACKER_H_
#define VESSEL_LPID_PTE_TRACKER_H_

#include "ossMemPool.hpp"
#include "vessel/fixedBitset.hpp"
#include "vessel/pageIdentifier.h"

#include <memory>

namespace engine
{
namespace vessel
{
   template<UINT32 UNIT_SIZE>
   class lpidPteTracker : public SDBObject
   {
      public:
         lpidPteTracker();
         ~lpidPteTracker() = default;
         lpidPteTracker(const lpidPteTracker &) = delete;
         lpidPteTracker &operator=(const lpidPteTracker &) = delete;

      public:
         void reset() {_tmap.clear();}

      private:
         using _TRACKER_UNIT = fixedBitset<UNIT_SIZE>;
         using _TRACKER_UNIT_PTR = std::unique_ptr<_TRACKER_UNIT>;
         using _TRACKER_UNIT_MAP = ossPoolMap<UINT32, _TRACKER_UNIT_PTR>;

         UINT32 _getUnitId(PAGE_ID lpid)const
         {
            return lpid >> _sequare;
         }
         UINT32 _getUnitPos(PAGE_ID lpid)const
         {
            return lpid & (UNIT_SIZE - 1);
         }

      private:
         UINT32 _sequare = 0;
         _TRACKER_UNIT_MAP _tmap;
   };//class class lpidPteTracker

   template<UINT32 UNIT_SIZE>
   lpidPteTracker<UNIT_SIZE>::lpidPteTracker()
   {
      BOOLEAN r = ossIsPowerOf2(UNIT_SIZE, &_sequare);
      SDB_ASSERT(r, "must be power of 2");
   }
} // namespace vessel

} // namespace engine


#endif//VESSEL_LPID_PTE_TRACKER_H_
