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
#include "rocksdb/iterator.h"
#include "vessel/lsm/lsmIndexKey.h"
#include "vessel/globalIndexID.h"
#include "vessel/memoryBlock.h"
#include "../bson/util/builder.h"
#include "vessel/indexObject.h"

namespace engine
{
namespace vessel
{
   class lsmIndexIterator : public indexIterator
   {
      public:
         lsmIndexIterator(){}
         virtual ~lsmIndexIterator();

      public:
         virtual INDEX_TYPE getIndexType()const {return INDEX_TYPE_LSM;}
         
      public:
         virtual INT32 open(requestContext *context,
                            indexObject *ic,
                            const options &o);

         virtual BOOLEAN isOpen()const;

         virtual void close();

         virtual BOOLEAN isReadyToRead()const;

         virtual INT32 seek(const bson::BSONObj &prevKey,
                            INT32 fieldCountToCmpInPrev,
                            const VEC_ELE_CMP &matchEles,
                            const inclusiveVec &matchInclusive,
                            const seekOptions &o);

         virtual INT32 seekKey(const ixmKey &key,
                               const seekOptions &o);

         virtual INT32 fastNext(const bson::BSONObj &prevKey,
                                INT32 fieldCountToCmpInPrev,
                                const VEC_ELE_CMP &matchEles,
                                const inclusiveVec &matchInclusive,
                                const seekOptions &o);

         virtual INT32 next();

         virtual void pause();

         virtual INT32 contains(const ixmKey &key, recordID &rid);

         virtual INT32 moveToTheNextOfEntry(const slice &entry);
      public:
         virtual DPS_LSN_OFFSET getLSN()const;
         virtual bson::BSONObj getKeyObj(bson::BufBuilder *builder)const;
         virtual DPS_TRANS_ID getTransID()const;
         virtual recordID getRid()const;
         virtual BOOLEAN equalToCurrentKey(const ixmKey &key)const;
         //virtual indexScanEntry getCurrentEntry()const;
         virtual INT32 pushCurrentEntryToBatch(rowBatch &batch)const;
         virtual UINT32 getCurrentEntrySize()const;
      private:
         INT32 seekFullKey(const rocksdb::Slice &fullKey);

         INT32 moveIterator(BOOLEAN forward);

         INT32 forwardToNextVisiblePostion();

         INT32 ensureBackwardToVisiblePosition();

         INT32 moveToNextVisiblePosition();

         INT32 ensureVisiblePosition();


      private:
         void _close();

         BOOLEAN _isReadyToRead()const;

         BOOLEAN _isMarkedRemoved(rocksdb::Iterator *itr)const;

      private:
         void _initKeyBoundWhenOpen(const globalIndexID &id);

      private:
         requestContext *_context = NULL;
         indexObject *_obj = NULL;
         globalIndexID _globalId;
         rocksdb::Iterator *_itr = NULL;
         LSM_IDX_KEY_BOUNDARY _lowBound;
         LSM_IDX_KEY_BOUNDARY _upBound;
         rocksdb::Slice _lowKey;
         rocksdb::Slice _upKey;
         lsmPureKeyEntry _currentEntry;
         bson::BufBuilder _builder;
         memoryBlock _backwardCurrentEntryCache;
         BOOLEAN _forward;
   };//class lsmIndexIterator
}//namespace vessel
}//namespace engine

#endif//VESSEL_LSM_INDEX_ITERATOR_H_