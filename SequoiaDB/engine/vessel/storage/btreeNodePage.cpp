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
//////// btreeNodePageHead
   void btreeNodePageHead::init(UINT32 pageSize,
                                UINT32 indexId,
                                BOOLEAN isRoot,
                                BOOLEAN isLeaf)
   {
      SDB_ASSERT(isValidPageSize(pageSize), "can not be invalid");
      SDB_ASSERT(INVALID_LOGICAL_INDEX_ID != indexId, "can not be invalid");

      reset();
      this->version = BTREE_NODE_PAGE_HEAD_VERSION;
      this->indexId = indexId;
      this->backOffset = getPageBodySize(pageSize);
      this->totalFreeSpace = this->backOffset - BTREE_NODE_PAGE_HEAD_SIZE;

      if (isRoot)
      {
         OSS_BIT_SET(this->flags, BTREE_NODE_FLAG_IS_ROOT);
      }
      if (isLeaf)
      {
         OSS_BIT_SET(this->flags, BTREE_NODE_FLAG_IS_LEAF);
      }

      return;
   }  

   void btreeNodePageHead::initAsRightNode(const btreeNodePageHead &src,
                                           UINT32 pageSize)
   {
      SDB_ASSERT(src.isValid(), "can not be invalid");
      SDB_ASSERT(isValidPageSize(pageSize), "can not be invalid");
      reset();

      this->version = src.version;
      this->indexId = src.indexId;
      this->backOffset = getPageBodySize(pageSize);
      this->totalFreeSpace = this->backOffset - BTREE_NODE_PAGE_HEAD_SIZE;
      this->rightChild = src.rightChild;
      this->transID = src.transID;

      /// always ignore root flag when build right node
      OSS_BIT_SET(this->flags, (src.flags & BTREE_NODE_FLAG_IS_LEAF));
   }

////////btreeItemSlot
   void btreeItemSlot::initAsNonLeafFormat(UINT16 offset,
                                           UINT16 size,
                                           PAGE_ID leftChild)
   {
      SDB_ASSERT(0 != size, "can not be invalid");

      reset();
      OSS_BIT_SET(flags, (FLAG_IN_USED));
      data.key.offset = offset;
      data.key.size = size;
      data.nlf.leftChild = leftChild;
      return;
   }

   void btreeItemSlot::initAsLeafFormat(UINT16 offset,
                                        UINT16 size,
                                        RECORD_SLOT_POS prefixPos)
   {
      reset();
      OSS_BIT_SET(flags, FLAG_IN_USED);
      data.key.offset = offset;
      data.key.size = size;
      if (INVALID_RECORD_SLOT_POS != prefixPos)
      {
         OSS_BIT_SET(flags, FLAG_KEY_COMPRESSESD);
         data.lf.prefixSlot = prefixPos;
      }
   
      return;
   }

////////btreeItemSlot end

   BOOLEAN initBtreeNodePage(UINT32 pageSize,
                             PAGE_ID pid,
                             PAGE_ID lpid,
                             PAGE_SNAPSHOT_VERION psv,
                             UINT32 indexId,
                             BOOLEAN isLeaf,
                             BOOLEAN isRoot,
                             CHAR *buf)
   {
      BOOLEAN r = FALSE;
      btreeNodePageHead *headPtr = NULL;
      btreeNodePageHead head;
   
      SDB_ASSERT(pageSize <= 65536, "can not be over 64k");

      if (OSS_UNLIKELY(INVALID_LOGICAL_INDEX_ID == indexId))
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
      headPtr->init(pageSize, indexId, isRoot, isLeaf);
      r = TRUE;

   done:
      return r;
   }
} // namespace vessel

} // namespace engine
