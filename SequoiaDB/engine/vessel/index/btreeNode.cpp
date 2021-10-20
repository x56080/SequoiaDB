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
#include "vessel/btreeNodePageIniter.h"
#include "vessel/memoryBlock.h"

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

   BOOLEAN btreeNode::isRoot()const
   {
      SDB_ASSERT(isValid(), "can not be invalid");
      return 0 == _depth;
   }

   BOOLEAN btreeNode::hasExternalKey()const
   {
      return 0 != OSS_BIT_TEST(getReadableHead()->flags, BTREE_NODE_FLAG_HAS_EXTERNAL_KEY);
   }

   BOOLEAN btreeNode::isLeaf()const
   {
      return INVALID_PAGE_ID == getReadableHead()->rightChild;
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
      SDB_ASSERT(isLeaf(), "must be leaf");
      const btreeNodePageHead *head = getReadableHead();
      UINT32 size = getSizeToSaveInNode(keySize);
      BOOLEAN r = (size <= head->totalFreeSpace);
      if (r && NULL != needCompact)
      {
         *needCompact = (head->freeSapceAfterLastSlot < size);
      }
      return r;
   }

   BOOLEAN btreeNode::hasPrefix()const
   {
      const btreeNodePageHead *head = getReadableHead();
      SDB_ASSERT(isLeaf(), "must be leaf");
      for (UINT32 i = 0; i < BTREE_NODE_MAX_PREFIX_COUNT; ++i)
      {
         if (!head->prefixes[i].isFree())
         {
            return TRUE;
         }
      }
      return FALSE;
   }

   UINT32 btreeNode::getSizeToSaveInNode(UINT32 keySize)const
   {
      UINT32 size = BTREE_NODE_SLOT_SIZE;
      if (!isLeaf() || !btreeItemSlot::isEnoughToSave(keySize))
      {
         size += keySize;
      }
      return size;
   }

   BOOLEAN btreeNode::isCompressionDisabled()const
   {
      return !_ic->getObj().getParams().isPrefixCompressionEnabled() ||
             _depth < _ic->getObj().getParams().btreeMinCompressionDepth ||
             !isLeaf();
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
      return _buffer->getWritableBodySlice().
             getWritableObjPtr<btreeItemSlot>(BTREE_NODE_PAGE_HEAD_SIZE +
                                              BTREE_NODE_SLOT_SIZE * pos);
   }

   const btreeItemSlot *btreeNode::getReadableSlot(RECORD_SLOT_ID pos)const
   {
      SDB_ASSERT(INVALID_RECORD_SLOT_ID != pos, "can not be invalid");
      SDB_ASSERT(isValid(), "can not be invalid");
      
      return _buffer->getReadableBodySlice().
             getReadableObjPtr<btreeItemSlot>(BTREE_NODE_PAGE_HEAD_SIZE +
                                              BTREE_NODE_SLOT_SIZE * pos);
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
      const btreeNodePageHead *head = getReadableHead();
      static const FLOAT32 _RATIO = 0.6;

      return hitHighWaterMark() &&
             !isVainPrefixRegen() &&
             (head->keyInSlotCount < (head->totalSlotCount * _RATIO)) &&
             (((FLOAT32)getOptimizedSizeByCompression() / getNodeSize()) <
             BTREE_NODE_EFFECTIVE_COMPRESSION_RATIO);
   }

   UINT32 btreeNode::getOptimizedSizeByCompression()const
   {
      SDB_ASSERT(isLeaf(), "must be leaf");
      UINT32 size = 0;
      const btreeNodePageHead *head = getReadableHead();
      for (UINT32 i = 0; i < BTREE_NODE_MAX_PREFIX_COUNT; ++i)
      {
         if (!head->prefixes[i].isFree())
         {
            size += head->prefixes[i].optimizedSize;
         }
      }
      return size;
   }

   UINT32 btreeNode::getTotalKeyDataAndSlotSize()const
   {
      const btreeNodePageHead *head = getReadableHead();
      return getNodeSize() - BTREE_NODE_PAGE_HEAD_SIZE -
             head->totalFreeSpace - getTotalKeyPrefixSize();
   }

   UINT32 btreeNode::getTotalKeyPrefixSize()const
   {
      UINT32 size = 0;
      const btreeNodePageHead *head = getReadableHead();
      for (UINT32 i = 0; i < BTREE_NODE_MAX_PREFIX_COUNT; ++i)
      {
         if (!head->prefixes[i].isFree())
         {
            size += head->prefixes[i].prefixSize;
         }
      }
      return size;
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

   INT32 btreeNode::insert(const ixmKey &key,
                           const recordID &rid,
                           const DPS_TRANS_ID &transID)
   {
      INT32 rc = SDB_OK;
      btreeItemLocation location;
      rc = locateKeyAndRid(key, rid, location);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to locate key:%d", rc);
         goto error;
      }

      rc = insert(location.slotPos, key, rid, transID);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to insert key:%d", rc);
         goto error;
      }
   done:
      return rc;
   error:
      goto done;
   }

   INT32 btreeNode::insert(RECORD_SLOT_ID pos,
                           const ixmKey &key,
                           const recordID &rid,
                           const DPS_TRANS_ID &transID)
   {
      INT32 rc = SDB_OK;
      BOOLEAN needCompact = FALSE;
      UINT32 keySize = 0;

      if (OSS_UNLIKELY(!isValid()))
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }
      else if (OSS_UNLIKELY(INVALID_RECORD_SLOT_ID == pos ||
                            !key.isValid() ||
                            !rid.valid()))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      else if (!isLeaf())
      {
         rc = SDB_VESSEL_OPERATOION_NOT_PERMITTED;
         goto error;
      }

      keySize = key.dataSize();
      if (!hasFreeSpaceToInsert(keySize, &needCompact))
      {
         rc = SDB_VESSEL_NOT_ENOUGH_SPACE_IN_PAGE;
         goto error;
      }
      else if (needCompact)
      {
         rc = compact();
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to compact node:%d", rc);
            goto error;
         }
      }

      /// do not comrpess key if it can be saved in slot.
      if (!hasPrefix() ||
          btreeItemSlot::isEnoughToSave(keySize))
      {
         rc = _insert(pos, key, rid);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to insert to node:%d", rc);
            goto error;
         }
      }
      else
      {
         btreeNodeCompressedKey ck;
         rc = tryToCompressKeyInserting(pos, key, ck);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to compress key:%d", rc);
            goto error;
         }

         if (ck.isValid())
         {
            rc = insertCompressedKey(pos, ck, rid);
            if (SDB_OK != rc)
            {
               PD_LOG(PDERROR, "failed to insert compressed key:%d", rc);
               goto error;
            }
         }
         else
         {
            rc = _insert(pos, key, rid);
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

      updateTransSN(transID);
      commit();
   done:
      return rc;
   error:
      goto done;
   }

   INT32 btreeNode::insertRaisedKey(RECORD_SLOT_ID pos,
                                    const ixmKey &key,
                                    const recordID &rid,
                                    const DPS_TRANS_ID &transID,
                                    PAGE_ID leftChild,
                                    PAGE_ID rightChild)
   {
      INT32 rc = SDB_OK;
      UINT32 savingSize = 0;
      const btreeNodePageHead *head = NULL;
      BOOLEAN appendOnly = FALSE;

      if (OSS_UNLIKELY(INVALID_RECORD_SLOT_ID == pos ||
                       !key.isValid() ||
                       !rid.valid() ||
                       INVALID_PAGE_ID == leftChild ||
                       INVALID_PAGE_ID == rightChild))
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
      else if (OSS_UNLIKELY(!hasExternalKey()))
      {
         /// should always split node with ext key when traverse down
         SDB_ASSERT(FALSE, "has external key");
         rc = SDB_VESSEL_OPERATOION_NOT_PERMITTED;
         goto error;
      }
      
      head = getReadableHead();
      if (head->totalSlotCount < pos)
      {
         PD_LOG(PDERROR, "invalid insert pos[%d], current slot count[%d]",
                pos, head->totalSlotCount);
         rc = SDB_OUT_OF_BOUND;
         goto error;
      }
      else if (head->totalFreeSpace < BTREE_NODE_SLOT_SIZE)
      {
         PD_LOG(PDERROR, "has not enough free space to insert");
         rc = SDB_VESSEL_NOT_ENOUGH_SPACE_IN_PAGE;
         goto error;
      }

      appendOnly = (head->totalSlotCount == pos);

      savingSize = getSizeToSaveInNode(key.dataSize());
      if (head->totalFreeSpace < savingSize)
      {
         if (head->freeSapceAfterLastSlot < BTREE_NODE_SLOT_SIZE)
         {
            rc = compact();
            if (SDB_OK != rc)
            {
               PD_LOG(PDERROR, "failed to compact node:%d", rc);
               goto error;
            }
         }

         rc = insertExternalKey(pos, key, leftChild);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to insert external key:%d", rc);
            goto error;
         }
      }
      else
      {
         if (head->freeSapceAfterLastSlot < savingSize)
         {
            rc = compact();
            if (SDB_OK != rc)
            {
               PD_LOG(PDERROR, "failed to compact node:%d", rc);
               goto error;
            }
         }

         rc = _insert(pos, key, rid, leftChild);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to insert key:%d", rc);
            goto error;
         }
      }
      
      if (appendOnly)
      {
         slice s = _buffer->getWritableBodySlice();
         SDB_ASSERT(s.isWritale(), "must be writable");
         s.getWritableObjPtr<btreeNodePageHead>(0)->rightChild = rightChild;
      }
      else
      {
         getWritableSlot(pos + 1)->data.pointer.leftChild = rightChild;
      }

      updateTransSN(transID);
      commit();
   done:
      return rc;
   error:
      goto done;
   }

   INT32 btreeNode::reactiveRemovedKey(const btreeItemLocation &location,
                                       const DPS_TRANS_ID &transID)
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
      else if (!getReadableSlot(location.slotPos)->isMarkedDelete())
      {
         PD_LOG(PDERROR, "item[%d] is not marked as removed", location.slotPos);
         rc = SDB_VESSEL_OPERATOION_NOT_PERMITTED;
         goto error;
      }

      rc = _buffer->autoGetWritableBodySlice(ws);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to prepare to write:%d", rc);
         goto error;
      }

      slot = getWritableSlot(location.slotPos);
      OSS_BIT_CLEAR(slot->flags, btreeItemSlot::FLAG_MARKED_DELETE);

      updateTransSN(transID);
      commit();

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
            res.keyMatched = TRUE;
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
            if (!isLeaf())
            {
               res.child = item.getSlot()->data.pointer.leftChild;
            }
            goto done;
         }

         middle = (low + high) >> 1;
      }

      res.slotPos = low;
      if (!isLeaf())
      {
         if (head->totalSlotCount == low)
         {
            res.child = head->rightChild;
         }
         else
         {
            res.child = getReadableSlot(low)->data.pointer.leftChild;
         }
      }
   done:
      return rc;
   error:
      goto done;
   }

   INT32 btreeNode::getItem(RECORD_SLOT_ID pos,
                            btreeIndexItem &item)const
   {
      INT32 rc = SDB_OK;
      const btreeNodePageHead *head = NULL;
      const btreeItemSlot *slot = NULL;
      const CHAR *keyData = NULL;
      const CHAR *prefixData = NULL;

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

      SDB_ASSERT(!slot->isDataInExtPage(), "TODO");

      if (slot->isDataInPageBody())
      {
         keyData = getReadableSlice().getReadablePtr(slot->data.pointer.offset,
                                                     slot->data.pointer.size);
         if (NULL == keyData)
         {
            PD_LOG(PDERROR, "failed to get key data[%d,%d]",
                   slot->data.pointer.offset,
                   slot->data.pointer.size);
            rc = SDB_VESSEL_INTERNAL_ERR;
            goto error;
         }
      }

      if (slot->isKeyCompressed())
      {
         const btreeNodePrefixSlot &ps = head->prefixes[slot->getPrefixSlotPos()];
         prefixData = getReadableSlice().getReadablePtr(ps.prefixOffset,
                                                        ps.prefixSize);
         if (NULL == prefixData)
         {
            PD_LOG(PDERROR, "failed to get prefix key data[%d,%d]",
                   ps.prefixOffset,
                   ps.prefixSize);
            rc = SDB_VESSEL_INTERNAL_ERR;
            goto error;
         } 
      }

      rc = item.init(pos, slot, keyData, prefixData);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to init index item:%d", rc);
         goto error;
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
      SDB_ASSERT(!(!isLeaf() && INVALID_PAGE_ID == leftChild),
                 "left child can not be invalid");

      btreeItemSlot *slot = NULL;
      UINT32 keySize = key.dataSize();
      UINT32 keyOffset = 0;
      UINT32 size = getSizeToSaveInNode(keySize);
      BOOLEAN saveKeyInPage = BTREE_NODE_SLOT_SIZE < size;
      btreeNodePageHead *head = NULL;
      slice writableSlice;

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

      if (saveKeyInPage)
      {
         keyOffset = getKeyDataOffsetToWrite(head, keySize);
         rc = writableSlice.write(keyOffset, keySize, key.data());
         if (OSS_UNLIKELY(SDB_OK != rc))
         {
            PD_LOG(PDERROR, "failed to write key data[%d,%d], rc:%d",
                   keyOffset, keySize, rc);
            goto error;
         }
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
      slot->reset();
      if (saveKeyInPage)
      {
         slot->initWhenDataInPage(rid, keyOffset, keySize,
                                  isLeaf() ? INVALID_PAGE_ID : leftChild);
      }
      else
      {
         SDB_ASSERT(isLeaf(), "must be leaf");
         slot->initWhenDataInSlot(rid, key.data(), keySize);
      }

      ++head->totalSlotCount;
      head->totalFreeSpace -= size;
      head->freeSapceAfterLastSlot -= size;
   done:
      return rc;
   error:
      goto done;
   }

   INT32 btreeNode::insertExternalKey(RECORD_SLOT_ID pos,
                                      const ixmKey &key,
                                      PAGE_ID leftChild)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(FALSE, "TODO");
   done:
      return rc;
   error:
      goto done;
   }

   INT32 btreeNode::tryToCompressKeyInserting(RECORD_SLOT_ID pos,
                                              const ixmKey &key,
                                              btreeNodeCompressedKey &ck)const
   {
      INT32 rc = SDB_OK;
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
      }
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
      INT32 rc = SDB_OK;
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
      BOOLEAN saveInPage = BTREE_NODE_SLOT_SIZE < size;
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

      if (saveInPage)
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

      slot->reset();
      if (!ck.hasSuffix())
      {
         slot->initWhenPerfectlyCompressed(rid, ck.getPrefixSlotPos());
      }
      else if (saveInPage)
      {
         slot->initWhenDataInPage(rid, suffixOffset, suffixSize);
         slot->setKeyCompressed(ck.getPrefixSlotPos());
      }
      else
      {
         slot->initWhenDataInSlot(rid, ck.getSuffix().data(), suffixSize);
         slot->setKeyCompressed(ck.getPrefixSlotPos());
      }

      ++head->totalSlotCount;
      head->totalFreeSpace -= size;
      head->freeSapceAfterLastSlot -= size;

      referenceToPrefix(*ps, ck.getSuffixSize());
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

   INT32 btreeNode::findSplitPivot(UINT32 factor,
                                   btreeIndexItem &pivot)const
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(2 == factor || 10 == factor, "can not be others");
      static const UINT32 _MIN_SLOT_COUNT = 3;
      UINT32 splitSize = 0;
      UINT32 scanned = 0;
      const btreeNodePageHead *head = NULL;

      pivot.fini();

      if (OSS_UNLIKELY(!isValid()))
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }

      head = getReadableHead();
      if (head->totalSlotCount < _MIN_SLOT_COUNT)
      {
         PD_LOG(PDERROR, "too few slots[%d] to split", head->totalSlotCount);
         rc = SDB_VESSEL_OPERATOION_NOT_PERMITTED;
         goto error;
      }

      splitSize = getTotalKeyDataAndSlotSize() / factor;

      for (INT32 i = (INT32)(head->totalSlotCount - 1); 0 <= i; --i)
      {
         UINT32 savingSize = 0;
         btreeIndexItem item;
         rc = getItem(i, item);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to get index item[%d], rc:%d", i, rc);
            goto error;
         }

         savingSize = item.getSavingSize();
         if (splitSize < (scanned + savingSize))
         {
            if (!pivot.isValid())
            {
               pivot = item;
            }
            break;
         }
         else
         {
            pivot = item;
            scanned += savingSize;
         }
      }
      
      if (OSS_UNLIKELY(0 == pivot.getSlotPos()))
      {
         PD_LOG(PDERROR, "pivot slot number can not be zero");
         rc = SDB_VESSEL_INTERNAL_ERR;
         goto error;
      }
   done:
      return rc;
   error:
      pivot.fini();
      goto done;
   }

   INT32 btreeNode::presplit(btreeIndexItem &pivot,
                             PAGE_ID &rightNode,
                             BOOLEAN orderedInsert)const
   {
      INT32 rc = SDB_OK;
      UINT32 factor = 2;
      memoryBlock mb;
      btreeNodePageSplitIniter initer;
      PAGE_ID rightLpid = INVALID_PAGE_ID;
      slice newPageSlice;

      pivot.fini();
      rightNode = INVALID_PAGE_ID;

      if (OSS_UNLIKELY(!isValid()))
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }

      if (orderedInsert && isLeaf())
      {
         factor = 10;
      }

      rc = findSplitPivot(factor, pivot);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to find split pivot:%d", rc);
         goto error;
      }

      rc = mb.reserve(getNodeSize());
      if (OSS_UNLIKELY(SDB_OK != rc))
      {
         PD_LOG(PDERROR, "failed to reserve mb space:%d", rc);
         goto error;
      }

      newPageSlice.makeWritable(mb.getCapacity(), mb.getBuffer());

      rc = buildSplitNode(pivot.getSlotPos() + 1, newPageSlice);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to build new page:%d", rc);
         goto error;
      }

      initer.set(newPageSlice);
      rc = _buffer->getLogicalPageSpace()->allocatePages(_buffer->getContext(),
                                                         &initer,
                                                         1, &rightLpid);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to allocate new node page:%d", rc);
         goto error;
      }

      rightNode = rightLpid;
   done:
      return rc;
   error:
      pivot.fini();
      rightNode = INVALID_PAGE_ID;
      goto done;
   }

   INT32 btreeNode::buildSplitNode(RECORD_SLOT_ID begin, slice &s)const
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(isValid(), "can not be invalid");
      SDB_ASSERT(INVALID_RECORD_SLOT_ID != begin, "can not be invalid");
      SDB_ASSERT(s.isWritale(), "must be writable");
      SDB_ASSERT(getNodeSize() <= s.getSize(), "must be enough");
      UINT32 backOffset = getNodeSize();
      UINT32 frontOffset = BTREE_NODE_PAGE_HEAD_SIZE;
      const btreeNodePageHead *head = getReadableHead();
      SDB_ASSERT(begin < head->totalSlotCount, "out of bound");
      btreeNodePageHead *newHead = s.getWritableObjPtr<btreeNodePageHead>(0);

      ossMemset(newHead, 0, BTREE_NODE_PAGE_HEAD_SIZE);
      newHead->version = head->version;
      newHead->clLogicalID = head->clLogicalID;
      newHead->indexId = head->indexId;
      newHead->totalFreeSpace = getNodeSize() - BTREE_NODE_PAGE_HEAD_SIZE;
      newHead->freeSapceAfterLastSlot = head->totalFreeSpace;
      newHead->rightChild = head->rightChild;
      newHead->transSN = head->transSN;

      for (RECORD_SLOT_ID i = begin; i < head->totalSlotCount; ++i)
      {
         UINT32 savingSize = 0;
         btreeItemSlot *slot = NULL;
         btreeIndexItem item;
         rc = getItem(i, item);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to get index item[%d], rc:%d", i, rc);
            goto error;
         }

         savingSize = item.getSavingSize();

         if (item.getSlot()->isKeyCompressed() &&
             newHead->prefixes[item.getSlot()->getPrefixSlotPos()].isFree())
         {
            SDB_ASSERT(isLeaf(), "must be leaf");
            btreeNodePrefixSlot &ps = newHead->prefixes[slot->getPrefixSlotPos()];
            UINT32 prefixSize = ixmKey(item.getPrefixData()).dataSize();
            UINT32 prefixOffset = backOffset - prefixSize;
            SDB_ASSERT(frontOffset < prefixOffset, "impossible");
            rc = s.write(prefixOffset, prefixSize, item.getPrefixData());
            if (OSS_UNLIKELY(SDB_OK != rc))
            {
               PD_LOG(PDERROR, "failed to write prefix data[%d,%d], rc:%d",
                      prefixOffset, prefixSize, rc);
               goto error;
            }
            ps.prefixOffset = prefixOffset;
            ps.prefixSize = prefixSize;
            backOffset -= prefixSize;
         }

         if (backOffset < (frontOffset + savingSize))
         {
            PD_LOG(PDERROR, "not enough free space to push item[%d]", i);
            rc = SDB_VESSEL_NOT_ENOUGH_SPACE_IN_PAGE;
            goto error;
         }

         slot = s.getWritableObjPtr<btreeItemSlot>(frontOffset);
         slot->reset();
         *slot = *(item.getSlot());

         /// auto remove external key
         if (item.getSlot()->isDataInPageBody() ||
             item.getSlot()->isDataInExtPage())
         {
            UINT32 keyDataSize = ixmKey(item.getKeyData()).dataSize();
            UINT32 keyDataOffset = backOffset - keyDataSize;
            rc = s.write(keyDataOffset, keyDataSize, item.getKeyData());
            if (OSS_UNLIKELY(SDB_OK != rc))
            {
               PD_LOG(PDERROR, "failed to write prefix data[%d,%d], rc:%d",
                      keyDataOffset, keyDataSize, rc);
               goto error;
            }

            slot->data.pointer.offset = keyDataOffset;
            slot->data.pointer.size = keyDataSize;
            OSS_BIT_CLEAR(slot->flags, btreeItemSlot::FLAG_DATA_IN_EXTERNAL_PAGE);
            OSS_BIT_SET(slot->flags, btreeItemSlot::FLAG_DATA_IN_PAGE_BODY);
            backOffset -= keyDataSize;
         }

         frontOffset += BTREE_NODE_SLOT_SIZE;

         if (slot->isKeyCompressed())
         {
            referenceToPrefix(newHead->prefixes[slot->getPrefixSlotPos()],
                              slot->getSuffixSize());
         }

         ++newHead->totalSlotCount;
      }

      newHead->totalFreeSpace = (backOffset - frontOffset);
      newHead->freeSapceAfterLastSlot = newHead->totalFreeSpace;
   done:
      return rc;
   error:
      goto done;
   }

   void btreeNode::referenceToPrefix(btreeNodePrefixSlot &ps,
                                     UINT32 suffixSize)
   {
      SDB_ASSERT(!ps.isFree(), "can not be invalid");

      ++ps.referencedCnt;
      if (btreeItemSlot::isEnoughToSave(suffixSize))
      {
         ps.optimizedSize += (ps.prefixSize + suffixSize);
      }
      else
      {
         ps.optimizedSize += ps.prefixSize;
      }
      return;
   }  

   void btreeNode::commit()
   {
      if (isValid() && _buffer->isWritable())
      {
         ISession *session = _buffer->getContext()->getSession();
         _buffer->commit(session->getLastLSN());
      }
      return;
   }

   void btreeNode::updateTransSN(const DPS_TRANS_ID &transID)
   {
      SDB_ASSERT(isValid() && _buffer->isWritable(), "must be writable");
      if (transID.isValid())
      {
         btreeNodePageHead *head = _buffer->getWritableBodySlice().
                                   getWritableObjPtr<btreeNodePageHead>(0);
         if (DPS_INVALID_TRANSID_SN == head->transSN ||
             head->transSN < transID.getSN())
         {
            head->transSN = transID.getSN();
         }
      }
   }

   INT32 btreeNode::compact()
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(FALSE, "TODO");
   done:
      return rc;
   error:
      goto done;
   }
} // namespace vessel

} // namespace engine

