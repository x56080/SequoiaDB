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

   Source File Name = btreeNodePage.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#include "vessel/btreeNodePage.h"
#include "pdTrace.hpp"
#include "ossLikely.hpp"
#include "ixmKey.hpp"

namespace engine
{
namespace vessel
{
////////btreeItemSlot
   void btreeItemSlot::initAsNonLeafFormat(const recordID &rid,
                                          UINT16 offset,
                                          UINT16 size,
                                          PAGE_ID leftChild,
                                          BOOLEAN isExternalKey)
   {
      SDB_ASSERT(rid.isValid(), "can not be invalid");
      SDB_ASSERT(0 != size, "can not be invalid");
      SDB_ASSERT(!(!isExternalKey && 0 == offset), "can not be invalid");
      reset();
      OSS_BIT_SET(flags, (FLAG_IN_USED));
      ridPos = rid.getPos();
      ridPage = rid.getPid();
      data.key.offset = offset;
      data.key.size = size;
      data.nlf.leftChild = leftChild;
      if (isExternalKey)
      {
         OSS_BIT_SET(flags, FLAG_KEY_IN_EXTERNAL_PAGE);
      }
      return;
   }

   void btreeItemSlot::initAsLeafFormat(const recordID &rid,
                                        UINT16 offset,
                                        UINT16 size,
                                        BOOLEAN compressed,
                                        RECORD_SLOT_POS prefixPos)
   {
      SDB_ASSERT(rid.isValid(), "can not be invalid");
      reset();
      OSS_BIT_SET(flags, FLAG_IN_USED);
      ridPos = rid.getPos();
      ridPage = rid.getPid();
      data.key.offset = offset;
      data.key.size = size;
      
      if (compressed)
      {
         OSS_BIT_SET(flags, FLAG_KEY_COMPRESSESD);
         SDB_ASSERT(isValidRecordSlotPosition(prefixPos), "can not be invalid");
      }
      
      data.lf.prefixSlot = prefixPos;

      return;
   }

////////btreeItemSlot end

   BOOLEAN initBtreeNodePage(UINT32 pageSize,
                             PAGE_ID pid,
                             PAGE_ID lpid,
                             PAGE_SNAPSHOT_VERION psv,
                             UINT32 cllid,
                             UINT32 indexId,
                             BOOLEAN isLeaf,
                             BOOLEAN isRoot,
                             CHAR *buf)
   {
      BOOLEAN r = FALSE;
      btreeNodePageHead *headPtr = NULL;
      btreeNodePageHead head;
   
      SDB_ASSERT(pageSize <= 65536, "can not be over 64k");

      if (OSS_UNLIKELY(DMS_INVALID_LOGICCLID == cllid ||
                       INVALID_LOGICAL_INDEX_ID == indexId))
      {
         goto done;
      }

      r = initCommonPage(PAGE_TYPE_BTREE_NODE, pageSize,
                         pid, lpid, psv, buf);
      if (!r)
      {
         goto done;
      }

      headPtr = (btreeNodePageHead *)((ossValuePtr)buf + PAGE_HEAD_SIZE);
      ossMemcpy(headPtr, &head, BTREE_NODE_PAGE_HEAD_SIZE);
      headPtr->version = BTREE_NODE_PAGE_HEAD_VERSION;
      headPtr->clLogicalID = cllid;
      headPtr->indexId = indexId;
      headPtr->rightChild = INVALID_PAGE_ID;
      headPtr->totalFreeSpace = getPageBodySize(pageSize) - BTREE_NODE_PAGE_HEAD_SIZE;
      headPtr->freeSapceAfterLastSlot = headPtr->totalFreeSpace;
      headPtr->flags = 0;
      if (isLeaf)
      {
         OSS_BIT_SET(headPtr->flags, BTREE_NODE_FLAG_IS_LEAF);
      }
      if (isRoot)
      {
         OSS_BIT_SET(headPtr->flags, BTREE_NODE_FLAG_IS_ROOT);
      }
      r = TRUE;

   done:
      return r;
   }
} // namespace vessel

} // namespace engine
