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

   Source File Name = mthMatchLogicNode.hpp

   Descriptive Name = Method Match logic Node Header

   When/how to use: this program may be used on binary and text-formatted
   versions of Method component. This file contains structure for matching
   operation, which is indicating whether a record matches a given matching
   rule.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          07/27/2016  LinYouBin    Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef MTH_MATCH_LOGICNODE_HPP_
#define MTH_MATCH_LOGICNODE_HPP_
#include "core.hpp"
#include "oss.hpp"
#include "ossUtil.hpp"
#include "../bson/bson.hpp"
#include "mthMatchNode.hpp"
#include <vector>

using namespace bson ;
using namespace std ;

namespace engine
{
   class _mthMatchLogicNode : public _mthMatchNode
   {
      public:
         _mthMatchLogicNode( _mthNodeAllocator *allocator ) ;
         virtual ~_mthMatchLogicNode() ;

      public: /* from parent */
         virtual INT32 init( const CHAR *fieldName, 
                             const BSONElement &element ) ;
         virtual void clear() ;
         virtual void setWeight( UINT32 weight ) ;
         virtual UINT32 getWeight() ;
         virtual BOOLEAN isTotalConverted() ;
         virtual BSONObj toBson() ;

      protected:
         virtual INT32 _init( const CHAR *fieldName, 
                              const BSONElement &element ) ;
         virtual void _clear() ;

      protected:
         INT32 _weight ;
   } ;

   class _mthMatchLogicAndNode : public _mthMatchLogicNode
   {
      public:
         _mthMatchLogicAndNode( _mthNodeAllocator *allocator ) ;
         virtual ~_mthMatchLogicAndNode() ;

      public:
         virtual INT32 getType() ;
         virtual const CHAR* getOperatorStr() ;
         virtual INT32 execute( const BSONObj &obj, 
                                _mthMatchTreeContext &context,
                                BOOLEAN &result ) ;
         virtual BOOLEAN isTotalConverted() ;
         virtual void release() ;
   } ;

   class _mthMatchLogicOrNode : public _mthMatchLogicNode
   {
      public:
         _mthMatchLogicOrNode( _mthNodeAllocator *allocator ) ;
         virtual ~_mthMatchLogicOrNode() ;

      public:
         virtual INT32 getType() ;
         virtual const CHAR* getOperatorStr() ;
         virtual INT32 execute( const BSONObj &obj, 
                                _mthMatchTreeContext &context,
                                BOOLEAN &result ) ;
         virtual INT32 calcPredicate( _rtnPredicateSet &predicateSet ) ;
         virtual INT32 extraEqualityMatches( BSONObjBuilder &builder ) ;
         virtual void release() ;
   } ;

   class _mthMatchLogicNotNode : public _mthMatchLogicAndNode
   {
      public:
         _mthMatchLogicNotNode( _mthNodeAllocator *allocator ) ;
         virtual ~_mthMatchLogicNotNode() ;

      public:
         virtual INT32 getType() ;
         virtual const CHAR* getOperatorStr() ;
         virtual INT32 execute( const BSONObj &obj, 
                                _mthMatchTreeContext &context,
                                BOOLEAN &result ) ;
         virtual INT32 calcPredicate( _rtnPredicateSet &predicateSet ) ;
         virtual INT32 extraEqualityMatches( BSONObjBuilder &builder ) ;
         virtual BOOLEAN isTotalConverted() ;
         virtual void release() ;
   } ;
}

#endif

