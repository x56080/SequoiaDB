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
      SDB_ASSERT(DMS_PAGE_SIZE32K == pageSize ||
                 DMS_PAGE_SIZE64K == pageSize, "invalid page size");
      UINT32 capacity = 0;
      UINT32 freeSize = 0;

      if (OSS_UNLIKELY(DMS_PAGE_SIZE32K != pageSize &&
                       DMS_PAGE_SIZE64K != pageSize))
      {
         goto done;
      }

      freeSize = pageSize - PAGE_HEAD_LEN - PAGE_TAIL_LEN - ROUTE_PAGE_HEAD_LEN;
      freeSize = freeSize & 0xffffffe0;///32 bytes aligned
      capacity = freeSize >> 2; /// capacity = freeSize / 4 
   done:
      return capacity;
   }

   BOOLEAN initRoutePage(UINT32 pageSize,
                         PAGE_ID lpid,
                         SNAPSHOT_ID snapshot,
                         UINT32 logicalId,
                         void *buf)
   {
      BOOLEAN r = FALSE;
      routePageHead *head = NULL;
      CHAR *ptr = NULL;
      UINT32 capacity = 0;
      if (OSS_UNLIKELY(!isValidPageSize(pageSize) ||
                       INVALID_PAGE_ID == lpid ||
                       DMS_INVALID_LOGICCLID == logicalId ||
                       NULL == buf))
      {
         goto done;
      }

      capacity = getCapacityOfRoutePage(pageSize);
      if (0 == capacity)
      {
         goto done;
      }

      initCommonPage(PAGE_TYPE_ROUTE, pageSize, lpid, snapshot, buf);
      ptr = (CHAR *)buf;
      head = (routePageHead *)(ptr + PAGE_HEAD_LEN);
      head->version = ROUTE_PAGE_VERSION;
      head->count = 0;
      head->logicalId = logicalId;
      ossMemset(ptr + PAGE_HEAD_LEN + ROUTE_PAGE_HEAD_LEN,
                0xFF, capacity << 2);
      r = TRUE;
   done:
      return r;
   }
}//namespace vessel
}//namespace engine