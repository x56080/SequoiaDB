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

   Source File Name = mthMatchLogicNode.cpp

   Descriptive Name = Method MatchLogicNode

   When/how to use: this program may be used on binary and text-formatted
   versions of Method component. This file contains functions for matcher, which
   indicates whether a record matches a given matching rule.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          07/14/2016  LinYouBin  Initial Draft

   Last Changed =

*******************************************************************************/
#include "mthMatchLogicNode.hpp"
#include "pd.hpp"

using namespace bson ;

namespace engine
{
   _mthMatchLogicNode::_mthMatchLogicNode( _mthNodeAllocator *allocator )
                      :_mthMatchNode( allocator )
   {
      
   }

   _mthMatchLogicNode::~_mthMatchLogicNode()
   {
      clear() ;
   }

   INT32 _mthMatchLogicNode::init( const CHAR *fieldName, 
                                   const BSONElement &element )
   {
      INT32 rc = SDB_OK ;
      rc = _mthMatchNode::init( fieldName, element ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "setFieldName failed:fieldName=%s,rc=%d", 
                 fieldName, rc ) ;
         goto error ;
      }

      rc = _init( fieldName, element ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "_init failed:rc=%d", rc ) ;
         goto error ;
      }

   done:
      return rc ;
   error:
      clear() ;
      goto done ;
   }

   INT32 _mthMatchLogicNode::_init( const CHAR *fieldName, 
                                    const BSONElement &element )
   {
      return SDB_OK ;
   }

   void _mthMatchLogicNode::clear()
   {
      _clear() ;
      _mthMatchNode::clear() ;
   }

   void _mthMatchLogicNode::_clear()
   {
      return ;
   }

   void _mthMatchLogicNode::setWeight( UINT32 weight )
   {
      _weight = weight ;
   }

   UINT32 _mthMatchLogicNode::getWeight()
   {
      return _weight ;
   }

   BOOLEAN _mthMatchLogicNode::isTotalConverted()
   {
      return FALSE ;
   }

   BSONObj _mthMatchLogicNode::toBson()
   {
      BSONObjBuilder builder ;
      BSONArrayBuilder sub( builder.subarrayStart( getOperatorStr() ) ) ;
      MATCHNODE_VECTOR::iterator iter = _children.begin() ;
      while ( iter != _children.end() )
      {
         sub << ( *iter )->toBson() ;
         iter++ ;
      }

      sub.doneFast() ;

      return builder.obj() ;
   }

   //*******************_mthMatchLogicAndNode***********************
   _mthMatchLogicAndNode::_mthMatchLogicAndNode( _mthNodeAllocator *allocator )
                         :_mthMatchLogicNode( allocator )
   {
   }

   _mthMatchLogicAndNode::~_mthMatchLogicAndNode()
   {
      clear() ;
   }

   INT32 _mthMatchLogicAndNode::getType()
   {
      return ( INT32 ) EN_MATCH_OPERATOR_LOGIC_AND ;
   }

   const CHAR* _mthMatchLogicAndNode::getOperatorStr()
   {
      return MTH_OPERATOR_STR_AND ;
   }

   INT32 _mthMatchLogicAndNode::execute( const BSONObj &obj, 
                                         _mthMatchTreeContext &context,
                                         BOOLEAN &result )
   {
      INT32 rc          = SDB_OK ;
      BOOLEAN tmpResult = TRUE ;

      result = TRUE ;
      _mthMatchNodeIterator iter( this ) ;
      while ( iter.more() )
      {
         _mthMatchNode *node = iter.next() ;
         rc = node->execute( obj, context, tmpResult ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "execute node failed:obj=%s,node=%s,rc=%d",
                    obj.toString().c_str(), node->toString().c_str(), rc ) ;
            goto error ;
         }

         if ( !tmpResult )
         {
            result = tmpResult ;
            if ( !context.isDollarListEnabled() )
            {
               break ;
            }
         }
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   BOOLEAN _mthMatchLogicAndNode::isTotalConverted()
   {
      //treat and as true
      return TRUE ;
   }

   void _mthMatchLogicAndNode::release()
   {
      if ( NULL != _allocator && _allocator->isAllocatedByme( this ) )
      {
         this->~_mthMatchLogicAndNode() ;
      }
      else
      {
         delete this ;
      }
   }

   //*******************_mthMatchLogicAndNode***********************
   _mthMatchLogicOrNode::_mthMatchLogicOrNode( _mthNodeAllocator *allocator )
                        :_mthMatchLogicNode( allocator )
   {
   }

   _mthMatchLogicOrNode::~_mthMatchLogicOrNode()
   {
      clear() ;
   }
   
   INT32 _mthMatchLogicOrNode::getType()
   {
      return ( INT32 ) EN_MATCH_OPERATOR_LOGIC_OR ;
   }

   const CHAR* _mthMatchLogicOrNode::getOperatorStr()
   {
      return MTH_OPERATOR_STR_OR ;
   }

   INT32 _mthMatchLogicOrNode::execute( const BSONObj &obj, 
                                        _mthMatchTreeContext &context,
                                        BOOLEAN &result )
   {
      INT32 rc          = SDB_OK ;
      BOOLEAN tmpResult = FALSE ;

      if ( getChildrenCount() == 0 )
      {
         result = TRUE ;
         goto done ;
      }

      {
         result = FALSE ;
         _mthMatchNodeIterator iter( this ) ;
         while ( iter.more() )
         {
            _mthMatchNode *node = iter.next() ;
            rc = node->execute( obj, context, tmpResult ) ;
            if ( SDB_OK != rc )
            {
               PD_LOG( PDERROR, "execute node failed:obj=%s,node=%s,rc=%d",
                       obj.toString().c_str(), node->toString().c_str(), rc ) ;
               goto error ;
            }

            if ( tmpResult )
            {
               result = TRUE ;
               if ( !context.isDollarListEnabled() )
               {
                  break ;
               }
            }
         }
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _mthMatchLogicOrNode::calcPredicate( _rtnPredicateSet &predicateSet )
   {
      // Logic or do not have predicatekey.
      return SDB_OK ;
   }

   INT32 _mthMatchLogicOrNode::extraEqualityMatches( BSONObjBuilder &builder )
   {
      // do not extraEqualityMatches in logic or.
      return SDB_OK ;
   }

   void _mthMatchLogicOrNode::release()
   {
      if ( NULL != _allocator && _allocator->isAllocatedByme( this ) )
      {
         this->~_mthMatchLogicOrNode() ;
      }
      else
      {
         delete this ;
      }
   }

   //*******************_mthMatchLogicNotNode***************************
   _mthMatchLogicNotNode::_mthMatchLogicNotNode( _mthNodeAllocator *allocator )
                         :_mthMatchLogicAndNode( allocator )
   {
   }

   _mthMatchLogicNotNode::~_mthMatchLogicNotNode()
   {
      clear() ;
   }

   INT32 _mthMatchLogicNotNode::getType()
   {
      return ( INT32 ) EN_MATCH_OPERATOR_LOGIC_NOT ;
   }

   const CHAR* _mthMatchLogicNotNode::getOperatorStr()
   {
      return MTH_OPERATOR_STR_NOT ;
   }

   INT32 _mthMatchLogicNotNode::execute( const BSONObj &obj, 
                                         _mthMatchTreeContext &context,
                                         BOOLEAN &result )
   {
      INT32 rc = SDB_OK ;
      BOOLEAN tmpResult = FALSE ;

      if ( getChildrenCount() == 0 )
      {
         result = TRUE ;
         goto done ;
      }

      rc = _mthMatchLogicAndNode::execute( obj, context, tmpResult ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "execute _mthMatchLogicNotNode failed:obj=%s,rc=%d",
                 obj.toString().c_str(), rc ) ;
         goto error ;
      }

      result = !tmpResult ;

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _mthMatchLogicNotNode::calcPredicate( _rtnPredicateSet &predicateSet )
   {
      // Logic not do not have predicatekey.
      return SDB_OK ;
   }

   INT32 _mthMatchLogicNotNode::extraEqualityMatches( BSONObjBuilder &builder )
   {
      // do not extraEqualityMatches in logic not.
      return SDB_OK ;
   }

   BOOLEAN _mthMatchLogicNotNode::isTotalConverted()
   {
      return FALSE ;
   }

   void _mthMatchLogicNotNode::release()
   {
      if ( NULL != _allocator && _allocator->isAllocatedByme( this ) )
      {
         this->~_mthMatchLogicNotNode() ;
      }
      else
      {
         delete this ;
      }
   }
}


