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

   Source File Name = indexEntryPage.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#include "vessel/indexEntryPage.h"

namespace engine
{
namespace vessel
{
   BOOLEAN initIndexEntryPage(UINT32 pageSize,
                              PAGE_ID pid,
                              PAGE_ID lpid,
                              PAGE_SNAPSHOT_VERION psv,
                              CHAR *buf)
   {
      BOOLEAN r = FALSE;
      indexEntryPageHead *headPtr = NULL;
      indexEntryPageHead head;

      r = initCommonPage(PAGE_TYPE_INDEX_ENTRY, pageSize,
                         pid, lpid, psv, buf);
      if (!r)
      {
         goto done;
      }

      headPtr = (indexEntryPageHead *)((ossValuePtr)buf + PAGE_HEAD_SIZE);
      ossMemcpy(headPtr, &head, INDEX_ENTRY_PAGE_HEAD_SIZE);
   done:
      return r;
   }
}//namespace vessel
}//namespace engine
