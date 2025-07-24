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

   Source File Name = btreeNodeBase.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

*******************************************************************************/
#include "vessel/btreeNodeBase.h"
#include "oss.h"
#include "ossLikely.hpp"
#include "pdTrace.hpp"
#include "vessel/btreeNodePage.h"
#include "vessel/memoryBlock.h"
#include "vessel/btreeContext.h"
#include "vessel/prefixGenerator.h"
#include "vessel/recordID.h"
#include <iterator>

namespace engine
{
namespace vessel
{
   constexpr UINT32 _ORDERED_W_FACTOR = 16;

   btreeNodeBase::btreeNodeBase(PAGE_ID nodeId,
                                UINT32 depth,
                                btreeContext *ctx,
                                const strictBuffer &buffer):
   _nodeId(nodeId),
   _depth(depth),
   _ctx(ctx),
   _buffer(buffer)
   {
      SDB_ASSERT(INVALID_PAGE_ID != _nodeId, "can not be invalid");
      SDB_ASSERT(nullptr != _ctx, "can not be invalid");
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
      BOOLEAN r = size <= _getReadableHead()->totalFreeSpace;
      if (r && nullptr != compaction)
      {
         *compaction = _getContinuousFreeSpace() < size;
      }
      return r;
   }

   BOOLEAN btreeNodeBase::betterToActiveCompression()const
   {
      SDB_ASSERT(isValid(), "can not be invalid");
      const btreeNodePageHead *header = _getReadableHead();
      return header->isLeaf() &&
             !header->isVainPrefixRegen() &&
             hitHighWaterMark();
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

   UINT16 btreeNodeBase::getPrefixCount()const
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

   INT32 btreeNodeBase::getOwnedEntry(RECORD_SLOT_POS pos,
                                      btreeKeyStringEntry &entry)const
   {
      INT32 rc = SDB_OK;
      entry.reset();

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
      else
      {
         const btreeItemSlot * slot = _getReadableSlot(pos); 
         if (slot->isMarkedDeleted())
         {
            PD_LOG(PDERROR, "item[%d] is marked deleted", pos);
            rc = SDB_VESSEL_IXM_ITEM_NOT_FOUND;
            goto error;
         }
         else if (slot->isKeyCompressed())
         {
            prefixedKeyString pks = _getPrefixedKeyString(pos);
            if (OSS_UNLIKELY(!pks.isValid()))
            {
               PD_LOG(PDERROR, "invalid entry found at pos[%d]", pos);
               rc = SDB_VESSEL_INTERNAL_ERR;
               goto error;
            }
            else
            {
               rc = entry.moveFrom(pks.getOwnedKeyString());
               if (OSS_UNLIKELY(SDB_OK != rc))
               {
                  PD_LOG(PDERROR, "failed to get owned entry at pos[%d], rc:%d", pos, rc);
                  goto error;
               }
            }
         }
         else
         {
            _itemRef ref = _getItemRef(pos);
            if (OSS_UNLIKELY(!ref.isValid()))
            {
               rc = SDB_VESSEL_INTERNAL_ERR;
               PD_LOG(PDERROR, "failed to get item[%d], rc:%d", pos, rc);
               SDB_ASSERT(FALSE, "can not be invalid");
               goto error;
            }
            keyString k(ref.data);
            if (OSS_LIKELY(k.isValid()))
            {
               rc = k.getOwned();
               if (SDB_OK != rc)
               {
                  PD_LOG(PDERROR, "failed to get key string owned:%d", rc);
                  goto error;
               }     
               rc = entry.moveFrom(std::move(k));
               if (OSS_UNLIKELY(SDB_OK != rc))
               {
                  PD_LOG(PDERROR, "failed to get owned entry at pos[%d], rc:%d", pos, rc);
                  goto error;
               }
            }
            else
            {
               PD_LOG(PDERROR, "invalid entry found at pos[%d]", pos);
               rc = SDB_VESSEL_INTERNAL_ERR;
               goto error;
            }
         }
      }
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
         rc = SDB_INVALID_OPERATION;
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
         rc = SDB_INVALID_OPERATION;
         goto error;
      }
      else if (nullptr == _getTreeCtx())
      {
         PD_LOG(PDERROR, "tree context is invalid");
         rc = SDB_INVALID_OPERATION;
         goto error;
      }

      rc = _locate(entry, res);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to locate key and rid:%d", rc);
         goto error;
      }
      else if (res.isIdentical())
      {
         /// identical key should never be inserted.
         PD_LOG(PDERROR, "identical entry found at[%d]", res.getPos());
         rc = SDB_IXM_IDENTICAL_KEY;
         goto error;
      }
      else if (res.isUpperBound() && _isRecentWriteOrdered())
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

      if (!idleRight)
      {
         if (hasPrefixes())
         {
            rc = _compactWhenHasPrefixes();
         }
         else
         {
            rc = _compact();
         }
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to compact node:%d", rc);
            goto error;
         }
      }

      /// do not goto error from here
      raisedKey.leftChild = getNodeId();
      raisedKey.rightChild = rightNode->getNodeId();
      raisedKey.fromLeaf = TRUE;
      raisedKey.transID = _getReadableHead()->transID;
      
      if (res.getPos() <= pivot)
      {
         rc = _leafInsert(entry, transID, res.getPos());
         if (SDB_OK != rc)
         {
            PD_LOG(PDSEVERE, "failed to insert key after node[%d] split:%d",
                  getNodeId(), rc);
            ossPanic();
            goto error;
         }
      }
      else
      {
         rc = rightNode->_leafInsert(entry, transID, res.getPos() - pivot - 1);
         if (SDB_OK != rc)
         {
            PD_LOG(PDSEVERE, "failed to insert key into right node[%d]:%d",
                   rightNode->getNodeId(), rc);
            ossPanic();
            goto error;
         }
      }
      _getTreeCtx()->statsAddLeafNode(
          rightNode->_getReadableHead()->prefixCount != 0);

   done:
      return rc;
   error:
      raisedKey.reset();
      /// always panic if right node created.
      /// do not need to rollback rightNode.
      goto done;
   }

   INT32 btreeNodeBase::_getWholeItems(ossPoolVector<slice> &elementsPartRefs,
                                       ossPoolVector<keyString> &items,
                                       INT32 boundaryOffset,
                                       UINT64 &itemsTotalSize) const
   {
      INT32 rc = SDB_OK;
      const btreeNodePageHead *head = _getReadableHead();
      elementsPartRefs.clear();
      items.clear();
      elementsPartRefs.reserve(head->totalSlotCount);
      items.reserve(head->totalSlotCount);
      itemsTotalSize = 0;
      if (hasCompressedItems())
      {
         for (UINT32 i = boundaryOffset; i < head->totalSlotCount; ++i)
         {
            prefixedKeyString pks = _getPrefixedKeyString(i);
            if (!pks.isValid())
            {
               rc = SDB_VESSEL_INTERNAL_ERR;
               PD_LOG(PDERROR, "failed to get prefixed key string");
               goto error;
            }
            if (pks.hasPrefix())
            {
               items.push_back(pks.getOwnedKeyString());
            }
            else
            {
               items.emplace_back(pks.getSuffix());
            }
            elementsPartRefs.push_back(items.back().getKeyElementsSlice());
            itemsTotalSize += items.back().getRawDataSize();
         }
      }
      else
      {
         for (UINT32 i = 0; i < head->totalSlotCount; ++i)
         {
            _itemRef ref = _getItemRef(i);
            if (OSS_UNLIKELY(!ref.isValid()))
            {
               rc = SDB_VESSEL_INTERNAL_ERR;
               PD_LOG(PDERROR, "failed to get item[%d], rc:%d", i, rc);
               SDB_ASSERT(FALSE, "can not be invalid");
               goto error;
            }
            items.emplace_back(ref.data);
            elementsPartRefs.push_back(items.back().getKeyElementsSlice());
            itemsTotalSize += items.back().getRawDataSize();
         }
      }
      itemsTotalSize += head->totalSlotCount * BTREE_NODE_SLOT_SIZE;
   done:
      return rc;
   error:
      goto done;
   }

   INT32 btreeNodeBase::_getLastKeptPrefixPosWhenRecompress(
       RECORD_SLOT_POS &lastKeptPrefixPos, UINT64 &optimizedSize) const
   {
      INT32 rc = SDB_OK;
      RECORD_SLOT_POS lastHigh = 0;
      optimizedSize = 0;
      for (UINT16 i = 0; i < _getReadableHead()->prefixCount; ++i)
      {
         const btreeNodePrefixSlot *prefixSlot = _getReadablePrefixSlot(i);
         if (OSS_UNLIKELY(nullptr == prefixSlot))
         {
            rc = SDB_VESSEL_INTERNAL_ERR;
            PD_LOG(PDERROR, "failed to get prefix[%d], rc:%d", i, rc);
            SDB_ASSERT(FALSE, "can not be invalid");
            goto error;
         }
         if (lastHigh == prefixSlot->low  &&
             BTREE_NODE_PREFIX_SLOT_SIZE <=prefixSlot->getOptimizedSize())
         {
            lastHigh = prefixSlot->high;
            lastKeptPrefixPos = i;
            optimizedSize += prefixSlot->getOptimizedSize();
         }
         else
         {
            break;
         }
      }

   done:
      return rc;
   error:
      goto done;
   }
   
   UINT64 btreeNodeBase::_getOrigTotalItemSize(RECORD_SLOT_POS begin,
                                               RECORD_SLOT_POS end) const
   {
      UINT64 sum = 0;
      for (RECORD_SLOT_POS i = begin; i < end; ++i)
      {
         const btreeItemSlot *slot = _getReadableSlot(i);
         SDB_ASSERT(nullptr != slot, "can not be invalid");
         sum += slot->data.key.size;
         if (isLeaf() && slot->isKeyCompressed())
         {
            const btreeNodePrefixSlot *prefixSlot = _getReadablePrefixSlot(slot->data.lf.prefixSlot);
            sum += prefixSlot->prefixSize;
         }
      }
      return sum;
   }

   UINT64 btreeNodeBase::_getPrefixesTotalSize() const
   {
      const btreeNodePageHead *head = _getReadableHead();
      SDB_ASSERT(nullptr != head, "can not be nullptr");
      UINT64 sum = 0;
      for (RECORD_SLOT_POS i = 0; i < head->prefixCount; ++i)
      {
         const btreeNodePrefixSlot *prefixSlot = _getReadablePrefixSlot(i);
         SDB_ASSERT(nullptr != prefixSlot, "can not be nullptr");
         sum += prefixSlot->prefixSize + BTREE_NODE_PREFIX_SLOT_SIZE;
      }
      return sum;
   }

   prefixGenerator::result btreeNodeBase::_generatePrefixes(
       const ossPoolVector<slice> &elementsPartRefs,
       const INT32 boundaryOffset) const
   {
      SDB_ASSERT(isLeaf(), "should be called on leaf node");
      prefixGenerator pg;
      pg.setOptions({BTREE_NODE_PREFIX_SLOT_SIZE,
                     prefixGenerator::options::DEFAULT_MAX_TREE_DEPTH,
                     prefixGenerator::options::DEFAULT_COMBINED_WEIGHT_FACTOR,
                     FALSE,
                     boundaryOffset});
      return pg.generate(elementsPartRefs.cbegin(),
                         elementsPartRefs.cend());
   }

   INT32 btreeNodeBase::_buildNodeWhenRecompress(strictBuffer &writableBuffer,
       RECORD_SLOT_POS lastKeptPrefixPos,
       RECORD_SLOT_POS boundaryOffset,
       const ossPoolVector<keyString> &items,
       const ossPoolVector<prefixGenerator::prefixItem> &prefixes,
       UINT64 &realTotalSize,
       UINT64 &optimizedBytes)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(isLeaf(), "should be called on leaf node");
      const btreeNodePageHead *oldHead = _getReadableHead();
      UINT32 frontPrefixesOffset = BTREE_NODE_PAGE_HEAD_SIZE;
      UINT32 usefulCount = 0;
      for (const auto x : prefixes)
      {
         if (x.worthToUse(BTREE_NODE_PREFIX_SLOT_SIZE))
         {
            ++usefulCount;
         }
      }
      UINT32 frontItemsOffset =
          frontPrefixesOffset + (lastKeptPrefixPos + 1 + usefulCount) *
                                    BTREE_NODE_PREFIX_SLOT_SIZE;
      btreeNodePageHead *wHead = nullptr;
      realTotalSize = 0;
      optimizedBytes = 0;
      if (!writableBuffer.isWritable())
      {
         rc = SDB_INVALIDARG;
         PD_LOG(PDERROR, "buffer must be writable");
         goto error;
      }
      wHead = writableBuffer.getWritableObjPtr<btreeNodePageHead>(0);
      *wHead=*oldHead;
      wHead->backOffset = writableBuffer.getSize();
      wHead->compressedItemCount = 0;
      wHead->prefixCount = 0;
      wHead->totalSlotCount = 0;
      if (INVALID_RECORD_SLOT_POS != lastKeptPrefixPos)
      {
         UINT64 keptRealTotalSize = 0;
         UINT64 keptOptimizedSize = 0;
         rc = _copyKeptPrefixesAndItems(writableBuffer,
                                        lastKeptPrefixPos,
                                        frontItemsOffset,
                                        keptRealTotalSize,
                                        keptOptimizedSize);
         if (OSS_UNLIKELY(SDB_OK != rc))
         {
            PD_LOG(PDERROR, "failed to copy kept prefixes and items");
            goto error;
         }
         realTotalSize += keptRealTotalSize;
         optimizedBytes += keptOptimizedSize;
         frontPrefixesOffset += (lastKeptPrefixPos + 1) * BTREE_NODE_PREFIX_SLOT_SIZE;
         frontItemsOffset += boundaryOffset * BTREE_NODE_SLOT_SIZE;
      }

      for (auto it = prefixes.begin(); it != prefixes.end(); ++it)
      {
         if (it->worthToUse(BTREE_NODE_PREFIX_SLOT_SIZE))
         {
            UINT32 prefixSlotOffset = frontPrefixesOffset;
            btreeNodePrefixSlot *prefixSlot =
                writableBuffer.getWritableObjPtr<btreeNodePrefixSlot>(
                    prefixSlotOffset);
            prefixSlot->low = it->low;
            prefixSlot->high = it->high;
            prefixSlot->prefixSize = it->prefix.size();
            if (wHead->backOffset < frontItemsOffset + prefixSlot->prefixSize)
            {
               PD_LOG(PDERROR, "not enough free space to insert prefix");
               SDB_ASSERT(FALSE, "impossible");
               rc = SDB_VESSEL_NOT_ENOUGH_FREE_RESOURCE;
               goto error;
            }
            wHead->backOffset -= prefixSlot->prefixSize;
            rc = writableBuffer.write(
                wHead->backOffset, prefixSlot->prefixSize, it->prefix.data());
            if (OSS_UNLIKELY(SDB_OK != rc))
            {
               PD_LOG(PDERROR, "failed to copy key data:%d", rc);
               SDB_ASSERT(FALSE, "impossible");
               goto error;
            }
            prefixSlot->prefixOffset = wHead->backOffset;
            frontPrefixesOffset += BTREE_NODE_PREFIX_SLOT_SIZE;
            realTotalSize += it->prefix.size() + BTREE_NODE_PREFIX_SLOT_SIZE;
            optimizedBytes += it->savedBytesWithExtra(BTREE_NODE_PREFIX_SLOT_SIZE);
            ++wHead->prefixCount;
         }
         for (RECORD_SLOT_POS i = it->low; i < it->high; ++i)
         {
            RECORD_SLOT_POS pos = i - boundaryOffset;
            slice itemSlice;
            if (it->worthToUse(BTREE_NODE_PREFIX_SLOT_SIZE))
            {
               itemSlice.reset(items[pos].getRawDataSize() - it->prefix.size(),
                               items[pos].getRawDataPtr() + it->prefix.size());
            }
            else
            {
               itemSlice.reset(items[pos].getRawDataSize(),
                               items[pos].getRawDataPtr());
            }
            if (wHead->backOffset <
                frontItemsOffset + BTREE_NODE_SLOT_SIZE + itemSlice.size())
            {
               PD_LOG(PDERROR, "not enough free space to insert item");
               SDB_ASSERT(FALSE, "impossible");
               rc = SDB_VESSEL_NOT_ENOUGH_FREE_RESOURCE;
               goto error;
            }
            UINT32 slotOffset = frontItemsOffset;
            wHead->backOffset -= itemSlice.size();
            rc = writableBuffer.write(
                wHead->backOffset, itemSlice.size(), itemSlice.data());
            if (OSS_UNLIKELY(SDB_OK != rc))
            {
               PD_LOG(PDERROR, "failed to copy key data:%d", rc);
               SDB_ASSERT(FALSE, "impossible");
               goto error;
            }
            btreeItemSlot *itemSlot =
                writableBuffer.getWritableObjPtr<btreeItemSlot>(slotOffset);
            if (it->worthToUse(BTREE_NODE_PREFIX_SLOT_SIZE))
            {
               itemSlot->initAsLeafFormat(wHead->backOffset,
                                          itemSlice.size(),
                                          wHead->prefixCount - 1);
               ++wHead->compressedItemCount;
            }
            else
            {
               itemSlot->initAsLeafFormat(wHead->backOffset,
                                          itemSlice.size(),
                                          INVALID_RECORD_SLOT_POS);
            }
            ++wHead->totalSlotCount;
            frontItemsOffset += BTREE_NODE_SLOT_SIZE;
            realTotalSize += itemSlice.size() + BTREE_NODE_SLOT_SIZE;
         }
      }
      wHead->totalFreeSpace = wHead->backOffset - frontItemsOffset;

   done:
      return rc;
   error:
      goto done;
   }

   INT32 btreeNodeBase::_copyKeptPrefixesAndItems(
       strictBuffer &writableBuffer,
       RECORD_SLOT_POS lastKeptPrefixPos,
       UINT32 frontItemsOffset,
       UINT64 &realTotalSize,
       UINT64 &optimizedSize) const
   {
      SDB_ASSERT(writableBuffer.isWritable(), "must be writable");
      SDB_ASSERT(isValidRecordSlotPosition(lastKeptPrefixPos),
                 "can not be invalid");
      INT32 rc = SDB_OK;
      btreeNodePageHead *wHead =
          writableBuffer.getWritableObjPtr<btreeNodePageHead>(0);
      UINT32 frontPrefixesOffset = BTREE_NODE_PAGE_HEAD_SIZE;
      realTotalSize = 0;
      optimizedSize = 0;
      for (RECORD_SLOT_POS i = 0; i <= lastKeptPrefixPos; ++i)
      {
         const _prefixRef pref = _getPrefixRef(i);
         if (OSS_UNLIKELY(!pref.isValid()))
         {
            rc = SDB_VESSEL_INTERNAL_ERR;
            PD_LOG(PDERROR, "failed to get prefix[%d], rc:%d", i, rc);
            SDB_ASSERT(FALSE, "can not be invalid");
            goto error;
         }
         UINT32 offset = frontPrefixesOffset + i * BTREE_NODE_PREFIX_SLOT_SIZE;
         btreeNodePrefixSlot *prefixSlot =
             writableBuffer.getWritableObjPtr<btreeNodePrefixSlot>(offset);
         prefixSlot->low = pref.slot->low;
         prefixSlot->high = pref.slot->high;
         prefixSlot->prefixSize = pref.data.size();
         if (wHead->backOffset <
             offset + pref.data.size() + BTREE_NODE_PREFIX_SLOT_SIZE)
         {
            PD_LOG(PDERROR, "not enough free space to insert item");
            SDB_ASSERT(FALSE, "impossible");
            rc = SDB_VESSEL_NOT_ENOUGH_FREE_RESOURCE;
            goto error;
         }
         UINT32 keyOffset = _getKeyDataOffsetToWrite(wHead, pref.data.size());
         rc = writableBuffer.write(
             keyOffset, pref.data.size(), pref.data.data());
         if (OSS_UNLIKELY(SDB_OK != rc))
         {
            PD_LOG(PDERROR, "failed to write prefix data");
            goto error;
         }
         prefixSlot->prefixOffset = keyOffset;
         wHead->backOffset -= pref.data.size();
         SDB_ASSERT(pref.slot->getOptimizedSize() >=
                        BTREE_NODE_PREFIX_SLOT_SIZE,
                    "must be valid");
         optimizedSize +=
             pref.slot->getOptimizedSize() - BTREE_NODE_PREFIX_SLOT_SIZE;
         realTotalSize += pref.data.size() + BTREE_NODE_PREFIX_SLOT_SIZE;
         ++wHead->prefixCount;
         for (RECORD_SLOT_POS j = pref.slot->low; j < pref.slot->high; ++j)
         {
            UINT32 offset = frontItemsOffset + j * BTREE_NODE_SLOT_SIZE;
            btreeItemSlot *slot =
                writableBuffer.getWritableObjPtr<btreeItemSlot>(offset);
            const _itemRef ref = _getItemRef(j);
            if (wHead->backOffset <
                offset + ref.data.size() + BTREE_NODE_SLOT_SIZE)
            {
               PD_LOG(PDERROR, "not enough free space to insert item");
               SDB_ASSERT(FALSE, "impossible");
               rc = SDB_VESSEL_NOT_ENOUGH_FREE_RESOURCE;
               goto error;
            }
            UINT32 keyOffset = _getKeyDataOffsetToWrite(wHead, ref.data.size());
            rc = writableBuffer.write(
                keyOffset, ref.data.size(), ref.data.data());
            if (OSS_UNLIKELY(SDB_OK != rc))
            {
               PD_LOG(PDERROR, "failed to write item data");
               goto error;
            }
            slot->initAsLeafFormat(keyOffset, ref.data.size(), i);
            realTotalSize += ref.data.size() + BTREE_NODE_SLOT_SIZE;
            wHead->backOffset -= ref.data.size();
            ++wHead->compressedItemCount;
            ++wHead->totalSlotCount;
         }
      }

   done:
      return rc;
   error:
      goto done;
   }

   INT64 btreeNodeBase::getCompressionOptimizedBytes(RECORD_SLOT_POS startPos) const
   {
      SDB_ASSERT(isValid(), "can not be invalid");
      const btreeNodePageHead *head = _getReadableHead();
      INT64 sum = 0;
      for (UINT32 i = startPos; i < head->prefixCount; ++i)
      {
         const btreeNodePrefixSlot *slot = _getReadablePrefixSlot(i);
         sum += static_cast<INT64>(slot->getOptimizedSize()) -
                BTREE_NODE_PREFIX_SLOT_SIZE;
      }
      return sum;
   }

   INT32 btreeNodeBase::recompress(BOOLEAN &recompressed,
                                   BOOLEAN fullyRegenerate)
   {
      INT32 rc = SDB_OK;
      recompressed = FALSE;
      SDB_ASSERT(isLeaf(), "should be called on leaf node");
      ossPoolVector<slice> elementsPartRefs;
      ossPoolVector<keyString> items;
      memoryBlock mb;
      UINT64 itemsTotalSize = 0;
      UINT64 totalOptimizedSize = 0;
      RECORD_SLOT_POS lastKeptPrefixPos = INVALID_RECORD_SLOT_POS;
      RECORD_SLOT_POS boundaryOffset = 0;
      if (OSS_UNLIKELY(!isValid()))
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }
      if (OSS_UNLIKELY(!isLeaf()))
      {
         rc = SDB_INVALID_OPERATION;
         goto error;
      }

      /// STEP:1: get index items
      if (!fullyRegenerate)
      {
         rc = _getLastKeptPrefixPosWhenRecompress(lastKeptPrefixPos,
                                                  totalOptimizedSize);
         if (OSS_UNLIKELY(SDB_OK != rc))
         {
            PD_LOG(PDERROR,
                   "failed to get last kept prefix position when recompress:%d",
                   rc);
            goto error;
         }

         if (INVALID_RECORD_SLOT_POS != lastKeptPrefixPos)
         {
            const btreeNodePrefixSlot *lastKeptPrefix =
                _getReadablePrefixSlot(lastKeptPrefixPos);
            if (OSS_UNLIKELY(nullptr == lastKeptPrefix))
            {
               rc = SDB_VESSEL_INTERNAL_ERR;
               PD_LOG(PDERROR,
                      "failed to get prefix[%d]:%d",
                      lastKeptPrefixPos,
                      rc);
               goto error;
            }
            boundaryOffset = lastKeptPrefix->high;
         }
      }

      rc = _getWholeItems(elementsPartRefs, items, boundaryOffset, itemsTotalSize);
      if (OSS_UNLIKELY(SDB_OK != rc))
      {
         PD_LOG(PDERROR, "failed to get items on node:%d", rc);
         goto error;
      }

      itemsTotalSize += _getOrigTotalItemSize(0, boundaryOffset);

      /// STEP:2: generate prefixes and validate output
      {
         prefixGenerator::result r = _generatePrefixes(elementsPartRefs, boundaryOffset);
         totalOptimizedSize += r.totalSavedSize;
         const ossPoolVector<prefixGenerator::prefixItem> &prefixes = r.prefixes;
         FLOAT64 compressionRatio = FLOAT64(totalOptimizedSize) / FLOAT64(itemsTotalSize);
         const INT64 existingOptimizedBytes = getCompressionOptimizedBytes();
         if (static_cast<INT64>(totalOptimizedSize) <= existingOptimizedBytes ||
             compressionRatio < ACCEPTABLE_COMPRESSION_RATIO ||
             static_cast<UINT32>(boundaryOffset) == getItemCount())
         {
            #ifdef DEBUG
            UINT32 compressedTotalSize =
                itemsTotalSize - r.totalSavedSize + BTREE_NODE_PAGE_HEAD_SIZE;
            if (compressedTotalSize > getNodeSize())
            {
               ossPanic();
            }
            #endif

            rc = _makeBufferWritable();
            if (SDB_OK != rc)
            {
               PD_LOG(PDERROR, "failed to make buffer writable:%d", rc);
               goto error;
            }
            else
            {
               btreeNodePageHead *head = _getWritableHead();
               OSS_BIT_SET(head->flags, BTREE_NODE_FLAG_VAIN_PREFIX_REGENERATION);
            }
            
            goto done;
         }
      
      /// STEP:3: build the new page
      
         rc = mb.reserve(getNodeSize());
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to reserve memory block:%d", rc);
            goto error;
         }
         strictBuffer newBuf;
         newBuf.makeWritable(mb.getCapacity(), mb.getBuffer());
         UINT64 realTotalEntrySize = 0;
         UINT64 optimizedBytes = 0;
         rc = _buildNodeWhenRecompress(newBuf,
                                       lastKeptPrefixPos,
                                       boundaryOffset,
                                       items,
                                       prefixes,
                                       realTotalEntrySize,
                                       optimizedBytes);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to build new node page:%d", rc);
            goto error;
         }

         /// STEP:4: write the new page
         const UINT32 oldCompressedEntryNum =
             _getReadableHead()->compressedItemCount;
         const UINT64 oldRealTotalEntrySize =
             itemsTotalSize - existingOptimizedBytes;
         const UINT32 oldPrefixCount = getPrefixCount();
         // SDB_ASSERT(realTotalEntrySize + totalOptimizedSize == itemsTotalSize,
         //            "must be equal");
         rc = _makeBufferWritable();
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to make buffer writable:%d", rc);
            goto error;
         }
         rc = _buffer.write(0, newBuf.getSize(), newBuf.getRPtr());
         if (OSS_UNLIKELY(SDB_OK != rc))
         {
            PD_LOG(PDERROR, "failed to write new page:%d", rc);
            goto error;
         }
         const btreeNodePageHead *head = _getReadableHead();
         SDB_ASSERT(nullptr != head, "must not be nullptr");
         _getTreeCtx()->statsRecompress(head->compressedItemCount,
                                        realTotalEntrySize,
                                        head->prefixCount,
                                        oldCompressedEntryNum,
                                        oldRealTotalEntrySize,
                                        oldPrefixCount);
      }
      recompressed = TRUE;
      
      done:
         return rc;
      error:
         goto done;
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
      SDB_ASSERT(!isLeaf(), "can not be leaf");

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
         rc = SDB_INVALID_OPERATION;
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
         else if (res.isIdentical())
         {
            /// should never found an identical raised key even it is marked deleted. 
            PD_LOG(PDERROR, "identical entry found at[%d]", res.getPos());       
            rc = SDB_IXM_IDENTICAL_KEY;
            goto error;
         }
         insertPos = res.getPos();
      }
      else
      {
         insertPos = 0;
      }

      appendOnly = (insertPos == head->totalSlotCount);
      head = nullptr;

      if (needCompact)
      {
         if (hasPrefixes())
         {
            rc = _compactWhenHasPrefixes();
         }
         else
         {
            rc = _compact();
         }
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
         rc = SDB_INVALID_OPERATION;
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
         rc = SDB_INVALID_OPERATION;
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
         rc = SDB_INVALID_OPERATION;
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
         rc = SDB_IXM_IDENTICAL_KEY;
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
         rc = SDB_INVALID_OPERATION;
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
         rc = SDB_INVALID_OPERATION;
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
         rc = SDB_INVALID_OPERATION;
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
      else if (res.isIdentical())
      {
         /// should never found an identical raised key even it is marked deleted. 
         PD_LOG(PDERROR, "identical entry found at[%d]", res.getPos());       
         rc = SDB_IXM_IDENTICAL_KEY;
         goto error;
      }

      if (res.getPos() == _getReadableHead()->totalSlotCount &&
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
      _ctx->statsAddNonLeafNode();
      /// non-leaf node will never be compressed,
      /// do not compact it.

      /// should not get error from here
      if (res.getPos() <= pivot)
      {
         rc = _insertRaisedKey(raisedKey, res.getPos());
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to insert raised key to current node:%d", rc);
            ossPanic();
            goto error;
         }
      }
      else
      {
         rc = rightNode->_insertRaisedKey(raisedKey, res.getPos() - pivot - 1);
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
         else if (res.isIdentical())
         {
            PD_LOG(PDERROR, "identical key found at[%d]", res.getPos());
            rc = SDB_IXM_IDENTICAL_KEY;
            goto error;
         }

         toInsert = res.getPos();
      }
      else
      {
         toInsert = 0;
      }

      if (needCompact)
      {
         if (hasPrefixes())
         {
            rc = _compactWhenHasPrefixes();
         }
         else
         {
            rc = _compact();
         }
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to compact node:%d", rc);
            goto error;
         }
      }

      if (hasPrefixes())
      {
         RECORD_SLOT_POS prefixPos = INVALID_RECORD_SLOT_POS;
         rc = _pickPrefix(entry, toInsert, prefixPos);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to pick prefix:%d", rc);
            goto error;
         }
         if (INVALID_RECORD_SLOT_POS != prefixPos)
         {
            rc = _insertWithPrefix(entry, toInsert, prefixPos);
            if (SDB_OK != rc)
            {
               PD_LOG(PDERROR, "failed to insert to node with prefix:%d", rc);
               goto error;
            }
         }
         else
         {
            rc = _insert(toInsert, entry);
            if (SDB_OK != rc)
            {
               PD_LOG(PDERROR, "failed to insert to node:%d", rc);
               goto error;
            }
            btreeNodePrefixSlot * prefixes = _getWritablePrefixSlot(0);
            if (OSS_UNLIKELY(nullptr == prefixes))
            {
               rc = SDB_VESSEL_INTERNAL_ERR;
               PD_LOG(PDERROR,
                     "failed to get first writable prefix slot ptr, rc:%d",
                     rc);
               goto error;
            }
            RECORD_SLOT_POS posToUpdateBound = _lowerBoundPrefixSlot(toInsert);
            _adjustPrefsixSlots(posToUpdateBound);
         }
      }
      else
      {
         rc = _insert(toInsert, entry);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to insert to node:%d", rc);
            goto error;
         }
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
      SDB_ASSERT(!hasCompressedItems(), "must have no compressed key");
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
         if (OSS_UNLIKELY(!ref.isValid()))
         {
            rc = SDB_VESSEL_INTERNAL_ERR;
            PD_LOG(PDERROR, "failed to get item[%d], rc:%d", i, rc);
            SDB_ASSERT(FALSE, "can not be invalid");
            goto error;
         }

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
      if ((0 != OSS_BIT_TEST(head->flags,
                             BTREE_NODE_FLAG_VAIN_PREFIX_REGENERATION)) &&
          !hitHighWaterMark())
      {
         OSS_BIT_CLEAR(head->flags, BTREE_NODE_FLAG_VAIN_PREFIX_REGENERATION);
      }
      
      _buffer.write(0, compactionBuffer.getSize(), compactionBuffer.getRPtr());
   done : 
   return rc;
   error:
      goto done;
   }

   INT32 btreeNodeBase::_compactWhenHasPrefixes()
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(isValid(), "can not be invalid");
      SDB_ASSERT(hasPrefixes(), "must have prefixes");
      memoryBlock mb;
      strictBuffer compactionBuffer;
      UINT32 frontOffset = BTREE_NODE_PAGE_HEAD_SIZE;
      btreeNodePageHead *wHead = nullptr;
      const btreeNodePageHead *head = _getReadableHead();
      ossPoolVector<btreeNodePrefixSlot> tempPrefixes;
      ossPoolVector<btreeItemSlot> tempItems(head->totalSlotCount);
      RECORD_SLOT_POS lastSavedPrefixPos = INVALID_RECORD_SLOT_POS;
      UINT64 prefixesSizeDiff = _getPrefixesTotalSize();
      UINT32 oldPrefixCount = head->prefixCount;
      if (head->totalFreeSpace == _getContinuousFreeSpace())
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
      wHead = compactionBuffer.getWritableObjPtr<btreeNodePageHead>(0);
      *wHead = *head;
      wHead->prefixCount = 0;
      wHead->backOffset = getNodeSize();
      wHead->totalFreeSpace = wHead->backOffset - frontOffset;

      /// STEP:1: generate temp prefixes and temp items and write key data to buffer
      for (RECORD_SLOT_POS i = 0; i < head->totalSlotCount; ++i)
      {
         _itemRef ref = _getItemRef(i);
         if (OSS_UNLIKELY(!ref.isValid()))
         {
            rc = SDB_VESSEL_INTERNAL_ERR;
            PD_LOG(PDERROR, "failed to get item[%d], rc:%d", i, rc);
            SDB_ASSERT(FALSE, "can not be invalid");
            goto error;
         }
         if (ref.slot->isKeyCompressed())
         {
            RECORD_SLOT_POS prefixPos = ref.slot->data.lf.prefixSlot;
            _prefixRef pref = _getPrefixRef(prefixPos);
            if (lastSavedPrefixPos != prefixPos)
            {
               UINT32 keyOffset = _getKeyDataOffsetToWrite(wHead, pref.data.size());
               compactionBuffer.write(keyOffset, pref.data.size(), pref.data.data());
               wHead->backOffset -= pref.data.size();
               btreeNodePrefixSlot newPrefixSlot(*pref.slot);
               newPrefixSlot.prefixOffset = keyOffset;
               tempPrefixes.push_back(newPrefixSlot);
               prefixesSizeDiff -= pref.data.size() + BTREE_NODE_PREFIX_SLOT_SIZE;
               lastSavedPrefixPos = prefixPos;
            }
            UINT32 keyOffset = _getKeyDataOffsetToWrite(wHead, ref.data.size());
            compactionBuffer.write(keyOffset, ref.data.size(), ref.data.data());
            wHead->backOffset -= ref.data.size();
            tempItems[i].initAsLeafFormat(keyOffset, ref.data.size(), tempPrefixes.size() - 1);
         }
         else
         {
            UINT32 keyOffset = _getKeyDataOffsetToWrite(wHead, ref.data.size());
            compactionBuffer.write(keyOffset, ref.data.size(), ref.data.data());
            wHead->backOffset -= ref.data.size();
            tempItems[i].initAsLeafFormat(keyOffset, ref.data.size(), INVALID_RECORD_SLOT_POS);
         }
      }

      /// STEP:2: write temp prefixes and items to buffer
      {
         btreeNodePrefixSlot *prefixesPtr =
             compactionBuffer.getWritableObjPtr<btreeNodePrefixSlot>(
                 BTREE_NODE_PAGE_HEAD_SIZE);
         std::copy(tempPrefixes.begin(), tempPrefixes.end(), prefixesPtr);
         frontOffset += tempPrefixes.size() * BTREE_NODE_PREFIX_SLOT_SIZE;
         btreeItemSlot *itemsPtr =
             compactionBuffer.getWritableObjPtr<btreeItemSlot>(
                 BTREE_NODE_PAGE_HEAD_SIZE +
                 BTREE_NODE_PREFIX_SLOT_SIZE * tempPrefixes.size());
         std::copy(tempItems.begin(), tempItems.end(), itemsPtr);
         frontOffset += tempItems.size() * BTREE_NODE_SLOT_SIZE;
         
         wHead->prefixCount = tempPrefixes.size();
         wHead->totalFreeSpace = wHead->backOffset - frontOffset;
      }

      if ((0 != OSS_BIT_TEST(wHead->flags,
                             BTREE_NODE_FLAG_VAIN_PREFIX_REGENERATION)) &&
          !hitHighWaterMark())
         {
            OSS_BIT_CLEAR(wHead->flags, BTREE_NODE_FLAG_VAIN_PREFIX_REGENERATION);
         }

         rc = _makeBufferWritable();
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to prepare to write:%d", rc);
            goto error;
         }
         _buffer.write(0, compactionBuffer.getSize(), compactionBuffer.getRPtr());
         _getTreeCtx()->statsCompact(
             prefixesSizeDiff, tempPrefixes.size(), oldPrefixCount);
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
      _getTreeCtx()->statsInsertUncompressedIndex(entry.getRawDataSize() +
                                                  BTREE_NODE_SLOT_SIZE);
   done:
      return rc;
   error:
      goto done;
   }

   INT32 btreeNodeBase::_canUsePrefixOfItem(const btreeItemSlot *slot,
                                        const slice &raw,
                                        INT16 &prefixSlotToUse,
                                        UINT32 &prefixSizeToUse) const
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(isLeaf(), "must be leaf node");
      if (slot->isKeyCompressed())
      {
         INT16 prefixSlotID = slot->data.lf.prefixSlot;
         const btreeNodePrefixSlot *prefixSlotPtr =
             _getReadablePrefixSlot(slot->data.lf.prefixSlot);
         if (OSS_UNLIKELY(nullptr == slot))
         {
            PD_LOG(PDERROR,
                   "failed to get readable prefix slot ptr[%d]",
                   prefixSlotID);
            rc = SDB_VESSEL_INTERNAL_ERR;
            goto error;
         }
         const CHAR *prefixData = _buffer.getSlice(
             prefixSlotPtr->prefixOffset, prefixSlotPtr->prefixSize).data();
         if (OSS_UNLIKELY(nullptr == prefixData))
         {
            PD_LOG(PDERROR,
                   "failed to get readable slot data ptr[%d]",
                   prefixSlotID);
            rc = SDB_VESSEL_INTERNAL_ERR;
            goto error;
         }
         if (ossMemcmp(raw.data(), prefixData, prefixSlotPtr->prefixSize) ==
                 0 &&
             prefixSlotPtr->prefixSize > prefixSizeToUse)
         {
            prefixSlotToUse = prefixSlotID;
            prefixSizeToUse = prefixSlotPtr->prefixSize;
         }
      }
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
      if (OSS_UNLIKELY(nullptr == slot))
      {
         PD_LOG(PDERROR, "failed to get writable slot ptr[%d]", pos);
         rc = SDB_VESSEL_INTERNAL_ERR;
         goto error;
      }
      size += slot->data.key.size;

      if (hasPrefixes())
      {
         btreeNodePrefixSlot *pslots = _getWritablePrefixSlot(0);
         SDB_ASSERT(nullptr != pslots && pslots->isValid(), "can not be invalid");

         if (slot->isKeyCompressed())
         {
            pslots[slot->data.lf.prefixSlot].decBounds(FALSE);
            _adjustPrefsixSlots(slot->data.lf.prefixSlot + 1, FALSE);
            --head->compressedItemCount;
            _getTreeCtx()->statsRemoveCompressedIndex(
                pslots[slot->data.lf.prefixSlot].prefixSize,
                slot->data.key.size + BTREE_NODE_SLOT_SIZE);
         }
         else
         {
            RECORD_SLOT_POS prefixPos = _upperBoundPrefixSlot(pos);
            SDB_ASSERT(isValidRecordSlotPosition(prefixPos), "can not be invalid");
            _adjustPrefsixSlots(prefixPos, FALSE);
            _getTreeCtx()->statsRemoveUncompressedIndex(slot->data.key.size +
                                                        BTREE_NODE_SLOT_SIZE);
         }
      }
      else
      {
         _getTreeCtx()->statsRemoveUncompressedIndex(slot->data.key.size +
                                                     BTREE_NODE_SLOT_SIZE);
      }

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
      _getTreeCtx()->statsRemoveMarkedDeletedIndexes(1);
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
      slice target = ks.getKeySliceAfterHeader();
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
         cmp = 0;
         INT16 step = count >> 1;
         RECORD_SLOT_POS pos = low + step;
         _itemRef ref = _getItemRef(pos);
         if (OSS_UNLIKELY(!ref.isValid()))
         {
            rc = SDB_VESSEL_INTERNAL_ERR;
            PD_LOG(PDERROR, "failed to get item[%d], rc:%d", pos, rc);
            SDB_ASSERT(FALSE, "can not be invalid");
            goto error;
         }
      
         if (ref.slot->isKeyCompressed())
         {
            prefixedKeyString pks = _getPrefixedKeyString(pos);
            if (OSS_UNLIKELY(!pks.isValid()))
            {
               rc = SDB_VESSEL_INTERNAL_ERR;
               PD_LOG(PDERROR, "failed to get prefixed item[%d], rc:%d", pos, rc);
               SDB_ASSERT(FALSE, "can not be invalid");
               goto error;
            }
            cmp = pks.compare(target);
         }
         else
         {
            btreeKeyStringEntry entry(ref.data);
            if (OSS_UNLIKELY(!entry.isValid()))
            {
               rc = SDB_VESSEL_INTERNAL_ERR;
               PD_LOG(PDERROR, "failed to transform item[%d] to entry, rc:%d", pos, rc);
               SDB_ASSERT(FALSE, "can not be invalid");
               goto error;
            }
            cmp = entry.getKeySlice().compare(target);
         }
         if (cmp < 0)
         {
            low = pos + 1;
            count -= (step + 1);
         }
         else
         {
            count = step;
         }
      }

      _initSeekResult(cmp, low, res);
   done:
      return rc;
   error:
      res.reset();
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
      INT32 cmp = 0;
      slice target = ks.getKeySlice();

      res.reset();
      while (0 < count)
      {
         cmp = 0;
         INT16 step = count >> 1;
         RECORD_SLOT_POS pos = low + step;
         _itemRef ref = _getItemRef(pos);
         if (OSS_UNLIKELY(!ref.isValid()))
         {
            rc = SDB_VESSEL_INTERNAL_ERR;
            PD_LOG(PDERROR, "failed to get item[%d], rc:%d", pos, rc);
            SDB_ASSERT(FALSE, "can not be invalid");
            goto error;
         }

         if (ref.slot->isKeyCompressed())
         {
            prefixedKeyString pks = _getPrefixedKeyString(pos);
            if (OSS_UNLIKELY(!pks.isValid()))
            {
               rc = SDB_VESSEL_INTERNAL_ERR;
               PD_LOG(PDERROR, "failed to get prefixed item[%d], rc:%d", pos, rc);
               SDB_ASSERT(FALSE, "can not be invalid");
               goto error;
            }
            cmp = pks.compare(target);
         }
         else
         {
            btreeKeyStringEntry entry(ref.data);
            if (OSS_UNLIKELY(!entry.isValid()))
            {
               rc = SDB_VESSEL_INTERNAL_ERR;
               PD_LOG(PDERROR, "failed to transform item[%d] to entry, rc:%d", pos, rc);
               SDB_ASSERT(FALSE, "can not be invalid");
               goto error;
            }
            cmp = entry.getKeySlice().compare(target);
         }
         if (cmp < 0)
         {
            low = pos + 1;
            count -= (step + 1);
         }
         else
         {
            count = step;
         }
      }

      _initSeekResult(cmp, low, res);

   done:
      return rc;
   error:
      res.reset();
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
                 (UINT16)pos <= _getReadableHead()->totalSlotCount, "invalid pos");
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

   btreeNodeBase::_prefixRef btreeNodeBase::_getPrefixRef(RECORD_SLOT_POS pos)const
   {
      const btreeNodePrefixSlot *slot = _getReadablePrefixSlot(pos);
      if (OSS_LIKELY(nullptr != slot))
      {
         return _prefixRef(slot, _buffer.getSlice(slot->prefixOffset,
                                                  slot->prefixSize));
      }
      else
      {
         return _prefixRef();
      }
   }

   prefixedKeyString btreeNodeBase::_getPrefixedKeyString(RECORD_SLOT_POS pos) const
   {
      _itemRef ref = _getItemRef(pos);
      SDB_ASSERT(ref.isValid(), "can not be invalid");
      if (isLeaf() && ref.slot->isKeyCompressed())
      {
         RECORD_SLOT_POS prefixPos = ref.slot->data.lf.prefixSlot;
         SDB_ASSERT(isValidRecordSlotPosition(prefixPos), "can not be invalid");
         _prefixRef pref = _getPrefixRef(prefixPos);
         return prefixedKeyString(ref.data, pref.data);
      }
      else
      {
         return prefixedKeyString(ref.data);
      }
   }

   UINT32 btreeNodeBase::_getContinuousFreeSpace()const
   {
      return _getBackOffset() - _getFrontOffset();
   }

   UINT32 btreeNodeBase::_getFrontOffset()const
   {
      const btreeNodePageHead *head = _getReadableHead();
      return BTREE_NODE_PAGE_HEAD_SIZE +
             (head->totalSlotCount * BTREE_NODE_SLOT_SIZE) +
             (head->prefixCount * BTREE_NODE_PREFIX_SLOT_SIZE);
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
      UINT32 splitSize = 0;
      UINT32 scanned = 0;
      const btreeNodePageHead *head = nullptr;
      static const UINT32 _MIN_SPLIT_ITEM_COUNT = 3;
      UINT32 factor = idleRight ? 10 : 2;
      INT32 lastPrefixSlotPos = -1;

      pivot = INVALID_RECORD_SLOT_POS;
      head = _getReadableHead();
      if (head->totalSlotCount < _MIN_SPLIT_ITEM_COUNT)
      {
         PD_LOG(PDERROR, "too few item count to split");
         rc = SDB_INVALID_OPERATION;
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

         savingSize = getSizeToSaveInNode(slot->data.key.size);
         if (slot->isKeyCompressed() &&
             lastPrefixSlotPos != slot->data.lf.prefixSlot)
         {
            savingSize +=
                _getReadablePrefixSlot(slot->data.lf.prefixSlot)->prefixSize;
            savingSize += BTREE_NODE_PREFIX_SLOT_SIZE;
            lastPrefixSlotPos = slot->data.lf.prefixSlot;
         }
         
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
      SDB_ASSERT(isValidRecordSlotPosition(pos), "can not be invalid");
      entry.reset();
      prefixedKeyString pks = _getPrefixedKeyString(pos);
      SDB_ASSERT(pks.isValid(), "can not be invalid");
      if (pks.hasPrefix())
      {
         rc = entry.moveFrom(pks.getOwnedKeyString());
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to init key entry at pos[%d], rc:%d", pos, rc);
            goto error;
         }
      }
      else
      {
         rc = entry.init(pks.getSuffix());
         if (OSS_UNLIKELY(SDB_OK != rc))
         {
            PD_LOG(PDERROR, "failed to init index entry:%d", rc);
            goto error;
         }

         rc = entry.getOwned();
         if (OSS_UNLIKELY(SDB_OK != rc))
         {
            PD_LOG(PDERROR, "failed to get owned entry:%d", rc);
            goto error;
         }
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

      if (hasCompressedItems())
      {
         rc = _buildCompressedRightNode(pivot + 1, *rightNode);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to build right node:%d", rc);
            goto error;
         }
      }
      else
      {
         rc = _buildRightNode(pivot + 1, *rightNode);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to build right node:%d", rc);
            goto error;
         }
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

   INT32 btreeNodeBase::_buildCompressedRightNode(
       RECORD_SLOT_POS begin, btreeNodeBase &rightNode) const
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(isValid(), "can not be invalid");
      SDB_ASSERT(isValidRecordSlotPosition(begin), "can not be invalid");
      SDB_ASSERT(isLeaf() && hasCompressedItems(), "must already have prefixes");
      const btreeNodePageHead* head = _getReadableHead();
      btreeNodePageHead * rHead = rightNode._getWritableHead();
      SDB_ASSERT(nullptr != head && nullptr != rHead, "can not be nullptr");
      RECORD_SLOT_POS beginPrefixPos = _findFirstPrefixPosWhenSplit(begin);
      UINT32 frontOffset = BTREE_NODE_PAGE_HEAD_SIZE;
      UINT64 origTotalEntrySize = 0;

      rc = rightNode._makeBufferWritable();
      if (OSS_UNLIKELY(SDB_OK != rc))
      {
         PD_LOG(PDERROR, "failed to get node ready to write:%d", rc);
         goto error;
      }
      
      rHead->initAsRightNode(*head, rightNode.getNodeSize());

      if (!isValidRecordSlotPosition(begin))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      for (INT32 i = beginPrefixPos; i < head->prefixCount; ++i)
      {
         const btreeNodePrefixSlot *prefixSlot = _getReadablePrefixSlot(i);
         btreeNodePrefixSlot *newPrefixSlot =
               rightNode._buffer.getWritableObjPtr<btreeNodePrefixSlot>(frontOffset);
         UINT32 keyOffset =
               rightNode._getKeyDataOffsetToWrite(rHead, prefixSlot->prefixSize);
         rightNode._buffer.write(keyOffset,
                     prefixSlot->prefixSize,
                     _buffer.getReadablePtr(
                        prefixSlot->prefixOffset, prefixSlot->prefixSize));
         newPrefixSlot->prefixOffset = keyOffset;
         newPrefixSlot->prefixSize = prefixSlot->prefixSize;
         rHead->backOffset = keyOffset;
         if (i != beginPrefixPos)
         {
            newPrefixSlot->low = prefixSlot->low - begin;
         }
         else
         {
            if (prefixSlot->low < begin)
            {
               newPrefixSlot->low = 0;
            }
            else
            {
               newPrefixSlot->low = prefixSlot->low - begin;
            }
         }
         frontOffset += BTREE_NODE_PREFIX_SLOT_SIZE;
         newPrefixSlot->high = prefixSlot->high - begin;
         rHead->prefixCount += 1;
         rHead->totalFreeSpace -=
               BTREE_NODE_PREFIX_SLOT_SIZE + newPrefixSlot->prefixSize;
      }

      for (RECORD_SLOT_POS i = begin; i < head->totalSlotCount; ++i)
      {
         btreeItemSlot *slot = nullptr;
         UINT32 keyOffset = 0;
         _itemRef ref = _getItemRef(i);
         if (OSS_UNLIKELY(!ref.isValid()))
         {
            rc = SDB_VESSEL_INTERNAL_ERR;
            PD_LOG(PDERROR, "failed to get item[%d], rc:%d", i, rc);
            SDB_ASSERT(FALSE, "can not be invalid");
            goto error;
         }

         if (rHead->backOffset <
             (frontOffset + getSizeToSaveInNode(ref.data.size())))
         {
            PD_LOG(PDERROR, "not enough free space to save item");
            rc = SDB_VESSEL_NOT_ENOUGH_SPACE_IN_PAGE;
            goto error;
         }

         keyOffset = rightNode._getKeyDataOffsetToWrite(rHead, ref.data.size());
         rc = rightNode._buffer.write(keyOffset, ref.data.size(), ref.data.data());
         if (OSS_UNLIKELY(SDB_OK != rc))
         {
            PD_LOG(PDERROR, "failed to write slice[%d,%d], rc:%d",
                   keyOffset, ref.data.size(), rc);
            goto error;
         }
         rHead->backOffset = keyOffset;
         origTotalEntrySize += BTREE_NODE_SLOT_SIZE + ref.data.size();
         slot = rightNode._buffer.getWritableObjPtr<btreeItemSlot>(frontOffset);
         if (ref.slot->isKeyCompressed())
         {
            slot->initAsLeafFormat(keyOffset,
                                   ref.data.size(),
                                   ref.slot->data.lf.prefixSlot -
                                       beginPrefixPos);
            ++rHead->compressedItemCount;
            origTotalEntrySize +=
                _getReadablePrefixSlot(ref.slot->data.lf.prefixSlot)
                    ->prefixSize;
         }
         else
         {
            slot->initAsLeafFormat(
                  keyOffset, ref.data.size(), INVALID_RECORD_SLOT_POS);
         }

         rHead->totalFreeSpace -= BTREE_NODE_SLOT_SIZE;
         rHead->totalFreeSpace -= ref.data.size();
         ++rHead->totalSlotCount;
         frontOffset += BTREE_NODE_SLOT_SIZE;
      }// for (RECORD_SLOT_POS i = begin; i < head->totalSlotCount; ++i)

      _getTreeCtx()->statsSplit(rHead->totalSlotCount,
                                rHead->compressedItemCount,
                                origTotalEntrySize,
                                rightNode.getNodeSize() -
                                    rHead->totalFreeSpace -
                                    BTREE_NODE_PAGE_HEAD_SIZE,
                                rHead->prefixCount);
   done:
      return rc;
   error:
      goto done;
   }

   INT32 btreeNodeBase::_allocateRightNode(std::unique_ptr<btreeNodeBase> &rightNode)
   {
      INT32 rc = SDB_OK;
      btreeNodePageHead header;
      header.initAsRightNode(*_getReadableHead(), getNodeSize());
      rc = _ctx->allocateNewNode(_depth, header, rightNode);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to allocate right node:%d", rc);
         goto error;
      }

      rc = rightNode->_makeBufferWritable();
      if (OSS_UNLIKELY(SDB_OK != rc))
      {
         PD_LOG(PDERROR, "failed to make right node writable:%d", rc);
         goto error;
      }

   done:
      return rc;
   error:
      rightNode.reset();
      goto done;
   }

   INT32 btreeNodeBase::_buildRightNode(RECORD_SLOT_POS begin,
                                        btreeNodeBase &node)const
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(isValid(), "can not be invalid");
      SDB_ASSERT(!hasCompressedItems(), "must have no compressed items");
      SDB_ASSERT(INVALID_RECORD_SLOT_POS != begin, "can not be invalid");
      SDB_ASSERT(getNodeSize() == node.getNodeSize(), "must be same");
      

      btreeNodePageHead *newHead = node._buffer.getWritableObjPtr<btreeNodePageHead>(0);
      const btreeNodePageHead *head = _getReadableHead();
      SDB_ASSERT(begin < head->totalSlotCount, "invalid begin");
      UINT32 frontOffset = BTREE_NODE_PAGE_HEAD_SIZE;

      newHead->initAsRightNode(*head, getNodeSize());

      for (RECORD_SLOT_POS i = begin; i < head->totalSlotCount; ++i)
      {
         btreeItemSlot *slot = nullptr;
         UINT32 keyOffset = 0;
         _itemRef ref = _getItemRef(i);
         if (OSS_UNLIKELY(!ref.isValid()))
         {
            rc = SDB_VESSEL_INTERNAL_ERR;
            PD_LOG(PDERROR, "failed to get item[%d], rc:%d", i, rc);
            SDB_ASSERT(FALSE, "can not be invalid");
            goto error;
         }
         if (newHead->backOffset <
             (frontOffset + getSizeToSaveInNode(ref.data.size())))
         {
            PD_LOG(PDERROR, "not enough free space to save item");
            rc = SDB_VESSEL_NOT_ENOUGH_SPACE_IN_PAGE;
            goto error;
         }

         keyOffset = _getKeyDataOffsetToWrite(newHead, ref.data.size());
         rc = node._buffer.write(keyOffset, ref.data.size(), ref.data.data());
         if (OSS_UNLIKELY(SDB_OK != rc))
         {
            PD_LOG(PDERROR, "failed to write slice[%d,%d], rc:%d",
                   keyOffset, ref.data.size(), rc);
            goto error;
         }

         slot = node._buffer.getWritableObjPtr<btreeItemSlot>(frontOffset);
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
         newHead->totalFreeSpace -= BTREE_NODE_SLOT_SIZE;
         newHead->totalFreeSpace -= ref.data.size();
         newHead->backOffset = keyOffset;
         ++newHead->totalSlotCount;
         frontOffset += BTREE_NODE_SLOT_SIZE;
      }//for (RECORD_SLOT_POS i = begin; i < head->totalSlotCount; ++i)
      {
         UINT64 totalEntrySize = node.getNodeSize() - newHead->totalFreeSpace -
                                 BTREE_NODE_PAGE_HEAD_SIZE;
         _getTreeCtx()->statsSplit(
             newHead->totalSlotCount, 0, totalEntrySize, totalEntrySize, 0);
      }
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
         UINT32 oldTotalSlotCount = head->totalSlotCount;
         UINT32 oldCompressedItemCount = head->compressedItemCount;
         UINT64 origTotalEntrySizeDiff = 0;
         UINT64 realTotalEntrySizeDiff = 0;
         for (RECORD_SLOT_POS i = head->totalSlotCount - 1; i >= pos; --i)
         {
            const btreeItemSlot *slot = _getReadableSlot(i);
            size += BTREE_NODE_SLOT_SIZE;
            size += slot->data.key.size;
            origTotalEntrySizeDiff += slot->data.key.size + BTREE_NODE_SLOT_SIZE;
            realTotalEntrySizeDiff += slot->data.key.size + BTREE_NODE_SLOT_SIZE;
            if (slot->isKeyCompressed())
            {
               btreeNodePrefixSlot *prefixSlot =
                  _getWritablePrefixSlot(slot->data.lf.prefixSlot);
               if (OSS_UNLIKELY(nullptr == prefixSlot))
               {
                  rc = SDB_VESSEL_INTERNAL_ERR;
                  PD_LOG(PDERROR,
                        "failed to get first writable prefix slot ptr, rc:%d",
                        rc);
                  goto error;
               }
               prefixSlot->high = i;
               --head->compressedItemCount;
               origTotalEntrySizeDiff += prefixSlot->prefixSize;
            }
         }

         head->totalFreeSpace += size;
         head->totalSlotCount = keptItemNum;
         _getTreeCtx()->statsTruncate(keptItemNum,
                                      head->compressedItemCount,
                                      oldTotalSlotCount,
                                      oldCompressedItemCount,
                                      origTotalEntrySizeDiff,
                                      realTotalEntrySizeDiff);
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

   INT32 btreeNodeBase::_insertWithPrefix(const btreeKeyStringEntry &entry,
                                          RECORD_SLOT_POS pos,
                                          RECORD_SLOT_POS prefixPos)
   {
      SDB_ASSERT(entry.isValid(), "entry must be valid");
      INT32 rc = SDB_OK;
      btreeNodePageHead *head = nullptr;
      UINT32 keyOffset = 0;
      UINT32 bytesOptimized = _getReadablePrefixSlot(prefixPos)->prefixSize;
      rc = _makeBufferWritable();
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to get writable slice:%d", rc);
         goto error;
      }

      head = _buffer.getWritableObjPtr<btreeNodePageHead>(0);

      if (!(0 <= prefixPos && prefixPos < head->prefixCount))
      {
         rc = SDB_OUT_OF_BOUND;
         PD_LOG(PDERROR,
                "prefix position is out of bound[0, %d), rc:%d",
                head->prefixCount,
                rc);
         goto error;
      }
      if (!(0 <= pos && pos <= head->totalSlotCount))
      {
         rc = SDB_INVALIDARG;
         PD_LOG(PDERROR, "invalid arguments");
         goto error;
      }

      {
         btreeNodePrefixSlot *prefixSlot = _getWritablePrefixSlot(prefixPos);
         if (OSS_UNLIKELY(nullptr == prefixSlot))
         {
            rc = SDB_VESSEL_INTERNAL_ERR;
            PD_LOG(PDERROR,
                   "failed to get first writable prefix slot ptr, rc:%d",
                   rc);
            goto error;
         }
         btreeItemSlot *slot = nullptr;
         if (pos < prefixSlot->low || prefixSlot->high < pos ||
             pos > head->totalSlotCount)
         {
            rc = SDB_INVALIDARG;
            PD_LOG(PDERROR, "invalid arguments");
            goto error;
         }

         UINT32 suffixSize = entry.getRawDataSize() - bytesOptimized;
         const CHAR *suffixPtr = entry.getRawDataPtr() + bytesOptimized;
         keyOffset = _getKeyDataOffsetToWrite(head, suffixSize);
         rc = _buffer.write(keyOffset, suffixSize, suffixPtr);
         if (OSS_UNLIKELY(SDB_OK != rc))
         {
            PD_LOG(PDERROR,
                   "failed to write key data to[%d,%d], rc:%d",
                   keyOffset,
                   entry.getRawDataSize(),
                   rc);
            goto error;
         }

         slot = _getWritableSlot(pos);
         if (OSS_UNLIKELY(nullptr == slot))
         {
            PD_LOG(PDERROR, "failed to get writable slot ptr[%d]", pos);
            rc = SDB_VESSEL_INTERNAL_ERR;
            goto error;
         }

         UINT32 moveSize = (head->totalSlotCount - pos) * BTREE_NODE_SLOT_SIZE;
         ossMemmove(slot + 1, slot, moveSize);

         slot->initAsLeafFormat(keyOffset, suffixSize, prefixPos);
         btreeNodePrefixSlot *prefixSlots = _getWritablePrefixSlot(0);
         if (OSS_UNLIKELY(nullptr == prefixSlots))
         {
            rc = SDB_VESSEL_INTERNAL_ERR;
            PD_LOG(PDERROR,
                   "failed to get first writable prefix slot ptr, rc:%d",
                   rc);
            goto error;
         }
         prefixSlots[prefixPos].incBounds(FALSE);
         _adjustPrefsixSlots(prefixPos + 1, TRUE);
         _updateAppendingFactor(head, head->totalSlotCount == pos);
         ++head->totalSlotCount;
         ++head->compressedItemCount;
         head->totalFreeSpace -= suffixSize + BTREE_NODE_SLOT_SIZE;
         head->backOffset = keyOffset;

         _getTreeCtx()->statsInsertCompressedIndex(
             bytesOptimized, suffixSize + BTREE_NODE_SLOT_SIZE);
      }

   done:
      return rc;
   error:
      goto done;
   }

   INT32 btreeNodeBase::_pickPrefix(const btreeKeyStringEntry &entry,
                                    RECORD_SLOT_POS pos,
                                    RECORD_SLOT_POS &prefixPos) const
   {
      SDB_ASSERT(isLeaf(), "must be leaf");
      SDB_ASSERT(hasPrefixes(), "must have prefixes");
      const btreeNodePageHead *head = _getReadableHead();
      INT32 rc = SDB_OK;
      
      prefixPos = -1;
      UINT32 bytesOptimized = 0;
      
      
      if (0 < pos && pos < head->totalSlotCount)
      {
         const btreeItemSlot *slot = _getReadableSlot(pos);
         if (OSS_UNLIKELY(nullptr == slot))
         {
            PD_LOG(PDERROR, "failed to get writable slot ptr[%d]", pos);
            rc = SDB_VESSEL_INTERNAL_ERR;
            goto error;
         }
         rc = _canUsePrefixOfItem(
             slot - 1, entry.getRawData(), prefixPos, bytesOptimized);
         if (SDB_OK != rc)
         {
            PD_LOG(
                PDERROR, "failed to decide whether to use prefix, rc:%d", rc);
            goto error;
         }
         rc = _canUsePrefixOfItem(
             slot, entry.getRawData(), prefixPos, bytesOptimized);
         if (SDB_OK != rc)
         {
            PD_LOG(
                PDERROR, "failed to decide whether to use prefix, rc:%d", rc);
            goto error;
         }
      }
      else if (0 == pos)
      {
         const btreeItemSlot *slot = _getReadableSlot(pos);
         if (OSS_UNLIKELY(nullptr == slot))
         {
            PD_LOG(PDERROR, "failed to get writable slot ptr[%d]", pos);
            rc = SDB_VESSEL_INTERNAL_ERR;
            goto error;
         }
         rc = _canUsePrefixOfItem(
             slot, entry.getRawData(), prefixPos, bytesOptimized);
         if (SDB_OK != rc)
         {
            PD_LOG(
                PDERROR, "failed to decide whether to use prefix, rc:%d", rc);
            goto error;
         }
      }
      else // head->totalSlotCount == pos
      {
         const btreeItemSlot *slot = _getReadableSlot(pos - 1);
         if (OSS_UNLIKELY(nullptr == slot))
         {
            PD_LOG(PDERROR, "failed to get writable slot ptr[%d]", pos);
            rc = SDB_VESSEL_INTERNAL_ERR;
            goto error;
         }
         rc = _canUsePrefixOfItem(
             slot, entry.getRawData(), prefixPos, bytesOptimized);
         if (SDB_OK != rc)
         {
            PD_LOG(
                PDERROR, "failed to decide whether to use prefix, rc:%d", rc);
            goto error;
         }
      }

   done:
      return rc;
   error:
      goto done;
   }

   BOOLEAN btreeNodeBase::isNeedToBeDestroyed()const
   {
      SDB_ASSERT(isValid(), "can not be invalid");
      if (isLeaf())
      {
         return 0 == _getReadableHead()->totalSlotCount;
      }
      else
      {
         /// has no child and no actived item
         UINT32 itemCount = _getReadableHead()->totalSlotCount;
         return !hasRightChild() &&
                (0 == itemCount ||
                  (1 == itemCount &&
                   _getReadableSlot(0)->isMarkedDeleted() &&
                   INVALID_PAGE_ID == _getReadableSlot(0)->data.nlf.leftChild));
      }

   }

   void btreeNodeBase::_reset()
   {
      _nodeId = INVALID_PAGE_ID;
      _depth = 0;
      _buffer.reset();
   }

   void btreeNodeBase::_adjustPrefsixSlots(RECORD_SLOT_POS pos, BOOLEAN inc)
   {
      SDB_ASSERT(hasPrefixes(), "can not be invalid");
      SDB_ASSERT(isValidRecordSlotPosition(pos), "can not be invalid");

      btreeNodePrefixSlot *pslots = _getWritablePrefixSlot(0);
      SDB_ASSERT(nullptr != pslots && pslots->isValid(), "can not be invalid");
      UINT32 prefixCount = getPrefixCount();
      for (UINT32 i = pos; i < prefixCount; ++i)
      {
         if (inc)
         {
            pslots[i].incBounds(TRUE);
         }
         else
         {
            pslots[i].decBounds(TRUE);
         }
      }
   }

   RECORD_SLOT_POS btreeNodeBase::_lowerBoundPrefixSlot(RECORD_SLOT_POS itemPos)const
   {
      SDB_ASSERT(isValidRecordSlotPosition(itemPos), "can not be invalid");
      UINT32 count = getPrefixCount();
      SDB_ASSERT(0 < count, "can not be invalid");
      const btreeNodePrefixSlot *pslots = _getReadablePrefixSlot(0);
      UINT32 low = 0, step = 0 , pos = 0;
      while (0 < count)
      {
         step = count / 2;
         pos = low + step;
         if (pslots[pos].low < itemPos)
         {
            low = pos + 1;
            count -= step + 1;
         }
         else
         {
            count = step;
         }
      }
      return low;
   }

   RECORD_SLOT_POS btreeNodeBase::_upperBoundPrefixSlot(RECORD_SLOT_POS itemPos) const
   {
      SDB_ASSERT(isValidRecordSlotPosition(itemPos), "can not be invalid");
      UINT32 count = getPrefixCount();
      SDB_ASSERT(0 < count, "can not be invalid");
      const btreeNodePrefixSlot *pslots = _getReadablePrefixSlot(0);
      UINT32 low = 0, step = 0, pos =0;
      while (0 < count)
      {
         step = count / 2;
         pos = low + step;
         if (pslots[pos].low <= itemPos)
         {
            low = pos + 1;
            count -= step + 1;
         }
         else
         {
            count = step;
         }
      }
      return low;
   }

   RECORD_SLOT_POS btreeNodeBase::_findFirstPrefixPosWhenSplit(RECORD_SLOT_POS begin) const
   {
      SDB_ASSERT(isValidRecordSlotPosition(begin), "can not be invalid");
      const btreeItemSlot *slot = _getReadableSlot(begin);
      if (slot->isKeyCompressed())
      {
         return slot->data.lf.prefixSlot;
      }
      RECORD_SLOT_POS pos = _upperBoundPrefixSlot(begin);
      SDB_ASSERT(isValidRecordSlotPosition(pos), "can not be invalid");
      UINT16 prefixCount = getPrefixCount();
      while(pos < prefixCount)
      {
         if (_getReadablePrefixSlot(pos)->isReferenced())
         {
            break;
         }
         ++pos;
      }
      return pos;
   }

   // lower bound binary search
   INT32 btreeNodeBase::_locateNextPrefixSlot(RECORD_SLOT_POS pos) const
   {
      SDB_ASSERT(isLeaf(), "must be leaf node");
      const btreeNodePageHead *head = _getReadableHead();
      const btreeItemSlot *slot = _getReadableSlot(pos);
      SDB_ASSERT(nullptr != slot, "can not be nullptr");
      if (slot->data.lf.prefixSlot >= 0)
      {
         return slot->data.lf.prefixSlot;
      }
      const btreeNodePrefixSlot *prefixSlot = _getReadablePrefixSlot(0);
      UINT16 count = head->prefixCount, i = 0, first = 0, step = 0;
      while (count > 0)
      {
         step = count / 2;
         i = first + step;
         if (prefixSlot[i].low <= pos)
         {
            first = ++i;
            count -= step + 1;
         }
         else
         {
            count = step;
         }
      }
      return i;
   }

   void btreeNodeBase::_initSeekResult(INT32 cmp,
                                       RECORD_SLOT_POS pos,
                                       btreeNodeSeekResult &res) const
   {
      SDB_ASSERT(isValidRecordSlotPosition(pos), "can not be invalid");
      const btreeNodePageHead *head = _getReadableHead();
      SDB_ASSERT(pos <= head->totalSlotCount, "out of bound");

      res._res = cmp;
      res._slotPos = pos;
      res._isUpperBound = (pos == head->totalSlotCount);

      if (head->isLeaf())
      {
         res._child = INVALID_PAGE_ID;
         res._isMarkedDeleted = FALSE;
      }
      else if (res.isUpperBound())
      {
         res._child = head->rightChild;
         res._isMarkedDeleted = FALSE;
      }
      else
      {
         const btreeItemSlot *slot = _getReadableSlot(pos);
         res._child = slot->data.nlf.leftChild;
         res._isMarkedDeleted = slot->isMarkedDeleted();
      }

      return;
   }
} // namespace vessel

} // namespace engine
