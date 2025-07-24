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

   Source File Name = btreeNodePage.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

*******************************************************************************/
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
                                           UINT32 nodeSize)
   {
      SDB_ASSERT(src.isValid(), "can not be invalid");
      SDB_ASSERT(BTREE_NODE_PAGE_HEAD_SIZE < nodeSize, "can not be invalid");
      reset();

      this->version = src.version;
      this->indexId = src.indexId;
      this->backOffset = nodeSize;
      this->totalFreeSpace = this->backOffset - BTREE_NODE_PAGE_HEAD_SIZE;
      this->rightChild = src.rightChild;
      this->transID = src.transID;

      UINT32 flags = src.flags;
      /// always ignore root flag when build right node
      OSS_BIT_CLEAR(flags, BTREE_NODE_FLAG_IS_ROOT);
      OSS_BIT_CLEAR(flags, BTREE_NODE_FLAG_VAIN_PREFIX_REGENERATION);
      this->flags = flags;
      return;
   }

////////btreeItemSlot
   void btreeItemSlot::initAsNonLeafFormat(UINT16 offset,
                                           UINT16 size,
                                           PAGE_ID leftChild)
   {
      SDB_ASSERT(0 != size, "can not be invalid");

      reset();
      OSS_BIT_SET(flags, FLAG_IN_USED);
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
      data.lf.prefixSlot = prefixPos;
      if (isValidRecordSlotPosition(prefixPos))
      {
         OSS_BIT_SET(flags, FLAG_KEY_COMPRESSESD);
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
