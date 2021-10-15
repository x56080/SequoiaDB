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
#include "vessel/btreeIndexDef.h"
#include "vessel/btreeIndexItem.h"
#include "vessel/btreeNodeCompressedKey.h"
#include "vessel/strictPointer.h"

namespace engine
{
namespace vessel
{
   class requestContext;
   class indexContext;
   class indexSpace;
   class logicalPageBuffer;

   class btreeNode : public SDBObject
   {
      friend class btreeAccessContext;
      public:
         btreeNode(){}
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

      private:
         explicit btreeNode(logicalPageBuffer *buffer,
                            const indexContext *ic,
                            UINT32 depth);

      public:
         OSS_INLINE BOOLEAN isValid()const
         {
            return NULL != _buffer && _buffer->isValid();
         }

         OSS_INLINE UINT32 getDepth()const
         {
            return _depth;
         }
      public:
         BOOLEAN isRoot()const;

         BOOLEAN hasExtNode()const;

         BOOLEAN isLeaf()const;

         UINT32 getTotalSlotCount()const;

         ossSharedLatchMode getMode()const;

         BOOLEAN hasPrefixes()const;

         BOOLEAN isCompressionDisabled()const;

         UINT32 getNodeSize()const;

      public:
         void reset();

         class locateResult : public SDBObject
         {
            public:
               OSS_INLINE locateResult(){}
               OSS_INLINE locateResult(BOOLEAN k, BOOLEAN i,
                                       BOOLEAN u, RECORD_SLOT_ID s):
                           keyMatched(k),
                           identical(i),
                           upperBound(u),
                           slotPos(s){}
               OSS_INLINE ~locateResult(){}
               locateResult(const locateResult &) = delete;
               OSS_INLINE locateResult &operator=(const locateResult &o)
               {
                  keyMatched = o.keyMatched;
                  identical = o.identical;
                  upperBound = o.upperBound;
                  slotPos = o.slotPos;
                  return *this;
               }

            public:
               BOOLEAN keyMatched = FALSE;
               BOOLEAN identical = FALSE;
               BOOLEAN upperBound = FALSE;
               RECORD_SLOT_ID slotPos = INVALID_RECORD_SLOT_ID;
         };//class locateResult

         INT32 locateKeyAndRid(const ixmKey &key,
                               const recordID &rid,
                               locateResult &result)const;

         INT32 getIndexItem(RECORD_SLOT_ID slotNo,
                            btreeIndexItem &item)const;

         BOOLEAN tryToEnsureLockExlusive();

      public:/// leaf node only

         /// always check free space first.
         INT32 insert(RECORD_SLOT_ID pos,
                      const ixmKey &key,
                      const recordID &rid,
                      const DPS_TRANS_ID *transID=NULL);

         BOOLEAN hasSpaceToInsert(const ixmKey &key)const;

      public:
         enum SPLIT_MODE
         {
            SPLIT_MODE_AVERAGE = 0,
            SPLIT_MODE_IDLE_RIGHT = 1,
            //SPLIT_MODE_IDLE_LEFT = 2,
         };//enum SPLIT_MODE

         INT32 beginToSplit(PAGE_ID &rightNode,
                            btreeIndexItem &pivot,
                            const SPLIT_MODE sm=SPLIT_MODE_AVERAGE)const;

      private:
         INT32 findSplitPivot(const SPLIT_MODE sm,
                              btreeIndexItem &pivot)const;

         INT32 splitTo(UINT32 bufferSize, CHAR *buffer, RECORD_SLOT_ID begin)const;

      private:

         //const btreeNodePageHead *getReadableHead()const;
         btreeNodePageHead *getWritableHead();
         UINT32 getTotalKeyAndSlotSize()const;
         UINT32 getTotalPrefixSize(const btreeNodePageHead *head)const;
         const btreeItemSlot *getReadableSlot(RECORD_SLOT_ID pos)const;
         btreeItemSlot *getWritableSlot(RECORD_SLOT_ID pos);
         UINT32 getSizeToSave(const ixmKey &key, UINT32 *keySize=NULL)const;
         
         const CHAR *getPrefixData(UINT32 prefixSlotPos)const;

         BOOLEAN isPreifxRegenVain()const;

      class __btreeNode : public SDBObject
      {
         public:
            __btreeNode(){}
            explicit __btreeNode(const strictPointer &ptr,
                                 const indexContext *ic,
                                 UINT32 depth);
            ~__btreeNode(){}

            __btreeNode(const __btreeNode &o):
            _ptr(o._ptr), _ic(o._ic), _depth(o._depth){}

            __btreeNode &operator=(const __btreeNode &o)
            {
               _ptr = o._ptr;
               _ic = o._ic;
               _depth = o._depth;
               return *this;
            }

         public:
            OSS_INLINE BOOLEAN isValid()const
            {
               return _ptr.isValid();
            }

            OSS_INLINE void reset()
            {
               _ptr.reset();
               _ic = NULL;
               _depth = 0;
               return;
            }

            OSS_INLINE const btreeNodePageHead *getReadableHead()const
            {
               SDB_ASSERT(isValid(), "can not be invalid");
               return _ptr.getReadableObjPtr<btreeNodePageHead>(0);
            }
            

         public:
            BOOLEAN isRoot()const;
            BOOLEAN hasExtNode()const;
            BOOLEAN isLeaf()const;
            BOOLEAN isVainPrefixRegen()const;
            UINT32 getSlotCount()const;
            BOOLEAN hasSpaceToInsert(UINT32 keySize,
                                     BOOLEAN *needCompact=NULL)const;
            BOOLEAN hasPrefix()const;
            UINT32 getCompressedKeyCount()const;
            BOOLEAN hitHighWaterMark()const;
            UINT32 getNodeSize()const;
            
         public: /// leaf node only
            INT32 insert(RECORD_SLOT_ID pos,
                         const ixmKey &key,
                         const recordID &rid,
                         const DPS_TRANS_ID *transID);

         private:
            UINT32 getSizeToSaveInNode(UINT32 keySize)const;
            BOOLEAN isCompressionDisabled()const;
            UINT32 getKeyDataOffsetToWrite(const btreeNodePageHead *head,
                                           UINT32 keyDataSize)const;

            btreeItemSlot *getWritableSlot(RECORD_SLOT_ID pos);
            const btreeItemSlot *getReadableSlot(RECORD_SLOT_ID pos)const;

            BOOLEAN hitHighWaterMark()const;
            BOOLEAN isBetterToBeRecompressed() const;
            UINT32 getOptimizedSizeByComprssion()const;

         private:
            INT32 compact();

            INT32 _insert(RECORD_SLOT_ID pos,
                          const ixmKey &key,
                          const recordID &rid,
                          PAGE_ID leftChild,
                          const DPS_TRANS_ID *transID);

         private:/// leaf node only
            INT32 tryToCompressKeyInserting(RECORD_SLOT_ID pos,
                                            const ixmKey &key,
                                            btreeNodeCompressedKey &ck)const;
            INT32 tryToCompressKey(const ixmKey &key,
                                   UINT32 prefixPos,
                                   const ixmKey &prefix,
                                   btreeNodeCompressedKey &ck)const;

            BOOLEAN recompress();

            INT32 insertCompressedKey(RECORD_SLOT_ID pos,
                                      const btreeNodeCompressedKey &ck,
                                      const recordID &rid,
                                      const DPS_TRANS_ID *transID);
         private:
            strictPointer _ptr;
            const indexContext *_ic = NULL;
            UINT32 _depth = 0;
      };//class __btreeNode

      private:
         __btreeNode getReadableNode()const;
         __btreeNode getWritableNode();

      private:
         logicalPageBuffer *_buffer = NULL;
         const indexContext *_ic = NULL;
         UINT32 _depth = 0;
      
   };//class btreeNode
} // namespace vessel

} // namespace engine


#endif//VESSEL_BTREE_NODE_H_