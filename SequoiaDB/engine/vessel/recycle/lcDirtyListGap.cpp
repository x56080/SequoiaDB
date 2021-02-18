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

   Source File Name = lcDirtyListGap.cpp

   Descriptive Name =

   When/how to use: this program may be used on binary and text-formatted
   versions of PMD component. This file contains functions for agent processing.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#include "vessel/lcDirtyListGap.h"
#include "dpsLogDef.hpp"
#include "pdTrace.hpp"

namespace engine
{
namespace vessel
{
   INT32 lcDirtyListGap::setup(UINT64 lsn)
   {
      INT32 rc = SDB_OK;
      if (DPS_INVALID_LSN_OFFSET == lsn)
      {
         PD_LOG(PDERROR, "can not init gap with invalid lsn");
         rc = SDB_INVALIDARG;
         goto error;
      }
   
      _mutex.lock();
      _gaps.clear();
      _gaps.push_back(lsnGap(lsn, DPS_INVALID_LSN_OFFSET));
      _mutex.unlock();

   done:
      return rc;
   error:
      goto done;
   }

   INT32 lcDirtyListGap::teardown()
   {
      _mutex.lock();
      _gaps.clear();
      _mutex.unlock();
      return SDB_OK;
   }

   INT32 lcDirtyListGap::removeGap(UINT64 lsn, UINT32 len)
   {
      INT32 rc = SDB_OK;
      std::list<lsnGap>::iterator itr;
      BOOLEAN locked = FALSE;
      UINT32 size = 0;
      if (DPS_INVALID_LSN_OFFSET == lsn || 0 == len)
      {
         PD_LOG(PDERROR, "can not remove gap with invalid log");
         rc = SDB_INVALIDARG;
         goto error;
      }

      _mutex.lock();
      locked = TRUE;
      for (itr = _gaps.begin(); itr != _gaps.end(); ++itr)
      {
         if (lsn > itr->end)
         {
            continue;
         }
         else if (lsn < itr->end)
         {
            UINT64 end = lsn + len;
            if (lsn == itr->begin && end == itr->end)
            {
               _gaps.erase(itr);
               goto done;
            }
            else if (lsn == itr->begin && end < itr->end)
            {
               itr->begin = end;
               goto done;
            }
            else if (lsn > itr->begin && end == itr->end)
            {
               itr->end = lsn;
               goto done;
            }
            else if (lsn > itr->begin && end < itr->end)
            {
               _gaps.insert(itr, lsnGap(itr->begin, lsn));
               itr->begin = end;
               goto done;
            }
            else
            {
               PD_LOG(PDERROR, "failed to remove gap with lsn:%lld, len:%d, current gap begin:%lld, gap end:%lld",
                   lsn, len, itr->begin, itr->end);
               rc = SDB_VESSEL_INTERNAL_ERR;
               goto error;
            }
         }
         else /// lsn == itr->end
         {
            PD_LOG(PDERROR, "failed to remove gap with lsn:%lld, len:%d, current gap begin:%lld, gap end:%lld",
                   lsn, len, itr->begin, itr->end);
            rc = SDB_VESSEL_INTERNAL_ERR;
            goto error;
         }
      }

      size = _gaps.size();
      _mutex.unlock();
      locked = FALSE;
      PD_LOG(PDERROR, "failed to remove gap with lsn:%lld, current list size:%d", lsn, size);
      rc = SDB_VESSEL_INTERNAL_ERR;
      goto error;

   done:
      if (locked)
      {
         _mutex.unlock();
      }
      return rc;
   error:
      goto done;
   }

   UINT64 lcDirtyListGap::getMinGapLSN()
   {
      UINT64 lsn = DPS_INVALID_LSN_OFFSET;
      _mutex.lock();
      if (!_gaps.empty())
      {
         lsn = _gaps.begin()->begin;
      }
      else
      {
         PD_LOG(PDERROR, "gaps should not be empty!");
      }
      _mutex.unlock();
      return lsn;
   }
} /// end of namespace vessel
} /// end of namespace engine
