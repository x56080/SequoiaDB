/*******************************************************************************


   Copyright (C) 2011-2018 SequoiaDB Ltd.

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

   http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.

   Source File Name = clsCataHashMatcher.hpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          2014-02-13  JHL  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef CLS_CATA_HASH_MATCHER_HPP_
#define CLS_CATA_HASH_MATCHER_HPP_

#include <vector>
#include <string>
#include "ossTypes.h"
#include "clsCatalogPredicate.hpp"
#include "utilMap.hpp"

namespace engine
{

   /*
      clsCataHashPredTree define
   */
   class clsCataHashPredTree : public SDBObject
   {
   typedef std::vector< clsCataHashPredTree * >             VEC_CLSCATAHASHPREDSET ;
   typedef _utilMap< std::string, bson::BSONElement, 10 >   MAP_CLSCATAHASHPREDFIELDS ;
   public:
      clsCataHashPredTree( bson::BSONObj shardingKey );

      ~clsCataHashPredTree();

      void upgradeToUniverse();

      BOOLEAN isUniverse();

      BOOLEAN isNull();

      CLS_CATA_LOGIC_TYPE getLogicType();

      void setLogicType( CLS_CATA_LOGIC_TYPE type );

      void addChild( clsCataHashPredTree *&pChild );

      INT32 addPredicate( const CHAR *pFieldName, bson::BSONElement beField );

      void clear();

      INT32 generateHashPredicate( UINT32 partitionBit, UINT32 internalV ) ;

      string toString() const ;

      INT32 matches( _clsCatalogItem* pCatalogItem,
                     BOOLEAN &result );

   private:
      // forbid copy constructor
      clsCataHashPredTree( clsCataHashPredTree &right ){}
   private:
      VEC_CLSCATAHASHPREDSET        _children;
      MAP_CLSCATAHASHPREDFIELDS     _fieldSet;
      CLS_CATA_LOGIC_TYPE           _logicType;
      bson::BSONObj                 _shardingKey;
      INT32                         _hashVal;
      BOOLEAN                       _hasPred;
      BOOLEAN                       _isNull;
   };

   /*
      clsCataHashMatcher define
   */
   class clsCataHashMatcher : public SDBObject
   {
      /*
         _CLS_CATA_PREDICATE_OBJ_TYPE define
      */
      typedef enum _CLS_CATA_PREDICATE_OBJ_TYPE
      {
         PREDICATE_OBJ_TYPE_OP_EQ = 0,
         PREDICATE_OBJ_TYPE_OP_NOT_EQ,
         PREDICATE_OBJ_TYPE_NOT_OP
      }CLS_CATA_PREDICATE_OBJ_TYPE ;

   public:
      // note: don't delete shardingkey before delete clsCataHashMatcher
      clsCataHashMatcher( const bson::BSONObj &shardingKey );

      virtual ~clsCataHashMatcher(){};

      INT32 loadPattern( const bson::BSONObj &matcher,
                         UINT32 partitionBit,
                         UINT32 internalV );

      INT32 matches( _clsCatalogItem* pCatalogItem,
                     BOOLEAN &result );

   private:
      INT32 parseAnObj( const bson::BSONObj &matcher,
                        clsCataHashPredTree &predicateSet );

      INT32 parseCmpOp( const bson::BSONElement &beField,
                        clsCataHashPredTree &predicateSet );

      INT32 parseLogicOp( const bson::BSONElement &beField,
                          clsCataHashPredTree &predicateSet );

      INT32 checkOpObj( const bson::BSONObj obj,
                        INT32 &result );

      INT32 checkETInnerObj( const bson::BSONObj obj, INT32 &result ) ;

   private:
      clsCataHashPredTree        _predicateSet;
      bson::BSONObj              _shardingKey;
      bson::BSONObj              _matcher;
   };
}

#endif // CLS_CATA_HASH_MATCHER_HPP_
