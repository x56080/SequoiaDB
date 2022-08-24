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

   Source File Name = btreeNodeBase.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#ifndef VESSEL_BTREE_NODE_BASE_H_
#define VESSEL_BTREE_NODE_BASE_H_

#include "vessel/btreeNodePage.h"
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
                                const strictBuffer &buffer);
         virtual ~btreeNodeBase() = default;

      protected:
         virtual INT32 _makeBufferWritable() = 0;
         virtual btreeContext *_getTreeCtx() {return nullptr;}

      public:
         OSS_INLINE BOOLEAN isValid()const {return INVALID_PAGE_ID != _nodeId;}
         OSS_INLINE UINT32 getDepth()const {return _depth;}
         OSS_INLINE UINT32 getSizeToSaveInNode(UINT32 keySize)const
         {
            return BTREE_NODE_SLOT_SIZE + keySize;
         }
         OSS_INLINE BOOLEAN isWritable()const {return _buffer.isWritable();}
         OSS_INLINE PAGE_ID getNodeId()const {return _nodeId;}

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
         UINT32 getCompressedBytes() const;
         void dumpChildNodes(ossPoolVector<PAGE_ID> &nodes)const;
         BOOLEAN hasFreeSpaceToInsert(UINT32 itemSize,
                                      BOOLEAN *compaction=nullptr)const;
         BOOLEAN betterToActiveCompression()const;
         BOOLEAN hitHighWaterMark()const;
         BOOLEAN hasCompressedItems()const;
         UINT32 getPrefixCount()const;
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

         INT32 recompress(BOOLEAN &recompressed);

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
         INT32 _getItems(ossPoolVector<slice> &elementsPartRefs,
                         ossPoolVector<keyString> &items) const;

         prefixGenerator::resultStat _generatePrefixes(
             const ossPoolVector<slice> &elementsPartRefs,
             ossPoolVector<prefixGenerator::prefixItem> &out) const;

         INT32 _buildNewNodePage(
             strictBuffer &writableBuffer,
             const ossPoolVector<keyString> &elementsPartRefs,
             const ossPoolVector<prefixGenerator::prefixItem> &out) const;

         INT32 _locateNextPrefixSlot(RECORD_SLOT_POS pos) const;

         RECORD_SLOT_POS _lowerBoundPrefixSlot(RECORD_SLOT_POS itemPos)const;

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

         INT32 _leafCompact();

         INT32 _insert(RECORD_SLOT_POS pos,
                       const btreeKeyStringEntry &entry,
                       PAGE_ID leftChild=INVALID_PAGE_ID);

         INT32 _pickPrefix(const btreeKeyStringEntry &entry,
                           RECORD_SLOT_POS pos,
                           INT16 &prefixPos,
                           UINT32 &bytesOptimized)const;

         INT32 _insertWithPrefix(const btreeKeyStringEntry &entry,
                                 RECORD_SLOT_POS pos,
                                 INT16 prefixPos,
                                 UINT32 bytesOptimized);

         void _updateAppendingFactor(btreeNodePageHead *head,
                                     BOOLEAN stillAppendOnly);

         INT32 _findSplitPivot(BOOLEAN idleRight,
                               RECORD_SLOT_POS &pivot)const;

         INT32 _saveRaisingEntry(RECORD_SLOT_POS pos,
                                 btreeKeyStringEntry &entry) const;

         INT32 _split(RECORD_SLOT_POS pivot,
                      std::unique_ptr<btreeNodeBase> &rightNode);

         INT32 _allocateRightNode(std::unique_ptr<btreeNodeBase> &rightNode);

         INT32 _buildRightNode(RECORD_SLOT_POS pos,
                               btreeNodeBase &node)const;

         INT32 _truncate(UINT32 keptItemNum);

         INT32 _insertRaisedKey(const btreeSplitRaisedKey &raisedKey,
                                RECORD_SLOT_POS pos=INVALID_RECORD_SLOT_POS);

         void _adjustPrefsixSlots(RECORD_SLOT_POS pos, BOOLEAN inc=TRUE);

      protected:
         PAGE_ID _nodeId = INVALID_PAGE_ID;
         UINT32 _depth = 0;
         strictBuffer _buffer;
   };//class btreeNodeBase

   using BTREE_NODE_UPTR = std::unique_ptr<btreeNodeBase>;

} // namespace vessel

} // namespace engine


#endif//VESSEL_BTREE_NODE_BASE_H_