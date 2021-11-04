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
         BOOLEAN isLeaf()const;
         
         UINT32 getItemCount()const;
         UINT32 getNodeSize()const;
         ossSharedLatchMode getLockingMode()const;
         BOOLEAN ensureExclusiveLocking();
         BOOLEAN isItemMarkedAsDeleted(RECORD_SLOT_ID pos)const;
         PAGE_ID getRightChild()const;
         PAGE_ID getLeftChild(RECORD_SLOT_ID pos)const;
         PAGE_ID getChild(RECORD_SLOT_ID pos)const;
         DPS_TRANS_ID getTransID()const;
         UINT32 getSplitedTimes()const;

         const logicalPageBuffer *getBuffer()const
         {
            return _buffer;
         }
         UINT32 getDepth()const
         {
            return _depth;
         }
         btreeItemSlot getItemSlot(RECORD_SLOT_ID pos)const;

      public:

         BOOLEAN hasFreeSpaceToInsert(UINT32 keySize,
                                       BOOLEAN *needCompact=NULL)const;
         BOOLEAN hasFreeSpaceToInsertRaisedKey(UINT32 keySize,
                                               BOOLEAN *needCompact=NULL)const;
         /// leaf node only
         INT32 leafInsert(const ixmKey &key,
                           const recordID &rid,
                           const DPS_TRANS_ID &transID);

         /// leaf node only
         INT32 splitLeafAndInsert(const ixmKey &key,
                                  const recordID &rid,
                                  const DPS_TRANS_ID &transID,
                                  btreeSplitRaisedKey &raisedKey);

         /// non-leaf node or new root only
         INT32 insertRaisedKey(const btreeSplitRaisedKey &raisedKey,
                                 const DPS_TRANS_ID &transID);

         INT32 splitNonLeafAndInsert(const btreeSplitRaisedKey &raisedKeyFromChild,
                                       const DPS_TRANS_ID &transID,
                                       btreeSplitRaisedKey &raisedKey);

         INT32 split(btreeSplitRaisedKey &raisedKey);

         /// non-leaf node only
         INT32 reactiveRemovedKey(const btreeItemLocation &location,
                                    const DPS_TRANS_ID &transID);

         INT32 exchangeWithNewRoot(btreeNode &newRoot);

         INT32 prepareToWrite();

         
      public:
         INT32 locateKeyAndRid(const ixmKey &key,
                                 const recordID &rid,
                                 btreeItemLocation &res)const;

         INT32 getItem(RECORD_SLOT_ID pos,
                        btreeIndexItem &item)const;

         INT32 seek(const BSONObj &prevKey,
                    INT32 fieldCountToCmpInPrev,
                    BOOLEAN exlusive,
                    const VEC_ELE_CMP &matchEle,
                    const inclusiveVec &matchInclusive,
                    INT32 direction,
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

         btreeItemSlot *getWritableSlot(RECORD_SLOT_ID pos);
         const btreeItemSlot *getReadableSlot(RECORD_SLOT_ID pos)const;

         const btreeNodePrefixSlot *getReadablePrefixSlot(UINT16 pos)const;
         btreeNodePrefixSlot *getWritablePrefixSlot(UINT16 pos);

         BOOLEAN hitHighWaterMark()const;
         BOOLEAN isBetterToBeRecompressed() const;

         BOOLEAN isVainPrefixRegen()const;
         BOOLEAN hasCompressedKeys()const;
         BOOLEAN hasPrefix()const;

         const btreeNodePageHead *getReadableHead()const;

         slice getReadableSlice()const;

         BOOLEAN isRecentWriteOrdered()const;

      private:
         void commit();
         void updateTransSN(const DPS_TRANS_ID &transID);

         INT32 _compact(BOOLEAN tryToRestoreExternalKey=TRUE);

         INT32 _leafInsert(const ixmKey &key,
                           const recordID &rid,
                           RECORD_SLOT_ID pos=INVALID_RECORD_SLOT_ID);

         INT32 _insert(RECORD_SLOT_ID pos,
                        const ixmKey &key,
                        const recordID &rid,
                        PAGE_ID leftChild=INVALID_PAGE_ID);

         INT32 _insertRaisedKey(const btreeSplitRaisedKey &raisedKey,
                                 RECORD_SLOT_ID pos=INVALID_RECORD_SLOT_ID);


         INT32 _split(RECORD_SLOT_ID pivot,
                        PAGE_ID &rightNode);

         INT32 insertExternalKey(RECORD_SLOT_ID pos,
                                 const ixmKey &key,
                                 const recordID &rid,
                                 PAGE_ID leftChild);

         INT32 getItemWithExtKey(RECORD_SLOT_ID pos,
                                 btreeIndexItem &item)const;

         INT32 buildRightNodeWhenSplit(RECORD_SLOT_ID pivot,
                                       slice &node)const;

         INT32 findSplitPivot(BOOLEAN idleRight,
                              RECORD_SLOT_ID &pivot)const;

         INT32 truncate(RECORD_SLOT_ID max);

         void updateAppendingFactor(btreeNodePageHead *head,
                                    BOOLEAN isAppending);

         btreeNode getRightNodeWhenSplit(PAGE_ID right, logicalPageBuffer &buffer);

      private:/// leaf node only
         INT32 tryToCompressKeyInserting(RECORD_SLOT_ID pos,
                                          const ixmKey &key,
                                          btreeNodeCompressedKey &ck)const;
         INT32 tryToCompressKey(const ixmKey &key,
                                 UINT32 prefixPos,
                                 const ixmKey &prefix,
                                 btreeNodeCompressedKey &ck)const;

         INT32 insertCompressedKey(RECORD_SLOT_ID pos,
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