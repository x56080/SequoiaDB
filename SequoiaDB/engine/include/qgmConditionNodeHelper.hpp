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

   Source File Name = qgmConditionNodeHelper.hpp

   Descriptive Name =

   When/how to use: this program may be used on binary and text-formatted
   versions of PMD component. This file contains functions for agent processing.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/14/2012  YW  Initial Draft

   Last Changed =

*******************************************************************************/

#ifndef QGMCONDITIONNODEHELPER_HPP_
#define QGMCONDITIONNODEHELPER_HPP_

#include "qgmConditionNode.hpp"
#include <sstream>

using namespace std ;
using namespace bson ;

namespace engine
{
   class _qgmConditionNodeHelper : public SDBObject
   {
   public:
      _qgmConditionNodeHelper( _qgmConditionNode *root ) ;
      virtual ~_qgmConditionNodeHelper() ;

      _qgmConditionNode* getRoot() { return _root ; }
      void setRoot( _qgmConditionNode *pRoot ) { _root = pRoot ; }

   public:
      static void releaseNodes( qgmConditionNodePtrVec &nodes ) ;

   public:
      string toJson() const ;
      string toString() const ;

      BSONObj toBson( BOOLEAN keepAlias = TRUE ) const ;

      /// get all fields in condition tree.
      /// eg: a > 1 and b < d and c = "abc"
      /// --> a, b, d, c
      void getAllAttr( qgmDbAttrPtrVec &fields ) ;

      /// _qgmConditionNode in nodes should be freed by caller.
      INT32 separate( qgmConditionNodePtrVec &nodes ) ;

      INT32 merge( qgmConditionNodePtrVec &nodes ) ;

      INT32 merge( _qgmConditionNode *node ) ;

   private:
      void _getAllAttr( _qgmConditionNode *node,
                        qgmDbAttrPtrVec &fields ) ;

      void  _separate( _qgmConditionNode *predicate,
                       qgmConditionNodePtrVec &nodes ) ;

      template< class Builder >
      INT32 _crtBson( const _qgmConditionNode *node,
                      Builder &bb,
                      BOOLEAN keepAlias ) const ;

   private:
      _qgmConditionNode *_root ;
   } ;
   typedef class _qgmConditionNodeHelper qgmConditionNodeHelper ;
}

#endif // QGMCONDITIONNODEHELPER_HPP_

