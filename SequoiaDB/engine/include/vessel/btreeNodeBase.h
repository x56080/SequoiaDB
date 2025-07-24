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

   Source File Name = btreeNodeBase.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef VESSEL_BTREE_NODE_BASE_H_
#define VESSEL_BTREE_NODE_BASE_H_

#include "vessel/btreeNodePage.h"
#include "vessel/recordID.h"
#include "vessel/strictBuffer.h"
#include "vessel/btreeStatistics.h"
#include "vessel/btreeSplitRaisedKey.h"
#include "vessel/btreeNodeSeekResult.h"
#include "vessel/prefixedKeyString.h"
#include "vessel/prefixGenerator.h"

#include <memory>

namespace engine
{
namespace vessel
{
   class btreeContext;

   class btreeNodeBase : public SDBObject
   {
      public:
         btreeNodeBase() = default;
         explicit btreeNodeBase(PAGE_ID nodeId,
                                UINT32 depth,
                                btreeContext* ctx,
                                const strictBuffer &buffer);
         virtual ~btreeNodeBase() = default;

      protected:
         virtual INT32 _makeBufferWritable() = 0;

      public:
         OSS_INLINE BOOLEAN isValid()const {return INVALID_PAGE_ID != _nodeId;}
         OSS_INLINE UINT32 getDepth()const {return _depth;}
         OSS_INLINE UINT32 getSizeToSaveInNode(UINT32 keySize)const
         {
            return BTREE_NODE_SLOT_SIZE + keySize;
         }
         OSS_INLINE BOOLEAN isWritable()const {return _buffer.isWritable();}
         OSS_INLINE PAGE_ID getNodeId()const {return _nodeId;}
         OSS_INLINE btreeContext *_getTreeCtx() const
         {
            SDB_ASSERT(nullptr != _ctx, "can not be invalid");
            return _ctx;
         }

         BOOLEAN isRoot()const;
         BOOLEAN isLeaf()const;
         UINT32 getItemCount()const;
         UINT32 getNodeSize()const;
         BOOLEAN isItemMarkedAsDeleted(RECORD_SLOT_POS pos)const;
         PAGE_ID getRightChild()const;
         BOOLEAN hasRightChild()const;
         BOOLEAN isRightChildLeaf()const;
         PAGE_ID getLeftChild(RECORD_SLOT_POS pos)const;
         PAGE_ID getChild(RECORD_SLOT_POS pos)const;
         DPS_TRANS_ID getTransID()const;
         btreeItemSlot getItemSlot(RECORD_SLOT_POS pos)const;
         INT64 getCompressionOptimizedBytes(RECORD_SLOT_POS startPos = 0) const;
         void dumpChildNodes(ossPoolVector<PAGE_ID> &nodes)const;
         BOOLEAN hasFreeSpaceToInsert(UINT32 itemSize,
                                      BOOLEAN *compaction=nullptr)const;
         BOOLEAN betterToActiveCompression()const;
         BOOLEAN hitHighWaterMark()const;
         BOOLEAN hasCompressedItems()const;
         UINT16 getPrefixCount()const;
         BOOLEAN hasPrefixes()const {return 0 < getPrefixCount();}
         BOOLEAN isNeedToBeDestroyed()const;

      public:
         INT32 locateEntry(const btreeKeyStringEntry &entry,
                           btreeNodeSeekResult &res) const;    

         /// key header will be ignored
         INT32 seek(const keyString &ks,
                    btreeNodeSeekResult &res) const;

         /// key header will be ignored
         INT32 seek(const keyString &ks,
                    RECORD_SLOT_POS pos,
                    BOOLEAN forward,/// range: [pos, last] if forward, [first, pos] if backward
                    btreeNodeSeekResult &res) const;

         /// compare with max/min key if forward/backward, 
         INT32 isOutOfKeyBound(const keyString &ks,
                               BOOLEAN forward,
                               BOOLEAN &outOfBound) const;

         INT32 getOwnedEntry(RECORD_SLOT_POS pos,
                             btreeKeyStringEntry &entry)const;

         INT32 remove(RECORD_SLOT_POS pos);

      public:
         /// leaf node only
         INT32 insert(const btreeKeyStringEntry &entry,
                      const DPS_TRANS_ID &transID=DPS_TRANS_ID());

         INT32 recompress(BOOLEAN &recompressed,
                          BOOLEAN fullyRegenerate = FALSE);

         INT32 splitAndInsert(const btreeKeyStringEntry &entry,
                              const DPS_TRANS_ID &transID,
                              btreeSplitRaisedKey &raisedKey);

      public:/// non-leaf node only.
         INT32 insertRaisedKey(const btreeSplitRaisedKey &raisedKey);

         /// when pos equals to item count in node,
         /// remove right child 
         INT32 removeChild(RECORD_SLOT_POS pos);

         INT32 reactiveRemovedKey(RECORD_SLOT_POS pos,
                                 const DPS_TRANS_ID &transID);

         INT32 refillChild(RECORD_SLOT_POS pos,
                           PAGE_ID child,
                           BOOLEAN childIsLeaf);

         INT32 splitAndInsert(const btreeSplitRaisedKey &raisedKey,
                              btreeSplitRaisedKey &newRaisedKey);


      protected:
         struct _itemRef : public SDBObject
         {
            _itemRef() = default;
            explicit _itemRef(const btreeItemSlot *s,
                              const slice &d):
            slot(s), data(d){}
            OSS_INLINE BOOLEAN isValid()const {return nullptr != slot &&
                                                      data.isValid();}
            const btreeItemSlot *slot = nullptr;
            slice data;
         };//struct _itemRef

         struct _prefixRef : public SDBObject
         {
            _prefixRef() = default;
            explicit _prefixRef(const btreeNodePrefixSlot *s,
                                const slice &d):
            slot(s), data(d){}
            OSS_INLINE BOOLEAN isValid()const {return nullptr != slot;}
            const btreeNodePrefixSlot *slot = nullptr;
            slice data;
         };//struct _prefixRef

      protected:
         void _reset();
         OSS_INLINE const btreeNodePageHead *_getReadableHead()const
         {
            return _buffer.getReadableObjPtr<btreeNodePageHead>(0);
         }
         OSS_INLINE btreeNodePageHead *_getWritableHead()
         {
            return _buffer.getWritableObjPtr<btreeNodePageHead>(0);
         }
         btreeNodePrefixSlot *_getWritablePrefixSlot(INT16 pos);
         const btreeNodePrefixSlot *_getReadablePrefixSlot(INT16 pos)const;
         const btreeItemSlot *_getReadableSlot(RECORD_SLOT_POS pos)const;
         btreeItemSlot *_getWritableSlot(RECORD_SLOT_POS pos);
         _itemRef _getItemRef(RECORD_SLOT_POS pos)const;
         _prefixRef _getPrefixRef(RECORD_SLOT_POS pos)const;
         prefixedKeyString _getPrefixedKeyString(RECORD_SLOT_POS pos)const;
         UINT32 _getContinuousFreeSpace()const;
         UINT32 _getFrontOffset()const;
         UINT32 _getBackOffset()const;
         UINT32 _getKeyDataOffsetToWrite(const btreeNodePageHead *head,
                                         UINT32 keyDataSize)const;
         BOOLEAN _isRecentWriteOrdered()const;

         INT32 _canUsePrefixOfItem(const btreeItemSlot *slot,
                                   const slice &raw,
                                   INT16 &prefixSlotToUse,
                                   UINT32 &prefixSizeToUse)const;

         // If a entry is compressed, its prefix and suffix will be concatenated
         // into a owned memory, else it is a reference to its whole entry data.
         INT32 _getWholeItems(ossPoolVector<slice> &elementsPartRefs,
                              ossPoolVector<keyString> &items,
                              INT32 boundaryOffset,
                              UINT64 &itemsTotalSize) const;

         INT32 _getLastKeptPrefixPosWhenRecompress(
             RECORD_SLOT_POS &lastKeptPrefixPos,
             UINT64 &optimizedSize) const;

         // calculate the sum of original elements size in [begin, end)
         UINT64 _getOrigTotalItemSize(RECORD_SLOT_POS begin,
                                      RECORD_SLOT_POS end) const;

         UINT64 _getPrefixesTotalSize() const;

         prefixGenerator::result _generatePrefixes(
             const ossPoolVector<slice> &elementsPartRefs,
             const INT32 boundaryOffset) const;

         INT32 _buildNodeWhenRecompress(
             strictBuffer &writableBuffer,
             RECORD_SLOT_POS lastKeptPrefixPos,
             RECORD_SLOT_POS boundaryOffset,
             const ossPoolVector<keyString> &items,
             const ossPoolVector<prefixGenerator::prefixItem> &prefixes,
             UINT64 &realTotalItemSize,
             UINT64 &optimizedBytes);

         INT32 _copyKeptPrefixesAndItems(
             strictBuffer &writableBuffer,
             RECORD_SLOT_POS lastKeptPrefixPos,
             UINT32 frontItemsOffset,
             UINT64 &realTotalSize,
             UINT64 &optimizedSize) const;

         INT32 _locateNextPrefixSlot(RECORD_SLOT_POS pos) const;

         RECORD_SLOT_POS _lowerBoundPrefixSlot(RECORD_SLOT_POS itemPos) const;

         RECORD_SLOT_POS _upperBoundPrefixSlot(RECORD_SLOT_POS itemPos) const;

         RECORD_SLOT_POS _findFirstPrefixPosWhenSplit(RECORD_SLOT_POS begin) const;

      protected:
         
         INT32 _locate(const btreeKeyStringEntry &ks,
                       btreeNodeSeekResult &res) const;

         INT32 _seek(const keyString &ks,
                     RECORD_SLOT_POS pos,
                     BOOLEAN forward,
                     btreeNodeSeekResult &res) const;

         INT32 _destroyItem(RECORD_SLOT_POS pos);

         INT32 _nonleafRemove(RECORD_SLOT_POS pos);

         INT32 _markRemoved(RECORD_SLOT_POS pos);

         void _updateTransID(const DPS_TRANS_ID &transID);

         INT32 _leafInsert(const btreeKeyStringEntry &entry,
                           const DPS_TRANS_ID &transID,
                           RECORD_SLOT_POS pos=INVALID_RECORD_SLOT_POS);

         INT32 _compact();

         INT32 _compactWhenHasPrefixes();

         INT32 _insert(RECORD_SLOT_POS pos,
                       const btreeKeyStringEntry &entry,
                       PAGE_ID leftChild=INVALID_PAGE_ID);

         INT32 _pickPrefix(const btreeKeyStringEntry &entry,
                           RECORD_SLOT_POS pos,
                           RECORD_SLOT_POS &prefixPos)const;

         INT32 _insertWithPrefix(const btreeKeyStringEntry &entry,
                                 RECORD_SLOT_POS pos,
                                 RECORD_SLOT_POS prefixPos);

         void _updateAppendingFactor(btreeNodePageHead *head,
                                     BOOLEAN stillAppendOnly);

         INT32 _findSplitPivot(BOOLEAN idleRight,
                               RECORD_SLOT_POS &pivot)const;

         INT32 _saveRaisingEntry(RECORD_SLOT_POS pos,
                                 btreeKeyStringEntry &entry) const;

         INT32 _split(RECORD_SLOT_POS pivot,
                      std::unique_ptr<btreeNodeBase> &rightNode);

         INT32 _buildCompressedRightNode(RECORD_SLOT_POS begin,
                                btreeNodeBase &rightNode) const;

         INT32 _allocateRightNode(std::unique_ptr<btreeNodeBase> &rightNode);

         INT32 _buildRightNode(RECORD_SLOT_POS pos,
                               btreeNodeBase &node)const;

         INT32 _truncate(UINT32 keptItemNum);

         INT32 _insertRaisedKey(const btreeSplitRaisedKey &raisedKey,
                                RECORD_SLOT_POS pos=INVALID_RECORD_SLOT_POS);

         void _adjustPrefsixSlots(RECORD_SLOT_POS pos, BOOLEAN inc=TRUE);

         void _initSeekResult(INT32 cmp,
                              RECORD_SLOT_POS pos,
                              btreeNodeSeekResult &res) const;

      protected:
         PAGE_ID _nodeId = INVALID_PAGE_ID;
         UINT32 _depth = 0;
         btreeContext *_ctx = nullptr;
         strictBuffer _buffer;
   };//class btreeNodeBase

   using BTREE_NODE_UPTR = std::unique_ptr<btreeNodeBase>;

} // namespace vessel

} // namespace engine


#endif//VESSEL_BTREE_NODE_BASE_H_