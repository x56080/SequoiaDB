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

   Source File Name = fixedLatchArray.hpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

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