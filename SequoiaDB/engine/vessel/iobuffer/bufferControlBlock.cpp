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

   Source File Name = bufferControlBlock.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#include "vessel/bufferControlBlock.h"
#include "pdTrace.hpp"

namespace engine
{
namespace vessel
{
/////////////atomicBufferCtlBlock
   BOOLEAN atomicBufferCtlBlock::incRefCntIfNormal(bufferControlBlock *old)
   {
      BOOLEAN r = FALSE;
      bufferControlBlock oldVal = _val.load(std::memory_order_relaxed);
      while (oldVal.isNormal())
      {
         bufferControlBlock newVal(oldVal);
         newVal.incRefCnt();
         if (_val.compare_exchange_weak(oldVal, newVal))
         {
            r = TRUE;
            break;
         }
      }

      if (nullptr != old)
      {
         *old = oldVal;
      }
      return r;
   }

   void atomicBufferCtlBlock::decRefCnt(BUFFER_CTL_FLAG_WORD flagToClear,
                                        bufferControlBlock *old)
   {
      BOOLEAN r = FALSE;
      bufferControlBlock oldVal = _val.load(std::memory_order_relaxed);
      while (oldVal.isReferenced())
      {
         bufferControlBlock newVal(oldVal);
         newVal.decRefCnt();
         if (0 != flagToClear)
         {
            newVal.clearFlag(flagToClear);
         }
         if (_val.compare_exchange_weak(oldVal, newVal))
         {
            r = TRUE;
            break;
         }
      }
      
      SDB_ASSERT(r, "invalid reference count");
      if (nullptr != old)
      {
         *old = oldVal;
      }
      return;
   }

   BOOLEAN atomicBufferCtlBlock::setRecyclingFromNormal(UINT32 refCntContdition,
                                                        BUFFER_CTL_FLAG_WORD flagCondition)
   {
      bufferControlBlock expectedVal, val;
      expectedVal.init(BUFFER_STATUS::NORMAL, refCntContdition, flagCondition);
      val.init(BUFFER_STATUS::RECYCLING, 0, 0);
      return _val.compare_exchange_strong(expectedVal, val);
   }

   BOOLEAN atomicBufferCtlBlock::setDiscardedFromRecycling()
   {
      bufferControlBlock expectedVal, val;
      expectedVal.init(BUFFER_STATUS::RECYCLING, 0, 0);
      val.init(BUFFER_STATUS::DISCARDED, 0, 0);
      return _val.compare_exchange_strong(expectedVal, val);
   }

   BOOLEAN atomicBufferCtlBlock::exchange(const bufferControlBlock &expected,
                                          const bufferControlBlock &val)
   {
      bufferControlBlock oldVal(expected);
      return _val.compare_exchange_strong(oldVal, val);
   }

   BOOLEAN atomicBufferCtlBlock::exchangeAndUpdateExpected(bufferControlBlock &expected,
                                                           const bufferControlBlock &val)
   {
      return _val.compare_exchange_strong(expected, val);
   }

   BUFFER_CTL_FLAG_WORD atomicBufferCtlBlock::clearFlags(BUFFER_CTL_FLAG_WORD flags)
   {
      bufferControlBlock oldVal = _val.load(std::memory_order_relaxed);
      do
      {
         bufferControlBlock newVal(oldVal);
         newVal.clearFlag(flags);
         if (_val.compare_exchange_weak(oldVal, newVal))
         {
            break;
         }
      } while (TRUE);
      
      return oldVal.getFlags();
   }

   BUFFER_CTL_FLAG_WORD atomicBufferCtlBlock::setFlags(BUFFER_CTL_FLAG_WORD flags)
   {
      bufferControlBlock oldVal = _val.load(std::memory_order_relaxed);
      do
      {
         bufferControlBlock newVal(oldVal);
         newVal.setFlag(flags);
         if (_val.compare_exchange_weak(oldVal, newVal))
         {
            break;
         }
      } while (TRUE);
      
      return oldVal.getFlags();
   }

   BUFFER_CTL_FLAG_WORD atomicBufferCtlBlock::updateFlags(BUFFER_CTL_FLAG_WORD toSet,
                                                          BUFFER_CTL_FLAG_WORD toClear)
   {
      bufferControlBlock oldVal = _val.load(std::memory_order_relaxed);
      do
      {
         bufferControlBlock newVal(oldVal);
         newVal.setFlag(toSet);
         newVal.clearFlag(toClear);
         if (_val.compare_exchange_weak(oldVal, newVal))
         {
            break;
         }
      } while (TRUE);
      
      return oldVal.getFlags();
   }

   BOOLEAN atomicBufferCtlBlock::setFlagsIfNot(BUFFER_CTL_FLAG_WORD condition,
                                               BUFFER_CTL_FLAG_WORD flags,
                                               BUFFER_CTL_FLAG_WORD *old)
   {
      BOOLEAN r = FALSE;
      SDB_ASSERT(0 == OSS_BIT_TEST(condition, flags), "can not be inclusive");
      bufferControlBlock oldVal = _val.load(std::memory_order_relaxed);
      while (!oldVal.testFlag(condition))
      {
         bufferControlBlock newVal(oldVal);
         newVal.setFlag(flags);
         if (_val.compare_exchange_weak(oldVal, newVal))
         {
            r = TRUE;
            break;
         }
      }
      
      if (nullptr != old)
      {
         *old = oldVal.getFlags();
      }
      return r;
   }
/////////////atomicBufferCtlBlock end
} // namespace vessel

} // namespace engine
