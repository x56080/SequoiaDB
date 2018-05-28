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

