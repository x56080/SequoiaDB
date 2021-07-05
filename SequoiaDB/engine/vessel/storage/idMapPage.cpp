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
   /*
   BOOLEAN get64AlignedIMPCapacity(UINT32 pageSize, UINT32 &capacity)
   {
      BOOLEAN r = FALSE;

      if (OSS_UNLIKELY(!isValidPageSize(pageSize)))
      {
         goto done;
      }

      capacity = (pageSize - PAGE_HEAD_SIZE - PAGE_TAIL_SIZE - ID_MAP_PAGE_HEAD_SIZE) /
                 sizeof(idMapSlot);
      capacity &= 0xFFFFFFC0;
      r = TRUE;
   done:
      return r;
   }
   */

   BOOLEAN initIdMapPage(UINT32 pageSize,
                         PAGE_ID pid,
                         PAGE_ID lpid,
                         PAGE_SNAPSHOT_VERION psv,
                         void *buf)
   {
      BOOLEAN r = FALSE;
      idMapPageHead *head = NULL;
      CHAR *ptr = (CHAR *)buf;
      UINT32 capacity = 0;

      if (OSS_UNLIKELY(!get64AlignedIMPCapacity(pageSize, capacity)))
      {
         goto done;
      }
      if (OSS_UNLIKELY(!initCommonPage(PAGE_TYPE_ID_MAP, pageSize, pid, lpid, psv, buf)))
      {
         goto done;
      }

      head = (idMapPageHead *)(ptr + PAGE_HEAD_SIZE);
      head->version = CURRENT_ID_MAP_PAGE_VERSION;
      head->flags = 0;
      head->pad = 0;
      ossMemset(ptr + PAGE_HEAD_SIZE + ID_MAP_PAGE_HEAD_SIZE,
                0xFF, capacity * sizeof(idMapSlot));
      r = TRUE;
   done:
      return r;
   }

   idMapSlot getIdMapSlot(ossValuePtr ptr, UINT32 slot)
   {
      idMapSlot obj;
      SDB_ASSERT(0 != ptr, "can not be null");
      SDB_ASSERT(slot < ID_MAP_PAGE_CAPACITY, "can not be invalid");
      obj = *((const idMapSlot *)ptr + slot); 
      return obj;
   }
}//namespace vessel
}//namespace engine