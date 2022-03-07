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

   Source File Name = fixedBitmap.hpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#ifndef VESSEL_FIXED_BITMAP_HPP_
#define VESSEL_FIXED_BITMAP_HPP_

#include "ossMemPool.hpp"
#include "pdTrace.hpp"
#include "ossLikely.hpp"
#include <bitset> //c++11


namespace engine
{
namespace vessel
{

   template<UINT32 UNIT_SIZE=512>
   class fixedBitmap : public SDBObject
   {
      public:
         fixedBitmap()
         {
            static_assert(0 != UNIT_SIZE, "invalid size");
            static_assert(ossIsAligned64(UNIT_SIZE), "should be 64 aligned");
            static_assert(UNIT_SIZE <= 4096, "should not be too large");
         }
         ~fixedBitmap()
         {
            
         }
         fixedBitmap(const fixedBitmap &) = delete;
         fixedBitmap &operator=(const fixedBitmap &) = delete;


      private:
         typedef std::bitset<UNIT_SIZE> _BITMAP;

         class _bitmapUnit : public SDBObject
         {
            public:
               void set()
               {
                  nonzeroBits = UNIT_SIZE;
                  bm.set();
               }

               void clear()
               {
                  nonzeroBits = 0;
                  bm.reset();
               }

            public:
               UINT32 nonzeroBits = 0;
               _BITMAP bm;
         };//class _bitmapUnit

         typedef ossPoolVector<_bitmapUnit> _BITMAP_UNIT_VEC;

         typedef ossPoolList<INT32> _UNIT_FREE_LIST;

      public:
         BOOLEAN isValid()const {return !_units.empty();}
         UINT32 getTotalBitNum()const {return UNIT_SIZE * _units.size();}
         BOOLEAN hasNonzeroBit()const {return !_fl.empty();}
         UINT32 getUnitCount()const {return _units.size();}
         constexpr UINT32 getUnitSize()const {return UNIT_SIZE;}

         void init(UINT32 unitCount)
         {
            SDB_ASSERT(0 < unitCount, "can not be invalid");
            SDB_ASSERT(((UINT64)unitCount * UNIT_SIZE) < (UINT64)OSS_SINT32_MAX,
                      "out of bound");
            fini();

            _units.resize(unitCount);
            for (INT32 i = 0; i < _units.size(); ++i)
            {
               _units[i].set();
               _fl.push_back(i);
            }
            return;
         }

         void fini()
         {
            _fl.clear();
            _units.clear();
         }

         /// find a unzero bit and clear it.
         /// return -1 if no unzero bit.
         INT32 pop()
         {
            INT32 bitPos = -1;
            while (!_fl.empty())
            {
               INT32 unitId = *(_fl.cbegin());
               _bitmapUnit &unit = _units[unitId];
               SDB_ASSERT(0 < unit.nonzeroBits, "impossible to be zero");
               UINT32 first = unit.bm._Find_first();
               SDB_ASSERT(first < UNIT_SIZE, "impossible to be out of bound");
               unit.bm.reset(first);
               if (0 == --unit.nonzeroBits)
               {
                  _fl.pop_front();
               }

               bitPos = (unitId * UNIT_SIZE) + static_cast<INT32>(first);
               break;
            }

            return bitPos;
         }

         void set(INT32 pos, BOOLEAN *beforeSet=nullptr)
         {
            SDB_ASSERT(0 <= pos && pos < (INT32)getTotalBitNum(), "out of bound");
            INT32 unitId = getUnitId(pos);
            UINT32 bitInUnit = pos % UNIT_SIZE;
            _bitmapUnit &unit = _units[unitId];
            BOOLEAN old = unit.bm.test(bitInUnit);
            if (nullptr != beforeSet)
            {
               *beforeSet = old;
            }

            if (!old)
            {
               unit.bm.set(bitInUnit);
               if (1 == (++unit.nonzeroBits))
               {
                  _fl.push_back(unitId);
               }
            }

            return;
         }

         BOOLEAN test(INT32 pos)const
         {
            SDB_ASSERT(0 <= pos && pos < (INT32)getTotalBitNum(), "out of bound");
            INT32 unitId = getUnitId(pos);
            UINT32 bitInUnit = pos % UNIT_SIZE;
            const _bitmapUnit &unit = _units[unitId];
            return unit.bm.test(bitInUnit);
         }

      private:
         INT32 getUnitId(UINT32 bitPos)const
         {
            return static_cast<INT32>(bitPos / UNIT_SIZE);
         }

      private:
         _UNIT_FREE_LIST _fl;
         _BITMAP_UNIT_VEC _units;

   };//class fixedBitmap
} // namespace vessel

} // namespace engine


#endif//VESSEL_FIXED_BITMAP_HPP_