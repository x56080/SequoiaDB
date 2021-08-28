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
#include "vessel/orderingWrapper.h"
#include "inclusiveVec.h"
#include "vessel/indexHandle.h"
#include "vessel/slice.h"
#include "rtnPredicate.hpp"
#include "vessel/indexEntryBuffer.h"

namespace engine
{
namespace vessel
{
   class indexIterator : public _utilPooledObject
   {
      public:
         indexIterator(){}
         virtual ~indexIterator();
         indexIterator(const indexIterator &) = delete;
         indexIterator &operator=(const indexIterator &) = delete;

      public:
         OSS_INLINE const indexHandle &getHandle()const
         {
            return _handle;
         }
         OSS_INLINE BOOLEAN isForward()const
         {
            return _forward;
         }
         OSS_INLINE const orderingWrapper &getOrdering()const
         {
            return _ordering;
         }

      public:
         virtual INDEX_TYPE getIndexType()const = 0;

      public:
         virtual INT32 open(requestContext *context,
                            const indexHandle &handle,
                            const orderingWrapper &ordering,
                            BOOLEAN forward) = 0;

         virtual void close() = 0;

         virtual void pause() = 0;

         /// lower bound of last key entry.
         virtual INT32 resume() = 0;

         virtual INT32 seek(const bson::BSONObj &prevKey,
                            INT32 fieldCountToCmpInPrev,
                            BOOLEAN upperBound,
                            const VEC_ELE_CMP &matchEles,
                            const inclusiveVec &matchInclusive) = 0;

         virtual INT32 seek(const bson::BSONObj &key,
                            BOOLEAN upperBound) = 0;

         virtual INT32 seek(const bson::BSONObj &key,
                            const recordID &rid,
                            BOOLEAN upperBound) = 0;

         virtual INT32 seek(const slice &entry,
                            BOOLEAN upperBound) = 0;
         
         /// move to next position from current
         /// until hit the different key or rid.
         virtual INT32 nextDiffKeyOrRid() = 0;

         /// move to next position from current
         /// until hit the matched entry.
         virtual INT32 nextTo(const bson::BSONObj &prevKey,
                              INT32 fieldCountToCmpInPrev,
                              BOOLEAN upperBound,
                              const VEC_ELE_CMP &matchEles,
                              const inclusiveVec &matchInclusive) = 0;

         /// move to next postion from current.
         virtual INT32 next() = 0;

         virtual BOOLEAN isReadyToRead()const = 0;

      public:
         /// The functions to access current tuple saved in iterator.
         /// User should always call 'isReadyToRead' first.

         virtual BOOLEAN isMarkedRemoved()const = 0;
         virtual UINT64 getLSN()const = 0;
         virtual void getKey(ixmKey &key)const = 0;
         virtual DPS_TRANS_ID getTransID()const = 0;
         virtual recordID getRid()const = 0;
         virtual slice getValue()const = 0;
         virtual UINT32 getEntrySize()const = 0;
         virtual INT32 copyKeyEntry(UINT32 bufferSize,
                                    CHAR *buffer)const = 0;
         virtual INT32 copyKeyEntryToBuffer(indexEntryBuffer &buffer) const = 0;

      protected:

         OSS_INLINE BOOLEAN _isOpen()const
         {
            return NULL != _context;
         }

         void _open(requestContext *context,
                    const indexHandle &handle,
                    const orderingWrapper &ordering,
                    BOOLEAN forward);
         void _close();

         requestContext *getContext()const
         {
            return _context;
         }
      private:
         requestContext *_context = NULL;
         indexHandle _handle;
         orderingWrapper _ordering;
         BOOLEAN _forward = TRUE;
   };//class indexIterator

   extern indexIterator *createIndexIterator(INDEX_TYPE type);
}//namespace vessel
}//namespace engine

#endif//VESSEL_INDEX_ITERATOR_H_