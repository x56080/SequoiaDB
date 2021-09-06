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
#include "vessel/lsm/lsmIdxKey.hpp"
#include "vessel/globalIndexID.h"
#include "vessel/memoryBlock.h"

namespace engine
{
namespace vessel
{
   class LSMDB;

   class lsmIndexIterator : public indexIterator
   {
      public:
         lsmIndexIterator(){}
         virtual ~lsmIndexIterator();

      public:
         virtual INDEX_TYPE getIndexType()const {return INDEX_TYPE_LSM;}
         
      public:
         virtual INT32 open(requestContext *context,
                            const indexContext *ic,
                            BOOLEAN forward);

         virtual void close();

         virtual BOOLEAN isReadyToRead()const;

         virtual INT32 seek(const bson::BSONObj &prevKey,
                            INT32 fieldCountToCmpInPrev,
                            BOOLEAN upperBound,
                            const VEC_ELE_CMP &matchEles,
                            const inclusiveVec &matchInclusive);

         virtual INT32 seek(const slice &entry,
                            BOOLEAN upperBound);

         virtual INT32 seek(const bson::BSONObj &key,
                            BOOLEAN upperBound);

         virtual INT32 seek(const bson::BSONObj &key,
                            const recordID &rid,
                            BOOLEAN upperBound);

         virtual INT32 next();

         virtual INT32 nextTo(const bson::BSONObj &prevKey,
                              INT32 fieldCountToCmpInPrev,
                              BOOLEAN upperBound,
                              const VEC_ELE_CMP &matchEles,
                              const inclusiveVec &matchInclusive);

         virtual INT32 nextDiffKeyOrRid();

         virtual void pause();

         virtual INT32 resume();
      public:
         virtual BOOLEAN isMarkedRemoved()const;
         virtual UINT64 getLSN()const;
         virtual void getKey(ixmKey &key)const;
         virtual DPS_TRANS_ID getTransID()const;
         virtual recordID getRid()const;
         virtual slice getValue()const;
         virtual UINT32 getEntrySize()const;
         virtual INT32 copyKeyEntry(UINT32 bufferSize,
                                    CHAR *buffer)const;
         virtual INT32 copyKeyEntryToBuffer(indexEntryBuffer &buffer) const;

      private:
         INT32 upperBoundKey(const bson::BSONObj &key);

         INT32 lowerBoundKey(const bson::BSONObj &key);

         INT32 upperBoundKeyAndRid(const bson::BSONObj &key,
                                   const recordID &rid);

         INT32 lowerBoundKeyAndRid(const bson::BSONObj &key,
                                   const recordID &rid);

         INT32 moveIterator();

      private:
         rocksdb::Slice packFullKey(const bson::BSONObj &key,
                                    const recordID &rid,
                                    DPS_LSN_OFFSET lsn,
                                    const DPS_TRANS_ID &transID,
                                    memoryBlock &mb);

         INT32 updateCurrentEntry();

         void _close();

         BOOLEAN _isReadyToRead()const;

         INT32 _backupEntry(const lsmKeyEntry &src,
                            lsmKeyEntry &dst,
                            memoryBlock &mb);

      private:
         globalIndexID _globalId;
         LSMDB *_lsmDB = NULL;
         rocksdb::Iterator *_itr = NULL;
         CHAR _lowBoundKey[LSM_MIN_FULL_KEY_SIZE];
         CHAR _upperBoundKey[LSM_MIN_FULL_KEY_SIZE];
         rocksdb::Slice _lowKey;
         rocksdb::Slice _upKey;
         lsmKeyEntry _currentEntry;
         BufBuilder _builder;
   };//class lsmIndexIterator
}//namespace vessel
}//namespace engine

#endif//VESSEL_LSM_INDEX_ITERATOR_H_