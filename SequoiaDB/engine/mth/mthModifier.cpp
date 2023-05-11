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

   Source File Name = mthModifier.cpp

   Descriptive Name = Method Modifier

   When/how to use: this program may be used on binary and text-formatted
   versions of Method component. This file contains functions for creating a
   new record based on old record and modify rule.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/14/2012  TW  Initial Draft

   Last Changed =

*******************************************************************************/
#include "core.hpp"
#include <algorithm>
#include "pd.hpp"
#include "mthModifier.hpp"
#include "rtn.hpp"
#include "pdTrace.hpp"
#include "mthTrace.hpp"
#include "utilMath.hpp"
#include "mthDef.hpp"

using namespace bson ;
using namespace std ;

namespace engine
{

#define ADD_CHG_FIELD_VALUE( builder, fieldName, value, strChg ) \
      do { \
         _hasModified = TRUE ; \
         if ( builder ) \
         { \
            BSONObjBuilder subBuilder ( builder->subobjStart ( strChg ) ) ; \
            subBuilder.append ( fieldName, value ) ; \
            subBuilder.done () ; \
         } \
      } while ( 0 )

#define ADD_CHG_ELEMENT( builder, ele, strChg ) \
   do { \
      _hasModified = TRUE ; \
      if ( builder ) \
      { \
         BSONObjBuilder subBuilder ( builder->subobjStart ( strChg ) ) ; \
         subBuilder.append ( ele ) ; \
         subBuilder.done () ; \
      } \
   } while ( 0 )

#define ADD_CHG_OBJECT( builder, obj, name ) \
   do { \
      _hasModified = TRUE ; \
      if ( builder ) \
      { \
         builder->append( name, obj ) ; \
      } \
   } while ( 0 )


#define ADD_CHG_ELEMENT_AS( builder, ele, eleFieldName, strChg ) \
   do { \
      _hasModified = TRUE ; \
      if ( builder ) \
      { \
         BSONObjBuilder subBuilder ( builder->subobjStart ( strChg ) ) ; \
         subBuilder.appendAs ( ele, eleFieldName ) ; \
         subBuilder.done () ; \
      } \
   } while ( 0 )

#define ADD_CHG_UNSET_FIELD( builder, fieldName ) \
   do { \
      _hasModified = TRUE ; \
      if ( builder ) \
      { \
         BSONObjBuilder subBuilder ( builder->subobjStart ( "$unset" ) ) ; \
         subBuilder.append ( fieldName, "" ) ; \
         subBuilder.done () ; \
      } \
   } while ( 0 )

#define ADD_CHG_NUMBER( builder, fieldName, value, strChg ) \
   do { \
      _hasModified = TRUE ; \
      if ( builder ) \
      { \
         BSONObjBuilder subBuilder ( builder->subobjStart ( strChg ) ) ; \
         subBuilder.append ( fieldName, value ) ; \
         subBuilder.done () ; \
      } \
   } while ( 0 )

#define ADD_CHG_ARRAY_OBJ( builder, obj, objFiledName, strChg ) \
   do { \
      _hasModified = TRUE ; \
      if ( builder ) \
      { \
         BSONObjBuilder subBuilder ( builder->subobjStart ( strChg ) ) ; \
         subBuilder.appendArray ( objFiledName, obj ) ; \
         subBuilder.done () ; \
      } \
   } while ( 0 )

#define SET_ARRAY_POS_NAME    "pos"
#define SET_ARRAY_OBJS_NAME   "objs"

   /*
      _mthModifier implement
   */
   INT32 _mthModifier::_addToKeepSet( const CHAR *fieldName )
   {
      INT32 rc = SDB_OK ;
      try
      {
         _keepKeys.insert( fieldName ) ;
      }
      catch ( std::exception &e )
      {
         PD_LOG( PDERROR, "Failed to add to keepKeys: %s", e.what() ) ;
         rc = ossException2RC( &e ) ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   // Free element if failure
   INT32 _mthModifier::_addToModifierVector( ModifierElement *element )
   {
      INT32 rc = SDB_OK ;
      try
      {
         _modifierElements.push_back( element ) ;
      }
      catch ( std::exception &e )
      {
         PD_LOG( PDERROR, "Failed to add to modiferElements: %s", e.what() ) ;
         rc = ossException2RC( &e ) ;
         goto error ;
      }

   done:
      return rc ;
   error:
      SAFE_OSS_DELETE( element ) ;
      goto done ;
   }

   /*
      _mthModifier implement
      The parameter ele stands for the operands for the operator of type.
   */
   // PD_TRACE_DECLARE_FUNCTION ( SDB__MTHMDF__ADDMDF, "_mthModifier::_addModifier" )
   INT32 _mthModifier::_addModifier ( const BSONElement &ele, ModType type )
   {
      INT32 rc = SDB_OK ;
      INT32 dollarNum = 0 ;
      mthModifierNode *pNode = NULL ;

      PD_TRACE_ENTRY ( SDB__MTHMDF__ADDMDF );

      if ( RENAME == type )
      {
         if ( ele.type() != String || ossStrchr ( ele.valuestr(), '.' ) != NULL ||
              ossStrncmp ( ele.valuestr(), "$", 1 ) == 0 )
         {
            PD_LOG_MSG ( PDERROR, "Rename field must be string,and can't start "
                         "with '$', and can't include '.', %s",
                         ele.toString().c_str() ) ;
            goto error ;
         }
      }
      else if ( ( PUSH_ALL == type || PULL_ALL == type ||
                  PULL_ALL_BY == type ) &&
                Array != ele.type () )
      {
         PD_LOG_MSG ( PDERROR, "$push_all/pull_all/pull_all_by field must "
                      "be array, %s", ele.toString().c_str() ) ;
         goto error ;
      }
      else if ( POP == type && !ele.isNumber() &&
                !( ele.isABSONObj() &&
                   ( 1 == ele.Obj().nFields() ) &&
                   ( 0 == ossStrcmp( ele.Obj().firstElementFieldName(),
                                     MTH_OPERATOR_STR_FIELD ) ) ) )
      {
         PD_LOG_MSG ( PDERROR, "$pop field must be number, %s",
                      ele.toString().c_str() ) ;
         goto error ;
      }
      else if ( ( BITOR == type || BITAND == type ||
                  BITXOR == type || BITNOT == type ) &&
                !ele.isNumber() )
      {
         PD_LOG_MSG ( PDERROR, "bitor/bitand/bitxor/bitnot field must be "
                      "array, %s", ele.toString().c_str() ) ;
         goto error ;
      }
      else if ( BIT == type && !ele.isABSONObj() )
      {
         PD_LOG_MSG ( PDERROR, "bit field must be object , %s",
                      ele.toString().c_str()) ;
         goto error ;
      }
      else if ( ADDTOSET == type && Array != ele.type() )
      {
         PD_LOG_MSG ( PDERROR, "addtoset field must be array, %s",
                      ele.toString().c_str() ) ;
         goto error ;
      }
      else if ( ( UNSET == type || RENAME == type ) &&
                ossStrcmp ( ele.fieldName(), DMS_ID_KEY_NAME ) == 0 )
      {
         PD_LOG_MSG ( PDERROR, "ID field can't be renamed or unset" ) ;
         goto error ;
      }
      else if ( ( REPLACE == type || KEEP == type ) &&
                ( NULL != ossStrchr( ele.fieldName(), '.' ) ||
                  '$' == ele.fieldName()[0] ) )
      {
         PD_LOG_MSG ( PDERROR, "$replace and $keep's field name can't start "
                      "with '$', and can't include '.', %s",
                      ele.toString().c_str() ) ;
         goto error ;
      }

      /// then check the field name valid
      rc = mthCheckFieldName ( ele.fieldName(), dollarNum ) ;
      if ( rc )
      {
         PD_LOG_MSG ( PDERROR, "Faild field name : %s", ele.fieldName() ) ;
         goto error ;
      }

      if ( REPLACE == type &&
           0 == ossStrcmp( ele.fieldName(), DMS_ID_KEY_NAME ) )
      {
         _isReplaceID = TRUE ;
      }

      /// add to vector
      if ( KEEP == type )
      {
         rc = _addToKeepSet( ele.fieldName() ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to add to keep set:rc=%d", rc ) ;
      }
      else
      {
         pNode = mthGetModifierNodeFactory()->createModNode( type, ele, dollarNum ) ;
         PD_CHECK( NULL != pNode, SDB_OOM, error, PDERROR,
                   "Failed to create modifier node by type(%d), rc: %d", type, rc ) ;

         rc = pNode->init( _strictDataMode ) ;
         PD_RC_CHECK( rc, PDERROR, "Init modifier node failed, rc: %d", rc ) ;

         rc = _addToModifierVector( pNode ) ;
         /// should set to NULL, _addToModifierVector will takeover the owner
         pNode = NULL ;
         PD_RC_CHECK( rc, PDERROR, "Failed to add modifier:rc=%d", rc ) ;
      }

   done :
      if ( pNode )
      {
         SDB_OSS_DEL pNode ;
         pNode = NULL ;
      }
      PD_TRACE_EXITRC ( SDB__MTHMDF__ADDMDF, rc ) ;
      return rc ;
   error :
      if ( SDB_OK == rc )
      {
         rc = SDB_INVALIDARG ;
      }
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__MTHMDF__APPINCMDF, "_mthModifier::_applyIncModifier" )
   template<class Builder>
   INT32 _mthModifier::_applyIncModifier ( const CHAR *pRoot, Builder &bb,
                                           const BSONElement &in,
                                           mthModifierIncNode &me )
   {
      PD_TRACE_ENTRY ( SDB__MTHMDF__APPINCMDF );
      INT32 rc        = SDB_OK ;
      BSONObjBuilder builder( 20 ) ;
      BSONObj objResult ;
      BSONElement resultEle ;
      BSONElement elt ;
      BOOLEAN strictMode = _strictDataMode ;

      if ( me.isModifyByField() )
      {
         rc = _getFieldModifier( me.getSourceFieldName(), elt ) ;
         if ( rc )
         {
            PD_LOG_MSG( PDERROR, "Get value of '$field' failed: %d",
                        rc ) ;
            goto error ;
         }
         // If the specified field dose not exist in the original record, just
         // keep the original field.
         if ( elt.eoo() )
         {
            bb.append( in ) ;
            goto done ;
         }
         else if ( !elt.isNumber() )
         {
            PD_LOG_MSG( ( _ignoreTypeError ?  PDDEBUG : PDERROR),
                        "Field %s is not a nubmer in record: %s",
                        me.getSourceFieldName(), _sourceRecord.toString().c_str() ) ;
            if ( _ignoreTypeError )
            {
               bb.append( in ) ;
               goto done ;
            }
            else
            {
               rc = SDB_INVALIDARG ;
               goto error ;
            }
         }
      }
      else if ( me._isSimple )
      {
         elt = me._toModify ;
      }
      else
      {
         elt = me._valueEle ;
      }

      if ( mthIsBiggerNumberType( me._minEle, in ) ||
           mthIsBiggerNumberType( me._maxEle, in ) )
      {
         strictMode = FALSE ;
      }

      rc = mthModifierInc( in, elt, strictMode, builder ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "Failed to execute $inc:rc=%d", rc ) ;
         goto error ;
      }

      objResult = builder.obj() ;
      if ( objResult.nFields() > 0 )
      {
         resultEle = objResult.firstElement() ;
         rc = me.validate( resultEle ) ;
         if ( rc )
         {
            goto error ;
         }

         bb.appendAs( resultEle, in.fieldName() ) ;
         ADD_CHG_ELEMENT_AS( _srcChgBuilder, in, pRoot, "$set" ) ;
         ADD_CHG_ELEMENT_AS( _dstChgBuilder, resultEle, pRoot, "$set" ) ;
      }
      else
      {
         // empty builder imply in is not changed
         bb.append( in ) ;
      }

   done:
      PD_TRACE_EXIT ( SDB__MTHMDF__APPINCMDF ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__MTHMDF__APPSETMDF, "_mthModifier::_applySetModifier" )
   template<class Builder>
   INT32 _mthModifier::_applySetModifier( const CHAR *pRoot, Builder &bb,
                                          const BSONElement &in,
                                          ModifierElement &me )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB__MTHMDF__APPSETMDF ) ;
      BOOLEAN isNeedSetNewValue = TRUE ;
      BSONElement realEle ;
      BOOLEAN needUnset = FALSE ;

      if ( me.isModifyByField() )
      {
         // Use value of other field to set the current field.
         rc = _getFieldModifier( me.getSourceFieldName(), realEle ) ;
         if ( rc )
         {
            PD_LOG_MSG( PDERROR, "Get value of '$field' failed: %d",
                        rc ) ;
            goto error ;
         }

         // If the field specified by '$field' dose not exist in the source
         // record, remove the target field.
         if ( realEle.eoo() )
         {
            needUnset = TRUE ;
         }
      }
      else
      {
         realEle = me._toModify ;
      }

      if ( isNeedSetNewValue && in.type() == realEle.type()
           && 0 == in.woCompare( realEle, false ) )
      {
         isNeedSetNewValue = FALSE ;
      }

      if ( isNeedSetNewValue )
      {
         ADD_CHG_ELEMENT_AS ( _srcChgBuilder, in, pRoot, "$set" ) ;
         if ( needUnset )
         {
            BSONObjBuilder builder ;
            builder.append( in.fieldName(), "" ) ;
            ADD_CHG_ELEMENT_AS( _dstChgBuilder, builder.done().firstElement(),
                                pRoot, "$unset" ) ;
         }
         else
         {
            ADD_CHG_ELEMENT_AS ( _dstChgBuilder, realEle, pRoot, "$set" ) ;
            bb.appendAs ( realEle, in.fieldName() ) ;
         }
      }
      // not change
      else
      {
         bb.append ( in ) ;
      }

   done:
      PD_TRACE_EXITRC ( SDB__MTHMDF__APPSETMDF, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__MTHMDF__APPCURDATEMDF, "_mthModifier::_applyCurDateModifier" )
   template<class Builder>
   INT32 _mthModifier::_applyCurDateModifier( const CHAR *pRoot, Builder &bb,
                                              const BSONElement &in,
                                              ModifierElement &me )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB__MTHMDF__APPCURDATEMDF ) ;
      mthModifierCurDateNode *pCurDateNode = (mthModifierCurDateNode*)&me ;
      BOOLEAN isNeedSetNewValue = TRUE ;
      BSONElement realEle ;

      if ( ( Date != in.type() && Timestamp != in.type() ) && _strictDataMode )
      {
         PD_LOG_MSG( ( _ignoreTypeError ?  PDDEBUG : PDERROR),
                     "Field %s is not Date or Timestamp in record: %s",
                     pRoot, _sourceRecord.toString().c_str() ) ;
         if ( _ignoreTypeError )
         {
            bb.append( in ) ;
            goto done ;
         }
         else
         {
            rc = SDB_INVALIDARG ;
            goto error ;
         }
      }

      realEle = pCurDateNode->calc() ;

      if ( isNeedSetNewValue && in.type() == realEle.type()
           && 0 == in.woCompare( realEle, false ) )
      {
         isNeedSetNewValue = FALSE ;
      }

      if ( isNeedSetNewValue )
      {
         ADD_CHG_ELEMENT_AS ( _srcChgBuilder, in, pRoot, "$set" ) ;
         ADD_CHG_ELEMENT_AS ( _dstChgBuilder, realEle, pRoot, "$set" ) ;
         bb.appendAs ( realEle, in.fieldName() ) ;
      }
      // not change
      else
      {
         bb.append ( in ) ;
      }

   done:
      PD_TRACE_EXITRC ( SDB__MTHMDF__APPCURDATEMDF, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__MTHMDF__APPPSHMDF, "_mthModifier::_applyPushModifier" )
   template<class Builder>
   INT32 _mthModifier::_applyPushModifier ( const CHAR *pRoot, Builder &bb,
                                            const BSONElement &in,
                                            ModifierElement &me )
   {
      PD_TRACE_ENTRY ( SDB__MTHMDF__APPPSHMDF );
      INT32 rc= SDB_OK ;
      BSONElement actualModifyEle ;
      // make sure the original type is array
      if ( Array != in.type() )
      {
         PD_LOG_MSG ( ( _ignoreTypeError ? PDDEBUG : PDERROR ),
                      "Original data type is not array: %s",
                      in.toString().c_str() ) ;
         if ( _ignoreTypeError )
         {
            bb.append( in ) ;
         }
         else
         {
            rc = SDB_INVALIDARG ;
         }
         goto done ;
      }

      {
         // create bson builder for the array
         BSONObjBuilder sub ( bb.subarrayStart ( in.fieldName() ) ) ;
         BSONObjIterator i ( in.embeddedObject() ) ;
         INT32 n = 0 ;
         while ( i.more() )
         {
            sub.append( i.next() ) ;
            n++ ;
         }

         if ( me.isModifyByField() )
         {
            rc = _getFieldModifier( me.getSourceFieldName(), actualModifyEle ) ;
            if ( rc )
            {
               PD_LOG_MSG( PDERROR, "Get value of '$field' failed: %d",
                           rc ) ;
               goto error ;
            }
         }
         else
         {
            actualModifyEle = me._toModify ;
         }
         if ( !actualModifyEle.eoo() )
         {
            sub.appendAs ( actualModifyEle, sub.numStr(n) ) ;
            _buildSetArray( _dstChgBuilder, pRoot, n, actualModifyEle ) ;
            _buildSetArray( _srcChgBuilder, pRoot, n, BSONArrayBuilder().arr() ) ;
         }
         sub.done() ;
      }

   done :
      PD_TRACE_EXITRC ( SDB__MTHMDF__APPPSHMDF, rc );
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__MTHMDF__APPPSHALLMDF, "_mthModifier::_applyPushAllModifier" )
   template<class Builder>
   INT32 _mthModifier::_applyPushAllModifier ( const CHAR *pRoot, Builder & bb,
                                               const BSONElement & in,
                                               ModifierElement & me )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB__MTHMDF__APPPSHALLMDF );
      // make sure the original type is array
      if ( in.type() != Array )
      {
         PD_LOG_MSG ( ( _ignoreTypeError ? PDDEBUG : PDERROR ),
                      "Original data type is not array: %s",
                      in.toString().c_str() ) ;
         if ( _ignoreTypeError )
         {
            bb.append( in ) ;
         }
         else
         {
            rc = SDB_INVALIDARG ;
         }
         goto done ;
      }
      // make sure the new type is array too
      if ( me._toModify.type() != Array )
      {
         PD_LOG_MSG ( PDERROR, "pushed data type is not array: %s",
                      me._toModify.toString().c_str());
         rc = SDB_INVALIDARG ;
         goto done ;
      }
      {
         // create bson builder for the array
         BSONObjBuilder sub ( bb.subarrayStart ( in.fieldName() ) ) ;
         BSONObjIterator i ( in.embeddedObject()) ;
         INT32 n = 0 ;
         INT32 pushNum = 0 ;
         while ( i.more() )
         {
            sub.append( i.next() ) ;
            n++ ;
         }

         INT32 beginPos = n ;

         i = BSONObjIterator ( me._toModify.embeddedObject()) ;
         while( i.more() )
         {
            sub.appendAs( i.next(), sub.numStr(n++) ) ;
            ++pushNum ;
         }
         BSONObj newObj = sub.done() ;

         if ( 0 != pushNum )
         {
            _buildSetArray( _dstChgBuilder, pRoot, beginPos,
                            me._toModify.embeddedObject() ) ;
            _buildSetArray( _srcChgBuilder, pRoot, beginPos,
                            BSONArrayBuilder().arr() ) ;
         }
      }

   done :
      PD_TRACE_EXITRC ( SDB__MTHMDF__APPPSHALLMDF, rc );
      return rc ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__MTHMDF__APPPLLMDF, "_mthModifier::_applyPullModifier" )
   template<class Builder>
   INT32 _mthModifier::_applyPullModifier ( const CHAR *pRoot, Builder &bb,
                                            const BSONElement &in,
                                            ModifierElement &me )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB__MTHMDF__APPPLLMDF );
      // make sure the original type is array
      if ( in.type() != Array )
      {
         PD_LOG_MSG ( ( _ignoreTypeError ? PDDEBUG : PDERROR ),
                      "Original data type is not array: %s",
                      in.toString().c_str() ) ;
         if ( _ignoreTypeError )
         {
            bb.append( in ) ;
         }
         else
         {
            rc = SDB_INVALIDARG ;
         }
         goto done ;
      }
      {
         // need to create a builder regardless if pull success or not
         // even if all elements matches, we still need this empty array
         BSONObjBuilder sub ( bb.subarrayStart ( in.fieldName() ) ) ;
         INT32 n = 0 ;
         BOOLEAN changed = FALSE ;
         INT32 changedPos = 0 ;
         // for each element in the original data
         BSONObjIterator i ( in.embeddedObject() ) ;
         while ( i.more() )
         {
            BSONElement ele = i.next() ;
            BOOLEAN allowed = TRUE ;
            if ( PULL == me._modType || PULL_BY == me._modType )
            {
               BSONElement realEle ;
               if ( me.isModifyByField() )
               {
                  rc = _getFieldModifier( me.getSourceFieldName(), realEle ) ;
                  if ( rc )
                  {
                     PD_LOG_MSG( PDERROR, "Get value of '$field' failed: %d",
                                 rc ) ;
                     goto error ;
                  }
               }
               else
               {
                  realEle = me._toModify ;
               }
               allowed = ! _pullElementMatch ( ele, realEle,
                                               ( PULL == me._modType ?
                                                 TRUE : FALSE ) ) ;
            }
            else
            {
               BSONObjIterator j ( me._toModify.embeddedObject() ) ;
               while ( j.more() )
               {
                  BSONElement eleM = j.next() ;
                  if ( _pullElementMatch( ele, eleM,
                                          ( PULL_ALL == me._modType ?
                                            TRUE : FALSE ) ) )
                  {
                     allowed = FALSE ;
                     break ;
                  }
               }
            }

            if ( allowed )
            {
               sub.appendAs ( ele, sub.numStr(n++) ) ;
            }
            else
            {
               if ( !changed )
               {
                  changedPos = n ;
               }
               changed = TRUE ;
            }
         }
         BSONObj newObj = sub.done() ;

         if ( changed )
         {
            _buildSetArray( _dstChgBuilder, pRoot, changedPos, -1, newObj ) ;
            _buildSetArray( _srcChgBuilder, pRoot, changedPos, -1, in.embeddedObject() ) ;
         }
      }

   done :
      PD_TRACE_EXITRC ( SDB__MTHMDF__APPPLLMDF, rc );
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__MTHMDF__APPPOPMDF, "_mthModifier::_applyPopModifier" )
   template<class Builder>
   INT32 _mthModifier::_applyPopModifier ( const CHAR *pRoot, Builder &bb,
                                           const BSONElement &in,
                                           ModifierElement &me )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB__MTHMDF__APPPOPMDF );
      // remove the n'th element from array's front or end
      // if input number is 0, then it doesn't do anything
      // input number < 0 means remove from front
      // input number > 0 means remove from end

      BSONElement actualEle ;

      if ( Array != in.type() )
      {
         PD_LOG_MSG ( ( _ignoreTypeError ? PDDEBUG : PDERROR ),
                      "Original data type is not array: %s",
                      in.toString().c_str() ) ;
         if ( _ignoreTypeError )
         {
            bb.append( in ) ;
         }
         else
         {
            rc = SDB_INVALIDARG ;
         }
         goto done ;
      }

      if ( me.isModifyByField() )
      {
         rc = _getFieldModifier( me.getSourceFieldName(), actualEle ) ;
         if ( rc )
         {
            PD_LOG_MSG( PDERROR, "Get value of '$field' failed: %d",
                        rc ) ;
            goto error ;
         }
         if ( actualEle.eoo() )
         {
            bb.append( in ) ;
            goto done ;
         }
      }
      else
      {
         actualEle = me._toModify ;
      }

      if ( !actualEle.isNumber() )
      {
         PD_LOG_MSG ( PDERROR, "pop data type must be a number: %s",
                      me._toModify.toString().c_str());
         rc = SDB_INVALIDARG ;
         goto done ;
      }

      // if specify 0, which means don't pop anything
      if ( actualEle.number() == 0 )
      {
         bb.append ( in ) ;
         goto done ;
      }
      {
         BSONObjBuilder sub ( bb.subarrayStart ( in.fieldName() ) ) ;
         INT32 n = 0 ;
         INT32 changedPos = 0 ;
         // if specify < 0, which means pop the n'th element from front
         if ( actualEle.number() < 0 )
         {
            changedPos = 0 ;
            INT32 m = (INT32)actualEle.number() ;
            BSONObjIterator i ( in.embeddedObject() ) ;
            while ( i.more() )
            {
               m++ ;
               BSONElement be = i.next() ;
               if ( m > 0 )
               {
                  sub.appendAs( be, sub.numStr(n++) ) ;
               }
            }
         }
         else
         {
            // if specify > 0, we need to pop the n'th element from end
            // first we need to know how many elements in total
            INT32 count = 0 ;
            INT32 m = (INT32)actualEle.number() ;
            BSONObjIterator i ( in.embeddedObject() ) ;
            while ( i.more())
            {
               i.next() ;
               count++ ;
            }
            count = count - m ;
            changedPos = count > 0 ? count : 0 ;
            i = BSONObjIterator ( in.embeddedObject() ) ;
            while ( i.more() )
            {
               if ( count > 0 )
               {
                  sub.appendAs( i.next(), sub.numStr(n++) ) ;
               }
               else
               {
                  break ;
               }

               count-- ;
            }
         }
         BSONObj newObj = sub.done() ;

         _buildSetArray( _dstChgBuilder, pRoot, changedPos, -1, newObj ) ;
         _buildSetArray( _srcChgBuilder, pRoot, changedPos, -1, in.embeddedObject() ) ;
      }

   done :
      PD_TRACE_EXITRC ( SDB__MTHMDF__APPPOPMDF, rc );
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__MTHMDF__APPBITMDF, "_mthModifier::_applyBitModifier" )
   template<class Builder>
   INT32 _mthModifier::_applyBitModifier ( const CHAR *pRoot, Builder &bb,
                                           const BSONElement &in,
                                           ModifierElement &me )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB__MTHMDF__APPBITMDF );
      BSONElement elt = me._toModify ;
      BSONType a = in.type() ;
      BSONType b = elt.type() ;
      BOOLEAN change = FALSE ;

      if ( NumberLong == a || NumberInt == a )
      {
         if ( NumberLong == a || NumberLong == b )
         {
            INT64 result = 0 ;
            rc = _bitCalc ( me._modType, in.numberLong(),
                            elt.numberLong(), result ) ;
            if ( SDB_OK == rc )
            {
               change = TRUE ;
               bb.append ( in.fieldName(), ( long long )result ) ;
               ADD_CHG_NUMBER ( _dstChgBuilder, pRoot, (long long)result,
                                "$set" ) ;
            }
         }
         else if ( NumberInt == a || NumberInt == b )
         {
            INT32 result = 0 ;
            rc = _bitCalc ( me._modType, in.numberInt(),
                            elt.numberInt(), result ) ;
            if ( SDB_OK == rc )
            {
               change = TRUE ;
               bb.append ( in.fieldName(), (int)result ) ;
               ADD_CHG_NUMBER ( _dstChgBuilder, pRoot, (int)result, "$set" ) ;
            }
         }
      }

      if ( change )
      {
         ADD_CHG_ELEMENT_AS ( _srcChgBuilder, in, pRoot, "$set" ) ;
      }
      else
      {
         //not change, should add the org element
         bb.append ( in ) ;
      }
      PD_TRACE_EXITRC ( SDB__MTHMDF__APPBITMDF, rc );
      return rc ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__MTHMDF__APPBITMDF2, "_mthModifier::_applyBitModifier2" )
   template<class Builder>
   INT32 _mthModifier::_applyBitModifier2 ( const CHAR *pRoot, Builder &bb,
                                            const BSONElement &in,
                                            ModifierElement &me )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__MTHMDF__APPBITMDF2 ) ;

      //if org is not int or long, not change
      if ( NumberInt != in.type() && NumberLong != in.type() )
      {
         bb.append ( in ) ;
         goto done ;
      }
      {
         ModType type = UNKNOWN ;
         BSONObjIterator it ( me._toModify.embeddedObject () ) ;
         INT32 x = in.numberInt () ;
         INT64 y = in.numberLong () ;

         while ( it.more () )
         {
            BSONElement e = it.next () ;
            if ( NumberInt != e.type () && NumberLong != e.type () )
            {
               PD_LOG_MSG ( PDERROR, "bit object field elements must be int or "
                            "long, %s", e.toString( TRUE ).c_str() ) ;
               rc = SDB_INVALIDARG ;
               goto done ;
            }
            {
               string oprName = "$bit" ;
               oprName += e.fieldName () ;
               type = _parseModType ( oprName.c_str() ) ;
            }
            if ( UNKNOWN == type )
            {
               PD_LOG_MSG ( PDERROR, "unknown bit operator, %s",
                            e.toString( TRUE ).c_str() ) ;
               rc = SDB_INVALIDARG ;
               goto done ;
            }

            rc = _bitCalc ( type, x, e.numberInt(), x ) ;
            if ( SDB_OK != rc )
            {
               goto done ;
            }
            rc = _bitCalc ( type, y, e.numberLong(), y ) ;
            if ( SDB_OK != rc )
            {
               goto done ;
            }
         }

         if ( ( INT64 ) x != y )
         {
            bb.append ( in.fieldName(), y ) ;
            ADD_CHG_NUMBER ( _dstChgBuilder, pRoot, y, "$set" ) ;
         }
         else
         {
            bb.append ( in.fieldName(), x ) ;
            ADD_CHG_NUMBER ( _dstChgBuilder, pRoot, x, "$set" ) ;
         }
         ADD_CHG_ELEMENT_AS ( _srcChgBuilder, in, pRoot, "$set" ) ;
      }
   done :
      PD_TRACE_EXITRC ( SDB__MTHMDF__APPBITMDF2, rc );
      return rc ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__MTHMDF__APPADD2SETMDF, "_mthModifier::_applyAddtoSetModifier" )
   template<class Builder>
   INT32 _mthModifier::_applyAddtoSetModifier ( const CHAR *pRoot, Builder &bb,
                                                const BSONElement &in,
                                                ModifierElement & me )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB__MTHMDF__APPADD2SETMDF ) ;
      // add each element in array into existing array
      // don't want to add duplicates in addtoset
      // make sure original data is array
      if ( Array != in.type() )
      {
         PD_LOG_MSG ( ( _ignoreTypeError ? PDDEBUG : PDERROR ),
                      "Original data type is not array: %s",
                      in.toString().c_str() ) ;
         if ( _ignoreTypeError )
         {
            bb.append( in ) ;
         }
         else
         {
            rc = SDB_INVALIDARG ;
         }
         goto done ;
      }
      // make sure added value is array
      if ( Array != me._toModify.type() )
      {
        PD_LOG_MSG ( PDERROR, "added data type is not array: %s",
                     me._toModify.toString().c_str()) ;
        rc = SDB_INVALIDARG ;
        goto done ;
      }
      {
         BSONObjBuilder sub ( bb.subarrayStart ( in.fieldName() ) ) ;
         BSONObjIterator i ( in.embeddedObject() ) ;
         BSONObjIterator j ( me._toModify.embeddedObject() ) ;
         INT32 n = 0 ;
         // make bsonelementset for everything we want to add
         BSONElementSet eleset ;
         while ( j.more() )
         {
            eleset.insert( j.next() ) ;
         }
         while ( i.more() )
         {
            BSONElement cur = i.next() ;
            sub.append( cur ) ;
            n++ ;
            eleset.erase( cur ) ;
         }
         INT32 orgNum = n ;
         BSONElementSet::iterator it ;
         for ( it = eleset.begin(); it != eleset.end(); it++ )
         {
            sub.appendAs( (*it), sub.numStr(n++) ) ;
         }
         BSONObj newObj = sub.done() ;

         //add new element
         if ( orgNum != n )
         {
            _buildSetArray( _dstChgBuilder, pRoot, orgNum, -1, newObj ) ;
            _buildSetArray( _srcChgBuilder, pRoot, orgNum, -1, in.embeddedObject() ) ;
         }
      }
   done :
      PD_TRACE_EXITRC ( SDB__MTHMDF__APPADD2SETMDF, rc );
      return rc ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__MTHMDF__BITCALC, "_mthModifier::_bitCalc" )
   template<class VType>
   INT32 _mthModifier::_bitCalc ( ModType type, VType l, VType r, VType & out )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB__MTHMDF__BITCALC );
      switch ( type )
      {
         case BITAND :
            out = l & r ;
            break ;
         case BITOR :
            out = l | r ;
            break ;
         case BITNOT :
            out = ~l ;
            break ;
         case BITXOR :
            out = l ^ r ;
            break ;
         default :
            PD_LOG_MSG ( PDERROR, "unknow bit modifier type[%d]", type ) ;
            rc = SDB_INVALIDARG ;
            break ;
      }
      PD_TRACE_EXITRC ( SDB__MTHMDF__BITCALC, rc );
      return rc ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__MTHMDF__APPBITMDF3, "_mthModifier::_appendBitModifier" )
   template<class Builder>
   INT32 _mthModifier::_appendBitModifier ( const CHAR *pRoot,
                                            const CHAR *pShort,
                                            Builder &bb, INT32 in,
                                            ModifierElement &me )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB__MTHMDF__APPBITMDF3 );
      BSONElement elt = me._toModify ;
      BSONType b = elt.type () ;

      if ( NumberLong == b )
      {
         INT64 result = 0 ;
         rc = _bitCalc ( me._modType, (INT64)in, elt.numberLong(), result ) ;
         if ( SDB_OK == rc )
         {
            bb.append ( pShort, result ) ;
            ADD_CHG_NUMBER ( _dstChgBuilder, pRoot, result, "$set" ) ;
         }
      }
      else if ( NumberInt == b )
      {
         INT32 result = 0 ;
         rc = _bitCalc ( me._modType, in, elt.numberInt(), result ) ;
         if ( SDB_OK == rc )
         {
            bb.append ( pShort, result ) ;
            ADD_CHG_NUMBER ( _dstChgBuilder, pRoot, result, "$set" ) ;
         }
      }

      PD_TRACE_EXITRC ( SDB__MTHMDF__APPBITMDF3, rc );
      return rc ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__MTHMDF__APPBITMDF22, "_mthModifier::_appendBitModifier2" )
   template<class Builder>
   INT32 _mthModifier::_appendBitModifier2 ( const CHAR *pRoot,
                                             const CHAR *pShort,
                                             Builder &bb, INT32 in,
                                             ModifierElement &me )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB__MTHMDF__APPBITMDF22 );
      ModType type = UNKNOWN ;
      BSONObjIterator it ( me._toModify.embeddedObject () ) ;
      INT32 x = in ;
      INT64 y = (INT64)in ;

      while ( it.more () )
      {
         BSONElement e = it.next () ;

         if ( NumberInt != e.type() && NumberLong != e.type() )
         {
            PD_LOG_MSG ( PDERROR, "bit object field elements must be int "
                         "or long, %s", e.toString( TRUE ).c_str() ) ;
            rc = SDB_INVALIDARG ;
            goto done ;
         }

         {
            string oprName = "$bit" ;
            oprName += e.fieldName () ;
            type = _parseModType ( oprName.c_str() ) ;
         }
         if ( UNKNOWN == type )
         {
            PD_LOG_MSG ( PDERROR, "unknown bit operator, %s",
                         e.toString( TRUE ).c_str() ) ;
            rc = SDB_INVALIDARG ;
            goto done ;
         }

         rc = _bitCalc ( type, x, e.numberInt(), x ) ;
         if ( SDB_OK != rc )
         {
            goto done ;
         }
         rc = _bitCalc ( type, y, e.numberLong(), y ) ;
         if ( SDB_OK != rc )
         {
            goto done ;
         }
      }

      if ( (INT64)x != y )
      {
         bb.append ( pShort, y ) ;
         ADD_CHG_NUMBER ( _dstChgBuilder, pRoot, y, "$set" ) ;
      }
      else
      {
         bb.append ( pShort, x ) ;
         ADD_CHG_NUMBER ( _dstChgBuilder, pRoot, x, "$set" ) ;
      }

   done :
      PD_TRACE_EXITRC ( SDB__MTHMDF__APPBITMDF22, rc );
      return rc ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__MTHMDF__APPSETARRMDF1, "_mthModifier::_applySetArrayModifier" )
   template<class Builder>
   INT32 _mthModifier::_applySetArrayModifier ( const CHAR *pRoot, Builder &bb,
                                                const BSONElement &in,
                                                ModifierElement &me )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__MTHMDF__APPSETARRMDF1 ) ;

      BSONElement beModify = me._toModify ;
      const CHAR *pShort = in.fieldName() ;
      INT32 beginPos = 0, endPos = 0 ;
      BSONObj arr ;

      if ( Array != in.type() )
      {
         PD_LOG_MSG ( ( _ignoreTypeError ? PDDEBUG : PDERROR ),
                      "Original data type is not array: %s",
                      in.toString().c_str() ) ;
         if ( _ignoreTypeError )
         {
            rc = _appendSetArrayModifier( pRoot, pShort, bb, me ) ;
            if ( SDB_OK == rc )
            {
               // Using $set in the rollback log, $set the original object
               // Replay log is set in _append function
               ADD_CHG_ELEMENT_AS ( _srcChgBuilder, in, pRoot, "$set" ) ;
            }
            else
            {
               PD_LOG_MSG ( PDERROR,
                            "Failed to apply $setarray [%s], rc: %d",
                            beModify.toString( TRUE, TRUE ).c_str(),
                            rc ) ;
            }
         }
         else
         {
            rc = SDB_INVALIDARG ;
         }
         goto done ;
      }

      rc = _parseSetArray( beModify, beginPos, endPos, arr ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG_MSG ( PDERROR,
                      "Failed to parse $setarray [%s], rc: %d",
                      beModify.toString( TRUE, TRUE ).c_str(),
                      rc ) ;
         goto done ;
      }

      {
         // create bson builder for the array
         BSONArrayBuilder newbb( bb.subarrayStart ( pShort ) ) ;

         INT32 idx = 0 ;
         BSONObjIterator iter ( in.embeddedObject()) ;
         while ( iter.more() && idx < beginPos )
         {
            newbb << iter.next() ;
            idx++ ;
         }

         if ( idx < beginPos )
         {
            // The size of orig array is smaller than the begin position,
            // we need to fill null elements between the end of original array
            // to the begin position
            // In this case, we need to log elements from the end of the
            // original array
            INT32 tmpBeginPos = beginPos ;

            beginPos = idx ;
            endPos = -1 ;

            // Fill null elements
            while ( idx < tmpBeginPos )
            {
               newbb.appendNull() ;
               idx ++ ;
            }

            // Append the new elements
            BSONObjIterator iterNew ( arr ) ;
            while ( iterNew.more() )
            {
               newbb << iterNew.next() ;
            }
         }
         else
         {
            INT32 addedCnt = 0, deletedCnt = 0 ;

            // Append the new elements
            BSONObjIterator iterNew ( arr ) ;
            while ( iterNew.more() )
            {
               newbb << iterNew.next() ;
               addedCnt ++ ;
            }

            if ( beginPos <= endPos )
            {
               // Skip elements between the begin position and the end position
               // in the original array
               while ( iter.more() && idx <= endPos )
               {
                  iter.next() ;
                  idx ++ ;
                  deletedCnt ++ ;
               }

               // If the number of added elements equals to the number deleted
               // elements, we only need to log the elements between the begin
               // position and the end position
               // Otherwise, the size of array is changed, we need to log the
               // elements from the begin position to the end
               // e.g. $setarray:{field:{pos:[2,3],objs:[2,3]}}, we only record
               // the objs [2,3]
               // e.g. $setarray:{field:{pos:[2,3],objs:[2,3,4]}}, the size of
               // array is changed, we need to record from position 2 to the end
               if ( addedCnt != deletedCnt || !iter.more() )
               {
                  endPos = -1 ;
               }

               // Append the remain elements in the original array
               while ( iter.more() )
               {
                  newbb << iter.next() ;
                  idx ++ ;
               }
            }
         }

         BSONObj newObj = newbb.done() ;
         _buildSetArray( _dstChgBuilder, pRoot, beginPos, endPos, newObj ) ;
         _buildSetArray( _srcChgBuilder, pRoot, beginPos, endPos, in.embeddedObject() ) ;
      }

   done :
      PD_TRACE_EXITRC( SDB__MTHMDF__APPSETARRMDF1, rc ) ;
      return rc ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__MTHMDF__APPSETARRMDF2, "_mthModifier::_appendSetArrayModifier" )
   template<class Builder>
   INT32 _mthModifier::_appendSetArrayModifier ( const CHAR *pRoot,
                                                 const CHAR *pShort,
                                                 Builder &bb,
                                                 ModifierElement &me )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__MTHMDF__APPSETARRMDF2 ) ;

      BSONElement beModify = me._toModify ;
      INT32 beginPos = 0, endPos = 0 ;
      BSONObj arr ;

      rc = _parseSetArray( beModify, beginPos, endPos, arr ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG_MSG ( PDERROR,
                      "Failed to parse $setarray [%s], rc: %d",
                      beModify.toString( TRUE, TRUE ).c_str(), rc ) ;
         goto done ;
      }

      {
         // Create bson builder for the array
         BSONArrayBuilder tmpbb ( bb.subarrayStart( pShort ) ) ;

         // Fill null for the beginning of the new array
         INT32 idx = 0 ;
         for ( idx = 0 ; idx < beginPos ; idx ++ )
         {
            tmpbb.appendNull() ;
         }

         // Fill the new array with given array
         BSONObjIterator iter( arr ) ;
         while ( iter.more() )
         {
            tmpbb << iter.next() ;
            idx ++ ;
         }
         BSONObj newObj = tmpbb.done() ;

         // Using $set in the replay log
         ADD_CHG_ARRAY_OBJ ( _dstChgBuilder, newObj, pRoot, "$set" ) ;
      }

   done :
      PD_TRACE_EXITRC( SDB__MTHMDF__APPSETARRMDF2, rc ) ;
      return rc ;
   }

   INT32 _mthModifier::_parseSetArray( const BSONElement &toModify,
                                       INT32 &beginPos, INT32 &endPos,
                                       BSONObj &arr )
   {
      INT32 rc = SDB_OK ;
      BSONObj boModify ;
      BSONElement bePos ;

      if ( Object != toModify.type() )
      {
         PD_LOG_MSG( PDERROR,
                     "$setarray input must be an object: %s",
                     toModify.toString( TRUE, TRUE ).c_str() ) ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      boModify = toModify.embeddedObject() ;

      // Parse SET_ARRAY_POS_NAME
      if ( !boModify.hasField( SET_ARRAY_POS_NAME ) )
      {
         PD_LOG_MSG( PDERROR,
                     "$setarray must have the %s field",
                     SET_ARRAY_POS_NAME ) ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }
      bePos = boModify.getField( SET_ARRAY_POS_NAME ) ;
      if ( NumberInt == bePos.type() )
      {
         beginPos = bePos.numberInt() ;
         if ( beginPos < 0 )
         {
            PD_LOG_MSG( PDERROR,
                        "The %s field in $setarray must be a positive integer",
                        SET_ARRAY_POS_NAME ) ;
            rc = SDB_INVALIDARG ;
            goto error ;
         }
         endPos = -1 ;
      }
      else if ( Array == bePos.type() )
      {
         INT32 idx = 0 ;
         BSONObjIterator i( bePos.embeddedObject() ) ;
         while ( i.more() && idx < 2 )
         {
            BSONElement beTmp = i.next() ;
            if ( NumberInt == beTmp.type() )
            {
               if ( 0 == idx )
               {
                  beginPos = beTmp.numberInt() ;
               }
               else if ( 1 == idx )
               {
                  endPos = beTmp.numberInt() ;
               }
            }
            else
            {
               PD_LOG_MSG( PDERROR,
                           "The %s field in $setarray must be an array with 2 integers",
                           SET_ARRAY_POS_NAME ) ;
               rc = SDB_INVALIDARG ;
               goto error ;
            }
            idx ++ ;
         }
         if ( 2 != idx || i.more() )
         {
            PD_LOG_MSG( PDERROR,
                        "The %s field in $setarray must be an array with 2 "
                        "integers", SET_ARRAY_POS_NAME ) ;
            rc = SDB_INVALIDARG ;
            goto error ;
         }
         if ( beginPos < 0 )
         {
            PD_LOG_MSG( PDERROR,
                        "The beginPos [%d] of %s field in $setarray must be a "
                        "positive integer", beginPos, SET_ARRAY_POS_NAME ) ;
            rc = SDB_INVALIDARG ;
            goto error ;
         }
         if ( endPos < 0 )
         {
            PD_LOG_MSG( PDERROR,
                        "The endPos [%d] of %s field in $setarray must be a "
                        "positive integer", endPos, SET_ARRAY_POS_NAME ) ;
            rc = SDB_INVALIDARG ;
            goto error ;
         }
         if ( beginPos > endPos )
         {
            PD_LOG_MSG( PDERROR,
                        "The beginPos [%d] of %s field in $setarray must be <= the "
                        "endPos [%d]", beginPos, SET_ARRAY_POS_NAME, endPos ) ;
            rc = SDB_INVALIDARG ;
            goto error ;
         }
      }
      else
      {
         PD_LOG_MSG( PDERROR,
                     "The %s field in $setarray must be an integer or an array",
                     SET_ARRAY_POS_NAME ) ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      // Parse SET_ARRAY_OBJS_NAME
      if ( !boModify.hasField( SET_ARRAY_OBJS_NAME ) )
      {
         PD_LOG_MSG( PDERROR,
                     "$setarray input %s must have %s field",
                     toModify.toString( TRUE, TRUE ).c_str(),
                     SET_ARRAY_OBJS_NAME ) ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }
      if ( Array != boModify.getField( SET_ARRAY_OBJS_NAME ).type() )
      {
         PD_LOG_MSG( PDERROR,
                     "%s field in $setarray input %s must be an array",
                     SET_ARRAY_OBJS_NAME,
                     toModify.toString( TRUE, TRUE ).c_str() ) ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }
      arr = boModify.getObjectField( SET_ARRAY_OBJS_NAME ) ;

   done :
      return rc ;
   error :
      goto done ;
   }

   template<class Builder>
   void _mthModifier::_buildSetArray ( Builder *builder, const CHAR *pRoot,
                                       INT32 beginPos, INT32 endPos,
                                       const BSONObj &arr )
   {
      _hasModified = TRUE ;

      if ( !builder )
      {
         return ;
      }

      SDB_ASSERT( beginPos >= 0, "beginPos must >= 0" ) ;

      if ( beginPos <= 10 && endPos < 0 )
      {
         // The "pos" field will take a space of the record, so if it is a small
         // array or change is applied to almost the whole array, we use $set
         // which may generate a smaller record
         ADD_CHG_ARRAY_OBJ( builder, arr, pRoot, "$set" ) ;
      }
      else
      {
         BSONObjBuilder subBuilder( builder->subobjStart( "$setarray" ) ) ;
         BSONObjBuilder sub2Builder( subBuilder.subobjStart( pRoot ) ) ;

         if ( beginPos <= endPos )
         {
            BSONObjBuilder posBuilder( sub2Builder.subarrayStart( SET_ARRAY_POS_NAME ) ) ;
            posBuilder.append( posBuilder.numStr( 0 ), beginPos ) ;
            posBuilder.append( posBuilder.numStr( 1 ), endPos ) ;
            posBuilder.done() ;

            BSONArrayBuilder arrBuilder( sub2Builder.subarrayStart( SET_ARRAY_OBJS_NAME ) ) ;
            BSONObjIterator iter( arr ) ;
            INT32 pos = 0 ;
            while ( iter.more() && pos < beginPos )
            {
               iter.next() ;
               pos ++ ;
            }
            while ( iter.more() && pos <= endPos )
            {
               arrBuilder << iter.next() ;
               pos ++ ;
            }
            arrBuilder.done() ;
         }
         else
         {
            sub2Builder.append( SET_ARRAY_POS_NAME, beginPos ) ;

            BSONArrayBuilder arrBuilder( sub2Builder.subarrayStart( SET_ARRAY_OBJS_NAME ) ) ;
            BSONObjIterator iter( arr ) ;
            INT32 pos = 0 ;
            while ( iter.more() && pos < beginPos )
            {
               iter.next() ;
               pos ++ ;
            }
            while ( iter.more() )
            {
               arrBuilder << iter.next() ;
            }
            arrBuilder.done() ;
         }

         sub2Builder.done() ;
         subBuilder.done() ;
      }
   }

   template<class Builder>
   void _mthModifier::_buildSetArray ( Builder *builder, const CHAR *pRoot,
                                       INT32 beginPos, const BSONObj &arr )
   {
      _hasModified = TRUE ;

      if ( !builder )
      {
         return ;
      }

      SDB_ASSERT( beginPos >= 0, "beginPos must >= 0" ) ;

      if ( beginPos == 0 )
      {
         ADD_CHG_ARRAY_OBJ( builder, arr, pRoot, "$set" ) ;
      }
      else
      {
         BSONObjBuilder subBuilder( builder->subobjStart( "$setarray" ) ) ;
         BSONObjBuilder sub2Builder( subBuilder.subobjStart( pRoot ) ) ;
         sub2Builder.append( SET_ARRAY_POS_NAME, beginPos ) ;
         sub2Builder.appendArray( SET_ARRAY_OBJS_NAME, arr ) ;
         sub2Builder.done() ;
         subBuilder.done() ;
      }
   }

   template<class Builder>
   void _mthModifier::_buildSetArray ( Builder *builder, const CHAR *pRoot,
                                       INT32 beginPos, const BSONElement &ele )
   {
      _hasModified = TRUE ;

      if ( !builder )
      {
         return ;
      }

      SDB_ASSERT( beginPos >= 0, "beginPos must >= 0" ) ;

      if ( beginPos == 0 )
      {
         BSONObjBuilder subBuilder( builder->subobjStart ( "$set" ) ) ;
         BSONArrayBuilder arrBuilder( subBuilder.subarrayStart( pRoot ) ) ;
         arrBuilder << ele ;
         arrBuilder.done() ;
         subBuilder.done() ;
      }
      else
      {
         BSONObjBuilder subBuilder( builder->subobjStart( "$setarray" ) ) ;
         BSONObjBuilder sub2Builder( subBuilder.subobjStart( pRoot ) ) ;
         sub2Builder.append( SET_ARRAY_POS_NAME, beginPos ) ;
         BSONArrayBuilder arrBuilder( sub2Builder.subarrayStart( SET_ARRAY_OBJS_NAME ) ) ;
         arrBuilder << ele ;
         arrBuilder.done() ;
         sub2Builder.done() ;
         subBuilder.done() ;
      }
   }

   BOOLEAN _mthModifier::_pullElementMatch( BSONElement& org,
                                            BSONElement& toMatch,
                                            BOOLEAN fullMatch )
   {
      // if the one we are trying to match is not object, then we call woCompare
      if ( toMatch.type() != Object )
      {
         return org.valuesEqual(toMatch) ;
      }
      // if we want to match an object but original data is not object, then
      // it's not possible to have a match
      if ( org.type() != Object )
      {
         return FALSE ;
      }

      if ( !fullMatch )
      {
         BSONObj objOrg = org.embeddedObject() ;
         BSONObjIterator itr( toMatch.embeddedObject() ) ;
         while( itr.more() )
         {
            BSONElement e = itr.next() ;
            BSONElement o = objOrg.getField( e.fieldName() ) ;

            if ( o.eoo() || 0 != e.woCompare( o, false ) )
            {
               return FALSE ;
            }
         }
         return TRUE ;
      }
      // otherwise let's do full compare if both sides are object
      return org.woCompare(toMatch, FALSE) == 0 ;
   }


   template<class Builder>
   void _mthModifier::_applyUnsetModifier(Builder &b)
   {}

   void _mthModifier::_applyUnsetModifier(BSONArrayBuilder &b)
   {
      b.appendNull() ;
   }
   ModType _mthModifier::_parseModType ( const CHAR * field )
   {
      return mthGetModifierOpParse().find( field ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__MTHMDF_PASELE, "_mthModifier::_parseElement" )
   INT32 _mthModifier::_parseElement ( const BSONElement &ele )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB__MTHMDF_PASELE );
      SDB_ASSERT ( ele.type() != Undefined, "Undefined element type" ) ;
      // get field name first
      ModType type = _parseModType( ele.fieldName () ) ;
      if ( UNKNOWN == type )
      {
         PD_LOG_MSG ( PDERROR, "Updator operator[%s] error", ele.fieldName () ) ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      if ( REPLACE == type )
      {
         _isReplace = TRUE ;
         if ( _modifierBits & MTH_MODIFIER_FIELD_OPR_BIT )
         {
            PD_LOG_MSG( PDERROR, "Operator[%s] can't be used with "
                        "others", ele.fieldName() ) ;
            rc = SDB_INVALIDARG ;
            goto error ;
         }
         _modifierBits |= MTH_MODIFIER_RECORD_OPR_BIT ;
      }
      else if ( KEEP != type )
      {
         if ( _modifierBits & MTH_MODIFIER_RECORD_OPR_BIT )
         {
            PD_LOG_MSG( PDERROR, "Operator[%s] can't be used with $replace",
                        ele.fieldName() ) ;
            rc = SDB_INVALIDARG ;
            goto error ;
         }
         _modifierBits |= MTH_MODIFIER_FIELD_OPR_BIT ;
      }

      // then check element type
      switch ( ele.type() )
      {
      case Object:
      {
         // for {$inc, $pull, etc...} cases
         BSONObjIterator j( ele.embeddedObject() ) ;
         // even thou this is a loop, we always exist after parsing the first
         // element
         while ( j.more () )
         {
            rc = _addModifier ( j.next(), type ) ;
            if ( SDB_OK != rc )
            {
               goto error ;
            }
         } // while

         break ;
      }
      default:
      {
         // each element must be an object, the field name is operator and
         // object contains field name and value
         // for example
         // $inc : { votes: 1 }    # for increment votes by 1
         PD_LOG_MSG ( PDERROR, "each element in modifier pattern must "
                      "be object" ) ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }
      }

   done :
      PD_TRACE_EXITRC ( SDB__MTHMDF_PASELE, rc );
      return rc ;
   error :
      goto done ;
   }

   void _mthModifier::modifierSort()
   {
      std::sort( _modifierElements.begin(),
                 _modifierElements.end(),
                 _mthModFieldNamesCompare( _dollarList ) ) ;
   }

   INT32 _mthModifier::_parseFullRecord( const BSONObj &record )
   {
      INT32 rc = SDB_OK ;
      ModType type = REPLACE ;
      _isReplace = TRUE ;
      _isReplaceID = TRUE ;

      BSONObjIterator iter( record ) ;
      while ( iter.more () )
      {
         rc = _addModifier( iter.next(), type ) ;
         if ( SDB_OK != rc )
         {
            goto error ;
         }
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _mthModifier::_setSourceRecord( const BSONObj &record )
   {
      INT32 rc = SDB_OK ;

      try
      {
         _sourceRecord = record ;
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

   // PD_TRACE_DECLARE_FUNCTION ( SDB__MTHMDF_LDPTN, "_mthModifier::loadPattern" )
   INT32 _mthModifier::loadPattern ( const BSONObj &modifierPattern,
                                     vector<INT64> *dollarList,
                                     BOOLEAN ignoreTypeError,
                                     const BSONObj* shardingKey,
                                     BOOLEAN strictDataMode,
                                     UINT32 logWriteMod,
                                     BOOLEAN calcIdxHash )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB__MTHMDF_LDPTN );
      _modifierPattern = modifierPattern.copy() ;
      INT32 eleNum = 0 ;
      _dollarList = dollarList ;
      _ignoreTypeError = ignoreTypeError ;
      _fieldCompare.setDollarList( _dollarList ) ;

      if ( DPS_LOG_WRITE_MOD_FULL == logWriteMod )
      {
         rc = _parseFullRecord( _modifierPattern ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to parse full record[%s], rc = %d",
                      _modifierPattern.toString().c_str(), rc ) ;
      }
      else
      {
         BSONObjIterator i( _modifierPattern ) ;
         while ( i.more() )
         {
            // Each element contains a modification operator and it's operands.
            rc = _parseElement(i.next() ) ;
            if ( rc )
            {
               PD_LOG_MSG ( PDERROR, "Failed to parse match "
                            "pattern[%s, pos: %d], rc: %d",
                            modifierPattern.toString().c_str(), eleNum,
                            rc ) ;
               goto error ;
            }
            eleNum ++ ;
         }

         /// if has $keep, but not $replace, report error
         if ( !_isReplace && _keepKeys.size() > 0 )
         {
            PD_LOG_MSG( PDERROR, "Operator $keep can only be used with "
                        "$replace") ;
            rc = SDB_INVALIDARG ;
            goto error ;
         }
         else if ( _isReplace )
         {
            MODIFIER_VEC::iterator iter ;
            if ( !_isReplaceID )
            {
               /// when not replace _id, keep the _id
               rc = _addToKeepSet( DMS_ID_KEY_NAME ) ;
               PD_RC_CHECK( rc, PDERROR, "Failed to add to keep set:rc=%d",
                            rc ) ;
            }

            iter = _modifierElements.begin() ;
            while ( iter != _modifierElements.end() )
            {
               ModifierElement *me = *iter ;
               BSONElement e = me->_toModify ;
               if ( _keepKeys.count( e.fieldName() ) > 0 )
               {
                  SAFE_OSS_DELETE( me ) ;
                  iter = _modifierElements.erase( iter ) ;
               }
               else
               {
                  iter++ ;
               }
            }
         }
      }

      modifierSort() ;

      if ( NULL != shardingKey && !shardingKey->isEmpty() )
      {
         _shardingKeyGen = SDB_OSS_NEW _ixmIndexKeyGen( *shardingKey ) ;
         if ( NULL == _shardingKeyGen )
         {
            rc = SDB_OOM ;
            PD_LOG_MSG( PDERROR,
                        "Failed to create new sharding key gen, rc=%d", rc ) ;
            goto error ;
         }
      }

      // if index hash bitmap is needed, calculate it with updated elements
      // NOTE: for replace, we don't known origin elements, so can not
      // calculate index hash bitmap
      if ( calcIdxHash &&
           !_isReplace &&
           _modifierElements.size() <= IXM_IDX_HASH_MAX_FIELD_NUM )
      {
         _idxHashBitmap.resetBitmap() ;
         for ( MODIFIER_VEC::iterator iter = _modifierElements.begin() ;
               iter != _modifierElements.end() ;
               ++ iter )
         {
            ModifierElement *mthEle = *iter ;
            _idxHashBitmap.setFieldBit( mthEle->_toModify.fieldName() ) ;
            if ( RENAME == mthEle->_modType )
            {
               // need consider new name for RENAME modify operator
               _idxHashBitmap.setFieldBit( mthEle->_toModify.valuestr() ) ;
            }
         }
      }
      else
      {
         _idxHashBitmap.setAllBits() ;
      }

      _initialized = TRUE ;
      _strictDataMode = strictDataMode ;

   done :
      PD_TRACE_EXITRC ( SDB__MTHMDF_LDPTN, rc );
      return rc ;
   error :
      goto done ;
   }

   BOOLEAN _mthModifier::_dupFieldName ( const BSONElement &l,
                                         const BSONElement &r )
   {
      return !l.eoo() && !r.eoo() && (l.rawdata() != r.rawdata()) &&
        ossStrncmp(l.fieldName(),r.fieldName(),ossStrlen(r.fieldName()))==0 ;
   }

   // when requested update want to change something that not exist in original
   // object, we need to append the original object in those cases
   // PD_TRACE_DECLARE_FUNCTION ( SDB__MTHMDF__APPNEW, "_mthModifier::_appendNew" )
   template<class Builder>
   INT32 _mthModifier::_appendNew ( const CHAR *pRoot, const CHAR *pShort,
                                    Builder& b, SINT32 *modifierIndex )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB__MTHMDF__APPNEW );
      ModifierElement *me = _modifierElements[(*modifierIndex)] ;
      BSONElement realEle = me->_toModify ;
      if ( me->isModifyByField() )
      {
         rc = _getFieldModifier( me->getSourceFieldName(), realEle ) ;
         if ( rc )
         {
            PD_LOG_MSG( PDERROR, "Get value of '$field' failed: %d",
                        rc ) ;
            goto error ;
         }

         if ( realEle.eoo() )
         {
            // If the field specified by '$field' dose not exist in the original
            // record, just skip.
            _incModifierIndex( modifierIndex ) ;
            goto done ;
         }
      }


      switch ( me->_modType )
      {
         case INC:
         {
            try
            {
            mthModifierIncNode *incModifier = ( mthModifierIncNode * ) me ;
            if ( incModifier->_isSimple )
            {
               b.appendAs ( realEle, pShort ) ;
               ADD_CHG_ELEMENT_AS ( _dstChgBuilder, realEle, pRoot, "$set" ) ;
            }
            else
            {
               if ( incModifier->_default.isNull() )
               {
                  //do nothing SEQUOIADBMAINSTREAM-4906
               }
               else
               {
                  if ( incModifier->_default.eoo() || incModifier->_default.isNumber() )
                  {
                     _interBuilder.reset() ;
                     BSONElement defaultResultEle ;
                     if ( incModifier->isModifyByField() )
                     {
                        if ( !realEle.isNumber() )
                        {
                           rc = SDB_INVALIDARG ;
                           PD_LOG_MSG( PDERROR, "Field %s is not a number in "
                                                "record: %s",
                                       me->getSourceFieldName(),
                                       _sourceRecord.toString().c_str() ) ;
                           goto error ;
                        }

                        rc = mthModifierInc( incModifier->_default, realEle,
                                             _strictDataMode, _interBuilder ) ;
                        if ( rc )
                        {
                           PD_LOG_MSG( PDERROR, "Calculate default value "
                                                "failed: %d", rc ) ;
                           goto done ;
                        }

                        if ( _interBuilder.isEmpty() )
                        {
                           // _interBuilder empty means the function
                           // mthModifierInc invoked above has done nothing.
                           // This will happend when the number value of realEle
                           // is 0. In this case, we should use the value of
                           // the 'Default' field.
                           if ( incModifier->_default.eoo() )
                           {
                              _interBuilder.append( "", 0 ) ;
                              defaultResultEle =
                                    _interBuilder.done().firstElement() ;
                           }
                           else
                           {
                              defaultResultEle = incModifier->_default ;
                           }
                        }
                        else
                        {
                           defaultResultEle = _interBuilder.done().firstElement() ;
                        }
                     }
                     else
                     {
                        defaultResultEle = incModifier->_defaultResult.firstElement() ;
                     }

                     rc = incModifier->validate( defaultResultEle ) ;
                     if ( rc )
                     {
                        goto done ;
                     }

                     b.appendAs ( defaultResultEle, pShort ) ;
                     ADD_CHG_ELEMENT_AS ( _dstChgBuilder, defaultResultEle,
                                          pRoot, "$set" ) ;
                  }
                  else
                  {
                     rc = SDB_SYS ;
                     PD_LOG( PDERROR, "Unreconigzed default value[%s]:rc=%d",
                             incModifier->_default.toPoolString().c_str(),
                             rc ) ;
                     SDB_ASSERT( FALSE, "Impossible" ) ;
                     goto done ;
                  }
               }
            }
         }
         catch( std::exception &e )
         {
            PD_LOG_MSG ( ( _ignoreTypeError ? PDINFO : PDERROR ),
                         "Failed to append for %s: %s",
                         realEle.toString().c_str(), e.what() ) ;
            if ( !_ignoreTypeError )
            {
               rc = ossException2RC( &e ) ;
               goto done ;
            }
         }

         break ;
      }
      case SET:
      {
         try
         {
            b.appendAs ( realEle, pShort ) ;
            ADD_CHG_ELEMENT_AS ( _dstChgBuilder, realEle, pRoot, "$set" ) ;
         }
         catch( std::exception &e )
         {
            PD_LOG_MSG ( ( _ignoreTypeError ? PDINFO : PDERROR ),
                         "Failed to append for %s: %s",
                         realEle.toString().c_str(), e.what() ) ;
            if ( !_ignoreTypeError )
            {
               rc = ossException2RC( &e ) ;
               goto done ;
            }
         }
         break ;
      }
      case CURRENT_DATE:
      {
         mthModifierCurDateNode* pCurDateNode = ( mthModifierCurDateNode* ) me ;
         try
         {
            realEle = pCurDateNode->calc() ;
            b.appendAs ( realEle, pShort ) ;

            ADD_CHG_ELEMENT_AS ( _dstChgBuilder, realEle, pRoot, "$set" ) ;
         }
         catch( std::exception &e )
         {
            PD_LOG_MSG ( ( _ignoreTypeError ? PDINFO : PDERROR ),
                         "Failed to append for %s: %s",
                         realEle.toString().c_str(), e.what() ) ;
            if ( !_ignoreTypeError )
            {
               rc = ossException2RC( &e ) ;
               goto done ;
            }
         }
         break ;
      }
      // this codepath should never been hit
      case UNSET:
      case PULL:
      case PULL_BY:
      case PULL_ALL:
      case PULL_ALL_BY:
      case POP:
      case RENAME:
      {
         PD_LOG_MSG ( PDERROR, "Unexpected codepath" ) ;
         rc = SDB_SYS ;
         goto done ;
      }
      // need to do something, but not implemented yet
      case PUSH:
      {
         // create bson builder for the array
         BSONObjBuilder bb ( b.subarrayStart( pShort ) ) ;
         bb.appendAs ( realEle, bb.numStr(0) ) ;
         BSONObj newObj = bb.done() ;

         ADD_CHG_ARRAY_OBJ ( _dstChgBuilder, newObj, pRoot, "$set" ) ;
         break ;
      }
      case PUSH_ALL:
      {
         // make sure the new type is array too
         if ( me->_toModify.type() != Array )
         {
            PD_LOG_MSG ( PDERROR, "pushed data type is not array: %s",
                         me->_toModify.toString().c_str());
            rc = SDB_INVALIDARG ;
            goto done ;
         }

         b.appendAs ( me->_toModify, pShort ) ;
         ADD_CHG_ELEMENT_AS ( _dstChgBuilder, me->_toModify, pRoot, "$set" ) ;
         break ;
      }
      case ADDTOSET:
      {
         // make sure added value is array
         if ( Array != me->_toModify.type() )
         {
           PD_LOG_MSG ( PDERROR, "added data type is not array: %s",
                        me->_toModify.toString().c_str()) ;
           rc = SDB_INVALIDARG ;
           goto done ;
         }
         BSONObjBuilder bb (b.subarrayStart( pShort ) ) ;
         BSONObjIterator j ( me->_toModify.embeddedObject() ) ;
         INT32 n = 0 ;
         // make bsonelementset for everything we want to add
         BSONElementSet eleset ;
         // insert into set to deduplicate
         while ( j.more() )
         {
            eleset.insert( j.next() ) ;
         }
         BSONElementSet::iterator it ;
         for ( it = eleset.begin(); it != eleset.end(); it++ )
         {
            bb.appendAs((*it), bb.numStr(n++)) ;
         }
         BSONObj newObj = bb.done() ;

         //add new element
         if ( n != 0 )
         {
            ADD_CHG_ARRAY_OBJ ( _dstChgBuilder, newObj, pRoot, "$set" ) ;
         }
         break ;
      }
      case BITXOR:
      case BITNOT:
      case BITAND:
      case BITOR:
         rc = _appendBitModifier ( pRoot, pShort, b, 0, *me ) ;
         break ;
      case BIT:
         rc = _appendBitModifier2 ( pRoot, pShort, b, 0, *me ) ;
         break ;
      case SETARRAY :
         rc = _appendSetArrayModifier( pRoot, pShort, b, *me ) ;
         break ;
      default:
         PD_LOG_MSG ( PDERROR, "unknow modifier type[%d]", me->_modType ) ;
         rc = SDB_INVALIDARG ;
         goto done ;
      }

      // here we actually consume modifier, then we add index
      if ( SDB_OK == rc )
      {
         _incModifierIndex( modifierIndex ) ;
      }

   done :
      if ( SDB_OK != rc )
      {
         _saveErrorElement( pShort ) ;
      }
      PD_TRACE_EXITRC ( SDB__MTHMDF__APPNEW, rc );
      return rc ;
   error:
      goto done ;
   }

   // Builder could be BSONObjBuilder or BSONArrayBuilder
   // _appendNewFromMods appends the current builder with the new field
   // root represent the current fieldName, me is the current modifier element
   // b is the builder, onedownseen represent the all subobjects have been
   // processed in the current object, and modifierIndex is the pointer for
   // current modifier
   // PD_TRACE_DECLARE_FUNCTION ( SDB__MTHMDF__APPNEWFRMMODS, "_mthModifier::_appendNewFromMods" )
   template<class Builder>
   INT32 _mthModifier::_appendNewFromMods ( CHAR **ppRoot,
                                            INT32 &rootBufLen,
                                            INT32 rootLen,
                                            UINT32 modifierRootLen,
                                            Builder &b,
                                            SINT32 *modifierIndex,
                                            BOOLEAN hasCreateNewRoot )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB__MTHMDF__APPNEWFRMMODS ) ;
      ModifierElement *me = _modifierElements[(*modifierIndex)] ;
      BOOLEAN hasUnknowDollar = FALSE ;
      const CHAR *pDollar = NULL ;
      INT32 newRootLen = rootLen ;
      BOOLEAN isRoot = FALSE ;
      INT32 savedLen = 0 ;

      MTH_SUBFIELD_STR newRootField ;

      // if the modified request does not exist in original one
      // first let's see if there's nested object in the request
      // ex. current root is user.name
      // however request is user.name.first.origin
      // in this case we'll have to create sub object 'first'

      // note fieldName is the FULL path "user.name.first.origin"
      // root is user.name.
      const CHAR *fieldName = me->_toModify.fieldName() ;
      // now temp is "first.origin"
      const CHAR *temp = fieldName + modifierRootLen ;
      // find the "." starting from root length
      const CHAR *dot = ossStrchr ( temp, '.' ) ;

      if ( UNSET == me->_modType ||
           PULL == me->_modType ||
           PULL_BY == me->_modType ||
           PULL_ALL == me->_modType ||
           PULL_ALL_BY == me->_modType ||
           POP == me->_modType ||
           RENAME == me->_modType ||
           NULLOPR == me->_modType )
      {
         // we don't continue for those types since they are not going to append
         // new records
         _incModifierIndex( modifierIndex ) ;
         goto done ;
      }

      if ( dot )
      {
         *(CHAR*)dot = 0 ;
      }
      pDollar = _fieldCompare.getDollarValue( temp, &hasUnknowDollar ) ;
      temp = *ppRoot + newRootLen ;
      rc = mthAppendString( ppRoot, rootBufLen, newRootLen, pDollar, -1,
                            &newRootLen ) ;
      // Restore
      if ( dot )
      {
         *(CHAR*)dot = '.' ;
      }

      if ( rc )
      {
         PD_LOG_MSG( PDERROR, "Failed to append string, rc: %d", rc ) ;
         goto error ;
      }
      else if ( hasUnknowDollar )
      {
         _incModifierIndex( modifierIndex ) ;
         goto done ;
      }

      if ( !hasCreateNewRoot )
      {
         isRoot = TRUE ;
         hasCreateNewRoot = TRUE ;
         if ( NULL != _dstChgBuilder )
         {
            savedLen = _dstChgBuilder->len() ;
            newRootField.append( *ppRoot, newRootLen ) ;
         }
      }

      // given example
      // user.name.first.origin
      // |         ^    #
      // | represent fieldName
      // ^ represent temp
      // # represent dot
      // if there is sub object
      if ( dot )
      {
         // create object builder for nf ("first" field)
         BSONObjBuilder bb ( b.subobjStart( temp ) ) ;
         // create a es for empty object
         const BSONObj obj ;
         BSONObjIteratorSorted es( obj ) ;
         // append '.'
         rc = mthAppendString ( ppRoot, rootBufLen, newRootLen, ".", 1,
                                &newRootLen ) ;
         if ( rc )
         {
            PD_LOG_MSG ( PDERROR, "Failed to append string, rc: %d", rc ) ;
            goto error ;
         }

         // create an object for path "user.name.first."
         // bb is the new builder, es is iterator
         // modifierIndex is the index
         rc = _buildNewObj ( ppRoot, rootBufLen, newRootLen,
                             bb, es, modifierIndex, hasCreateNewRoot ) ;
         if ( rc )
         {
            PD_LOG_MSG ( PDERROR, "Failed to build new object for %s, rc: %d",
                         me->_toModify.toString().c_str(), rc ) ;
            goto error ;
         }
         bb.done() ;
      }
      // if we can't find ".", then we are not embedded BSON, let's just
      // create whatever object we asked
      // for example current root is "user.name."
      // and we want {$set: {user.name.firstname, "tao wang"}}
      // here temp will be firstname, and dot will be NULL
      else
      {
         // call _appendNew to append modified element into the current builder
         try
         {
            rc = _appendNew ( *ppRoot, temp, b, modifierIndex ) ;
         }
         catch( std::exception &e )
         {
            PD_LOG_MSG ( PDERROR, "Failed to append for %s: %s",
                         me->_toModify.toString().c_str(), e.what() );
            rc = SDB_INVALIDARG ;
            goto error ;
         }

         if ( rc )
         {
            PD_LOG_MSG ( PDERROR, "Failed to append for %s, rc: %d",
                         me->_toModify.toString().c_str(), rc ) ;
            goto error ;
         }
      }

   done :
      if ( SDB_OK == rc )
      {
         if ( isRoot && NULL != _dstChgBuilder
              && _dstChgBuilder->len() != savedLen )
         {
            ADD_CHG_UNSET_FIELD ( _srcChgBuilder, newRootField.str() ) ;
         }
      }
      PD_TRACE_EXITRC ( SDB__MTHMDF__APPNEWFRMMODS, rc );
      return rc ;
   error :
      goto done ;
   }
   // if the original object has the element we asked to modify, then e is the
   // original element, b is the builder, me is the info that we want to modify
   // PD_TRACE_DECLARE_FUNCTION ( SDB__MTHMDF__ALYCHG, "_mthModifier::_applyChange" )
   template<class Builder>
   INT32 _mthModifier::_applyChange ( CHAR **ppRoot,
                                      INT32 &rootBufLen,
                                      INT32 rootLen,
                                      BSONElement &e,
                                      Builder &b,
                                      SINT32 *modifierIndex,
                                      BSONObj currentObj )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB__MTHMDF__ALYCHG ) ;
      ModifierElement *me = _modifierElements[(*modifierIndex)] ;

      // basically we need to take the original data from e, and use modifier
      // element me to make some change, and add into builder b
      switch ( me->_modType )
      {
      case INC:
      {
         mthModifierIncNode *incMe = ( mthModifierIncNode* ) me ;
         rc = _applyIncModifier ( *ppRoot, b, e, *incMe ) ;
         break ;
      }
      case SET:
         rc = _applySetModifier ( *ppRoot, b, e, *me ) ;
         break ;
      case UNSET:
      {
         ADD_CHG_ELEMENT_AS ( _srcChgBuilder, e, *ppRoot, "$set" ) ;
         ADD_CHG_ELEMENT_AS ( _dstChgBuilder, me->_toModify, *ppRoot,
                              "$unset" ) ;
         _applyUnsetModifier( b ) ;
         break ;
      }
      case CURRENT_DATE:
         rc = _applyCurDateModifier( *ppRoot, b, e, *me  ) ;
         break ;
      case PUSH:
         rc = _applyPushModifier ( *ppRoot, b, e, *me ) ;
         break ;
      case PUSH_ALL:
         rc = _applyPushAllModifier ( *ppRoot, b, e, *me ) ;
         break ;
      // given an input, remove all matching items when they match any of the
      // input
      case PULL:
      case PULL_BY:
      // given an input, remove all matching items when they match the whole
      // input
      case PULL_ALL:
      case PULL_ALL_BY:
         rc = _applyPullModifier ( *ppRoot, b, e, *me ) ;
         break ;
      case POP:
         rc = _applyPopModifier ( *ppRoot, b, e, *me ) ;
         break ;
      case BITNOT:
      case BITXOR:
      case BITAND:
      case BITOR:
         rc = _applyBitModifier ( *ppRoot, b, e, *me ) ;
         break ;
      case BIT:
         rc = _applyBitModifier2 ( *ppRoot, b, e, *me ) ;
         break ;
      case ADDTOSET:
         rc = _applyAddtoSetModifier ( *ppRoot, b, e, *me ) ;
         break ;
      case RENAME:
      {
         const CHAR *pDotR = ossStrrchr( *ppRoot, '.' ) ;
         UINT32 pos = pDotR ? ( pDotR - *ppRoot + 1 ) : 0 ;
         _utilString<> newNameStr ;

         newNameStr.append( *ppRoot, pos ) ;
         newNameStr.append( me->_toModify.valuestr(),
                            ossStrlen( me->_toModify.valuestr() ) ) ;

         if ( currentObj.hasField( me->_toModify.valuestr() ) )
         {
            rc = SDB_INVALIDARG ;
            PD_LOG_MSG( PDERROR, "Cannot rename field name [%s] to [%s] as it already exists, "
                        "rc: %d", *ppRoot, me->_toModify.valuestr(), rc ) ;
            goto done ;
         }

         ADD_CHG_ELEMENT_AS ( _srcChgBuilder, e, *ppRoot, "$set" ) ;
         ADD_CHG_UNSET_FIELD ( _srcChgBuilder, newNameStr.str() ) ;

         //for the new obj,should unset the old, and set the new
         ADD_CHG_UNSET_FIELD ( _dstChgBuilder, *ppRoot ) ;
         ADD_CHG_ELEMENT_AS ( _dstChgBuilder, e, newNameStr.str(), "$set" ) ;

         b.appendAs ( e, me->_toModify.valuestr() ) ;
         break ;
      }
      case NULLOPR:
         break ;
      case SETARRAY :
         rc = _applySetArrayModifier( *ppRoot, b, e, *me ) ;
         break ;
      default :
         PD_LOG_MSG ( PDERROR, "unknown modifier type[%d]", me->_modType ) ;
         rc = SDB_INVALIDARG ;
         goto done ;
      }
      if ( SDB_OK == rc )
      {
         _incModifierIndex( modifierIndex ) ;
      }

   done :
      if ( SDB_OK != rc )
      {
         _saveErrorElement( e ) ;
      }
      PD_TRACE_EXITRC ( SDB__MTHMDF__ALYCHG, rc );
      return rc ;
   }

   template<class Builder>
   INT32 _mthModifier::_buildNewObjReplace( Builder &b,
                                            BSONObjIteratorSorted &es )
   {
      {
         BSONObjBuilder undoRBuilder ;
         while ( es.more() )
         {
            BSONElement e = es.next() ;
            if ( _keepKeys.count( e.fieldName() ) )
            {
               b.append( e ) ;
            }

            undoRBuilder.append( e ) ;
         }

         ADD_CHG_OBJECT( _srcChgBuilder, undoRBuilder.obj(), "$replace" ) ;
      }

      {
         BSONObjBuilder redoRBuilder ;
         UINT32 i = 0 ;
         while ( i < _modifierElements.size() )
         {
            redoRBuilder.append( _modifierElements[i]->_toModify ) ;
            b.append( _modifierElements[i]->_toModify ) ;
            ++i ;
         }

         ADD_CHG_OBJECT( _dstChgBuilder, redoRBuilder.obj(), "$replace" ) ;
      }

      {
         if ( _keepKeys.size() > 0 )
         {
            BSONObjBuilder redoKBuilder ;
            ossPoolSet<string>::iterator it = _keepKeys.begin() ;
            while ( it != _keepKeys.end() )
            {
               // make sure $keep is after $replace
               redoKBuilder.append( *it, 1 ) ;
               ++it ;
            }

            ADD_CHG_OBJECT( _dstChgBuilder, redoKBuilder.obj(), "$keep" ) ;
         }
      }

      return SDB_OK  ;
   }

   // Builder could be BSONObjBuilder or BSONArrayBuilder
   // This function is recursively called to build new object
   // The prerequisit is that _modifierElement is sorted, which supposed to
   // happen at end of loadPattern
   // PD_TRACE_DECLARE_FUNCTION ( SDB__MTHMDF__BLDNEWOBJ, "_mthModifier::_buildNewObj" )
   template<class Builder>
   INT32 _mthModifier::_buildNewObj ( CHAR **ppRoot,
                                      INT32 &rootBufLen,
                                      INT32 rootLen,
                                      Builder &b,
                                      BSONObjIteratorSorted &es,
                                      SINT32 *modifierIndex,
                                      BOOLEAN hasCreateNewRoot,
                                      BSONObj currentObj )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB__MTHMDF__BLDNEWOBJ ) ;

      // get the next element in the object
      BSONElement e ;
      // previous element is set to empty
      BSONElement prevE ;
      UINT32 compareLeftPos = 0 ;
      INT32 newRootLen = rootLen ;

      if ( _isReplace )
      {
         rc = _buildNewObjReplace( b, es ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to build new object:rc=%d", rc ) ;
         goto done ;
      }

      e = es.next() ;

      // loop until we hit end of original object, or end of modifier list
      while( !e.eoo() && (*modifierIndex)<(SINT32)_modifierElements.size() )
      {
         // if we get two elements with same field name, we don't need to
         // continue checking, simply append it to the builder
         if ( _dupFieldName(prevE, e))
         {
            b.append( e ) ;
            prevE = e ;
            e = es.next() ;
            continue ;
         }
         prevE = e ;

         // every time we build the current field, let's set root to original
         (*ppRoot)[rootLen] = '\0' ;
         newRootLen = rootLen ;

         // construct the full path of the current field name
         // say current root is "user.employee.", and this object contains
         // "name, age" fields, then first loop we get user.employee.name
         // second round get user.employee.age
         rc = mthAppendString ( ppRoot, rootBufLen, newRootLen,
                                e.fieldName(), -1, &newRootLen ) ;
         PD_RC_CHECK ( rc, PDERROR, "Failed to append string, rc: %d", rc ) ;

         // compare the full field name with requested update field
         /*FieldCompareResult cmp = compareDottedFieldNames (
               _modifierElements[(*modifierIndex)]->_toModify.fieldName(),
               *ppRoot ) ;*/
         FieldCompareResult cmp = _fieldCompare.compField (
               _modifierElements[(*modifierIndex)]->_toModify.fieldName(),
               *ppRoot, &compareLeftPos, NULL ) ;

         // compare the full path
         // we have few situations need to handle
         // 1) current field is a parent of requested field
         // for example, currentfield = user, requested field = user.name.test
         // this situation called LEFT_SUBFIELD

         // 2) current field is same as requested field
         // for example both current field and requests are user.name.test
         // this situation called SAME

         // 3) current field is not same as requested field, and alphabatically
         // current field is greater than requested field
         // for example current field is user.myname, requested field is
         // user.abc
         // this situation called LEFT_BEFORE

         // 4) current field is not same as requested field, and alphabatically
         // current field is smaller than requested field
         // for example current field is user.myname, requested field is
         // user.name
         // this situation called RIGHT_BEFORE

         // 5) requested field is a parent of current field
         // for example current field is user.name.test, requested field is user
         // howwever since we are doing merge, this situation should NEVER
         // HAPPEN!!
         switch ( cmp )
         {
         case LEFT_SUBFIELD:
            // ex, modify request $set:{user.name,"taoewang"}
            // field: user
            // make sure the BSONElement is object or array
            // if the requested field already exist but it's not object nor
            // array, we should report error since we can't create sub field in
            // other type of element
            if ( e.type() != Object && e.type() != Array )
            {
               PD_LOG_MSG ( ( _ignoreTypeError ? PDDEBUG : PDERROR ),
                            "Invalid field type: %s", e.toString().c_str() ) ;
               if ( _ignoreTypeError )
               {
                  _incModifierIndex( modifierIndex ) ;
                  continue ;
               }
               else
               {
                  rc = SDB_INVALIDARG ;
                  goto error ;
               }
            }

            // add "." at end
            rc = mthAppendString ( ppRoot, rootBufLen, newRootLen, ".", 1,
                                   &newRootLen ) ;
            PD_RC_CHECK ( rc, PDERROR, "Failed to append string, rc: %d", rc ) ;

            // if we are dealing with object, then let's create a new object
            // builder starting from our current fieldName
            if ( e.type() == Object )
            {
               BSONObjBuilder bb(b.subobjStart(e.fieldName()));
               // get the object for the current element, and create sorted
               // iterator on it
               BSONObjIteratorSorted bis(e.Obj());

               // add fieldname into path and recursively call _buildNewObj
               // to create embedded object
               // root is original root + current field + .
               // bb is new object builder
               // bis is the sorted iterator
               // modifierIndex is the current modifier we are working on
               rc = _buildNewObj ( ppRoot, rootBufLen, newRootLen,
                                   bb, bis, modifierIndex,
                                   hasCreateNewRoot, e.Obj() ) ;
               if ( rc )
               {
                  PD_LOG_MSG ( PDERROR, "Failed to build object: %s, rc: %d",
                               e.toString().c_str(), rc ) ;
                  goto error ;
               }
               // call bb.done() to close the builder
               bb.done() ;
            }
            else
            {
               // if it's not object, then we must have array
               // now let's create BSONArrayBuilder
               BSONArrayBuilder ba( b.subarrayStart( e.fieldName() ) ) ;
               //BSONArrayIteratorSorted bis(BSONArray(e.embeddedObject()));
               BSONObjIteratorSorted bis(e.embeddedObject());
               // add fieldname into path and recursively call _buildNewObj
               // to create embedded object
               // root is original root + current field + .
               // ba is new array builder
               // bis is the sorted iterator
               // modifierIndex is the current modifier we are working on
               rc = _buildNewObj ( ppRoot, rootBufLen, newRootLen,
                                   ba, bis, modifierIndex,
                                   hasCreateNewRoot ) ;
               if ( rc )
               {
                  PD_LOG_MSG ( PDERROR, "Failed to build array: %s, rc: %d",
                               e.toString().c_str(), rc ) ;
                  goto error ;
               }
               ba.done() ;
            }
            // process to the next element
            e = es.next() ;
            // note we shouldn't touch modifierIndex here, we should only
            // change it at the place actually consuming it
            break ;

         case LEFT_BEFORE:
            // if the modified request does not exist in original one
            // first let's see if there's nested object in the request
            // ex. current root is user. and our first element is "name"
            // however request is user.address
            // in this case we'll have to create sub object 'address' first

            // _appendNewFromMods appends the current builder with the new field
            // _modifierElement[modifierIndex] represents the current
            // ModifyElement, b is the builder, root is the string of current
            // root field, onedownseen is the set for all subobjects

            // first let's revert root to original
            (*ppRoot)[rootLen] = '\0' ;
            newRootLen = rootLen ;
            rc = _appendNewFromMods ( ppRoot, rootBufLen, newRootLen,
                                      compareLeftPos, b,
                                      modifierIndex, hasCreateNewRoot ) ;
            PD_RC_CHECK ( rc, PDERROR, "Failed to append for %s, rc: %d",
                          _modifierElements[(*modifierIndex)
                          ]->_toModify.toString().c_str(), rc ) ;
            // note we don't change e here because we just add the field
            // requested by modifier into new object, the original e shoudln't
            // be changed.

            // we also don't change modifierIndex here since it should be
            // changed by the actual consumer function, not in this loop
            break ;

         case SAME:
            // in this situation, the requested field is the one we are
            // processing, so that we don't need to change object metadata,
            // let's just apply the change
            // e is the current element, b is the current builder, modifierIndex
            // is the current modifier
            try
            {
               rc = _applyChange ( ppRoot, rootBufLen, newRootLen, e, b,
                                   modifierIndex, currentObj ) ;
            }
            catch( std::exception &e )
            {
               PD_LOG_MSG ( PDERROR, "Failed to apply changes for %s: %s",
                            _modifierElements[(*modifierIndex)
                            ]->_toModify.toString().c_str(),
                            e.what() ) ;
               rc = SDB_INVALIDARG ;
               goto error ;
            }
            if ( rc )
            {
               PD_LOG_MSG ( PDERROR, "Failed to apply change for %s, rc: %d",
                            _modifierElements[(*modifierIndex)
                            ]->_toModify.toString().c_str(), rc ) ;
               goto error ;
            }
            // since we have processed the original data, we increase element
            e=es.next();
            // again, don't change modifierIndex in loop
            break ;

         case RIGHT_BEFORE:
            // in this situation, the original field is alphabetically ahead of
            // requested field.
            // for example current field is user.name but requested field is
            // user.plan, then we simply add the field into new object
            // original object doesn't need to change

            // In the situation we are processing different object, for example
            // requested update field is user.newfield.test
            // current processing e is mydata.test
            // in this case, we still keep appending mydata.test until hitting
            // end of the object and return, without touching user.newfield.test
            // so we should be safe here
            b.append(e) ;
            // and increase element
            e=es.next() ;
            break ;

         case RIGHT_SUBFIELD:
         default :
            //we should never reach this codepath
            PD_LOG_MSG ( PDERROR, "Reaching unexpected codepath, cmp( %s, %s, "
                         "res: %d )", _modifierElements[(*modifierIndex)
                         ]->_toModify.toString().c_str(),
                         *ppRoot, cmp ) ;
            rc = SDB_SYS ;
            goto error ;
         }
      }
      // we break out the loop either hitting end of original object, or end of
      // the modifier list

      // if there's still any leftover in original object, let's append them
      while ( !e.eoo() )
      {
         b.append(e) ;
         e=es.next() ;
      }

      while ( (*modifierIndex) < (SINT32)_modifierElements.size() )
      {
         (*ppRoot)[rootLen] = '\0' ;
         newRootLen = rootLen ;

         // compare the full field name with requested update field
         /*FieldCompareResult cmp = compareDottedFieldNames (
               _modifierElements[(*modifierIndex)]->_toModify.fieldName(),
               *ppRoot ) ;*/
         FieldCompareResult cmp = _fieldCompare.compField (
               _modifierElements[(*modifierIndex)]->_toModify.fieldName(),
               *ppRoot, &compareLeftPos, NULL ) ;
         if ( LEFT_SUBFIELD == cmp )
         {
            rc = _appendNewFromMods ( ppRoot, rootBufLen, newRootLen,
                                      compareLeftPos, b,
                                      modifierIndex, hasCreateNewRoot ) ;
            if ( rc )
            {
               PD_LOG_MSG ( PDERROR, "Failed to append for %s, rc: %d",
                            _modifierElements[(*modifierIndex)
                            ]->_toModify.toString().c_str(), rc );
               goto error ;
            }
         }
         else
         {
            goto done ;
         }
      }

   done :
      PD_TRACE_EXITRC ( SDB__MTHMDF__BLDNEWOBJ, rc );
      return rc ;
   error :
      goto done ;
   }

   void _mthModifier::_resetErrorElement()
   {
      _tmpErrorObj = BSONObj() ;
      _errorFieldElement = BSONElement() ;
   }

   void _mthModifier::_saveErrorElement( BSONElement &errorEle )
   {
      _errorFieldElement = errorEle ;
   }

   void _mthModifier::_saveErrorElement( const CHAR *fieldName )
   {
      if ( NULL != fieldName )
      {
         try
         {
            BSONObjBuilder builder( 20 ) ;
            builder.appendUndefined( fieldName ) ;
            _tmpErrorObj = builder.obj() ;
            _errorFieldElement = _tmpErrorObj.firstElement() ;
         }
         catch( std::exception &e )
         {
            PD_LOG( PDERROR, "Save error element occur exception: %s",
                    e.what() ) ;
         }
      }
   }

   INT32 _mthModifier::_getFieldModifier( const CHAR* fieldName,
                                          BSONElement& fieldEle )
   {
      INT32 rc = SDB_OK ;

      try
      {
         fieldEle = _sourceRecord.getFieldDotted( fieldName ) ;
      }
      catch ( std::exception& e )
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

   BSONElement _mthModifier::getErrorElement()
   {
      return _errorFieldElement ;
   }

   // given a source BSON object and empty target, the returned target will
   // contains modified data

   // since we are dealing with tons of BSON object conversion, this part should
   // ALWAYS protected by try{} catch{}
   // PD_TRACE_DECLARE_FUNCTION ( SDB__MTHMDF_MODIFY, "_mthModifier::modify" )
   INT32 _mthModifier::modify ( const BSONObj &source, BSONObj &target,
                                BSONObj *srcID, BSONObj *srcChange,
                                BSONObj *dstID, BSONObj *dstChange,
                                BSONObj *srcShardingKey,
                                BSONObj *dstShardingKey )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB__MTHMDF_MODIFY );
      SDB_ASSERT(_initialized, "The modifier has not been initialized, please "
                 "call 'loadPattern' before using it" ) ;
      SDB_ASSERT(target.isEmpty(), "target should be empty") ;

      CHAR *pBuffer = NULL ;
      INT32 bufferSize = 0 ;

      _hasModified = FALSE ;
      _resetErrorElement() ;

      if ( _dollarList && _dollarList->size() > 0 )
      {
         modifierSort() ;
      }

      // create a builder with 10% extra space for buffer
      BSONObjBuilder builder ( (int)(source.objsize()*1.1));
      // create sorted iterator
      BSONObjIteratorSorted es(source) ;

      // index for modifier, should be less than _modifierElements.size()
      // say if we have
      // {$inc: {employee.salary, 100}, $set: {employee.status, "promoted"}},
      // then we have 2 modifier ($inc and $set), so modifierIndex start from 0
      // and should end at 1

      SINT32 modifierIndex = -1 ;
      _incModifierIndex( &modifierIndex ) ;

      pBuffer = (CHAR*)SDB_THREAD_ALLOC ( SDB_PAGE_SIZE ) ;
      if ( !pBuffer )
      {
         PD_LOG_MSG ( PDERROR, "Failed to allocate buffer for select" ) ;
         rc = SDB_OOM ;
         goto error ;
      }
      bufferSize = SDB_PAGE_SIZE ;
      pBuffer[0] = '\0' ;

      if ( srcChange )
      {
         _srcChgBuilder = SDB_OSS_NEW BSONObjBuilder ( (int)(source.objsize()*0.3) ) ;
         if ( !_srcChgBuilder )
         {
            rc = SDB_OOM ;
            PD_LOG_MSG ( PDERROR, "Failed to alloc memory for src BSONObjBuilder" ) ;
            goto error ;
         }
      }
      if ( dstChange )
      {
         _dstChgBuilder = SDB_OSS_NEW BSONObjBuilder ( (int)(source.objsize()*0.3) ) ;
         if ( !_dstChgBuilder )
         {
            rc = SDB_OOM ;
            PD_LOG_MSG ( PDERROR, "Failed to alloc memory for dst BSONObjBuilder" ) ;
            goto error ;
         }
      }

      rc = _setSourceRecord( source ) ;
      PD_RC_CHECK( rc, PDERROR, "Set source record when modifing failed: %d",
                   rc ) ;

      // create a new object based on the source
      // "" is empty root, builder is BSONObjBuilder
      // es is our iterator, and modifierIndex is the current modifier we are
      // going to apply
      // when this call returns SDB_OK, we should call builder.obj() to create
      // BSONObject from the builder.
      rc = _buildNewObj ( &pBuffer, bufferSize, 0, builder, es,
                          &modifierIndex, FALSE, _sourceRecord ) ;
      if ( rc )
      {
         PD_LOG_MSG ( PDERROR, "Failed to modify target, rc: %d", rc ) ;
         goto error ;
      }
      // now target owns the builder buffer, since obj() will decouple() the
      // buffer from builder, and assign holder to the new BSONObj
      target=builder.obj();

      if ( srcID )
      {
         BSONObjBuilder srcIDBuilder ;
         BSONElement id = source.getField ( DMS_ID_KEY_NAME ) ;
         if ( id.eoo() )
         {
            PD_LOG_MSG ( PDERROR, "Failed to get source oid, %s",
                         source.toString().c_str() ) ;
            rc = SDB_SYS ;
            goto error ;
         }
         srcIDBuilder.append ( id ) ;
         *srcID = srcIDBuilder.obj() ;
      }
      if ( srcChange )
      {
         *srcChange = _srcChgBuilder->obj () ;
      }
      if ( dstID )
      {
         BSONObjBuilder dstIDBuilder ;
         BSONElement id = target.getField ( DMS_ID_KEY_NAME ) ;
         if ( id.eoo() )
         {
            PD_LOG_MSG ( PDERROR, "Failed to get target oid, %s",
                         target.toString().c_str() ) ;
            rc = SDB_SYS ;
            goto error ;
         }
         dstIDBuilder.append ( id ) ;
         *dstID = dstIDBuilder.obj() ;
      }
      if ( dstChange )
      {
         *dstChange = _dstChgBuilder->obj () ;
      }

      if ( NULL != _shardingKeyGen )
      {
         if ( NULL != srcShardingKey )
         {
            BSONObjSet keySet ;
            rc = _shardingKeyGen->getKeys( source, keySet, NULL, TRUE, TRUE ) ;
            if ( SDB_OK != rc )
            {
               PD_LOG ( PDERROR, "Failed to get sharding key from obj: %s, rc=%d",
                        source.toString().c_str(), rc ) ;
               goto error ;
            }

            if ( keySet.size() == 1 )
            {
               *srcShardingKey = *keySet.begin() ;
            }
         }

         if ( NULL != dstShardingKey )
         {
            BSONObjSet keySet ;
            rc = _shardingKeyGen->getKeys( target, keySet, NULL, TRUE, TRUE ) ;
            if ( SDB_OK != rc )
            {
               PD_LOG ( PDERROR, "Failed to get sharding key from obj: %s, rc=%d",
                        target.toString().c_str(), rc ) ;
               goto error ;
            }

            if ( keySet.size() == 1 )
            {
               *dstShardingKey = *keySet.begin() ;
            }
         }
      }

   done :
      if ( pBuffer )
      {
         SDB_THREAD_FREE ( pBuffer ) ;
      }
      if ( _srcChgBuilder )
      {
         SDB_OSS_DEL _srcChgBuilder ;
         _srcChgBuilder = NULL ;
      }
      if ( _dstChgBuilder )
      {
         SDB_OSS_DEL _dstChgBuilder ;
         _dstChgBuilder = NULL ;
      }
      PD_TRACE_EXITRC ( SDB__MTHMDF_MODIFY, rc );
      return rc ;
   error :
      _pmdEDUCB *cb = pmdGetThreadEDUCB() ;
      if ( NULL != cb )
      {
         BSONElement ele = source.getField( DMS_ID_KEY_NAME ) ;
         if ( !ele.eoo() )
         {
            cb->appendInfo( EDU_INFO_ERROR, ",%s=%s", DMS_ID_KEY_NAME,
                            ele.toString( FALSE ).c_str() ) ;
         }
      }
      goto done ;
   }

}

