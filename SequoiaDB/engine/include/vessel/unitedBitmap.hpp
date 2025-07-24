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

   Source File Name = unitedBitmap.hpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef VESSEL_UNITED_BITMAP_HPP_
#define VESSEL_UNITED_BITMAP_HPP_

#include "ossMemPool.hpp"
#include "pdTrace.hpp"
#include "ossLikely.hpp"
#include "vessel/fixedBitset.hpp"
#include "vessel/bitsetTree.hpp"

namespace engine
{
namespace vessel
{

#pragma pack(4)

   template<UINT32 UNIT_SIZE>
   class unitedBitmap : public SDBObject
   {
      public:
         unitedBitmap()
         {
            static_assert(0 != UNIT_SIZE, "invalid size");
            static_assert(UNIT_SIZE <= 16384, "should not be too large");
         }
         ~unitedBitmap()
         {
            for (UINT32 i = 0; i < _units.size(); ++i)
            {
               _bitmapUnit *unit = _units[i];
               if (nullptr != unit)
               {
                  SDB_OSS_DEL unit;
               }
            }
         }
         unitedBitmap(const unitedBitmap &) = delete;
         unitedBitmap &operator=(const unitedBitmap &) = delete;

      public:
         class options : public SDBObject
         {
            public:
               UINT32 minFreeReused = 1;
               UINT32 frozenPreUnits = 0;
         };//class options


      private:
         class _bitmapUnit : public SDBObject
         {
            public:
               _bitmapUnit(){}
               ~_bitmapUnit(){}

            public:
               UINT32 nonzeroBits = 0;
               fixedBitset<UNIT_SIZE> bs;
         };//class _bitmapUnit

         typedef class ossPoolVector<_bitmapUnit *> _BITMAP_UNIT_VEC;

      public:
         UINT32 getTotalBitNum()const {return UNIT_SIZE * _units.size();}
         BOOLEAN none()const {return _indexTree.none();}
         UINT32 getUnitCount()const {return _units.size();}
         constexpr UINT32 getUnitSize()const {return UNIT_SIZE;}

         void init(const options &o)
         {
            fini();
            _o = o;
            if (0 < _o.frozenPreUnits)
            {
               _units.resize(_o.frozenPreUnits, nullptr);
               _indexTree.fastRefill(_o.frozenPreUnits, FALSE);
            }
            return;
         }

         void fini()
         {
            _o = options();
            _indexTree.clear();
            for (UINT32 i = 0; i < _units.size(); ++i)
            {
               _bitmapUnit *unit = _units[i];
               if (nullptr != unit)
               {
                  SDB_OSS_DEL unit;
               }
            }
            _units.clear();
            _units.shrink_to_fit();
         }

         INT32 extendUnitNum(UINT32 num, BOOLEAN unzero=TRUE)
         {
            INT32 rc = SDB_OK;
            UINT32 current = _units.size();

            if (OSS_UNLIKELY(_indexTree.getMaxBitCount() <
                             (num + current)))
            {
               rc = SDB_VESSEL_OUT_OF_RESOURCE;
               goto error;
            }
            else if (OSS_UNLIKELY(0 == num))
            {
               goto done;
            }

            try
            {
               _units.reserve(num);
            }
            catch(const std::exception& e)
            {
               rc = SDB_OOM;
               goto error;
            }
            
            for (UINT32 i = 0; i < num; ++i)
            {
               _bitmapUnit *unit = SDB_OSS_NEW _bitmapUnit();
               if (OSS_UNLIKELY(nullptr == unit))
               {
                  rc = SDB_OOM;
                  goto error;
               }

               if (unzero)
               {
                  unit->bs.setAll();
                  unit->nonzeroBits = UNIT_SIZE;
               }

               _units.push_back(unit);
            }

            for (UINT32 i = 0; i < num; ++i)
            {
               _indexTree.pushBack(unzero);
            }
            
         done:
            return rc;
         error:
            for (UINT32 i = current; i < _units.size(); ++i)
            {
               _bitmapUnit *unit = _units[i];
               if (nullptr != unit)
               {
                  SDB_OSS_DEL unit;
               }
            }
            _units.resize(current);
            goto done;
         }

         INT32 appendUnit(const fixedBitset<UNIT_SIZE> &bs)
         {
            INT32 rc = SDB_OK;
            _bitmapUnit *unit = nullptr;

            if (OSS_UNLIKELY(_indexTree.getMaxBitCount() <
                             (_units.size() + 1)))
            {
               rc = SDB_VESSEL_OUT_OF_RESOURCE;
               goto error;
            }

            try
            {
               _units.reserve(1);
            }
            catch(const std::exception& e)
            {
               rc = SDB_OOM;
               goto error;
            }

            unit = SDB_OSS_NEW _bitmapUnit();
            if (OSS_UNLIKELY(nullptr == unit))
            {
               rc = SDB_OOM;
               goto error;
            }

            unit->bs = bs;
            unit->nonzeroBits = unit->bs.getNonzeroBitCount();
            _units.push_back(unit);
            _indexTree.pushBack(_o.minFreeReused <= unit->nonzeroBits);

         done:
            return rc;
         error:
            goto done;
         }

         INT32 findFirst()const
         {
            INT32 bitPos = -1;
            INT32 unitId = _indexTree.findFirst();
            if (0 <= unitId)
            {
               const _bitmapUnit *unit = _units[unitId];
               bitPos = unit->bs.findFirst();
               SDB_ASSERT(0 <= bitPos, "must be found");
               bitPos = (unitId * UNIT_SIZE) + bitPos;
            }

            return bitPos;
         }

         /// find a unzero bit and clear it.
         /// return -1 if no unzero bit found.
         INT32 pop()
         {
            INT32 bitPos = -1;
            INT32 unitId = _indexTree.findFirst();
            if (0 <= unitId)
            {
               _bitmapUnit *unit = _units[unitId];
               SDB_ASSERT(0 < unit->nonzeroBits, "impossible");

               INT32 first = unit->bs.findFirst();
               SDB_ASSERT(0 <= first, "must be found");
               unit->bs.clear(first);
   
               if (0 == --unit->nonzeroBits)
               {
                  _indexTree.reset(unitId);
               }

               bitPos = (unitId * UNIT_SIZE) + first;
            }

            return bitPos;
         }

         void set(UINT32 pos, BOOLEAN *beforeSet=nullptr)
         {
            SDB_ASSERT(pos < getTotalBitNum(), "out of bound");
            UINT32 unitId = getUnitId(pos);
            UINT32 bitInUnit = getBitInUnit(pos);

            SDB_ASSERT(unitId < _units.size(), "out of bound");
            _bitmapUnit *unit = _units[unitId];
            SDB_ASSERT(nullptr != unit, "unit may be frozen");

            BOOLEAN old = unit->bs.test(bitInUnit);
            if (nullptr != beforeSet)
            {
               *beforeSet = old;
            }

            if (!old)
            {
               unit->bs.set(bitInUnit);
               ++unit->nonzeroBits;
               if (_o.minFreeReused <= unit->nonzeroBits &&
                   !_indexTree.test(unitId))
               {
                  _indexTree.set(unitId);
               }
            }

            return;
         }

         void clear(UINT32 pos, BOOLEAN *beforeClear=nullptr)
         {
            SDB_ASSERT(pos < getTotalBitNum(), "out of bound");
            UINT32 unitId = getUnitId(pos);
            UINT32 bitInUnit = getBitInUnit(pos);

            SDB_ASSERT(unitId < _units.size(), "out of bound");
            _bitmapUnit *unit = _units[unitId];
            SDB_ASSERT(nullptr != unit, "unit may be frozen");

            BOOLEAN old = unit->bs.test(bitInUnit);
            if (nullptr != beforeClear)
            {
               *beforeClear = old;
            }

            if (old)
            {
               unit->bs.clear(bitInUnit);
               --unit->nonzeroBits;
               if (0 == unit->nonzeroBits)
               {
                  _indexTree.reset(unitId);
               }
            }

            return;
         }

         BOOLEAN test(UINT32 pos, BOOLEAN *freeToAlloc=nullptr)const
         {
            SDB_ASSERT(pos < getTotalBitNum(), "out of bound");
            UINT32 unitId = getUnitId(pos);
            UINT32 bitInUnit = getBitInUnit(pos);
            SDB_ASSERT(unitId < _units.size(), "out of bound");
            _bitmapUnit *unit = _units[unitId];
            SDB_ASSERT(nullptr != unit, "unit may be frozen");
            if (nullptr != freeToAlloc)
            {
               *freeToAlloc = _indexTree.test(unitId);
            }
            return unit->bs.test(bitInUnit);
         }

      private:
         UINT32 getUnitId(UINT32 bitPos)const
         {
            return bitPos / UNIT_SIZE;
         }
         UINT32 getBitInUnit(UINT32 bitPos)const
         {
            return bitPos % UNIT_SIZE;
         }

      private:
         options _o;
         bitsetTree _indexTree;
         _BITMAP_UNIT_VEC _units;

   };//class unitedBitmap

#pragma pack()
} // namespace vessel

} // namespace engine


#endif//VESSEL_UNITED_BITMAP_HPP_