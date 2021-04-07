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

   Source File Name = extentDef.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#include "vessel/extentDef.h"

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
}//namespace vessel
}//namespace engine