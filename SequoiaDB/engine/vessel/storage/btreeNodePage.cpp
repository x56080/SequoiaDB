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
   void btreeItemSlot::initWhenDataInPage(const recordID &rid,
                                          UINT16 offset,
                                          UINT16 size,
                                          PAGE_ID leftChild)
   {
      SDB_ASSERT(rid.valid(), "can not be invalid");
      reset();
      OSS_BIT_SET(flags, (FLAG_IN_USED|FLAG_DATA_IN_PAGE_BODY));
      ridSlot = rid.getSlotID();
      ridPage = rid.getPageID();
      data.pointer.size = size;
      data.pointer.offset = offset;
      data.pointer.leftChild = leftChild;
      return;
   }

   void btreeItemSlot::initWhenDataInSlot(const recordID &rid,
                                          const CHAR *keyData,
                                          UINT32 keySize)
   {
      SDB_ASSERT(rid.valid(), "can not be invalid");
      SDB_ASSERT(NULL != keyData, "can not be null");
      SDB_ASSERT(keySize <= sizeof(btreeItemSlot::slotData), "out of size");
      reset();
      OSS_BIT_SET(flags, (FLAG_IN_USED|FLAG_DATA_IN_SLOT));
      ridSlot = rid.getSlotID();
      ridPage = rid.getPageID();
      ossMemcpy(data.keyData, keyData, keySize);
      return;
   }

   void btreeItemSlot::initWhenPerfectlyCompressed(const recordID &rid,
                                                   UINT32 prefixPos)
   {
      SDB_ASSERT(rid.valid(), "can not be invalid");
      reset();
      OSS_BIT_SET(flags, FLAG_IN_USED);
      ridSlot = rid.getSlotID();
      ridPage = rid.getPageID();
      setKeyCompressed(prefixPos);
      return;
   }

   void btreeItemSlot::initWhenDataInExtPage(const recordID &rid,
                                             UINT16 size,
                                             PAGE_ID leftChild,
                                             PAGE_ID extPage)
   {
      SDB_ASSERT(rid.valid(), "can not be invalid");
      SDB_ASSERT(INVALID_PAGE_ID != leftChild, "can not be invalid");
      SDB_ASSERT(INVALID_PAGE_ID != extPage, "can not be invalid");
      reset();
      OSS_BIT_SET(flags, FLAG_IN_USED);
      ridSlot = rid.getSlotID();
      ridPage = rid.getPageID();
      data.pointer.leftChild = leftChild;
      data.pointer.size = size;
      data.pointer.pad = extPage;
      return;
   }

   BOOLEAN btreeItemSlot::hasNoSuffix()const
   {
      SDB_ASSERT(isValid(), "must be invalid");
      SDB_ASSERT(isKeyCompressed(), "must be compressed");
      return !isDataInPageBody() && !isDataInSlot();
   }

   UINT32 btreeItemSlot::getSuffixSize()const
   {
      SDB_ASSERT(isValid(), "must be invalid");
      SDB_ASSERT(isKeyCompressed(), "must be compressed");

      if (isDataInSlot())
      {
         return ixmKey(data.getKeyData()).dataSize();
      }
      else if (isDataInPageBody())
      {
         return data.pointer.size;
      }
      
      return 0;
   }
////////btreeItemSlot end

   BOOLEAN initBtreeNodePage(UINT32 pageSize,
                             PAGE_ID pid,
                             PAGE_ID lpid,
                             PAGE_SNAPSHOT_VERION psv,
                             UINT32 cllid,
                             UINT32 indexId,
                             PAGE_ID rightChild,
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
      headPtr->clLogicalID = cllid;
      headPtr->indexId = indexId;
      headPtr->rightChild = rightChild;
      headPtr->totalFreeSpace = getPageBodySize(pageSize) - BTREE_NODE_PAGE_HEAD_SIZE;
      headPtr->freeSapceAfterLastSlot = headPtr->totalFreeSpace;
      r = TRUE;

   done:
      return r;
   }
} // namespace vessel

} // namespace engine
