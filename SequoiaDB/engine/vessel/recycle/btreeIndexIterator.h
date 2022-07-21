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
   class indexObject;

   class btreeIndexIterator : public indexIterator
   {
      public:
         btreeIndexIterator() = default;
         virtual ~btreeIndexIterator();

      public:
         INT32 init(const options &o,
                    indexObject *obj,
                    indexSpaceAccessCtx &&ctx);

      public:
         virtual INDEX_ITERATOR_TYPE getType() const override
         {
            return INDEX_ITERATOR_TYPE::BTREE;
         }

         virtual void reset() override;

         virtual INT32 seek(const bson::BSONObj &prevKey,
                            INT32 fieldCountToCmpInPrev,
                            const VEC_ELE_CMP &matchEles,
                            const inclusiveVec &matchInclusive,
                            const seekOptions &o) override;

         virtual INT32 seekKey(const ixmKey &key,
                               const seekOptions &o) override;

         virtual INT32 locate(const slice &keyString,
                              const bson::BSONObj &info,
                              const seekOptions &o) override;

         virtual INT32 next() override;

         virtual INT32 advance(const bson::BSONObj &prevKey,
                               INT32 fieldCountToCmpInPrev,
                               const VEC_ELE_CMP &matchEles,
                               const inclusiveVec &matchInclusive,
                               const seekOptions &o) override;

         virtual BOOLEAN isReadyToRead()const;

      public:
         virtual bson::BSONObj getKeyObj(bson::BufBuilder *builder)const  override;
         virtual slice getKeyString()const override;
         virtual DPS_LSN_OFFSET getLSN()const override;
         virtual DPS_TRANS_ID getTransID()const override;
         virtual recordID getRid()const override;
         virtual BOOLEAN equals(const ixmKey &key)const override;
         virtual bson::BSONObj getLocationInfo()const override;

      private:
         OSS_INLINE BOOLEAN hasLocation()const
         {
            return isValidRecordSlotPosition(_pos);
         }
         INT32 locateKeyInTree(const bson::BSONObj &prevKey,
                               INT32 fieldCountToCmpInPrev,
                               const VEC_ELE_CMP &matchEles,
                               const inclusiveVec &matchInclusive,
                               const seekOptions &o);

         INT32 locateKeyInSubTree(const bson::BSONObj &prevKey,
                                  INT32 fieldCountToCmpInPrev,
                                  const VEC_ELE_CMP &matchEles,
                                  const inclusiveVec &matchInclusive,
                                  const seekOptions &o);

         INT32 advanceInSubTree(const bson::BSONObj &prevKey,
                                 INT32 fieldCountToCmpInPrev,
                                 const VEC_ELE_CMP &matchEles,
                                 const inclusiveVec &matchInclusive,
                                 const seekOptions &o);

         INT32 relocateKeyAndRidInTree(const ixmKey &key,
                                       const recordID &rid,
                                       btreeItemLocation &location);
      private:
         void resetLocation();

         void resetPositionOfCurrentNode(RECORD_SLOT_POS pos);

         void clearPositionOfCurrentNode();

         INT32 cacheCurrentItem();

         INT32 prepareToGoBackToAncestors(BOOLEAN &obstructed,
                                          INT32 &ancestorDepth,
                                          BOOLEAN &footPrintIsFaithFul);

         BOOLEAN isCurrentItemMarkedDeleted();

         INT32 nextAtLeaf(BOOLEAN &obstructed);

         INT32 nextAtNonLeaf(BOOLEAN &obstructed);

         INT32 goBackToAncestor(BOOLEAN &obstructed);

         INT32 findPosInAncestor(UINT32 ancestorDepth,
                                 RECORD_SLOT_POS &pos);

         INT32 traverseDownToBottom(const btreeItemLocation &location);

      private:

         recordID getCurrentIndexRid()const;

      private:
         options _o;
         btreeAccessContext _ctx;
         RECORD_SLOT_POS _pos = INVALID_RECORD_SLOT_POS;
         btreeIndexItem _item;
         bson::BufBuilder _builder;
   };//class btreeIndexIterator
} // namespace vessel

} // namespace engine


#endif//VESSEL_BTREE_INDEX_ITERATOR_H_