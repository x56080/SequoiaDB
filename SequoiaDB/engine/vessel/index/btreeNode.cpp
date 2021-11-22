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
#include "vessel/btreeNodeCompressedKey.h"
#include "vessel/logicalPageBuffer.h"
#include "vessel/memoryBlock.h"
#include "ossMemPool.hpp"
#include "vessel/btreeExtKeyPageIniter.h"
#include "vessel/btreeExternalKeyPage.h"
#include "vessel/btreeAccessContext.h"
#include "vessel/indexEntryPageAccessor.h"
#include "vessel/indexUtils.h"

namespace engine
{
namespace vessel
{
   btreeNode::btreeNode(logicalPageBuffer *buffer,
                        UINT32 depth,
                        const indexContext *ic):
   _buffer(buffer),
   _depth(depth),
   _ic(ic)
   {
      SDB_ASSERT(NULL != _buffer && _buffer->isValid(), "can not be invalid");
      SDB_ASSERT(NULL != _ic && _ic->isValid(), "can not be invalid");
      SDB_ASSERT(_ic->getIndexType() == INDEX_TYPE_BTREE, "must be btree");
   }

   BOOLEAN btreeNode::isRoot()const
   {
      SDB_ASSERT(isValid(), "can not be invalid");
      return 0 != OSS_BIT_TEST(getReadableHead()->flags, BTREE_NODE_FLAG_IS_ROOT);
   }

   BOOLEAN btreeNode::hasExternalKey()const
   {
      return INVALID_PAGE_ID != getReadableHead()->externalKeyPage;
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
      if (r && NULL != needCompact)
      {
         *needCompact = (head->freeSapceAfterLastSlot < size);
      }
      return r;
   }

   BOOLEAN btreeNode::hasFreeSpaceToInsertRaisedKey(UINT32 keySize,
                                                    BOOLEAN *needCompact)const
   {
      SDB_ASSERT(0 < keySize, "can not be zero");
      return hasFreeSpaceToInsert(keySize + BTREE_NODE_SLOT_SIZE, needCompact);
   }

   BOOLEAN btreeNode::hasCompressedKeys()const
   {
      return isLeaf() && 0 < getReadableHead()->compressedItemCount;
   }

   BOOLEAN btreeNode::hasPrefix()const
   {
      return 0 < getReadableHead()->prefixCount;
   }

   const btreeNodePageHead *btreeNode::getReadableHead()const
   {
      SDB_ASSERT(isValid(), "can not be invalid");
      return _buffer->getReadableBodySlice().getReadableObjPtr<btreeNodePageHead>(0);
   }

   slice btreeNode::getReadableSlice()const
   {
      SDB_ASSERT(isValid(), "can not be invalid");
      return _buffer->getReadableBodySlice();
   }

   DPS_TRANS_ID btreeNode::getTransID()const
   {
      const btreeNodePageHead *head = getReadableHead();
      return DPS_TRANS_ID(head->transSN, head->transNode);
   }

   UINT32 btreeNode::getSplitedTimes()const
   {
      return getReadableHead()->splitedTimes;
   }

   btreeItemSlot btreeNode::getItemSlot(RECORD_SLOT_ID pos)const
   {
      return *getReadableSlot(pos);
   }

   BOOLEAN btreeNode::isCompressionDisabled()const
   {
      return !isLeaf() ||
             !_ic->getObj().getParams().isPrefixCompressionEnabled() ||
             _depth < _ic->getObj().getParams().btreeMinCompressionDepth;
   }

   UINT32 btreeNode::getKeyDataOffsetToWrite(const btreeNodePageHead *head,
                                             UINT32 keyDataSize)const
   {
      SDB_ASSERT(0 < keyDataSize, "can not be zero");
      SDB_ASSERT(NULL != head, "can not be null");
      SDB_ASSERT(keyDataSize <= head->freeSapceAfterLastSlot,
                 "out of resource");
      return BTREE_NODE_PAGE_HEAD_SIZE +
             (BTREE_NODE_SLOT_SIZE * head->totalSlotCount) +
             head->freeSapceAfterLastSlot -
             keyDataSize;
   }

   btreeItemSlot *btreeNode::getWritableSlot(RECORD_SLOT_ID pos)
   {
      SDB_ASSERT(INVALID_RECORD_SLOT_ID != pos, "can not be invalid");
      SDB_ASSERT(isValid(), "can not be invalid");
      SDB_ASSERT(_buffer->isWritable(), "must be prepared");
      UINT32 offset = BTREE_NODE_PAGE_HEAD_SIZE +
                      (BTREE_NODE_PREFIX_SLOT_SIZE * getReadableHead()->prefixCount) +
                      (BTREE_NODE_SLOT_SIZE * pos);
      return _buffer->getWritableBodySlice().
             getWritableObjPtr<btreeItemSlot>(offset);
   }

   const btreeItemSlot *btreeNode::getReadableSlot(RECORD_SLOT_ID pos)const
   {
      SDB_ASSERT(INVALID_RECORD_SLOT_ID != pos, "can not be invalid");
      SDB_ASSERT(isValid(), "can not be invalid");
      SDB_ASSERT(pos < getReadableHead()->totalSlotCount, "out of bound");
      UINT32 offset = BTREE_NODE_PAGE_HEAD_SIZE +
                      (BTREE_NODE_PREFIX_SLOT_SIZE * getReadableHead()->prefixCount) +
                      (BTREE_NODE_SLOT_SIZE * pos);
      return _buffer->getReadableBodySlice().
             getReadableObjPtr<btreeItemSlot>(offset);
   }

   const btreeNodePrefixSlot *btreeNode::getReadablePrefixSlot(UINT16 pos)const
   {
      SDB_ASSERT(isValid(), "can not be invalid");
      const btreeNodePageHead *head = getReadableHead();
      SDB_ASSERT(pos < head->prefixCount, "out of bound");
      UINT32 offset = BTREE_NODE_PAGE_HEAD_SIZE + (pos * BTREE_NODE_PREFIX_SLOT_SIZE);
      return getReadableSlice().getReadableObjPtr<btreeNodePrefixSlot>(offset);
   }

   btreeNodePrefixSlot *btreeNode::getWritablePrefixSlot(UINT16 pos)
   {
      SDB_ASSERT(isValid() && _buffer->isWritable(), "can not be invalid");
      const btreeNodePageHead *head = getReadableHead();
      SDB_ASSERT(pos < head->prefixCount, "out of bound");
      UINT32 offset = BTREE_NODE_PAGE_HEAD_SIZE + (pos * BTREE_NODE_PREFIX_SLOT_SIZE);
      return _buffer->getWritableBodySlice().getWritableObjPtr<btreeNodePrefixSlot>(offset);
   }

   UINT32 btreeNode::getNodeSize()const
   {
      SDB_ASSERT(isValid(), "can not be invalid");
      return _buffer->getReadableBodySlice().getSize();
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

   BOOLEAN btreeNode::isItemMarkedAsDeleted(RECORD_SLOT_ID pos)const
   {
      SDB_ASSERT(isValid(), "can not be invalid");
      SDB_ASSERT(INVALID_RECORD_SLOT_ID != pos, "can not be invalid");
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

   PAGE_ID btreeNode::getLeftChild(RECORD_SLOT_ID pos)const
   {
      SDB_ASSERT(isValid(), "can not be invalid");
      SDB_ASSERT(INVALID_RECORD_SLOT_ID != pos, "can not be invalid");
      SDB_ASSERT(!isLeaf(), "can not be leaf");
      SDB_ASSERT(pos < getReadableHead()->totalSlotCount, "out of bound");
      return getReadableSlot(pos)->data.nlf.leftChild;
   }

   PAGE_ID btreeNode::getChild(RECORD_SLOT_ID pos)const
   {
      SDB_ASSERT(isValid(), "can not be invalid");
      SDB_ASSERT(INVALID_RECORD_SLOT_ID != pos, "can not be invalid");
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

   INT32 btreeNode::destroyItem(RECORD_SLOT_ID pos)
   {
      INT32 rc = SDB_OK;
      if (OSS_UNLIKELY(INVALID_RECORD_SLOT_ID == pos))
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

   INT32 btreeNode::nonleafRemove(RECORD_SLOT_ID pos)
   {
      INT32 rc = SDB_OK;
      if (OSS_UNLIKELY(INVALID_RECORD_SLOT_ID == pos))
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
      else if (isLeaf())
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
                                RECORD_SLOT_ID pos)
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
      RECORD_SLOT_ID toInsert = INVALID_RECORD_SLOT_ID;

      if (!hasFreeSpaceToInsert(keySize, &needCompact))
      {
         rc = SDB_VESSEL_NOT_ENOUGH_SPACE_IN_PAGE;
         goto error;
      }

      if (INVALID_RECORD_SLOT_ID != pos)
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

      if (!hasPrefix())
      {
         rc = _insert(toInsert, key, rid);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to insert to node:%d", rc);
            goto error;
         }
      }
      else
      {
         btreeNodeCompressedKey ck;
         rc = tryToCompressKeyInserting(toInsert, key, ck);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to compress key:%d", rc);
            goto error;
         }

         if (ck.isValid())
         {
            rc = insertCompressedKey(toInsert, ck, rid);
            if (SDB_OK != rc)
            {
               PD_LOG(PDERROR, "failed to insert compressed key:%d", rc);
               goto error;
            }
         }
         else
         {
            rc = _insert(toInsert, key, rid);
            if (SDB_OK != rc)
            {
               PD_LOG(PDERROR, "failed to insert to node:%d", rc);
               goto error;
            }
         }
      }

      if (!isCompressionDisabled() && isBetterToBeRecompressed())
      {
         BOOLEAN recompressed = FALSE;
         recompress(recompressed);
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

   INT32 btreeNode::insertRaisedKeyAsExtKey(const btreeSplitRaisedKey &raisedKey)
   {
      INT32 rc = SDB_OK;
      rc = _insertExternalKey(raisedKey);
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
      slice ws;
      btreeItemSlot *slot = NULL;

      if (OSS_UNLIKELY(!location.identical ||
                       INVALID_RECORD_SLOT_ID == location.slotPos))
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

      rc = _buffer->autoGetWritableBodySlice(ws);
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
      RECORD_SLOT_ID low = INVALID_RECORD_SLOT_ID;
      RECORD_SLOT_ID high = INVALID_RECORD_SLOT_ID;
      RECORD_SLOT_ID bound = INVALID_RECORD_SLOT_ID;
      btreeIndexItem item;
      INT32 result = 0;
      orderingWrapper ow;
      RECORD_SLOT_ID pos = INVALID_RECORD_SLOT_ID;

      location = btreeItemLocation();
      outOfBound = FALSE;

      if (OSS_UNLIKELY(!isValid()))
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }
      
      SDB_ASSERT(0 < getItemCount(), "can not be empty");
      SDB_ASSERT(!hasCompressedKeys(), "TODO");

      ow = _ic->getObj().getPattern().getOrdering();
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
      result = indexUtils::compareKey(ixmKey(item.getSavingKeyData()).toBson(builder),
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
      result = indexUtils::compareKey(ixmKey(item.getSavingKeyData()).toBson(builder),
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

      if (OSS_UNLIKELY(INVALID_RECORD_SLOT_ID == pos))
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

   INT32 btreeNode::keyAdvance(RECORD_SLOT_ID pos,
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
      RECORD_SLOT_ID low = INVALID_RECORD_SLOT_ID;
      RECORD_SLOT_ID high = INVALID_RECORD_SLOT_ID;
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
      ow = _ic->getObj().getPattern().getOrdering();

      if (forward)
      {
         ixmKey key;
         low = (INVALID_RECORD_SLOT_ID == pos) ?
                0 : pos;
         high = getItemCount() - 1;
         rc = getItem(high, item);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to get item[%d], rc:%d", high, rc);
            goto error;
         }

         key.assign(item.getSavingKeyData());
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
         high = (INVALID_RECORD_SLOT_ID == pos) ?
                 (getItemCount() - 1) : pos;
         rc = getItem(high, item);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to get item[%d], rc:%d", high, rc);
            goto error;
         }
         key.assign(item.getSavingKeyData());
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
         RECORD_SLOT_ID found = INVALID_RECORD_SLOT_ID;
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
      ow = _ic->getObj().getPattern().getOrdering();
      high = (INT32)(head->totalSlotCount) - 1;
      middle = (low + high) >> 1;

      while (low <= high)
      {
         INT32 cmp = 0;
         btreeIndexItem item;
         rc = getItem((RECORD_SLOT_ID)middle, item);
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
            res.slotPos = (RECORD_SLOT_ID)middle;
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

   INT32 btreeNode::getItem(RECORD_SLOT_ID pos,
                            btreeIndexItem &item)const
   {
      INT32 rc = SDB_OK;
      const btreeNodePageHead *head = NULL;
      const btreeItemSlot *slot = NULL;
      slice rs;

      item.fini();

      if (OSS_UNLIKELY(!isValid()))
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }
      else if (OSS_UNLIKELY(INVALID_RECORD_SLOT_ID == pos))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      rs = _buffer->getReadableBodySlice();

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
      else if (!slot->isKeyInExtPage())
      {
         const CHAR *keyData = rs.getReadablePtr(slot->data.key.offset,
                                                 slot->data.key.size);
         if (OSS_UNLIKELY(NULL == keyData))
         {
            PD_LOG(PDERROR, "failed to get key data[%d,%d]",
                   slot->data.key.offset,
                   slot->data.key.size);
            rc = SDB_VESSEL_INTERNAL_ERR;
            goto error;
         }

         item.initWhenNormal(pos, slot, keyData);
      }
      else /// external key
      {
         rc = getItemWithExtKey(pos, item);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to get item with ext key:%d", rc);
            goto error;
         }
      }
   done:
      return rc;
   error:
      goto done;
   }

   INT32 btreeNode::_insert(RECORD_SLOT_ID pos,
                            const ixmKey &key,
                            const recordID &rid,
                            PAGE_ID leftChild)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(isValid(), "can not be invalid");
      SDB_ASSERT(INVALID_RECORD_SLOT_ID != pos, "can not be invalid");
      SDB_ASSERT(key.isValid() && rid.valid(), "can not be invalid");

      btreeItemSlot *slot = NULL;
      UINT32 keySize = key.dataSize();
      UINT32 keyOffset = 0;
      UINT32 size = getSizeToSaveInNode(keySize);
      btreeNodePageHead *head = NULL;
      slice writableSlice;
      BOOLEAN appendonly = FALSE;

      rc = _buffer->autoGetWritableBodySlice(writableSlice);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to get writable slice:%d", rc);
         goto error;
      }

      head = writableSlice.getWritableObjPtr<btreeNodePageHead>(0);
      /// should be prechecked outside
      SDB_ASSERT(size <= head->freeSapceAfterLastSlot, "impossible");
      SDB_ASSERT(pos <= head->totalSlotCount, "impossible");
      appendonly = (pos == head->totalSlotCount);

      keyOffset = getKeyDataOffsetToWrite(head, keySize);
      rc = writableSlice.write(keyOffset, keySize, key.data());
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
         slot->initAsNonLeafFormat(rid, keyOffset, keySize, leftChild, FALSE);
      }

      updateAppendingFactor(head, appendonly);
      ++head->totalSlotCount;
      head->totalFreeSpace -= size;
      head->freeSapceAfterLastSlot -= size;
   done:
      return rc;
   error:
      goto done;
   }

   INT32 btreeNode::_insertRaisedKey(const btreeSplitRaisedKey &raisedKey,
                                     RECORD_SLOT_ID pos)
   {
      INT32 rc = SDB_OK;
      const btreeNodePageHead *head = NULL;
      BOOLEAN appendOnly = FALSE;
      UINT32 savingSize = 0;
      RECORD_SLOT_ID insertPos = INVALID_RECORD_SLOT_ID;

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
      else if (OSS_UNLIKELY(hasExternalKey()))
      {
         /// should always split node with ext key when traverse down
         SDB_ASSERT(FALSE, "has external key");
         rc = SDB_VESSEL_OPERATOION_NOT_PERMITTED;
         goto error;
      }
      else if (!hasFreeSpaceToInsertRaisedKey(raisedKey.getKeySize()))
      {
         PD_LOG(PDERROR, "not enough free space to save raised key");
         rc = SDB_VESSEL_NOT_ENOUGH_SPACE_IN_PAGE;
         goto error;
      }

      savingSize = getSizeToSaveInNode(raisedKey.getKeySize());
      head = getReadableHead();

      if (INVALID_RECORD_SLOT_ID != pos)
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

      if (head->freeSapceAfterLastSlot < savingSize)
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
         slice s = _buffer->getWritableBodySlice();
         SDB_ASSERT(s.isWritale(), "must be writable");
         s.getWritableObjPtr<btreeNodePageHead>(0)->rightChild = raisedKey.rightChild;
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

   INT32 btreeNode::_insertExternalKey(const btreeSplitRaisedKey &raisedKey,
                                       RECORD_SLOT_ID pos)
   {
      INT32 rc = SDB_OK;
      const btreeNodePageHead *head = NULL;
      RECORD_SLOT_ID insertPos = INVALID_RECORD_SLOT_ID;
      BOOLEAN appendOnly = FALSE;

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
      else if (OSS_UNLIKELY(hasExternalKey()))
      {
         /// should always split node with ext key when traverse down
         SDB_ASSERT(FALSE, "has external key");
         rc = SDB_VESSEL_OPERATOION_NOT_PERMITTED;
         goto error;
      }
      else if (hasFreeSpaceToInsertRaisedKey(raisedKey.getKeySize()))
      {
         SDB_ASSERT(FALSE, "should not insert it as external key");
         rc = SDB_VESSEL_OPERATOION_NOT_PERMITTED;
         goto error;
      }
      else if (!hasFreeSpaceToInsert(0))
      {
         PD_LOG(PDERROR, "not enough free space to save one slot");
         rc = SDB_VESSEL_NOT_ENOUGH_SPACE_IN_PAGE;
         goto error;
      }

      head = getReadableHead();
      if (INVALID_RECORD_SLOT_ID != pos)
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

      if (head->freeSapceAfterLastSlot < BTREE_NODE_SLOT_SIZE)
      {
         rc = _compact(BTREE_NODE_SLOT_SIZE);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to compact node:%d", rc);
            goto error;
         }
      }

      rc = insertExternalKey(insertPos, ixmKey(raisedKey.getKeyData()),
                             raisedKey.rid, raisedKey.leftChild);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to insert external key:%d", rc);
         goto error;
      }

      if (appendOnly)
      {
         slice s = _buffer->getWritableBodySlice();
         SDB_ASSERT(s.isWritale(), "must be writable");
         s.getWritableObjPtr<btreeNodePageHead>(0)->rightChild = raisedKey.rightChild;
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

   INT32 btreeNode::insertExternalKey(RECORD_SLOT_ID pos,
                                      const ixmKey &key,
                                      const recordID &rid,
                                      PAGE_ID leftChild)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(isValid(), "can not be invalid");
      SDB_ASSERT(!isLeaf(), "can not be leaf");
      SDB_ASSERT(!hasExternalKey(), "external key already exists");
      SDB_ASSERT(key.isValid(), "can not be invalid");
      SDB_ASSERT(rid.valid(), "can not be invalid");
      SDB_ASSERT(INVALID_RECORD_SLOT_ID != pos, "can not be invalid");
      SDB_ASSERT(INVALID_PAGE_ID != leftChild, "can not be invalid");

      btreeItemSlot *slot = NULL;
      logicalPageSpace *lps = _buffer->getLogicalPageSpace();
      btreeExtKeyPageIniter initer;
      PAGE_ID extp = INVALID_PAGE_ID;
      btreeNodePageHead *head = NULL;
      slice ns;
      BOOLEAN appendonly = FALSE;
      UINT32 keySize = key.dataSize();
      SDB_ASSERT(0 < keySize, "can not be empty");

      rc = _buffer->autoGetWritableBodySlice(ns);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to get writable slice");
         goto error;
      }

      head = ns.getWritableObjPtr<btreeNodePageHead>(0);
      SDB_ASSERT(pos <= head->totalSlotCount, "out of bound");
      SDB_ASSERT(BTREE_NODE_SLOT_SIZE <= head->freeSapceAfterLastSlot, "out of resource");
      appendonly = (pos == head->totalSlotCount);

      slot = getWritableSlot(pos);
      if (OSS_UNLIKELY(NULL == slot))
      {
         PD_LOG(PDERROR, "failed to get writable slot ptr[%d]", pos);
         rc = SDB_VESSEL_INTERNAL_ERR;
         goto error;
      }

      initer._indexId = _ic->getIndexID();
      initer._key.reset(keySize, key.data());
      rc = lps->allocatePage(_buffer->getContext(),
                             &initer, extp);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to allocate ext page:%d", rc);
         goto error;
      }

      /// do not goto error from here

      if (pos < head->totalSlotCount)
      {
         UINT32 moveSize = (head->totalSlotCount - pos) * BTREE_NODE_SLOT_SIZE;
         ossMemmove(slot + 1, slot, moveSize);
      }

      head->externalKeyPage = extp;
      slot->initAsNonLeafFormat(rid, 0, keySize, leftChild, TRUE);
      updateAppendingFactor(head, appendonly);
      ++head->totalSlotCount;
      head->totalFreeSpace -= BTREE_NODE_SLOT_SIZE;
      head->freeSapceAfterLastSlot -= BTREE_NODE_SLOT_SIZE;
   done:
      return rc;
   error:
      goto done;
   }

   INT32 btreeNode::getItemWithExtKey(RECORD_SLOT_ID pos,
                                      btreeIndexItem &item)const
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(isValid(), "can not be invalid");
      SDB_ASSERT(INVALID_RECORD_SLOT_ID != pos, "can not be invalid");
      SDB_ASSERT(!item.isValid(), "can not be valid");
      SDB_ASSERT(hasExternalKey(), "no external key exists");

      const btreeItemSlot *slot = NULL;
      logicalPageSpace *lps = _buffer->getLogicalPageSpace();
      requestContext *context = _buffer->getContext();
      logicalPageBuffer lpb;
      ossValuePtr ptr = 0;
      ossSharedLatchMode mode(OSS_SHARED_LATCH_MODE_ENUM_SHARED);
      slice rs;
      const btreeExternalKeyPageHead *head = NULL;

      slot = getReadableSlot(pos);
      if (NULL == slot)
      {
         PD_LOG(PDERROR, "failed to get readable slot[%d]", pos);
         rc = SDB_VESSEL_INTERNAL_ERR;
         goto error;
      }

      SDB_ASSERT(slot->isKeyInExtPage(), "must be external key");

      rc = lps->getLogicalPageBuffer(context,
                                     getReadableHead()->externalKeyPage,
                                     mode, lpb);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to get page buffer[%d], rc:%d",
                getReadableHead()->externalKeyPage, rc);
         goto error;
      }

      ptr = (ossValuePtr)(lpb.getRuntimeBuffer().getReadbleSlice().data());
      rc = validatePage(ptr, PAGE_TYPE_BTREE_EXTERNAL_KEY,
                        lpb.getPageSize(), lpb.getGlobalPid().page(),
                        lpb.getLogicalPid(), lpb.getCowTrigger().getPsv());
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to validate external key page[%s], rc:%d",
               lpb.getGlobalPid().toString().c_str(), rc);
         goto error;
      }

      rs = lpb.getReadableBodySlice();
      head = rs.getReadableObjPtr<btreeExternalKeyPageHead>(0);
      if (head->size != slot->data.key.size ||
          head->indexId != _ic->getIndexID())
      {
         PD_LOG(PDERROR, "unexpected page head found[%s]",
                getReadableHead()->externalKeyPage);
         rc = SDB_VESSEL_PAGE_HEAD_NOT_MATCH;
         goto error;
      }

      rc = item.initWhenExtKey(pos, slot, head->size,
                               rs.getReadablePtr(BTREE_EXT_KEY_PAGE_HEAD_SIZE, head->size));
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to save external key:%d", rc);
         goto error;
      }
   done:
      lpb.fini();
      return rc;
   error:
      item.fini();
      goto done;
   }

   INT32 btreeNode::tryToCompressKeyInserting(RECORD_SLOT_ID pos,
                                              const ixmKey &key,
                                              btreeNodeCompressedKey &ck)const
   {
      INT32 rc = SDB_OK;/*
      SDB_ASSERT(INVALID_RECORD_SLOT_ID != pos, "can not be invalid");
      SDB_ASSERT(key.isValid(), "can not be invalid");
      SDB_ASSERT(isLeaf(), "must be leaf");

      const btreeNodePageHead *head = NULL;
      const btreeItemSlot *slot = NULL;
      const btreeNodePrefixSlot *ps = NULL;
      const CHAR *prefixData = NULL;
      slice nodeSlice;

      ck.reset();

      if (!hasPrefix())
      {
         goto done;
      }

      head = getReadableHead();
      SDB_ASSERT(pos <= head->totalSlotCount, "out of slot bound");
      slot = getReadableSlot(pos < head->totalSlotCount ? pos : (pos - 1));
      if (OSS_UNLIKELY(NULL == slot))
      {
         PD_LOG(PDERROR, "failed to get slot[%d]");
         rc = SDB_VESSEL_INTERNAL_ERR;
         goto error;
      }

      if (!slot->isKeyCompressed())
      {
         goto done;
      }

      ps = &(head->prefixes[slot->getPrefixSlotPos()]);
      if (OSS_UNLIKELY(ps->isFree()))
      {
         PD_LOG(PDERROR, "unexpected free prefix slot[%d]", slot->getPrefixSlotPos());
         rc = SDB_VESSEL_INTERNAL_ERR;
         goto error;
      }

      nodeSlice = getReadableSlice();
      prefixData = nodeSlice.getReadablePtr(ps->prefixOffset, ps->prefixSize);
      if (OSS_UNLIKELY(NULL == prefixData))
      {
         PD_LOG(PDERROR, "failed to get prefix data[%d,%d]",
                ps->prefixOffset, ps->prefixSize);
         rc = SDB_VESSEL_INTERNAL_ERR;
         goto error;
      }

      rc = tryToCompressKey(key, slot->getPrefixSlotPos(),
                            ixmKey(prefixData), ck);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to compress key:%d", rc);
         goto error;
      }*/
   done:
      return rc;
   error:
      goto done;
   }

   INT32 btreeNode::tryToCompressKey(const ixmKey &key,
                                     UINT32 prefixPos,
                                     const ixmKey &prefix,
                                     btreeNodeCompressedKey &ck)const
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(prefixPos < BTREE_NODE_MAX_PREFIX_COUNT, "can not be invalid");
      SDB_ASSERT(key.isValid(), "can not be invalid");
      SDB_ASSERT(prefix.isValid(), "can not be invalid");
      SDB_ASSERT(!isCompressionDisabled(), "can not be disabled");
      SDB_ASSERT(!ck.isValid(), "can not be valid");

      ixmKeyCompressor::result res;
      ixmKeyCompressor compressor;
      rc = compressor.initPrefix(_ic->getObj().getParams().btreeMaxPrefixFields,
                                 prefix.data());
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to init compressor:%d", rc);
         goto error;
      }

      rc = compressor.compress(key, res);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to compress key:%d", rc);
         goto error;
      }

      if (res.ok())
      {
         ixmKey suffix;
         if (!res.isPerfectlyCompressed())
         {
            res.getSuffuix(suffix);
         }
         
         ck.shallowInit(prefixPos, prefix, suffix);
         if (ck.hasSuffix())
         {
            rc = ck.getSuffixOwned();
            if (SDB_OK != rc)
            {
               PD_LOG(PDERROR, "failed to get suffix owned:%d", rc);
               goto error;
            }
         }
      }
   done:
      return rc;
   error:
      ck.reset();
      goto done;
   }

   INT32 btreeNode::insertCompressedKey(RECORD_SLOT_ID pos,
                                        const btreeNodeCompressedKey &ck,
                                        const recordID &rid)
   {
      INT32 rc = SDB_OK;/*
      SDB_ASSERT(isValid(), "can not be invalid");
      SDB_ASSERT(INVALID_RECORD_SLOT_ID != pos, "can not be invalid");
      SDB_ASSERT(ck.isValid(), "can not be invalid");
      SDB_ASSERT(rid.valid(), "can not be invalid");
      SDB_ASSERT(isLeaf(), "must be leaf node");

      btreeNodePageHead *head = NULL;
      btreeItemSlot *slot = NULL;
      UINT32 suffixSize = ck.getSuffixSize();
      UINT32 suffixOffset = 0;
      UINT32 size = getSizeToSaveInNode(suffixSize);
      btreeNodePrefixSlot *ps = NULL;
      slice nodeSlice;

      rc = _buffer->autoGetWritableBodySlice(nodeSlice);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to get writable slice:%d", rc);
         goto error;
      }

      head = nodeSlice.getWritableObjPtr<btreeNodePageHead>(0);
      SDB_ASSERT(ck.getPrefixSlotPos() < (INT32)BTREE_NODE_MAX_PREFIX_COUNT, "out of bound");
      ps = &(head->prefixes[ck.getPrefixSlotPos()]);
      if (OSS_UNLIKELY(ps->isFree()))
      {
         PD_LOG(PDERROR, "prefix[%d] is not valid", ck.getPrefixSlotPos());
         rc = SDB_VESSEL_INTERNAL_ERR;
         goto error;
      }
      
      /// should be prechecked out side
      SDB_ASSERT(pos <= head->totalSlotCount, "out of bound");
      SDB_ASSERT(size <= head->freeSapceAfterLastSlot, "out of resource");

      slot = getWritableSlot(pos);
      if (OSS_UNLIKELY(NULL == slot))
      {
         PD_LOG(PDERROR, "failed to get writable slot ptr");
         rc = SDB_VESSEL_INTERNAL_ERR;
         goto error;
      }

      if (ck.hasSuffix())
      {
         suffixOffset = getKeyDataOffsetToWrite(head, suffixSize);
         rc = nodeSlice.write(suffixOffset, suffixSize, ck.getSuffix().data());
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to write ptr[%d, %d], rc:%d",
                   suffixOffset, suffixSize, rc);
            goto error;
         }
      }

      if (pos < head->totalSlotCount)
      {
         UINT32 moveSize = (head->totalSlotCount - pos) * BTREE_NODE_SLOT_SIZE;
         ossMemmove(slot + 1, slot, moveSize);
      }

      /// do not goto error from here
      slot->initWhenCompressed(rid, ck.getPrefixSlotPos(), suffixOffset, suffixSize);
      ++head->totalSlotCount;
      head->totalFreeSpace -= size;
      head->freeSapceAfterLastSlot -= size;
      ++ps->referencedCnt;*/
   done:
      return rc;
   error:
      goto done;
   }

   INT32 btreeNode::recompress(BOOLEAN &recompressed)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(FALSE, "TODO");
   done:
      return rc;
   error:
      goto done;
   }

   INT32 btreeNode::findSplitPivot(BOOLEAN idleRight,
                                   RECORD_SLOT_ID &pivot)const
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(isValid(), "can not be invalid");
      SDB_ASSERT(!hasCompressedKeys(), "TODO");
      UINT32 splitSize = 0;
      UINT32 scanned = 0;
      const btreeNodePageHead *head = NULL;
      static const UINT32 _MIN_SPLIT_ITEM_COUNT = 3;
      UINT32 factor = idleRight ? 10 : 2;

      pivot = INVALID_RECORD_SLOT_ID;
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

      if (OSS_UNLIKELY(INVALID_RECORD_SLOT_ID == pivot ||
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
      pivot = INVALID_RECORD_SLOT_ID;
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
      RECORD_SLOT_ID pivot = INVALID_RECORD_SLOT_ID;
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
                            !rid.valid()))
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
      item.fini();

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
                   _ic->getIndexID(), _buffer->getLogicalPid(), rc);
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
                   _ic->getIndexID(), rightNode, rc);
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

   INT32 btreeNode::_splitAndCompact(RECORD_SLOT_ID pivot,
                                     PAGE_ID &rightNode)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(isValid(), "can not be invalid");
      SDB_ASSERT(INVALID_RECORD_SLOT_ID != pivot, "can not be invalid");

      memoryBlock mb;
      btreeNodePageSplitIniter initer;
      logicalPageSpace *lps = _buffer->getLogicalPageSpace();
      slice buffer;
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

      initer.set(buffer);
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

      head = _buffer->getWritableBodySlice().getWritableObjPtr<btreeNodePageHead>(0);
      ++head->splitedTimes;
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

   INT32 btreeNode::buildRightNodeWhenSplit(RECORD_SLOT_ID begin,
                                            slice &node)const
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(isValid(), "can not be invalid");
      SDB_ASSERT(INVALID_RECORD_SLOT_ID != begin, "can not be invalid");
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

      newHead->clLogicalID = head->clLogicalID;
      newHead->indexId = head->indexId;
      newHead->totalFreeSpace = getNodeSize() - BTREE_NODE_PAGE_HEAD_SIZE;
      newHead->freeSapceAfterLastSlot = newHead->totalFreeSpace;
      newHead->rightChild = head->rightChild;
      newHead->transSN = head->transSN;
      newHead->transNode = head->transNode;

      for (RECORD_SLOT_ID i = begin; i < head->totalSlotCount; ++i)
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

         if (newHead->freeSapceAfterLastSlot <= item.getSavingSize())
         {
            PD_LOG(PDERROR, "not enough free space to save item");
            rc = SDB_VESSEL_NOT_ENOUGH_SPACE_IN_PAGE;
            goto error;
         }

         keyOffset = getKeyDataOffsetToWrite(newHead, item.getSavedKeyDataSize());
         rc = node.write(keyOffset, item.getSavedKeyDataSize(), item.getSavingKeyData());
         if (OSS_UNLIKELY(SDB_OK != rc))
         {
            PD_LOG(PDERROR, "failed to write slice[%d,%d], rc:%d",
                   keyOffset, item.getSavedKeyDataSize(), rc);
            goto error;
         }

         newHead->totalFreeSpace -= item.getSavedKeyDataSize();
         newHead->freeSapceAfterLastSlot = newHead->totalFreeSpace;

         if (newHead->freeSapceAfterLastSlot < BTREE_NODE_SLOT_SIZE)
         {
            PD_LOG(PDERROR, "not enough free space to save slot");
            rc = SDB_VESSEL_NOT_ENOUGH_SPACE_IN_PAGE;
            goto error;
         }

         slot = node.getWritableObjPtr<btreeItemSlot>(frontOffset);
         if (isLeaf())
         {
            slot->initAsLeafFormat(item.getRid(), keyOffset,
                                   item.getSavedKeyDataSize(), FALSE);
         }
         else
         {
            slot->initAsNonLeafFormat(item.getRid(), keyOffset,
                                      item.getSavedKeyDataSize(),
                                      item.getSlot().data.nlf.leftChild,
                                      FALSE);
         }
         newHead->totalFreeSpace -= BTREE_NODE_SLOT_SIZE;
         newHead->freeSapceAfterLastSlot = newHead->totalFreeSpace;
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

         updateTransID();
      }
      return;
   }

   void btreeNode::updateTransID()
   {
      SDB_ASSERT(isValid() && _buffer->isWritable(), "must be writable");
      DPS_TRANS_ID transID = _buffer->getContext()->getTransIDWithoutTag();
      if (transID.isValid())
      {
         btreeNodePageHead *head = _buffer->getWritableBodySlice().
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
      slice compactionBuffer;
      UINT32 frontOffset = BTREE_NODE_PAGE_HEAD_SIZE;
      UINT32 backOffset = getNodeSize();
      btreeNodePageHead *head = NULL;
      const btreeNodePageHead *rh = getReadableHead();
      BOOLEAN releaseExtPage = FALSE;
      slice bodySlice = getReadableSlice();
      slice writableSlice;


      if (getReadableHead()->totalFreeSpace ==
          getReadableHead()->freeSapceAfterLastSlot)
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

      rc = _buffer->autoGetWritableBodySlice(writableSlice);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to prepare to write:%d", rc);
         goto error;
      }

      head = compactionBuffer.getWritableObjPtr<btreeNodePageHead>(0);
      ossMemcpy(head, rh, BTREE_NODE_PAGE_HEAD_SIZE);

      for (RECORD_SLOT_ID i = 0; i < rh->totalSlotCount; ++i)
      {
         btreeIndexItem item;
         const btreeItemSlot *slot = getReadableSlot(i);
         SDB_ASSERT(!slot->isKeyCompressed(), "TODO");
         const CHAR *keyData = NULL;
         UINT32 keySize = 0;
         btreeItemSlot newSlot;

         if (slot->isKeyInExtPage())
         {
            SDB_ASSERT(!isLeaf(), "can not be leaf");
            if ((slot->data.key.size + reserved) <= rh->totalFreeSpace)
            {
               rc = getItem(i, item);
               if (SDB_OK != rc)
               {
                  PD_LOG(PDERROR, "failed to get external key at[%d], rc:%d",
                         i, rc);
                  goto error;
               }

               keySize = item.getSavedKeyDataSize();
               keyData = item.getSavingKeyData();
               releaseExtPage = TRUE;
            }
            else
            {
               SDB_ASSERT(FALSE, "should always split node first when has ext key");
            }
         }
         else
         {
            keySize = slot->data.key.size;
            keyData = bodySlice.getReadablePtr(slot->data.key.offset,
                                               slot->data.key.size);
            SDB_ASSERT(NULL != keyData && 0 < keySize, "can not be invalid");
         }

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
               newSlot.initAsLeafFormat(recordID(slot->ridPage, slot->ridSlot),
                                        backOffset, keySize, FALSE);
            }
            else
            {
               newSlot.initAsNonLeafFormat(recordID(slot->ridPage, slot->ridSlot),
                                           backOffset, keySize,
                                           slot->data.nlf.leftChild, FALSE);
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
      head->freeSapceAfterLastSlot = head->totalFreeSpace;

      if (releaseExtPage)
      {
         PAGE_ID lpid = head->externalKeyPage;
         head->externalKeyPage = INVALID_PAGE_ID;
         rc = _buffer->getLogicalPageSpace()->releasePage(_buffer->getContext(), lpid);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to release external key page[%d], rc:%d", lpid, rc);
            rc = SDB_OK;
         }
      }

      ossMemcpy(writableSlice.getWPtr(), compactionBuffer.data(), compactionBuffer.getSize());
      SDB_ASSERT(!hasExternalKey(), "ext key should always be compacted so far");
   done:
      return rc;
   error:
      goto done;
   }

   INT32 btreeNode::truncate(RECORD_SLOT_ID max)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(isValid() && _buffer->isWritable(), "can not be invalid");
      SDB_ASSERT(INVALID_RECORD_SLOT_ID != max, "can not be invalid");
      UINT32 size = 0;
      btreeNodePageHead *head = _buffer->getWritableBodySlice().getWritableObjPtr<btreeNodePageHead>(0);
      if (NULL == head)
      {
         PD_LOG(PDERROR, "failed to get writable head ptr");
         rc = SDB_VESSEL_INTERNAL_ERR;
         goto error;
      }

      SDB_ASSERT((UINT32)(max + 1) < head->totalSlotCount, "invalid truncate range");
      SDB_ASSERT(!hasCompressedKeys(), "TODO");

      for (RECORD_SLOT_ID i = head->totalSlotCount - 1; i > max; --i)
      {
         const btreeItemSlot *slot = getReadableSlot(i);
         size += BTREE_NODE_SLOT_SIZE;
         if (slot->isKeyInExtPage())
         {
            SDB_ASSERT(!isLeaf(), "should not be leaf");
            SDB_ASSERT(INVALID_PAGE_ID != head->externalKeyPage, "impossible");
            rc = _buffer->getLogicalPageSpace()
                 ->releasePage(_buffer->getContext(), head->externalKeyPage);
            if (SDB_OK != rc)
            {
               PD_LOG(PDERROR, "failed to release external key page[%d], rc:%d",
                      head->externalKeyPage, rc);
               rc = SDB_OK;
            }
            head->externalKeyPage = INVALID_PAGE_ID;
         }
         else
         {
            size += slot->data.key.size;
         }
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
      SDB_ASSERT(!buffer.isValid(), "can not be valid");

      btreeNode node;
      requestContext *context = _buffer->getContext();
      logicalPageSpace *lps = _buffer->getLogicalPageSpace();
      ossSharedLatchMode mode(OSS_SHARED_LATCH_MODE_ENUM_EXCLUSIVE);

      INT32 rc = lps->getLogicalPageBuffer(context, right, mode, buffer);
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

      node = btreeNode(&buffer, _depth, _ic);

   done:
      return node;
   error:
      buffer.fini();
      goto done;
   }

   INT32 btreeNode::split(btreeSplitRaisedKey &raisedKey)
   {
      INT32 rc = SDB_OK;
      RECORD_SLOT_ID pivot = INVALID_RECORD_SLOT_ID;
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

      if (isRecentWriteOrdered() && !hasExternalKey())
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
      item.fini();

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
      RECORD_SLOT_ID pivot = INVALID_RECORD_SLOT_ID;
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
          isRecentWriteOrdered() &&
          !hasExternalKey())
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
      item.fini();

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
                   _ic->getIndexID(), rightNode, rc);
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

   INT32 btreeNode::exchangeWithNewRoot(btreeNode &newRoot)
   {
      INT32 rc = SDB_OK;
      UINT32 splitedTimes = 0;
      memoryBlock mb;
      UINT32 nodeSize = 0;
      btreeNodePageHead *currentPageHead = NULL;
      btreeNodePageHead *newRootHead = NULL;

      if (OSS_UNLIKELY(!isValid() || !newRoot.isValid()))
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }
      else if (!isRoot() || !newRoot.isRoot())
      {
         rc = SDB_VESSEL_OPERATOION_NOT_PERMITTED;
         goto error;
      }
      else if (!_buffer->isWritable() || !newRoot._buffer->isWritable())
      {
         rc = SDB_VESSEL_OPERATOION_NOT_PERMITTED;
         goto error;
      }

      SDB_ASSERT(1 == newRoot.getItemCount(), "must be one");
      SDB_ASSERT(0 == newRoot.getSplitedTimes(), "must be 0");
      SDB_ASSERT(_buffer->getLogicalPid() == newRoot.getReadableSlot(0)->data.nlf.leftChild,
                 "must be same");
      SDB_ASSERT(getReadableHead()->clLogicalID == newRoot.getReadableHead()->clLogicalID,
                  "must be same");
      SDB_ASSERT(getReadableHead()->indexId == newRoot.getReadableHead()->indexId,
                 "must be same");
      splitedTimes = getReadableHead()->splitedTimes;
      SDB_ASSERT(0 < splitedTimes, "can not be zero");
      
      nodeSize = getNodeSize();
      rc = mb.reserve(nodeSize);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to reserve mb size:%d", rc);
         goto error;
      }

      ossMemcpy(mb.getBuffer(), newRoot._buffer->getReadableBodySlice().data(), nodeSize);
      ossMemcpy(newRoot._buffer->getWritableBodySlice().getWPtr(),
                _buffer->getReadableBodySlice().data(), nodeSize);
      ossMemcpy(_buffer->getWritableBodySlice().getWPtr(),
                mb.getBuffer(), nodeSize);

      /// reset current page
      currentPageHead =  _buffer->getWritableBodySlice().getWritableObjPtr<btreeNodePageHead>(0);
      currentPageHead->splitedTimes = splitedTimes;
      OSS_BIT_CLEAR(currentPageHead->flags, BTREE_NODE_FLAG_IS_LEAF);
      getWritableSlot(0)->data.nlf.leftChild = newRoot._buffer->getLogicalPid();

      /// reset new root page head
      newRootHead = newRoot._buffer->getWritableBodySlice().getWritableObjPtr<btreeNodePageHead>(0);
      newRootHead->splitedTimes = 0;
      OSS_BIT_CLEAR(newRootHead->flags, BTREE_NODE_FLAG_IS_ROOT);
   done:
      return rc;
   error:
      goto done;
   }

   INT32 btreeNode::find(RECORD_SLOT_ID low,
                         RECORD_SLOT_ID high,
                         const bson::BSONObj &prevKey,
                         INT32 fieldCountToCmpInPrev,
                         const VEC_ELE_CMP &matchEle,
                         const inclusiveVec &matchInclusive,
                         BOOLEAN exclusive,
                         BOOLEAN forward,
                         bson::BufBuilder &bb,
                         RECORD_SLOT_ID &pos)const
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(isValid(), "can not be invalid");
      SDB_ASSERT(INVALID_RECORD_SLOT_ID != low, "can not be invalid");
      SDB_ASSERT(INVALID_RECORD_SLOT_ID != high, "can not be invalid");
      SDB_ASSERT(low <= high, "can not be invalid");
      SDB_ASSERT(!hasCompressedKeys(), "TODO");

      INT32 direction = forward ? 1 : -1;
      btreeIndexItem item;
      orderingWrapper ow = _ic->getObj().getPattern().getOrdering();

      pos = INVALID_RECORD_SLOT_ID;

      INT32 l = (INT32)low;
      INT32 h = (INT32)high;
      INT32 m = 0;

      while (TRUE)
      {
         INT32 result = 0;
         ixmKey key;
         item.fini();

         if (l > h)
         {
            INT32 tmp = forward ? l : h;
            if ((INT32)low <= tmp && tmp <= (INT32)high)
            {
               pos = (RECORD_SLOT_ID)tmp;
            }
            goto done;
         }

         m = (l + h) >> 1;
         rc = getItem((RECORD_SLOT_ID)m, item);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to get item[%d], rc:%d", m, rc);
            goto error;
         }

         bb.reset();
         key.assign(item.getSavingKeyData());
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
      pos = INVALID_RECORD_SLOT_ID;
      goto done;
   }

   INT32 btreeNode::removeChild(RECORD_SLOT_ID pos)
   {
      INT32 rc = SDB_OK;
      if (OSS_UNLIKELY(INVALID_RECORD_SLOT_ID == pos))
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

   INT32 btreeNode::_removeChild(RECORD_SLOT_ID pos)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(isValid(), "can not be invalid");
      SDB_ASSERT(INVALID_RECORD_SLOT_ID != pos, "can not be invalid");
      SDB_ASSERT(!isLeaf(), "can not be leaf");

      const btreeNodePageHead *rh = getReadableHead();
      btreeNodePageHead *head = NULL;
      slice bodySlice;

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

      rc = _buffer->autoGetWritableBodySlice(bodySlice);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to get node ready to write:%d", rc);
         goto error;
      }

      rh = NULL;
      head = bodySlice.getWritableObjPtr<btreeNodePageHead>(0);
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

   INT32 btreeNode::_destroySlot(RECORD_SLOT_ID pos)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(INVALID_RECORD_SLOT_ID != pos, "can not be invalid");
      SDB_ASSERT(isValid(), "can not be invalid");
      SDB_ASSERT(getLockingMode().isExclusive(), "must be exlusive");
      SDB_ASSERT(pos < getReadableHead()->totalSlotCount, "out of bound");
      SDB_ASSERT(_buffer->isWritable(), "must be writable");

      btreeNodePageHead *head = NULL;
      btreeItemSlot *slot = getWritableSlot(pos);
      SDB_ASSERT(NULL != slot, "can not b e null");
      UINT32 size = BTREE_NODE_SLOT_SIZE;

      SDB_ASSERT(!slot->hasPrefixSlot(), "TODO");
      if (!slot->isKeyInExtPage())
      {
         size += slot->data.key.size;
      }
      else
      {
         SDB_ASSERT(INVALID_PAGE_ID != head->externalKeyPage, "can not be invalid");
         SDB_ASSERT(!isLeaf() && !isRoot(), "impossible");
         _buffer->getLogicalPageSpace()->releasePage(_buffer->getContext(),
                                                     head->externalKeyPage);
         head->externalKeyPage = INVALID_PAGE_ID;
      }

      if ((pos + 1) < head->totalSlotCount)
      {
         UINT32 moveSize = (head->totalSlotCount - pos - 1) *
                           BTREE_NODE_SLOT_SIZE;
         ossMemmove(slot, slot + 1, moveSize);
      }

      --head->totalSlotCount;
      head->freeSapceAfterLastSlot += BTREE_NODE_SLOT_SIZE;
      head->totalFreeSpace += size;

   done:
      return rc;
   error:
      goto done;
   }

   INT32 btreeNode::_nonleafRemove(RECORD_SLOT_ID pos)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(INVALID_RECORD_SLOT_ID != pos, "can not be invalid");
      SDB_ASSERT(isValid(), "can not be invalid");
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
   done:
      return rc;
   error:
      goto done;
   }
} // namespace vessel

} // namespace engine

