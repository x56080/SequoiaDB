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

   Source File Name = indexIterator.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#ifndef VESSEL_INDEX_ITERATOR_H_
#define VESSEL_INDEX_ITERATOR_H_

#include "vessel/indexDef.h"
#include "vessel/indexHandle.h"
#include "utilPooledObject.hpp"
#include "ixmKey.hpp"
#include "dms.hpp"
#include "vessel/requestContext.h"
#include "inclusiveVec.h"
#include "vessel/slice.h"
#include "rtnPredicate.hpp"
#include "vessel/indexScanContext.h"
#include "vessel/rowBatch.h"

namespace engine
{
namespace vessel
{
   class indexIterator : public _utilPooledObject
   {
      public:
         indexIterator(){}
         virtual ~indexIterator(){}
         indexIterator(const indexIterator &) = delete;
         indexIterator &operator=(const indexIterator &) = delete;

      public:
         virtual INDEX_TYPE getIndexType()const = 0;

      public:
         class options : public SDBObject
         {
            public:
               options(){}
               explicit options(BOOLEAN inclusive, BOOLEAN forward):
                        _inclusive(inclusive),
                        _forward(forward){}
               ~options(){}
               options(const options &b):
               _inclusive(b._inclusive),
               _forward(b._forward){}
               options &operator=(const options &b)
               {
                  _inclusive = b._inclusive;
                  _forward = b._forward;
                  return *this;
               }
            
            public:
               OSS_INLINE BOOLEAN isInclusive()const
               {
                  return _inclusive;
               }
               OSS_INLINE BOOLEAN isForward()const
               {
                  return _forward;
               }
               OSS_INLINE void setInclusive(BOOLEAN inclusive)
               {
                  _inclusive = inclusive;
               }
               OSS_INLINE INT32 getDirection()const
               {
                  return _forward ? 1 : -1;
               }

            private:
               BOOLEAN _inclusive = TRUE;
               BOOLEAN _forward = TRUE;
         };//class options

      public:
         virtual INT32 open(requestContext *context,
                            indexContext *ic) = 0;

         virtual BOOLEAN isOpen()const = 0;

         virtual void close() = 0;

         virtual INT32 seek(const bson::BSONObj &prevKey,
                            INT32 fieldCountToCmpInPrev,
                            const VEC_ELE_CMP &matchEles,
                            const inclusiveVec &matchInclusive,
                            const options &o) = 0;

         virtual INT32 seekKey(const ixmKey &key,
                               const options &o) = 0;

         virtual INT32 next(BOOLEAN forward) = 0;

         virtual INT32 fastNext(const bson::BSONObj &prevKey,
                                INT32 fieldCountToCmpInPrev,
                                const VEC_ELE_CMP &matchEles,
                                const inclusiveVec &matchInclusive,
                                const options &o) = 0;

         virtual BOOLEAN isReadyToRead()const = 0;

         virtual void pause() = 0;

         virtual INT32 contains(const ixmKey &key, recordID &rid) = 0;

         virtual INT32 moveToTheNextOfEntry(const slice &entry,
                                            BOOLEAN forward) = 0;

      public:
         /// The functions to access current tuple saved in iterator.
         /// User should always call 'isReadyToRead' first.
         virtual bson::BSONObj getKeyObj(bson::BufBuilder *builder)const = 0;
         virtual DPS_LSN_OFFSET getLSN()const = 0;
         virtual DPS_TRANS_ID getTransID()const = 0;
         virtual recordID getRid()const = 0;
         virtual BOOLEAN equalToCurrentKey(const ixmKey &key)const = 0;
         virtual INT32 pushCurrentEntryToBatch(rowBatch &batch)const = 0;
         virtual UINT32 getCurrentEntrySize()const = 0;
   };//class indexIterator

   extern indexIterator *createIndexIterator(INDEX_TYPE type);
}//namespace vessel
}//namespace engine

#endif//VESSEL_INDEX_ITERATOR_H_