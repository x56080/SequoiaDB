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

   Source File Name = indexUtils.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

*******************************************************************************/
#include "vessel/indexUtils.h"
#include "ossLikely.hpp"
#include "pdTrace.hpp"
#include "vessel/indexDef.h"
#include "ixm_common.hpp"
#include "xxHashInc.h"


namespace engine
{
namespace vessel
{
   bson::BSONObj indexUtils::buildKeyToSeek(const bson::BSONObj &key,
                                            INT32 keyFieldsToCmp,
                                            const VEC_ELE_CMP & matchEle,
                                            bson::BufBuilder *outerBuilder)
   {
      bson::BufBuilder b;
      BSONObjBuilder builder(NULL == outerBuilder ? b : *outerBuilder);
      BSONObjIterator itr(key);
      INT32 index = 0;
      for (; index < keyFieldsToCmp; ++index)
      {
         SDB_ASSERT(itr.more(), "must be more");
         builder.appendAs(itr.next(), "");
      }

      for (; index < (INT32)(matchEle.size()); ++index)
      {
         builder.appendAs(*(matchEle.at(index)), "");
      }

      if (NULL == outerBuilder)
      {
         return builder.obj();
      }
      else
      {
         return builder.done();
      }
   }

   INT32 indexUtils::compareKey(const BSONObj &currentKey,
                                 const BSONObj &prevKey,
                                 INT32 keepFieldsNum, BOOLEAN skipToNext,
                                 const VEC_ELE_CMP &matchEle,
                                 const inclusiveVec &matchInclusive,
                                 const bson::Ordering &o, INT32 direction)
   {
      BSONObjIterator ll ( currentKey ) ;
      BSONObjIterator rr ( prevKey ) ;
      VEC_ELE_CMP::const_iterator eleItr = matchEle.begin() ;
      UINT32 incVecPos = 0;
      UINT32 mask = 1 ;
      INT32 retCode = 0 ;
      // match keepFieldsNum fields
      for ( INT32 i = 0 ; i < keepFieldsNum; ++i, mask<<=1 )
      {
         BSONElement curEle = ll.next() ;
         BSONElement prevEle = rr.next() ;
         // skip those fields since we don't want to match them from
         // startstopkey iterator
         ++eleItr ;
         ++incVecPos ;
         INT32 result = curEle.woCompare ( prevEle, FALSE ) ;
         if ( o.descending ( mask ))
            result = -result ;
         if ( result )
         {
            retCode = result ;
            goto done ;
         }
      }
      // if all the keepFieldsNum fields got matched, let's see if we want to
      // simply skip to next key, if so we don't need to match all other
      // elements
      // if that happen, the return value should be -direction, since we want to
      // return -1 if searching forward, otherwise return 1
      if ( skipToNext )
      {
         retCode = -direction ;
         goto done ;
      }
      // if all keepFieldsNum fields got matched, and we want to further match
      // startstopkey iterator, let's move on
      for ( ; ll.more(); mask<<=1 )
      {
         // curEle is always get from current key
         BSONElement curEle = ll.next() ;
         // now let's get the expected element from startstopkey iterator
         BSONElement prevEle = **eleItr ;
         ++eleItr ;
         INT32 result = curEle.woCompare ( prevEle, FALSE ) ;
         if ( o.descending ( mask ))
            result = -result ;
         if ( result )
         {
            retCode = result ;
            goto done ;
         }
         // when getting here, that means the key matches expectation, then
         // let's see if we want inclusive predicate. If not we need to return
         // the negative of direction ( -1 for forward scan, otherwise 1 )
         if (!matchInclusive[incVecPos])
         {
            retCode = -direction ;
            goto done ;
         }
         // when get here, it means key match AND inclusive
         ++incVecPos ;
      }
   done :
      return retCode ;
   }

   UINT32 indexUtils::createPatternFieldNameHash(const CHAR *fieldName)
   {
      SDB_ASSERT(NULL != fieldName, "can not be null");
      UINT32 hash = 0;
      UINT32 size = 0;
      while (TRUE)
      {
         const CHAR *p = fieldName + size;
         if ('\0' == *p || '.' == *p)
         {
            break;
         }
         ++size;
      }

      if (0 < size)
      {
         hash = XXH3_64bits(fieldName, size);
      }
      return hash;
   }

   BOOLEAN indexUtils::fieldNameAssociate(const strSlice &l,
                                          const strSlice &r)
   {
      BOOLEAN res = TRUE;
      UINT32 i = 0;

      while (i < l.strLen() && i < r.strLen())
      {
         CHAR lc = l.at(i);
         CHAR rc = r.at(i);
         if (lc != rc)
         {
            res = FALSE;
            goto done;
         }

         if ('.' == lc)
         {
            goto done;
         }

         ++i;
      }

      if (l.strLen() < r.strLen())
      {
         res = ('.' == r.at(i));
      }
      else if (r.strLen() < l.strLen())
      {
         res = ('.' == l.at(i));
      }

   done:
      return res;
   }

   BOOLEAN indexUtils::fieldNameAssociate(const CHAR *l,
                                          const CHAR *r)
   {
      SDB_ASSERT(NULL != l && NULL != r, "can not be invalid");
      BOOLEAN res = TRUE;
      const CHAR *longer = r;
      UINT32 i = 0;
      while ('\0' != l[i])
      {
         if ('\0' == r[i])
         {
            longer = l;
            break;
         }

         if (l[i] != r[i])
         {
            res = FALSE;
            goto done;
         }

         if ('.' == l[i])
         {
            goto done;
         }

         ++i;
      }
      
      res = ('\0' == longer[i] || '.' == longer[i]);
   done:
      return res;
   }
}//namespace vessel
}//namespace engine