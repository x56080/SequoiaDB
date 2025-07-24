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

   Source File Name = routePage.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

*******************************************************************************/
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