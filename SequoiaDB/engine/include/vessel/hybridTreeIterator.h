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

   Source File Name = hybridTreeIterator.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#ifndef VESSEL_HYBRID_TREE_ITERATOR_H_
#define VESSEL_HYBRID_TREE_ITERATOR_H_

#include "vessel/indexIterator.h"
#include "vessel/lsm/lsmIndexIterator.h"

namespace engine
{
namespace vessel
{
   class hybridTreeIterator : public indexIterator
   {
      public:
         hybridTreeIterator() = default;
         virtual ~hybridTreeIterator();

      public:
         INT32 init(const lsmColumnFamily &cf,
                    const globalLogicalClId &cl,
                    const indexObject *obj);

      public:
          virtual INDEX_ITERATOR_TYPE getType() const override {return INDEX_ITERATOR_TYPE::HYBRID_TREE;} 
          virtual void reset() override;

      public:/// init iterator location. 
         virtual INT32 seek(const VEC_ELE_CMP &eles,
                            const inclusiveVec &iv,
                            const options &o) override;
                            
         virtual INT32 seek(const bson::BSONObj &key,
                            const inclusiveVec &iv,
                            const options &o) override;

         virtual INT32 equal(const VEC_ELE_CMP &matchEles) override;
         virtual INT32 equal(const bson::BSONObj &key) override;

         /// locate
         virtual INT32 locateNext(const indexEntryLocation *location,
                                  const options &o) override;

      public:
         virtual INT32 next();

         /// reseek from current pos, to fast skip unmatched entries.
         virtual INT32 advance(const bson::BSONObj &prevKey,
                               INT32 fieldCountToCmpInPrev,
                               const VEC_ELE_CMP &matchEles,
                               const inclusiveVec &iv) override;

         virtual BOOLEAN isReadyToRead()const override;

         virtual INT32 pause();

      public:
         virtual bson::BSONObj getKeyObj(BOOLEAN withFieldName,
                                         bson::BufBuilder *buf)const override;
         virtual DPS_LSN_OFFSET getLSN()const override;
         virtual DPS_TRANS_ID getTransID()const override;
         virtual recordID getRid()const override;
         virtual INT32 initOrUpdateLocation(IDX_ENTRY_LOCATION_UPTR &location) const override;

      private:
         lsmIndexIterator _lsm;

   };//class hybridTreeIterator
} // namespace vessel

} // namespace engine


#endif//VESSEL_HYBRID_TREE_ITERATOR_H_