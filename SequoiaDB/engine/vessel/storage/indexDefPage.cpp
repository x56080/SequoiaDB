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

   Source File Name = indexDefPage.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#include "vessel/indexDefPage.h"

namespace engine
{
namespace vessel
{
   BOOLEAN initIndexDefPage(UINT32 pageSize, UINT32 lpid,
                            const indexDefRecord &record,
                            CHAR *buf)
   {
      BOOLEAN r = FALSE;
      indexDefRecord *head = NULL;
      if (!isValidPageSize(pageSize))
      {
         goto done;
      }
      else if (INVALID_PAGE_ID == lpid)
      {
         goto done;
      }
      else if (!record.isValid())
      {
         goto done;
      }
      else if (NULL == buf)
      {
         goto done;
      }

      initCommonPage(PAGE_TYPE_INDEX_DEF, pageSize, lpid, buf);
      head = (indexDefRecord *)(buf + PAGE_HEAD_LEN);
      *head = record;
   done:
      return r;
   }
}//namespace vessel
}//namespace engine
