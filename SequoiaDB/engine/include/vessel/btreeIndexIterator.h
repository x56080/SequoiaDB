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
#include "vessel/btreeAccessor.h"
#include "../bson/util/builder.h"

namespace engine
{
namespace vessel
{
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
            return _accessor.isValid();
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

         virtual INT32 next();

         virtual INT32 prev();

         virtual INT32 advanceTo(const bson::BSONObj &prevKey,
                                 INT32 fieldCountToCmpInPrev,
                                 const VEC_ELE_CMP &matchEles,
                                 const inclusiveVec &matchInclusive,
                                 const options &o);

         virtual BOOLEAN isReadyToRead()const;

         virtual void pause();
      public:
         virtual UINT64 getLSN()const;
         virtual slice getKey()const;
         virtual DPS_TRANS_ID getTransID()const;
         virtual recordID getRid()const;
         virtual BOOLEAN equalToCurrentKey(const ixmKey &key)const;
         virtual INT32 pushCurrentEntryToBatch(indexScanEntryBatch &batch)const;
         virtual indexScanEntry getCurrentEntry()const;
      private:
         void resetToSeek();
         INT32 cacheSeekResult(const btreeItemLocation &location);

      private:
         btreeAccessor _accessor;
         bson::BufBuilder _builder;
         btreeAccessContext _bac;
         btreeNode _node;
         btreeIndexItem _item;
   };//class btreeIndexIterator
} // namespace vessel

} // namespace engine


#endif//VESSEL_BTREE_INDEX_ITERATOR_H_