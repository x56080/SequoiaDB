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

   Source File Name = btreeExternalKeyPage.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#include "vessel/btreeExternalKeyPage.h"
#include "vessel/indexDef.h"

namespace engine
{
namespace vessel
{
   BOOLEAN initBtreeExtKeyPage(UINT32 pageSize,
                               PAGE_ID pid,
                               PAGE_ID lpid,
                               PAGE_SNAPSHOT_VERION psv,
                               UINT32 indexId,
                               UINT32 keySize,
                               const CHAR *keyData,
                               CHAR *buf)
   {
      BOOLEAN r = FALSE;
      btreeExternalKeyPageHead *head = NULL;

      SDB_ASSERT(pageSize <= 65536, "can not be over 64k");

      if (INVALID_LOGICAL_INDEX_ID == indexId ||
          0 == keySize ||
          NULL == keyData)
      {
         goto done;
      }

      if (!initCommonPage(PAGE_TYPE_BTREE_EXTERNAL_KEY,
                          pageSize, pid, lpid,
                          psv, buf))
      {
         goto done;
      }

      head = (btreeExternalKeyPageHead *)((ossValuePtr)buf + PAGE_HEAD_SIZE);
      head->version = BTREE_EXT_KEY_PAGE_HEAD_VERSION;
      head->flags = 0;
      head->indexId = indexId;
      head->size = keySize;
      ossMemcpy((CHAR *)((ossValuePtr)buf + PAGE_HEAD_SIZE + BTREE_EXT_KEY_PAGE_HEAD_SIZE),
                keyData, keySize);
   done:
      return r;
   }
} // namespace vessel

} // namespace engine
