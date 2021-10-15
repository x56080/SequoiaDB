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
#include "vessel/btreeNodeCompressor.h"
#include "vessel/logicalPageBuffer.h"
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
      size -= getTotalPrefixSize(head);
      return size;
   }

   UINT32 btreeNode::getTotalPrefixSize(const btreeNodePageHead *head)const
   {
      SDB_ASSERT(NULL != head, "can not be null");
      UINT32 size = 0;

      for (UINT32 i = 0; i < BTREE_NODE_MAX_PREFIX_COUNT; ++i)
      {
         if (!head->prefixes[i].isValid())
         {
            break;
         }
         size += head->prefixes[i].prefixSize;
      }
      
   done:
      return size;
   }

   const btreeItemSlot *btreeNode::getReadableSlot(RECORD_SLOT_ID pos)const
   {
      SDB_ASSERT(isValid(), "can not be invalid");
      SDB_ASSERT(INVALID_RECORD_SLOT_ID != pos, "can not be invalid");
      const btreeNodePageHead *head = getReadableHead();
      const btreeItemSlot *slot = NULL;
      if (OSS_UNLIKELY(head->totalSlotCount <= pos))
      {
         SDB_ASSERT(FALSE, "out of bound");
         PD_LOG(PDERROR, "slot pos[%d] out of range[%d]",
                pos, head->totalSlotCount);
      }
      else
      {
         UINT32 offset = pos * BTREE_NODE_SLOT_SIZE + BTREE_NODE_PAGE_HEAD_SIZE;
         slot = _buffer->getRuntimeBuffer().getReadablePtrOfBody<btreeItemSlot>(offset);
         if (OSS_UNLIKELY(NULL == slot || !slot->isValid()))
         {
            SDB_ASSERT(FALSE, "impossible");
            PD_LOG(PDERROR, "slot [%d] is not valid", pos);
            slot = NULL;
         }
      }

      return slot;
   }

   btreeItemSlot *btreeNode::getWritableSlot(RECORD_SLOT_ID pos)
   {
      SDB_ASSERT(isValid(), "can not be invalid");
      SDB_ASSERT(INVALID_RECORD_SLOT_ID != pos, "can not be invalid");
      SDB_ASSERT(_buffer->getRuntimeBuffer().isWritingPrepared(), "must be prepared");

      btreeItemSlot *slot = NULL;
      const btreeNodePageHead *head = getReadableHead();
      if (OSS_UNLIKELY(head->totalSlotCount <= pos))
      {
         SDB_ASSERT(FALSE, "out of bound");
         PD_LOG(PDERROR, "slot no[%d] out of range[%d]",
                pos, head->totalSlotCount);
      }
      else
      {
         UINT32 offset = pos * BTREE_NODE_SLOT_SIZE + BTREE_NODE_PAGE_HEAD_SIZE;
         slot = _buffer->getRuntimeBuffer().getWritablePtrOfBody<btreeItemSlot>(offset);
         if (OSS_UNLIKELY(NULL == slot))
         {
            SDB_ASSERT(FALSE, "impossible");
            PD_LOG(PDERROR, "slot [%d] is not valid", pos);
         }
      }
      return slot;
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
         INT32 res = 0;
         btreeIndexItem item;
         rc = getIndexItem((RECORD_SLOT_ID)middle, item);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to get item at[%d], rc:%d", middle, rc);
            goto error;
         }

         res = item.woCompare(key, ow.toBsonOrdering());
         if (0 == res)
         {
            res = rid.compare(item.getRid());
            result.keyMatched = TRUE;
         }

         if (res < 0)
         {
            low = middle + 1;
         }
         else if (0 < res)
         {
            high = middle - 1;
         }
         else
         {
            result.slotPos = (RECORD_SLOT_ID)middle;
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
         result.slotPos = low;
      }
   done:
      return rc;
   error:
      goto done;
   }

   INT32 btreeNode::getIndexItem(RECORD_SLOT_ID slotPos,
                                 btreeIndexItem &item)const
   {
      INT32 rc = SDB_OK;
      const btreeNodePageHead *head = NULL;
      const btreeItemSlot *slot = NULL;
      const CHAR *keyData = NULL;
      const btreeNodePrefixSlot *prefixSlot = NULL;
      const CHAR *prefixData = NULL;
      item.fini();

      if (OSS_UNLIKELY(!isValid()))
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }
      else if (OSS_UNLIKELY(INVALID_RECORD_SLOT_ID == slotPos))
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

      if (head->totalSlotCount <= slotPos)
      {
         rc = SDB_OUT_OF_BOUND;
         goto error;
      }

      slot = getReadableSlot(slotPos);
      if (OSS_UNLIKELY(NULL == slot))
      {
         PD_LOG(PDERROR, "slot[%d] is invalid", slotPos);
         rc = SDB_VESSEL_INTERNAL_ERR;
         goto error;
      }

      SDB_ASSERT(!slot->isDataInExtPage(), "TODO");

      if (slot->isDataInPageBody())
      {
         ossValuePtr dataPtr = 0;
         rc = _buffer->getRuntimeBuffer().getReadablePtrOfBodyWithRc(slot->data.pointer.offset,
                                                                     slot->data.pointer.size,
                                                                     dataPtr);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to get pointer of key[%d,%d], rc:%d",
                   slot->data.pointer.offset,
                   slot->data.pointer.size, rc);
            goto error;
         }
         keyData = (const CHAR *)dataPtr;
      }

      if (slot->isKeyCompressed())
      {
         prefixData = getPrefixData(slot->getPrefixSlotPos());
         if (OSS_UNLIKELY(NULL == prefixData))
         {
            PD_LOG(PDERROR, "failed to get prefix data");
            rc = SDB_VESSEL_INTERNAL_ERR;
            goto error;
         }
      }

      rc = item.init(slotPos, slot, keyData, prefixData);
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

   BOOLEAN btreeNode::hasPrefixes()const
   {
      const btreeNodePageHead *head = getReadableHead();
      return head->prefixes[0].isValid();
   }

   BOOLEAN btreeNode::isCompressionDisabled()const
   {
      SDB_ASSERT(isValid(), "can not be invalid");
      return !_ic->getObj().getParams().isPrefixCompressionEnabled() ||
             !isLeaf() ||
             _depth < BTREE_COMPRESSION_MIN_DEPTH;
   }

   UINT32 btreeNode::getNodeSize()const
   {
      SDB_ASSERT(isValid(), "can not be invalid");
      return getPageBodySize(_buffer->getRuntimeBuffer().getPageSize());
   }

   UINT32 btreeNode::getSizeToSave(const ixmKey &key,
                                   UINT32 *keySize)const
   {
      SDB_ASSERT(isValid(), "can not be invalid");
      UINT32 size = BTREE_NODE_SLOT_SIZE;
      UINT32 keyDataSize = key.dataSize();
      if (!isLeaf() || !btreeItemSlot::isEnoughToSave(keyDataSize))
      {
         size += keyDataSize;
      }
      if (NULL != keySize)
      {
         *keySize = keyDataSize;
      }
      return size;
   }

   const CHAR *btreeNode::getPrefixData(UINT32 prefixSlotPos)const
   {
      SDB_ASSERT(isValid(), "can not be invalid");
      SDB_ASSERT(prefixSlotPos < BTREE_NODE_MAX_PREFIX_COUNT, "out of bound");
      const btreeNodePageHead *head = getReadableHead();
      const CHAR *prefix = NULL;
      const btreeNodePrefixSlot *ps = &(head->prefixes[prefixSlotPos]);
      if (ps->isValid())
      {
         const runtimePageBuffer &rpb = _buffer->getRuntimeBuffer();
         prefix = (const CHAR *)(rpb.getReadablePtrOfBody(ps->prefixOffset,
                                                          ps->prefixSize));
         if (NULL == prefix)
         {
            PD_LOG(PDERROR, "failed to get prefix[%d,%d]",
                   ps->prefixOffset, ps->prefixSize);
         }         
      }
      return prefix;
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

   INT32 btreeNode::beginToSplit(PAGE_ID &rightNode,
                                 btreeIndexItem &pivot,
                                 const SPLIT_MODE sm)const
   {
      INT32 rc = SDB_OK;
      ossSharedLatchMode mode(OSS_SHARED_LATCH_MODE_ENUM_EXCLUSIVE);
      btreeNodePageIniter initer;
      logicalPageBuffer lpb;
      btreeNode node;
      logicalPageSpace *lps = NULL;
      requestContext *context = NULL;

      rightNode = INVALID_PAGE_ID;
      pivot.fini();
      
      if (OSS_UNLIKELY(!isValid()))
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }
      else if (OSS_UNLIKELY(!_buffer->getLockingMode().isExclusive()))
      {
         rc = SDB_VESSEL_FORBIDDEN_OP_WLT;
         goto error;
      }

      rc = findSplitPivot(sm, pivot);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to find split item:%d", rc);
         goto error;
      }

      context = _buffer->getContext();
      lps = _buffer->getLogicalPageSpace();
      initer.set(context->getLogicalCLID(), _ic->getIndexID());
      rc = lps->allocatePages(context, &initer, 1, &rightNode);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to allocate new page:%d", rc);
         goto error;
      }

      rc = lps->getLogicalPageBuffer(context, rightNode, mode, lpb);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to get buffer of page[%d], rc:%d",
                rightNode, rc);
         goto error;
      }
   done:
      return rc;
   error:
      if (INVALID_PAGE_ID != rightNode)
      {
         lps->releasePages(context, 1, &rightNode);
         rightNode= INVALID_PAGE_ID;
      }
      pivot.fini();
      goto done;
   }

   INT32 btreeNode::findSplitPivot(const SPLIT_MODE sm,
                                   btreeIndexItem &pivot)const
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(isValid(), "can not be invalid");
      SDB_ASSERT(!pivot.isValid(), "can not be valid");
      static const UINT32 _MIN_SLOT_COUNT = 3;

      const btreeNodePageHead *head = getReadableHead();
      UINT32 maxSplitSize = 0;
      UINT32 totalSize = 0;
      UINT32 scannedSize = 0;
      
      if (head->totalSlotCount < _MIN_SLOT_COUNT)
      {
         PD_LOG(PDERROR, "too few slots[%d] to split", head->totalSlotCount);
         rc = SDB_VESSEL_OPERATOION_NOT_PERMITTED;
         goto error;
      }

      totalSize = getTotalKeyAndSlotSize();
      if (SPLIT_MODE_AVERAGE == sm)
      {
         maxSplitSize = totalSize / 2;
      }
      else
      {
         maxSplitSize = totalSize / 10;
      }

      for (INT32 i = (INT32)(head->totalSlotCount) - 1; i >= 0; --i)
      {
         UINT32 savingSize = 0;
         btreeIndexItem item;
         rc = getIndexItem(i, item);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to get index item[%d], rc:%d", i, rc);
            goto error;
         }

         savingSize = item.getSavingSize();
         if (maxSplitSize < (scannedSize + savingSize))
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
            scannedSize += savingSize;
         }
      }

      SDB_ASSERT(pivot.isValid(), "must be valid");
      if (OSS_UNLIKELY(0 == pivot.getSlotNo()))
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

   BOOLEAN btreeNode::hasSpaceToInsert(const ixmKey &key)const
   {
      SDB_ASSERT(isValid(), "can not be invalid");
      SDB_ASSERT(key.isValid(), "can not be invalid");
      SDB_ASSERT(isLeaf(), "must be leaf node");

      const btreeNodePageHead *head = getReadableHead();
      return getSizeToSave(key) <= head->totalFreeSpace;
   }

   INT32 btreeNode::insert(RECORD_SLOT_ID pos,
                           const ixmKey &key,
                           const recordID &rid,
                           const DPS_TRANS_ID *transID)
   {
      INT32 rc = SDB_OK;
      __btreeNode node;
      BOOLEAN prepared = FALSE;

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
      
      node = getReadableNode();
      if (OSS_UNLIKELY(!node.isLeaf()))
      {
         SDB_ASSERT(FALSE, "must be leaf node");
         rc = SDB_VESSEL_OPERATOION_NOT_PERMITTED;
         goto error;
      }
      else if (!node.hasSpaceToInsert(key.dataSize()))
      {
         rc = SDB_VESSEL_NOT_ENOUGH_SPACE_IN_PAGE;
         goto error;
      }
      else if (node.getSlotCount() < pos)
      {
         PD_LOG(PDERROR, "pos[%d] to insert out of bound[%d]",
                pos, node.getSlotCount());
         rc = SDB_OUT_OF_BOUND;
         goto error;
      }

      rc = _buffer->prepareToWrite();
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to prepare to write:%d", rc);
         goto error;
      }
      prepared = TRUE;

      node = getWritableNode();
      rc = node.insert(pos, key, rid, transID);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to insert to node:%d", rc);
         goto error;
      }

      _buffer->commit(_buffer->getContext()->getSession()->getLastLSN());
   done:
      return rc;
   error:
      if (prepared)
      {
         _buffer->abort();
      }
      goto done;
   }

   INT32 btreeNode::insertNormalKey(RECORD_SLOT_ID pos,
                                    const ixmKey &key,
                                    const recordID &rid,
                                    PAGE_ID leftChild,
                                    const DPS_TRANS_ID *transID)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(isValid(), "can not be invalid");
      SDB_ASSERT(INVALID_RECORD_SLOT_ID != pos, "can not be invalid");
      SDB_ASSERT(key.isValid(), "can not be invalid");
      SDB_ASSERT(!(!isLeaf() && INVALID_PAGE_ID == leftChild),
                 "left child can not be invalid");

      btreeNodePageHead *head = NULL;
      UINT32 size = 0;
      BOOLEAN prepared = FALSE;
      btreeItemSlot *slot = NULL;
      ossValuePtr keyPtr = 0;
      UINT32 keyOffset = 0;
      UINT32 keySize = 0;

      rc = _buffer->prepareToWrite();
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to prepare to write:%d", rc);
         goto error;
      }
      prepared = TRUE;

      head = getWritableHead();
      if (head->totalSlotCount < pos)
      {
         PD_LOG(PDERROR, "position[%d] out of slot bound[%d]",
                pos, head->totalSlotCount);
         rc = SDB_OUT_OF_BOUND;
         goto error;
      }

      size = getSizeToSave(key, &keySize);
      if (head->freeSapceAfterLastSlot < size)
      {
         rc = SDB_VESSEL_NOT_ENOUGH_SPACE_IN_PAGE;
         goto error;
      }

      slot = getWritableSlot(pos);
      if (OSS_UNLIKELY(NULL == slot))
      {
         PD_LOG(PDERROR, "failed to get writable slot[%d]", pos);
         rc = SDB_VESSEL_INTERNAL_ERR;
         goto error;
      }

      if (BTREE_NODE_SLOT_SIZE < size)
      {
         keyOffset = getKeyDataOffsetToWrite(head, keySize);
         runtimePageBuffer &rpb = _buffer->getWritableBuffer();
         rc = rpb.getWritablePtrOfBodyWithRc(keyOffset, keySize, keyPtr);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to get writable ptr:%d", rc);
            goto error;
         }

         ossMemcpy((CHAR *)keyPtr, key.data(), keySize);
      }

      /// do not goto error from here.
      if (pos < head->totalSlotCount)
      {
         UINT32 moveSize = (head->totalSlotCount - pos) * BTREE_NODE_SLOT_SIZE;
         ossMemmove(slot + 1, slot, moveSize);
      }

      slot->reset();
      if (BTREE_NODE_SLOT_SIZE < size)
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

      if (NULL != transID && transID->isValid() &&
          head->transSN < transID->getSN())
      {
         head->transSN = transID->getSN();
      }

      _buffer->commit(_buffer->getContext()->getSession()->getLastLSN());

   done:
      return rc;
   error:
      if (prepared)
      {
         _buffer->abort();
      }
      goto done;
   }

   INT32 btreeNode::insertCompressedKey(RECORD_SLOT_ID pos,
                                        const btreeNodeCompressedKey &ck,
                                        const recordID &rid,
                                        const DPS_TRANS_ID *transID)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(isValid(), "can not be invalid");
      SDB_ASSERT(INVALID_RECORD_SLOT_ID != pos, "can not be invalid");
      SDB_ASSERT(ck.isValid(), "can not be invalid");
      SDB_ASSERT(rid.valid(), "can not be invalid");

      btreeNodePageHead *head = NULL;
      BOOLEAN prepared = FALSE;
      btreeItemSlot *slot = NULL;
      ossValuePtr suffixPtr = 0;
      UINT32 suffixOffset = 0;
      UINT32 suffixSize = 0;
      UINT32 size = BTREE_NODE_SLOT_SIZE;

      rc = _buffer->prepareToWrite();
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to prepare to write:%d", rc);
         goto error;
      }
      prepared = TRUE;

      head = getWritableHead();
      if (head->totalSlotCount < pos)
      {
         PD_LOG(PDERROR, "position[%d] out of slot bound[%d]",
                pos, head->totalSlotCount);
         rc = SDB_OUT_OF_BOUND;
         goto error;
      }

      slot = getWritableSlot(pos);
      if (OSS_UNLIKELY(NULL == slot))
      {
         PD_LOG(PDERROR, "failed to get writable slot[%d]", pos);
         rc = SDB_VESSEL_INTERNAL_ERR;
         goto error;
      }

      suffixSize = ck.getSuffixSize();
      if (!btreeItemSlot::isEnoughToSave(suffixSize))
      {
         suffixOffset = getKeyDataOffsetToWrite(head, suffixSize);
         runtimePageBuffer &rpb = _buffer->getWritableBuffer();
         rc = rpb.getWritablePtrOfBodyWithRc(suffixOffset, suffixSize, suffixPtr);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to get writable ptr:%d", rc);
            goto error;
         }

         ossMemcpy((CHAR *)suffixPtr, ck.getSuffix().data(), suffixSize);
         size += suffixSize;
      }

      /// do not goto error from here
      if (pos < head->totalSlotCount)
      {
         UINT32 moveSize = (head->totalSlotCount - pos) * BTREE_NODE_SLOT_SIZE;
         ossMemmove(slot + 1, slot, moveSize);
      }

      slot->reset();
      if (0 == suffixSize)
      {
         slot->initWhenPerfectlyCompressed(rid, ck.getPrefixSlotPos());
      }
      else if (btreeItemSlot::isEnoughToSave(suffixSize))
      {
         slot->initWhenDataInSlot(rid, ck.getSuffix().data(), suffixSize);
         slot->setKeyCompressed(ck.getPrefixSlotPos());
      }
      else
      {
         slot->initWhenDataInPage(rid, suffixOffset, suffixSize);
         slot->setKeyCompressed(ck.getPrefixSlotPos());
      }

      ++head->totalSlotCount;
      head->totalFreeSpace -= size;
      head->freeSapceAfterLastSlot -= size;

      if (NULL != transID && transID->isValid() &&
          head->transSN < transID->getSN())
      {
         head->transSN = transID->getSN();
      }

      ++(head->prefixes[ck.getPrefixSlotPos()].referencedCnt);

      _buffer->commit(_buffer->getContext()->getSession()->getLastLSN());
   done:
      return rc;
   error:
      if (prepared)
      {
         _buffer->abort();
      }
      goto done;
   }

   UINT32 btreeNode::getKeyDataOffsetToWrite(const btreeNodePageHead *head,
                                             UINT32 keyDataSize)
   {
      SDB_ASSERT(0 < keyDataSize, "can not be zero");
      SDB_ASSERT(NULL != head, "can not be null");
      UINT32 offset = 0;
      SDB_ASSERT(keyDataSize <= head->freeSapceAfterLastSlot,
                 "out of resource");
      return BTREE_NODE_PAGE_HEAD_SIZE +
             (BTREE_NODE_SLOT_SIZE * head->totalSlotCount) +
             head->freeSapceAfterLastSlot -
             keyDataSize;
   }

   INT32 btreeNode::splitTo(UINT32 bufferSize,
                            CHAR *buffer,
                            RECORD_SLOT_ID begin)const
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(isValid(), "can not be invalid");
      SDB_ASSERT(NULL != buffer, "can not be null");
      SDB_ASSERT(INVALID_RECORD_SLOT_ID != begin, "can not be invalid");

      UINT32 nodeSize = getNodeSize();
      SDB_ASSERT(nodeSize <= bufferSize, "invalid buffer size");
      const btreeNodePageHead *rh = getReadableHead();
      SDB_ASSERT(begin < rh->totalSlotCount, "out of bound");

      ossMemset(buffer, 0, nodeSize);
      btreeNodePageHead *head = (btreeNodePageHead *)buffer;
      head->version = rh->version;
      head->clLogicalID = rh->clLogicalID;
      head->indexId = rh->indexId;
      head->totalFreeSpace = nodeSize - BTREE_NODE_PAGE_HEAD_SIZE;
      head->freeSapceAfterLastSlot = head->totalFreeSpace;
      head->rightChild = rh->rightChild;
      head->extNode = INVALID_PAGE_ID;
      head->transSN = rh->transSN;

      UINT32 frontOffset = BTREE_NODE_PAGE_HEAD_SIZE;
      const CHAR *lastPrefix = NULL;
      INT32 whichPrefix = -1;

      for (RECORD_SLOT_ID i = begin; i < rh->totalSlotCount; ++i)
      {
         btreeItemSlot *slot = (btreeItemSlot *)((CHAR *)head + frontOffset);
         btreeIndexItem item;
         rc = getIndexItem(i, item);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to get index item[%d], rc:%d", i, rc);
            goto error;
         }

         SDB_ASSERT(item.getSavingSize() <= head->freeSapceAfterLastSlot, "impossible");

         slot->reset();
         slot->setInUsed();
         slot->ridSlot = item.getSlot()->ridSlot;
         slot->ridPage = item.getSlot()->ridPage;

         if (item.getSlot()->isKeyCompressed())
         {
            if (lastPrefix != item.getPrefixData())
            {
               rc = addNewPrefix(head, ixmKey(item.getPrefixData()), &whichPrefix);
               if (SDB_OK != rc)
               {
                  PD_LOG(PDERROR, "failed to add new prefix to node:%d", rc);
                  goto error;
               }
               lastPrefix = item.getPrefixData();
            }
         }
      }
      
   done:
      return rc;
   error:
      goto done;
   }

   INT32 btreeNode::addNewPrefix(btreeNodePageHead *head,
                                 const ixmKey &prefix,
                                 INT32 *which)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(NULL != head, "can not be null");
      SDB_ASSERT(prefix.isValid(), "can not be invalid");

      UINT32 offset = 0;
      UINT32 prefixSize = prefix.dataSize();
      btreeNodePrefixSlot *slot = NULL;

      if (head->freeSapceAfterLastSlot < prefixSize)
      {
         rc = SDB_VESSEL_NOT_ENOUGH_SPACE_IN_PAGE;
         goto error;
      }

      for (INT32 i = 0; i < (INT32)BTREE_NODE_MAX_PREFIX_COUNT; ++i)
      {
         if (head->prefixes[i].isValid())
         {
            continue;
         }

         slot = &(head->prefixes[i]);
         if (NULL != which)
         {
            *which = i;
         }
         break;
      }

      if (NULL == slot)
      {
         rc = SDB_VESSEL_NOT_ENOUGH_FREE_RESOURCE;
         goto error;
      }

      offset = getKeyDataOffsetToWrite(head, prefixSize);
      ossMemcpy((CHAR *)head + offset, prefix.data(), prefixSize);

      slot->prefixSize = prefixSize;
      slot->prefixOffset = (UINT16)offset;
      slot->referencedLow = INVALID_RECORD_SLOT_ID;
      slot->referencedHigh = INVALID_RECORD_SLOT_ID;
      head->totalFreeSpace -= prefixSize;
      head->freeSapceAfterLastSlot -= prefixSize;
   done:
      return rc;
   error:
      if (NULL != which)
      {
         *which = -1;
      }
      goto done;
   }

   INT32 btreeNode::tryToCompressKeyInserting(RECORD_SLOT_ID pos,
                                              const ixmKey &key,
                                              btreeNodeCompressedKey &compressedKey)const
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(INVALID_RECORD_SLOT_ID != pos, "can not be invalid");
      SDB_ASSERT(key.isValid(), "can not be invalid");
      SDB_ASSERT(isLeaf(), "must be leaf");

      const btreeNodePageHead *head = getReadableHead();
      SDB_ASSERT(pos <= head->totalSlotCount, "out of slot bound");
      const btreeItemSlot *slot = NULL;
      INT32 prefixPos = -1;
      ossValuePtr prefixPtr = 0;
      const btreeNodePrefixSlot *ps = NULL;

      compressedKey.reset();

      if (!hasPrefixes() || !key.isCompressable())
      {
         goto done;
      }

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

      prefixPos = slot->getPrefixSlotPos();
      ps = &(head->prefixes[prefixPos]);
      if (!ps->isValid())
      {
         PD_LOG(PDERROR, "invalid prefix slot[%d]", prefixPos);
         rc = SDB_VESSEL_INTERNAL_ERR;
         goto error;
      }

      rc = _buffer->getRuntimeBuffer().getReadablePtrOfBodyWithRc(ps->prefixOffset,
                                                                  ps->prefixSize,
                                                                  prefixPtr);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to get prefix data[%d,%d], rc:%d",
                ps->prefixOffset, ps->prefixSize, rc);
         goto error;
      }

      rc = tryToCompressKey(key, prefixPos,
                            ixmKey((const CHAR *)prefixPtr),
                            compressedKey);
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
                                     INT32 prefixPos,
                                     const ixmKey &prefix,
                                     btreeNodeCompressedKey &compressedKey)const
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(key.isValid(), "can not be invalid");
      SDB_ASSERT(prefix.isValid(), "can not be invalid");
      SDB_ASSERT(!isCompressionDisabled(), "can not be disabled");
      SDB_ASSERT(!compressedKey.isValid(), "can not be valid");
      btreeNodeCompressor compressor;

      if (compressor.compress(prefixPos, prefix,
                              key, compressedKey))
      {
         if (compressedKey.hasSuffix())
         {
            rc = compressedKey.getSuffixOwned();
            if (SDB_OK != rc)
            {
               PD_LOG(PDERROR, "failed to get owned suffix:%d", rc);
               goto error;
            }
         }
      }
   done:
      return rc;
   error:
      compressedKey.reset();
      goto done;
   }

   btreeNode::__btreeNode btreeNode::getReadableNode()const
   {
      SDB_ASSERT(isValid(), "can not be invalid");
      return __btreeNode(_buffer->getReadableBodyPtr(),
                         _ic, _depth);
   }

   btreeNode::__btreeNode btreeNode::getWritableNode()
   {
      SDB_ASSERT(isValid(), "can not be invalid");
      SDB_ASSERT(_buffer->getRuntimeBuffer().isWritingPrepared(), "must be prepared");
      return __btreeNode(_buffer->getWritableBodyPtr(),
                         _ic, _depth);
   }

/////////////__btreeNode begin
   btreeNode::__btreeNode::__btreeNode(const strictPointer &ptr,
                                       const indexContext *ic,
                                       UINT32 depth):
   _ptr(ptr), _ic(ic), _depth(depth)
   {
      SDB_ASSERT(ptr.isValid(), "can not be invalid");
      SDB_ASSERT(NULL != ic && ic->isValid(), "can not be invalid");
   }

   BOOLEAN btreeNode::__btreeNode::isRoot()const
   {
      SDB_ASSERT(isValid(), "can not be invalid");
      return 0 == _depth;
   }

   BOOLEAN btreeNode::__btreeNode::hasExtNode()const
   {
      return getReadableHead()->extNode != INVALID_PAGE_ID;
   }

   BOOLEAN btreeNode::__btreeNode::isLeaf()const
   {
      return INVALID_PAGE_ID == getReadableHead()->rightChild;
   }

   BOOLEAN btreeNode::__btreeNode::isVainPrefixRegen()const
   {
      const btreeNodePageHead *head = getReadableHead();
      return 0 != OSS_BIT_TEST(head->flags, BTREE_NODE_FLAG_VAIN_PREFIX_REGENERATION);
   }

   UINT32 btreeNode::__btreeNode::getSlotCount()const
   {
      const btreeNodePageHead *head = getReadableHead();
      return head->totalSlotCount;
   }

   BOOLEAN btreeNode::__btreeNode::hasSpaceToInsert(UINT32 keySize,
                                                    BOOLEAN *needCompact)const
   {
      SDB_ASSERT(0 < keySize, "impossible");
      const btreeNodePageHead *head = getReadableHead();
      UINT32 size = getSizeToSaveInNode(keySize);
      BOOLEAN r = size <= head->totalFreeSpace;
      if (r && NULL != needCompact)
      {
         *needCompact = head->freeSapceAfterLastSlot < size;
      }
      return r;
   }

   BOOLEAN btreeNode::__btreeNode::hasPrefix()const
   {
      const btreeNodePageHead *head = getReadableHead();
      SDB_ASSERT(isLeaf(), "must be leaf");
      /// prefix slots should always be compacted.
      return head->prefixes[0].isValid();
   }

   UINT32 btreeNode::__btreeNode::getCompressedKeyCount()const
   {
      const btreeNodePageHead *head = getReadableHead();
      SDB_ASSERT(isLeaf(), "must be leaf");
      UINT32 cnt = 0;

      for (UINT32 i = 0; i < BTREE_NODE_MAX_PREFIX_COUNT; ++i)
      {
         const btreeNodePrefixSlot &ps = head->prefixes[i];
         if (!ps.isValid())
         {
            break;
         }
         cnt += ps.referencedCnt;
      }
      return cnt;
   }

   UINT32 btreeNode::__btreeNode::getSizeToSaveInNode(UINT32 keySize)const
   {
      UINT32 size = BTREE_NODE_SLOT_SIZE;
      if (!isLeaf() || !btreeItemSlot::isEnoughToSave(keySize))
      {
         size += keySize;
      }
      return size;
   }

   BOOLEAN btreeNode::__btreeNode::isCompressionDisabled()const
   {
      return !_ic->getObj().getParams().isPrefixCompressionEnabled() ||
             !isLeaf() ||
             _depth < BTREE_COMPRESSION_MIN_DEPTH;
   }

   UINT32 btreeNode::__btreeNode::getKeyDataOffsetToWrite(const btreeNodePageHead *head,
                                                          UINT32 keyDataSize)const
   {
      SDB_ASSERT(0 < keyDataSize, "can not be zero");
      SDB_ASSERT(NULL != head, "can not be null");
      UINT32 offset = 0;
      SDB_ASSERT(keyDataSize <= head->freeSapceAfterLastSlot,
                 "out of resource");
      return BTREE_NODE_PAGE_HEAD_SIZE +
             (BTREE_NODE_SLOT_SIZE * head->totalSlotCount) +
             head->freeSapceAfterLastSlot -
             keyDataSize;
   }

   btreeItemSlot *btreeNode::__btreeNode::getWritableSlot(RECORD_SLOT_ID pos)
   {
      SDB_ASSERT(INVALID_RECORD_SLOT_ID != pos, "can not be invalid");
      SDB_ASSERT(_ptr.isWritable(), "must be writable");
      return _ptr.getWritableObjPtr<btreeItemSlot>(BTREE_NODE_PAGE_HEAD_SIZE +
                                                   BTREE_NODE_SLOT_SIZE * pos);
   }

   const btreeItemSlot *btreeNode::__btreeNode::getReadableSlot(RECORD_SLOT_ID pos)const
   {
      SDB_ASSERT(INVALID_RECORD_SLOT_ID != pos, "can not be invalid");
      SDB_ASSERT(_ptr.isReadable(), "must be valid");
      return _ptr.getReadableObjPtr<btreeItemSlot>(BTREE_NODE_PAGE_HEAD_SIZE +
                                                   BTREE_NODE_SLOT_SIZE * pos);
   }

   UINT32 btreeNode::__btreeNode::getNodeSize()const
   {
      return _ptr.getSize();
   }

   BOOLEAN btreeNode::__btreeNode::hitHighWaterMark()const
   {
      const btreeNodePageHead *head = getReadableHead();
      return ((FLOAT32)head->totalFreeSpace / getNodeSize()) <=
             (1.0 - BTREE_NODE_HIGH_WATER_MARK);
   }

   BOOLEAN btreeNode::__btreeNode::isBetterToBeRecompressed()const
   {
      return hitHighWaterMark() &&
             !isCompressionDisabled() &&
             !isVainPrefixRegen() &&
             (((FLOAT32)getOptimizedSizeByComprssion() / getNodeSize()) <
             BTREE_NODE_EFFECTIVE_COMPRESSION_THRESHOLD);
   }

   UINT32 btreeNode::__btreeNode::getOptimizedSizeByComprssion()const
   {
      SDB_ASSERT(isLeaf(), "must be leaf");
      UINT32 size = 0;
      const btreeNodePageHead *head = getReadableHead();
      for (UINT32 i = 0; i < BTREE_NODE_MAX_PREFIX_COUNT; ++i)
      {
         if (!head->prefixes[i].isValid())
         {
            break;
         }

         size += head->prefixes[i].optimizedSize;
      }
      return size;
   }

   INT32 btreeNode::__btreeNode::insert(RECORD_SLOT_ID pos,
                                        const ixmKey &key,
                                        const recordID &rid,
                                        const DPS_TRANS_ID *transID)
   {
      INT32 rc = SDB_OK;
      BOOLEAN needCompact = FALSE;
      UINT32 keySize = key.dataSize();

      if (OSS_UNLIKELY(!isValid()))
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }
      if (OSS_UNLIKELY(!_ptr.isWritable()))
      {
         rc = SDB_VESSEL_OPERATOION_NOT_PERMITTED;
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
      else if (!hasSpaceToInsert(keySize, &needCompact))
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
         rc = _insert(pos, key, rid, INVALID_PAGE_ID, transID);
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
            rc = insertCompressedKey(pos, ck, rid, transID);
            if (SDB_OK != rc)
            {
               PD_LOG(PDERROR, "failed to insert compressed key:%d", rc);
               goto error;
            }
         }
         else
         {
            rc = _insert(pos, key, rid, INVALID_PAGE_ID, transID);
            if (SDB_OK != rc)
            {
               PD_LOG(PDERROR, "failed to insert to node:%d", rc);
               goto error;
            }
         }
      }

      if (isBetterToBeRecompressed())
      {
         recompress();
      }
   done:
      return rc;
   error:
      goto done;
   }

   INT32 btreeNode::__btreeNode::_insert(RECORD_SLOT_ID pos,
                                         const ixmKey &key,
                                         const recordID &rid,
                                         PAGE_ID leftChild,
                                         const DPS_TRANS_ID *transID)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(_ptr.isWritable(), "can not be invalid");
      SDB_ASSERT(INVALID_RECORD_SLOT_ID != pos, "can not be invalid");
      SDB_ASSERT(key.isValid(), "can not be invalid");
      SDB_ASSERT(rid.valid(), "can not be invalid");
      SDB_ASSERT(!(!isLeaf() && INVALID_PAGE_ID == leftChild),
                 "left child can not be invalid");

      btreeItemSlot *slot = NULL;
      UINT32 keySize = key.dataSize();
      UINT32 keyOffset = 0;
      UINT32 size = getSizeToSaveInNode(keySize);
      BOOLEAN saveKeyInPage = BTREE_NODE_SLOT_SIZE < size;
      btreeNodePageHead *head = _ptr.getWritableObjPtr<btreeNodePageHead>(0);
      if (OSS_UNLIKELY(NULL == head))
      {
         PD_LOG(PDERROR, "failed to get writable head ptr");
         rc = SDB_VESSEL_INTERNAL_ERR;
         goto error;
      }

      /// should be prechecked outside
      SDB_ASSERT(size <= head->freeSapceAfterLastSlot, "impossible");
      SDB_ASSERT(pos <= head->totalSlotCount, "impossible");

      if (saveKeyInPage)
      {
         keyOffset = getKeyDataOffsetToWrite(head, keySize);
         rc = _ptr.write(keyOffset, keySize, key.data());
         if (OSS_UNLIKELY(SDB_OK != rc))
         {
            PD_LOG(PDERROR, "failed to write key data:%d", rc);
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
      if (NULL != transID && transID->isValid() &&
          head->transSN < transID->getSN())
      {
         head->transSN = transID->getSN();
      }
   done:
      return rc;
   error:
      goto done;
   }

   INT32 btreeNode::__btreeNode::tryToCompressKeyInserting(RECORD_SLOT_ID pos,
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

      ck.reset();

      if (!hasPrefix() || !key.isCompressable())
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
      if (OSS_UNLIKELY(!ps->isValid()))
      {
         PD_LOG(PDERROR, "unexpected invalid prefix slot[%d]", slot->getPrefixSlotPos());
         rc = SDB_VESSEL_INTERNAL_ERR;
         goto error;
      }

      prefixData = _ptr.getReadablePtr(ps->prefixOffset, ps->prefixSize);
      if (OSS_UNLIKELY(NULL == prefixData))
      {
         PD_LOG(PDERROR, "failed to get prefix data");
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

   INT32 btreeNode::__btreeNode::tryToCompressKey(const ixmKey &key,
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

      btreeNodeCompressor compressor;

      if (compressor.compress(prefixPos, prefix, key, ck))
      {
         if (ck.hasSuffix())
         {
            rc = ck.getSuffixOwned();
            if (SDB_OK != rc)
            {
               PD_LOG(PDERROR, "failed to get owned suffix:%d", rc);
               goto error;
            }
         }
      }
   done:
      return rc;
   error:
      goto done;
   }

   INT32 btreeNode::__btreeNode::insertCompressedKey(RECORD_SLOT_ID pos,
                                                     const btreeNodeCompressedKey &ck,
                                                     const recordID &rid,
                                                     const DPS_TRANS_ID *transID)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(_ptr.isWritable(), "can not be invalid");
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

      head = _ptr.getWritableObjPtr<btreeNodePageHead>(0);
      if (OSS_UNLIKELY(NULL == head))
      {
         PD_LOG(PDERROR, "failed to get writable head ptr");
         rc = SDB_VESSEL_INTERNAL_ERR;
         goto error;
      }
      
      SDB_ASSERT(ck.getPrefixSlotPos() < BTREE_NODE_MAX_PREFIX_COUNT, "out of bound");
      ps = &(head->prefixes[ck.getPrefixSlotPos()]);
      if (OSS_UNLIKELY(!ps->isValid()))
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
         rc = _ptr.write(suffixOffset, suffixSize, ck.getSuffix().data());
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

      ++ps->referencedCnt;
      if (saveInPage)
      {
         ps->incOptimizedSize(ck.getPrefix().dataSize());
      }
      else
      {
         ps->incOptimizedSize(ck.getPrefix().dataSize() + suffixSize);
      }

      if (NULL != transID && transID->isValid() &&
          head->transSN < transID->getSN())
      {
         head->transSN = transID->getSN();
      }
   done:
      return rc;
   error:
      goto done;
   }

   BOOLEAN btreeNode::__btreeNode::recompress()
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(_ptr.isWritable(), "can not be invalid");
      SDB_ASSERT(isLeaf(), "must be leaf node");
      memoryBlock mb;

      rc = mb.reserve(_ptr.getSize());
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to allocate mem");
         rc = SDB_OOM;
         goto error;
      }
   done:
      return rc;
   error:
      goto done;
   }
/////////////__btreeNode end

} // namespace vessel

} // namespace engine

