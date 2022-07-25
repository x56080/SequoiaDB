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

   Source File Name = lsmIndexIterator.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#ifndef VESSEL_LSM_INDEX_ITERATOR_H_
#define VESSEL_LSM_INDEX_ITERATOR_H_

#include "vessel/indexIterator.h"
#include "vessel/globalIndexID.h"
#include "vessel/lsm/lsmColumnFamily.h"
#include "vessel/objectIdentifier.h"
#include "vessel/lsm/lsmIndexMetaKey.h"
#include "vessel/keyString.h"

namespace engine
{
namespace vessel
{  
   class indexObject;

   class lsmIndexIterator : public indexIterator
   {
      public:
         lsmIndexIterator() = default;
         virtual ~lsmIndexIterator();

      public:
         INT32 init(const lsmColumnFamily &cf,
                    const globalLogicalClId &cl,
                    const indexObject *obj);
         
      public:
         virtual INDEX_ITERATOR_TYPE getType() const override {return INDEX_ITERATOR_TYPE::LSM;}

         virtual void reset() override;

         virtual BOOLEAN isReadyToRead()const override;

         virtual INT32 seek(const VEC_ELE_CMP &eles,
                            const inclusiveVec &iv,
                            const options &o) override;

         virtual INT32 seek(const bson::BSONObj &key,
                            const inclusiveVec &iv,
                            const options &o) override;

         virtual INT32 equal(const VEC_ELE_CMP &matchEles) override;
         virtual INT32 equal(const bson::BSONObj &key) override;

         virtual INT32 locateNext(const indexEntryLocation *location,
                                  const options &o) override;

         virtual INT32 next() override;

         virtual INT32 advance(const bson::BSONObj &prevKey,
                               INT32 fieldCountToCmpInPrev,
                               const VEC_ELE_CMP &matchEles,
                               const inclusiveVec &matchInclusive) override;

         virtual INT32 pause() override;

      public:
         virtual bson::BSONObj getKeyObj(BOOLEAN withFieldName,
                                         bson::BufBuilder *buf)const;
         virtual DPS_LSN_OFFSET getLSN()const override;
         virtual DPS_TRANS_ID getTransID()const override;
         virtual recordID getRid()const override;
         virtual INT32 initOrUpdateLocation(IDX_ENTRY_LOCATION_UPTR &location) const override;

      public:
         BOOLEAN isMarkedRemoved()const;

      private:
         INT32 _reinitIterator(const options &o, BOOLEAN allInclusive);

         INT32 _moveIterator();

         OSS_INLINE BOOLEAN _isInternalItrReady()const {return nullptr != _itr;}

         OSS_INLINE BOOLEAN _isInited()const {return nullptr != _obj;}

         BOOLEAN _isReadyToRead()const;

         BOOLEAN _isMarkedRemoved(rocksdb::Iterator *itr)const;

         void _initKeyBoundWhenOpen(const globalIndexID &id);

         INT32 _seekKeyString(const keyString &ks,
                              BOOLEAN forPrev);

         void _resetCurrentEntry();

         INT32 _initCurrentEntry(const rocksdb::Iterator *itr);

         INT32 _locateNextEntry(const keyString &entryKey);

      private:
         options _o;
         lsmColumnFamily _cf;
         const indexObject *_obj = nullptr;
         globalIndexID _globalId;
         rocksdb::Iterator *_itr = nullptr;
         lsmIndexIdKey _lowBound;
         lsmIndexIdKey _upBound;
         rocksdb::Slice _lowKey;
         rocksdb::Slice _upKey;
         keyString _ks;
   };//class lsmIndexIterator
}//namespace vessel
}//namespace engine

#endif//VESSEL_LSM_INDEX_ITERATOR_H_