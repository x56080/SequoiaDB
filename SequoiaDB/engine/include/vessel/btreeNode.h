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

   Source File Name = btreeNode.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#ifndef VESSEL_BTREE_NODE_H_
#define VESSEL_BTREE_NODE_H_

#include "vessel/btreeNodePage.h"
#include "vessel/btreeNodeItem.h"
#include "vessel/btreeNodeSeekResult.h"
#include "ossSharedLatch.hpp"
#include "vessel/btreeSplitRaisedKey.h"
#include "rtnPredicate.hpp"
#include "vessel/strictBuffer.h"
#include "vessel/btreeKeyStringEntry.h"
#include "vessel/prefixedKeyString.h"

namespace engine
{
namespace vessel
{
   class logicalPageBuffer;
   class btreeAccessContext;
   class indexProperties;

   class btreeNode : public SDBObject
   {
      public:
         btreeNode() = default;
          ~btreeNode() = default;

         explicit btreeNode(UINT32 depth, btreeAccessContext *ctx);
         explicit btreeNode(UINT32 depth,
                            logicalPageBuffer *buffer,
                            btreeAccessContext *ctx);

      public:
         OSS_INLINE BOOLEAN isValid()const
         {
            return nullptr != _buffer &&
                   nullptr != _ctx;
         }

         OSS_INLINE void reset()
         {
            _depth = 0;
            _buffer = nullptr;
            _ctx = nullptr;
            return;
         }

      public:
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
         BOOLEAN betterToBeDestroyed()const;

         BOOLEAN becameEmptyAfterRemoving(RECORD_SLOT_POS pos)const;


         const logicalPageBuffer *getBuffer()const
         {
            return _buffer;
         }
         UINT32 getDepth()const
         {
            return _depth;
         }
         btreeItemSlot getItemSlot(RECORD_SLOT_POS pos)const;

         void dumpAllSubNodes(ossPoolVector<PAGE_ID> &nodes);

      public:

         BOOLEAN hasFreeSpaceToInsert(UINT32 entrySize,
                                      BOOLEAN *needCompact=nullptr)const;
         /// leaf node only
         INT32 leafInsert(const btreeKeyStringEntry &entry);

         /// leaf node only
         INT32 splitLeafAndInsert(const btreeKeyStringEntry &entry,
                                  btreeSplitRaisedKey &raisedKey);

         /// non-leaf node only
         INT32 insertRaisedKey(const btreeSplitRaisedKey &raisedKey);

         INT32 splitNonLeafAndInsert(const btreeSplitRaisedKey &raisedKeyFromChild,
                                     btreeSplitRaisedKey &raisedKey);

         // INT32 split(RECORD_SLOT_POS &pos,
         //             btreeSplitRaisedKey &raisedKey);

         INT32 compress();

         /// non-leaf node only
         INT32 reactiveRemovedKey(RECORD_SLOT_POS pos);

         INT32 resetRemovedChild(RECORD_SLOT_POS pos,
                                 PAGE_ID child,
                                 BOOLEAN childIsLeaf);

         INT32 resetAsEmptyNode();

      public:
         INT32 removeEntry(RECORD_SLOT_POS pos);

         /// non-leaf node only
         /// when pos equals to item count in node,
         /// remove right child
         INT32 removeChild(RECORD_SLOT_POS pos);
         
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

         INT32 isOutOfKeyBound(const keyString &ks,
                               BOOLEAN forward,
                               BOOLEAN &outOfBound) const;

         INT32 getItem(RECORD_SLOT_POS pos,
                       btreeNodeItem &item)const;

      private:
         struct _entryRef
         {
            OSS_INLINE BOOLEAN isValid()const {return nullptr != slot;}
            const btreeItemSlot *slot = nullptr;
            slice data;
         };//struct _entryRef

      private:
         const indexProperties *getProperties()const;
         OSS_INLINE UINT32 getSizeToSaveInNode(UINT32 keySize)const
         {
            return BTREE_NODE_SLOT_SIZE + keySize;
         }
         BOOLEAN isCompressionDisabled()const;
         UINT32 getKeyDataOffsetToWrite(const btreeNodePageHead *head,
                                          UINT32 keyDataSize)const;

         btreeItemSlot *getWritableSlot(RECORD_SLOT_POS pos);
         const btreeItemSlot *getReadableSlot(RECORD_SLOT_POS pos)const;

         const btreeNodePrefixSlot *getReadablePrefixSlot(UINT16 pos)const;
         btreeNodePrefixSlot *getWritablePrefixSlot(UINT16 pos);

         BOOLEAN hitHighWaterMark()const;
         BOOLEAN isBetterToBeRecompressed() const;

         BOOLEAN isVainPrefixRegen()const;
         BOOLEAN hasCompressedKeys()const;
         BOOLEAN hasPrefix()const;
         UINT32 getFrontOffset()const;
         UINT32 getBackOffset()const;
         UINT32 getContinuousFreeSpace()const;

         const btreeNodePageHead *getReadableHead()const;

         strictBuffer getReadableBuffer()const;

         BOOLEAN isRecentWriteOrdered()const;

         prefixedKeyString _getPrefixedKeyString(RECORD_SLOT_POS pos);

         INT32 _locateEntry(const btreeKeyStringEntry &ks,
                            btreeNodeSeekResult &res) const;

         INT32 _seek(const keyString &ks,
                     RECORD_SLOT_POS pos,
                     BOOLEAN forward,
                     btreeNodeSeekResult &res) const;

         _entryRef _getEntryRef(RECORD_SLOT_POS pos) const;

         INT32 _locateNextPrefixSlot(RECORD_SLOT_POS pos) const;

         INT32 _canUsePrefixOfItem(btreeItemSlot *slot,
                                   const slice &raw,
                                   INT16 &prefixSlotToUse,
                                   UINT16 prefixSizeToUse) const;

      private:
         void commit();
         void updateTransID(const DPS_TRANS_ID &transID);

         INT32 _compact();

         INT32 _leafCompact();

         INT32 _leafInsert(const btreeKeyStringEntry &entry,
                           RECORD_SLOT_POS pos=INVALID_RECORD_SLOT_POS);

         INT32 _insert(RECORD_SLOT_POS pos,
                       const btreeKeyStringEntry &entry,
                       PAGE_ID leftChild=INVALID_PAGE_ID);

         INT32 _insertRaisedKey(const btreeSplitRaisedKey &raisedKey,
                                 RECORD_SLOT_POS pos=INVALID_RECORD_SLOT_POS);

         INT32 truncate(RECORD_SLOT_POS max);

         void updateAppendingFactor(btreeNodePageHead *head,
                                    BOOLEAN isAppending);

         INT32 _removeChild(RECORD_SLOT_POS pos);

         INT32 _destroySlot(RECORD_SLOT_POS pos);

         INT32 _nonleafRemove(RECORD_SLOT_POS pos);

         INT32 _markRemoved(RECORD_SLOT_POS pos);

      private:
         INT32 _split(RECORD_SLOT_POS pivot,
                      std::unique_ptr<logicalPageBuffer> &rightNodeBuffer);

         INT32 _buildRightNodeWhenSplit(RECORD_SLOT_POS begin,
                                        strictBuffer &node)const;

         INT32 _findSplitPivot(BOOLEAN idleRight,
                               RECORD_SLOT_POS &pivot)const;

         INT32 _saveRaisingEntry(RECORD_SLOT_POS pos,
                                 btreeKeyStringEntry &entry) const;

      private:
         UINT32 _depth = 0;
         logicalPageBuffer *_buffer = nullptr;
         btreeAccessContext *_ctx = nullptr;
         BOOLEAN _inPath = FALSE;
   };//class btreeNode
} // namespace vessel

} // namespace engine


#endif//VESSEL_BTREE_NODE_H_