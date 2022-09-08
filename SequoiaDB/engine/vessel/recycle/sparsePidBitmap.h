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

   Source File Name = sparsePidBitmap.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/
#ifndef VESSEL_PID_SPARSE_BITMAP_H_
#define VESSEL_PID_SPARSE_BITMAP_H_

#include "ossMemPool.hpp"
#include "vessel/fixedBitset.hpp"
#include "vessel/pageIdentifier.h"
#include "ossLatch.hpp"

namespace engine
{
namespace vessel
{
   ///TODO: use roaring bitmap to manage pids.
   class sparsePidBitmap : public SDBObject
   {
      public:
         sparsePidBitmap(ossSLatch *latch=nullptr):_latch(latch){}
         ~sparsePidBitmap() = default;
         sparsePidBitmap(const sparsePidBitmap &) = delete;
         sparsePidBitmap &operator=(const sparsePidBitmap &) = delete;
         sparsePidBitmap(sparsePidBitmap &&o):
         _tmap(std::move(o._tmap)){}
         sparsePidBitmap &operator=(sparsePidBitmap &&o)
         {
            reset();
            _tmap = std::move(o._tmap);
            return *this;
         }

      public:
         void reset();
         INT32 set(PAGE_ID pid);
         void reset(PAGE_ID pid, BOOLEAN *oldVal=nullptr);
         BOOLEAN test(PAGE_ID pid);
         INT32 prepare(PAGE_ID pid);
         BOOLEAN isEmpty();
      private:
         static constexpr UINT32 _UNIT_SIZE = 512;
         using _TRACKER_UNIT = fixedBitset<_UNIT_SIZE>;
         using _TRACKER_UNIT_UPTR = std::unique_ptr<_TRACKER_UNIT>;
         using _TRACKER_UNIT_MAP = ossPoolMap<UINT32, _TRACKER_UNIT_UPTR>;

      public:
         /// WARNING: iterator not thread-safe!
         class iterator : public SDBObject
         {
            friend class sparsePidBitmap;
            public:
               BOOLEAN isValid()const {return INVALID_PAGE_ID != _value;}
               void reset(){ _value = INVALID_PAGE_ID;}
               PAGE_ID getValue()const {return _value;}

            private:
               PAGE_ID _value = INVALID_PAGE_ID;
         };

         iterator begin()const;
         void next(iterator &itr)const;

      private:
         OSS_INLINE UINT32 _getUnitId(PAGE_ID pid)const
         {
            static_assert(512 == _UNIT_SIZE, "must be 512");
            return pid >> 9;
         }
         OSS_INLINE UINT32 _getUnitPos(PAGE_ID pid)const
         {
            return pid & (_UNIT_SIZE - 1);
         }

         _TRACKER_UNIT *_ensureUnit(UINT32 unitId);
         _TRACKER_UNIT *_getUnit(UINT32 unitId);
         const _TRACKER_UNIT *_getUnit(UINT32 unitId)const;

      private:
         ossSLatch *_latch = nullptr;
         _TRACKER_UNIT_MAP _tmap;
   };//class sparsePidBitmap
} // namespace vessel

} // namespace engine


#endif//VESSEL_PID_SPARSE_BITMAP_H_