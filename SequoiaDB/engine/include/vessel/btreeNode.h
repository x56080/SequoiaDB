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
#include "vessel/logicalPageBuffer.h"

namespace engine
{
namespace vessel
{
   class indexContext;

   class btreeNode : public SDBObject
   {
      public:
         btreeNode(){}
         explicit btreeNode(logicalPageBuffer *buffer,
                            const indexContext *ic,
                            UINT32 depth);
         ~btreeNode(){}
         btreeNode(const btreeNode &o):
         _buffer(o._buffer),
         _ic(o._ic),
         _depth(o._depth)
         {}
         btreeNode &operator=(const btreeNode &o)
         {
            _buffer = o._buffer;
            _ic = o._ic;
            _depth = o._depth;
            return *this;
         }

      public:
         OSS_INLINE BOOLEAN isValid()const
         {
            return NULL != _buffer && _buffer->isValid();
         }

         OSS_INLINE void reset()
         {
            _buffer = NULL;
            _ic = NULL;
            _depth = 0;
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
         public:
            /// leaf node only
            BOOLEAN hasFreeSpaceToInsert(UINT32 keySize,
                                         BOOLEAN *needCompact=NULL)const;

            /// leaf node only
            INT32 insert(const ixmKey &key,
                         const recordID &rid,
                         const DPS_TRANS_ID &transID);

            /// leaf node only
            INT32 insert(RECORD_SLOT_ID pos,
                         const ixmKey &key,
                         const recordID &rid,
                         const DPS_TRANS_ID &transID);

            /// non-leaf node only
            INT32 insertRaisedKey(RECORD_SLOT_ID pos,
                                  const ixmKey &key,
                                  const recordID &rid,
                                  const DPS_TRANS_ID &transID,
                                  PAGE_ID leftChild,
                                  PAGE_ID rightChild);

            /// non-leaf node only
            INT32 reactiveRemovedKey(const btreeItemLocation &location,
                                     const DPS_TRANS_ID &transID);
         public:
            INT32 locateKeyAndRid(const ixmKey &key,
                                  const recordID &rid,
                                  btreeItemLocation &res)const;

            INT32 getItem(RECORD_SLOT_ID pos,
                          btreeIndexItem &item)const;

            /// leaf: do 50-50 split when insert not ordered
            ///       else do 90-10 split
            /// non-leaf: always do 50-50 split
            INT32 presplit(btreeIndexItem &pivot,
                           PAGE_ID &rightNode,
                           BOOLEAN orderedInsert=FALSE)const;

         private:
            UINT32 getSizeToSaveInNode(UINT32 keySize)const;
            BOOLEAN isCompressionDisabled()const;
            UINT32 getKeyDataOffsetToWrite(const btreeNodePageHead *head,
                                           UINT32 keyDataSize)const;

            btreeItemSlot *getWritableSlot(RECORD_SLOT_ID pos);
            const btreeItemSlot *getReadableSlot(RECORD_SLOT_ID pos)const;

            BOOLEAN hitHighWaterMark()const;
            BOOLEAN isBetterToBeRecompressed() const;
            UINT32 getOptimizedSizeByCompression()const;
            UINT32 getTotalKeyDataAndSlotSize()const;
            UINT32 getTotalKeyPrefixSize()const;

            BOOLEAN isVainPrefixRegen()const;
            BOOLEAN hasPrefix()const;

            OSS_INLINE const btreeNodePageHead *getReadableHead()const
            {
               SDB_ASSERT(isValid(), "can not be invalid");
               return _buffer->getReadableBodySlice().getReadableObjPtr<btreeNodePageHead>(0);
            }

            OSS_INLINE slice getReadableSlice()const
            {
               SDB_ASSERT(isValid(), "can not be invalid");
               return _buffer->getReadableBodySlice();
            }

            void commit();

            void updateTransSN(const DPS_TRANS_ID &transID);

         private:
            INT32 compact();

            INT32 _insert(RECORD_SLOT_ID pos,
                          const ixmKey &key,
                          const recordID &rid,
                          PAGE_ID leftChild=INVALID_PAGE_ID);

            INT32 insertExternalKey(RECORD_SLOT_ID pos,
                                    const ixmKey &key,
                                    PAGE_ID leftChild);

            INT32 findSplitPivot(UINT32 factor,
                                 btreeIndexItem &pivot)const;

            INT32 buildSplitNode(RECORD_SLOT_ID begin, slice &s)const;

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

            static void referenceToPrefix(btreeNodePrefixSlot &ps,
                                          UINT32 suffixSize);

      protected:
         logicalPageBuffer *_buffer = NULL;
         const indexContext *_ic = NULL;
         UINT32 _depth = 0;
   };//class btreeNode
} // namespace vessel

} // namespace engine


#endif//VESSEL_BTREE_NODE_H_