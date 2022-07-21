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

namespace engine
{
namespace vessel
{
   btreeNode::btreeNode(UINT32 depth,
                        logicalPageBuffer *buffer,
                        btreeAccessContext *ctx):
   _depth(depth),
   _buffer(buffer),
   _ctx(ctx)
   {
      SDB_ASSERT(isValid(), "can not be invalid");
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

   INT32 btreeNode::getBirthNodeLevel()const
   {
      return getReadableHead()->birthNodeLevel;
   }

   BOOLEAN btreeNode::hasFreeSpaceToInsert(UINT32 keySize,
                                           BOOLEAN *needCompact)const
   {
      const btreeNodePageHead *head = getReadableHead();
      UINT32 size = getSizeToSaveInNode(keySize);
      BOOLEAN r = (size <= head->totalFreeSpace);
      if (r && NULL != needCompact)
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
      const btreeNodePageHead *head = getReadableHead();
      return DPS_TRANS_ID(head->transSN, head->transNode);
   }

   btreeItemSlot btreeNode::getItemSlot(RECORD_SLOT_POS  pos)const
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

   btreeItemSlot *btreeNode::getWritableSlot(RECORD_SLOT_POS  pos)
   {
      SDB_ASSERT(INVALID_RECORD_SLOT_POS  != pos, "can not be invalid");
      SDB_ASSERT(isValid(), "can not be invalid");
      SDB_ASSERT(_buffer->isWritable(), "must be prepared");
      UINT32 offset = BTREE_NODE_PAGE_HEAD_SIZE +
                      (BTREE_NODE_PREFIX_SLOT_SIZE * getReadableHead()->prefixCount) +
                      (BTREE_NODE_SLOT_SIZE * pos);
      return _buffer->getWritableBodyBuffer().
             getWritableObjPtr<btreeItemSlot>(offset);
   }

   const btreeItemSlot *btreeNode::getReadableSlot(RECORD_SLOT_POS  pos)const
   {
      SDB_ASSERT(INVALID_RECORD_SLOT_POS  != pos, "can not be invalid");
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

   ossSharedLatchMode btreeNode::getLockingMode()const
   {
      SDB_ASSERT(isValid(), "can not be invalid");
      return _buffer->getLockingMode();
   }

   BOOLEAN btreeNode::ensureExclusiveLocking()
   {
      SDB_ASSERT(isValid(), "can not be invalid");
      BOOLEAN r = FALSE;
      if (_buffer->getLockingMode().isExclusive())
      {
         r = TRUE;
      }
      else if (_buffer->getLockingMode().isUpgrade())
      {
         r = _buffer->tryLockExclusiveFromUpgrade();
      }
      else
      {
         r = _buffer->tryLockExclusiveFromShared();
      }
      return r;
   }

   BOOLEAN btreeNode::isItemMarkedAsDeleted(RECORD_SLOT_POS  pos)const
   {
      SDB_ASSERT(isValid(), "can not be invalid");
      SDB_ASSERT(INVALID_RECORD_SLOT_POS  != pos, "can not be invalid");
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

   PAGE_ID btreeNode::getLeftChild(RECORD_SLOT_POS pos)const
   {
      SDB_ASSERT(isValid(), "can not be invalid");
      SDB_ASSERT(INVALID_RECORD_SLOT_POS  != pos, "can not be invalid");
      SDB_ASSERT(!isLeaf(), "can not be leaf");
      SDB_ASSERT(pos < getReadableHead()->totalSlotCount, "out of bound");
      return getReadableSlot(pos)->data.nlf.leftChild;
   }

   PAGE_ID btreeNode::getChild(RECORD_SLOT_POS pos)const
   {
      SDB_ASSERT(isValid(), "can not be invalid");
      SDB_ASSERT(INVALID_RECORD_SLOT_POS  != pos, "can not be invalid");
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

   INT32 btreeNode::leafInsert(const ixmKey &key,
                               const recordID &rid)
   {
      INT32 rc = SDB_OK;

      if (OSS_UNLIKELY(!key.isValid() ||
                       !rid.isValid()))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      else if (OSS_UNLIKELY(!isValid()))
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }
      else if (!_buffer->getLockingMode().isExclusive())
      {
         rc = SDB_VESSEL_FORBIDDEN_OP_WLT;
         goto error;
      }

      rc = _leafInsert(key, rid);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to insert into leaf:%d", rc);
         goto error;
      }

      commit();
   done:
      return rc;
   error:
      goto done;
   }

   INT32 btreeNode::leafRemove(RECORD_SLOT_POS  pos)
   {
      INT32 rc = SDB_OK;
      if (OSS_UNLIKELY(INVALID_RECORD_SLOT_POS  == pos))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      else if (OSS_UNLIKELY(!isValid()))
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }
      else if (!_buffer->getLockingMode().isExclusive())
      {
         rc = SDB_VESSEL_FORBIDDEN_OP_WLT;
         goto error;
      }
      else if (getReadableHead()->totalSlotCount <= pos)
      {
         rc = SDB_OUT_OF_BOUND;
         goto error;
      }
      else if (!isLeaf() || becameEmptyAfterRemoving(pos))
      {
         rc = SDB_VESSEL_OPERATOION_NOT_PERMITTED;
         goto error;
      }

      rc = _destroySlot(pos);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to destroy item[%d], rc:%d", rc);
         goto error;
      }

      commit();
   done:
      return rc;
   error:
      goto done;
   }

   INT32 btreeNode::nonleafRemove(RECORD_SLOT_POS  pos)
   {
      INT32 rc = SDB_OK;
      if (OSS_UNLIKELY(INVALID_RECORD_SLOT_POS  == pos))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      else if (OSS_UNLIKELY(!isValid()))
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }
      else if (!_buffer->getLockingMode().isExclusive())
      {
         rc = SDB_VESSEL_FORBIDDEN_OP_WLT;
         goto error;
      }
      else if (getReadableHead()->totalSlotCount <= pos)
      {
         rc = SDB_OUT_OF_BOUND;
         goto error;
      }
      else if (isLeaf() || becameEmptyAfterRemoving(pos))
      {
         rc = SDB_VESSEL_OPERATOION_NOT_PERMITTED;
         goto error;
      }

      rc = _nonleafRemove(pos);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to remove item[%d] in node:%d", pos, rc);
         goto error;
      }

      commit();
   done:
      return rc;
   error:
      goto done;
   }


   INT32 btreeNode::_leafInsert(const ixmKey &key,
                                const recordID &rid,
                                RECORD_SLOT_POS  pos)
   { 
      INT32 rc = SDB_OK;
      SDB_ASSERT(isValid(), "can not be invalid");
      SDB_ASSERT(key.isValid() && rid.isValid(), "can not be invalid");
      SDB_ASSERT(_buffer->getLockingMode().isExclusive(), "must be exclusive");
      SDB_ASSERT(isLeaf(), "must be leaf");

      BOOLEAN needCompact = FALSE;
      UINT32 keySize = key.dataSize();
      UINT32 savingSize = getSizeToSaveInNode(keySize);
      btreeItemLocation location;
      RECORD_SLOT_POS  toInsert = INVALID_RECORD_SLOT_POS;

      if (!hasFreeSpaceToInsert(keySize, &needCompact))
      {
         rc = SDB_VESSEL_NOT_ENOUGH_SPACE_IN_PAGE;
         goto error;
      }

      if (INVALID_RECORD_SLOT_POS  != pos)
      {
         if (getReadableHead()->totalSlotCount < pos)
         {
            PD_LOG(PDERROR, "insert pos[%d] out of bound", pos);
            rc = SDB_OUT_OF_BOUND;
            goto error;
         }

         toInsert = pos;
      }
      else
      {
         rc = locateKeyAndRid(key, rid, location);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to locate key and rid:%d", rc);
            goto error;
         }
         toInsert = location.slotPos;
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

      rc = _insert(toInsert, key, rid);
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

   INT32 btreeNode::reactiveRemovedKey(const btreeItemLocation &location)
   {
      INT32 rc = SDB_OK;
      btreeItemSlot *slot = NULL;

      if (OSS_UNLIKELY(!location.identical ||
                       INVALID_RECORD_SLOT_POS  == location.slotPos))
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
      else if (getReadableHead()->totalSlotCount <= location.slotPos)
      {
         rc = SDB_OUT_OF_BOUND;
         goto error;
      }
      else if (!getReadableSlot(location.slotPos)->isMarkedDeleted())
      {
         PD_LOG(PDERROR, "item[%d] is not marked as removed", location.slotPos);
         rc = SDB_IXM_IDENTICAL_KEY;
         goto error;
      }

      rc = _buffer->prepareToWrite();
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to prepare to write:%d", rc);
         goto error;
      }

      slot = getWritableSlot(location.slotPos);
      OSS_BIT_CLEAR(slot->flags, btreeItemSlot::FLAG_MARKED_DELETED);

      commit();

   done:
      return rc;
   error:
      goto done;
   }

   INT32 btreeNode::keyLocate(const BSONObj &prevKey,
                              INT32 fieldCountToCmpInPrev,
                              const VEC_ELE_CMP &matchEle,
                              const inclusiveVec &matchInclusive,
                              BOOLEAN exclusive,
                              BOOLEAN forward,
                              btreeItemLocation &location,
                              BOOLEAN &outOfBound,
                              bson::BufBuilder *bb)
   {
      INT32 rc = SDB_OK;
      bson::BufBuilder localBuilder;
      bson::BufBuilder *builder = (NULL == bb) ? &localBuilder : bb;
      INT32 direction = forward ? 1 : -1;
      RECORD_SLOT_POS  low = INVALID_RECORD_SLOT_POS;
      RECORD_SLOT_POS  high = INVALID_RECORD_SLOT_POS;
      RECORD_SLOT_POS  bound = INVALID_RECORD_SLOT_POS;
      btreeIndexItem item;
      INT32 result = 0;
      orderingWrapper ow;
      RECORD_SLOT_POS  pos = INVALID_RECORD_SLOT_POS;

      location = btreeItemLocation();
      outOfBound = FALSE;

      if (OSS_UNLIKELY(!isValid()))
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }
      
      SDB_ASSERT(0 < getItemCount(), "can not be empty");
      SDB_ASSERT(!hasCompressedKeys(), "TODO");

      ow = getProperties()->getPattern().getOrdering();
      low = 0;
      high = getItemCount() - 1;
      bound = forward ? low : high;

      rc = getItem(bound, item);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to get item of pos[%d], rc:%d",
                bound, rc);
         goto error;
      }

      builder->reset();
      result = indexUtils::compareKey(ixmKey(item.getKeySlice().data()).toBson(builder),
                                      prevKey, fieldCountToCmpInPrev,
                                      exclusive, matchEle, matchInclusive,
                                      ow.toBsonOrdering(), direction);
      if (0 <= (direction * result))
      {
         outOfBound = FALSE;
         
         if (forward)
         {
            /// can not get child at leaf node, it may used to save prefix ptr.
            location.child = isLeaf() ? INVALID_PAGE_ID : getLeftChild(0);
            location.isUpperBound = FALSE;
            location.slotPos = 0;
         }
         else
         {
            location.child = isLeaf() ? INVALID_PAGE_ID : getRightChild();
            location.isUpperBound = TRUE;
            location.slotPos = high + 1;
         }

         goto done;
      }

      bound = forward ? high : low;
      rc = getItem(bound, item);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to get item of pos[%d], rc:%d",
                bound, rc);
         goto error;
      }

      builder->reset();
      result = indexUtils::compareKey(ixmKey(item.getKeySlice().data()).toBson(builder),
                                      prevKey, fieldCountToCmpInPrev,
                                      exclusive, matchEle, matchInclusive,
                                      ow.toBsonOrdering(), direction);
      if ( direction * result < 0 )
      {
         outOfBound = TRUE;
         if (forward)
         {
            location.child = isLeaf() ? INVALID_PAGE_ID : getRightChild();
            location.isUpperBound = TRUE;
            location.slotPos = high + 1;
         }
         else
         {
            location.child = isLeaf() ? INVALID_PAGE_ID : getLeftChild(0);
            location.isUpperBound = FALSE;
            location.slotPos = 0;
         }

         goto done;
      }

      builder->reset();
      rc = find(low, high, prevKey, fieldCountToCmpInPrev,
                matchEle, matchInclusive, exclusive,
                forward, *builder, pos);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to find key in node:%d", rc);
         goto error;
      }

      if (OSS_UNLIKELY(INVALID_RECORD_SLOT_POS  == pos))
      {
         PD_LOG(PDERROR, "failed to find key in current node");
         rc = SDB_VESSEL_INTERNAL_ERR;
         goto error;
      }
      SDB_ASSERT(pos <= high, "out of bound");
      outOfBound = FALSE;
      location.child = isLeaf() ? INVALID_PAGE_ID : getLeftChild(pos);
      location.isUpperBound = FALSE;
      location.slotPos = pos;

   done:
      return rc;
   error:
      location = btreeItemLocation();
      goto done;
   }

   INT32 btreeNode::keyAdvance(RECORD_SLOT_POS  pos,
                              const BSONObj &prevKey,
                              INT32 fieldCountToCmpInPrev,
                              const VEC_ELE_CMP &matchEle,
                              const inclusiveVec &matchInclusive,
                              BOOLEAN exclusive,
                              BOOLEAN forward,
                              BOOLEAN &goBackToFather,
                              btreeItemLocation &location,
                              bson::BufBuilder *bb)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(!hasCompressedKeys(), "TODO");
      RECORD_SLOT_POS  low = INVALID_RECORD_SLOT_POS;
      RECORD_SLOT_POS  high = INVALID_RECORD_SLOT_POS;
      BOOLEAN currentNode = FALSE;
      btreeIndexItem item;
      orderingWrapper ow;
      INT32 direction = forward ? 1 : -1;
      bson::BufBuilder localBuilder;
      bson::BufBuilder *builder = (NULL == bb) ? &localBuilder : bb;

      goBackToFather = FALSE;
      location = btreeItemLocation();
      if (NULL != bb)
      {
         bb->reset();
      }

      if (OSS_UNLIKELY(!isValid()))
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }

      SDB_ASSERT(0 < getItemCount(), "can not be empty");
      ow = getProperties()->getPattern().getOrdering();

      if (forward)
      {
         ixmKey key;
         low = (INVALID_RECORD_SLOT_POS  == pos) ?
                0 : pos;
         high = getItemCount() - 1;
         rc = getItem(high, item);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to get item[%d], rc:%d", high, rc);
            goto error;
         }

         key.assign(item.getKeySlice().data());
         builder->reset();
         currentNode = (0 <= indexUtils::compareKey(key.toBson(builder), prevKey,
                                                    fieldCountToCmpInPrev,
                                                    exclusive, matchEle,
                                                    matchInclusive,
                                                    ow.toBsonOrdering(),
                                                    direction));
      }
      else
      {
         ixmKey key;
         low = 0;
         high = (INVALID_RECORD_SLOT_POS  == pos) ?
                 (getItemCount() - 1) : pos;
         rc = getItem(high, item);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to get item[%d], rc:%d", high, rc);
            goto error;
         }
         key.assign(item.getKeySlice().data());
         builder->reset();
         currentNode = (0 >= indexUtils::compareKey(key.toBson(builder), prevKey,
                                                    fieldCountToCmpInPrev,
                                                    exclusive, matchEle,
                                                    matchInclusive,
                                                    ow.toBsonOrdering(),
                                                    direction));
      }

      if (!currentNode)
      {
         goBackToFather = TRUE;
         goto done;
      }
      else
      {
         RECORD_SLOT_POS  found = INVALID_RECORD_SLOT_POS;
         builder->reset();
         rc = find(low, high, prevKey, fieldCountToCmpInPrev,
                matchEle, matchInclusive, exclusive,
                forward, *builder, found);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to find key in current node:%d", rc);
            goto error;
         }

         SDB_ASSERT(found < getReadableHead()->totalSlotCount, "impossible");
         location.child = isLeaf() ? INVALID_PAGE_ID : getLeftChild(found);
         location.slotPos = found;
         location.isUpperBound = FALSE;
         location.identical = FALSE;
      }

      
   done:
      return rc;
   error:
      goto done;
   }

   INT32 btreeNode::locateKeyAndRid(const ixmKey &key,
                                    const recordID &rid,
                                    btreeItemLocation &res)const
   {
      INT32 rc = SDB_OK;
      const btreeNodePageHead *head = NULL;
      orderingWrapper ow;
      INT32 low = 0;
      INT32 high = 0;
      INT32 middle = 0;

      res = btreeItemLocation();

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
      ow = getProperties()->getPattern().getOrdering();
      high = (INT32)(head->totalSlotCount) - 1;
      middle = (low + high) >> 1;

      while (low <= high)
      {
         INT32 cmp = 0;
         btreeIndexItem item;
         rc = getItem((RECORD_SLOT_POS)  middle, item);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to get item at[%d], rc:%d", middle, rc);
            goto error;
         }

         cmp = item.woCompare(key, ow.toBsonOrdering());
         if (0 == cmp)
         {
            cmp = rid.compare(item.getRid());
         }

         if (cmp < 0)
         {
            low = middle + 1;
         }
         else if (0 < cmp)
         {
            high = middle - 1;
         }
         else
         {
            res.slotPos = (RECORD_SLOT_POS)middle;
            res.identical = TRUE;
            res.child = isLeaf() ? INVALID_PAGE_ID :
                        item.getSlot().data.nlf.leftChild;
            res.isUpperBound = FALSE;
            goto done;
         }

         middle = (low + high) >> 1;
      }

      res.slotPos = low;
      if (!isLeaf())
      {
         if ((INT32)head->totalSlotCount == low)
         {
            res.child = head->rightChild;
         }
         else
         {
            res.child = getReadableSlot(low)->data.nlf.leftChild;
         }
      }

      res.isUpperBound = (head->totalSlotCount == res.slotPos);
   done:
      return rc;
   error:
      res = btreeItemLocation();
      goto done;
   }

   INT32 btreeNode::getItem(RECORD_SLOT_POS  pos,
                            btreeIndexItem &item)const
   {
      INT32 rc = SDB_OK;
      const btreeNodePageHead *head = NULL;
      const btreeItemSlot *slot = NULL;
      strictBuffer buffer;

      item.reset();

      if (OSS_UNLIKELY(!isValid()))
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }
      else if (OSS_UNLIKELY(INVALID_RECORD_SLOT_POS  == pos))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      buffer = _buffer->getReadableBodyBuffer();

      head = getReadableHead();
      if (head->totalSlotCount <= pos)
      {
         rc = SDB_OUT_OF_BOUND;
         goto error;
      }

      slot = getReadableSlot(pos);
      if (OSS_UNLIKELY(NULL == slot))
      {
         PD_LOG(PDERROR, "failed to get readable slot");
         rc = SDB_VESSEL_INTERNAL_ERR;
         goto error;
      }

      if (slot->isKeyCompressed())
      {
         SDB_ASSERT(FALSE, "TODO");
      }
      else
      {
         slice keySlice;
         const CHAR *keyData = buffer.getReadablePtr(slot->data.key.offset,
                                                 slot->data.key.size);
         if (OSS_UNLIKELY(NULL == keyData))
         {
            PD_LOG(PDERROR, "failed to get key data[%d,%d]",
                   slot->data.key.offset,
                   slot->data.key.size);
            rc = SDB_VESSEL_INTERNAL_ERR;
            goto error;
         }

         keySlice.reset(slot->data.key.size, keyData);
         item.initWhenNormal(pos, *slot, keySlice);
      }
   done:
      return rc;
   error:
      goto done;
   }

   INT32 btreeNode::_insert(RECORD_SLOT_POS  pos,
                            const ixmKey &key,
                            const recordID &rid,
                            PAGE_ID leftChild)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(isValid(), "can not be invalid");
      SDB_ASSERT(INVALID_RECORD_SLOT_POS  != pos, "can not be invalid");
      SDB_ASSERT(key.isValid() && rid.isValid(), "can not be invalid");

      btreeItemSlot *slot = NULL;
      UINT32 keySize = key.dataSize();
      UINT32 keyOffset = 0;
      UINT32 size = getSizeToSaveInNode(keySize);
      btreeNodePageHead *head = NULL;
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

      keyOffset = getKeyDataOffsetToWrite(head, keySize);
      rc = buffer.write(keyOffset, keySize, key.data());
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to write key data to[%d,%d], rc:%d",
                keyOffset, keySize, rc);
         goto error;
      }

      slot = getWritableSlot(pos);
      if (OSS_UNLIKELY(NULL == slot))
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
      if (INVALID_PAGE_ID == leftChild)
      {
         SDB_ASSERT(isLeaf(), "must be leaf node");
         slot->initAsLeafFormat(rid, keyOffset, keySize, FALSE);
      }
      else
      {
         SDB_ASSERT(!isLeaf(), "can not be leaf node");
         slot->initAsNonLeafFormat(rid, keyOffset, keySize, leftChild);
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
                                     RECORD_SLOT_POS  pos)
   {
      INT32 rc = SDB_OK;
      const btreeNodePageHead *head = NULL;
      BOOLEAN appendOnly = FALSE;
      UINT32 savingSize = 0;
      RECORD_SLOT_POS  insertPos = INVALID_RECORD_SLOT_POS;
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
      else if (OSS_UNLIKELY(!getLockingMode().isExclusive()))
      {
         rc = SDB_VESSEL_FORBIDDEN_OP_WLT;
         goto error;
      }
      else if (!hasFreeSpaceToInsert(raisedKey.getKeySize(), &needCompact))
      {
         PD_LOG(PDERROR, "not enough free space to save raised key");
         rc = SDB_VESSEL_NOT_ENOUGH_SPACE_IN_PAGE;
         goto error;
      }

      savingSize = getSizeToSaveInNode(raisedKey.getKeySize());
      head = getReadableHead();

      if (INVALID_RECORD_SLOT_POS  != pos)
      {
         if (head->totalSlotCount < pos)
         {
            PD_LOG(PDERROR, "insert pos[%d] out of bound", pos);
            rc = SDB_OUT_OF_BOUND;
            goto error;
         }
         insertPos = pos;
      }
      else
      {
         btreeItemLocation location;
         rc = locateKeyAndRid(ixmKey(raisedKey.getKeyData()), raisedKey.rid, location);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to locate key and rid:%d", rc);
            goto error;
         }
         insertPos = location.slotPos;
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

      rc = _insert(insertPos, ixmKey(raisedKey.getKeyData()),
                     raisedKey.rid, raisedKey.leftChild);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to insert key:%d", rc);
         goto error;
      }

      SDB_ASSERT(BTREE_NODE_SLOT_SIZE <= head->totalFreeSpace, "impossible");

      if (appendOnly)
      {
         strictBuffer buffer = _buffer->getWritableBodyBuffer();
         SDB_ASSERT(buffer.isWritable(), "must be writable");
         buffer.getWritableObjPtr<btreeNodePageHead>(0)->rightChild = raisedKey.rightChild;
      }
      else
      {
         getWritableSlot(insertPos + 1)->data.nlf.leftChild = raisedKey.rightChild;
      }
   done:
      return rc;
   error:
      goto done;
   }

   INT32 btreeNode::findSplitPivot(BOOLEAN idleRight,
                                   RECORD_SLOT_POS  &pivot)const
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(isValid(), "can not be invalid");
      SDB_ASSERT(!hasCompressedKeys(), "TODO");
      UINT32 splitSize = 0;
      UINT32 scanned = 0;
      const btreeNodePageHead *head = NULL;
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

      for (INT32 i = (INT32)(head->totalSlotCount - 1); 0 <= i; --i)
      {
         UINT32 savingSize = 0;
         const btreeItemSlot *slot = getReadableSlot(i);
         if (OSS_UNLIKELY(NULL == slot || !slot->isValid()))
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

      if (OSS_UNLIKELY(INVALID_RECORD_SLOT_POS  == pivot ||
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

   INT32 btreeNode::splitLeafAndInsert(const ixmKey &key,
                                       const recordID &rid,
                                       btreeSplitRaisedKey &raisedKey)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(isValid(), "can not be invalid");
      SDB_ASSERT(key.isValid() && rid.isValid(), "can not be invalid");

      btreeItemLocation location;
      RECORD_SLOT_POS  pivot = INVALID_RECORD_SLOT_POS;
      BOOLEAN idleRight = FALSE;
      btreeIndexItem item;
      PAGE_ID rightNode = INVALID_PAGE_ID;
      raisedKey.reset();

      if (OSS_UNLIKELY(!isValid()))
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }
      else if (OSS_UNLIKELY(!key.isValid() ||
                            !rid.isValid()))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      else if (!getLockingMode().isExclusive())
      {
         rc = SDB_VESSEL_FORBIDDEN_OP_WLT;
         goto error;
      }
      else if (!isLeaf())
      {
         rc = SDB_VESSEL_OPERATOION_NOT_PERMITTED;
         goto error;
      }

      rc = locateKeyAndRid(key, rid, location);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to locate key and rid:%d", rc);
         goto error;
      }

      if (location.slotPos == getReadableHead()->totalSlotCount &&
          isRecentWriteOrdered())
      {
         idleRight = TRUE;
      }

      rc = findSplitPivot(idleRight, pivot);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to find split pivot:%d", rc);
         goto error;
      }

      rc = getItem(pivot, item);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to get item[%d], rc:%d", pivot, rc);
         goto error;
      }

      raisedKey.rid = item.getRid();
      item.exportOriginalKey(raisedKey.keyBuilder);
      item.reset();

      rc = _splitAndCompact(pivot, rightNode);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to split node:%d", rc);
         goto error;
      }

      raisedKey.leftChild = _buffer->getLogicalPid();
      raisedKey.rightChild = rightNode;

      /// should not get error from here

      if (location.slotPos <= pivot)
      {         
         rc = _leafInsert(key, rid, location.slotPos);
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
         logicalPageBuffer buffer;
         btreeNode node = getRightNodeWhenSplit(rightNode, buffer);
         if (!node.isValid())
         {
            buffer.fini();
            PD_LOG(PDSEVERE, "failed to get right node");
            ossPanic();
            rc = SDB_VESSEL_INTERNAL_ERR;
            goto error;
         }
         rc = node._leafInsert(key, rid, location.slotPos - pivot - 1);
         if (SDB_OK != rc)
         {
            buffer.fini();
            PD_LOG(PDSEVERE, "failed to insert key into right node[%d,%d]:%d",
                   _ctx->getIndexObject()->getLogicalID(), rightNode, rc);
            ossPanic();
            goto error;
         }

         node.commit();
         buffer.fini();
      }
   done:
      return rc;
   error:
      raisedKey.reset();
      goto done;
   }

   INT32 btreeNode::_splitAndCompact(RECORD_SLOT_POS  pivot,
                                     PAGE_ID &rightNode)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(isValid(), "can not be invalid");
      SDB_ASSERT(INVALID_RECORD_SLOT_POS  != pivot, "can not be invalid");

      memoryBlock mb;
      btreeNodePageSplitIniter initer;
      logicalPageSpace *lps = _buffer->getLogicalPageSpace();
      strictBuffer buffer;
      btreeNodePageHead *head = NULL;
      PAGE_ID pivotLeftChild = INVALID_PAGE_ID;

      rightNode = INVALID_PAGE_ID;

      if (!_buffer->getLockingMode().isExclusive())
      {
         PD_LOG(PDERROR, "hold wrong type locking[%d] to split node",
                _buffer->getLockingMode().getModeEnum());
         rc = SDB_VESSEL_FORBIDDEN_OP_WLT;
         goto error;
      }

      rc = mb.reserve(getNodeSize());
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to reserve memory:%d", rc);
         goto error;
      }

      buffer.makeWritable(mb.getCapacity(), mb.getBuffer());
      rc = buildRightNodeWhenSplit(pivot + 1, buffer);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to build right node:%d", rc);
         goto error;
      }
      SDB_ASSERT(0 < pivot, "can not be zero");

      rc = _buffer->prepareToWrite();
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to prepare to write:%d", rc);
         goto error;
      }

      initer.set(buffer.getSlice());
      rc = lps->allocatePage(_buffer->getContext(), &initer, rightNode);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to allocate new node page:%d", rc);
         goto error;
      }

      if (!isLeaf())
      {
         pivotLeftChild = getReadableSlot(pivot)->data.nlf.leftChild;
      }

      rc = truncate(pivot - 1);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to truncate node:%d", rc);
         goto error;
      }

      head = _buffer->getWritableBodyBuffer().getWritableObjPtr<btreeNodePageHead>(0);
      if (!isLeaf())
      {
         head->rightChild = pivotLeftChild;
      }
      else if ((0 != OSS_BIT_TEST(head->flags, BTREE_NODE_FLAG_VAIN_PREFIX_REGENERATION)) &&
               !hitHighWaterMark())
      {
         OSS_BIT_CLEAR(head->flags, BTREE_NODE_FLAG_VAIN_PREFIX_REGENERATION);
      }

      _compact(0);
   done:
      return rc;
   error:
      if (INVALID_PAGE_ID != rightNode)
      {
         _buffer->getLogicalPageSpace()
                 ->releasePage(_buffer->getContext(), rightNode);
      }

      rightNode = INVALID_PAGE_ID;
      goto done;
   }

   INT32 btreeNode::buildRightNodeWhenSplit(RECORD_SLOT_POS  begin,
                                            strictBuffer &node)const
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(isValid(), "can not be invalid");
      SDB_ASSERT(INVALID_RECORD_SLOT_POS  != begin, "can not be invalid");
      SDB_ASSERT(getNodeSize() == node.getSize(), "must be same");

      btreeNodePageHead *newHead = node.getWritableObjPtr<btreeNodePageHead>(0);
      const btreeNodePageHead *head = getReadableHead();
      SDB_ASSERT(begin < head->totalSlotCount, "invalid begin");
      UINT32 frontOffset = BTREE_NODE_PAGE_HEAD_SIZE;
      btreeNodePageHead tmp;
      ossMemcpy(newHead, &tmp, BTREE_NODE_PAGE_HEAD_SIZE);

      newHead->version = head->version;
      if (0 != OSS_BIT_TEST(head->flags, BTREE_NODE_FLAG_IS_LEAF))
      {
         OSS_BIT_SET(newHead->flags, BTREE_NODE_FLAG_IS_LEAF);
      }

      newHead->indexId = head->indexId;
      newHead->totalFreeSpace = getNodeSize() - BTREE_NODE_PAGE_HEAD_SIZE;
      newHead->backOffset = getNodeSize();
      newHead->rightChild = head->rightChild;
      newHead->transSN = head->transSN;
      newHead->transNode = head->transNode;
      newHead->birthNodeLevel = head->birthNodeLevel;

      for (RECORD_SLOT_POS  i = begin; i < head->totalSlotCount; ++i)
      {
         btreeItemSlot *slot = NULL;
         UINT32 keyOffset = 0;
         btreeIndexItem item;
         rc = getItem(i, item);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to get item[%d], rc:%d", i, rc);
            goto error;
         }

         SDB_ASSERT(!item.getSlot().isKeyCompressed(), "TODO");

         if (newHead->backOffset <
             (frontOffset + getSizeToSaveInNode(item.getKeySlice().getSize())))
         {
            PD_LOG(PDERROR, "not enough free space to save item");
            rc = SDB_VESSEL_NOT_ENOUGH_SPACE_IN_PAGE;
            goto error;
         }

         keyOffset = getKeyDataOffsetToWrite(newHead, item.getKeySlice().getSize());
         rc = node.write(keyOffset, item.getKeySlice().getSize(), item.getKeySlice().data());
         if (OSS_UNLIKELY(SDB_OK != rc))
         {
            PD_LOG(PDERROR, "failed to write slice[%d,%d], rc:%d",
                   keyOffset, item.getKeySlice().getSize(), rc);
            goto error;
         }

         slot = node.getWritableObjPtr<btreeItemSlot>(frontOffset);
         if (isLeaf())
         {
            slot->initAsLeafFormat(item.getRid(), keyOffset,
                                   item.getKeySlice().getSize(), FALSE);
         }
         else
         {
            slot->initAsNonLeafFormat(item.getRid(), keyOffset,
                                      item.getKeySlice().getSize(),
                                      item.getSlot().data.nlf.leftChild);
         }
         newHead->totalFreeSpace -= BTREE_NODE_SLOT_SIZE;
         newHead->totalFreeSpace -= item.getKeySlice().getSize();
         newHead->backOffset = keyOffset;
         ++newHead->totalSlotCount;
         frontOffset += BTREE_NODE_SLOT_SIZE;
      }
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
         _buffer->commit(executor->getEndLsn());

         updateTransID(DPS_TRANS_ID());
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
         if (DPS_INVALID_TRANSID_SN == head->transSN ||
             head->transSN < transID.getSN())
         {
            head->transSN = transID.getSN();
            head->transNode = transID.getNodeID();
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
      SDB_ASSERT(NULL != head, "can not be null");

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
      btreeNodePageHead *head = NULL;
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

      for (RECORD_SLOT_POS  i = 0; i < rh->totalSlotCount; ++i)
      {
         btreeIndexItem item;
         const btreeItemSlot *slot = getReadableSlot(i);
         SDB_ASSERT(!slot->isKeyCompressed(), "TODO");
         const CHAR *keyData = writableBuffer.getReadablePtr(slot->data.key.offset,
                                                             slot->data.key.size);
         UINT32 keySize = slot->data.key.size;
         SDB_ASSERT(NULL != keyData && 0 < keySize, "can not be invalid");
         btreeItemSlot newSlot;

         if (backOffset < (frontOffset + BTREE_NODE_SLOT_SIZE + keySize))
         {
            PD_LOG(PDERROR, "not enough free space to insert item");
            SDB_ASSERT(FALSE, "impossible");
            rc = SDB_VESSEL_NOT_ENOUGH_FREE_RESOURCE;
            goto error;
         }

         if (0 < keySize)
         {
            backOffset -= keySize;
            rc = compactionBuffer.write(backOffset, keySize, keyData);
            if (OSS_UNLIKELY(SDB_OK != rc))
            {
               PD_LOG(PDERROR, "failed to copy key data:%d", rc);
               SDB_ASSERT(FALSE, "impossible");
               goto error;
            }

            if (isLeaf())
            {
               newSlot.initAsLeafFormat(recordID(slot->ridPage, slot->ridPos),
                                        backOffset, keySize, FALSE);
            }
            else
            {
               newSlot.initAsNonLeafFormat(recordID(slot->ridPage, slot->ridPos),
                                           backOffset, keySize,
                                           slot->data.nlf.leftChild);
            }
         }
         else
         {
            newSlot = *slot;
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
      SDB_ASSERT(INVALID_RECORD_SLOT_POS  != max, "can not be invalid");
      UINT32 size = 0;
      btreeNodePageHead *head = _buffer->getWritableBodyBuffer().getWritableObjPtr<btreeNodePageHead>(0);
      if (NULL == head)
      {
         PD_LOG(PDERROR, "failed to get writable head ptr");
         rc = SDB_VESSEL_INTERNAL_ERR;
         goto error;
      }

      SDB_ASSERT((UINT32)(max + 1) < head->totalSlotCount, "invalid truncate range");
      SDB_ASSERT(!hasCompressedKeys(), "TODO");

      for (RECORD_SLOT_POS  i = head->totalSlotCount - 1; i > max; --i)
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

   btreeNode btreeNode::getRightNodeWhenSplit(PAGE_ID right,
                                              logicalPageBuffer &buffer)
   {
      SDB_ASSERT(isValid(), "can not be invalid");
      SDB_ASSERT(INVALID_PAGE_ID != right, "can not be invalid");
      SDB_ASSERT(!_ctx->isNonPte(), "wrong mode");
      SDB_ASSERT(!buffer.isValid(), "can not be valid");

      btreeNode node;
      indexSpace *is = _ctx->getSpaceCtx().getIndexSpace();
      INT32 rc = is->getLogicalPageBuffer(_ctx->getSpaceCtx(), right,
                                          !_ctx->isNonPte(), buffer);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to get page[%d] buffer, rc:%d", right, rc);
         goto error;
      }
      
      rc = buffer.prepareToWrite();
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "faile to get page[%d] writable:%d", right, rc);
         goto error;
      }

      node = btreeNode(_depth, &buffer, _ctx);

   done:
      return node;
   error:
      buffer.fini();
      goto done;
   }

   INT32 btreeNode::split(btreeSplitRaisedKey &raisedKey)
   {
      INT32 rc = SDB_OK;
      RECORD_SLOT_POS  pivot = INVALID_RECORD_SLOT_POS;
      PAGE_ID rightNode = INVALID_PAGE_ID;
      btreeIndexItem item;
      BOOLEAN idleRight = FALSE;

      raisedKey.reset();

      if (OSS_UNLIKELY(!isValid()))
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }
      else if (!getLockingMode().isExclusive())
      {
         rc = SDB_VESSEL_FORBIDDEN_OP_WLT;
         goto error;
      }

      if (isRecentWriteOrdered())
      {
         idleRight = TRUE;
      }

      rc = findSplitPivot(idleRight, pivot);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to find split pivot:%d", rc);
         goto error;
      }

      rc = getItem(pivot, item);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to get item[%d], rc:%d", pivot, rc);
         goto error;
      }

      raisedKey.rid = item.getRid();
      item.exportOriginalKey(raisedKey.keyBuilder);
      item.reset();

      rc = _splitAndCompact(pivot, rightNode);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to split node:%d", rc);
         goto error;
      }

      raisedKey.leftChild = _buffer->getLogicalPid();
      raisedKey.rightChild = rightNode;
      commit();
   done:
      return rc;
   error:
      raisedKey.reset();
      goto done;
   }

   INT32 btreeNode::splitNonLeafAndInsert(const btreeSplitRaisedKey &raisedKeyFromChild,
                                          btreeSplitRaisedKey &raisedKey)
   {
      INT32 rc = SDB_OK;
      btreeItemLocation location;
      RECORD_SLOT_POS  pivot = INVALID_RECORD_SLOT_POS;
      BOOLEAN idleRight = FALSE;
      btreeIndexItem item;
      PAGE_ID rightNode = INVALID_PAGE_ID;

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
      else if (!getLockingMode().isExclusive())
      {
         rc = SDB_VESSEL_FORBIDDEN_OP_WLT;
         goto error;
      }
      

      rc = locateKeyAndRid(ixmKey(raisedKeyFromChild.getKeyData()),
                           raisedKeyFromChild.rid, location);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to locate key and rid:%d", rc);
         goto error;
      }

      if (location.slotPos == getReadableHead()->totalSlotCount &&
          isRecentWriteOrdered())
      {
         idleRight = TRUE;
      }

      rc = findSplitPivot(idleRight, pivot);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to find split pivot:%d", rc);
         goto error;
      }

      rc = getItem(pivot, item);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to get item[%d], rc:%d", pivot, rc);
         goto error;
      }

      raisedKey.rid = item.getRid();
      item.exportOriginalKey(raisedKey.keyBuilder);
      item.reset();

      rc = _splitAndCompact(pivot, rightNode);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to split node:%d", rc);
         goto error;
      }

      raisedKey.leftChild = _buffer->getLogicalPid();
      raisedKey.rightChild = rightNode;

      /// should not get error from here
      if (location.slotPos <= pivot)
      {
         rc = _insertRaisedKey(raisedKeyFromChild, location.slotPos);
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
         logicalPageBuffer buffer;
         btreeNode node = getRightNodeWhenSplit(rightNode, buffer);
         if (!node.isValid())
         {
            PD_LOG(PDSEVERE, "failed to get right node");
            ossPanic();
            rc = SDB_VESSEL_INTERNAL_ERR;
            goto error;
         }
         rc = node._insertRaisedKey(raisedKeyFromChild, location.slotPos - pivot - 1);
         if (SDB_OK != rc)
         {
            PD_LOG(PDSEVERE, "failed to insert raised key into right node[%d,%d]:%d",
                   _ctx->getIndexObject()->getLogicalID(), rightNode, rc);
            buffer.fini();
            ossPanic();
            goto error;
         }

         node.commit();
         buffer.fini();
      }
   done:
      return rc;
   error:
      raisedKey.reset();
      goto done;
   }

   INT32 btreeNode::prepareToWrite()
   {
      INT32 rc = SDB_OK;
      if (OSS_UNLIKELY(!isValid()))
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }
      else if (!_buffer->getLockingMode().isExclusive())
      {
         rc = SDB_VESSEL_FORBIDDEN_OP_WLT;
         goto error;
      }

      rc = _buffer->prepareToWrite();
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to prepare to write:%d", rc);
         goto error;
      }
   done:
      return rc;
   error:
      goto done;
   }

   INT32 btreeNode::find(RECORD_SLOT_POS  low,
                         RECORD_SLOT_POS  high,
                         const bson::BSONObj &prevKey,
                         INT32 fieldCountToCmpInPrev,
                         const VEC_ELE_CMP &matchEle,
                         const inclusiveVec &matchInclusive,
                         BOOLEAN exclusive,
                         BOOLEAN forward,
                         bson::BufBuilder &bb,
                         RECORD_SLOT_POS  &pos)const
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(isValid(), "can not be invalid");
      SDB_ASSERT(INVALID_RECORD_SLOT_POS  != low, "can not be invalid");
      SDB_ASSERT(INVALID_RECORD_SLOT_POS  != high, "can not be invalid");
      SDB_ASSERT(low <= high, "can not be invalid");
      SDB_ASSERT(!hasCompressedKeys(), "TODO");

      INT32 direction = forward ? 1 : -1;
      btreeIndexItem item;
      orderingWrapper ow = getProperties()->getPattern().getOrdering();

      pos = INVALID_RECORD_SLOT_POS;

      INT32 l = (INT32)low;
      INT32 h = (INT32)high;
      INT32 m = 0;

      while (TRUE)
      {
         INT32 result = 0;
         ixmKey key;
         item.reset();

         if (l > h)
         {
            INT32 tmp = forward ? l : h;
            if ((INT32)low <= tmp && tmp <= (INT32)high)
            {
               pos = (RECORD_SLOT_POS)  tmp;
            }
            goto done;
         }

         m = (l + h) >> 1;
         rc = getItem((RECORD_SLOT_POS)  m, item);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to get item[%d], rc:%d", m, rc);
            goto error;
         }

         bb.reset();
         key.assign(item.getKeySlice().data());
         result = indexUtils::compareKey(key.toBson(&bb), prevKey,
                                         fieldCountToCmpInPrev,
                                         exclusive,
                                         matchEle,
                                         matchInclusive,
                                         ow.toBsonOrdering(),
                                         direction);
         if (result < 0)
         {
            l = m + 1;
         }
         else if (0 < result)
         {
            h = m - 1;
         }
         else
         {
            if (forward)
            {
               h = m - 1;
            }
            else
            {
               l = m + 1;
            }
         }
      }
   done:
      return rc;
   error:
      pos = INVALID_RECORD_SLOT_POS;
      goto done;
   }

   INT32 btreeNode::removeChild(RECORD_SLOT_POS  pos)
   {
      INT32 rc = SDB_OK;
      if (OSS_UNLIKELY(INVALID_RECORD_SLOT_POS  == pos))
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
      else if (OSS_UNLIKELY(!getLockingMode().isExclusive()))
      {
         rc = SDB_VESSEL_FORBIDDEN_OP_WLT;
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

   INT32 btreeNode::_removeChild(RECORD_SLOT_POS  pos)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(isValid(), "can not be invalid");
      SDB_ASSERT(INVALID_RECORD_SLOT_POS  != pos, "can not be invalid");
      SDB_ASSERT(!isLeaf(), "can not be leaf");

      const btreeNodePageHead *rh = getReadableHead();
      btreeNodePageHead *head = NULL;
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

      rc = _buffer->autoGetWritableBodyBuffer(buffer);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to get node ready to write:%d", rc);
         goto error;
      }

      rh = NULL;
      head = buffer.getWritableObjPtr<btreeNodePageHead>(0);
      if (pos == head->totalSlotCount)
      {
         head->rightChild = INVALID_PAGE_ID;
      }
      else
      {
         btreeItemSlot *slot = getWritableSlot(pos);
         SDB_ASSERT(INVALID_PAGE_ID != slot->data.nlf.leftChild, "impossible");
         slot->data.nlf.leftChild = INVALID_PAGE_ID;

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
      SDB_ASSERT(INVALID_RECORD_SLOT_POS  != pos, "can not be invalid");
      SDB_ASSERT(isValid(), "can not be invalid");
      SDB_ASSERT(getLockingMode().isExclusive(), "must be exlusive");
      SDB_ASSERT(pos < getReadableHead()->totalSlotCount, "out of bound");

      btreeNodePageHead *head = NULL;
      btreeItemSlot *slot = NULL;
      UINT32 size = BTREE_NODE_SLOT_SIZE;
      strictBuffer buffer;

      rc = _buffer->autoGetWritableBodyBuffer(buffer);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to get buffer ready to write:%d", rc);
         goto error;
      }

      head = buffer.getWritableObjPtr<btreeNodePageHead>(0);
      if (OSS_UNLIKELY(NULL == head))
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
      SDB_ASSERT(getLockingMode().isExclusive(), "must be exclusive");
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

   INT32 btreeNode::_markRemoved(RECORD_SLOT_POS  pos)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(isValidRecordSlotPosition(pos), "can not be invalid");
      SDB_ASSERT(isValid(), "can not be invalid");
      SDB_ASSERT(pos < (INT16)getItemCount(), "out of bound");
      SDB_ASSERT(getLockingMode().isExclusive(), "must be exclusive");
      SDB_ASSERT(!isLeaf(), "can not be leaf");

      btreeItemSlot *slot = NULL;

      rc = prepareToWrite();
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

   BOOLEAN btreeNode::becameEmptyAfterRemoving(RECORD_SLOT_POS  pos)const
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
                                      PAGE_ID child)
   {
      INT32 rc = SDB_OK;
      if (OSS_UNLIKELY(INVALID_RECORD_SLOT_POS  == pos ||
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
      else if (!getLockingMode().isExclusive())
      {
         rc = SDB_VESSEL_FORBIDDEN_OP_WLT;
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

      rc = prepareToWrite();
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to get node ready to write:%d", rc);
         goto error;
      }

      if (pos < (INT16)getItemCount())
      {
         getWritableSlot(pos)->data.nlf.leftChild = child;
      }
      else
      {
         btreeNodePageHead *head = _buffer->getWritableBodyBuffer().
                                   getWritableObjPtr<btreeNodePageHead>(0);
         head->rightChild = child;
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
      btreeNodePageHead *header = NULL;
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
      header->transSN = DPS_INVALID_TRANSID_SN;
      header->transNode = DPS_INVALID_TRANSID_NODEID;
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
} // namespace vessel

} // namespace engine

