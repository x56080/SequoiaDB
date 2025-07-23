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

   Source File Name = clsCatalogMatcher.hpp

   Descriptive Name =

   When/how to use: this program may be used on binary and text-formatted
   versions of runtime component. This file contains code logic for
   common functions for coordinator node.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================

   Last Changed =

*******************************************************************************/
#ifndef CLSCATALOGMATCHER_HPP_
#define CLSCATALOGMATCHER_HPP_

#include "core.hpp"
#include "oss.hpp"
#include "../bson/bsonobj.h"
#include "clsCatalogPredicate.hpp"

namespace engine
{
   class clsCatalogPredicateTree ;
   class _clsCatalogItem ;

   /*
      clsCatalogMatcher define
   */
   class clsCatalogMatcher : public SDBObject
   {
   public:
      clsCatalogMatcher( const bson::BSONObj &shardingKey );

      INT32 loadPattern( const bson::BSONObj &matcher );

      INT32 matches( _clsCatalogItem* pCatalogItem,
                     BOOLEAN &result );

   private:
      INT32 parseAnObj( const bson::BSONObj &matcher,
                        clsCatalogPredicateTree &predicateSet );

      INT32 parseCmpOp( const bson::BSONElement &beField,
                        clsCatalogPredicateTree &predicateSet );

      INT32 parseOpObj( const BSONElement & beField,
                        clsCatalogPredicateTree & predicateSet );

      INT32 parseLogicOp( const bson::BSONElement &beField,
                          clsCatalogPredicateTree &predicateSet ) ;
      BOOLEAN isOpObj( const bson::BSONObj obj ) ;

      BOOLEAN _isExistUnreconigzeOp( const bson::BSONObj obj ) ;

   private:
      clsCatalogPredicateTree    _predicateSet;
      bson::BSONObj              _shardingKey;
      bson::BSONObj              _matcher;
   } ;

}

#endif // CLSCATALOGMATCHER_HPP_

