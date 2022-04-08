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

   Source File Name = lobMetaDataFile.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#include "vessel/lobMetaDataFile.h"
#include "pdTrace.hpp"
#include "ossLikely.hpp"
#include "vessel/lobmUberBlock.h"
#include "vessel/strictBuffer.h"

namespace engine
{
namespace vessel
{
   INT32 lobMetaDataFile::_open(BOOLEAN isCreating)
   {
      INT32 rc = SDB_OK;
      if (isCreating)
      {
         rc = _create();
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to create new lobm file:%d", rc);
            goto error;
         }
      }
      else
      {
         rc = _open();
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to open lobm file:%d", rc);
            goto error;
         }
      }
   done:
      return rc;
   error:
      goto done;
   }

   void lobMetaDataFile::_close()
   {
      _scanner.reset();
   }

   INT32 lobMetaDataFile::_create()
   {
      INT32 rc = SDB_OK;
      UINT64 *sme = nullptr;

      if (0 != storageFile::getSegmentCount())
      {
         PD_LOG(PDERROR, "invalid data segment count:%d", storageFile::getSegmentCount());
         rc = SDB_VESSEL_INTERNAL_ERR;
         goto error;
      }

      sme = (UINT64 *)getReservedAreaPtr();
      if (nullptr == sme)
      {
         PD_LOG(PDERROR, "failed to get sme ptr");
         rc = SDB_VESSEL_INTERNAL_ERR;
         goto error;
      }

      rc = allocateNewSegment();
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to extend lobm file[%s], rc:%d",
               getFullPath(), rc);
         goto error;
      }

      _scanner.init(sme, PAGE_COUNT_PER_SEG, FALSE);
   done:
      return rc;
   error:
      goto done;
   }

   INT32 lobMetaDataFile::_open()
   {
      INT32 rc = SDB_OK;
      UINT64 *sme = nullptr;

      if (0 == getSegmentCount())
      {
         PD_LOG(PDERROR, "has no data segment at all");
         rc = SDB_VESSEL_INTERNAL_ERR;
         goto error;
      }

      sme = (UINT64 *)getReservedAreaPtr();
      if (nullptr == sme)
      {
         PD_LOG(PDERROR, "failed to get sme ptr");
         rc = SDB_VESSEL_INTERNAL_ERR;
         goto error;
      }

      _scanner.load(sme, getSegmentCount() * PAGE_COUNT_PER_SEG);
   done:
      return rc;
   error:
      goto done;
   }

   INT32 lobMetaDataFile::reservePage(PAGE_ID &pid, mmapPagePointer &ptr)
   {
      INT32 rc = SDB_OK;
      INT32 pos = -1;

      pid = INVALID_PAGE_ID;
      ptr.reset();

      std::unique_lock<std::mutex> guard(_mutex);

      if (OSS_UNLIKELY(!isOpen()))
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }

      if (0 == _scanner.getNonZeroedCount())
      {
         rc = allocateNewSegment();
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to extend new segment:%d", rc);
            goto error;
         }

         _scanner.extend(PAGE_COUNT_PER_SEG, FALSE);
      }

      if (_scanner.findAndClearNext(pos))
      {
         ossValuePtr p = 0;
         pid = static_cast<PAGE_ID>(pos);
         rc = getPagePtr(pid, p);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to get page[%d] ptr:%d", pid, rc);
            /// we do not rollback sme here.
            goto error;
         }
         ptr.reset(p);
      }
      else
      {
         PD_LOG(PDERROR, "no free page found");
         rc = SDB_VESSEL_INTERNAL_ERR;
         goto error;
      }
   done:
      return rc;
   error:
      pid = INVALID_PAGE_ID;
      ptr.reset();
      goto done;
   }

   void lobMetaDataFile::freePage(PAGE_ID pid)
   {
      SDB_ASSERT(isOpen(), "can not be close");
      if (INVALID_PAGE_ID != pid && pid < _scanner.getCapacity())
      {
         std::unique_lock<std::mutex> guard(_mutex);
         _scanner.setBit(pid, FALSE);
      }
   }

   void lobMetaDataFile::freePages(UINT32 size, const PAGE_ID *pids)
   {
      SDB_ASSERT(isOpen(), "can not be close");
      if (0 < size && nullptr != pids)
      {
         std::unique_lock<std::mutex> guard(_mutex);
         for (UINT32 i = 0; i < size; ++i)
         {
            if (INVALID_PAGE_ID != pids[i] &&
                pids[i] < _scanner.getCapacity())
            {
               _scanner.setBit(pids[i], FALSE);
            }
         }
      }
   }
} // namespace vessel

} // namespace engine

