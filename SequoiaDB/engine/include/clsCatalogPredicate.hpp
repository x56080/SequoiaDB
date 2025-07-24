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

   Source File Name = clsCatalogPredicate.hpp

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
#ifndef CLSCATALOGPREDICATE_HPP_
#define CLSCATALOGPREDICATE_HPP_

#include "rtnPredicate.hpp"
#include "utilMap.hpp"

namespace engine
{
   class clsCatalogPredicateTree;
   class _clsCatalogItem;

   typedef _utilMap< std::string, rtnStartStopKey*, 10 > MAP_CLSCATAPREDICATEFIELD ;
   typedef std::vector< clsCatalogPredicateTree * >      VEC_CLSCATAPREDICATESET ;

   /*
      _CLS_CATA_LOGIC_TYPE define
   */
   typedef enum _CLS_CATA_LOGIC_TYPE
   {
      CLS_CATA_LOGIC_INVALID        = 0,
      CLS_CATA_LOGIC_AND            = 1,
      CLS_CATA_LOGIC_OR,
   }CLS_CATA_LOGIC_TYPE ;

   /*
      clsCatalogPredicateTree define
   */
   class clsCatalogPredicateTree : public SDBObject
   {
   public:
      clsCatalogPredicateTree( bson::BSONObj shardingKey ) ;
      ~clsCatalogPredicateTree() ;

      void upgradeToUniverse() ;
      BOOLEAN isUniverse() ;
      CLS_CATA_LOGIC_TYPE getLogicType() ;
      void setLogicType( CLS_CATA_LOGIC_TYPE type ) ;
      void addChild( clsCatalogPredicateTree *pChild ) ;
      INT32 addPredicate( const CHAR *pFieldName, bson::BSONElement beField );
      void adjustByShardingKey() ;
      void clear() ;
      INT32 matches( _clsCatalogItem * pCatalogItem, BOOLEAN & result ) ;

      string toString() const ;

   protected:
      /// compareLU <= 0, compare lowbound and stop key
      /// compareLR >=0, compare upbound and start key
      INT32 _matches( bson::BSONObjIterator itrSK,
                      bson::BSONObjIterator itrLB,
                      bson::BSONObjIterator itrUB,
                      BOOLEAN & result,
                      BOOLEAN isCloseInterval,
                      INT32 compareLU ) ;

   private:
      // forbid copy constructor
      clsCatalogPredicateTree( clsCatalogPredicateTree &right ){}
   private:
      VEC_CLSCATAPREDICATESET       _children ;
      rtnPredicateSet               _predicateSet ;
      CLS_CATA_LOGIC_TYPE           _logicType ;
      bson::BSONObj                 _shardingKey ;

   } ;

}

#endif // CLSCATALOGPREDICATE_HPP_

