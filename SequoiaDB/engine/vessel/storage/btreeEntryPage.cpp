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

#include "vessel/btreeEntryPage.h"

namespace engine
{
namespace vessel
{
   BOOLEAN initBtreeEntryPage(UINT32 pageSize,
                              PAGE_ID pid,
                              PAGE_ID lpid,
                              PAGE_SNAPSHOT_VERION psv,
                              UINT32 logicalIndexId,
                              CHAR *buf)
   {
      BOOLEAN r = FALSE;
      SDB_ASSERT(INVALID_LOGICAL_INDEX_ID != logicalIndexId, "can not be invalid");
      btreeEntryPageHead *headPtr = NULL;
      btreeEntryPageHead head;

      r = initCommonPage(PAGE_TYPE_BTREE_ENTRY, pageSize,
                         pid, lpid, psv, buf);
      if (!r)
      {
         goto done;
      }

      headPtr = (btreeEntryPageHead *)((ossValuePtr)buf + PAGE_HEAD_SIZE);
      headPtr->version = BTREE_ENTRY_PAGE_VERSION;
      headPtr->logicalIndexId = logicalIndexId;
      headPtr->btreeRoot = INVALID_PAGE_ID;
      headPtr->replayTick = 0;
   done:
      return r;
   }
}//namespace vessel
}//namespace engine
