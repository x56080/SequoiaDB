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

   Source File Name = spaceManagementPage.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#include "vessel/spaceManagementPage.h"
#include "pdTrace.hpp"
#include "ossLikely.hpp"
#include "dms.hpp"
#include "vessel/storageFileDef.h"
#include "vessel/bitMapUtils.h"

namespace engine
{
namespace vessel
{
   BOOLEAN getSMPCapacityOrCount(UINT32 pageSize,
                                 UINT32 *capacity,
                                 UINT32 *count)
   {
      BOOLEAN r = FALSE;
      UINT32 fileCapacity = 0;
      UINT32 pageCapacity = 0;
      UINT32 realCapacity = 0;

      switch (pageSize)
      {
      case DMS_PAGE_SIZE8K:
         fileCapacity = STORAGE_FILE_SIZE >> 13;
         break;
      case DMS_PAGE_SIZE16K:
         fileCapacity = STORAGE_FILE_SIZE >> 14;
         break;
      case DMS_PAGE_SIZE32K:
         fileCapacity = STORAGE_FILE_SIZE >> 15;
         break;
      case DMS_PAGE_SIZE64K:
         fileCapacity = STORAGE_FILE_SIZE >> 16;
         break;
      default:
         goto done;
         break;
      }

      ///We want to set capacity as power of 2.
      ///But because of page head and tail, only half page can be used.
      pageCapacity = (pageSize << 2);/// pageCapacity = pageSize / 2 * 8;
      realCapacity = fileCapacity <= pageCapacity ? fileCapacity : pageCapacity;
      if (0 != (fileCapacity & (realCapacity - 1)))
      {
         goto done;
      }
      if (NULL != capacity)
      {
         *capacity = realCapacity;
      }
      if (NULL != count)
      {
         *count = fileCapacity / realCapacity;
      }
      r = TRUE;
      
   done:
      return r;
   }

   BOOLEAN initSmp(UINT32 pageSize, PAGE_ID pid, UINT32 occupied, CHAR *buf)
   {
      BOOLEAN r = FALSE;
      spaceManagementPageHead *head = NULL;
      UINT32 capacity = 0;
      UINT32 bitsCount = 0;

      if (OSS_UNLIKELY(!isValidPageSize(pageSize) ||
                       INVALID_PAGE_ID == pid ||
                       NULL == buf))
      {
         goto done;
      }

      if (OSS_UNLIKELY(!getSMPCapacityOrCount(pageSize, &capacity, NULL)))
      {
         goto done;
      }

      if (OSS_UNLIKELY(capacity < occupied))
      {
         goto done;
      }

      initCommonPage(PAGE_TYPE_SMP, pageSize, pid, buf);
      head = (spaceManagementPageHead *)(buf + PAGE_HEAD_LEN);
      head->version = SMP_VERSION_1;
      head->flags = 0;
      head->free = capacity - occupied;
      head->pad = 0;

      bitsCount = capacity >> 6;
      ossMemset((buf + PAGE_HEAD_LEN + SMP_HEAD_LEN), 0xFF, (capacity >> 3));
      for (UINT32 i = 0; i < occupied; ++i)
      {
         if (!setNotFreeIfFree64(bitsCount, (UINT64 *)(buf + PAGE_HEAD_LEN + SMP_HEAD_LEN), i))
         {
            goto done;
         }
      }
      r = TRUE;
   done:
      return r;
   }

}//namespace vessel
}//namespace engine