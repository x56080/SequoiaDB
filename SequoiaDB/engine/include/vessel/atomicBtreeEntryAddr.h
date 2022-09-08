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

   Source File Name = atomicBtreeEntryAddr.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#ifndef VESSEL_ATOMIC_BTREE_ENTRY_ADDR_H_
#define VESSEL_ATOMIC_BTREE_ENTRY_ADDR_H_

#include "vessel/pageIdentifier.h"
#include <atomic>

namespace engine
{
namespace vessel
{
   struct btreeEntryAddr
   {
      OSS_INLINE void reset()
      {
         psn = 0;
         pid = INVALID_PAGE_ID;
      }
      OSS_INLINE BOOLEAN isValid()const
      {
         return INVALID_PAGE_ID != pid;
      }
      OSS_INLINE PAGE_ID getVisiblePid(UINT32 psn)const
      {
         if (!isValid() || psn < this->psn)
         {
            return INVALID_PAGE_ID;
         }
         else
         {
            return pid;
         }
      }

      UINT32 psn = 0;
      PAGE_ID pid = INVALID_PAGE_ID;
   };

   class atomicBtreeEntryAddr : public SDBObject
   {
      public:
         atomicBtreeEntryAddr() = default;
         ~atomicBtreeEntryAddr() = default;

      public:
         OSS_INLINE void reset()
         {
            _addr.store(btreeEntryAddr(), std::memory_order_relaxed);
         }
         OSS_INLINE btreeEntryAddr get()const
         {
            return _addr.load(std::memory_order_relaxed);
         }
         OSS_INLINE void set(PAGE_ID pid, UINT32 psn)
         {
            btreeEntryAddr addr;
            addr.pid = pid;
            addr.psn = psn;
            _addr.store(addr, std::memory_order_relaxed);
         }

      private:
         static_assert(8 == sizeof(btreeEntryAddr), "out of size");
         std::atomic<btreeEntryAddr> _addr;
   };//class atomicBtreeEntryAddr
} // namespace vessel

} // namespace engine


#endif//VESSEL_ATOMIC_BTREE_ENTRY_ADDR_H_