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

   Source File Name = sparsePidBitmap.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#include "vessel/sparsePidBitmap.h"
#include "pdTrace.hpp"
#include "ossLikely.hpp"
#include "ossLatchGuard.hpp"

namespace engine
{
namespace vessel
{
   void sparsePidBitmap::reset()
   {
      ossSLatchGuard guard(_latch, EXCLUSIVE);
      _tmap.clear();
   } 

   INT32 sparsePidBitmap::set(PAGE_ID pid)
   {
      INT32 rc = SDB_OK;
      if (OSS_UNLIKELY(INVALID_PAGE_ID == pid))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      else
      {
         ossSLatchGuard guard(_latch, EXCLUSIVE);
         _TRACKER_UNIT *unit = _ensureUnit(_getUnitId(pid));
         if (OSS_UNLIKELY(nullptr == unit))
         {
            PD_LOG(PDERROR, "failed to allocate mem.");
            rc = SDB_OOM;
            goto error;
         }

         unit->set(_getUnitPos(pid));
      }
   done:
      return rc;
   error:
      goto done;
   }

   void sparsePidBitmap::reset(PAGE_ID pid, BOOLEAN *oldVal)
   {
      SDB_ASSERT(INVALID_PAGE_ID != pid, "can not be invalid");
      if (nullptr != oldVal)
      {
         *oldVal = FALSE;
      }

      ossSLatchGuard guard(_latch, EXCLUSIVE);
      _TRACKER_UNIT *unit = _getUnit(_getUnitId(pid));
      if (nullptr != unit)
      {
         if (nullptr != oldVal)
         {
            *oldVal = unit->test(_getUnitPos(pid));
         }
         unit->clear(_getUnitPos(pid));
      }
      return;
   }

   BOOLEAN sparsePidBitmap::test(PAGE_ID pid)
   {
      SDB_ASSERT(INVALID_PAGE_ID != pid, "can not be invalid");
      ossSLatchGuard guard(_latch, SHARED);
      const _TRACKER_UNIT *unit = _getUnit(_getUnitId(pid));
      return nullptr != unit && unit->test(_getUnitPos(pid));
   }

   INT32 sparsePidBitmap::prepare(PAGE_ID pid)
   {
      INT32 rc = SDB_OK;
      if (OSS_UNLIKELY(INVALID_PAGE_ID == pid))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      else
      {
         ossSLatchGuard guard(_latch, EXCLUSIVE);
         _TRACKER_UNIT *unit = _ensureUnit(_getUnitId(pid));
         if (OSS_UNLIKELY(nullptr == unit))
         {
            PD_LOG(PDERROR, "failed to allocate mem.");
            rc = SDB_OOM;
            goto error;
         }
      }
   done:
      return rc;
   error:
      goto done;
   }

   BOOLEAN sparsePidBitmap::isEmpty()
   {
      ossSLatchGuard guard(_latch, SHARED);
      return _tmap.empty();
   }

   sparsePidBitmap::_TRACKER_UNIT *sparsePidBitmap::_ensureUnit(UINT32 unitId)
   {
      _TRACKER_UNIT *unit = nullptr;
      auto itr = _tmap.find(unitId);
      if (_tmap.end() != itr)
      {
         unit = itr->second.get();
      }
      else
      {
         unit = SDB_OSS_NEW _TRACKER_UNIT();
         if (OSS_LIKELY(nullptr != unit))
         {
            _tmap[unitId] = std::move(_TRACKER_UNIT_UPTR(unit));
         }
      }

      return unit;
   }

   sparsePidBitmap::_TRACKER_UNIT *sparsePidBitmap::_getUnit(UINT32 unitId)
   {
      _TRACKER_UNIT *unit = nullptr;
      auto itr = _tmap.find(unitId);
      if (_tmap.end() != itr)
      {
         unit = itr->second.get();
      }

      return unit;
   }

   const sparsePidBitmap::_TRACKER_UNIT *sparsePidBitmap::_getUnit(UINT32 unitId)const
   {
      const _TRACKER_UNIT *unit = nullptr;
      auto itr = _tmap.find(unitId);
      if (_tmap.cend() != itr)
      {
         unit = itr->second.get();
      }

      return unit;
   }

   sparsePidBitmap::iterator sparsePidBitmap::begin()const
   {
      iterator res;
      for (auto itr = _tmap.cbegin(); itr != _tmap.cend(); ++itr)
      {
         UINT32 unitId = _tmap.cbegin()->first;
         const _TRACKER_UNIT *unit = _tmap.cbegin()->second.get();
         INT32 first = unit->findFirst();
         if (0 <= first)
         {
            static_assert(512 == _UNIT_SIZE, "must be 512");
            res._value = (unitId << 9) + (UINT32)first;
            break;
         }
      }
      return res;
   }

   void sparsePidBitmap::next(iterator &itr)const
   {
      PAGE_ID pid = itr._value;
      itr.reset();
      if (INVALID_PAGE_ID != pid)
      {
         UINT32 minUnitId = _getUnitId(pid);
         _TRACKER_UNIT_MAP::const_iterator i = _tmap.lower_bound(minUnitId);
         for (; i != _tmap.cend(); ++i)
         {
            INT32 prev = i->first == minUnitId ?
                         (INT32)_getUnitPos(pid) : -1;
            INT32 pos = i->second->findNext(prev);
            if (0 <= pos)
            {
               static_assert(512 == _UNIT_SIZE, "must be 512");
               itr._value = (i->first << 9) + (UINT32)pos;
               break;
            }
         }
      }

      return;
   }
} // namespace vessel

} // namespace engine
