/*******************************************************************************


   Copyright (C) 2011-2023 SequoiaDB Ltd.

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

   Source File Name = mthModifierNode.hpp

   Descriptive Name = Method Modifier Header

   When/how to use: this program may be used on binary and text-formatted
   versions of Method component. This file contains structure for modify
   operation, which is changing a data record based on modification rule.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          05/11/2023  XJH Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef MTHMODIFIER_NODE_HPP_
#define MTHMODIFIER_NODE_HPP_

#include "mthModifierUtil.hpp"
#include "utilPooledObject.hpp"

using namespace bson ;

namespace engine
{

   class _mthModifierNode ;

   /*
      _mthModifierFactory define
   */
   class _mthModifierNodeFactory
   {
      public:
         _mthModifierNodeFactory() {}
         ~_mthModifierNodeFactory() {}

         _mthModifierNode* createModNode( ModType type,
                                          const BSONElement &e,
                                          INT32 dollarNum ) ;
   } ;
   typedef _mthModifierNodeFactory mthModifierNodeFactory ;

   mthModifierNodeFactory*    mthGetModifierNodeFactory() ;

   /*
      _mthModifierNode define
   */
   class _mthModifierNode : public utilPooledObject
   {
   public :
      BSONElement _toModify ;    // the element to modify
      const CHAR *_sourceField ; // Used in format {$field:<name>}, pointing to
                                 // the <name>, will be used in evaluation stage
                                 // to fetch the field from the source record.
      ModType     _modType ;
      INT32       _dollarNum ;

      _mthModifierNode ( const BSONElement &e, ModType type,
                         INT32 dollarNum = 0 )
      {
         _toModify = e ;
         _sourceField = NULL ;
         _modType = type ;
         _dollarNum = dollarNum ;
      }

      virtual ~_mthModifierNode() {}

      BOOLEAN isModifyByField() const
      {
         return ( NULL != _sourceField ) ;
      }

      const CHAR* getSourceFieldName() const
      {
         return _sourceField ;
      }

   public:
      virtual INT32 init( BOOLEAN strictDataMode ) ;
      virtual INT32 validate( const BSONElement &resultEle ) ;

   protected:
      INT32    _parseField( const BSONElement &e ) ;

   } ;
   typedef _mthModifierNode mthModifierNode ;
   typedef _mthModifierNode ModifierElement ;

   /*
      _mthModifierIncNode define
   */
   class _mthModifierIncNode : public _mthModifierNode
   {
   public :
      BOOLEAN     _isSimple ;
      BSONElement _valueEle ;
      BSONElement _default ;
      BSONElement _minEle ;
      bsonDecimal _minDecimal ;

      BSONElement _maxEle ;
      bsonDecimal _maxDecimal ;

      BSONObj     _defaultResult ;

      _mthModifierIncNode( const BSONElement &e, ModType type,
                           INT32 dollarNum = 0 )
                        : _mthModifierNode( e, type, dollarNum )
      {
         SDB_ASSERT( INC == type, "Type invalid" ) ;
         _isSimple = TRUE ;
      }

   public:
      virtual INT32 init( BOOLEAN strictDataMode ) ;
      virtual INT32 validate( const BSONElement &resultEle ) ;

   protected:
      INT32 _calcDefaultResult( BOOLEAN strictMode ) ;

   } ;
   typedef _mthModifierIncNode mthModifierIncNode ;

   /*
      _mthModifierCurDateNode define
   */
   class _mthModifierCurDateNode : public _mthModifierNode
   {
   public:
      _mthModifierCurDateNode( const BSONElement &e, ModType type,
                               INT32 dollarNum = 0 )
      : _mthModifierNode( e, type, dollarNum ),
        _builder( 30 )
      {
         SDB_ASSERT( CURRENT_DATE == type, "Type invalid" ) ;
         _isDate = TRUE ;
      }

      /// caller should try/catch
      BSONElement calc() ;

      BOOLEAN  isDate() const { return _isDate ; }

   public:
      virtual INT32 init( BOOLEAN strictDataMode ) ;

   protected:
      INT32 _analyzeModifyEle( BSONElement e ) ;

   private:
      BSONObjBuilder       _builder ;
      BOOLEAN              _isDate ;
   } ;
   typedef _mthModifierCurDateNode mthModifierCurDateNode ;

   /*
      _mthModFieldNamesCompare define
   */
   class _mthModFieldNamesCompare
   {
   private:
      vector<INT64> *_dollarList ;

   public:
      _mthModFieldNamesCompare( vector<INT64> *dollarList = NULL )
      : _dollarList(NULL)
      {
         _dollarList = dollarList ;
      }

      BOOLEAN operator () ( const mthModifierNode *l,
                            const mthModifierNode *r ) const
      {
         _mthModCompareNatrualOrder compare ( _dollarList ) ;
         FieldCompareResult result = compare.compField( l->_toModify.fieldName(),
                                                        r->_toModify.fieldName() ) ;
         return ( (result == RIGHT_SUBFIELD) || (result == LEFT_BEFORE) ) ;
      }
   } ;
   typedef _mthModFieldNamesCompare mthModFieldNamesCompare ;

}

#endif //MTHMODIFIER_NODE_HPP_

