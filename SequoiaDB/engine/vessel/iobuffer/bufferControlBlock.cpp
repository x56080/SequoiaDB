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

   Source File Name = bufferControlBlock.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

*******************************************************************************/
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

   BOOLEAN atomicBufferCtlBlock::setRecyclingFromNormal()
   {
      bufferControlBlock expectedVal, val;
      expectedVal.init(BUFFER_STATUS::NORMAL, 0, 0);
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

   BUFFER_CTL_FLAG_WORD atomicBufferCtlBlock::resetFlags()
   {
      bufferControlBlock oldVal = _val.load(std::memory_order_relaxed);
      do
      {
         bufferControlBlock newVal(oldVal);
         newVal.resetFlags();
         if (_val.compare_exchange_weak(oldVal, newVal))
         {
            break;
         }
      } while (TRUE);
      
      return oldVal.getFlags();
   }

   BUFFER_CTL_FLAG_WORD atomicBufferCtlBlock::clearFlag(BUFFER_CTL_FLAG_WORD flags)
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

   BUFFER_CTL_FLAG_WORD atomicBufferCtlBlock::setFlag(BUFFER_CTL_FLAG_WORD flags)
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

   BUFFER_CTL_FLAG_WORD atomicBufferCtlBlock::updateFlag(BUFFER_CTL_FLAG_WORD toSet,
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

   BOOLEAN atomicBufferCtlBlock::setFlagIfNot(BUFFER_CTL_FLAG_WORD condition,
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
