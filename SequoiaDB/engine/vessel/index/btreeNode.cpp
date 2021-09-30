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

   Source File Name = btreeNode.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#include "vessel/btreeNode.h"
#include "vessel/requestContext.h"
#include "vessel/indexContext.h"
#include "ossLikely.hpp"
#include "pdTrace.hpp"
#include "vessel/orderingWrapper.h"
#include "vessel/indexSpace.h"
#include "vessel/btreeNodePageIniter.h"

namespace engine
{
namespace vessel
{
   btreeNode::btreeNode(logicalPageBuffer *buffer,
                        const indexContext *ic,
                        UINT32 depth):
   _buffer(buffer),
   _ic(ic),
   _depth(depth)
   {
      SDB_ASSERT(NULL != _buffer && _buffer->isValid(), "can not be invalid");
      SDB_ASSERT(NULL != _ic && _ic->isValid(), "can not be invalid");
   }

   void btreeNode::reset()
   {
      _buffer = NULL;
      _ic = NULL;
      _depth = 0;
      return;
   }


   BOOLEAN btreeNode::isRoot()const
   {
      SDB_ASSERT(isValid(), "can not be invalid");
      return 0 == _depth;
   }

   BOOLEAN btreeNode::isLeaf()const
   {
      SDB_ASSERT(isValid(), "can not be invalid");
      const btreeNodePageHead *head = getReadableHead();
      SDB_ASSERT(NULL != head, "impossible");
      return INVALID_PAGE_ID == head->rightChild;
   }

   UINT32 btreeNode::getTotalSlotCount()const
   {
      SDB_ASSERT(isValid(), "can not be invalid");
      const btreeNodePageHead *head = getReadableHead();
      SDB_ASSERT(NULL != head, "impossible");
      return head->totalSlotCount;
   }

   BOOLEAN btreeNode::hasExtNode()const
   {
      SDB_ASSERT(isValid(), "can not be invalid");
      const btreeNodePageHead *head = getReadableHead();
      SDB_ASSERT(NULL != head, "impossible");
      return INVALID_PAGE_ID != head->extNode;
   }

   const btreeNodePageHead *btreeNode::getReadableHead()const
   {
      SDB_ASSERT(isValid(), "can not be invalid");
      return _buffer->getRuntimeBuffer().getReadablePtrOfBody<btreeNodePageHead>(0);
   }

   btreeNodePageHead *btreeNode::getWritableHead()
   {
      SDB_ASSERT(isValid(), "can not be invalid");
      return _buffer->getRuntimeBuffer().getWritablePtrOfBody<btreeNodePageHead>(0);
   }

   UINT32 btreeNode::getTotalKeyAndSlotSize()const
   {
      SDB_ASSERT(isValid(), "can not be invalid");
      const btreeNodePageHead *head = getReadableHead();
      UINT32 size = getPageBodySize(_buffer->getRuntimeBuffer().getPageSize());
      size -= BTREE_NODE_PAGE_HEAD_SIZE;
      size -= head->totalFreeSpace;
      if (0 != OSS_BIT_TEST(head->flags, BTREE_NODE_FLAG_PFRFIX_CREATED))
      {
         size -= getTotalPrefixSize(head);
      }
      return size;
   }

   UINT32 btreeNode::getTotalPrefixSize(const btreeNodePageHead *head)const
   {
      SDB_ASSERT(NULL != head, "can not be null");
      UINT32 size = 0;

      if (0 == OSS_BIT_TEST(head->flags, BTREE_NODE_FLAG_PFRFIX_CREATED))
      {
         goto done;
      }

      for (UINT32 i = 0; i < MAX_BTREE_NODE_PREFIX_SLOT_COUNT; ++i)
      {
         size += head->prefixes[i].prefixSize;
      }
      
   done:
      return size;
   }

   const btreeNodeSlot *btreeNode::getReadableSlot(RECORD_SLOT_ID slotNo)const
   {
      SDB_ASSERT(isValid(), "can not be invalid");
      SDB_ASSERT(INVALID_RECORD_SLOT_ID != slotNo, "can not be invalid");
      const btreeNodePageHead *head = getReadableHead();
      const btreeNodeSlot *slot = NULL;
      if (OSS_UNLIKELY(head->totalSlotCount <= slotNo))
      {
         SDB_ASSERT(FALSE, "out of bound");
         PD_LOG(PDERROR, "slot no[%d] out of range[%d]",
                slotNo, head->totalSlotCount);
      }
      else
      {
         UINT32 offset = slotNo * BTREE_NODE_SLOT_SIZE + BTREE_NODE_PAGE_HEAD_SIZE;
         slot = _buffer->getRuntimeBuffer().getReadablePtrOfBody<btreeNodeSlot>(offset);
         if (OSS_UNLIKELY(NULL == slot || !slot->isValid()))
         {
            SDB_ASSERT(FALSE, "impossible");
            PD_LOG(PDERROR, "slot [%d] is not valid", slotNo);
            slot = NULL;
         }
      }

      return slot;
   }

   const btreeNodePrefixSlot *btreeNode::getPrefixReferencedBySlot(RECORD_SLOT_ID slotNo,
                                                                   INT32 *which)const
   {
      SDB_ASSERT(isValid(), "can not be invalid");
      SDB_ASSERT(INVALID_RECORD_SLOT_ID != slotNo, "can not be invalid");
      const btreeNodePrefixSlot *prefix = NULL;
      const btreeNodePageHead *head = getReadableHead();

      if (NULL != which)
      {
         *which = -1;
      }

      if (0 == OSS_BIT_TEST(head->flags, BTREE_NODE_FLAG_PFRFIX_CREATED))
      {
         goto done;
      }
      else if (head->totalSlotCount <= slotNo)
      {
         goto done;
      }

      for (UINT32 i = 0; i < MAX_BTREE_NODE_PREFIX_SLOT_COUNT; ++i)
      {
         const btreeNodePrefixSlot &prefixSlot = head->prefixes[i];
         if (!prefixSlot.isValid())
         {
            break;
         }
         else if (slotNo < prefixSlot.referencedLow)
         {
            break;
         }
         else if (prefixSlot.referencedLow <= slotNo &&
                  slotNo <= prefixSlot.referencedHigh)
         {
            prefix = &prefixSlot;
            if (NULL != which)
            {
               *which = i;
            }
            break;
         }
         else
         {
            continue;
         }
      }

   done:
      return prefix;
   }

   INT32 btreeNode::locateKeyAndRid(const ixmKey &key,
                                    const recordID &rid,
                                    locateResult &result)const
   {
      INT32 rc = SDB_OK;
      const btreeNodePageHead *head = NULL;
      orderingWrapper ow;

      INT32 low = 0;
      INT32 high = 0;
      INT32 middle = 0;

      result = locateResult();

      if (OSS_UNLIKELY(!isValid()))
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }
      else if (OSS_UNLIKELY(!key.isValid()))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      head = getReadableHead();
      if (OSS_UNLIKELY(NULL == head))
      {
         PD_LOG(PDERROR, "failed to get readable page head");
         rc = SDB_VESSEL_INTERNAL_ERR;
         goto error;
      }

      ow = _ic->getObj().getPattern().getOrdering();
      high = (INT32)(head->totalSlotCount) - 1;
      middle = ((low + high) >> 1);

      while (low <= high)
      {
         ixmKey currentKey;
         INT32 res = 0;
         btreeIndexItem item;
         rc = getIndexItem((RECORD_SLOT_ID)middle, item);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to get item at[%d], rc:%d", middle, rc);
            goto error;
         }

         if (item.getSlot()->isKeyCompressed())
         {
            SDB_ASSERT(FALSE, "TODO");
         }

         item.getKeyWhenNotCompressed(currentKey);
         res = key.woCompare(currentKey, ow.toBsonOrdering());
         if (0 == res)
         {
            res = rid.compare(item.getRid());
            result.keyMatched = TRUE;
         }

         if (res < 0)
         {
            high = middle - 1;
         }
         else if (0 < res)
         {
            low = middle + 1;
         }
         else
         {
            result.slotNo = (RECORD_SLOT_ID)middle;
            result.identical = TRUE;
            goto done;
         }

         middle = ((low + high) >> 1); 
      }

      if ((INT32)(head->totalSlotCount) == low)
      {
         result.upperBound = TRUE;
      }
      else
      {
         result.slotNo = low;
      }
   done:
      return rc;
   error:
      goto done;
   }

   INT32 btreeNode::getIndexItem(RECORD_SLOT_ID slotNo,
                                 btreeIndexItem &item)const
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(isValid(), "can not be invalid");
      SDB_ASSERT(INVALID_RECORD_SLOT_ID != slotNo, "can not be invalid");
      const btreeNodePageHead *head = NULL;
      const btreeNodeSlot *slot = NULL;
      const CHAR *keyData = NULL;
      const btreeNodePrefixSlot *prefix = NULL;
      item.fini();

      if (OSS_UNLIKELY(!isValid()))
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }
      else if (OSS_UNLIKELY(INVALID_RECORD_SLOT_ID == slotNo))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      head = getReadableHead();
      if (OSS_UNLIKELY(NULL == head))
      {
         PD_LOG(PDERROR, "failed to get readble head ptr");
         rc = SDB_VESSEL_INTERNAL_ERR;
         goto error;
      }

      if (head->totalSlotCount <= slotNo)
      {
         rc = SDB_OUT_OF_BOUND;
         goto error;
      }

      slot = getReadableSlot(slotNo);
      if (OSS_UNLIKELY(NULL == slot))
      {
         PD_LOG(PDERROR, "slot[%d] is invalid", slotNo);
         rc = SDB_VESSEL_INTERNAL_ERR;
         goto error;
      }

      SDB_ASSERT(!slot->isExternalKey(), "TODO");

      if (!slot->isKeyInSlot())
      {
         keyData = _buffer->getRuntimeBuffer().getReadablePtrOfBody<CHAR>(slot->data.pointer.offset);
         if (OSS_UNLIKELY(NULL == keyData))
         {
            PD_LOG(PDERROR, "failed to get key data ptr");
            rc = SDB_VESSEL_INTERNAL_ERR;
            goto error;
         }
      }

      if (slot->isKeyCompressed())
      {
         prefix = getPrefixReferencedBySlot(slotNo);
         if (OSS_UNLIKELY(NULL == prefix))
         {
            PD_LOG(PDERROR, "failed to get prefix of slot[%d]", slotNo);
            rc = SDB_VESSEL_INTERNAL_ERR;
            goto error;
         }
      }

      rc = item.init(slotNo, slot, keyData, prefix);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to init index item");
         rc = SDB_VESSEL_INTERNAL_ERR;
         goto error;
      }
   done:
      return rc;
   error:
      goto done;
   }

   ossSharedLatchMode btreeNode::getMode()const
   {
      SDB_ASSERT(isValid(), "can not be invalid");
      ossSharedLatchMode mode;
      if (OSS_LIKELY(isValid()))
      {
         mode = _buffer->getLockingMode();
      }
      return mode;
   }

   BOOLEAN btreeNode::isPrefixEnabled()const
   {
      SDB_ASSERT(isValid(), "can not be invalid");
      const btreeNodePageHead *head = getReadableHead();
      return 0 != OSS_BIT_TEST(head->flags, BTREE_NODE_FLAG_PFRFIX_CREATED);
   }

   BOOLEAN btreeNode::tryToEnsureLockExlusive()
   {
      BOOLEAN r = FALSE;
      SDB_ASSERT(isValid(), "can not be invalid");
      ossSharedLatchMode mode = _buffer->getLockingMode();
      SDB_ASSERT(!mode.isNone(), "impossible");
      if (mode.isShared())
      {
         r = _buffer->tryLockExclusiveFromShared();
      }
      else if (mode.isUpgrade())
      {
         r = _buffer->tryLockExclusiveFromUpgrade();
      }
      else
      {
         r = TRUE;
      }
      
      return r;
   }

   INT32 btreeNode::presplit(PAGE_ID &newPage,
                             btreeIndexItem &pivot)
   {
      INT32 rc = SDB_OK;
      ossSharedLatchMode mode(OSS_SHARED_LATCH_MODE_ENUM_EXCLUSIVE);
      btreeNodePageIniter initer;
      logicalPageBuffer lpb;
      btreeNode brother;
      btreeIndexItem item;
      logicalPageSpace *lps = NULL;
      requestContext *context = NULL;

      newPage = INVALID_PAGE_ID;
      pivot.fini();
      
      if (OSS_UNLIKELY(!isValid()))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      else if (OSS_UNLIKELY(!_buffer->getLockingMode().isExclusive()))
      {
         rc = SDB_VESSEL_FORBIDDEN_OP_WLT;
         goto error;
      }

      rc = findSplitPivot(item);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to find split item:%d", rc);
         goto error;
      }

      context = _buffer->getContext();
      lps = _buffer->getLogicalPageSpace();
      initer.set(context->getLogicalCLID(), _ic->getIndexID());
      rc = lps->allocatePages(context, &initer, 1, &newPage);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to allocate new page:%d", rc);
         goto error;
      }

      rc = lps->getLogicalPageBuffer(context, newPage, mode, lpb);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to get buffer of page[%d], rc:%d",
                newPage, rc);
         goto error;
      }
   done:
      return rc;
   error:
      if (INVALID_PAGE_ID != newPage)
      {
         lps->releasePages(context, 1, &newPage);
         newPage = INVALID_PAGE_ID;
      }
      pivot.fini();
      goto done;
   }

   INT32 btreeNode::findSplitPivot(btreeIndexItem &pivot)const
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(isValid(), "can not be invalid");
      static const UINT32 _MIN_SLOT_COUNT = 3;
      static const FLOAT32 _ORDERED_INSERT_RATIO = 0.95;

      const btreeNodePageHead *head = getReadableHead();
      UINT32 maxSplitSize = 0;
      UINT32 totalSize = 0;
      UINT32 scannedSize = 0;
      RECORD_SLOT_ID pivotSlotNo = INVALID_RECORD_SLOT_ID;
      
      if (head->totalSlotCount < _MIN_SLOT_COUNT)
      {
         PD_LOG(PDERROR, "too few slots[%d] to split", head->totalSlotCount);
         rc = SDB_VESSEL_OPERATOION_NOT_PERMITTED;
         goto error;
      }

      totalSize = getTotalKeyAndSlotSize();
      if (hasExtNode() ||
          ((FLOAT32)(head->orderedInsert) / head->totalSlotCount) < _ORDERED_INSERT_RATIO)
      {
         maxSplitSize = totalSize / 2;
      }
      else
      {
         maxSplitSize = totalSize / 10;
      }

      for (INT32 i = (INT32)(head->totalSlotCount) - 1; i >= 0; --i)
      {
         btreeIndexItem item;
         rc = getIndexItem(i, item);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to get index item[%d], rc:%d", i, rc);
            goto error;
         }

         scannedSize += item.getSavingSize();
         if (maxSplitSize < scannedSize)
         {
            break;
         }
         else
         {
            pivotSlotNo = i;
            pivot = item;
         }
      }

      if (INVALID_RECORD_SLOT_ID == pivotSlotNo)
      {
         pivotSlotNo = head->totalSlotCount - 2;
      }

      if (OSS_UNLIKELY(0 == pivotSlotNo))
      {
         PD_LOG(PDERROR, "split pivot can not be zero");
         rc = SDB_VESSEL_INTERNAL_ERR;
         goto error;
      }
   done:
      return rc;
   error:
      pivot.fini();
      goto done;
   }

   BOOLEAN btreeNode::hasSpaceToInsert(const ixmKey &key)const
   {
      SDB_ASSERT(isValid(), "can not be invalid");
      SDB_ASSERT(key.isValid(), "can not be invalid");

      const btreeNodePageHead *head = getReadableHead();
      UINT32 totalSize = BTREE_NODE_SLOT_SIZE;
      UINT32 keySize = key.dataSize();
      
      /// do not care about compression.
      if (!isKeyCanBeSavedInSlot(keySize))
      {
         totalSize += keySize;
      }

      if (!isLeaf())
      {
         /// preallocate space for ext key slot.
         totalSize += BTREE_NODE_SLOT_SIZE;
      }         

      return totalSize <= head->totalFreeSpace;
   }


   INT32 btreeNode::_splitTo(RECORD_SLOT_ID begin, btreeNode &node)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(isValid(), "can not be invalid");
      SDB_ASSERT(INVALID_RECORD_SLOT_ID != begin, "can not be invalid");
      SDB_ASSERT(node.isValid(), "can not be invalid");

      btreeNodePageHead *head = node.getWritableHead();
      SDB_ASSERT(0 == head->totalSlotCount, "must be empty");
      SDB_ASSERT(!node.isPrefixEnabled(), "can not be enabled");
      const btreeNodePageHead *srcHead = getReadableHead();
      SDB_ASSERT(begin < srcHead->totalSlotCount, "out of bound");

      for (RECORD_SLOT_ID i = begin; i < srcHead->totalSlotCount; ++i)
      {
         btreeIndexItem item;
         rc = getIndexItem(i, item);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to get index item[%d], rc:%d", i, rc);
            goto error;
         }

         rc = node.pushBackWhenSplit(item);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to push back item:%d", rc);
            goto error;
         }
      }
   done:
      return rc;
   error:
      goto done;
   }

   ixmKey btreeNode::getKeyPrefix(INT32 which)const
   {
      SDB_ASSERT(isValid(), "can not be invalid");
      SDB_ASSERT(0 <= which && which < (INT32)MAX_BTREE_NODE_PREFIX_SLOT_COUNT,
                 "can not be invalid");

      ixmKey prefix;
      const btreeNodePageHead *head = getReadableHead();
      SDB_ASSERT(0 != OSS_BIT_TEST(head->flags, BTREE_NODE_FLAG_PFRFIX_CREATED), "no prefix");

      if (head->prefixes[which].isValid())
      {
         UINT32 offset = head->prefixes[which].prefixOffset;
         UINT32 size = head->prefixes[which].prefixSize;
         ossValuePtr ptr = 0;
         INT32 rc =  _buffer->getRuntimeBuffer().getReadablePtrOfBodyWithRc(offset, size, ptr);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to get ptr of offset:size [%d,%d], rc:%d",
                   offset, size, rc);
            goto done;
         }

         prefix.assign((const CHAR *)ptr);
      }

   done:
      return prefix;
   }

   UINT32 btreeNode::getKeyDataOffsetToWrite(UINT32 keyDataSize)const
   {
      SDB_ASSERT(0 < keyDataSize, "can not be zero");
      const btreeNodePageHead *head = getReadableHead();
      SDB_ASSERT((keyDataSize + BTREE_NODE_SLOT_SIZE) <= head->freeSapceAfterLastSlot,
                 "out of resource");
      return BTREE_NODE_PAGE_HEAD_SIZE +
             (BTREE_NODE_SLOT_SIZE * head->totalSlotCount) +
             head->freeSapceAfterLastSlot -
             keyDataSize;
   }

   INT32 btreeNode::pushBackWhenSplit(const btreeIndexItem &item)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(isValid(), "can not be invalid");
      SDB_ASSERT(item.isValid(), "can not be invalid");

      UINT32 keyDataSize = 0;
      UINT32 keyDataOffset = 0;
      UINT32 slotOffset = 0;
      btreeNodeSlot *slotPtr = NULL;
      UINT32 size = item.getSavingSize();
      btreeNodePageHead *head = NULL;

      rc = _buffer->prepareToWrite();
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to prepare to write:%d", rc);
         goto error;
      }

      head = getWritableHead();
      if (OSS_UNLIKELY(NULL == head))
      {
         PD_LOG(PDERROR, "failed to get writable head ptr");
         rc = SDB_VESSEL_INTERNAL_ERR;
         goto error;
      }

      if (head->freeSapceAfterLastSlot < size)
      {
         PD_LOG(PDERROR, "not enough free space");
         rc = SDB_VESSEL_NOT_ENOUGH_SPACE_IN_PAGE;
         goto error;
      }

      slotOffset = BTREE_NODE_PAGE_HEAD_SIZE +
                   (head->totalSlotCount * BTREE_NODE_SLOT_SIZE);
      slotPtr = _buffer->getRuntimeBuffer().getWritablePtrOfBody<btreeNodeSlot>(slotOffset);
      if (NULL == slotPtr)
      {
         PD_LOG(PDERROR, "failed to get wriable ptr of slot");
         rc = SDB_VESSEL_INTERNAL_ERR;
         goto error;
      }
      *slotPtr = *item.getSlot();

      if (!item.getSlot()->isKeyInSlot())
      {
         ossValuePtr keyDataPtr = 0;
         keyDataSize = ixmKey(item.getKeyData()).dataSize();
         keyDataOffset = getKeyDataOffsetToWrite(keyDataSize);
         
         rc = _buffer->getRuntimeBuffer().getWritablePtrOfBodyWithRc(keyDataOffset,
                                                                     keyDataSize,
                                                                     keyDataPtr);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to get writable data ptr:%d", rc);
            goto error;
         }

         ossMemcpy((CHAR *)keyDataPtr, item.getKeyData(), keyDataSize);
         slotPtr->data.pointer.offset = keyDataOffset;
         OSS_BIT_CLEAR(slotPtr->flags, btreeNodeSlot::FLAG_KEY_IN_EXTERNAL_PAGE);
      }

      head->totalFreeSpace -= size;
      head->freeSapceAfterLastSlot -= size;
      ++head->totalSlotCount;
      ++head->orderedInsert;
   done:
      return rc;
   error:
      if (_buffer->getRuntimeBuffer().isWritingPrepared())
      {
         _buffer->abort();
      }
      goto done;
   }
} // namespace vessel

} // namespace engine

