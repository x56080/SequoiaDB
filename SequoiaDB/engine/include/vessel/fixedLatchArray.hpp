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

   Source File Name = fixedLatchArray.hpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef VESSEL_FIXED_LATCH_ARRAY_HPP_
#define VESSEL_FIXED_LATCH_ARRAY_HPP_

#include "ossMemPool.hpp"
#include "pdTrace.hpp"
#include "ossLatch.hpp"

namespace engine
{
namespace vessel
{
   template<class T>
   class fixedLatchArray : public SDBObject
   {
      public:
         fixedLatchArray(){}
         ~fixedLatchArray(){}
         fixedLatchArray(const fixedLatchArray &) = delete;
         fixedLatchArray &operator=(const fixedLatchArray &) = delete;

      public:
         void fini()
         {
            _latches.clear();
         }

         void init(UINT32 size)
         {
            fini();
            SDB_ASSERT(ossIsPowerOf2(size), "must be power of 2");
            _latches.resize(size);
         }

         UINT32 getSize()const {return _latches.size();}

         T *at(UINT32 pos)
         {
            SDB_ASSERT(pos < _latches.size(), "out of bound");
            return pos < _latches.size() ? (_latches.data() + pos) : nullptr;
         }

         T *mod(UINT32 value)
         {
            SDB_ASSERT(!_latches.empty(), "can not be empty");
            return _latches.empty() ?
                   nullptr : (_latches.data() + (value & (_latches.size() - 1)));
         }

      private:
         std::vector<T> _latches;
      
   };//class fixedLatchArray

   typedef fixedLatchArray<ossSpinSLatchPOSIX> FIXED_POSIX_S_LATCH_ARRAY;
} // namespace vessel

} // namespace engine


#endif//VESSEL_FIXED_LATCH_ARRAY_HPP_