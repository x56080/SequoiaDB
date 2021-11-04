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

   Source File Name = indexUtils.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#include "vessel/indexUtils.h"
#include "ossLikely.hpp"
#include "pdTrace.hpp"
#include "vessel/indexDef.h"
#include "ixm_common.hpp"

namespace engine
{
namespace vessel
{
   bson::BSONObj indexUtils::buildIndexDefObj(const strSlice &indexName,
                                              const indexKeyPattern &keyPattern,
                                              const indexParameters &params)
   {
      bson::BSONObjBuilder builder;
      SDB_ASSERT(!indexName.empty(), "can not be empty");
      SDB_ASSERT(keyPattern.isValid(), "can not be invalid");
      SDB_ASSERT(params.isValid(), "can not be inavlid");

      builder.append(IXM_NAME_FIELD, indexName.str());
      builder.append(IXM_KEY_FIELD, keyPattern.getPattern());
      params.exportToBson(builder);
      return builder.obj();
   }

   INT32 indexUtils::parseIndexDefObj(const bson::BSONObj &obj,
                                       strSlice *indexName,
                                       indexKeyPattern *keyPattern,
                                       indexParameters *params)
   {
      INT32 rc = SDB_OK;

      if (NULL != indexName)
      {
         indexName->reset();
         bson::BSONElement ele = obj.getField(IXM_NAME_FIELD);
         if (bson::String != ele.type())
         {
            PD_LOG(PDERROR, "index name not found");
            rc = SDB_INVALIDARG;
            goto error;
         }
         indexName->reset(ele.valuestr());
      }

      if (NULL != keyPattern)
      {
         keyPattern->reset();
         bson::BSONElement ele = obj.getField(IXM_KEY_FIELD);
         if (bson::Object != ele.type())
         {
            PD_LOG(PDERROR, "index key define not found");
            rc = SDB_INVALIDARG;
            goto error;
         }

         rc = keyPattern->set(ele.embeddedObject());
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to set key pattern from obj:%d", rc);
            goto error;
         }
      }

      if (NULL != params)
      {
         if (!params->extractFromBson(obj))
         {
            PD_LOG(PDERROR, "failed to extract common options");
            rc = SDB_INVALIDARG;
            goto error;
         }
      }

   done:
      return rc;
   error:
      goto done;
   }

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
}//namespace vessel
}//namespace engine