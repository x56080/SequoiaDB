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
#include "ixmKey.hpp"
#include "vessel/btreeIndexItem.h"
#include "vessel/btreeNodeCompressedKey.h"
#include "vessel/btreeItemLocation.h"
#include "ossSharedLatch.hpp"
#include "vessel/btreeSplitRaisedKey.h"
#include "rtnPredicate.hpp"
#include "vessel/strictBuffer.h"

namespace engine
{
namespace vessel
{
   class logicalPageBuffer;
   class indexContext;

   class btreeNode : public SDBObject
   {
      public:
         btreeNode(){}

         explicit btreeNode(logicalPageBuffer *buffer,
                            UINT32 depth,
                            const indexContext *ic);

         ~btreeNode(){}
         btreeNode(const btreeNode &o):
         _buffer(o._buffer),
         _depth(o._depth),
         _ic(o._ic)
         {}
         btreeNode &operator=(const btreeNode &o)
         {
            _buffer = o._buffer;
            _depth = o._depth;
            _ic = o._ic;
            return *this;
         }

      public:
         OSS_INLINE BOOLEAN isValid()const
         {
            return NULL != _buffer;
         }

         OSS_INLINE void reset()
         {
            _depth = 0;
            _buffer = NULL;
            _ic = NULL;
            return;
         }

      public:
         BOOLEAN isRoot()const;
         BOOLEAN hasExternalKey()const;
         PAGE_ID getExternalKeyPage()const;
         BOOLEAN isLeaf()const;
         
         UINT32 getItemCount()const;
         UINT32 getNodeSize()const;
         ossSharedLatchMode getLockingMode()const;
         BOOLEAN ensureExclusiveLocking();
         BOOLEAN isItemMarkedAsDeleted(RECORD_SLOT_POS pos)const;
         PAGE_ID getRightChild()const;
         BOOLEAN hasRightChild()const;
         PAGE_ID getLeftChild(RECORD_SLOT_POS pos)const;
         PAGE_ID getChild(RECORD_SLOT_POS pos)const;
         DPS_TRANS_ID getTransID()const;
         UINT32 getSplitedTimes()const;

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
         INT32 prepareToWrite();

      public:

         BOOLEAN hasFreeSpaceToInsert(UINT32 keySize,
                                       BOOLEAN *needCompact=NULL)const;
         BOOLEAN hasFreeSpaceToInsertRaisedKey(UINT32 keySize,
                                               BOOLEAN *needCompact=NULL)const;
         /// leaf node only
         INT32 leafInsert(const ixmKey &key,
                           const recordID &rid);

         /// leaf node only
         INT32 splitLeafAndInsert(const ixmKey &key,
                                  const recordID &rid,
                                  btreeSplitRaisedKey &raisedKey);

         /// non-leaf node only
         INT32 insertRaisedKey(const btreeSplitRaisedKey &raisedKey);

         INT32 insertRaisedKeyAsExtKey(const btreeSplitRaisedKey &raisedKey);

         INT32 splitNonLeafAndInsert(const btreeSplitRaisedKey &raisedKeyFromChild,
                                       btreeSplitRaisedKey &raisedKey);

         INT32 split(btreeSplitRaisedKey &raisedKey);

         /// non-leaf node only
         INT32 reactiveRemovedKey(const btreeItemLocation &location);

         INT32 exchangeWithNewRoot(btreeNode &newRoot);

         INT32 resetRemovedChild(RECORD_SLOT_POS pos,
                                 PAGE_ID child);

         INT32 resetAsEmptyNode();
      public:
         /// non-leaf only
         INT32 nonleafRemove(RECORD_SLOT_POS pos);

         INT32 leafRemove(RECORD_SLOT_POS pos);

         /// non-leaf node only
         /// when pos equals to item count in node,
         /// remove right child
         INT32 removeChild(RECORD_SLOT_POS pos);
         
      public:
         INT32 locateKeyAndRid(const ixmKey &key,
                                 const recordID &rid,
                                 btreeItemLocation &res)const;

         INT32 getItem(RECORD_SLOT_POS pos,
                        btreeIndexItem &item)const;

         INT32 keyLocate(const BSONObj &prevKey,
                         INT32 fieldCountToCmpInPrev,
                         const VEC_ELE_CMP &matchEle,
                         const inclusiveVec &matchInclusive,
                         BOOLEAN exclusive,
                         BOOLEAN forward,
                         btreeItemLocation &location,
                         BOOLEAN &outOfBound,
                         bson::BufBuilder *bb=NULL);

         INT32 keyAdvance(RECORD_SLOT_POS pos,
                          const BSONObj &prevKey,
                          INT32 fieldCountToCmpInPrev,
                          const VEC_ELE_CMP &matchEle,
                          const inclusiveVec &matchInclusive,
                          BOOLEAN exclusive,
                          BOOLEAN forward,
                          BOOLEAN &goBackToFather,
                          btreeItemLocation &location,
                          bson::BufBuilder *bb=NULL);

      private:
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

         const btreeNodePageHead *getReadableHead()const;

         strictBuffer getReadableBuffer()const;

         BOOLEAN isRecentWriteOrdered()const;

         INT32 find(RECORD_SLOT_POS low,
                    RECORD_SLOT_POS high,
                    const bson::BSONObj &prevKey,
                    INT32 fieldCountToCmpInPrev,
                    const VEC_ELE_CMP &matchEle,
                    const inclusiveVec &matchInclusive,
                    BOOLEAN exclusive,
                    BOOLEAN forward,
                    bson::BufBuilder &bb,
                    RECORD_SLOT_POS &pos)const;

      private:
         void commit();
         void updateTransID(const DPS_TRANS_ID &transID);

         INT32 _compact(UINT32 reserved);

         INT32 _leafInsert(const ixmKey &key,
                           const recordID &rid,
                           RECORD_SLOT_POS pos=INVALID_RECORD_SLOT_POS);

         INT32 _insert(RECORD_SLOT_POS pos,
                        const ixmKey &key,
                        const recordID &rid,
                        PAGE_ID leftChild=INVALID_PAGE_ID);

         INT32 _insertRaisedKey(const btreeSplitRaisedKey &raisedKey,
                                 RECORD_SLOT_POS pos=INVALID_RECORD_SLOT_POS);

         INT32 _insertExternalKey(const btreeSplitRaisedKey &raisedKey,
                                  RECORD_SLOT_POS pos=INVALID_RECORD_SLOT_POS);


         INT32 _splitAndCompact(RECORD_SLOT_POS pivot,
                                PAGE_ID &rightNode);

         INT32 insertExternalKey(RECORD_SLOT_POS pos,
                                 const ixmKey &key,
                                 const recordID &rid,
                                 PAGE_ID leftChild);

         INT32 getItemWithExtKey(RECORD_SLOT_POS pos,
                                 btreeIndexItem &item)const;

         INT32 buildRightNodeWhenSplit(RECORD_SLOT_POS begin,
                                       strictBuffer &node)const;

         INT32 findSplitPivot(BOOLEAN idleRight,
                              RECORD_SLOT_POS &pivot)const;

         INT32 truncate(RECORD_SLOT_POS max);

         void updateAppendingFactor(btreeNodePageHead *head,
                                    BOOLEAN isAppending);

         btreeNode getRightNodeWhenSplit(PAGE_ID right, logicalPageBuffer &buffer);

      private:

         INT32 _removeChild(RECORD_SLOT_POS pos);

         INT32 _destroySlot(RECORD_SLOT_POS pos);

         INT32 _nonleafRemove(RECORD_SLOT_POS pos);

         INT32 _markRemoved(RECORD_SLOT_POS pos);

      private:/// leaf node only
         INT32 tryToCompressKeyInserting(RECORD_SLOT_POS pos,
                                          const ixmKey &key,
                                          btreeNodeCompressedKey &ck)const;
         INT32 tryToCompressKey(const ixmKey &key,
                                 UINT32 prefixPos,
                                 const ixmKey &prefix,
                                 btreeNodeCompressedKey &ck)const;

         INT32 insertCompressedKey(RECORD_SLOT_POS pos,
                                    const btreeNodeCompressedKey &ck,
                                    const recordID &rid);

         INT32 recompress(BOOLEAN &recompressed);
      private:
         logicalPageBuffer *_buffer = NULL;
         UINT32 _depth = 0;
         const indexContext *_ic = NULL;
   };//class btreeNode
} // namespace vessel

} // namespace engine


#endif//VESSEL_BTREE_NODE_H_