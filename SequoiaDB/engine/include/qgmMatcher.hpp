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

   Source File Name = qgmMatcher.hpp

   Descriptive Name =

   When/how to use: this program may be used on binary and text-formatted
   versions of PMD component. This file contains declare for QGM operators

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          04/09/2013  YW  Initial Draft

   Last Changed =

*******************************************************************************/

#ifndef QGMMATCHER_HPP_
#define QGMMATCHER_HPP_

#include "qgmDef.hpp"
#include <utilMap.hpp>

using namespace bson ;

namespace engine
{
   struct _qgmConditionNode ;

   struct _qgmMatcherDataNode
   {
      BSONObj        _obj ;
      BSONElement    _element ;

      _qgmMatcherDataNode( BSONObj obj = BSONObj() )
      {
         _obj = obj ;
         _element = _obj.firstElement() ;
      }
   } ;
   typedef _qgmMatcherDataNode qgmMatcherDataNode ;

   /*
      _qgmMatcher define
   */
   class _qgmMatcher : public SDBObject
   {
   typedef _utilMap< void*, qgmMatcherDataNode, 8 >      MAP_DATA_NODE ;
   typedef MAP_DATA_NODE::iterator                       MAP_DATA_NODE_IT ;

   public:
      _qgmMatcher( _qgmConditionNode *node ) ;
      virtual ~_qgmMatcher() ;

      void     resetDataNode() ;

   public:
      OSS_INLINE BOOLEAN ready(){return _ready ;}

      string toString() const ;

      INT32 match( const qgmFetchOut &fetch,
                   BOOLEAN &r ) ;

   private:
      INT32 _match( const _qgmConditionNode *node,
                    const qgmFetchOut &fetch,
                    BOOLEAN &r ) ;

   private:
      _qgmConditionNode *_condition ;
      BOOLEAN           _ready ;

      MAP_DATA_NODE     _mapDataNode ;

   } ;

   typedef class _qgmMatcher qgmMatcher ;
}

#endif

