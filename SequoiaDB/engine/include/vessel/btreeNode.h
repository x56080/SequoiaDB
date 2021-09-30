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
#include "vessel/logicalPageBuffer.h"
#include "vessel/btreeIndexItem.h"

namespace engine
{
namespace vessel
{
   class requestContext;
   class indexContext;
   class indexSpace;

   class btreeNode : public SDBObject
   {
      friend class btreeNodePath;
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
         void reset();

         BOOLEAN isRoot()const;

      public:
         BOOLEAN hasExtNode()const;

         BOOLEAN isLeaf()const;

         UINT32 getTotalSlotCount()const;

         ossSharedLatchMode getMode()const;

         BOOLEAN isPrefixEnabled()const;

      public:
         class locateResult : public SDBObject
         {
            public:
               OSS_INLINE locateResult(){}
               OSS_INLINE locateResult(BOOLEAN k, BOOLEAN i,
                                       BOOLEAN u, RECORD_SLOT_ID s):
                           keyMatched(k),
                           identical(i),
                           upperBound(u),
                           slotNo(s){}
               OSS_INLINE ~locateResult(){}
               locateResult(const locateResult &) = delete;
               OSS_INLINE locateResult &operator=(const locateResult &o)
               {
                  keyMatched = o.keyMatched;
                  identical = o.identical;
                  upperBound = o.upperBound;
                  slotNo = o.slotNo;
                  return *this;
               }

            public:
               BOOLEAN keyMatched = FALSE;
               BOOLEAN identical = FALSE;
               BOOLEAN upperBound = FALSE;
               RECORD_SLOT_ID slotNo = INVALID_RECORD_SLOT_ID;
         };//class locateResult

         INT32 locateKeyAndRid(const ixmKey &key,
                               const recordID &rid,
                               locateResult &result)const;

         INT32 getIndexItem(RECORD_SLOT_ID slotNo,
                            btreeIndexItem &item)const;

         BOOLEAN hasSpaceToInsert(const ixmKey &key)const;

         BOOLEAN tryToEnsureLockExlusive();

         INT32 presplit(PAGE_ID &newPage,
                        btreeIndexItem &pivot);

      private:
         INT32 findSplitPivot(btreeIndexItem &pivot)const;

         INT32 _splitTo(RECORD_SLOT_ID begin,
                        btreeNode &node);

         INT32 pushBackWhenSplit(const btreeIndexItem &item);

      private:
         const btreeNodePageHead *getReadableHead()const;
         btreeNodePageHead *getWritableHead();
         UINT32 getTotalKeyAndSlotSize()const;
         UINT32 getTotalPrefixSize(const btreeNodePageHead *head)const;
         const btreeNodeSlot *getReadableSlot(RECORD_SLOT_ID slotNo)const;
         const btreeNodePrefixSlot *getPrefixReferencedBySlot(RECORD_SLOT_ID slotNo,
                                                              INT32 *which=NULL)const;
         ixmKey getKeyPrefix(INT32 which)const;

         UINT32 getKeyDataOffsetToWrite(UINT32 keyDataSize)const;
      private:
         logicalPageBuffer *_buffer = NULL;
         const indexContext *_ic = NULL;
         UINT32 _depth = 0;
      
   };//class btreeNode
} // namespace vessel

} // namespace engine


#endif//VESSEL_BTREE_NODE_H_