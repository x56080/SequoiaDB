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
#include "vessel/indexObject.h"
#include "ossLikely.hpp"
#include "pdTrace.hpp"
#include "vessel/orderingWrapper.h"
#include "vessel/indexSpace.h"
#include "vessel/btreeNodePageIniter.h"
#include "vessel/logicalPageBuffer.h"
#include "vessel/memoryBlock.h"
#include "ossMemPool.hpp"
#include "vessel/btreeAccessContext.h"
#include "vessel/indexUtils.h"
#include "vessel/keyStringCoder.h"

namespace engine
{
namespace vessel
{
   btreeNode::btreeNode(UINT32 depth, btreeAccessContext *ctx):
   _depth(depth),
   _ctx(ctx)
   {
      SDB_ASSERT(nullptr != _ctx, "can not be invalid");
      _buffer = _ctx->getBuffer(_depth);
      SDB_ASSERT(nullptr != _buffer, "can not be invalid");
      _inPath = TRUE;
   }

   btreeNode::btreeNode(UINT32 depth,
                        logicalPageBuffer *buffer,
                        btreeAccessContext *ctx):
   _depth(depth),
   _buffer(buffer),
   _ctx(ctx)
   {
      SDB_ASSERT(nullptr != _ctx, "can not be invalid");
      SDB_ASSERT(nullptr != _buffer, "can not be invalid");
      _inPath = _buffer == _ctx->getBuffer(_depth);
   }

   BOOLEAN btreeNode::isRoot()const
   {
      return 0 != OSS_BIT_TEST(getReadableHead()->flags, BTREE_NODE_FLAG_IS_ROOT);
   }

   BOOLEAN btreeNode::isLeaf()const
   {
      return 0 != OSS_BIT_TEST(getReadableHead()->flags, BTREE_NODE_FLAG_IS_LEAF);
   }

   BOOLEAN btreeNode::isVainPrefixRegen()const
   {
      const btreeNodePageHead *head = getReadableHead();
      return 0 != OSS_BIT_TEST(head->flags, BTREE_NODE_FLAG_VAIN_PREFIX_REGENERATION);
   }

   UINT32 btreeNode::getItemCount()const
   {
      return getReadableHead()->totalSlotCount;
   }

   BOOLEAN btreeNode::hasFreeSpaceToInsert(UINT32 keySize,
                                           BOOLEAN *needCompact)const
   {
      const btreeNodePageHead *head = getReadableHead();
      UINT32 size = getSizeToSaveInNode(keySize);
      BOOLEAN r = (size <= head->totalFreeSpace);
      if (r && nullptr != needCompact)
      {
         *needCompact = (getBackOffset() < (size + getFrontOffset()));
      }
      return r;
   }

   BOOLEAN btreeNode::hasCompressedKeys()const
   {
      return 0 < getReadableHead()->compressedItemCount;
   }

   BOOLEAN btreeNode::hasPrefix()const
   {
      return 0 < getReadableHead()->prefixCount;
   }

   UINT32 btreeNode::getFrontOffset()const
   {
      return BTREE_NODE_PAGE_HEAD_SIZE +
             (getReadableHead()->totalSlotCount * BTREE_NODE_SLOT_SIZE);
   }

   UINT32 btreeNode::getBackOffset()const
   {
      return getReadableHead()->backOffset;
   }

   UINT32 btreeNode::getContinuousFreeSpace()const
   {
      return getBackOffset() - getFrontOffset();
   }

   const btreeNodePageHead *btreeNode::getReadableHead()const
   {
      SDB_ASSERT(isValid(), "can not be invalid");
      return _buffer->getReadableBodyBuffer().getReadableObjPtr<btreeNodePageHead>(0);
   }

   strictBuffer btreeNode::getReadableBuffer()const
   {
      SDB_ASSERT(isValid(), "can not be invalid");
      return _buffer->getReadableBodyBuffer();
   }

   DPS_TRANS_ID btreeNode::getTransID()const
   {
      return getReadableHead()->transID;
   }

   btreeItemSlot btreeNode::getItemSlot(RECORD_SLOT_POS pos)const
   {
      return *getReadableSlot(pos);
   }

   BOOLEAN btreeNode::isCompressionDisabled()const
   {
      return !isLeaf() ||
             !getProperties()->isPrefixCompressionEnabled();
   }

   UINT32 btreeNode::getKeyDataOffsetToWrite(const btreeNodePageHead *head,
                                             UINT32 keyDataSize)const
   {
      SDB_ASSERT(0 < keyDataSize, "can not be invalid");
      SDB_ASSERT(nullptr != head, "can not be null");
      SDB_ASSERT(keyDataSize < head->backOffset, "out of bound");
      return head->backOffset - keyDataSize;
   }

   btreeItemSlot *btreeNode::getWritableSlot(RECORD_SLOT_POS pos)
   {
      SDB_ASSERT(INVALID_RECORD_SLOT_POS != pos, "can not be invalid");
      SDB_ASSERT(isValid(), "can not be invalid");
      SDB_ASSERT(_buffer->isWritable(), "must be prepared");
      UINT32 offset = BTREE_NODE_PAGE_HEAD_SIZE +
                      (BTREE_NODE_PREFIX_SLOT_SIZE * getReadableHead()->prefixCount) +
                      (BTREE_NODE_SLOT_SIZE * pos);
      return _buffer->getWritableBodyBuffer().
             getWritableObjPtr<btreeItemSlot>(offset);
   }

   const btreeItemSlot *btreeNode::getReadableSlot(RECORD_SLOT_POS pos)const
   {
      SDB_ASSERT(INVALID_RECORD_SLOT_POS != pos, "can not be invalid");
      SDB_ASSERT(isValid(), "can not be invalid");
      SDB_ASSERT(pos < getReadableHead()->totalSlotCount, "out of bound");
      UINT32 offset = BTREE_NODE_PAGE_HEAD_SIZE +
                      (BTREE_NODE_PREFIX_SLOT_SIZE * getReadableHead()->prefixCount) +
                      (BTREE_NODE_SLOT_SIZE * pos);
      return _buffer->getReadableBodyBuffer().
             getReadableObjPtr<btreeItemSlot>(offset);
   }

   const btreeNodePrefixSlot *btreeNode::getReadablePrefixSlot(UINT16 pos)const
   {
      SDB_ASSERT(isValid(), "can not be invalid");
      const btreeNodePageHead *head = getReadableHead();
      SDB_ASSERT(pos < head->prefixCount, "out of bound");
      UINT32 offset = BTREE_NODE_PAGE_HEAD_SIZE + (pos * BTREE_NODE_PREFIX_SLOT_SIZE);
      return getReadableBuffer().getReadableObjPtr<btreeNodePrefixSlot>(offset);
   }

   btreeNodePrefixSlot *btreeNode::getWritablePrefixSlot(UINT16 pos)
   {
      SDB_ASSERT(isValid() && _buffer->isWritable(), "can not be invalid");
      const btreeNodePageHead *head = getReadableHead();
      SDB_ASSERT(pos < head->prefixCount, "out of bound");
      UINT32 offset = BTREE_NODE_PAGE_HEAD_SIZE + (pos * BTREE_NODE_PREFIX_SLOT_SIZE);
      return _buffer->getWritableBodyBuffer().getWritableObjPtr<btreeNodePrefixSlot>(offset);
   }

   UINT32 btreeNode::getNodeSize()const
   {
      SDB_ASSERT(isValid(), "can not be invalid");
      return _buffer->getReadableBodyBuffer().getSize();
   }

   BOOLEAN btreeNode::hitHighWaterMark()const
   {
      const btreeNodePageHead *head = getReadableHead();
      return ((FLOAT32)head->totalFreeSpace / getNodeSize()) <=
             (1.0 - BTREE_NODE_HIGH_WATER_MARK);
   }

   BOOLEAN btreeNode::isBetterToBeRecompressed()const
   {
      SDB_ASSERT(!isCompressionDisabled(), "can not be disabled");

      return !isCompressionDisabled() &&
             hitHighWaterMark() &&
             !isVainPrefixRegen();
   }

   BOOLEAN btreeNode::isItemMarkedAsDeleted(RECORD_SLOT_POS pos)const
   {
      SDB_ASSERT(isValid(), "can not be invalid");
      SDB_ASSERT(INVALID_RECORD_SLOT_POS != pos, "can not be invalid");
      SDB_ASSERT(!isLeaf(), "can not be leaf");
      SDB_ASSERT(pos < getReadableHead()->totalSlotCount, "out of bound");
      return getReadableSlot(pos)->isMarkedDeleted();
   }

   PAGE_ID btreeNode::getRightChild()const
   {
      SDB_ASSERT(isValid(), "can not be invalid");
      SDB_ASSERT(!isLeaf(), "can not be leaf");
      return getReadableHead()->rightChild;
   }

   BOOLEAN btreeNode::hasRightChild()const
   {
      return INVALID_PAGE_ID != getRightChild();
   }

   BOOLEAN btreeNode::isRightChildLeaf()const
   {
      return getReadableHead()->isRightChildLeaf();
   }

   PAGE_ID btreeNode::getLeftChild(RECORD_SLOT_POS pos)const
   {
      SDB_ASSERT(isValid(), "can not be invalid");
      SDB_ASSERT(INVALID_RECORD_SLOT_POS != pos, "can not be invalid");
      SDB_ASSERT(!isLeaf(), "can not be leaf");
      SDB_ASSERT(pos < getReadableHead()->totalSlotCount, "out of bound");
      return getReadableSlot(pos)->data.nlf.leftChild;
   }

   PAGE_ID btreeNode::getChild(RECORD_SLOT_POS pos)const
   {
      SDB_ASSERT(isValid(), "can not be invalid");
      SDB_ASSERT(INVALID_RECORD_SLOT_POS != pos, "can not be invalid");
      SDB_ASSERT(!isLeaf(), "can not be leaf");

      PAGE_ID child = INVALID_PAGE_ID;
      const btreeNodePageHead *head = getReadableHead();
      if (pos < head->totalSlotCount)
      {
         child = getReadableSlot(pos)->data.nlf.leftChild;
      }
      else if (pos == head->totalSlotCount)
      {
         child = head->rightChild;
      }
      else
      {
         SDB_ASSERT(FALSE, "out of bound");
      }
      return child;
   }

   const indexProperties *btreeNode::getProperties()const
   {
      return nullptr == _ctx ? nullptr : &(_ctx->getIndexObject()->getProperties());
   }

   BOOLEAN btreeNode::betterToBeDestroyed()const
   {
      SDB_ASSERT(isValid(), "can not be invalid");
      if (isLeaf())
      {
         return 0 == getReadableHead()->totalSlotCount;
      }
      else
      {
         return !hasRightChild() &&
                1 == getReadableHead()->totalSlotCount &&
                getReadableSlot(0)->isMarkedDeleted() &&
                INVALID_PAGE_ID == getReadableSlot(0)->data.nlf.leftChild;
      }
   }

   INT32 btreeNode::locateEntry(const btreeKeyStringEntry &entry,
                                btreeNodeSeekResult &res) const
   {
      INT32 rc = SDB_OK;
      res.reset();

      if (OSS_UNLIKELY(!entry.isValid()))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      else if (OSS_UNLIKELY(!isValid()))
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }

      rc = _locateEntry(entry, res);
      if (SDB_OK != rc)
      {
         goto error;
      }

   done:
      return rc;
   error:
      goto done;
   }

   INT32 btreeNode::leafInsert(const btreeKeyStringEntry &entry)
   {
      INT32 rc = SDB_OK;

      if (OSS_UNLIKELY(!entry.isValid()))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      else if (OSS_UNLIKELY(!isValid()))
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }

      rc = _leafInsert(entry);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to insert entry into leaf:%d", rc);
         goto error;
      }

      commit();
   done:
      return rc;
   error:
      goto done;
   }

   INT32 btreeNode::removeEntry(RECORD_SLOT_POS pos)
   {
      INT32 rc = SDB_OK;
      if (OSS_UNLIKELY(!isValidRecordSlotPosition(pos)))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      else if (OSS_UNLIKELY(!isValid()))
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }
      else if (getReadableHead()->totalSlotCount <= pos)
      {
         rc = SDB_OUT_OF_BOUND;
         goto error;
      }
      else if (isLeaf())
      {
         rc = _destroySlot(pos);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to destroy item[%d], rc:%d", rc);
            goto error;
         }
      }
      else
      {
         rc = _nonleafRemove(pos);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to remove entry:%d", rc);
            goto error;
         }
      }

      commit();
   done:
      return rc;
   error:
      goto done;
   }

   INT32 btreeNode::_leafInsert(const btreeKeyStringEntry &entry,
                                RECORD_SLOT_POS pos)
   { 
      INT32 rc = SDB_OK;
      SDB_ASSERT(isValid(), "can not be invalid");
      SDB_ASSERT(entry.isValid() && entry.hasKeyTail(), "can not be invalid");
      SDB_ASSERT(isLeaf(), "must be leaf");

      BOOLEAN needCompact = FALSE;
      UINT32 keySize = entry.getRawDataSize();
      UINT32 savingSize = getSizeToSaveInNode(keySize);
      btreeNodeSeekResult res;
      RECORD_SLOT_POS toInsert = INVALID_RECORD_SLOT_POS;

      if (!hasFreeSpaceToInsert(keySize, &needCompact))
      {
         rc = SDB_VESSEL_NOT_ENOUGH_SPACE_IN_PAGE;
         goto error;
      }

      if (INVALID_RECORD_SLOT_POS != pos)
      {
         if (getReadableHead()->totalSlotCount < pos)
         {
            PD_LOG(PDERROR, "insert pos[%d] out of bound", pos);
            rc = SDB_OUT_OF_BOUND;
            goto error;
         }

         toInsert = pos;
      }
      else if (0 == getReadableHead()->totalSlotCount)
      {
         toInsert = 0;
      }
      else
      {
         rc = _locateEntry(entry, res);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to locate key and rid:%d", rc);
            goto error;
         }
         SDB_ASSERT(!res.isIdentical(), "impossible");
         toInsert = res.slotPos;
      }

      if (needCompact)
      {
         rc = _compact(savingSize);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to compact btree node:%d", rc);
            goto error;
         }
      }

      rc = _insert(toInsert, entry);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to insert to node:%d", rc);
         goto error;
      }

   done:
      return rc;
   error:
      goto done;
   }

   INT32 btreeNode::insertRaisedKey(const btreeSplitRaisedKey &raisedKey)
   {
      INT32 rc = SDB_OK;
      rc = _insertRaisedKey(raisedKey);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to insert raised key:%d", rc);
         goto error;
      }

      commit();
   done:
      return rc;
   error:
      goto done;
   }

   INT32 btreeNode::reactiveRemovedKey(RECORD_SLOT_POS pos)
   {
      INT32 rc = SDB_OK;
      btreeItemSlot *slot = nullptr;

      if (OSS_UNLIKELY(!isValidRecordSlotPosition(pos)))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      else if (OSS_UNLIKELY(!isValid()))
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }
      else if (OSS_UNLIKELY(isLeaf()))
      {
         rc = SDB_VESSEL_OPERATOION_NOT_PERMITTED;
         goto error;
      }
      else if (getReadableHead()->totalSlotCount <= pos)
      {
         rc = SDB_OUT_OF_BOUND;
         goto error;
      }
      else if (!getReadableSlot(pos)->isMarkedDeleted())
      {
         PD_LOG(PDERROR, "item[%d] is not marked as removed", pos);
         rc = SDB_VESSEL_OPERATOION_NOT_PERMITTED;
         goto error;
      }
      else
      {
         rc = _ctx->makeWritable(*_buffer);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to make buffer writable:%d", rc);
            goto error;
         }
      }

      slot = getWritableSlot(pos);
      SDB_ASSERT(nullptr != slot, "impossible");
      OSS_BIT_CLEAR(slot->flags, btreeItemSlot::FLAG_MARKED_DELETED);

      commit();

   done:
      return rc;
   error:
      goto done;
   }

   INT32 btreeNode::_locateEntry(const btreeKeyStringEntry &ks,
                                 btreeNodeSeekResult &res) const
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(isValid(), "can not be invalid");
      SDB_ASSERT(ks.isValid(), "can not be invalid");
      const btreeNodePageHead *head = getReadableHead();
      SDB_ASSERT(nullptr != head, "can not be invalid");
      INT16 count = head->totalSlotCount;
      SDB_ASSERT(0 < count, "can not be invalid");
      RECORD_SLOT_POS low = 0;
      slice target = ks.getKeySlice();
      INT32 cmp = 0;

      res.reset();
      while (0 < count)
      {
         INT16 step = count >> 1;
         RECORD_SLOT_POS pos = low + step;
         _entryRef ref = _getEntryRef(pos);
         SDB_ASSERT(ref.isValid(), "can not be invalid");
         SDB_ASSERT(!ref.slot->isKeyCompressed(), "TODO");
         cmp = keyString::compareCoding(target.getSize(),
                                        target.getData(),
                                        ref.data.getSize(),
                                        ref.data.getData());
         if (0 <= cmp)
         {
            low = pos + 1;
            count -= (step + 1);
         }
         else
         {
            count = step;
         }
      }

      res.res = cmp;
      res.slotPos = low;
      res.isUpperBound = (low == head->totalSlotCount);
      if (!isLeaf())
      {
         res.child = (low == head->totalSlotCount) ?
                     head->rightChild :
                     getReadableSlot(low)->data.nlf.leftChild;
      }
   done:
      return rc;
   error:
      goto done;
   }

   INT32 btreeNode::getItem(RECORD_SLOT_POS pos,
                            btreeNodeItem &item)const
   {
      INT32 rc = SDB_OK;
      _entryRef ref;
      btreeKeyStringEntry entry;

      item.reset();

      if (OSS_UNLIKELY(!isValid()))
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }
      else if (OSS_UNLIKELY(!isValidRecordSlotPosition(pos)))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      else if (OSS_UNLIKELY(getItemCount() <= (UINT32)pos))
      {
         rc = SDB_OUT_OF_BOUND;
         goto error;
      }

      ref = _getEntryRef(pos);
      rc = entry.init(ref.data);
      if (OSS_UNLIKELY(SDB_OK != rc))
      {
         PD_LOG(PDERROR, "failed to parse entry[%d] data:%d", pos, rc);
         goto error;
      }

      item.init(_buffer->getLogicalPid(), pos, *ref.slot, entry);
   done:
      return rc;
   error:
      goto done;
   }

   INT32 btreeNode::_insert(RECORD_SLOT_POS pos,
                            const btreeKeyStringEntry &entry,
                            PAGE_ID leftChild)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(isValid(), "can not be invalid");
      SDB_ASSERT(INVALID_RECORD_SLOT_POS != pos, "can not be invalid");
      SDB_ASSERT(entry.isValid(), "can not be invalid");

      btreeItemSlot *slot = nullptr;
      slice raw = entry.getRawData();
      UINT32 keyOffset = 0;
      UINT32 size = getSizeToSaveInNode(raw.getSize());
      btreeNodePageHead *head = nullptr;
      strictBuffer buffer;
      BOOLEAN appendonly = FALSE;

      rc = _buffer->autoGetWritableBodyBuffer(buffer);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to get writable slice:%d", rc);
         goto error;
      }

      head = buffer.getWritableObjPtr<btreeNodePageHead>(0);
      /// should be prechecked outside
      SDB_ASSERT((size + getFrontOffset()) <= getBackOffset(), "out of bound");
      SDB_ASSERT(pos <= head->totalSlotCount, "impossible");
      appendonly = (pos == head->totalSlotCount);

      keyOffset = getKeyDataOffsetToWrite(head, raw.getSize());
      rc = buffer.write(keyOffset, raw.getSize(), raw.data());
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to write key data to[%d,%d], rc:%d",
                keyOffset, raw.getSize(), rc);
         goto error;
      }

      slot = getWritableSlot(pos);
      if (OSS_UNLIKELY(nullptr == slot))
      {
         PD_LOG(PDERROR, "failed to get writable slot ptr[%d]", pos);
         rc = SDB_VESSEL_INTERNAL_ERR;
         goto error;
      }

      if (pos < head->totalSlotCount)
      {
         UINT32 moveSize = (head->totalSlotCount - pos) * BTREE_NODE_SLOT_SIZE;
         ossMemmove(slot + 1, slot, moveSize);
      }

      /// do not goto error from here
      if (isLeaf())
      {
         slot->initAsLeafFormat(keyOffset, raw.getSize());
      }
      else
      {
         SDB_ASSERT(INVALID_PAGE_ID != leftChild, "can not be invalid");
         slot->initAsNonLeafFormat(keyOffset, raw.getSize(), leftChild);
      }

      updateAppendingFactor(head, appendonly);
      ++head->totalSlotCount;
      head->totalFreeSpace -= size;
      head->backOffset = keyOffset;
   done:
      return rc;
   error:
      goto done;
   }

   INT32 btreeNode::_insertRaisedKey(const btreeSplitRaisedKey &raisedKey,
                                     RECORD_SLOT_POS pos)
   {
      INT32 rc = SDB_OK;
      const btreeNodePageHead *head = nullptr;
      BOOLEAN appendOnly = FALSE;
      UINT32 savingSize = 0;
      RECORD_SLOT_POS insertPos = INVALID_RECORD_SLOT_POS;
      BOOLEAN needCompact = FALSE;

      if (OSS_UNLIKELY(!raisedKey.isValid()))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      else if (OSS_UNLIKELY(!isValid()))
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }
      else if (OSS_UNLIKELY(isLeaf()))
      {
         rc = SDB_VESSEL_OPERATOION_NOT_PERMITTED;
         goto error;
      }
      else if (!hasFreeSpaceToInsert(raisedKey.entry.getRawDataSize(), &needCompact))
      {
         PD_LOG(PDERROR, "not enough free space to save raised key");
         rc = SDB_VESSEL_NOT_ENOUGH_SPACE_IN_PAGE;
         goto error;
      }

      savingSize = getSizeToSaveInNode(raisedKey.entry.getRawDataSize());
      head = getReadableHead();

      if (INVALID_RECORD_SLOT_POS != pos)
      {
         if (head->totalSlotCount < pos)
         {
            PD_LOG(PDERROR, "insert pos[%d] out of bound", pos);
            rc = SDB_OUT_OF_BOUND;
            goto error;
         }
         insertPos = pos;
      }
      else if (0 < head->totalSlotCount)
      {
         btreeNodeSeekResult res;
         rc = _locateEntry(raisedKey.entry, res);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to locate pos to insert:%d", rc);
            goto error;
         }
         SDB_ASSERT(!res.isIdentical(), "impossible");
         insertPos = res.slotPos;
      }
      else
      {
         insertPos = 0;
      }

      appendOnly = (insertPos == head->totalSlotCount);

      if (needCompact)
      {
         rc = _compact(savingSize);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to compact node:%d", rc);
            goto error;
         }
      }
      else
      {
         rc = _ctx->makeWritable(*_buffer);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to make buffer writable:%d", rc);
            goto error;
         }
      }

      rc = _insert(insertPos, raisedKey.entry, raisedKey.leftChild);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to insert key:%d", rc);
         goto error;
      }

      if (appendOnly)
      {
         strictBuffer buffer = _buffer->getWritableBodyBuffer();
         SDB_ASSERT(buffer.isWritable(), "must be writable");
         btreeNodePageHead *h = buffer.getWritableObjPtr<btreeNodePageHead>(0);
         SDB_ASSERT(raisedKey.leftChild == h->rightChild, "must be same");
         h->rightChild = raisedKey.rightChild;
         if (raisedKey.fromLeaf)
         {
            OSS_BIT_SET(h->flags, BTREE_NODE_FLAG_RIGHT_CHILD_IS_LEAF);
         }
      }
      else
      {
         btreeItemSlot *slot = getWritableSlot(insertPos + 1);
         slot->data.nlf.leftChild = raisedKey.rightChild;
         if (raisedKey.fromLeaf)
         {
            slot->setRaisedFromLeaf();
         }
      }
   done:
      return rc;
   error:
      goto done;
   }

   INT32 btreeNode::_findSplitPivot(BOOLEAN idleRight,
                                   RECORD_SLOT_POS &pivot)const
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(isValid(), "can not be invalid");
      SDB_ASSERT(!hasCompressedKeys(), "TODO");
      UINT32 splitSize = 0;
      UINT32 scanned = 0;
      const btreeNodePageHead *head = nullptr;
      static const UINT32 _MIN_SPLIT_ITEM_COUNT = 3;
      UINT32 factor = idleRight ? 10 : 2;

      pivot = INVALID_RECORD_SLOT_POS;
      head = getReadableHead();
      if (head->totalSlotCount < _MIN_SPLIT_ITEM_COUNT)
      {
         PD_LOG(PDERROR, "too few item count to split");
         rc = SDB_VESSEL_OPERATOION_NOT_PERMITTED;
         goto error;
      }

      splitSize = getNodeSize() / factor;

      for (INT32 i = (INT32)(head->totalSlotCount) - 1; 0 <= i; --i)
      {
         UINT32 savingSize = 0;
         const btreeItemSlot *slot = getReadableSlot(i);
         if (OSS_UNLIKELY(nullptr == slot || !slot->isValid()))
         {
            PD_LOG(PDERROR, "invalid slot [%d] found", i);
            rc = SDB_VESSEL_INTERNAL_ERR;
            goto error;
         }

         savingSize = slot->data.key.size + BTREE_NODE_SLOT_SIZE;

         scanned += savingSize;
         if (splitSize < scanned)
         {
            break;
         }

         pivot = i;
      }

      if (OSS_UNLIKELY(INVALID_RECORD_SLOT_POS == pivot ||
                       (head->totalSlotCount - 1) == pivot))
      {
         pivot = head->totalSlotCount - 2;
      }
      
      if (OSS_UNLIKELY(0 == pivot))
      {
         PD_LOG(PDERROR, "pivot slot number can not be zero");
         rc = SDB_VESSEL_INTERNAL_ERR;
         goto error;
      }
   done:
      return rc;
   error:
      pivot = INVALID_RECORD_SLOT_POS;
      goto done;
   }

   INT32 btreeNode::splitLeafAndInsert(const btreeKeyStringEntry &entry,
                                       btreeSplitRaisedKey &raisedKey)
   {
      INT32 rc = SDB_OK;
      RECORD_SLOT_POS pivot = INVALID_RECORD_SLOT_POS;
      BOOLEAN idleRight = FALSE;
      btreeNodeSeekResult res;
      logicalPageBuffer rightNodeBuffer;
      raisedKey.reset();

      if (OSS_UNLIKELY(!isValid()))
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }
      else if (OSS_UNLIKELY(!entry.isValid()))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      else if (!isLeaf())
      {
         rc = SDB_VESSEL_OPERATOION_NOT_PERMITTED;
         goto error;
      }

      rc = _locateEntry(entry, res);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to locate key and rid:%d", rc);
         goto error;
      }

      if (res.isUpperBound && isRecentWriteOrdered())
      {
         idleRight = TRUE;
      }

      rc = _findSplitPivot(idleRight, pivot);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to find split pivot:%d", rc);
         goto error;
      }

      rc = _saveRaisingEntry(pivot, raisedKey.entry);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to save raising entry:%d", rc);
         goto error;
      }

      rc = _split(pivot, rightNodeBuffer);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to split node:%d", rc);
         goto error;
      }

      raisedKey.leftChild = _buffer->getLogicalPid();
      raisedKey.rightChild = rightNodeBuffer.getLogicalPid();
      raisedKey.fromLeaf = TRUE;

      /// should not get error from here
      if (res.slotPos <= pivot)
      {         
         rc = _leafInsert(entry, res.slotPos);
         if (SDB_OK != rc)
         {
            PD_LOG(PDSEVERE, "failed to insert key after node[%d,%d] split:%d",
                   _ctx->getIndexObject()->getLogicalID(), 
                   _buffer->getLogicalPid(), rc);
            ossPanic();
            goto error;
         }

         commit();
      }
      else
      {
         btreeNode rightNode(_depth, &rightNodeBuffer, _ctx);
         rc = rightNode._leafInsert(entry, res.slotPos - pivot - 1);
         if (SDB_OK != rc)
         {
            PD_LOG(PDSEVERE, "failed to insert key into right node[%d,%d]:%d",
                   _ctx->getIndexObject()->getLogicalID(),
                   rightNodeBuffer.getLogicalPid(), rc);
            ossPanic();
            goto error;
         }

         rightNode.commit();
      }
   done:
      rightNodeBuffer.fini();
      return rc;
   error:
      raisedKey.reset();
      goto done;
   }

   INT32 btreeNode::_split(RECORD_SLOT_POS pivot, logicalPageBuffer &rightNodeBuffer)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(isValid(), "can not be invalid");
      SDB_ASSERT(INVALID_RECORD_SLOT_POS != pivot, "can not be invalid");

      memoryBlock mb;
      btreeNodePageSplitIniter initer;
      strictBuffer buffer;
      btreeNodePageHead *head = nullptr;
      PAGE_ID pivotLeftChild = INVALID_PAGE_ID;
      BOOLEAN newRightChildIsLeaf = FALSE;

      rightNodeBuffer.fini();

      rc = mb.reserve(getNodeSize());
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to reserve memory:%d", rc);
         goto error;
      }

      buffer.makeWritable(mb.getCapacity(), mb.getBuffer());
      rc = _buildRightNodeWhenSplit(pivot + 1, buffer);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to build right node:%d", rc);
         goto error;
      }
      SDB_ASSERT(0 < pivot, "can not be zero");

      rc = _ctx->makeWritable(*_buffer);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to make buffer writable:%d", rc);
         goto error;
      }

      initer.set(buffer.getSlice());
      rc = _ctx->allocateNewNode(&initer, rightNodeBuffer);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to allocate right node buffer:%d", rc);
         goto error;
      }

      if (!isLeaf())
      {
         pivotLeftChild = getReadableSlot(pivot)->data.nlf.leftChild;
         newRightChildIsLeaf = getReadableSlot(pivot)->isRaisedFromLeaf();
      }

      rc = truncate(pivot - 1);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to truncate node:%d", rc);
         goto error;
      }

      /// do not goto error from here
      head = _buffer->getWritableBodyBuffer().getWritableObjPtr<btreeNodePageHead>(0);
      if (head->isRoot())
      {
         head->resetRoot();
      }

      if (!isLeaf())
      {
         head->rightChild = pivotLeftChild;
         if (newRightChildIsLeaf)
         {
            OSS_BIT_SET(head->flags, BTREE_NODE_FLAG_RIGHT_CHILD_IS_LEAF);
         }
      }
      else if ((0 != OSS_BIT_TEST(head->flags, BTREE_NODE_FLAG_VAIN_PREFIX_REGENERATION)) &&
               !hitHighWaterMark())
      {
         OSS_BIT_CLEAR(head->flags, BTREE_NODE_FLAG_VAIN_PREFIX_REGENERATION);
      }

   done:
      return rc;
   error:
      if (rightNodeBuffer.isValid())
      {
         _ctx->destroyNode(rightNodeBuffer);
      }
      goto done;
   }

   INT32 btreeNode::_buildRightNodeWhenSplit(RECORD_SLOT_POS begin,
                                             strictBuffer &node)const
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(isValid(), "can not be invalid");
      SDB_ASSERT(INVALID_RECORD_SLOT_POS != begin, "can not be invalid");
      SDB_ASSERT(getNodeSize() == node.getSize(), "must be same");

      btreeNodePageHead *newHead = node.getWritableObjPtr<btreeNodePageHead>(0);
      const btreeNodePageHead *head = getReadableHead();
      SDB_ASSERT(begin < head->totalSlotCount, "invalid begin");
      UINT32 frontOffset = BTREE_NODE_PAGE_HEAD_SIZE;

      newHead->initAsRightNode(*head, _buffer->getPageSize());

      for (RECORD_SLOT_POS i = begin; i < head->totalSlotCount; ++i)
      {
         btreeItemSlot *slot = nullptr;
         UINT32 keyOffset = 0;
         _entryRef ref = _getEntryRef(i);
         SDB_ASSERT(ref.isValid(), "impossible");
         SDB_ASSERT(!ref.slot->isKeyCompressed(), "TODO");

         if (newHead->backOffset <
             (frontOffset + getSizeToSaveInNode(ref.data.size())))
         {
            PD_LOG(PDERROR, "not enough free space to save item");
            rc = SDB_VESSEL_NOT_ENOUGH_SPACE_IN_PAGE;
            goto error;
         }

         keyOffset = getKeyDataOffsetToWrite(newHead, ref.data.size());
         rc = node.write(keyOffset, ref.data.size(), ref.data.data());
         if (OSS_UNLIKELY(SDB_OK != rc))
         {
            PD_LOG(PDERROR, "failed to write slice[%d,%d], rc:%d",
                   keyOffset, ref.data.size(), rc);
            goto error;
         }

         slot = node.getWritableObjPtr<btreeItemSlot>(frontOffset);
         if (isLeaf())
         {
            slot->initAsLeafFormat(keyOffset, ref.data.size(), FALSE);
         }
         else
         {
            slot->initAsNonLeafFormat(keyOffset,
                                      ref.data.size(),
                                      ref.slot->data.nlf.leftChild);
         }
         newHead->totalFreeSpace -= BTREE_NODE_SLOT_SIZE;
         newHead->totalFreeSpace -= ref.data.size();
         newHead->backOffset = keyOffset;
         ++newHead->totalSlotCount;
         frontOffset += BTREE_NODE_SLOT_SIZE;
      }//for (RECORD_SLOT_POS i = begin; i < head->totalSlotCount; ++i)
   done:
      return rc;
   error:
      goto done;
   }

   void btreeNode::commit()
   {
      if (isValid() && _buffer->isWritable())
      {
         IExecutor *executor = _buffer->getContext()->getExecutor();
         updateTransID(executor->getTransID());
         _buffer->commit(executor->getEndLsn());
      }
      return;
   }

   void btreeNode::updateTransID(const DPS_TRANS_ID &transID)
   {
      SDB_ASSERT(isValid() && _buffer->isWritable(), "must be writable");
      if (transID.isValid())
      {
         btreeNodePageHead *head = _buffer->getWritableBodyBuffer().
                                   getWritableObjPtr<btreeNodePageHead>(0);
         if (!head->transID.isValid() ||
             head->transID < transID)
         {
            head->transID = transID;
         }
      }
   }

   static const UINT32 _ORDERED_W_FACTOR = 16;

   BOOLEAN btreeNode::isRecentWriteOrdered()const
   {
      return _ORDERED_W_FACTOR == getReadableHead()->appendingFactor;
   }

   void btreeNode::updateAppendingFactor(btreeNodePageHead *head,
                                         BOOLEAN isAppending)
   {
      SDB_ASSERT(nullptr != head, "can not be null");

      if (isAppending && head->appendingFactor < _ORDERED_W_FACTOR)
      {
         ++head->appendingFactor;
      }
      else
      {
         head->appendingFactor = 0;
      }
      return;
   }

   INT32 btreeNode::_compact(UINT32 reserved)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(isValid(), "can not be invalid");
      SDB_ASSERT(!hasCompressedKeys(), "TODO");
      memoryBlock mb;
      strictBuffer compactionBuffer;
      UINT32 frontOffset = BTREE_NODE_PAGE_HEAD_SIZE;
      UINT32 backOffset = getNodeSize();
      btreeNodePageHead *head = nullptr;
      const btreeNodePageHead *rh = getReadableHead();
      strictBuffer writableBuffer;


      if (getReadableHead()->totalFreeSpace == getContinuousFreeSpace())
      {
         /// already been compacted
         goto done;
      }

      rc = mb.reserve(getNodeSize());
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to reserve memory block size:%d", rc);
         goto error;
      }

      compactionBuffer.makeWritable(mb.getCapacity(), mb.getBuffer());

      rc = _buffer->autoGetWritableBodyBuffer(writableBuffer);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to prepare to write:%d", rc);
         goto error;
      }

      head = compactionBuffer.getWritableObjPtr<btreeNodePageHead>(0);
      ossMemcpy(head, rh, BTREE_NODE_PAGE_HEAD_SIZE);

      for (RECORD_SLOT_POS i = 0; i < rh->totalSlotCount; ++i)
      {
         _entryRef ref = _getEntryRef(i);
         SDB_ASSERT(ref.isValid(), "can not be invalid");
         SDB_ASSERT(!ref.slot->isKeyCompressed(), "TODO");

         btreeItemSlot newSlot;
         if (backOffset < (frontOffset + getSizeToSaveInNode(ref.data.size())))
         {
            PD_LOG(PDERROR, "not enough free space to insert item");
            SDB_ASSERT(FALSE, "impossible");
            rc = SDB_VESSEL_NOT_ENOUGH_FREE_RESOURCE;
            goto error;
         }

         if (0 < ref.data.size())
         {
            backOffset -= ref.data.size();
            rc = compactionBuffer.write(backOffset, ref.data.size(), ref.data.data());
            if (OSS_UNLIKELY(SDB_OK != rc))
            {
               PD_LOG(PDERROR, "failed to copy key data:%d", rc);
               SDB_ASSERT(FALSE, "impossible");
               goto error;
            }

            if (isLeaf())
            {
               newSlot.initAsLeafFormat(backOffset, ref.data.size(), FALSE);
            }
            else
            {
               newSlot.initAsNonLeafFormat(backOffset, ref.data.size(),
                                           ref.slot->data.nlf.leftChild);
            }
         }
         else
         {
            newSlot = *ref.slot;
         }

         *(compactionBuffer.getWritableObjPtr<btreeItemSlot>(frontOffset)) = newSlot;
         frontOffset += BTREE_NODE_SLOT_SIZE;
      }

      head->totalFreeSpace = backOffset - frontOffset;
      head->backOffset = backOffset;

      ossMemcpy(writableBuffer.getWPtr(),
                compactionBuffer.getRPtr(),
                compactionBuffer.getSize());
   done:
      return rc;
   error:
      goto done;
   }

   INT32 btreeNode::truncate(RECORD_SLOT_POS max)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(isValid() && _buffer->isWritable(), "can not be invalid");
      SDB_ASSERT(INVALID_RECORD_SLOT_POS != max, "can not be invalid");
      UINT32 size = 0;
      btreeNodePageHead *head = _buffer->getWritableBodyBuffer().getWritableObjPtr<btreeNodePageHead>(0);
      if (nullptr == head)
      {
         PD_LOG(PDERROR, "failed to get writable head ptr");
         rc = SDB_VESSEL_INTERNAL_ERR;
         goto error;
      }

      SDB_ASSERT((UINT32)(max + 1) < head->totalSlotCount, "invalid truncate range");
      SDB_ASSERT(!hasCompressedKeys(), "TODO");

      for (RECORD_SLOT_POS i = head->totalSlotCount - 1; i > max; --i)
      {
         const btreeItemSlot *slot = getReadableSlot(i);
         size += BTREE_NODE_SLOT_SIZE;
         size += slot->data.key.size;
      }

      head->totalFreeSpace += size;
      head->totalSlotCount = max + 1;
   done:
      return rc;
   error:
      goto done;
   }

   INT32 btreeNode::splitNonLeafAndInsert(const btreeSplitRaisedKey &raisedKeyFromChild,
                                          btreeSplitRaisedKey &raisedKey)
   {
      INT32 rc = SDB_OK;
      btreeNodeSeekResult res;
      RECORD_SLOT_POS pivot = INVALID_RECORD_SLOT_POS;
      BOOLEAN idleRight = FALSE;
      logicalPageBuffer rightNodeBuffer;
      raisedKey.reset();

      if (OSS_UNLIKELY(!isValid()))
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }
      else if (OSS_UNLIKELY(isLeaf()))
      {
         rc = SDB_VESSEL_OPERATOION_NOT_PERMITTED;
         goto error;
      }
      else if (OSS_UNLIKELY(!raisedKeyFromChild.isValid()))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      rc = _locateEntry(raisedKeyFromChild.entry, res);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to locate key and rid:%d", rc);
         goto error;
      }

      if (res.slotPos == getReadableHead()->totalSlotCount &&
          isRecentWriteOrdered())
      {
         idleRight = TRUE;
      }

      rc = _findSplitPivot(idleRight, pivot);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to find split pivot:%d", rc);
         goto error;
      }

      rc = _saveRaisingEntry(pivot, raisedKey.entry);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to save raising entry:%d", rc);
         goto error;
      }

      rc = _split(pivot, rightNodeBuffer);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to split node:%d", rc);
         goto error;
      }

      raisedKey.leftChild = _buffer->getLogicalPid();
      raisedKey.rightChild = rightNodeBuffer.getLogicalPid();
      raisedKey.fromLeaf = FALSE;

      /// should not get error from here
      if (res.slotPos <= pivot)
      {
         rc = _insertRaisedKey(raisedKeyFromChild, res.slotPos);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to insert raised key to current node:%d", rc);
            ossPanic();
            goto error;
         }

         commit();
      }
      else
      {
         btreeNode rightNode(_depth, &rightNodeBuffer, _ctx);
         rc = rightNode._insertRaisedKey(raisedKeyFromChild, res.slotPos - pivot - 1);
         if (SDB_OK != rc)
         {
            PD_LOG(PDSEVERE, "failed to insert raised key into right node[%d,%d]:%d",
                   _ctx->getIndexObject()->getLogicalID(),
                   rightNodeBuffer.getLogicalPid(), rc);
            ossPanic();
            goto error;
         }

         rightNode.commit();
      }
   done:
      return rc;
   error:
      raisedKey.reset();
      goto done;
   }

   INT32 btreeNode::removeChild(RECORD_SLOT_POS pos)
   {
      INT32 rc = SDB_OK;
      if (OSS_UNLIKELY(INVALID_RECORD_SLOT_POS == pos))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      else if (OSS_UNLIKELY(!isValid()))
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }
      else if (OSS_UNLIKELY(isLeaf()))
      {
         rc = SDB_VESSEL_OPERATOION_NOT_PERMITTED;
         goto error;
      }

      rc = _removeChild(pos);
      if (SDB_OK != rc)
      {
         goto error;
      }

      commit();
   done:
      return rc;
   error:
      goto done;
   }

   INT32 btreeNode::_removeChild(RECORD_SLOT_POS pos)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(isValid(), "can not be invalid");
      SDB_ASSERT(INVALID_RECORD_SLOT_POS != pos, "can not be invalid");
      SDB_ASSERT(!isLeaf(), "can not be leaf");

      const btreeNodePageHead *rh = getReadableHead();
      btreeNodePageHead *head = nullptr;
      strictBuffer buffer;

      if (rh->totalSlotCount < pos)
      {
         PD_LOG(PDERROR, "pos[%d] to remove is out of total slot count[%d]",
                pos, rh->totalSlotCount);
         rc = SDB_OUT_OF_BOUND;
         goto error;
      }
      else if (INVALID_PAGE_ID == getChild(pos))
      {
         if (INVALID_PAGE_ID == rh->rightChild)
         {
            PD_LOG(PDERROR, "already has no child in pos[%d]", pos);
            rc = SDB_VESSEL_OPERATOION_NOT_PERMITTED;
            goto error;
         }
      }

      rc = _ctx->makeWritable(*_buffer);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to get node ready to write:%d", rc);
         goto error;
      }

      rc = _buffer->autoGetWritableBodyBuffer(buffer);
      SDB_ASSERT(SDB_OK == rc, "must be ok");

      rh = nullptr;
      head = buffer.getWritableObjPtr<btreeNodePageHead>(0);
      if (pos == head->totalSlotCount)
      {
         head->rightChild = INVALID_PAGE_ID;
         OSS_BIT_CLEAR(head->flags, BTREE_NODE_FLAG_RIGHT_CHILD_IS_LEAF);
      }
      else
      {
         btreeItemSlot *slot = getWritableSlot(pos);
         SDB_ASSERT(INVALID_PAGE_ID != slot->data.nlf.leftChild, "impossible");
         slot->data.nlf.leftChild = INVALID_PAGE_ID;
         slot->clearRaisedFromLeaf();

         /// non-leaf node must have one item at least
         if (slot->isMarkedDeleted() && 1 < head->totalSlotCount)
         {
            rc = _destroySlot(pos);
            if (SDB_OK != rc)
            {
               PD_LOG(PDERROR, "failed to destroy slot[%d], rc:%d", pos, rc);
               goto error;
            }
         }
      }

   done:
      return rc;
   error:
      goto done;
   }

   INT32 btreeNode::_destroySlot(RECORD_SLOT_POS pos)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(INVALID_RECORD_SLOT_POS != pos, "can not be invalid");
      SDB_ASSERT(isValid(), "can not be invalid");
      SDB_ASSERT(pos < getReadableHead()->totalSlotCount, "out of bound");

      btreeNodePageHead *head = nullptr;
      btreeItemSlot *slot = nullptr;
      UINT32 size = BTREE_NODE_SLOT_SIZE;
      strictBuffer buffer;

      rc = _ctx->makeWritable(*_buffer);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to get node ready to write:%d", rc);
         goto error;
      }

      rc = _buffer->autoGetWritableBodyBuffer(buffer);
      SDB_ASSERT(SDB_OK == rc, "must be ok");

      head = buffer.getWritableObjPtr<btreeNodePageHead>(0);
      if (OSS_UNLIKELY(nullptr == head))
      {
         PD_LOG(PDERROR, "failed to get page head");
         rc = SDB_VESSEL_INTERNAL_ERR;
         goto error;
      }

      slot = getWritableSlot(pos);
      SDB_ASSERT(!slot->hasPrefixSlot(), "TODO");
      size += slot->data.key.size;

      if ((pos + 1) < head->totalSlotCount)
      {
         UINT32 moveSize = (head->totalSlotCount - pos - 1) *
                           BTREE_NODE_SLOT_SIZE;
         ossMemmove(slot, slot + 1, moveSize);
      }

      --head->totalSlotCount;
      head->totalFreeSpace += size;

   done:
      return rc;
   error:
      goto done;
   }

   INT32 btreeNode::_nonleafRemove(RECORD_SLOT_POS pos)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(isValidRecordSlotPosition(pos), "can not be invalid");
      SDB_ASSERT(isValid(), "can not be invalid");
      SDB_ASSERT(pos < (INT16)getItemCount(), "out of bound");
      SDB_ASSERT(!isLeaf(), "can not be leaf");
      SDB_ASSERT(pos < getReadableHead()->totalSlotCount, "out of bound");

      const btreeItemSlot *rs = getReadableSlot(pos);

      if (rs->isMarkedDeleted())
      {
         PD_LOG(PDERROR, "item with pos[%d] has already been removed", pos);
         rc = SDB_VESSEL_IXM_ITEM_NOT_FOUND;
         goto error;
      }
      else if (INVALID_PAGE_ID != rs->data.nlf.leftChild ||
               1 == getItemCount())
      {
         rc = _markRemoved(pos);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to mark pos[%d] removed:%d", pos, rc);
            goto error;
         }
      }
      else
      {
         rc = _destroySlot(pos);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to destroy slot[%d], rc:%d", rc);
            goto error;
         }
      }
   done:
      return rc;
   error:
      goto done;
   }

   INT32 btreeNode::_markRemoved(RECORD_SLOT_POS pos)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(isValidRecordSlotPosition(pos), "can not be invalid");
      SDB_ASSERT(isValid(), "can not be invalid");
      SDB_ASSERT(pos < (INT16)getItemCount(), "out of bound");
      SDB_ASSERT(!isLeaf(), "can not be leaf");

      btreeItemSlot *slot = nullptr;

      rc = _ctx->makeWritable(*_buffer);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to get buffer ready to write:%d", rc);
         goto error;
      }

      slot = getWritableSlot(pos);
      SDB_ASSERT(!slot->isMarkedDeleted(), "already been removed");
      slot->markDeleted();
   done:
      return rc;
   error:
      goto done;
   }

   BOOLEAN btreeNode::becameEmptyAfterRemoving(RECORD_SLOT_POS pos)const
   {
      SDB_ASSERT(isValid(), "can not be invalid");
      SDB_ASSERT(isValidRecordSlotPosition(pos), "can not be invalid");
      SDB_ASSERT(pos < (INT16)getItemCount(), "out of bound");

      BOOLEAN r = FALSE;
      if (isLeaf())
      {
         if (1 == getItemCount())
         {
            r = TRUE;
         }
      }
      else
      {
         if (!hasRightChild() &&
             1 == getItemCount() &&
             getReadableSlot(pos)->data.nlf.leftChild == INVALID_PAGE_ID)
         {
            r = TRUE;
         }
      }

      return r;
   }

   INT32 btreeNode::resetRemovedChild(RECORD_SLOT_POS pos,
                                      PAGE_ID child,
                                      BOOLEAN childIsLeaf)
   {
      INT32 rc = SDB_OK;
      if (OSS_UNLIKELY(INVALID_RECORD_SLOT_POS == pos ||
                       INVALID_PAGE_ID == child))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      else if (OSS_UNLIKELY(!isValid()))
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }
      else if (OSS_UNLIKELY(isLeaf()))
      {
         rc = SDB_VESSEL_OPERATOION_NOT_PERMITTED;
         goto error;
      }
      else if ((INT16)getItemCount() < pos)
      {
         rc = SDB_OUT_OF_BOUND;
         goto error;
      }
      else if (INVALID_PAGE_ID != getChild(pos))
      {
         PD_LOG(PDERROR, "child at [%d] is valid", pos);
         rc = SDB_VESSEL_OPERATOION_NOT_PERMITTED;
         goto error;
      }

      rc = _ctx->makeWritable(*_buffer);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to get node ready to write:%d", rc);
         goto error;
      }

      if (pos < (INT16)getItemCount())
      {
         btreeItemSlot *slot = getWritableSlot(pos);
         slot->data.nlf.leftChild = child;
         if (childIsLeaf)
         {
            slot->setRaisedFromLeaf();
         }
      }
      else
      {
         btreeNodePageHead *head = _buffer->getWritableBodyBuffer().
                                   getWritableObjPtr<btreeNodePageHead>(0);
         head->rightChild = child;
         if (childIsLeaf)
         {
            OSS_BIT_SET(head->flags, BTREE_NODE_FLAG_RIGHT_CHILD_IS_LEAF);
         }
      }
      commit();

   done:
      return rc;
   error:
      goto done;
   }

   void btreeNode::dumpAllSubNodes(ossPoolVector<PAGE_ID> &nodes)
   {
      SDB_ASSERT(isValid(), "can not be invalid");
      SDB_ASSERT(!isLeaf(), "can not be leaf");
      const btreeNodePageHead *header = getReadableHead();
      if (INVALID_PAGE_ID != header->rightChild)
      {
         nodes.push_back(header->rightChild);
      }

      for (UINT32 i = 0; i < getItemCount(); ++i)
      {
         const btreeItemSlot *slot = getReadableSlot(i);
         if (INVALID_PAGE_ID != slot->data.nlf.leftChild)
         {
            nodes.push_back(slot->data.nlf.leftChild);
         }
      }

      return;
   }

   INT32 btreeNode::resetAsEmptyNode()
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(isValid(), "can not be invalid");
      btreeNodePageHead *header = nullptr;
      strictBuffer buffer;
      BOOLEAN isRootNode = isRoot();
      BOOLEAN isLeafNode = isLeaf();

      rc = _buffer->autoGetWritableBodyBuffer(buffer);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to prepare to write:%d", rc);
         goto error;
      }

      header = buffer.getWritableObjPtr<btreeNodePageHead>(0);
      header->totalFreeSpace = getNodeSize() - BTREE_NODE_PAGE_HEAD_SIZE;
      header->totalSlotCount = 0;
      header->backOffset = getNodeSize();
      header->prefixCount = 0;
      header->compressedItemCount = 0;
      header->appendingFactor = 0;
      header->rightChild = INVALID_PAGE_ID;
      header->transID.reset();
      header->flags = 0;
      if (isRootNode)
      {
         OSS_BIT_SET(header->flags, BTREE_NODE_FLAG_IS_ROOT);
      }
      if (isLeafNode)
      {
         OSS_BIT_SET(header->flags, BTREE_NODE_FLAG_IS_LEAF);
      }

      commit();

   done:
      return rc;
   error:
      goto done;
   }

   btreeNode::_entryRef btreeNode::_getEntryRef(RECORD_SLOT_POS pos) const
   {
      btreeNode::_entryRef ref;
      strictBuffer buffer = _buffer->getReadableBodyBuffer();
      const btreeItemSlot *slot = getReadableSlot(pos);
      if (nullptr != slot)
      {
         const CHAR *data = buffer.getReadablePtr(slot->data.key.offset,
                                                  slot->data.key.size);
         if (OSS_UNLIKELY(nullptr == data))
         {
            PD_LOG(PDERROR, "failed to get key data[%d,%d]",
                   slot->data.key.offset,
                   slot->data.key.size);
         }
         else
         {
            ref.slot = slot;
            ref.data.reset(slot->data.key.size, data);
         }
      }
      
      return ref;
   }

   INT32 btreeNode::seek(const keyString &ks,
                         btreeNodeSeekResult &res) const
   {
      INT32 rc = SDB_OK;
      res.reset();
      
      if (OSS_UNLIKELY(!ks.isValid()))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      else if (OSS_UNLIKELY(!isValid()))
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }
      else
      {
         rc = _seek(ks, 0, TRUE, res);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to seek in node:%d", rc);
            goto error;
         }
      }
   done:
      return rc;
   error:
      goto done;
   }

   INT32 btreeNode::seek(const keyString &ks,
                         RECORD_SLOT_POS pos,
                         BOOLEAN forward,
                         btreeNodeSeekResult &res) const
   {
      INT32 rc = SDB_OK;
      if (OSS_UNLIKELY(!ks.isValid() ||
                       !isValidRecordSlotPosition(pos)))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      else if (OSS_UNLIKELY(!isValid()))
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }
      else
      {
         rc = _seek(ks, pos, forward, res);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to seek in node:%d", rc);
            goto error;
         }
      }
   done:
      return rc;
   error:
      goto done;
   }

   INT32 btreeNode::isOutOfKeyBound(const keyString &ks,
                                    BOOLEAN forward,
                                    BOOLEAN &outOfBound) const
   {
      INT32 rc = SDB_OK;
      RECORD_SLOT_POS pos = INVALID_RECORD_SLOT_POS;
      INT32 direction = forward ? 1 : -1;
      _entryRef ref;
      btreeKeyStringEntry entry;
      INT32 result = 0;

      if (OSS_UNLIKELY(!isValid()))
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }
      else if (OSS_UNLIKELY(!ks.isValid()))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      else if (forward)
      {
         UINT32 count = getItemCount();
         SDB_ASSERT(0 < count, "can not be invalid");
         pos = count - 1;
      }
      else
      {
         pos = 0;
      }

      ref = _getEntryRef(pos);
      SDB_ASSERT(ref.isValid(), "can not be invalid");
      rc = entry.init(ref.data);
      if (OSS_UNLIKELY(SDB_OK != rc))
      {
         PD_LOG(PDERROR, "failed to init btree entry:%d", rc);
         goto error;
      }

      result = entry.compareElements(ks);
      outOfBound = ((result * direction) < 0);
   done:
      return rc;
   error:
      goto done;
   }

   INT32 btreeNode::_seek(const keyString &ks,
                          RECORD_SLOT_POS pos,
                          BOOLEAN forward,
                          btreeNodeSeekResult &res) const
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(ks.isValid(), "can not be invalid");
      SDB_ASSERT(isValidRecordSlotPosition(pos), "can not be invalid");

      const btreeNodePageHead *head = getReadableHead();
      SDB_ASSERT(nullptr != head, "can not be invalid");

      INT32 cmp = 0;
      INT16 count = 0;
      RECORD_SLOT_POS low = forward ? pos : 0;
      RECORD_SLOT_POS high = forward ? head->totalSlotCount : pos;
      res.reset();

      if (high <= low)
      {
         PD_LOG(PDERROR, "invalid seek range[%d, %d]", low, high);
         rc = SDB_OUT_OF_BOUND;
         goto error;
      }

      count = high - low;

      while (0 < count)
      {
         INT16 step = count >> 1;
         RECORD_SLOT_POS pos = low + step;
         _entryRef ref = _getEntryRef(pos);
         SDB_ASSERT(ref.isValid(), "can not be invalid");
         SDB_ASSERT(!ref.slot->isKeyCompressed(), "TODO");
         keyString entry(ref.data);
         if (OSS_UNLIKELY(!entry.isValid()))
         {
            PD_LOG(PDERROR, "index entry may be crashed[%d, %d]",
                   _buffer->getLogicalPid(), pos);
            rc = SDB_VESSEL_PAGE_CRASHED;
            goto error;
         }

         cmp = ks.compareElements(entry);
         if (0 <= cmp)
         {
            low = pos + 1;
            count -= (step + 1);
         }
         else
         {
            count = step;
         }
      }

      res.res = cmp;
      res.slotPos = low;
      res.isUpperBound = (low == head->totalSlotCount);
      if (!isLeaf())
      {
         res.child = (low == head->totalSlotCount) ?
                     head->rightChild :
                     getReadableSlot(low)->data.nlf.leftChild;
      }
   done:
      return rc;
   error:
      goto done;
   }

   INT32 btreeNode::_saveRaisingEntry(RECORD_SLOT_POS pos,
                                      btreeKeyStringEntry &entry) const
   {
      INT32 rc = SDB_OK;
      _entryRef ref = _getEntryRef(pos);
      SDB_ASSERT(ref.isValid(), "can not be invalid");
      entry.reset();

      rc = entry.init(ref.data);
      if (OSS_UNLIKELY(SDB_OK != rc))
      {
         PD_LOG(PDERROR, "failed to init index entry:%d", rc);
         goto error;
      }

      rc = entry.getOwned();
      if (OSS_UNLIKELY(SDB_OK != rc))
      {
         PD_LOG(PDERROR, "failed to get owned entry:L%d", rc);
         goto error;
      }
   done:
      return rc;
   error:
      entry.reset();
      goto done;
   }

} // namespace vessel

} // namespace engine

