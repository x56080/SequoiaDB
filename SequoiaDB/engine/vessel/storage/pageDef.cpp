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

   Source File Name = pageDef.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#include "vessel/pageDef.h"
#include "dms.hpp"

namespace engine
{
namespace vessel
{
   BOOLEAN validatePageHeadAndTail(ossValuePtr ptr, UINT32 pageSize)
   {
      BOOLEAN r = FALSE;
      const pageHead *head = NULL;
      UINT64 tail = DPS_INVALID_LSN_OFFSET;
      if (OSS_UNLIKELY(0 == ptr || (pageSize < (PAGE_HEAD_LEN + sizeof(UINT64)))))
      {
         goto done;
      }

      head = (const pageHead *)ptr;
      tail = *((const UINT64 *)(ptr + pageSize - sizeof(UINT64)));
      r = head->lsn == tail &&
          INVALID_PAGE_TYPE != head->type &&
          PAGE_VERSION_1 == head->version &&
          head->inUsed() &&
          head->size == pageSize;
   done:
      return r;
   }

   BOOLEAN isValidPageSize(UINT32 pageSize)
   {
      BOOLEAN r = FALSE;
      switch (pageSize)
      {
      case DMS_PAGE_SIZE4K:
         r = TRUE;
         break;
      case DMS_PAGE_SIZE8K:
         r = TRUE;
         break;
      case DMS_PAGE_SIZE16K:
         r = TRUE;
         break;
      case DMS_PAGE_SIZE32K:
         r = TRUE;
         break;
      case DMS_PAGE_SIZE64K:
         r = TRUE;
         break;
      case DMS_PAGE_SIZE128K:
         r = TRUE;
         break;
      case DMS_PAGE_SIZE256K:
         r = TRUE;
         break;
      case DMS_PAGE_SIZE512K:
         r = TRUE;
         break;
      default:
         break;
      }
      return r;
   }

   UINT32 getPageBodySize(UINT32 pageSize)
   {
      UINT32 bodySize = 0;
      if (OSS_UNLIKELY(!isValidPageSize(pageSize)))
      {
         goto done;
      }

      bodySize = pageSize - PAGE_HEAD_LEN - PAGE_TAIL_LEN;

   done:
      return bodySize;
   }

   void initCommonPage(UINT16 pageType,
                       UINT32 pageSize,
                       UINT32 pageID,
                       void *buf)
   {
      SDB_ASSERT(INVALID_PAGE_TYPE != pageType, "can not be invalid");
      SDB_ASSERT(isValidPageSize(pageSize), "can not be invalid");
      SDB_ASSERT(INVALID_PAGE_ID != pageID, "can not be invalid");
      SDB_ASSERT(NULL != buf, "can not be null");
      ossMemset(buf, 0, pageSize);
      pageHead *head = (pageHead *)buf;
      getEyeCatcher(pageType, head->eyeCatcher[0], head->eyeCatcher[1]);
      head->version = PAGE_VERSION_1;
      head->type = pageType;
      head->size = pageSize;
      head->pageID = pageID;
      head->lsn = DPS_INVALID_LSN_OFFSET;
      head->setInUsed();
      UINT64 *tail = (UINT64 *)((CHAR *)buf + pageSize - PAGE_TAIL_LEN);
      *tail = DPS_INVALID_LSN_OFFSET;
      return;
   }
}//namespace vessel
}//namespace engine