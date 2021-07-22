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

   Source File Name = routePage.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#include "vessel/routePage.h"
#include "pdTrace.hpp"
#include "ossLikely.hpp"
#include "dms.hpp"

namespace engine
{
namespace vessel
{
   UINT32 getCapacityOfRoutePage(UINT32 pageSize)
   {
      UINT32 capacity = 0;
      UINT32 freeSize = 0;

      if (OSS_UNLIKELY(DMS_PAGE_SIZE32K != pageSize &&
                       DMS_PAGE_SIZE64K != pageSize))
      {
         SDB_ASSERT(FALSE, "invalid page size");
         goto done;
      }

      freeSize = pageSize - PAGE_HEAD_SIZE - PAGE_TAIL_SIZE - ROUTE_PAGE_HEAD_SIZE;
      freeSize = freeSize & 0xffffffe0;///32 bytes aligned
      capacity = freeSize >> 2; /// capacity = freeSize / 4
   done:
      return capacity;
   }

   BOOLEAN initRoutePage(UINT32 pageSize,
                         PAGE_ID pid,
                         PAGE_ID lpid,
                         PAGE_SNAPSHOT_VERION psv,
                         UINT32 logicalId,
                         INT32 lvl,
                         void *buf)
   {
      BOOLEAN r = FALSE;
      routePageHead *head = NULL;
      CHAR *ptr = NULL;
      UINT32 capacity = 0;
      if (OSS_UNLIKELY(!isValidPageSize(pageSize) ||
                       INVALID_PAGE_ID == pid ||
                       INVALID_PAGE_ID == lpid ||
                       INVALID_PAGE_SNAPSHOT_VERSION == psv ||
                       DMS_INVALID_LOGICCLID == logicalId ||
                       !isValidRoutePageLvl(lvl) ||
                       NULL == buf))
      {
         goto done;
      }

      capacity = getCapacityOfRoutePage(pageSize);
      if (0 == capacity)
      {
         goto done;
      }

      if (!initCommonPage(PAGE_TYPE_ROUTE, pageSize, pid, lpid, psv, buf))
      {
         goto done;
      }

      ptr = (CHAR *)buf;
      head = (routePageHead *)(ptr + PAGE_HEAD_SIZE);
      head->version = ROUTE_PAGE_VERSION;
      head->size = 0;
      head->logicalId = logicalId;
      head->lvl = lvl;
      ossMemset(ptr + PAGE_HEAD_SIZE + ROUTE_PAGE_HEAD_SIZE,
                0xFF, capacity << 2);
      r = TRUE;
   done:
      return r;
   }
}//namespace vessel
}//namespace engine