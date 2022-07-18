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
#include "vessel/lsm/lsmIndexKey.h"
#include "vessel/globalIndexID.h"
#include "vessel/lsm/lsmColumnFamily.h"

namespace engine
{
namespace vessel
{  
   class indexObject;

   class lsmIndexIterator : public indexIterator
   {
      public:
         lsmIndexIterator();
         virtual ~lsmIndexIterator();

      public:
         INT32 init(const lsmColumnFamily &cf,
                    const globalLogicalClId &cl,
                    const indexObject *obj,
                    const options &o);
         
      public:
         virtual const CHAR *getName()const override {return "lsmIndexIterator";}

         virtual void reset() override;

         virtual BOOLEAN isReadyToRead()const override;

         virtual INT32 seek(const bson::BSONObj &prevKey,
                            INT32 fieldCountToCmpInPrev,
                            const VEC_ELE_CMP &matchEles,
                            const inclusiveVec &matchInclusive,
                            const seekOptions &o) override;

         virtual INT32 seekKey(const ixmKey &key,
                               const seekOptions &o) override;

         virtual INT32 locate(const slice &encodedKey,
                              const recordID &rid,
                              const seekOptions &o) override;

         virtual INT32 next() override;

         virtual INT32 advance(const bson::BSONObj &prevKey,
                               INT32 fieldCountToCmpInPrev,
                               const VEC_ELE_CMP &matchEles,
                               const inclusiveVec &matchInclusive,
                               const seekOptions &o) override;
      public:
         virtual DPS_LSN_OFFSET getLSN()const override;
         virtual bson::BSONObj getKeyObj(bson::BufBuilder *builder)const override;
         virtual DPS_TRANS_ID getTransID()const override;
         virtual recordID getRid()const override;
         virtual BOOLEAN equals(const ixmKey &key)const override;
         virtual slice getEncodedKey()const override;

      private:
         INT32 seekFullKey(const rocksdb::Slice &fullKey,
                           BOOLEAN forPrev);

         INT32 moveIterator(BOOLEAN forward);

         INT32 forwardToNextVisiblePostion();

         INT32 ensureBackwardToVisiblePosition();

         INT32 moveToNextVisiblePosition();

         INT32 ensureVisiblePosition();

         OSS_INLINE BOOLEAN _isValid()const {return nullptr != _itr;}

         BOOLEAN _isReadyToRead()const;

         BOOLEAN _isMarkedRemoved(rocksdb::Iterator *itr)const;

         void _initKeyBoundWhenOpen(const globalIndexID &id);

         INT32 _cacheBackwardEntry(const rocksdb::Slice &s);

         INT32 _ensureBackwardEntryCache(UINT32 size);

      private:
         options _o;
         lsmColumnFamily _cf;
         const indexObject *_obj = nullptr;
         globalIndexID _globalId;
         rocksdb::Iterator *_itr = nullptr;
         LSM_IDX_KEY_BOUNDARY _lowBound;
         LSM_IDX_KEY_BOUNDARY _upBound;
         rocksdb::Slice _lowKey;
         rocksdb::Slice _upKey;
         lsmPureKeyEntry _currentEntry;
         CHAR *_backwardEntryCache = nullptr;
         UINT32 _backwardEntryCacheSize = 0;
         UINT32 _backwardEntrySize = 0;
   };//class lsmIndexIterator
}//namespace vessel
}//namespace engine

#endif//VESSEL_LSM_INDEX_ITERATOR_H_