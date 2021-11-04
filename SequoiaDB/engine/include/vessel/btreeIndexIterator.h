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

   Source File Name = btreeIndexIterator.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#ifndef VESSEL_BTREE_INDEX_ITERATOR_H_
#define VESSEL_BTREE_INDEX_ITERATOR_H_

#include "vessel/indexIterator.h"
#include "../bson/util/builder.h"
#include "vessel/btreeAccessContext.h"

namespace engine
{
namespace vessel
{
   class requestContext;

   class btreeIndexIterator : public indexIterator
   {
      public:
         btreeIndexIterator();
         virtual ~btreeIndexIterator();

      public:
         virtual INDEX_TYPE getIndexType()const {return INDEX_TYPE_BTREE;}

      public:
         virtual BOOLEAN isOpen()const
         {
            return NULL != _context;
         }

         virtual INT32 open(requestContext *context,
                            indexContext *ic);

         virtual void close();

         virtual INT32 seek(const bson::BSONObj &prevKey,
                            INT32 fieldCountToCmpInPrev,
                            const VEC_ELE_CMP &matchEles,
                            const inclusiveVec &matchInclusive,
                            const options &o);

         virtual INT32 seekEntry(const slice &entry,
                                 const options &o);

         virtual INT32 seekKey(const ixmKey &key,
                               const options &o);

         virtual INT32 next(BOOLEAN forward);

         virtual INT32 seekFromCurrentPosition(const bson::BSONObj &prevKey,
                                               INT32 fieldCountToCmpInPrev,
                                               const VEC_ELE_CMP &matchEles,
                                               const inclusiveVec &matchInclusive,
                                               const options &o);

         virtual BOOLEAN isReadyToRead()const;

         virtual INT32 contains(const ixmKey &key, recordID &rid);

         virtual void pause();
      public:
         virtual UINT64 getLSN()const;
         virtual slice getKey()const;
         virtual DPS_TRANS_ID getTransID()const;
         virtual recordID getRid()const;
         virtual BOOLEAN equalToCurrentKey(const ixmKey &key)const;
         virtual INT32 pushCurrentEntryToBatch(indexScanEntryBatch &batch)const;
         //virtual indexScanEntry getCurrentEntry()const;

      private:
         OSS_INLINE BOOLEAN hasLocation()const
         {
            return INVALID_RECORD_SLOT_ID != _pos;
         }
         INT32 locateKeyInTree(const bson::BSONObj &prevKey,
                               INT32 fieldCountToCmpInPrev,
                               const VEC_ELE_CMP &matchEles,
                               const inclusiveVec &matchInclusive,
                               const options &o);

         INT32 relocateKeyAndRidInTree(const ixmKey &key,
                                       const recordID &rid,
                                       btreeItemLocation &location);

         INT32 relocateAndMoveToNext(BOOLEAN forward);

      private:
         void resetLocation();

         void resetPositionOfCurrentNode(RECORD_SLOT_ID pos);

         void clearPositionOfCurrentNode();

         INT32 cacheCurrentItem();

         INT32 prepareToGoBackToAncestors(BOOLEAN forward,
                                          BOOLEAN &obstructed,
                                          INT32 &ancestorDepth,
                                          BOOLEAN &footPrintIsFaithFul);

         BOOLEAN isCurrentItemMarkedDeleted();

         INT32 nextAtLeaf(BOOLEAN forward, BOOLEAN &obstructed);

         INT32 nextAtNonLeaf(BOOLEAN forward,
                             BOOLEAN &obstructed);

         INT32 goBackToAncestor(RECORD_SLOT_ID pos,
                                BOOLEAN forward,
                                BOOLEAN &obstructed);

         INT32 findPosInAncestor(UINT32 ancestorDepth,
                                 RECORD_SLOT_ID &pos);

      private:

         recordID getCurrentIndexRid()const;

         slice getOriginalKeySlice()const;

      private:
         requestContext *_context = NULL;
         btreeAccessContext _bac;
         RECORD_SLOT_ID _pos = INVALID_RECORD_SLOT_ID;
         btreeIndexItem _item;
         bson::StackBufBuilder _originalKeyBuffer;
         bson::BufBuilder _builder;
   };//class btreeIndexIterator
} // namespace vessel

} // namespace engine


#endif//VESSEL_BTREE_INDEX_ITERATOR_H_