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

   Source File Name = lpageMapping.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#include "vessel/lpageMapping.h"
#include "pdTrace.hpp"
#include "vessel/idMapFile.h"
#include "vessel/lpageHashTable.h"
#include "ossLatchGuard.hpp"
#include "vessel/idMapPage.h"

namespace engine
{
namespace vessel
{
   lpageMapping::lpageMapping()
   {

   }

   lpageMapping::~lpageMapping()
   {
      fini();
   }

   void lpageMapping::fini()
   {
      _base = nullptr;
      _basePageCount = 0;
      _workingTable.clear();
      _archivedTable.clear();
      _counter.store(0, std::memory_order_relaxed);
   }

   void lpageMapping::rebase(const idMapFile *base)
   {
      SDB_ASSERT(nullptr != base && base->isOpen(), "can not be invalid");
      ossRWMutexGuard guard(&_mutex, EXCLUSIVE);
      _basePageCount = base->getTotalPageCount();
      _base = base;
      _archivedTable.clear();
      return;
   }

   INT32 lpageMapping::set(PAGE_ID lpid, const lpageDescriptor &desc)
   {
      INT32 rc = SDB_OK;
      if (OSS_UNLIKELY(nullptr == _base))
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }
      else if (OSS_UNLIKELY(INVALID_PAGE_ID == lpid))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      {
         ossRWMutexGuard guard(&_mutex, SHARED);
         rc = _workingTable.set(lpid, desc);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to update page[%d], rc:%d", rc);
            goto error;
         }

         _counter.fetch_add(1, std::memory_order_relaxed);
      }
   done:
      return rc;
   error:
      goto done;
   }

   INT32 lpageMapping::get(PAGE_ID lpid, lpageDescriptor &desc)
   {
      INT32 rc = SDB_OK;
      desc.reset();

      if (OSS_UNLIKELY(INVALID_PAGE_ID == lpid))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      else if (OSS_UNLIKELY(!isReady()))
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }

      {
         ossRWMutexGuard guard(&_mutex, SHARED);
         if (_workingTable.get(lpid, desc))
         {
            if (!desc.isValid())
            {
               rc = SDB_VESSEL_LOGICAL_PAGE_UNMAPPED;
               goto error;
            }

            goto done;
         }

         if (_archivedTable.get(lpid, desc))
         {
            if (!desc.isValid())
            {
               rc = SDB_VESSEL_LOGICAL_PAGE_UNMAPPED;
               goto error;
            }

            goto done;
         }

         rc = getFromBase(lpid, desc);
         if (SDB_OK != rc)
         {
            goto error;
         }

      }
   done:
      return rc;
   error:
      goto done;
   }

   void lpageMapping::archive()
   {
      SDB_ASSERT(isReady(), "must be ready");
      SDB_ASSERT(_archivedTable.isEmpty(), "must be empty");
      ossRWMutexGuard guard(&_mutex, EXCLUSIVE);
      if (!_workingTable.isEmpty())
      {
         _archivedTable = std::move(_workingTable);
      }
      _counter.store(0, std::memory_order_relaxed);
   }

   void lpageMapping::dumpArchivedTable(ossPoolMap<PAGE_ID, lpageDescriptor> &m)
   {
      ossRWMutexGuard guard(&_mutex, SHARED);
      _archivedTable.dump(m);
   }

   void lpageMapping::dumpArchivedTable(DELTA_PAGE_LIST &dpl)
   {
      ossRWMutexGuard guard(&_mutex, SHARED);
      _archivedTable.dump(dpl);
   }

   INT32 lpageMapping::getFromBase(PAGE_ID lpid, lpageDescriptor &desc)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(INVALID_PAGE_ID != lpid, "can not be invalid");
      PAGE_ID pid = getImpPidOfLpid(lpid);
      ossValuePtr ptr = 0;
      idMapSlot slot;

      if (_basePageCount <= pid)
      {
         rc = SDB_VESSEL_LOGICAL_PAGE_UNMAPPED;
         goto error;
      }

      rc = _base->getPagePtr(pid, ptr);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to get page[%d] ptr:%d", pid, rc);
         goto error;
      }

      slot = getIdMapSlot(ptr, getIdMapSlotNo(lpid));
      if (slot.isFree())
      {
         rc = SDB_VESSEL_LOGICAL_PAGE_UNMAPPED;
         goto error;
      }

      desc.reset(slot.pid, -1, slot.psv);
   done:
      return rc;
   error:
      goto done;
   }

   UINT64 lpageMapping::getWorkingTableBufferSize()const
   {
      return _workingTable.getBufferSize();
   }

} // namespace vessel

} // namespace engine

