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
   BOOLEAN isPageCrashed(ossValuePtr ptr, UINT32 pageSize)
   {
      SDB_ASSERT(0 != ptr, "can not be null");
      SDB_ASSERT(isValidPageSize(pageSize), "must be valid");
      const pageHead *head = (const pageHead *)ptr;
      const UINT64 *tail =(const UINT64 *)(ptr + pageSize - PAGE_TAIL_SIZE);
      CHAR eyecacher[2] = {};
      getPageEyeCatcher(eyecacher[0], eyecacher[1]);

      return head->eyeCatcher[0] != eyecacher[0] ||
          head->eyeCatcher[1] != eyecacher[1] ||
          head->version != PAGE_VERSION_1 ||
          head->size != pageSize ||
          head->type == INVALID_PAGE_TYPE ||
          head->pid == INVALID_PAGE_ID ||
          head->lpid == INVALID_PAGE_ID ||
          head->lsn != *tail ||
          head->psv == INVALID_PAGE_SNAPSHOT_VERSION ||
          head->pad0 != 0 ||
          head->pad1 != 0 ||
          !head->inUsed();
   }

   INT32 validatePage(ossValuePtr ptr,
                      PAGE_TYPE type,
                      UINT32 pageSize,
                      PAGE_ID pid,
                      PAGE_ID lpid,
                      PAGE_SNAPSHOT_VERION psv)
   {
      INT32 rc = SDB_OK;
      const pageHead *head = (const pageHead *)ptr;

      if (0 == ptr ||
          INVALID_PAGE_TYPE == type ||
          !isValidPageSize(pageSize) ||
          INVALID_PAGE_ID == pid ||
          INVALID_PAGE_ID == lpid ||
          INVALID_PAGE_SNAPSHOT_VERSION == psv)
      {
         SDB_ASSERT(FALSE, "should not be invalid");
         rc = SDB_INVALIDARG;
         goto error;
      }
      else if (isPageCrashed(ptr, pageSize))
      {
         rc = SDB_VESSEL_PAGE_CRASHED;
         goto error;
      }
      else if (type != head->type ||
               !head->inUsed() ||
               pageSize != head->size ||
               pid != head->pid ||
               lpid != head->lpid ||
               psv != head->psv)
      {
         rc = SDB_VESSEL_PAGE_HEAD_NOT_MATCH;
         goto error;
      }
   done:
      return rc;
   error:
      goto done;
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
         SDB_ASSERT(FALSE, "impossible");
         goto done;
      }

      bodySize = pageSize - PAGE_HEAD_SIZE - PAGE_TAIL_SIZE;

   done:
      return bodySize;
   }

   BOOLEAN initCommonPage(UINT16 pageType,
                          UINT32 pageSize,
                          PAGE_ID pid,
                          PAGE_ID lpid,
                          PAGE_SNAPSHOT_VERION psv,
                          void *buf)
   {
      BOOLEAN r = FALSE;
      pageHead *head = (pageHead *)buf;
      UINT64 *tail = NULL;

      if (OSS_UNLIKELY(INVALID_PAGE_TYPE == pageType))
      {
         goto done;
      }
      else if (OSS_UNLIKELY(!isValidPageSize(pageSize)))
      {
         goto done;
      }
      else if (OSS_UNLIKELY(INVALID_PAGE_ID == pid))
      {
         goto done;
      }
      else if (OSS_UNLIKELY(INVALID_PAGE_SNAPSHOT_VERSION == psv))
      {
         goto done;
      }
      else if (OSS_UNLIKELY(NULL == buf))
      {
         goto done;
      }

      ossMemset(buf, 0, pageSize);
      getPageEyeCatcher(head->eyeCatcher[0], head->eyeCatcher[1]);
      head->version = PAGE_VERSION_1;
      head->type = pageType;
      head->flags = 0;
      head->size = pageSize;
      head->pid = pid;
      head->lpid = lpid;
      head->lsn = DPS_INVALID_LSN_OFFSET;
      head->psv = psv;
      head->setInUsed();
      tail = (UINT64 *)((CHAR *)buf + pageSize - PAGE_TAIL_SIZE);
      *tail = DPS_INVALID_LSN_OFFSET;

      r = TRUE;

   done:
      SDB_ASSERT(r, "must be ok");
      return r;
   }

   BOOLEAN updatePageLsn(ossValuePtr ptr,
                         DPS_LSN_OFFSET lsn)
   {
      BOOLEAN r = FALSE;
      if (OSS_UNLIKELY(0 == ptr))
      {
         goto done;
      }
      else if (OSS_UNLIKELY(!isValidPageSize(((pageHead *)ptr)->size)))
      {
         goto done;
      }

      ((pageHead *)ptr)->lsn = lsn;
      *((UINT64 *)(ptr + ((pageHead *)ptr)->size - sizeof(UINT64))) = lsn;
      r = TRUE;
   done:
      return r;
   }
}//namespace vessel
}//namespace engine