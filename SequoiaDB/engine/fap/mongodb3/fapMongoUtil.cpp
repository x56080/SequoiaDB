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

   Source File Name = fapMongoUtil.cpp

   Descriptive Name =

   When/how to use: this program may be used on binary and text-formatted
   versions of PMD component. This file contains functions for agent processing.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who         Description
   ====== =========== =========== ========================================
          11/04/2021  fangjiabin  Initial Draft

   Last Changed =

*******************************************************************************/
#include "fapMongoUtil.hpp"
#include "mthModifier.hpp"
#include "fapMongoMessage.hpp"
#include "pdSecure.hpp"

using namespace bson ;
using namespace engine ;

namespace fap
{

   static _mongoErrorObjAssit errorObjAssit ;

   /*
      _mongoMsgBuffer implement
   */
   _mongoMsgBuffer::_mongoMsgBuffer()
   {
      _pData = NULL ;
      _size = 0 ;
      _capacity = 0 ;
      _alloc( MEMERY_BLOCK_SIZE ) ;
   }

   _mongoMsgBuffer::~_mongoMsgBuffer()
   {
      if ( NULL != _pData )
      {
         SDB_OSS_FREE( _pData ) ;
         _pData = NULL ;
      }
      _size = 0 ;
      _capacity = 0 ;
   }

   INT32 _mongoMsgBuffer::_alloc( const UINT32 size )
   {
      INT32 rc = SDB_OK ;

      if( size <= _capacity )
      {
         goto done ;
      }

      _pData = ( CHAR *) SDB_OSS_MALLOC( size ) ;
      if ( NULL == _pData )
      {
         rc = SDB_OOM ;
         PD_LOG ( PDERROR, "Failed to malloc memory, rc: %d", rc ) ;
         goto error ;
      }

      _capacity = size ;

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _mongoMsgBuffer::_realloc( const UINT32 size )
   {
      INT32 rc = SDB_OK ;
      CHAR* ptr = NULL ;

      if ( size <= _capacity )
      {
         // do nothing
         goto done ;
      }

      ptr = ( CHAR * )SDB_OSS_REALLOC( _pData, size ) ;
      if ( NULL == ptr )
      {
         rc = SDB_OOM ;
         PD_LOG ( PDERROR, "Failed to realloc memory, rc: %d", rc ) ;
         goto error ;
      }

      _pData = ptr ;
      _capacity = size ;
      ossMemset( _pData + _size, 0, _capacity - _size ) ;

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _mongoMsgBuffer::write( const CHAR *pIn, const UINT32 inLen,
                                 BOOLEAN align , INT32 bytes )
   {
      INT32 rc   = SDB_OK ;
      INT32 size = 0 ;        // new size to realloc
      INT32 num  = 0 ;        // number of memory block

      if ( NULL == pIn )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG ( PDERROR, "The content to be written can't be null, "
                  "rc: %d", rc ) ;
         goto error ;
      }

      if( inLen > _capacity - _size )
      {
         // digit size of memory needed
         num = ( inLen + _size ) / MEMERY_BLOCK_SIZE + 1 ;
         size = num * MEMERY_BLOCK_SIZE ;

         rc = _realloc( size ) ;
         if( SDB_OK != rc )
         {
            goto error ;
         }
      }

      ossMemcpy( _pData + _size, pIn, inLen ) ;
      if ( align )
      {
         _size += ossRoundUpToMultipleX( inLen, bytes );
      }
      else
      {
         _size += inLen ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _mongoMsgBuffer::write( const BSONObj &obj, BOOLEAN align, INT32 bytes )
   {
      INT32 rc   = SDB_OK ;
      INT32 size = 0 ;        // new size to realloc
      INT32 num  = 0 ;        // number of memory block
      UINT32 objsize = obj.objsize() ;

      if( objsize > _capacity - _size )
      {
         // digit size of memory needed
         num = ( objsize + _size ) / MEMERY_BLOCK_SIZE + 1 ;
         size = num * MEMERY_BLOCK_SIZE ;

         rc = _realloc( size ) ;
         if( SDB_OK != rc )
         {
            goto error ;
         }
      }

      ossMemcpy( _pData + _size, obj.objdata(), objsize ) ;
      if ( align )
      {
         _size += ossRoundUpToMultipleX( objsize, bytes ) ;
      }
      else
      {
         _size += objsize ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _mongoMsgBuffer::advance( const UINT32 pos )
   {
      INT32 rc = SDB_OK ;

      if ( pos > _capacity )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG( PDERROR, "Position must be less than capacity, rc: %d", rc ) ;
         goto error ;
      }

      _size = pos ;

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _mongoMsgBuffer::reserve( const UINT32 size )
   {
      INT32 rc = SDB_OK ;

      if ( size < _capacity )
      {
         goto done ;
      }

      rc = _realloc( size ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Failed to realloc memory, rc: %d", rc ) ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   void _mongoMsgBuffer::zero()
   {
      ossMemset( _pData, 0, _capacity ) ;
      _size = 0 ;
   }

   /*
      _mongoFilterHelper implement
   */
   _mongoFilterHelper::_mongoFilterHelper()
   {
   }

   INT32 _mongoFilterHelper::loadPattern( const BSONObj &pattern )
   {
      return _matchTree.loadPattern( pattern ) ;
   }

   INT32 _mongoFilterHelper::matches( const BSONObj &matchTarget, BOOLEAN &result )
   {
      if ( _matchTree.isInitialized() )
      {
         return _matchTree.matches( matchTarget, result ) ;
      }
      else
      {
         result = TRUE ;
         return SDB_OK ;
      }
   }

   void mongoInitMsgHeader( MsgHeader *pMsg, INT32 opCode, UINT64 reqID, UINT32 tid )
   {
      pMsg->messageLength = sizeof( MsgHeader ) ;
      pMsg->eye = MSG_COMM_EYE_DEFAULT ;
      pMsg->opCode = opCode  ;
      pMsg->version = SDB_PROTOCOL_VER_CUR ;
      pMsg->flags = 0 ;
      pMsg->requestID = reqID ;
      pMsg->routeID.value = 0 ;
      pMsg->TID = tid ;

      ossMemset( &(pMsg->globalID), 0, sizeof(pMsg->globalID) ) ;
      ossMemset( pMsg->reserve, 0, sizeof( pMsg->reserve ) ) ;
   }

   /*
      _mongoErrorObjAssit implement
   */
   _mongoErrorObjAssit::_mongoErrorObjAssit()
   {
      for ( SINT32 i = -SDB_MAX_ERROR; i <= SDB_MAX_WARNING ; i ++ )
      {
         BSONObjBuilder berror ;
         berror.append ( FAP_MONGO_FIELD_NAME_OK, 0 ) ;
         berror.append ( FAP_MONGO_FIELD_NAME_CODE, utilSdbRC2MongoRC( i ) ) ;
         berror.append ( FAP_MONGO_FIELD_NAME_CODENAME, getErrDesp ( i ) ) ;
         berror.append ( FAP_MONGO_FIELD_NAME_ERRMSG, getErrDesp ( i ) ) ;
         _errorObjsArray[ i + SDB_MAX_ERROR ] = berror.obj() ;
      }
   }

   void _mongoErrorObjAssit::release()
   {
      for ( SINT32 i = -SDB_MAX_ERROR; i <= SDB_MAX_WARNING ; i ++ )
      {
         _errorObjsArray[ i + SDB_MAX_ERROR ] = BSONObj() ;
      }
   }

   BSONObj _mongoErrorObjAssit::getErrorObj( INT32 errorCode )
   {
      // check flags
      if ( errorCode < -SDB_MAX_ERROR || errorCode > SDB_MAX_WARNING )
      {
         PD_LOG ( PDERROR, "Error code error[rc:%d]", errorCode ) ;
         errorCode = SDB_SYS ;
      }
      return _errorObjsArray[ SDB_MAX_ERROR + errorCode ] ;
   }

   void mongoReleaseErrorBson()
   {
      errorObjAssit.release() ;
   }

   // generate a new record based on matcher condition and update condition
   INT32 mongoGenerateNewRecord( const BSONObj &matcher,
                                 const BSONObj &updatorObj,
                                 const BSONObj &setOnInsert,
                                 BSONObj &target )
   {
      INT32 rc = SDB_OK ;
      mthMatchTree matcherTree ;
      mthModifier modifier ;

      try
      {
         BSONObj source ;

         rc = matcherTree.loadPattern ( matcher ) ;
         PD_RC_CHECK ( rc, PDERROR, "Failed to load matcher[%s], rc: %d",
                       PD_SECURE_OBJ( matcher ), rc ) ;

         source = matcherTree.getEqualityQueryObject( NULL ) ;

         rc = modifier.loadPattern( updatorObj, NULL, TRUE, NULL, FALSE ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to load updator[%s], rc: %d",
                      PD_SECURE_OBJ( updatorObj ), rc ) ;

         rc = modifier.modify( source, target ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to generate new record, rc: %d",
                      rc ) ;

         if ( !setOnInsert.isEmpty() )
         {
            BSONObj newTarget ;
            BSONObj setObj ;
            BSONObjBuilder builder ;
            builder.append( FAP_MONGO_UPDATOR_SET, setOnInsert ) ;
            setObj = builder.obj() ;

            mthModifier setModifier ;
            rc = setModifier.loadPattern( setObj ) ;
            PD_RC_CHECK( rc, PDERROR, "Invalid pattern is detected: { %s }, "
                         "rc: %d", PD_SECURE_STR( setOnInsert.toString() ), rc ) ;
            rc = setModifier.modify( target, newTarget ) ;
            PD_RC_CHECK( rc, PDERROR, "failed to generate upsertor "
                         "record(rc=%d) by " FAP_MONGO_UPDATOR_SETINSERT, rc ) ;

            target = newTarget ;
         }

         // check if have oid
         if ( !target.hasElement( FAP_MONGO_FIELD_NAME_ID ) )
         {
            BSONObjBuilder builder ;
            builder.appendOID( FAP_MONGO_FIELD_NAME_ID, NULL, TRUE ) ;
            builder.appendElements( target ) ;
            target = builder.obj() ;
         }
      }
      catch( std::exception &e )
      {
         rc = ossException2RC( &e ) ;
         PD_LOG( PDERROR, "Generate new record exception: %s, rc: %d",
                 e.what(), rc ) ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   BSONObj mongoGetErrorBson( INT32 errorCode, const CHAR *pErrMsg )
   {
      if ( pErrMsg && '\0' != *pErrMsg )
      {
         try
         {
            BSONObjBuilder berror ;
            berror.append ( FAP_MONGO_FIELD_NAME_OK, 0 ) ;
            berror.append ( FAP_MONGO_FIELD_NAME_CODE, utilSdbRC2MongoRC( errorCode ) ) ;
            berror.append ( FAP_MONGO_FIELD_NAME_ERRMSG, pErrMsg ) ;
            return berror.obj() ;
         }
         catch( ... )
         {
         }
      }

      return errorObjAssit.getErrorObj( errorCode ) ;
   }

   void mongoBuildErrorBson( BSONObjBuilder &builder, INT32 errorCode,
                             const CHAR *pErrMsg, const BSONObj &objDetail )
   {
      BOOLEAN addCodeName = FALSE ;
      BOOLEAN addErrMsg = FALSE ;

      try
      {
         builder.append( FAP_MONGO_FIELD_NAME_OK, 0 ) ;
         builder.append( FAP_MONGO_FIELD_NAME_CODE, utilSdbRC2MongoRC( errorCode ) ) ;

         if ( !objDetail.isEmpty() )
         {
            BSONObjIterator itr( objDetail ) ;
            while( itr.more() )
            {
               BSONElement e = itr.next() ;
               if ( 0 == ossStrcmp( e.fieldName(), OP_ERRNOFIELD ) )
               {
                  /// ignore it
               }
               else if ( 0 == ossStrcmp( e.fieldName(), OP_ERRDESP_FIELD ) )
               {
                  builder.appendAs( e, FAP_MONGO_FIELD_NAME_CODENAME ) ;
                  addCodeName = TRUE ;
               }
               else if ( 0 == ossStrcmp( e.fieldName(), OP_ERR_DETAIL ) )
               {
                  if ( !addErrMsg && String == e.type() && *(e.valuestr()) )
                  {
                     builder.appendAs( e, FAP_MONGO_FIELD_NAME_ERRMSG ) ;
                     addErrMsg = TRUE ;
                  }
               }
               else
               {
                  /// append other
                  builder.append( e ) ;
               }
            }
         }

         if ( !addCodeName )
         {
            builder.append( FAP_MONGO_FIELD_NAME_CODENAME, getErrDesp( errorCode ) ) ;
            addCodeName = TRUE ;
         }
         if ( !addErrMsg )
         {
            if ( pErrMsg && *pErrMsg )
            {
               builder.append ( FAP_MONGO_FIELD_NAME_ERRMSG, pErrMsg ) ;
            }
            else
            {
               builder.append( FAP_MONGO_FIELD_NAME_ERRMSG, getErrDesp( errorCode ) ) ;
            }
            addErrMsg = TRUE ;
         }
      }
      catch ( ... )
      {
      }
   }

   BOOLEAN mongoCheckBigEndian()
   {
      BOOLEAN bigEndian = FALSE ;
      union
      {
         unsigned int i ;
         unsigned char s[4] ;
      } c ;

      c.i = 0x12345678 ;
      if ( 0x12 == c.s[0] )
      {
         bigEndian = TRUE ;
      }

      return bigEndian ;
   }

   INT32 mongoGetIntElement( const BSONObj &obj, const CHAR *pFieldName,
                             INT32 &value )
   {
      INT32 rc = SDB_OK ;
      SDB_ASSERT ( pFieldName, "field name can't be NULL" ) ;

      try
      {
         BSONElement ele = obj.getField ( pFieldName ) ;
         PD_CHECK ( !ele.eoo(), SDB_FIELD_NOT_EXIST, error, PDDEBUG,
                    "Can't locate field '%s': %s",
                    pFieldName,
                    PD_SECURE_OBJ( obj ) ) ;
         PD_CHECK ( ele.isNumber(), SDB_INVALIDARG, error, PDWARNING,
                    "Unexpected field type : %s, supposed to be Integer",
                    PD_SECURE_OBJ( obj ) ) ;
         value = ele.numberInt() ;
      }
      catch ( std::exception &e )
      {
         rc = ossException2RC( &e ) ;
         PD_LOG( PDERROR, "An exception occurred when getting int ele: %s, rc: %d",
                 e.what(), rc ) ;
         goto error ;
      }

   done :
      return rc ;
   error :
      goto done ;
   }

   INT32 mongoGetStringElement ( const BSONObj &obj, const CHAR *pFieldName,
                                 const CHAR *&pValue )
   {
      INT32 rc = SDB_OK ;
      SDB_ASSERT ( pFieldName && &pValue, "field name and value can't be NULL" ) ;

      try
      {
         BSONElement ele = obj.getField ( pFieldName ) ;
         PD_CHECK ( !ele.eoo(), SDB_FIELD_NOT_EXIST, error, PDDEBUG,
                    "Can't locate field '%s': %s",
                    pFieldName,
                    PD_SECURE_OBJ( obj ) ) ;
         PD_CHECK ( String == ele.type(), SDB_INVALIDARG, error, PDWARNING,
                    "Unexpected field type : %s, supposed to be String",
                    PD_SECURE_OBJ( obj ) ) ;
         pValue = ele.valuestr() ;
      }
      catch ( std::exception &e )
      {
         rc = ossException2RC( &e ) ;
         PD_LOG( PDERROR, "An exception occurred when getting string ele: "
                 "%s, rc: %d", e.what(), rc ) ;
         goto error ;
      }

   done :
      return rc ;
   error :
      goto done ;
   }

   INT32 mongoGetArrayElement ( const BSONObj &obj, const CHAR *pFieldName,
                                BSONObj &value )
   {
      INT32 rc = SDB_OK ;
      SDB_ASSERT ( pFieldName , "field name can't be NULL" ) ;

      try
      {
         BSONElement ele = obj.getField ( pFieldName ) ;
         PD_CHECK ( !ele.eoo(), SDB_FIELD_NOT_EXIST, error, PDDEBUG,
                    "Can't locate field '%s': %s",
                    pFieldName,
                    PD_SECURE_OBJ( obj ) ) ;
         PD_CHECK ( Array == ele.type(), SDB_INVALIDARG, error, PDWARNING,
                    "Unexpected field type : %s, supposed to be Array",
                    PD_SECURE_OBJ( obj ) ) ;
         value = ele.embeddedObject() ;
      }
      catch ( std::exception &e )
      {
         rc = ossException2RC( &e ) ;
         PD_LOG( PDERROR, "An exception occurred when getting array ele: "
                 "%s, rc: %d", e.what(), rc ) ;
         goto error ;
      }

   done :
      return rc ;
   error :
      goto done ;
   }

   INT32 mongoGetNumberLongElement ( const BSONObj &obj, const CHAR *pFieldName,
                                     INT64 &value )
   {
      INT32 rc = SDB_OK ;
      SDB_ASSERT ( pFieldName, "field name can't be NULL" ) ;

      try
      {
         BSONElement ele = obj.getField ( pFieldName ) ;
         PD_CHECK ( !ele.eoo(), SDB_FIELD_NOT_EXIST, error, PDDEBUG,
                    "Can't locate field '%s': %s",
                    pFieldName,
                    PD_SECURE_OBJ( obj ) ) ;
         PD_CHECK ( ele.isNumber(), SDB_INVALIDARG, error, PDWARNING,
                    "Unexpected field type : %s, supposed to be number",
                    PD_SECURE_OBJ( obj ) ) ;
         value = ele.numberLong() ;
      }
      catch ( std::exception &e )
      {
         rc = ossException2RC( &e ) ;
         PD_LOG( PDERROR, "An exception occurred when getting numberlong ele: %s, "
                 "rc: %d", e.what(), rc ) ;
         goto error ;
      }

   done :
      return rc ;
   error :
      goto done ;
   }

   INT32 mongoGetBooleanElement ( const BSONObj &obj, const CHAR *fieldName,
                                 BOOLEAN &value )
   {
      SINT32 rc = SDB_OK ;
      SDB_ASSERT ( fieldName , "field name can't be NULL" ) ;

      try
      {
         BSONElement ele = obj.getField ( fieldName ) ;
         PD_CHECK ( !ele.eoo(), SDB_FIELD_NOT_EXIST, error, PDDEBUG,
                    "Can't locate field '%s': %s",
                    fieldName,
                    obj.toString().c_str() ) ;
         PD_CHECK ( Bool == ele.type(), SDB_INVALIDARG, error, PDDEBUG,
                    "Unexpected field type : %s, supposed to be Bool",
                    obj.toString().c_str()) ;
         value = ele.boolean() ;
      }
      catch( std::exception &e )
      {
         rc = ossException2RC( &e ) ;
         PD_RC_CHECK( rc, PDERROR, "Occur exception: %s", e.what() ) ;
      }

   done :
      return rc ;
   error :
      goto done ;
   }

   INT32 mongoGetObjElement ( const BSONObj &obj, const CHAR *fieldName,
                              BSONObj &value )
   {
      SINT32 rc = SDB_OK ;
      SDB_ASSERT ( fieldName , "field name can't be NULL" ) ;

      try
      {
         BSONElement ele = obj.getField ( fieldName ) ;
         PD_CHECK ( !ele.eoo(), SDB_FIELD_NOT_EXIST, error, PDDEBUG,
                    "Can't locate field '%s': %s",
                    fieldName,
                    obj.toString().c_str() ) ;
         PD_CHECK ( Object == ele.type(), SDB_INVALIDARG, error, PDDEBUG,
                    "Unexpected field type : %s, supposed to be Object",
                    obj.toString().c_str()) ;
         value = ele.embeddedObject() ;
      }
      catch( std::exception &e )
      {
         rc = ossException2RC( &e ) ;
         PD_RC_CHECK( rc, PDERROR, "Occur exception: %s", e.what() ) ;
      }

   done :
      return rc ;
   error :
      goto done ;
   }

   INT32 mongoBuildDupkeyErrObj( const BSONObj &sdbErrobj, const CHAR* clFullName,
                                 BSONObjBuilder &builder )
   {
      INT32 rc = SDB_OK ;
      BSONElement indexNameEle ;
      BSONElement indexValueEle ;
      stringstream ss ;

      try
      {
         builder.append( FAP_MONGO_FIELD_NAME_OK, 0 ) ;
         builder.append( FAP_MONGO_FIELD_NAME_CODE, utilSdbRC2MongoRC( SDB_IXM_DUP_KEY ) ) ;
         builder.append( FAP_MONGO_FIELD_NAME_CODENAME, getErrDesp( SDB_IXM_DUP_KEY ) ) ;

         ss << getErrDesp( SDB_IXM_DUP_KEY ) ;

         BSONObjIterator itr( sdbErrobj ) ;
         while( itr.more() )
         {
            BSONElement e = itr.next() ;

            if ( 0 == ossStrcmp( e.fieldName(), FIELD_NAME_INDEXNAME ) )
            {
               indexNameEle = e ;
            }
            else if ( 0 == ossStrcmp( e.fieldName(), FIELD_NAME_INDEXVALUE ) )
            {
               indexValueEle = e ;
               builder.appendAs( e, FAP_MONGO_FIELD_NAME_KEYVAL ) ;
            }
            else if ( 0 == ossStrcmp( e.fieldName(), FIELD_NAME_INDEX ) )
            {
               builder.appendAs( e, FAP_MONGO_FIELD_NAME_KEYDEF ) ;
            }
            else if ( 0 == ossStrcmp( e.fieldName(), OP_ERRNOFIELD ) ||
                      0 == ossStrcmp( e.fieldName(), OP_ERRDESP_FIELD ) ||
                      0 == ossStrcmp( e.fieldName(), OP_ERR_DETAIL ) )
            {
               /// ignore it
            }
            else
            {
               builder.append( e ) ;
            }
         }

         if ( clFullName )
         {
            ss << " collection: " << clFullName ;
         }

         if ( String == indexNameEle.type() )
         {
            ss << " index: " << indexNameEle.String() ;
         }

         if ( Object == indexValueEle.type() )
         {
            ss << " dup key: " << indexValueEle.embeddedObject().toString() ;
         }

         builder.append ( FAP_MONGO_FIELD_NAME_ERRMSG, ss.str().c_str() ) ;
      }
      catch ( std::exception &e )
      {
         rc = ossException2RC( &e ) ;
         PD_LOG( PDERROR, "An exception occurred when building duplicate key "
                 "error obj: %s, rc: %d", e.what(), rc ) ;
         goto error ;
      }

   done :
      return rc ;
   error :
      goto done ;
   }

   INT32 mongoCheckUpdator( BSONObj &updator, BOOLEAN &hasOp, BSONObj &setOnInsert )
   {
      INT32 rc = SDB_OK ;
      hasOp = FALSE ;
      BOOLEAN needBuild = FALSE ;

      try
      {
         BSONObjIterator itr( updator ) ;
         while ( itr.more() )
         {
            BSONElement ele = itr.next() ;
            if ( '$' == ele.fieldName()[0] )
            {
               hasOp = TRUE ;
               if ( 0 == ossStrcmp( ele.fieldName(), FAP_MONGO_UPDATOR_SETINSERT ) )
               {
                  needBuild = TRUE ;
                  break ;
               }
            }
            else
            {
               /// when first is not operation, it's {a:x,b:y} for replace
               break ;
            }
         }

         if ( needBuild )
         {
            BSONObjBuilder builder( updator.objsize() ) ;
            BSONObjIterator itr( updator ) ;
            while ( itr.more() )
            {
               BSONElement ele = itr.next() ;
               if ( '$' == ele.fieldName()[0] )
               {
                  hasOp = TRUE ;
                  if ( 0 == ossStrcmp( ele.fieldName(), FAP_MONGO_UPDATOR_SETINSERT ) )
                  {
                     if ( Object == ele.type() )
                     {
                        setOnInsert = ele.embeddedObject() ;
                     }
                     continue ;
                  }
               }

               builder.append( ele ) ;
            }

            if ( builder.isEmpty() )
            {
               builder.append( FAP_MONGO_UPDATOR_SET, BSONObj() ) ;
            }
            updator = builder.obj() ;
         }
      }
      catch( std::exception &e )
      {
         rc = ossException2RC( &e ) ;
         PD_LOG( PDERROR, "An exception occurred when checking if cond has op: "
                 "%s, rc: %d", e.what(), rc ) ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   void mongoFixInsertObject( const BSONObj &inObj, BSONObjBuilder &builder,
                              BOOLEAN &hasRebuildOID, BSONObj *pOutObj )
   {
      BSONElement idEle ;
      INT32 fixPos = -1 ;
      ossTimestamp tm ;
      INT32 index = -1 ;
      hasRebuildOID = FALSE ;

      BSONObjIterator itr ( inObj ) ;
      while( itr.more() )
      {
         ++index ;
         BSONElement e = itr.next() ;

         if ( -1 == fixPos && Timestamp == e.type() &&
              (Date_t)0 == e.timestampTime() && 0 == e.timestampInc() )
         {
            fixPos = index ;
         }
         else if ( idEle.eoo() && 0 == ossStrcmp( e.fieldName(), FAP_MONGO_FIELD_NAME_ID ) )
         {
            idEle = e ;
         }

         if ( -1 != fixPos && !idEle.eoo() )
         {
            break ;
         }
      }

      if ( !idEle.eoo() && -1 == fixPos )
      {
         if ( pOutObj )
         {
            *pOutObj = inObj ;
         }
         else
         {
            builder.appendElements( inObj ) ;
            builder.done() ;
         }
         goto done ;
      }

      /// rebuild object
      if ( idEle.eoo() )
      {
         builder.appendOID( FAP_MONGO_FIELD_NAME_ID, NULL, TRUE ) ;
         hasRebuildOID = TRUE ;
      }
      else
      {
         builder.append( idEle ) ;
      }

      index = -1 ;
      itr = BSONObjIterator( inObj ) ;
      while( itr.more() )
      {
         ++index ;
         BSONElement e = itr.next() ;
         /// skip _id
         if ( !idEle.eoo() && idEle.fieldName() == e.fieldName() )
         {
            idEle = BSONElement() ;
         }
         else if ( -1 == fixPos || index < fixPos )
         {
            builder.append( e ) ;
         }
         else if ( Timestamp == e.type() && (Date_t)0 == e.timestampTime() &&
                   0 == e.timestampInc() )
         {
            if ( 0 == tm.time )
            {
               ossGetCurrentTime( tm ) ;
            }
            OpTime opTm( tm.time, tm.microtm ) ;
            builder.appendTimestamp( e.fieldName(), opTm.asDate() ) ;
         }
         else
         {
            builder.append( e ) ;
         }
      }

      if ( pOutObj )
      {
         *pOutObj = builder.done() ;
      }
      else
      {
         builder.done() ;
      }

   done:
      return ;
   }

   INT32 mongoRebuildOKReply( engine::rtnContextBuf &bodyBuf )
   {
      INT32 rc = SDB_OK ;

      try
      {
         BSONObjBuilder bob ;
         bob.append( FAP_MONGO_FIELD_NAME_OK, 1 ) ;

         if ( 1 == bodyBuf.recordNum() )
         {
            BSONObj obj( bodyBuf.data() ) ;
            bob.appendElementsUnique( obj ) ;
         }

         bodyBuf = engine::rtnContextBuf( bob.obj() ) ;
      }
      catch ( std::exception &e )
      {
         rc = ossException2RC( &e ) ;
         PD_LOG( PDERROR, "An exception occurred when rebuilding OK reply: %s, "
                 "rc: %d", e.what(), rc ) ;
         goto error ;
      }

   done :
      return rc ;
   error :
      goto done ;
   }

   std::string mongoGetNonce()
   {
      const UINT64 n = 0 ;
      std::stringstream ss ;
      ss << std::hex << n ;
      return ss.str() ;
   }

   struct _utilFapRCMapItem
   {
      INT32 sdbRC ;
      INT32 mongoRC ;
   } ;

   static const _utilFapRCMapItem s_Sdb2MongoRCMap[] =
   {
      { SDB_DMS_CS_NOTEXIST, 26 },
      { SDB_DMS_NOTEXIST, 26 },
      { SDB_AUTH_AUTHORITY_FORBIDDEN, 18 },
      { SDB_AUTH_INCOMPATIBLE, 18 },
      { SDB_IXM_NOTEXIST, 27 },
      { SDB_IXM_EXIST, 85 }
   } ;

   INT32 utilSdbRC2MongoRC( INT32 sdbRC )
   {
      if ( sdbRC < 0 )
      {
         for ( const auto &item : s_Sdb2MongoRCMap )
         {
            if ( item.sdbRC == sdbRC )
            {
               return item.mongoRC ;
            }
         }
      }
      return sdbRC ;
   }
}

