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
#include "utilPooledObject.hpp"
#include "ixmKey.hpp"
#include "dms.hpp"
#include "inclusiveVec.h"
#include "vessel/slice.h"
#include "rtnPredicate.hpp"
#include "../bson/util/builder.h"
#include "dpsDef.hpp"
#include "vessel/recordID.h"
#include "vessel/indexEntryLocation.h"

#include <memory>

namespace engine
{
namespace vessel
{
   class indexIterator : public _utilPooledObject
   {
      public:
         indexIterator() = default;
         virtual ~indexIterator() = default;
         indexIterator(const indexIterator &) = delete;
         indexIterator &operator=(const indexIterator &) = delete;

      public:
         struct options
         {
            OSS_INLINE INT32 getDirection()const
            {
               return forward ? 1 : -1;
            }

            BOOLEAN forward = TRUE;
         };//class options

      public:
         virtual INDEX_ITERATOR_TYPE getType() const = 0;
         virtual void reset() = 0;

         virtual INT32 seek(const VEC_ELE_CMP &matchEles,
                            const inclusiveVec &matchInclusive) = 0;

         virtual INT32 seek(const bson::BSONObj &key,
                            const inclusiveVec &matchInclusive) = 0;

         virtual INT32 locate(const indexEntryLocation *location) = 0;

         virtual INT32 next() = 0;

         /// reseek from current pos, to fast skip unmatched entries.
         virtual INT32 advance(const bson::BSONObj &prevKey,
                               INT32 fieldCountToCmpInPrev,
                               const VEC_ELE_CMP &matchEles,
                               const inclusiveVec &matchInclusive) = 0;

         virtual BOOLEAN isReadyToRead()const = 0;

         virtual INT32 pause(IDX_ENTRY_LOCATION_UPTR &location) = 0;

         virtual INT32 resume(const indexEntryLocation *location) = 0;

      public:
         /// Access valid data saved in iterator.
         /// User should always ensure 'isReadyToRead' first.

         /// return builder.done() if buf is not null
         virtual bson::BSONObj getKeyObj(BOOLEAN withFieldName,
                                         bson::BufBuilder *buf)const = 0;
         virtual DPS_LSN_OFFSET getLSN()const = 0;
         virtual DPS_TRANS_ID getTransID()const = 0;
         virtual recordID getRid()const = 0;
         virtual INT32 initOrUpdateLocation(IDX_ENTRY_LOCATION_UPTR &location) const = 0;

   };//class indexIterator

   using INDEX_ITERATOR_UPTR = std::unique_ptr<indexIterator>;

}//namespace vessel
}//namespace engine

#endif//VESSEL_INDEX_ITERATOR_H_