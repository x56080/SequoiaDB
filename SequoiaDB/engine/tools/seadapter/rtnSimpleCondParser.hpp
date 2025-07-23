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

   Source File Name = rtnSimpleCondParser.hpp

   Descriptive Name = Simple condition parser

   When/how to use: this program may be used on binary and text-formatted
   versions of data management component. This file contains structure for
   DMS storage unit and its methods.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          08/18/2017  YSD  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef RTN_SIMPLECONDPARSER_HPP__
#define RTN_SIMPLECONDPARSER_HPP__

#include "core.hpp"
#include "../bson/bson.h"
#include "rtnSimpleCondNode.hpp"

using namespace bson ;
using namespace engine ;

namespace seadapter
{
   // Factory takes care of creating and releasing nodes of a parse tree.
   class _rtnCondNodeFactory : public SDBObject
   {
      public:
         _rtnCondNodeFactory() ;
         ~_rtnCondNodeFactory() ;

      public:
         rtnCondNode* createNode( rtnCondNodeAllocator *allocator,
                                  RTN_COND_NODE_TYPE type ) ;
         void releaseNode( rtnCondNode* node ) ;
   } ;
   typedef _rtnCondNodeFactory rtnCondNodeFactory ;

   // Parse a condition which represents in a BSONObj.
   class _rtnSimpleCondParseTree : public SDBObject
   {
      typedef map<string, rtnCondNode*> TAG_NODE_MAP ;

      public:
         _rtnSimpleCondParseTree() ;
         ~_rtnSimpleCondParseTree() ;

         // tagFieldNames is used to put tags on the nodes with these names.
         // We can later get the node by use the name.
         INT32 parse( const BSONObj &condition ) ;
         void clear() ;
         BOOLEAN hasTextCond() ;
         rtnCondNode* getTextNode() ;
         BOOLEAN textNodeInNot() const { return _textNodeInNot ; }
         BOOLEAN textNodeInOr() const { return _textNodeInOr; }
         INT32 updateNode( rtnCondNode *node, const BSONElement &newEle ) ;
         BSONObj toBson() ;

      private:
         INT32 _parseElement( const BSONElement &ele, rtnCondNode *parent ) ;
         INT32 _parseObjectElement( const BSONElement &ele,
                                    rtnCondNode *parent ) ;
         INT32 _praseArrayElement( const BSONElement &ele,
                                   rtnCondNode *parent ) ;
         INT32 _parseNormalElement( const BSONElement &ele,
                                    rtnCondNode *parent ) ;
         INT32 _parseLogicAnd( const BSONElement &ele, rtnCondNode *parent ) ;
         INT32 _parseLogicOr( const BSONElement &ele, rtnCondNode *parent ) ;
         INT32 _parseLogicNot( const BSONElement &ele, rtnCondNode *parent ) ;
         INT32 _parseOpText( const BSONElement &ele, rtnCondNode *parent ) ;
         INT32 _parseLogicElements( const BSONElement &ele,
                                    rtnCondNode *parent ) ;
         INT32 _getElementKeysFormat( const BSONElement &ele ) ;
         void _releaseTree( rtnCondNode *root ) ;

      private:
         BSONObj              _condition ;
         rtnCondNode          *_root ;
         rtnCondNodeAllocator _allocator ;
         rtnCondNode          *_textNode ;
         // Whether text node is descendant of a $not clause.
         BOOLEAN              _textNodeInNot ;
         BOOLEAN              _textNodeInOr ;
   } ;
   typedef _rtnSimpleCondParseTree rtnSimpleCondParseTree ;

   rtnCondNodeFactory* rtnGetCondNodeFactory() ;
}

#endif /* RTN_SIMPLECONDPARSER_HPP__ */
