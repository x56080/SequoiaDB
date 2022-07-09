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

         struct seekOptions
         {
            BOOLEAN inclusive = TRUE;
         };//class seekOptions

      public:
         virtual const CHAR *getName()const = 0;
         virtual void reset() = 0;

         virtual INT32 seek(const bson::BSONObj &prevKey,
                            INT32 fieldCountToCmpInPrev,
                            const VEC_ELE_CMP &matchEles,
                            const inclusiveVec &matchInclusive,
                            const seekOptions &o) = 0;

         virtual INT32 seekKey(const ixmKey &key,
                               const seekOptions &o) = 0;

         virtual INT32 locate(const slice &encodedKey,
                              const recordID &rid,
                              const seekOptions &o) = 0;

         virtual INT32 next() = 0;

         virtual INT32 advance(const bson::BSONObj &prevKey,
                               INT32 fieldCountToCmpInPrev,
                               const VEC_ELE_CMP &matchEles,
                               const inclusiveVec &matchInclusive,
                               const seekOptions &o) = 0;

         virtual BOOLEAN isReadyToRead()const = 0;

      public:
         /// Access valid data saved in iterator.
         /// User should always ensure 'isReadyToRead' first.
         virtual bson::BSONObj getKeyObj(bson::BufBuilder *builder)const = 0;
         virtual slice getEncodedKey()const = 0;
         virtual DPS_LSN_OFFSET getLSN()const {return DPS_INVALID_LSN_OFFSET;}
         virtual DPS_TRANS_ID getTransID()const {return DPS_TRANS_ID();}
         virtual recordID getRid()const = 0;
         virtual BOOLEAN equals(const ixmKey &key)const = 0;
   };//class indexIterator

}//namespace vessel
}//namespace engine

#endif//VESSEL_INDEX_ITERATOR_H_