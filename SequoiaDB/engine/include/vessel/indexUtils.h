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

   Source File Name = indexUtils.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#ifndef VESSEL_INDEX_UTILS_H_
#define VESSEL_INDEX_UTILS_H_

#include "vessel/strSlice.h"
#include "vessel/indexKeyPattern.h"
#include "../bson/bson.hpp"
#include "vessel/indexDescription.h"
#include "rtnPredicate.hpp"

namespace engine
{
namespace vessel
{
   class indexUtils : public SDBObject
   {
      public:
         ~indexUtils() = delete;

      public:
         static bson::BSONObj buildIndexDefObj(const indexDescription &desc);

         // /// if output set as null, it will not be parsed.
         // static INT32 parseIndexDefObj(const bson::BSONObj &obj,
         //                               strSlice *indexName,
         //                               indexKeyPattern *keyPattern,
         //                               indexParameters *params);

         static OSS_INLINE BOOLEAN isForwardDirection(INT32 direction)
         {
            return 0 <= direction;
         }

         /// return builder.done() if outer builder is not null
         static bson::BSONObj buildKeyToSeek(const bson::BSONObj &key,
                                             INT32 keyFieldsToCmp,
                                             const VEC_ELE_CMP & matchEle,
                                             bson::BufBuilder *outerBuilder=NULL);

         static INT32 compareKey(const BSONObj &currentKey,
                                 const BSONObj &prevKey,
                                 INT32 keepFieldsNum, BOOLEAN skipToNext,
                                 const VEC_ELE_CMP &matchEle,
                                 const inclusiveVec &matchInclusive,
                                 const bson::Ordering &o, INT32 direction);

         /// stop at the first dot or terminating.
         static UINT32 createPatternFieldNameHash(const CHAR *fieldName);

         /// stop at the first dot or terminating.
         static BOOLEAN fieldNameAssociate(const strSlice &l,
                                           const strSlice &r);

         /// stop at the first dot or terminating.
         static BOOLEAN fieldNameAssociate(const CHAR *l,
                                           const CHAR *r);
   };//class indexUtils

   
}//namespace vessel
}//namesapce engine

#endif//VESSEL_INDEX_UTILS_H_
