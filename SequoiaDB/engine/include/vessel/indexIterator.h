/*******************************************************************************

   Copyright (C) 2011-Present SequoiaDB Ltd.

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

      http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.

   Source File Name = indexIterator.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef VESSEL_INDEX_ITERATOR_H_
#define VESSEL_INDEX_ITERATOR_H_

#include "vessel/indexDef.h"
#include "utilPooledObject.hpp"
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
            BOOLEAN pointGetOptimized = FALSE;
         };//class options

      public:
         virtual INDEX_ITERATOR_TYPE getType() const = 0;
         virtual void reset() = 0;


      public:/// init iterator location. 
         virtual INT32 seek(const VEC_ELE_CMP &eles,
                            const inclusiveVec &iv,
                            const options &o) = 0;
                            
         virtual INT32 seek(const bson::BSONObj &key,
                            const inclusiveVec &iv,
                            const options &o) = 0;

         /// locate
         virtual INT32 locateNext(const indexEntryLocation *location,
                                  const options &o) = 0;

         virtual INT32 pause() = 0;

         virtual INT32 resume() = 0;
      public:

         virtual INT32 next() = 0;

         /// reseek from current pos, to fast skip unmatched entries.
         virtual INT32 advance(const bson::BSONObj &prevKey,
                               INT32 fieldCountToCmpInPrev,
                               const VEC_ELE_CMP &matchEles,
                               const inclusiveVec &iv) = 0;

         virtual BOOLEAN isReadyToRead()const = 0;

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