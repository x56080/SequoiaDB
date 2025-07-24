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

   Source File Name = atomicBtreeEntryAddr.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

*******************************************************************************/
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