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

   Source File Name = indexUtils.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef VESSEL_INDEX_UTILS_H_
#define VESSEL_INDEX_UTILS_H_

#include "vessel/strSlice.h"
#include "vessel/indexKeyPattern.h"
#include "../bson/bson.hpp"
#include "rtnPredicate.hpp"
#include "vessel/indexProperties.h"

namespace engine
{
namespace vessel
{
   class indexUtils : public SDBObject
   {
      public:
         ~indexUtils() = delete;

      public:
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
