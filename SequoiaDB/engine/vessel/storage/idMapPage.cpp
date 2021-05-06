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

   Source File Name = idMapPage.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#include "vessel/idMapPage.h"
#include "dms.hpp"

namespace engine
{
namespace vessel
{
   BOOLEAN get64AlignedIMPCapacity(UINT32 pageSize, UINT32 &capacity)
   {
      BOOLEAN r = FALSE;

      if (DMS_PAGE_SIZE32K != pageSize)
      {
         goto done;
      }

      capacity = (pageSize - PAGE_HEAD_LEN - PAGE_TAIL_LEN - ID_MAP_PAGE_HEAD_LEN) /
                 sizeof(idMapSlot);
      capacity &= 0xFFFFFFC0;
      r = TRUE;
   done:
      return r;
   }

   BOOLEAN initIdMapPage(UINT32 pageSize, PAGE_ID pid, void *buf)
   {
      BOOLEAN r = FALSE;
      idMapPageHead *head = NULL;
      CHAR *ptr = (CHAR *)buf;
      UINT32 capacity = 0;

      if (OSS_UNLIKELY(!isValidPageSize(pageSize) ||
                       INVALID_PAGE_ID == pid ||
                       NULL == buf))
      {
         goto done;
      }

      if (!get64AlignedIMPCapacity(pageSize, capacity))
      {
         goto done;
      }

      initCommonPage(PAGE_TYPE_ID_MAP, pageSize, pid, buf);
      head = (idMapPageHead *)(ptr + PAGE_HEAD_LEN);
      head->version = CURRENT_ID_MAP_PAGE_VERSION;
      head->free = capacity;
      ossMemset(ptr + PAGE_HEAD_LEN + ID_MAP_PAGE_HEAD_LEN,
                0xFF, capacity * sizeof(idMapSlot));
      r = TRUE;
   done:
      return r;
   }
}//namespace vessel
}//namespace engine