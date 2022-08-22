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

   Source File Name = btreeNodeBase.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#include "vessel/btreeNodeBase.h"
#include "ossLikely.hpp"
#include "pdTrace.hpp"
#include "vessel/memoryBlock.h"
#include "vessel/btreeContext.h"

namespace engine
{
namespace vessel
{
   constexpr UINT32 _ORDERED_W_FACTOR = 16;

   btreeNodeBase::btreeNodeBase(PAGE_ID nodeId,
                                UINT32 depth,
                                const strictBuffer &buffer):
   _nodeId(nodeId),
   _depth(depth),
   _buffer(buffer)
   {
      SDB_ASSERT(INVALID_PAGE_ID != _nodeId, "can not be invalid");
      SDB_ASSERT(buffer.isValid(), "can not be invalid");
   }

   BOOLEAN btreeNodeBase::isRoot()const
   {
      SDB_ASSERT(isValid(), "can not be invalid");
      return _getReadableHead()->isRoot();
   }

   BOOLEAN btreeNodeBase::isLeaf()const
   {
      SDB_ASSERT(isValid(), "can not be invalid");
      return _getReadableHead()->isLeaf();
   }

   UINT32 btreeNodeBase::getItemCount()const
   {
      SDB_ASSERT(isValid(), "can not be invalid");
      return _getReadableHead()->totalSlotCount;
   }

   UINT32 btreeNodeBase::getNodeSize()const
   {
      SDB_ASSERT(isValid(), "can not be invalid");
      return _buffer.getSize();
   }

   BOOLEAN btreeNodeBase::isItemMarkedAsDeleted(RECORD_SLOT_POS pos)const
   {
      SDB_ASSERT(isValid(), "can not be invalid"); 
      return _getReadableSlot(pos)->isMarkedDeleted();
   }

   PAGE_ID btreeNodeBase::getRightChild()const
   {
      SDB_ASSERT(!isLeaf(), "can not be leaf");
      return _getReadableHead()->rightChild;
   }

   BOOLEAN btreeNodeBase::hasRightChild()const
   {
      return INVALID_PAGE_ID != getRightChild();
   }

   BOOLEAN btreeNodeBase::isRightChildLeaf()const
   {
      SDB_ASSERT(!isLeaf(), "can not be leaf");
      return _getReadableHead()->isRightChildLeaf();
   }

   PAGE_ID btreeNodeBase::getLeftChild(RECORD_SLOT_POS pos)const
   {
      SDB_ASSERT(!isLeaf(), "can not be leaf");
      return _getReadableSlot(pos)->data.nlf.leftChild;
   }

   PAGE_ID btreeNodeBase::getChild(RECORD_SLOT_POS pos)const
   {
      SDB_ASSERT(!isLeaf(), "can not be leaf");
      UINT32 count = _getReadableHead()->totalSlotCount;
      SDB_ASSERT((UINT32)pos <= count, "out of bound");
      return (UINT32)pos < count ?
             _getReadableSlot(pos)->data.nlf.leftChild :
             _getReadableHead()->rightChild;
   }

   DPS_TRANS_ID btreeNodeBase::getTransID()const
   {
      SDB_ASSERT(isValid(), "can not be invalid"); 
      return _getReadableHead()->transID;
   }

   btreeItemSlot btreeNodeBase::getItemSlot(RECORD_SLOT_POS pos)const
   {
      SDB_ASSERT(isValid(), "can not be invalid"); 
      return *_getReadableSlot(pos);
   }

   void btreeNodeBase::dumpChildNodes(ossPoolVector<PAGE_ID> &nodes)const
   {
      SDB_ASSERT(!isLeaf(), "can not be leaf");
      nodes.clear();
      UINT16 count = _getReadableHead()->totalSlotCount;
      nodes.reserve(count);
      for (RECORD_SLOT_POS i = 0; (UINT16)i < count; ++i)
      {
         nodes.push_back(getChild(i));
      }
      return;
   }

   BOOLEAN btreeNodeBase::hasFreeSpaceToInsert(UINT32 itemSize,
                                               BOOLEAN *compaction)const
   {
      SDB_ASSERT(isValid(), "can not be invalid");
      UINT32 size = getSizeToSaveInNode(itemSize);
      return size <= _getReadableHead()->totalFreeSpace;
   }

   BOOLEAN btreeNodeBase::betterToActiveCompression()const
   {
      SDB_ASSERT(isValid(), "can not be invalid");
      const btreeNodePageHead *header = _getReadableHead();
      return header->isLeaf() &&
             hitHighWaterMark() &&
             0 != OSS_BIT_TEST(header->flags, BTREE_NODE_FLAG_VAIN_PREFIX_REGENERATION);
   }

   BOOLEAN btreeNodeBase::hitHighWaterMark()const
   {
      SDB_ASSERT(isValid(), "can not be invalid");
      const btreeNodePageHead *header = _getReadableHead();
      return ((FLOAT32)header->totalFreeSpace / getNodeSize()) <=
             (1.0 - BTREE_NODE_HIGH_WATER_MARK);
   }

   BOOLEAN btreeNodeBase::hasCompressedItems()const
   {
      SDB_ASSERT(isValid(), "can not be invalid");
      const btreeNodePageHead *header = _getReadableHead();
      return 0 < header->compressedItemCount;
   }

   UINT32 btreeNodeBase::getPrefixCount()const
   {
      SDB_ASSERT(isValid(), "can not be invalid");
      return _getReadableHead()->prefixCount;
   }

   INT32 btreeNodeBase::locateEntry(const btreeKeyStringEntry &entry,
                                    btreeNodeSeekResult &res) const
   {
      INT32 rc = SDB_OK;
      if (OSS_UNLIKELY(!isValid()))
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }
      else if (!entry.isValid())
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      else
      {
         rc = _locate(entry, res);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to locate entry:%d", rc);
            goto error;
         }
      }
   done:
      return rc;
   error:
      res.reset();
      goto done;
   }

   INT32 btreeNodeBase::seek(const keyString &ks,
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
      res.reset();
      goto done;
   }

   INT32 btreeNodeBase::seek(const keyString &ks,
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
      res.reset();
      goto done;
   }

   INT32 btreeNodeBase::isOutOfKeyBound(const keyString &ks,
                                        BOOLEAN forward,
                                        BOOLEAN &outOfBound) const
   {
      INT32 rc = SDB_OK;
      RECORD_SLOT_POS pos = INVALID_RECORD_SLOT_POS;
      INT32 direction = forward ? 1 : -1;
      _itemRef ref;
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

      ref = _getItemRef(pos);
      SDB_ASSERT(ref.isValid(), "can not be invalid");
      rc = entry.init(ref.data);
      if (OSS_UNLIKELY(SDB_OK != rc))
      {
         PD_LOG(PDERROR, "failed to init btree entry:%d", rc);
         goto error;
      }

      result = entry.getKeySlice().compare(ks.getKeySliceAfterHeader());
      outOfBound = ((result * direction) < 0);
   done:
      return rc;
   error:
      goto done;
   }

   INT32 btreeNodeBase::getItem(RECORD_SLOT_POS pos,
                                btreeNodeItem &item)const
   {
      INT32 rc = SDB_OK;
      _itemRef ref;
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

      ref = _getItemRef(pos);
      rc = entry.init(ref.data);
      if (OSS_UNLIKELY(SDB_OK != rc))
      {
         PD_LOG(PDERROR, "failed to parse entry[%d] data:%d", pos, rc);
         goto error;
      }

      item.init(getNodeId(), pos, *ref.slot, entry);
   done:
      return rc;
   error:
      goto done;
   }

   INT32 btreeNodeBase::remove(RECORD_SLOT_POS pos)
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
      else if (_getReadableHead()->totalSlotCount <= (UINT16)pos)
      {
         rc = SDB_OUT_OF_BOUND;
         goto error;
      }
      else if (isLeaf())
      {
         rc = _destroyItem(pos);
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

   done:
      return rc;
   error:
      goto done;
   }

   INT32 btreeNodeBase::insert(const btreeKeyStringEntry &entry,
                               const DPS_TRANS_ID &transID)
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
      else if (OSS_UNLIKELY(!isLeaf()))
      {
         rc = SDB_VESSEL_OPERATOION_NOT_PERMITTED;
         goto error;
      }

      rc = _leafInsert(entry, transID);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to insert entry into leaf:%d", rc);
         goto error;
      }
   done:
      return rc;
   error:
      goto done;
   }

   INT32 btreeNodeBase::splitAndInsert(const btreeKeyStringEntry &entry,
                                       const DPS_TRANS_ID &transID,
                                       btreeSplitRaisedKey &raisedKey)
   {
      INT32 rc = SDB_OK;
      RECORD_SLOT_POS pivot = INVALID_RECORD_SLOT_POS;
      BOOLEAN idleRight = FALSE;
      btreeNodeSeekResult res;
      BTREE_NODE_UPTR rightNode;

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
         PD_LOG(PDERROR, "can not split non-leaf node and insert entry");
         rc = SDB_VESSEL_OPERATOION_NOT_PERMITTED;
         goto error;
      }
      else if (nullptr == _getTreeCtx())
      {
         PD_LOG(PDERROR, "tree context is invalid");
         rc = SDB_VESSEL_OPERATOION_NOT_PERMITTED;
         goto error;
      }

      rc = _locate(entry, res);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to locate key and rid:%d", rc);
         goto error;
      }

      if (res.isUpperBound && _isRecentWriteOrdered())
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

      rc = _split(pivot, rightNode);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to split node:%d", rc);
         goto error;
      }

      /// do not goto error from here
      raisedKey.leftChild = getNodeId();
      raisedKey.rightChild = rightNode->getNodeId();
      raisedKey.fromLeaf = TRUE;
      raisedKey.transID = _getReadableHead()->transID;

      if (!idleRight)
      {
         rc = _compact();
         if (SDB_OK != rc)
         {
            PD_LOG(PDSEVERE, "failed to compact node:%d", rc);
            ossPanic();
            goto error;
         }
      }

      if (res.slotPos <= pivot)
      {
         rc = _leafInsert(entry, transID, res.slotPos);
         PD_LOG(PDSEVERE, "failed to insert key after node[%d] split:%d",
                getNodeId(), rc);
         ossPanic();
         goto error;
      }
      else
      {
         rc = rightNode->_leafInsert(entry, transID, res.slotPos - pivot - 1);
         if (SDB_OK != rc)
         {
            PD_LOG(PDSEVERE, "failed to insert key into right node[%d]:%d",
                   rightNode->getNodeId(), rc);
            ossPanic();
            goto error;
         }
      }

   done:
      return rc;
   error:
      raisedKey.reset();
      /// always panic if right node created.
      /// do not need to rollback rightNode.
      goto done;
   }

   INT32 btreeNodeBase::recompress(BOOLEAN &recompressed)
   {
      SDB_ASSERT(FALSE, "TODO");
      return SDB_OK;
   }

   INT32 btreeNodeBase::insertRaisedKey(const btreeSplitRaisedKey &raisedKey)
   {
      INT32 rc = SDB_OK;
      rc = _insertRaisedKey(raisedKey);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to insert raised key:%d", rc);
         goto error;
      }

   done:
      return rc;
   error:
      goto done;
   }

   INT32 btreeNodeBase::_insertRaisedKey(const btreeSplitRaisedKey &raisedKey,
                                         RECORD_SLOT_POS pos)
   {
      INT32 rc = SDB_OK;
      const btreeNodePageHead *head = nullptr;
      BOOLEAN appendOnly = FALSE;
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
      
      head = _getReadableHead();

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
         rc = _locate(raisedKey.entry, res);
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
      head = nullptr;

      if (needCompact)
      {
         rc = _compact();
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to compact node:%d", rc);
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
         btreeNodePageHead *h = _getWritableHead();
         SDB_ASSERT(raisedKey.leftChild == h->rightChild ||
                    INVALID_PAGE_ID == h->rightChild, "must be same");
         h->rightChild = raisedKey.rightChild;
         if (raisedKey.fromLeaf)
         {
            OSS_BIT_SET(h->flags, BTREE_NODE_FLAG_RIGHT_CHILD_IS_LEAF);
         }
         else
         {
            OSS_BIT_CLEAR(h->flags, BTREE_NODE_FLAG_RIGHT_CHILD_IS_LEAF);
         }
      }
      else
      {
         btreeItemSlot *slot = _getWritableSlot(insertPos + 1);
         slot->data.nlf.leftChild = raisedKey.rightChild;
         if (raisedKey.fromLeaf)
         {
            slot->setRaisedFromLeaf();
         }
         else
         {
            slot->clearRaisedFromLeaf();
         }
      }

      _updateTransID(raisedKey.transID);
   done:
      return rc;
   error:
      goto done;
   }

   INT32 btreeNodeBase::removeChild(RECORD_SLOT_POS pos)
   {
      INT32 rc = SDB_OK;
      btreeNodePageHead *head = nullptr;

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

      if (getItemCount() < (UINT16)pos)
      {
         PD_LOG(PDERROR, "pos[%d] to remove is out of total slot count[%d]",
                pos, getItemCount());
         rc = SDB_OUT_OF_BOUND;
         goto error;
      }
      else if (INVALID_PAGE_ID == getChild(pos))
      {
         PD_LOG(PDERROR, "already has no child in pos[%d]", pos);
         rc = SDB_VESSEL_OPERATOION_NOT_PERMITTED;
         goto error;
      }

      rc = _makeBufferWritable();
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to get node ready to write:%d", rc);
         goto error;
      }

      head = _getWritableHead();
      if (pos == head->totalSlotCount)
      {
         head->rightChild = INVALID_PAGE_ID;
         OSS_BIT_CLEAR(head->flags, BTREE_NODE_FLAG_RIGHT_CHILD_IS_LEAF);
      }
      else
      {
         btreeItemSlot *slot = _getWritableSlot(pos);
         slot->data.nlf.leftChild = INVALID_PAGE_ID;
         slot->clearRaisedFromLeaf();

         /// non-leaf node must have one item at least
         if (slot->isMarkedDeleted() && 1 < head->totalSlotCount)
         {
            rc = _destroyItem(pos);
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

   INT32 btreeNodeBase::reactiveRemovedKey(RECORD_SLOT_POS pos,
                                           const DPS_TRANS_ID &transID)
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
      else if (OSS_UNLIKELY(isLeaf()))
      {
         rc = SDB_VESSEL_OPERATOION_NOT_PERMITTED;
         goto error;
      }
      else if (getItemCount() <= (UINT16)pos)
      {
         rc = SDB_OUT_OF_BOUND;
         goto error;
      }
      else if (!_getReadableSlot(pos)->isMarkedDeleted())
      {
         PD_LOG(PDERROR, "item[%d] is not marked as removed", pos);
         rc = SDB_VESSEL_OPERATOION_NOT_PERMITTED;
         goto error;
      }
      else
      {
         rc = _makeBufferWritable();
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to make buffer writable:%d", rc);
            goto error;
         }

         btreeItemSlot *slot = _getWritableSlot(pos);
         OSS_BIT_CLEAR(slot->flags, btreeItemSlot::FLAG_MARKED_DELETED);
         _updateTransID(transID);
      }

   done:
      return rc;
   error:
      goto done;
   }

   INT32 btreeNodeBase::refillChild(RECORD_SLOT_POS pos,
                                    PAGE_ID child,
                                    BOOLEAN childIsLeaf)
   {
      INT32 rc = SDB_OK;
      if (OSS_UNLIKELY(!isValidRecordSlotPosition(pos) ||
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
      else if (getItemCount() < (UINT16)pos)
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

      rc = _makeBufferWritable();
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to get node ready to write:%d", rc);
         goto error;
      }

      if ((UINT16)pos < getItemCount())
      {
         btreeItemSlot *slot = _getWritableSlot(pos);
         slot->data.nlf.leftChild = child;
         if (childIsLeaf)
         {
            slot->setRaisedFromLeaf();
         }
      }
      else
      {
         btreeNodePageHead *head = _getWritableHead();
         head->rightChild = child;
         if (childIsLeaf)
         {
            OSS_BIT_SET(head->flags, BTREE_NODE_FLAG_RIGHT_CHILD_IS_LEAF);
         }
      }

   done:
      return rc;
   error:
      goto done;
   }

   INT32 btreeNodeBase::splitAndInsert(const btreeSplitRaisedKey &raisedKey,
                                       btreeSplitRaisedKey &newRaisedKey)
   {
      INT32 rc = SDB_OK;
      btreeNodeSeekResult res;
      RECORD_SLOT_POS pivot = INVALID_RECORD_SLOT_POS;
      BOOLEAN idleRight = FALSE;
      BTREE_NODE_UPTR rightNode;
      newRaisedKey.reset();

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
      else if (OSS_UNLIKELY(!raisedKey.isValid()))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      rc = _locate(raisedKey.entry, res);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to locate key and rid:%d", rc);
         goto error;
      }

      if (res.slotPos == _getReadableHead()->totalSlotCount &&
          _isRecentWriteOrdered())
      {
         idleRight = TRUE;
      }

      rc = _findSplitPivot(idleRight, pivot);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to find split pivot:%d", rc);
         goto error;
      }

      rc = _saveRaisingEntry(pivot, newRaisedKey.entry);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to save raising entry:%d", rc);
         goto error;
      }

      rc = _split(pivot, rightNode);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to split node:%d", rc);
         goto error;
      }

      /// do not goto error from here
      newRaisedKey.leftChild = getNodeId();
      newRaisedKey.rightChild = rightNode->getNodeId();
      newRaisedKey.fromLeaf = FALSE;
      newRaisedKey.transID = _getReadableHead()->transID;

      /// non-leaf node will never be compressed,
      /// do not compact it.

      /// should not get error from here
      if (res.slotPos <= pivot)
      {
         rc = _insertRaisedKey(raisedKey, res.slotPos);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to insert raised key to current node:%d", rc);
            ossPanic();
            goto error;
         }
      }
      else
      {
         rc = rightNode->_insertRaisedKey(raisedKey, res.slotPos - pivot - 1);
         if (SDB_OK != rc)
         {
            PD_LOG(PDSEVERE, "failed to insert raised key into right node[%d]:%d",
                   rightNode->getNodeId(), rc);
            ossPanic();
            goto error;
         }
      }
   done:
      return rc;
   error:
      newRaisedKey.reset();
      goto done;
   }

   INT32 btreeNodeBase::_leafInsert(const btreeKeyStringEntry &entry,
                                    const DPS_TRANS_ID &transID,
                                    RECORD_SLOT_POS pos)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(isValid(), "can not be invalid");
      SDB_ASSERT(entry.isValid(), "can not be invalid");
      SDB_ASSERT(isLeaf(), "must be leaf");

      BOOLEAN needCompact = FALSE;
      UINT32 keySize = entry.getRawDataSize();
      btreeNodeSeekResult res;
      RECORD_SLOT_POS toInsert = INVALID_RECORD_SLOT_POS;

      if (!hasFreeSpaceToInsert(keySize, &needCompact))
      {
         rc = SDB_VESSEL_NOT_ENOUGH_SPACE_IN_PAGE;
         goto error;
      }

      if (isValidRecordSlotPosition(pos))
      {
         if (_getReadableHead()->totalSlotCount < pos)
         {
            PD_LOG(PDERROR, "insert pos[%d] out of bound", pos);
            rc = SDB_OUT_OF_BOUND;
            goto error;
         }

         toInsert = pos;
      }
      else if (0 < _getReadableHead()->totalSlotCount)
      {
         rc = _locate(entry, res);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to locate key and rid:%d", rc);
            goto error;
         }
         SDB_ASSERT(!res.isIdentical(), "impossible");
         toInsert = res.slotPos;
      }
      else
      {
         toInsert = 0;
      }

      if (needCompact)
      {
         rc = _compact();
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

      _updateTransID(transID);

   done:
      return rc;
   error:
      goto done;
   }

   INT32 btreeNodeBase::_compact()
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(isValid(), "can not be invalid");
      SDB_ASSERT(0 < getPrefixCount(), "TODO");
      memoryBlock mb;
      strictBuffer compactionBuffer;
      UINT32 frontOffset = BTREE_NODE_PAGE_HEAD_SIZE;
      UINT32 backOffset = getNodeSize();
      btreeNodePageHead *head = nullptr;
      UINT32 itemCount = 0;

      if (_getContinuousFreeSpace() == 
          _getReadableHead()->totalFreeSpace)
      {
         goto done;
      }

      rc = mb.reserve(getNodeSize());
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to reserve memory block:%d", rc);
         goto error;
      }
      compactionBuffer.makeWritable(mb.getCapacity(), mb.getBuffer());

      rc = _makeBufferWritable();
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to prepare to write:%d", rc);
         goto error;
      }

      head = compactionBuffer.getWritableObjPtr<btreeNodePageHead>(0);
      ossMemcpy(head, _getReadableHead(), BTREE_NODE_PAGE_HEAD_SIZE);
      itemCount = head->totalSlotCount;

      for (RECORD_SLOT_POS i = 0; (UINT32)i < itemCount; ++i)
      {
         _itemRef ref = _getItemRef(i);
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
               newSlot.initAsLeafFormat(backOffset, ref.data.size());
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

      _buffer.write(0, compactionBuffer.getSize(), compactionBuffer.getRPtr());
   done:
      return rc;
   error:
      goto done;
   }

   INT32 btreeNodeBase::_insert(RECORD_SLOT_POS pos,
                                const btreeKeyStringEntry &entry,
                                PAGE_ID leftChild)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(isValid(), "can not be invalid");
      SDB_ASSERT(INVALID_RECORD_SLOT_POS != pos, "can not be invalid");
      SDB_ASSERT(entry.isValid(), "can not be invalid");

      btreeItemSlot *slot = nullptr;
      UINT32 keyOffset = 0;
      UINT32 size = getSizeToSaveInNode(entry.getRawDataSize());
      btreeNodePageHead *head = nullptr;
      BOOLEAN appendonly = FALSE;

      rc = _makeBufferWritable();
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to get writable slice:%d", rc);
         goto error;
      }

      head = _buffer.getWritableObjPtr<btreeNodePageHead>(0);
      /// should be prechecked outside
      SDB_ASSERT((size + _getFrontOffset()) <= _getBackOffset(), "out of bound");
      SDB_ASSERT(pos <= head->totalSlotCount, "impossible");
      appendonly = (pos == head->totalSlotCount);

      keyOffset = _getKeyDataOffsetToWrite(head, entry.getRawDataSize());
      rc = _buffer.write(keyOffset,
                         entry.getRawDataSize(),
                         entry.getRawDataPtr());
      if (OSS_UNLIKELY(SDB_OK != rc))
      {
         PD_LOG(PDERROR, "failed to write key data to[%d,%d], rc:%d",
                keyOffset, entry.getRawDataSize(), rc);
         goto error;
      }

      slot = _getWritableSlot(pos);
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
         slot->initAsLeafFormat(keyOffset, entry.getRawDataSize());
      }
      else
      {
         SDB_ASSERT(INVALID_PAGE_ID != leftChild, "can not be invalid");
         slot->initAsNonLeafFormat(keyOffset, entry.getRawDataSize(), leftChild);
      }

      _updateAppendingFactor(head, appendonly);
      ++head->totalSlotCount;
      head->totalFreeSpace -= size;
      head->backOffset = keyOffset;
   done:
      return rc;
   error:
      goto done;
   }

   void btreeNodeBase::_updateTransID(const DPS_TRANS_ID &transID)
   {
      SDB_ASSERT(isWritable(), "must be writable");
      if (transID.isValid())
      {
         btreeNodePageHead *head = _buffer.getWritableObjPtr<btreeNodePageHead>(0);
         if (!head->transID.isValid() ||
             head->transID < transID)
         {
            head->transID = transID;
         }
      }
      return;
   }
   
   INT32 btreeNodeBase::_destroyItem(RECORD_SLOT_POS pos)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(isValidRecordSlotPosition(pos), "can not be invalid");
      SDB_ASSERT(isValid(), "can not be invalid");
      SDB_ASSERT((UINT16)pos < _getReadableHead()->totalSlotCount, "out of bound");

      btreeNodePageHead *head = nullptr;
      btreeItemSlot *slot = nullptr;
      UINT32 size = BTREE_NODE_SLOT_SIZE;

      rc = _makeBufferWritable();
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to get node ready to write:%d", rc);
         goto error;
      }

      head = _buffer.getWritableObjPtr<btreeNodePageHead>(0);
      if (OSS_UNLIKELY(nullptr == head))
      {
         PD_LOG(PDERROR, "failed to get page head");
         rc = SDB_VESSEL_INTERNAL_ERR;
         goto error;
      }

      slot = _getWritableSlot(pos);
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

   INT32 btreeNodeBase::_nonleafRemove(RECORD_SLOT_POS pos)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(isValidRecordSlotPosition(pos), "can not be invalid");
      SDB_ASSERT(isValid(), "can not be invalid");
      SDB_ASSERT((UINT16)pos < _getReadableHead()->totalSlotCount, "out of bound");
      SDB_ASSERT(!isLeaf(), "can not be leaf");

      const btreeItemSlot *rs = _getReadableSlot(pos);

      if (rs->isMarkedDeleted())
      {
         PD_LOG(PDERROR, "item with pos[%d] has already been removed", pos);
         rc = SDB_VESSEL_IXM_ITEM_NOT_FOUND;
         goto error;
      }
      else if (INVALID_PAGE_ID != rs->data.nlf.leftChild ||
               1 == _getReadableHead()->totalSlotCount)
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
         rc = _destroyItem(pos);
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

   INT32 btreeNodeBase::_markRemoved(RECORD_SLOT_POS pos)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(isValidRecordSlotPosition(pos), "can not be invalid");
      SDB_ASSERT(isValid(), "can not be invalid");
      SDB_ASSERT((UINT16)pos < _getReadableHead()->totalSlotCount, "out of bound");
      SDB_ASSERT(!isLeaf(), "can not be leaf");

      btreeItemSlot *slot = nullptr;
      rc = _makeBufferWritable();
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to get buffer ready to write:%d", rc);
         goto error;
      }

      slot = _getWritableSlot(pos);
      SDB_ASSERT(!slot->isMarkedDeleted(), "already been removed");
      slot->markDeleted();
   done:
      return rc;
   error:
      goto done;
   }

   INT32 btreeNodeBase::_seek(const keyString &ks,
                              RECORD_SLOT_POS pos,
                              BOOLEAN forward,
                              btreeNodeSeekResult &res) const
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(ks.isValid(), "can not be invalid");
      SDB_ASSERT(isValidRecordSlotPosition(pos), "can not be invalid");

      const btreeNodePageHead *head = _getReadableHead();
      SDB_ASSERT(nullptr != head, "can not be invalid");

      INT32 cmp = 0;
      INT16 count = 0;
      RECORD_SLOT_POS low = forward ? pos : 0;
      RECORD_SLOT_POS high = forward ? head->totalSlotCount : pos + 1;
      slice keySlice = ks.getKeySliceAfterHeader();
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
         _itemRef ref = _getItemRef(pos);
         SDB_ASSERT(ref.isValid(), "can not be invalid");
         SDB_ASSERT(!ref.slot->isKeyCompressed(), "TODO");
         keyString entry(ref.data);
         if (OSS_UNLIKELY(!entry.isValid()))
         {
            PD_LOG(PDERROR, "index entry may be crashed[%d, %d]",
                   getNodeId(), pos);
            SDB_ASSERT(FALSE, "entry crashed");
            rc = SDB_VESSEL_PAGE_CRASHED;
            goto error;
         }

         cmp = keySlice.compare(entry.getKeySlice());
         if (0 < cmp)
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
                     _getReadableSlot(low)->data.nlf.leftChild;
      }
   done:
      return rc;
   error:
      goto done;
   }

   INT32 btreeNodeBase::_locate(const btreeKeyStringEntry &ks,
                                btreeNodeSeekResult &res) const
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(isValid(), "can not be invalid");
      SDB_ASSERT(ks.isValid(), "can not be invalid");

      const btreeNodePageHead *head = _getReadableHead();
      SDB_ASSERT(nullptr != head, "can not be invalid");
      INT16 count = head->totalSlotCount;
      SDB_ASSERT(0 < count, "can not be invalid");
      RECORD_SLOT_POS low = 0;
      slice target = ks.getRawData();
      INT32 cmp = 0;

      res.reset();
      while (0 < count)
      {
         INT16 step = count >> 1;
         RECORD_SLOT_POS pos = low + step;
         _itemRef ref = _getItemRef(pos);
         SDB_ASSERT(ref.isValid(), "can not be invalid");
         SDB_ASSERT(!ref.slot->isKeyCompressed(), "TODO");
         cmp = keyString::compareCoding(target.getSize(),
                                        target.getData(),
                                        ref.data.getSize(),
                                        ref.data.getData());
         if (0 < cmp)
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
                     _getReadableSlot(low)->data.nlf.leftChild;
      }
   done:
      return rc;
   error:
      goto done;
   }

   const btreeItemSlot *btreeNodeBase::_getReadableSlot(RECORD_SLOT_POS pos)const
   {
      SDB_ASSERT(isValidRecordSlotPosition(pos) &&
                 (UINT16)pos < _getReadableHead()->totalSlotCount, "invalid pos");
      UINT32 offset = BTREE_NODE_PAGE_HEAD_SIZE +
                      (_getReadableHead()->prefixCount * BTREE_NODE_PREFIX_SLOT_SIZE) +
                      (pos * BTREE_NODE_SLOT_SIZE);
      return _buffer.getReadableObjPtr<btreeItemSlot>(offset);
   }

   btreeItemSlot *btreeNodeBase::_getWritableSlot(RECORD_SLOT_POS pos)
   {
      SDB_ASSERT(isValidRecordSlotPosition(pos) &&
                 (UINT16)pos < _getReadableHead()->totalSlotCount, "invalid pos");
      UINT32 offset = BTREE_NODE_PAGE_HEAD_SIZE +
                      (_getReadableHead()->prefixCount * BTREE_NODE_PREFIX_SLOT_SIZE) + 
                      (pos * BTREE_NODE_SLOT_SIZE);
      return _buffer.getWritableObjPtr<btreeItemSlot>(offset);
   }

   btreeNodeBase::_itemRef btreeNodeBase::_getItemRef(RECORD_SLOT_POS pos) const
   {
      const btreeItemSlot *slot = _getReadableSlot(pos);
      if (OSS_LIKELY(nullptr != slot))
      {
         return _itemRef(slot, _buffer.getSlice(slot->data.key.offset,
                                                slot->data.key.size));
      }
      else
      {
         return _itemRef();
      }    
   }

   UINT32 btreeNodeBase::_getContinuousFreeSpace()const
   {
      return _getBackOffset() - _getFrontOffset();
   }

   UINT32 btreeNodeBase::_getFrontOffset()const
   {
      return BTREE_NODE_PAGE_HEAD_SIZE +
             (_getReadableHead()->totalSlotCount * BTREE_NODE_SLOT_SIZE);
   }
   
   UINT32 btreeNodeBase::_getBackOffset()const
   {
      return _getReadableHead()->backOffset;     
   }

   UINT32 btreeNodeBase::_getKeyDataOffsetToWrite(const btreeNodePageHead *head,
                                                  UINT32 keyDataSize)const
   {
      SDB_ASSERT(0 < keyDataSize, "can not be invalid");
      SDB_ASSERT(keyDataSize < head->backOffset, "out of bound");
      return head->backOffset - keyDataSize;
   }

   BOOLEAN btreeNodeBase::_isRecentWriteOrdered()const
   {
      return _ORDERED_W_FACTOR == _getReadableHead()->appendingFactor;
   }

   void btreeNodeBase::_updateAppendingFactor(btreeNodePageHead *head,
                                              BOOLEAN stillAppendOnly)
   {
      SDB_ASSERT(nullptr != head, "can not be invalid");
      if (stillAppendOnly)
      {
         if (head->appendingFactor < _ORDERED_W_FACTOR)
         {
            ++head->appendingFactor;
         }
      }
      else
      {
         head->appendingFactor = 0;
      }
      return;
   }

   INT32 btreeNodeBase::_findSplitPivot(BOOLEAN idleRight,
                                        RECORD_SLOT_POS &pivot)const
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(isValid(), "can not be invalid");
      SDB_ASSERT(!hasPrefixes(), "TODO");
      UINT32 splitSize = 0;
      UINT32 scanned = 0;
      const btreeNodePageHead *head = nullptr;
      static const UINT32 _MIN_SPLIT_ITEM_COUNT = 3;
      UINT32 factor = idleRight ? 10 : 2;

      pivot = INVALID_RECORD_SLOT_POS;
      head = _getReadableHead();
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
         const btreeItemSlot *slot = _getReadableSlot(i);
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

   INT32 btreeNodeBase::_saveRaisingEntry(RECORD_SLOT_POS pos,
                                          btreeKeyStringEntry &entry) const
   {
      INT32 rc = SDB_OK;
      _itemRef ref = _getItemRef(pos);
      SDB_ASSERT(ref.isValid(), "can not be invalid");

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

   INT32 btreeNodeBase::_split(RECORD_SLOT_POS pivot,
                               std::unique_ptr<btreeNodeBase> &rightNode)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(isValid(), "can not be invalid");
      SDB_ASSERT(isValidRecordSlotPosition(pivot), "can not be invalid");
      SDB_ASSERT(0 < pivot, "can not be zero");

      PAGE_ID pivotLeftChild = INVALID_PAGE_ID;
      BOOLEAN newRightChildIsLeaf = FALSE;

      rc = _allocateRightNode(rightNode);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to allocate right node:%d", rc);
         goto error;
      }

      rc = _buildRightNode(pivot + 1, *rightNode);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to build right node:%d", rc);
         goto error;
      }

      if (!isLeaf())
      {
         pivotLeftChild = _getReadableSlot(pivot)->data.nlf.leftChild;
         newRightChildIsLeaf = _getReadableSlot(pivot)->isRaisedFromLeaf();
      }

      rc = _truncate(pivot);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to truncate node:%d", rc);
         goto error;
      }

      /// do not goto error from here
      {
         btreeNodePageHead *head = _getWritableHead();
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
      }
   done:
      return rc;
   error:
      goto done;
   }

   INT32 btreeNodeBase::_allocateRightNode(std::unique_ptr<btreeNodeBase> &rightNode)
   {
      INT32 rc = SDB_OK;
      btreeContext *bc = _getTreeCtx();
      SDB_ASSERT(nullptr != bc, "can not be invalid");
      btreeNodePageHead header;
      header.initAsRightNode(*_getReadableHead(), getNodeSize());
      rc = bc->allocateNewNode(_depth, header, rightNode);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to allocate right node:%d", rc);
         goto error;
      }

   done:
      return rc;
   error:
      rightNode.reset();
      goto done;
   }

   INT32 btreeNodeBase::_buildRightNode(RECORD_SLOT_POS pos,
                                        btreeNodeBase &node)const
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(isValid(), "can not be invalid");
      SDB_ASSERT(isValidRecordSlotPosition(pos), "can not be invalid");

      btreeNodePageHead *rightHead = node._getWritableHead();
      SDB_ASSERT(nullptr != rightHead, "can not be invalid");
      strictBuffer &buffer = node._buffer;
      const btreeNodePageHead *head = _getReadableHead();
      SDB_ASSERT((UINT16)pos < head->totalSlotCount, "invalid pos");
      UINT32 frontOffset = BTREE_NODE_PAGE_HEAD_SIZE;

      for (RECORD_SLOT_POS i = pos; (UINT16)i < head->totalSlotCount; ++i)
      {
         btreeItemSlot *slot = nullptr;
         UINT32 keyOffset = 0;
         _itemRef ref = _getItemRef(i);
         SDB_ASSERT(ref.isValid(), "impossible");
         SDB_ASSERT(!ref.slot->isKeyCompressed(), "TODO");

         if (rightHead->backOffset <
             (frontOffset + getSizeToSaveInNode(ref.data.size())))
         {
            PD_LOG(PDERROR, "not enough free space to save item");
            rc = SDB_VESSEL_NOT_ENOUGH_SPACE_IN_PAGE;
            goto error;
         }

         keyOffset = _getKeyDataOffsetToWrite(rightHead, ref.data.size());
         rc = buffer.write(keyOffset, ref.data.size(), ref.data.data());
         if (OSS_UNLIKELY(SDB_OK != rc))
         {
            PD_LOG(PDERROR, "failed to write slice[%d,%d], rc:%d",
                   keyOffset, ref.data.size(), rc);
            goto error;
         }

         slot = buffer.getWritableObjPtr<btreeItemSlot>(frontOffset);
         if (isLeaf())
         {
            slot->initAsLeafFormat(keyOffset, ref.data.size());
         }
         else
         {
            slot->initAsNonLeafFormat(keyOffset,
                                      ref.data.size(),
                                      ref.slot->data.nlf.leftChild);
         }
         rightHead->totalFreeSpace -= BTREE_NODE_SLOT_SIZE;
         rightHead->totalFreeSpace -= ref.data.size();
         rightHead->backOffset = keyOffset;
         ++rightHead->totalSlotCount;
         frontOffset += BTREE_NODE_SLOT_SIZE;
      }//for (RECORD_SLOT_POS i = begin; i < head->totalSlotCount; ++i)
   done:
      return rc;
   error:
      goto done;
   }

   INT32 btreeNodeBase::_truncate(UINT32 keptItemNum)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(isValid(), "can not be invalid");

      if ((UINT32)getItemCount() <= keptItemNum)
      {
         goto done;
      }

      rc = _makeBufferWritable();
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to make buffer writable:%d", rc);
         goto error;
      }
      else
      {
         UINT32 size = 0;
         RECORD_SLOT_POS pos = (RECORD_SLOT_POS)keptItemNum;
         btreeNodePageHead *head = _getWritableHead();
         for (RECORD_SLOT_POS i = head->totalSlotCount - 1; i >= pos; --i)
         {
            const btreeItemSlot *slot = _getReadableSlot(i);
            size += BTREE_NODE_SLOT_SIZE;
            size += slot->data.key.size;
            if(slot->isKeyCompressed())
            {
               btreeNodePrefixSlot *prefixSlot =
                  _getWritablePrefixSlot(slot->data.lf.prefixSlot);
               prefixSlot->high = i;
               --head->compressedItemCount;
            }
         }

         head->totalFreeSpace += size;
         head->totalSlotCount = keptItemNum;
      }

   done:
      return rc;
   error:
      goto done;
   }

   btreeNodePrefixSlot *btreeNodeBase::_getWritablePrefixSlot(INT16 pos)
   {
      SDB_ASSERT(0 <= pos, "can not be invalid");
      UINT32 offset = BTREE_NODE_PAGE_HEAD_SIZE + (pos * BTREE_NODE_PREFIX_SLOT_SIZE);
      return _buffer.getWritableObjPtr<btreeNodePrefixSlot>(offset);
   }

   const btreeNodePrefixSlot *btreeNodeBase::_getReadablePrefixSlot(INT16 pos)const
   {
      SDB_ASSERT(0 <= pos, "can not be invalid");
      UINT32 offset = BTREE_NODE_PAGE_HEAD_SIZE + (pos * BTREE_NODE_PREFIX_SLOT_SIZE);
      return _buffer.getReadableObjPtr<btreeNodePrefixSlot>(offset);
   }

} // namespace vessel

} // namespace engine
