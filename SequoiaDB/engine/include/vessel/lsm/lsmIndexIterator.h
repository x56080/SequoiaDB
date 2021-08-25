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

namespace engine
{
namespace vessel
{
   class LSMDB;

   class lsmIndexIterator : public indexIteratorKernal
   {
      public:
         lsmIndexIterator();
         virtual ~lsmIndexIterator();

      public:
         virtual INT32 open(requestContext *context,
                            const indexHandle &handle,
                            const orderingWrapper &ordering,
                            INT32 direction,
                            rtnPredicateListIterator *predicate,
                            memoryBlock &entryBuffer);

         virtual void close();

      private:
         INT32 _open();

         void _close();

         INT32 seekToLast();

         rocksdb::Slice getLastEntry()const;

      private:
         LSMDB *_lsmDB = NULL;
         rocksdb::Iterator *_itr = NULL;
         CHAR _lowBoundKey[LSM_MIN_FULL_KEY_SIZE];
         CHAR _upperBoundKey[LSM_MIN_FULL_KEY_SIZE];
         rocksdb::Slice _lowKey;
         rocksdb::Slice _upKey;
   };//class lsmIndexIterator
}//namespace vessel
}//namespace engine

#endif//VESSEL_LSM_INDEX_ITERATOR_H_