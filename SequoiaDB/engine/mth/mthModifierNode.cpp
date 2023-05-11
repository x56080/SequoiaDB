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

   Source File Name = mthModifierNode.cpp

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

#include "mthModifierNode.hpp"
#include "mthMatchNode.hpp"

using namespace bson ;

namespace engine
{

   /*
      _mthModifierFactory implement
   */
   _mthModifierNode* _mthModifierNodeFactory::createModNode( ModType type,
                                                             const BSONElement &e,
                                                             INT32 dollarNum )
   {
      _mthModifierNode *pNode = NULL ;

      switch ( type )
      {
         case INC :
            pNode = SDB_OSS_NEW mthModifierIncNode( e, type, dollarNum ) ;
            break ;
         case CURRENT_DATE :
            pNode = SDB_OSS_NEW mthModifierCurDateNode( e, type, dollarNum );
            break ;
         default :
            pNode = SDB_OSS_NEW mthModifierNode( e, type, dollarNum ) ;
            break ;
      }

      return pNode ;
   }

   mthModifierNodeFactory* mthGetModifierNodeFactory()
   {
      static mthModifierNodeFactory s_modFactory ;
      return &s_modFactory ;
   }

   /*
      _mthModifierNode implement
   */
   INT32 _mthModifierNode::_parseField( const BSONElement &e )
   {
      INT32 rc = SDB_OK ;

      try
      {
         if ( Object == e.type() )
         {
            BSONObj subObj = e.Obj() ;
            BSONElement ele = subObj.getField( MTH_OPERATOR_STR_FIELD ) ;
            if ( !ele.eoo() )
            {
               if ( 1 != subObj.nFields() )
               {
                  PD_LOG_MSG( PDERROR, "'$field' cannot be used with other "
                                       "field: %s",
                              subObj.toString().c_str() ) ;
                  rc = SDB_INVALIDARG ;
                  goto error ;
               }
               else if ( String != ele.type() || 0 == ossStrlen( ele.valuestrsafe() ) )
               {
                  PD_LOG_MSG( PDERROR, "Value of '$field' should be a valid "
                                       "field name: %s",
                              ele.toString().c_str() ) ;
                  rc = SDB_INVALIDARG ;
                  goto error ;
               }

               _sourceField = ele.valuestrsafe() ;
            }
         }
      }
      catch ( std::exception &e )
      {
         rc = ossException2RC( &e ) ;
         PD_LOG( PDERROR, "Unexpected exception occurred: %s", e.what() ) ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _mthModifierNode::init( BOOLEAN strictDataMode )
   {
      return _parseField( _toModify ) ;
   }

   INT32 _mthModifierNode::validate( const BSONElement &resultEle )
   {
      return SDB_OK ;
   }

   /*
      _mthModifierIncNode implement
   */
   INT32 _mthModifierIncNode::init( BOOLEAN strictDataMode )
   {
      INT32 rc = SDB_OK ;

      /*
      Format:
         { $inc : { <fieldName> : <number> [...] } }
         { $inc : { <fieldName> : { $field : <fieldName> } [...] } }

      OR:
         { $inc : { <fieldName> : { Value : <value>, [ Default : x, Min : y, Max : z ] } [...] } }
         { $inc : { <fieldName> : { Value : { $field : <fieldName> }, [ Default : x, Min : y, Max : z ] } [...] } }
      */

      try
      {
         if ( _toModify.isNumber() )
         {
            _isSimple = TRUE ;
            rc = _mthModifierNode::init( strictDataMode ) ;
            if ( rc )
            {
               goto error ;
            }
         }
         else if ( Object == _toModify.type() )
         {
            BSONElement eField ;
            BOOLEAN hasValue = FALSE ;

            BSONObjIterator itr( _toModify.embeddedObject() ) ;
            while( itr.more() )
            {
               BSONElement e = itr.next() ;
               const CHAR *pFieldName = e.fieldName() ;

               if ( 0 == ossStrcmp( pFieldName, MTH_MOD_INC_VALUE ) )
               {
                  _valueEle = e ;
                  hasValue = TRUE ;
               }
               else if ( 0 == ossStrcmp( pFieldName, MTH_MOD_INC_DEFAULT ) )
               {
                  _default = e ;
                  hasValue = TRUE ;
               }
               else if ( 0 == ossStrcmp( pFieldName, MTH_MOD_INC_MIN ) )
               {
                  _minEle = e ;
                  hasValue = TRUE ;
               }
               else if ( 0 == ossStrcmp( pFieldName, MTH_MOD_INC_MAX ) )
               {
                  _maxEle = e ;
                  hasValue = TRUE ;
               }
               else if ( 0 == ossStrcmp( pFieldName, MTH_OPERATOR_STR_FIELD ) )
               {
                  eField = e ;
               }
               else
               {
                  PD_LOG_MSG( PDERROR, "Unrecognized option for $inc: %s", pFieldName ) ;
                  rc = SDB_INVALIDARG ;
                  goto error ;
               }
            }

            // Use can use either the format {$inc:{a:{$field:"b"}} or
            // {$inc:{a:{Value:1...}}}, but not both of them at the same time.
            // Wrong format: {$inc:{a:{$field:"b", Value:1...}}}
            if ( !eField.eoo() && hasValue )
            {
               PD_LOG_MSG( PDERROR, "'$field' can not be used at the same level with "
                                    "'Value/Default/Min/Max'") ;
               rc = SDB_INVALIDARG ;
               goto error ;
            }
            else if ( !eField.eoo() )
            {
               /// only $field
               _isSimple = TRUE ;
               rc = _mthModifierNode::init( strictDataMode ) ;
               if ( rc )
               {
                  goto error ;
               }
            }
            else
            {
               if ( !( _valueEle.isNumber() || Object == _valueEle.type() ) ||
                    !( _minEle.isNumber() || _minEle.eoo() ) ||
                    !( _maxEle.isNumber() || _maxEle.eoo() ) ||
                    !( _default.isNumber() || _default.eoo() || _default.isNull() ) )
               {
                  rc = SDB_INVALIDARG ;
               }
               else if ( Object == _valueEle.type() )
               {
                  rc = _parseField( _valueEle ) ;
                  if ( rc )
                  {
                     goto error ;
                  }
               }

               if ( rc )
               {
                  PD_LOG_MSG( PDERROR, "Operator[%s] param[%s] is invalid, The format shoud be: "
                              "{%s : {<name> : <number> }} or "
                              "{%s : {<name> : {$field : <name>}}} or "
                              "{%s : {<name> : {Value : x, [Default : x, Min : y, Max : z]}}} or "
                              "{%s : {<name> : {Value : {$field : <name>}, [Default : ...]}}}",
                              MTH_MODIFIER_INC, _toModify.toString().c_str(),
                              MTH_MODIFIER_INC, MTH_MODIFIER_INC, MTH_MODIFIER_INC,
                              MTH_MODIFIER_INC ) ;
                  goto error ;
               }

               _isSimple = FALSE ;

               /// calc default
               rc = _calcDefaultResult( strictDataMode ) ;
               if ( rc )
               {
                  goto error ;
               }
            }
         }
         else
         {
            PD_LOG_MSG( PDERROR, "The value type of $inc operator should be number"
                                 " or object: %s", _toModify.toString().c_str() ) ;
            rc = SDB_INVALIDARG ;
            goto error ;
         }
      }
      catch( std::exception &e )
      {
         rc = ossException2RC( &e ) ;
         PD_LOG( PDERROR, "Occur exception: %s", e.what() ) ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _mthModifierIncNode::_calcDefaultResult( BOOLEAN strictMode )
   {
      INT32 rc = SDB_OK ;

      if ( _isSimple )
      {
         goto done ;
      }

      // check param
      if ( _minEle.isNumber() && _maxEle.isNumber() )
      {
         if ( _maxEle.woCompare( _minEle, FALSE ) < 0 )
         {
            rc = SDB_INVALIDARG ;
            PD_LOG_MSG( PDERROR, "Max(%s) is less then Min(%s):rc=%d",
                        _maxEle.toString( FALSE ).c_str(),
                        _minEle.toString( FALSE ).c_str(), rc ) ;
            goto error ;
         }
      }

      if ( _minEle.type() == NumberDecimal )
      {
         _minDecimal = _minEle.numberDecimal() ;
      }

      if ( _maxEle.type() == NumberDecimal )
      {
         _maxDecimal = _maxEle.numberDecimal() ;
      }

      // calculate default result
      if ( _default.isNumber() || _default.eoo() )
      {
         BSONObj tmpValue ;
         BSONElement leftEle ;
         BSONObjBuilder builder( 20 ) ;

         if ( _default.eoo() )
         {
            tmpValue = BSON( "" << 0 ) ;
            leftEle = tmpValue.firstElement() ;
         }
         else
         {
            leftEle = _default ;
         }

         if ( mthIsBiggerNumberType( _minEle, leftEle ) ||
              mthIsBiggerNumberType( _maxEle, leftEle ) )
         {
            strictMode = FALSE ;
         }

         if ( !_sourceField )
         {
            // Increase by fixed value, the _defaultResult can be calculated
            // at parsing time.
            rc = mthModifierInc( leftEle, _valueEle, strictMode, builder ) ;
            if ( SDB_OK != rc )
            {
               PD_LOG_MSG( PDERROR, "Invalid inc:default=%s,inc=%s,min=%s,max=%s,"
                                    "rc=%d", leftEle.toString( FALSE ).c_str(),
                           _valueEle.toString( FALSE ).c_str(),
                           _minEle.toString( FALSE ).c_str(),
                           _maxEle.toString( FALSE ).c_str(), rc ) ;
               goto error ;
            }
            _defaultResult = builder.obj() ;
            if ( 0 == _defaultResult.nFields() )
            {
               // empty builder imply leftEle is not changed. set leftEle as result
               BSONObjBuilder defaultBuilder( 20 ) ;
               defaultBuilder.appendAs( leftEle, "" ) ;
               _defaultResult = defaultBuilder.obj() ;
            }
         }
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _mthModifierIncNode::validate( const BSONElement &resultEle )
   {
      INT32 rc = SDB_OK ;
      INT32 compRC = 0 ;

      if ( _isSimple )
      {
         goto done ;
      }

      if ( EOO == resultEle.type() )
      {
         goto done ;
      }

      if ( _minEle.isNumber() )
      {
         if ( (resultEle.type() == NumberInt || resultEle.type() == NumberLong) &&
              _minEle.type() == NumberDecimal )
         {
            compRC = _minDecimal.compareLong( resultEle.numberLong() ) ;
         }
         else
         {
            compRC = _minEle.woCompare( resultEle, FALSE ) ;
         }

         if ( compRC > 0 )
         {
            rc = SDB_VALUE_OVERFLOW ;
            PD_LOG_MSG( PDERROR, "Result[%s] is less than min[%s], rc: %d",
                        resultEle.toString( FALSE ).c_str(),
                        _minEle.toString( FALSE ).c_str(), rc ) ;
            goto error ;
         }
      }

      if ( _maxEle.isNumber() )
      {
         if ( (resultEle.type() == NumberInt || resultEle.type() == NumberLong) &&
              _maxEle.type() == NumberDecimal )
         {
            compRC = _maxDecimal.compareLong( resultEle.numberLong() ) ;
         }
         else
         {
            compRC = _maxEle.woCompare( resultEle, FALSE ) ;
         }

         if ( compRC < 0 )
         {
            rc = SDB_VALUE_OVERFLOW ;
            PD_LOG_MSG( PDERROR, "Result[%s] is grater than max[%s], rc: %d",
                        resultEle.toString( FALSE ).c_str(),
                        _maxEle.toString( FALSE ).c_str(), rc ) ;
            goto error ;
         }
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   /*
      _mthModifierCurDateNode implement
   */
   INT32 _mthModifierCurDateNode::_analyzeModifyEle( BSONElement e )
   {
      INT32 rc = SDB_OK ;

      if ( Bool == e.type() )
      {
         _isDate = e.Bool() ;
      }
      else if ( NumberInt == e.type() )
      {
         if ( Date == e.numberInt() )
         {
            _isDate = TRUE ;
         }
         else if ( Timestamp == e.numberInt() )
         {
            _isDate = FALSE ;
         }
         else
         {
            rc = SDB_INVALIDARG ;
            goto error ;
         }
      }
      else if ( String == e.type() )
      {
         if ( 0 == ossStrcasecmp( e.valuestr(), MTH_CAST_STR_DATE ) )
         {
            _isDate = TRUE ;
         }
         else if ( 0 == ossStrcasecmp( e.valuestr(), MTH_CAST_STR_TIMESTAMP ) )
         {
            _isDate = FALSE ;
         }
         else
         {
            rc = SDB_INVALIDARG ;
            goto error ;
         }
      }
      /// unknown type
      else
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   BSONElement _mthModifierCurDateNode::calc()
   {
      ossTimestamp tm ;
      ossGetCurrentTime( tm ) ;

      _builder.reset() ;

      if ( isDate() )
      {
         UINT64 millsec = ossTimestampToMilliseconds( tm ) ;
         _builder.appendDate( "", (Date_t)millsec ) ;
      }
      else
      {
         OpTime opTm( tm.time, tm.microtm ) ;
         _builder.appendTimestamp( "", opTm.asDate() ) ;
      }

      return _builder.done().firstElement() ;
   }

   INT32 _mthModifierCurDateNode::init( BOOLEAN strictDataMode )
   {
      INT32 rc = SDB_OK ;

      /*
      Format:
         { $currentDate : { <fieldName> : true/false } }
         { $currentDate : { <fieldName> : 9/17 } }
         { $currentDate : { <fieldName> : "date"/"timestamp" } }

         { $currentDate : { <fieldName> : { "$type" : true/false } } }
         { $currentDate : { <fieldName> : { "$type" : 9/17 } } }
         { $currentDate : { <fieldName> : { "$type" : "date"/"timestamp" } } }
      */

      try
      {
         if ( Object != _toModify.type() )
         {
            rc = _analyzeModifyEle( _toModify ) ;
         }
         else
         {
            BSONObj typeObj = _toModify.embeddedObject() ;
            if ( typeObj.nFields() != 1 && 0 != ossStrcmp( typeObj.firstElementFieldName(),
                                                           MTH_MOD_CURDATE_TYPE ) )
            {
               rc = SDB_INVALIDARG ;
            }
            else
            {
               rc = _analyzeModifyEle( typeObj.firstElement() ) ;
            }
         }

         if ( rc )
         {
            PD_LOG_MSG( PDERROR, "Operator[%s] param(%s) is invalid. The format should be: "
                        "{ %s : {<name> : true/false/9/17/'date'/'timestamp'}} or "
                        "{ %s : {<name> : {$type : true/false/9/17/'date'/'timestamp'}}}",
                       MTH_MODIFIER_CURRENTDATE, _toModify.toString().c_str(),
                       MTH_MODIFIER_CURRENTDATE, MTH_MODIFIER_CURRENTDATE ) ;
            goto error ;
         }
      }
      catch( std::exception &e )
      {
         PD_LOG_MSG( PDERROR, "Occur exception: %s", e.what() ) ;
         rc = ossException2RC( &e ) ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

}


