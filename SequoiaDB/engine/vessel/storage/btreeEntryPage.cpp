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

   Source File Name = indexEntryPage.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

*******************************************************************************/
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
      headPtr->transferTick = 0;
   done:
      return r;
   }
}//namespace vessel
}//namespace engine
