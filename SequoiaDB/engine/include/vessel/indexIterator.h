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
#include "vessel/memoryBlock.h"
#include "vessel/indexHandle.h"
#include "utilPooledObject.hpp"
#include "ixmKey.hpp"
#include "dms.hpp"
#include "vessel/requestContext.h"
#include "rtnPredicate.hpp"
#include "vessel/orderingWrapper.h"
#include "vessel/memoryBlock.h"

namespace engine
{
namespace vessel
{
   class indexIteratorKernal : public _utilPooledObject
   {
      public:
         indexIteratorKernal(){}
         virtual ~indexIteratorKernal(){}
         indexIteratorKernal(const indexIteratorKernal &) = delete;
         indexIteratorKernal &operator=(const indexIteratorKernal &) = delete;

      public:
         OSS_INLINE const indexHandle &getHandle()const
         {
            return _handle;
         }
         OSS_INLINE INT32 getDirection()const
         {
            return _direction;
         }
         OSS_INLINE const bson::Ordering *getOrdering()const
         {
            return _ordering.toBsonOrdering();
         }

      public:
         virtual INDEX_TYPE getIndexType()const = 0;

      public:
         virtual BOOLEAN isValid()const = 0;

         virtual INT32 open(requestContext *context,
                            const indexHandle &handle,
                            const orderingWrapper &ordering,
                            INT32 direction,
                            rtnPredicateListIterator *predicate,
                            memoryBlock &entryBuffer) = 0;

         virtual void close() = 0;

         virtual INT32 getNext(BOOLEAN &hitTheEnd) = 0;

      public:
         /// The functions to access current data saved in iterator.
         /// User should always call 'getNext' first and ensure iterator not hits the end.

         virtual UINT64 getCurrentLSN()const = 0;
         virtual ixmKey getCurrentKey()const = 0;
         virtual DPS_TRANS_ID getCurrentTransID()const = 0;
         virtual dmsRecordID getCurrentRid()const = 0;

      protected:
         OSS_INLINE rtnPredicateListIterator *getPredicate()
         {
            return _predicate;
         }

         INT32 _open(requestContext *context,
                     const indexHandle &handle,
                     const orderingWrapper &ordering,
                     INT32 direction,
                     rtnPredicateListIterator *predicate,
                     memoryBlock &entryBuffer);

         void _close();

         memoryBlock &getEntryBuffer()const
         {
            return *_entryBuffer;
         }

      private:
         requestContext *_context = NULL;
         indexHandle _handle;
         orderingWrapper _ordering;
         INT32 _direction = 1;
         rtnPredicateListIterator *_predicate = NULL;
         memoryBlock *_entryBuffer = NULL;
   };//class indexIteratorKernal

   class indexIterator : public SDBObject
   {
      public:
         indexIterator(){}
         ~indexIterator(){}
         indexIterator(const indexIterator &) = delete;
         indexIterator &operator=(const indexIterator &) = delete;
         indexIterator(indexIterator &&o);
         indexIterator &operator=(indexIterator &&o);

      public:
         OSS_INLINE BOOLEAN isValid()const
         {
            return NULL != _kernal;
         }

      public:


      private:
         indexIteratorKernal *_kernal = NULL;
   };//class indexIterator
}//namespace vessel
}//namespace engine

#endif//VESSEL_INDEX_ENTRY_H_