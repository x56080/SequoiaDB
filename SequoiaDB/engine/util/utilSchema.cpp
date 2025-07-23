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

   Source File Name = utilSchema.cpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          02/07/2023  HGM Initial Draft

   Last Changed =

*******************************************************************************/
#include "utilSchema.hpp"
#include "msgDef.hpp"
#include "msg.h"
#include "utilStr.hpp"
#include "pd.hpp"
#include "utilTrace.hpp"
#include "pdTrace.hpp"

using namespace bson ;

namespace engine
{
   #define UTIL_SCHEMA_COLUMN_NAME_SZ 127

   static void _utilAppendFlagString( CHAR *buffer,
                                      const CHAR *flagStr )
   {
      if ( 0 != *buffer )
      {
         ossStrncat( buffer, "|", 1 ) ;
      }
      ossStrncat( buffer, flagStr, ossStrlen( flagStr ) ) ;
   }

   static const CHAR *_utilRestrict2RestrictDesc( UINT32 value, CHAR *buffer )
   {
      buffer[ 0 ] = '\0' ;

      if ( OSS_BIT_TEST( value, UTIL_SCHEMA_COLUMN_NOT_NULL ) )
      {
         _utilAppendFlagString( buffer, FIELD_NAME_NOT_NULL ) ;
      }
      if ( OSS_BIT_TEST( value, UTIL_SCHEMA_COLUMN_NOT_ARRAY ) )
      {
         _utilAppendFlagString( buffer, FIELD_NAME_NOT_ARRAY ) ;
      }

      return buffer ;
   }

   static INT32 _utilRestrictDesc2Restrict( const CHAR *desc, UINT32 &value )
   {
      INT32 rc = SDB_OK ;

      value = 0 ;

      try
      {
         vector< string > values ;
         values = utilStrSplit( desc, "|" ) ;
         for ( vector< string >::const_iterator itr = values.begin() ;
               itr != values.end() ;
               ++ itr )
         {
            if ( 0 == ossStrcasecmp( itr->c_str(), FIELD_NAME_NOT_NULL ) )
            {
               OSS_BIT_SET( value, UTIL_SCHEMA_COLUMN_NOT_NULL ) ;
            }
            else if ( 0 == ossStrcasecmp( itr->c_str(), FIELD_NAME_NOT_ARRAY ) )
            {
               OSS_BIT_SET( value, UTIL_SCHEMA_COLUMN_NOT_ARRAY ) ;
            }
            else
            {
               rc = SDB_INVALIDARG ;
               PD_LOG_MSG( PDERROR, "Failed to parse restrict description, "
                           "invalid restrict [%s]", itr->c_str() ) ;
               goto error ;
            }
         }
      }
      catch ( exception &e )
      {
         PD_LOG( PDERROR, "Failed to parse restrict description, "
                 "occur exception %s", e.what() ) ;
         rc = ossException2RC( &e ) ;
         goto error ;
      }

   done:
      return rc ;

   error:
      goto done ;
   }

   static INT32 _utilCheckColumnName( const CHAR *columnName )
   {
      INT32 rc = SDB_OK ;

      PD_LOG_MSG_CHECK( NULL != columnName,
                        SDB_INVALIDARG, error, PDERROR,
                        "Failed to check column name, should not be invalid" ) ;
      PD_LOG_MSG_CHECK( '\0' != columnName[ 0 ],
                        SDB_INVALIDARG, error, PDERROR,
                        "Failed to check column name, should not be empty" ) ;
      PD_LOG_MSG_CHECK( '$' != columnName[ 0 ],
                        SDB_INVALIDARG, error, PDERROR,
                        "Failed to check column name [%s], should not "
                        "start with \'$\' in column name", columnName ) ;
      PD_LOG_MSG_CHECK( NULL == ossStrchr( columnName, '.' ),
                        SDB_INVALIDARG, error, PDERROR,
                        "Failed to check column name [%s], should not "
                        "contain \'.\' in column name", columnName ) ;
      PD_LOG_MSG_CHECK( UTIL_SCHEMA_COLUMN_NAME_SZ >= ossStrlen( columnName ),
                        SDB_INVALIDARG, error, PDERROR,
                        "Failed to check column name [%s], whose size "
                        "should not be greater than %d", columnName, UTIL_SCHEMA_COLUMN_NAME_SZ) ;

   done:
      return rc ;

   error:
      goto done ;
   }

   static INT32 _utilCheckKeyPattern( const BSONObj &keyPattern,
                                      const CHAR *columnName,
                                      BOOLEAN &hasColumn )
   {
      INT32 rc = SDB_OK ;

      SDB_ASSERT( NULL != columnName, "column is invalid" ) ;

      UINT32 colNameLen = ossStrlen( columnName ) ;
      hasColumn = FALSE ;

      try
      {
         BSONObjIterator iter( keyPattern ) ;
         while ( iter.more() )
         {
            BSONElement ele = iter.next() ;
            const CHAR *keyName = ele.fieldName() ;
            if ( ( 0 == ossStrncmp( keyName, columnName, colNameLen ) ) &&
                 ( ( '\0' == keyName[ colNameLen ] ) ||
                   ( '.' == keyName[ colNameLen ] ) ) )
            {
               hasColumn = TRUE ;
               break ;
            }
         }
      }
      catch ( exception &e )
      {
         PD_LOG( PDERROR, "Failed to check column, occur exception %s",
                 e.what() ) ;
         rc = ossException2RC( &e ) ;
         goto error ;
      }

   done:
      return rc ;

   error:
      goto done ;
   }

   /*
      _utilSchemaColAttr implement
    */
   _utilSchemaColAttr::_utilSchemaColAttr()
   : _name( NULL ),
     _type( EOO ),
     _restrictFlags( 0 ),
     _writeDefault(),
     _readDefault()
   {
   }

   _utilSchemaColAttr::_utilSchemaColAttr( const _utilSchemaColAttr &attr )
   : _name( attr._name ),
     _type( attr._type ),
     _restrictFlags( attr._restrictFlags ),
     _writeDefault( attr._writeDefault ),
     _readDefault( attr._readDefault )
   {
   }

   _utilSchemaColAttr::~_utilSchemaColAttr()
   {
   }

   INT32 _utilSchemaColAttr::parse( const CHAR *name,
                                    const bson::BSONObj &boDefine,
                                    BOOLEAN fromUser,
                                    BOOLEAN isNewAdded,
                                    UINT32 &parsedMask )
   {
      INT32 rc = SDB_OK ;

      _reset() ;

      try
      {
         const CHAR *typeName = NULL ;
         BOOLEAN hasDefault = FALSE ;

         parsedMask = 0 ;

         BSONObjIterator iter( boDefine ) ;
         while ( iter.more() )
         {
            BSONElement ele = iter.next() ;
            const CHAR *fieldName = ele.fieldName() ;

            if ( 0 == ossStrcmp( FIELD_NAME_TYPE, fieldName ) )
            {
               PD_CHECK( String == ele.type(), SDB_INVALIDARG, error, PDERROR,
                         "Failed to parse field [%s], it is not a string",
                         FIELD_NAME_TYPE ) ;
               typeName = ele.valuestrsafe() ;
               _type = bsonColumnTypeNameToBSONType( typeName ) ;
               PD_CHECK( EOO != _type, SDB_INVALIDARG, error, PDERROR,
                         "Failed to parse field [%s], unknown type [%s]",
                         FIELD_NAME_TYPE, typeName ) ;
               OSS_BIT_SET( parsedMask, UTIL_SCHEMA_ATTR_MASK_COL_TYPE ) ;
            }
            else if ( 0 == ossStrcmp( FIELD_NAME_RESTRICT, fieldName ) )
            {
               if ( fromUser )
               {
                  const CHAR *desc = NULL ;
                  UINT32 restrictFlags = 0 ;

                  PD_CHECK( String == ele.type(), SDB_INVALIDARG, error, PDERROR,
                            "Failed to parse field [%s], it is not a string",
                            FIELD_NAME_RESTRICT ) ;
                  desc = ele.valuestrsafe() ;
                  rc = _utilRestrictDesc2Restrict( desc, restrictFlags ) ;
                  PD_RC_CHECK( rc, PDERROR, "Failed to parse restrict "
                               "description, rc: %d", rc ) ;
                  _restrictFlags = restrictFlags ;
               }
               else
               {
                  PD_CHECK( NumberInt == ele.type(), SDB_INVALIDARG, error, PDERROR,
                            "Failed to parse field [%s], it is not a integer",
                            FIELD_NAME_RESTRICT ) ;
                  _restrictFlags = (UINT32)( ele.numberInt() ) ;
               }

               OSS_BIT_SET( parsedMask, UTIL_SCHEMA_ATTR_MASK_COL_NNULL ) ;
               OSS_BIT_SET( parsedMask, UTIL_SCHEMA_ATTR_MASK_COL_NARRAY ) ;
            }
            else if ( ( fromUser ) &&
                      ( 0 == ossStrcmp( FIELD_NAME_DEFAULT, fieldName ) ) )
            {
               // check if conflicts with WriteDefault and ReadDefault
               if ( OSS_BIT_TEST( parsedMask, UTIL_SCHEMA_ATTR_MASK_COL_WDEF ) )
               {
                  PD_LOG_MSG( PDERROR, "Failed to parse field [%s], "
                              "already defined with [%s]",
                              FIELD_NAME_DEFAULT, FIELD_NAME_WRITEDEFAULT ) ;
                  rc = SDB_INVALIDARG ;
                  goto error ;
               }
               if ( OSS_BIT_TEST( parsedMask, UTIL_SCHEMA_ATTR_MASK_COL_RDEF ) )
               {
                  PD_LOG_MSG( PDERROR, "Failed to parse field [%s], "
                              "already defined with [%s]",
                              FIELD_NAME_DEFAULT, FIELD_NAME_READDEFAULT ) ;
                  rc = SDB_INVALIDARG ;
                  goto error ;
               }
               _writeDefault = ele ;
               OSS_BIT_SET( parsedMask, UTIL_SCHEMA_ATTR_MASK_COL_WDEF ) ;
               if ( isNewAdded )
               {
                  _readDefault = ele ;
                  OSS_BIT_SET( parsedMask, UTIL_SCHEMA_ATTR_MASK_COL_RDEF ) ;
               }
               hasDefault = TRUE ;
            }
            else if ( 0 == ossStrcmp( FIELD_NAME_WRITEDEFAULT, fieldName ) )
            {
               if ( fromUser && hasDefault )
               {
                  PD_LOG_MSG( PDERROR, "Failed to parse field [%s], "
                              "already defined with [%s]",
                              FIELD_NAME_WRITEDEFAULT, FIELD_NAME_DEFAULT ) ;
                  rc = SDB_INVALIDARG ;
                  goto error ;
               }
               _writeDefault = ele ;
               OSS_BIT_SET( parsedMask, UTIL_SCHEMA_ATTR_MASK_COL_WDEF ) ;
            }
            else if ( 0 == ossStrcmp( FIELD_NAME_READDEFAULT, fieldName ) )
            {
               if ( fromUser && hasDefault )
               {
                  PD_LOG_MSG( PDERROR, "Failed to parse field [%s], "
                              "already defined with [%s]",
                              FIELD_NAME_READDEFAULT, FIELD_NAME_DEFAULT ) ;
                  rc = SDB_INVALIDARG ;
                  goto error ;
               }
               _readDefault = ele ;
               OSS_BIT_SET( parsedMask, UTIL_SCHEMA_ATTR_MASK_COL_RDEF ) ;
            }
            else if ( fromUser )
            {
               PD_LOG_MSG( PDERROR, "Failed to parse field [%s], "
                           "it is unknown", fieldName ) ;
               rc = SDB_INVALIDARG ;
               goto error ;
            }
         }

//         if ( fromUser &&
//              OSS_BIT_TEST( parsedMask, UTIL_SCHEMA_ATTR_MASK_COL_TYPE ) )
//         {
//            if ( OSS_BIT_TEST( parsedMask, UTIL_SCHEMA_ATTR_MASK_COL_WDEF ) )
//            {
//               PD_CHECK( ( ( _type == _writeDefault.type() ) ||
//                           ( Undefined == _writeDefault.type() ) ||
//                           ( jstNULL == _writeDefault.type() ) ),
//                         SDB_INVALIDARG, error, PDERROR,
//                         "Failed to parse field [%s], it is not a type [%s]",
//                         FIELD_NAME_WRITEDEFAULT, getWriteDefaultTypeName() ) ;
//            }
//            if ( OSS_BIT_TEST( parsedMask, UTIL_SCHEMA_ATTR_MASK_COL_RDEF ) )
//            {
//               PD_CHECK( ( ( _type == _readDefault.type() ) ||
//                           ( Undefined == _readDefault.type() ) ||
//                           ( jstNULL == _readDefault.type() ) ),
//                         SDB_INVALIDARG, error, PDERROR,
//                         "Failed to parse field [%s], it is not a type [%s]",
//                         FIELD_NAME_READDEFAULT, getReadDefaultTypeName() ) ;
//            }
//         }

         _name = name ;
      }
      catch ( exception &e )
      {
         PD_LOG( PDERROR, "Failed parse schema column, occur exception %s",
                 e.what() ) ;
         rc = ossException2RC( &e ) ;
         goto error ;
      }

   done:
      return rc ;

   error:
      goto done ;
   }

   INT32 _utilSchemaColAttr::toBSON( BSONObjBuilder &builder,
                                     UINT32 mask ) const
   {
      INT32 rc = SDB_OK ;

      try
      {
         CHAR restrictDescString[ 128 ] = { 0 } ;

         if ( OSS_BIT_TEST( mask, UTIL_SCHEMA_ATTR_MASK_COL_TYPE ) &&
              EOO != _type )
         {
            const CHAR *typeName = bsonTypeToColumnTypeName( _type ) ;
            if ( '\0' != typeName[ 0 ] )
            {
               builder.append( FIELD_NAME_TYPE, typeName ) ;
            }
         }

         if ( OSS_BIT_TEST( mask, UTIL_SCHEMA_ATTR_MASK_COL_RDEF ) &&
              !_readDefault.eoo() )
         {
            builder.appendAs( _readDefault, FIELD_NAME_READDEFAULT ) ;
         }

         if ( OSS_BIT_TEST( mask, UTIL_SCHEMA_ATTR_MASK_COL_WDEF ) &&
              !_writeDefault.eoo() )
         {
            builder.appendAs( _writeDefault, FIELD_NAME_WRITEDEFAULT ) ;
         }

         if ( OSS_BIT_TEST( mask,
                            ( UTIL_SCHEMA_ATTR_MASK_COL_NNULL |
                              UTIL_SCHEMA_ATTR_MASK_COL_NARRAY ) ) )
         {
            builder.append( FIELD_NAME_RESTRICT, _restrictFlags ) ;
            builder.append( FIELD_NAME_RESTRICT_DESC,
                            _utilRestrict2RestrictDesc( _restrictFlags,
                                                        restrictDescString ) ) ;
         }
      }
      catch ( exception &e )
      {
         PD_LOG( PDERROR, "Failed to build BSON for column attr, "
                 "occur exception %s", e.what() ) ;
         rc = ossException2RC( &e ) ;
         goto error ;
      }

   done:
      return rc ;

   error:
      goto done ;
   }

   const CHAR *_utilSchemaColAttr::getTypeName() const
   {
      return bsonTypeToColumnTypeName( _type ) ;
   }

   const CHAR *_utilSchemaColAttr::getWriteDefaultTypeName() const
   {
      return bsonTypeToColumnTypeName( getWriteDefaultType() ) ;
   }

   const CHAR *_utilSchemaColAttr::getReadDefaultTypeName() const
   {
      return bsonTypeToColumnTypeName( getReadDefaultType() ) ;
   }

   BOOLEAN _utilSchemaColAttr::adjustForOID()
   {
      BOOLEAN hasAdjust = FALSE ;

      if ( EOO != _type )
      {
         _type = EOO ;
         hasAdjust = TRUE ;
      }
      if ( 0 != _restrictFlags )
      {
         _restrictFlags = 0 ;
         hasAdjust = TRUE ;
      }
      if ( !_writeDefault.eoo() )
      {
         _writeDefault = BSONElement() ;
         hasAdjust = TRUE ;
      }
      if ( !_readDefault.eoo() )
      {
         _readDefault = BSONElement() ;
         hasAdjust = TRUE ;
      }

      return hasAdjust ;
   }

   BOOLEAN _utilSchemaColAttr::adjustForShardingKey()
   {
      BOOLEAN hasAdjust = FALSE ;

      if ( !_readDefault.eoo() )
      {
         _readDefault = BSONElement() ;
         hasAdjust = TRUE ;
      }

      return hasAdjust ;
   }

   /*
      _utilSchemaColumn implement
    */
   _utilSchemaColumn::_utilSchemaColumn()
   : _define()
   {
   }

   _utilSchemaColumn::_utilSchemaColumn( const _utilSchemaColAttr &attr )
   : _utilSchemaColAttr( attr ),
     _define()
   {
   }

   _utilSchemaColumn::~_utilSchemaColumn()
   {
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__UTILSCHEMACOL_PARSE, "_utilSchemaColumn::parse" )
   INT32 _utilSchemaColumn::parse( const CHAR *name,
                                   const BSONObj &boDefine,
                                   BOOLEAN fromUser,
                                   BOOLEAN needGetOwned )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__UTILSCHEMACOL_PARSE ) ;

      _reset() ;

      try
      {
         UINT32 parsedMask = 0 ;

         _define = needGetOwned ? boDefine.getOwned() : boDefine ;

         rc = _utilSchemaColAttr::parse( name, boDefine, fromUser, fromUser, parsedMask ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to parse attributes of column[%s], "
                      "rc: %d", name, rc ) ;
         if ( fromUser )
         {
            PD_CHECK( OSS_BIT_TEST( parsedMask, UTIL_SCHEMA_ATTR_MASK_COL_TYPE ),
                      SDB_INVALIDARG, error, PDERROR,
                      "Failed to parse attributes of column [%s], unknown type",
                      name ) ;
         }
      }
      catch ( exception &e )
      {
         PD_LOG( PDERROR, "Failed parse schema column, occur exception %s",
                 e.what() ) ;
         rc = ossException2RC( &e ) ;
         goto error ;
      }

   done:
      PD_TRACE_EXITRC( SDB__UTILSCHEMACOL_PARSE, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__UTILSCHEMACOL_TOBSON_BLD, "_utilSchemaColumn::toBSON" )
   INT32 _utilSchemaColumn::toBSON( BSONObjBuilder &builder ) const
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__UTILSCHEMACOL_TOBSON_BLD ) ;

      try
      {
         BSONObjBuilder subBuilder( builder.subobjStart( _name ) ) ;
         rc = _utilSchemaColAttr::toBSON( subBuilder ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to build BSON for attributes, "
                      "rc: %d", rc ) ;
         subBuilder.doneFast() ;
      }
      catch ( exception &e )
      {
         PD_LOG( PDERROR, "Failed to build BSON for column [%s], "
                 "occur exception %s", _name, e.what() ) ;
         rc = ossException2RC( &e ) ;
         goto error ;
      }

   done:
      PD_TRACE_EXITRC( SDB__UTILSCHEMACOL_TOBSON_BLD, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__UTILSCHEMACOL_TOBSON, "_utilSchemaColumn::toBSON" )
   INT32 _utilSchemaColumn::toBSON( bson::BSONObj &boColumn ) const
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__UTILSCHEMACOL_TOBSON ) ;

      try
      {
         BSONObjBuilder builder ;

         rc = toBSON( builder ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to build BSON for "
                      "schema column [%s], rc: %d", _name, rc ) ;
      }
      catch ( exception &e )
      {
         PD_LOG( PDERROR, "Failed to build BSON for schema column [%s], "
                 "occur exception %s", _name, e.what() ) ;
         rc = ossException2RC( &e ) ;
         goto error ;
      }

   done:
      PD_TRACE_EXITRC( SDB__UTILSCHEMACOL_TOBSON, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   /*
      _utilSchemaAttr implement
    */
   _utilSchemaAttr::_utilSchemaAttr()
   : _strictMode( FALSE )
   {
   }

   _utilSchemaAttr::~_utilSchemaAttr()
   {
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__UTILSCHEMAATTR_PARSE, "_utilSchemaAttr::parse" )
   INT32 _utilSchemaAttr::parse( const BSONObj &boOptions,
                                 BOOLEAN fromUser,
                                 UINT32 &parsedMask )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__UTILSCHEMAATTR_PARSE ) ;

      _reset() ;

      parsedMask = 0 ;

      try
      {
         BSONObjIterator it( boOptions ) ;
         while ( it.more() )
         {
            BSONElement ele = it.next() ;
            if ( 0 == ossStrcmp( FIELD_NAME_STRICTMODE, ele.fieldName() ) )
            {
               PD_CHECK( Bool == ele.type(), SDB_INVALIDARG, error, PDERROR,
                         "Failed to parse field [%s], it is not a boolean", FIELD_NAME_STRICTMODE ) ;
               _strictMode = ele.boolean() ;
               OSS_BIT_SET( parsedMask, UTIL_SCHEMA_ATTR_MASK_STRICTMODE ) ;
            }
            else
            {
               PD_LOG_MSG( PDERROR, "Failed to parse field [%s], it is unknown", ele.fieldName() ) ;
               rc = SDB_INVALIDARG ;
               goto error ;
            }
         }
      }
      catch ( exception &e )
      {
         PD_LOG( PDERROR, "Failed parse schema, occur exception %s", e.what() ) ;
         rc = ossException2RC( &e ) ;
         goto error ;
      }

   done:
      PD_TRACE_EXITRC( SDB__UTILSCHEMAATTR_PARSE, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__UTILSCHEMAATTR_TOBSON, "_utilSchemaAttr::toBSON" )
   INT32 _utilSchemaAttr::toBSON( BSONObjBuilder &builder,
                                  UINT32 mask ) const
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__UTILSCHEMAATTR_TOBSON ) ;

      try
      {
         if ( OSS_BIT_TEST( mask, UTIL_SCHEMA_ATTR_MASK_STRICTMODE ) )
         {
            builder.appendBool( FIELD_NAME_STRICTMODE, _strictMode ) ;
         }
      }
      catch ( exception &e )
      {
         PD_LOG( PDERROR, "Failed to build BSON, occur exception %s",
                 e.what() ) ;
         rc = ossException2RC( &e ) ;
         goto error ;
      }

   done:
      PD_TRACE_EXITRC( SDB__UTILSCHEMAATTR_TOBSON, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__UTILSCHEMAATTR__PARSEELE, "_utilSchemaAttr::_parse" )
   INT32 _utilSchemaAttr::_parse( const bson::BSONElement &ele,
                                  BOOLEAN fromUser,
                                  UINT32 &parsedMask )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__UTILSCHEMAATTR__PARSEELE ) ;

      try
      {
         const CHAR *fieldName = ele.fieldName() ;
         if ( 0 == ossStrcmp( fieldName, FIELD_NAME_STRICTMODE ) )
         {
            PD_CHECK( Bool == ele.type(), SDB_INVALIDARG, error, PDERROR,
                      "Failed to parse field [%s], it is not a boolean",
                      FIELD_NAME_STRICTMODE ) ;
            _strictMode = ele.boolean() ;
            OSS_BIT_SET( parsedMask, UTIL_SCHEMA_ATTR_MASK_STRICTMODE ) ;
         }
         else if ( fromUser )
         {
            PD_LOG_MSG( PDERROR, "Failed to parse field [%s], "
                        "it is unknown", fieldName ) ;
            rc = SDB_INVALIDARG ;
            goto error ;
         }
      }
      catch ( exception &e )
      {
         PD_LOG( PDERROR, "Failed parse schema, occur exception %s", e.what() ) ;
         rc = ossException2RC( &e ) ;
         goto error ;
      }

   done:
      PD_TRACE_EXITRC( SDB__UTILSCHEMAATTR__PARSEELE, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   /*
      _utilSchema implement
    */
   _utilSchema::_utilSchema()
   : _utilSchemaAttr(),
     _define(),
     _name( NULL ),
     _version( 0 ),
     _columnList(),
     _collection( NULL )
   {
   }

   _utilSchema::~_utilSchema()
   {
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__UTILSCHEMA_PARSE, "_utilSchema::parse" )
   INT32 _utilSchema::parse( const bson::BSONObj &boDefine,
                             BOOLEAN fromUser,
                             BOOLEAN needGetOwned )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__UTILSCHEMA_PARSE ) ;

      _reset() ;

      try
      {
         UINT32 parsedMask = 0 ;

         _define = needGetOwned ? boDefine.getOwned() : boDefine ;

         BSONObjIterator iter( _define ) ;
         while ( iter.more() )
         {
            BSONElement ele = iter.next() ;
            const CHAR *fieldName = ele.fieldName() ;

            if ( 0 == ossStrcmp( fieldName, FIELD_NAME_NAME ) )
            {
               PD_CHECK( String == ele.type(), SDB_INVALIDARG, error, PDERROR,
                         "Failed to parse field [%s], it is not a string",
                         FIELD_NAME_NAME ) ;
               if ( fromUser )
               {
                  PD_CHECK( ele.valuestrsize() <= UTIL_SCHEMA_NAME_MAX_SIZE,
                            SDB_INVALIDARG, error, PDERROR,
                            "Failed to parse field [%s], the limit of size is %u",
                            FIELD_NAME_NAME, UTIL_SCHEMA_NAME_MAX_SIZE ) ;
               }
               _name = ele.valuestrsafe() ;
            }
            else if ( 0 == ossStrcmp( fieldName, FIELD_NAME_COLUMNS ) )
            {
               PD_CHECK( Object == ele.type(), SDB_INVALIDARG, error, PDERROR,
                         "Failed to parse field [%s], it is not an array",
                         FIELD_NAME_COLUMNS ) ;
               rc = _parseColumns( ele.embeddedObject(), fromUser ) ;
               PD_RC_CHECK( rc, PDERROR, "Failed to parse columns, rc: %d", rc ) ;
            }
            else if ( 0 == ossStrcmp( fieldName, FIELD_NAME_VERSION ) &&
                      !fromUser )
            {
               PD_CHECK( NumberInt == ele.type(), SDB_INVALIDARG, error, PDERROR,
                         "Failed to parse field [%s], it is not an integer",
                         FIELD_NAME_VERSION ) ;
               _version = (UINT32)( ele.numberInt() ) ;
            }
            else if ( 0 == ossStrcmp( fieldName, FIELD_NAME_COLLECTION ) &&
                      !fromUser )
            {
               PD_CHECK( String == ele.type(), SDB_INVALIDARG, error, PDERROR,
                         "Failed to parse field [%s], it is not a string",
                         FIELD_NAME_COLLECTION ) ;
               _collection = ele.valuestr() ;
            }
            else
            {
               rc = _utilSchemaAttr::_parse( ele, fromUser, parsedMask ) ;
               PD_RC_CHECK( rc, PDERROR, "Failed to parse schema attributes, "
                            "rc: %d", rc ) ;
            }
         }
      }
      catch ( exception &e )
      {
         PD_LOG( PDERROR, "Failed parse schema, occur exception %s", e.what() ) ;
         rc = ossException2RC( &e ) ;
         goto error ;
      }

   done:
      PD_TRACE_EXITRC( SDB__UTILSCHEMA_PARSE, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__UTILSCHEMA_TOBSON, "_utilSchema::toBSON" )
   INT32 _utilSchema::toBSON( bson::BSONObj &boSchema, UINT32 mask ) const
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__UTILSCHEMA_TOBSON ) ;

      try
      {
         BSONObjBuilder builder ;

         rc = toBSON( builder, mask ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to build BSON for schema, "
                      "rc: %d", rc ) ;

         boSchema = builder.obj() ;
      }
      catch ( exception &e )
      {
         PD_LOG( PDERROR, "Failed to build BSON for schema, "
                 "occur exception %s", e.what() ) ;
         rc = ossException2RC( &e ) ;
         goto error ;
      }

   done:
      PD_TRACE_EXITRC( SDB__UTILSCHEMA_TOBSON, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__UTILSCHEMA_TOBSON_BUILDER, "_utilSchema::toBSON" )
   INT32 _utilSchema::toBSON( bson::BSONObjBuilder &builder, UINT32 mask ) const
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__UTILSCHEMA_TOBSON_BUILDER ) ;

      try
      {
         builder.append( FIELD_NAME_NAME, _name ) ;
         builder.append( FIELD_NAME_VERSION, _version ) ;

         if ( NULL != _collection )
         {
            builder.append( FIELD_NAME_COLLECTION, _collection ) ;
         }

         rc = _utilSchemaAttr::toBSON( builder, mask ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to build BSON for schema attr, "
                      "rc: %d", rc ) ;

         if ( OSS_BIT_TEST( mask, UTIL_SCHEMA_ATTR_MASK_COL_ALL ) )
         {
            rc = _columnsToBSON( builder ) ;
            PD_RC_CHECK( rc, PDERROR, "Failed to build BSON for columns, "
                         "rc: %d", rc ) ;
         }
      }
      catch ( exception &e )
      {
         PD_LOG( PDERROR, "Failed to build BSON for schema, "
                 "occur exception %s", e.what() ) ;
         rc = ossException2RC( &e ) ;
         goto error ;
      }

   done:
      PD_TRACE_EXITRC( SDB__UTILSCHEMA_TOBSON_BUILDER, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__UTILSCHEMA_COPY, "_utilSchema::copy" )
   INT32 _utilSchema::copy( const _utilSchema &schema )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__UTILSCHEMA_COPY ) ;

      if ( schema.isValid() )
      {
         BSONObj boSchema ;

         rc = schema.toBSON( boSchema ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to build BSON for schema, "
                      "rc: %d", rc ) ;

         rc = parse( boSchema, FALSE, TRUE ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to parse schema, rc: %d", rc ) ;
      }

   done:
      PD_TRACE_EXITRC( SDB__UTILSCHEMA_COPY, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__UTILSCHEMA__REBUILD, "_utilSchema::_rebuild" )
   INT32 _utilSchema::_rebuild()
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__UTILSCHEMA__REBUILD ) ;

      BSONObj boDefine ;

      rc = toBSON( boDefine ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to build define, rc: %d", rc ) ;

      rc = parse( boDefine, FALSE, FALSE ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to parse rebuild schema, "
                   "rc: %d", rc ) ;

   done:
      PD_TRACE_EXITRC( SDB__UTILSCHEMA__REBUILD, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__UTILSCHEMA_ADJUSTOID, "_utilSchema::adjustOID" )
   INT32 _utilSchema::adjustOID( BOOLEAN needRebuild )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__UTILSCHEMA_ADJUSTOID ) ;

      BOOLEAN hasAdjust = FALSE ;

      utilSchemaColumn *column = getColumn( FIELD_NAME_RECORD_OID ) ;
      if ( NULL != column )
      {
         column->adjustForOID() ;
         hasAdjust = TRUE ;
      }

      if ( needRebuild && hasAdjust )
      {
         rc = _rebuild() ;
         PD_RC_CHECK( rc, PDERROR, "Failed to rebuild adjusted schema, "
                      "rc: %d", rc ) ;
      }

   done:
      PD_TRACE_EXITRC( SDB__UTILSCHEMA_ADJUSTOID, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__UTILSCHEMA_ADJUST_KEY, "_utilSchema::adjustShardingKey" )
   INT32 _utilSchema::adjustShardingKey( const BSONObj &shardingKey,
                                         BOOLEAN needRebuild )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__UTILSCHEMA_ADJUST_KEY ) ;

      BOOLEAN hasAdjust = FALSE ;

      if ( !shardingKey.isEmpty() )
      {
         for ( UTIL_SCHEMA_COLUMN_MAP_IT iter = _columnMap.begin() ;
               iter != _columnMap.end() ;
               ++ iter )
         {
            BOOLEAN hasColumn = FALSE ;

            rc = _utilCheckKeyPattern( shardingKey,
                                       iter->first._pString,
                                       hasColumn ) ;
            PD_RC_CHECK( rc, PDERROR, "Failed to check key pattern, "
                         "rc: %d", rc ) ;

            if ( hasColumn )
            {
               iter->second->adjustForShardingKey() ;
               hasAdjust = TRUE ;
            }
         }
      }

      if ( needRebuild && hasAdjust )
      {
         rc = _rebuild() ;
         PD_RC_CHECK( rc, PDERROR, "Failed to rebuild adjusted schema, "
                      "rc: %d", rc ) ;
      }

   done:
      PD_TRACE_EXITRC( SDB__UTILSCHEMA_ADJUST_KEY, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__UTILSCHEMA_GETDEFAULTKEYS, "_utilSchema::getDefaultKeys" )
   INT32 _utilSchema::getDefaultKeys( const BSONObj &keyPattern,
                                      BSONObj &keys )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__UTILSCHEMA_GETDEFAULTKEYS ) ;

      try
      {
         if ( keyPattern.isEmpty() )
         {
            goto done ;
         }

         BSONObjBuilder builder ;
         BSONObjIterator iter ( keyPattern ) ;
         while ( iter.more() )
         {
            BSONElement ele = iter.next() ;
            const CHAR *fieldName = ele.fieldName() ;
            if ( NULL != ossStrchr( fieldName, '.' ) )
            {
               builder.appendUndefined( fieldName ) ;
            }
            else
            {
               utilSchemaColumn *column = getColumn( fieldName ) ;
               if ( NULL != column && column->hasWriteDefault() )
               {
                  builder.appendAs( column->getWriteDefault(), fieldName ) ;
               }
               else
               {
                  builder.appendUndefined( fieldName ) ;
               }
            }
         }

         keys = builder.obj() ;
      }
      catch ( exception &e )
      {
         PD_LOG( PDERROR, "Failed to get default keys, occur exception %s",
                 e.what() ) ;
         rc = ossException2RC( &e ) ;
         goto error ;
      }

   done:
      PD_TRACE_EXITRC( SDB__UTILSCHEMA_GETDEFAULTKEYS, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__UTILSCHEMA__PARSECOLUMNS, "_utilSchema::_parseColumns" )
   INT32 _utilSchema::_parseColumns( const BSONObj &boColumns, BOOLEAN fromUser )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__UTILSCHEMA__PARSECOLUMNS ) ;

      try
      {
         _columnList.clear() ;
         _columnMap.clear() ;

         _columnList.reserve( boColumns.nFields() ) ;

         BSONObjIterator iter( boColumns ) ;
         while ( iter.more() )
         {
            utilSchemaColumn column ;
            BSONElement beColumn = iter.next() ;
            const CHAR *columnName = beColumn.fieldName() ;
            BSONObj boColumn ;

            if ( fromUser )
            {
               rc = _utilCheckColumnName( columnName ) ;
               PD_RC_CHECK( rc, PDERROR, "Failed to check column name, rc: %d",
                            rc ) ;
            }

            PD_CHECK( Object == beColumn.type(), SDB_INVALIDARG, error, PDERROR,
                      "Failed to parse column [%s], it is not an object",
                      columnName ) ;
            boColumn = beColumn.embeddedObject() ;

            rc = column.parse( columnName, boColumn, fromUser, FALSE ) ;
            PD_RC_CHECK( rc, PDERROR, "Failed to parse column [%s], rc: %d",
                         beColumn.toPoolString().c_str(), rc ) ;

            rc = addColumn( column ) ;
            PD_RC_CHECK( rc, PDERROR, "Failed to add column [%s], rc: %d",
                         columnName, rc ) ;
         }
      }
      catch ( exception &e )
      {
         PD_LOG( PDERROR, "Failed parse schema columns, occur exception %s", e.what() ) ;
         rc = ossException2RC( &e ) ;
         goto error ;
      }

   done:
      PD_TRACE_EXITRC( SDB__UTILSCHEMA__PARSECOLUMNS, rc ) ;
      return rc ;

   error:
      _columnList.clear() ;
      _columnMap.clear() ;
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__UTILSCHEMA__COLUMNSTOBSON, "_utilSchema::_columnsToBSON" )
   INT32 _utilSchema::_columnsToBSON( BSONObjBuilder &builder ) const
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__UTILSCHEMA__COLUMNSTOBSON ) ;

      try
      {
         BSONObjBuilder subBuilder( builder.subobjStart( FIELD_NAME_COLUMNS ) ) ;
         for ( UTIL_SCHEMA_COLUMN_LIST_CIT iter = _columnList.begin() ;
               iter != _columnList.end() ;
               ++ iter )
         {
            const utilSchemaColumn &column = *iter ;
            rc = column.toBSON( subBuilder ) ;
            PD_RC_CHECK( rc, PDERROR, "Failed to build BSON for column [%s], "
                         "rc: %d", column.getName(), rc ) ;
         }
      }
      catch ( exception &e )
      {
         PD_LOG( PDERROR, "Failed to build BSON for columns, "
                 "occur exception %s", e.what() ) ;
         rc = ossException2RC( &e ) ;
         goto error ;
      }

   done:
      PD_TRACE_EXITRC( SDB__UTILSCHEMA__COLUMNSTOBSON, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__UTILSCHEMA_GETCOL, "_utilSchema::getColumn" )
   utilSchemaColumn *_utilSchema::getColumn( const CHAR *columnName )
   {
      utilSchemaColumn *result = NULL ;

      PD_TRACE_ENTRY( SDB__UTILSCHEMA_GETCOL ) ;

      UTIL_SCHEMA_COLUMN_MAP_IT iter = _columnMap.find( columnName ) ;
      if ( iter != _columnMap.end() )
      {
         result = iter->second ;
      }

      PD_TRACE_EXIT( SDB__UTILSCHEMA_GETCOL ) ;

      return result ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__UTILSCHEMA_GETCOL_CONST, "_utilSchema::getColumn" )
   const utilSchemaColumn *_utilSchema::getColumn( const CHAR *columnName ) const
   {
      const utilSchemaColumn *result = NULL ;

      PD_TRACE_ENTRY( SDB__UTILSCHEMA_GETCOL_CONST ) ;

      UTIL_SCHEMA_COLUMN_MAP_CIT iter = _columnMap.find( columnName ) ;
      if ( iter != _columnMap.end() )
      {
         result = iter->second ;
      }

      PD_TRACE_EXIT( SDB__UTILSCHEMA_GETCOL_CONST ) ;

      return result ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__UTILSCHEMA_HASCOL, "_utilSchema::hasColumn" )
   BOOLEAN _utilSchema::hasColumn( const CHAR *columnName ) const
   {
      BOOLEAN result = FALSE ;

      PD_TRACE_ENTRY( SDB__UTILSCHEMA_HASCOL ) ;

      result = ( _columnMap.count( columnName ) > 0 ) ;

      PD_TRACE_EXIT( SDB__UTILSCHEMA_HASCOL ) ;

      return result ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__UTILSCHEMA_ADDCOL, "_utilSchema::addColumn" )
   INT32 _utilSchema::addColumn( const utilSchemaColumn &column )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__UTILSCHEMA_ADDCOL ) ;

      try
      {
         PD_CHECK( 0 == _columnMap.count( column.getName() ),
                   SDB_INVALIDARG, error, PDERROR,
                   "Failed to parse column [%s], it is duplicated",
                   column.getName() ) ;

         _columnList.push_back( column ) ;
         {
            utilSchemaColumn &lastColumn = _columnList.back() ;
            _columnMap.insert(
                  UTIL_SCHEMA_COLUMN_MAP::value_type( lastColumn.getName(),
                                                      &lastColumn ) ) ;
         }
      }
      catch ( exception &e )
      {
         PD_LOG( PDERROR, "Failed to add column, occur exception %s",
                 e.what() ) ;
         rc = ossException2RC( &e ) ;
         goto error ;
      }

   done:
      PD_TRACE_EXITRC( SDB__UTILSCHEMA_ADDCOL, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__UTILSCHEMA_DROPCOL, "_utilSchema::dropColumn" )
   INT32 _utilSchema::dropColumn( const CHAR *columnName )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__UTILSCHEMA_DROPCOL ) ;

      try
      {
         _columnMap.erase( columnName ) ;
         for ( UTIL_SCHEMA_COLUMN_LIST_IT iter = _columnList.begin() ;
               iter != _columnList.end() ;
               ++ iter )
         {
            if ( 0 == ossStrcmp( iter->getName(), columnName ) )
            {
               _columnList.erase( iter ) ;
               break ;
            }
         }
      }
      catch ( exception &e )
      {
         PD_LOG( PDERROR, "Failed to drop column, occur exception %s",
                 e.what() ) ;
         rc = ossException2RC( &e ) ;
         goto error ;
      }

   done:
      PD_TRACE_EXITRC( SDB__UTILSCHEMA_DROPCOL, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__UTILSCHEMA_ALTERCOL, "_utilSchema::alterColumn" )
   INT32 _utilSchema::alterColumn( const CHAR *columnName,
                                   const utilSchemaColAttr &attr,
                                   UINT32 alterMask )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__UTILSCHEMA_ALTERCOL ) ;

      utilSchemaColumn *column = getColumn( columnName ) ;
      PD_CHECK( NULL != column, SDB_INVALIDARG, error, PDERROR,
                "Failed to get column [%s]", columnName ) ;

      if ( OSS_BIT_TEST( alterMask, UTIL_SCHEMA_ATTR_MASK_COL_TYPE ) )
      {
         column->setType( attr.getType() ) ;
      }
      if ( OSS_BIT_TEST( alterMask, UTIL_SCHEMA_ATTR_MASK_COL_NNULL ) )
      {
         column->setNotNull( attr.isNotNull() ) ;
      }
      if ( OSS_BIT_TEST( alterMask, UTIL_SCHEMA_ATTR_MASK_COL_NARRAY ) )
      {
         column->setNotArray( attr.isNotArray() ) ;
      }
      if ( OSS_BIT_TEST( alterMask, UTIL_SCHEMA_ATTR_MASK_COL_WDEF ) )
      {
         column->setWriteDefault( attr.getWriteDefault() ) ;
      }
      if ( OSS_BIT_TEST( alterMask, UTIL_SCHEMA_ATTR_MASK_COL_RDEF ) )
      {
         column->setReadDefault( attr.getReadDefault() ) ;
      }

   done:
      PD_TRACE_EXITRC( SDB__UTILSCHEMA_ALTERCOL, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__UTILSCHEMA_RENAMECOL, "_utilSchema::renameColumn" )
   INT32 _utilSchema::renameColumn( const CHAR *columnName,
                                    const CHAR *newColumnName )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__UTILSCHEMA_RENAMECOL ) ;

      try
      {
         utilSchemaColumn *column = getColumn( columnName ) ;
         PD_CHECK( NULL != column, SDB_INVALIDARG, error, PDERROR,
                   "Failed to get column [%s]", columnName ) ;

         _columnMap.erase( columnName ) ;

         column->setName( newColumnName ) ;
         _columnMap.insert(
               UTIL_SCHEMA_COLUMN_MAP::value_type( column->getName(),
                                                   column ) ) ;
      }
      catch ( exception &e )
      {
         PD_LOG( PDERROR, "Failed to rename column, occur exception %s",
                 e.what() ) ;
         rc = ossException2RC( &e ) ;
         goto error ;
      }

   done:
      PD_TRACE_EXITRC( SDB__UTILSCHEMA_RENAMECOL, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__UTILSCHEMA_DROPDEF, "_utilSchema::dropDefault" )
   INT32 _utilSchema::dropDefault( const CHAR *columnName )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__UTILSCHEMA_DROPDEF ) ;

      utilSchemaColumn *column = getColumn( columnName ) ;
      PD_CHECK( NULL != column, SDB_INVALIDARG, error, PDERROR,
                "Failed to get column [%s]", columnName ) ;

      column->setWriteDefault( BSONElement() ) ;
      if ( NULL == _collection )
      {
         column->setReadDefault( BSONElement() ) ;
      }

   done:
      PD_TRACE_EXITRC( SDB__UTILSCHEMA_DROPDEF, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__UTILSCHEMA_SETATTR, "_utilSchema::setAttributes" )
   INT32 _utilSchema::setAttributes( const utilSchemaAttr &attr,
                                     UINT32 alterMask )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__UTILSCHEMA_SETATTR ) ;

      if ( OSS_BIT_TEST( alterMask, UTIL_SCHEMA_ATTR_MASK_STRICTMODE ) )
      {
         setStrictMode( attr.isStrictMode() ) ;
      }

      PD_TRACE_EXITRC( SDB__UTILSCHEMA_SETATTR, rc ) ;

      return rc ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__UTILSCHEMA_CHECKDEFAULTKEYS, "_utilSchema::checkDefaultKeys" )
   INT32 _utilSchema::checkDefaultKeys( const bson::BSONObj &keyPattern,
                                        BOOLEAN checkWriteDefault,
                                        BOOLEAN checkReadDefault,
                                        const CHAR *&conflictColumnName,
                                        const _utilSchema *oldSchema ) const
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__UTILSCHEMA_CHECKDEFAULTKEYS ) ;

      try
      {
         BSONObjIterator iter( keyPattern ) ;
         while ( iter.more() )
         {
            BSONElement ele = iter.next() ;
            const CHAR *name = ele.fieldName() ;
            const CHAR *p = ossStrchr( name, '.' ) ;
            ossPoolString tmpName ;
            if ( NULL != p )
            {
               tmpName.assign( name, p - name ) ;
               name = tmpName.c_str() ;
            }
            const utilSchemaColumn *column = getColumn( name ) ;
            const utilSchemaColumn *oldColumn = NULL ;
            if ( NULL != column )
            {
               if ( checkWriteDefault && column->hasWriteDefault() )
               {
                  if ( NULL != oldSchema )
                  {
                     oldColumn = oldSchema->getColumn( name ) ;
                     if ( ( NULL == oldColumn ) ||
                          ( !( oldColumn->hasWriteDefault() ) ) ||
                          ( !( oldColumn->hasSameWriteDefault( *column ) ) ) )
                     {
                        conflictColumnName = column->getName() ;
                        break ;
                     }
                  }
                  else
                  {
                     conflictColumnName = column->getName() ;
                     break ;
                  }
               }
               if ( checkReadDefault && column->hasReadDefault() )
               {
                  if ( NULL != oldSchema )
                  {
                     if ( NULL == oldColumn )
                     {
                        oldColumn = oldSchema->getColumn( name ) ;
                     }
                     if ( ( NULL == oldColumn ) ||
                          ( !( oldColumn->hasReadDefault() ) ) )
                     {
                        conflictColumnName = column->getName() ;
                        break ;
                     }
                  }
                  else
                  {
                     conflictColumnName = column->getName() ;
                     break ;
                  }
               }
            }
         }
      }
      catch ( exception &e )
      {
         PD_LOG( PDERROR, "Failed to check default keys, "
                 "occur exception %s", e.what() ) ;
         rc = ossException2RC( &e ) ;
         goto error ;
      }

   done:
      PD_TRACE_EXITRC( SDB__UTILSCHEMA_CHECKDEFAULTKEYS, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   /*
      alter schema helpers
    */
   static UTIL_SCHEMA_ALTER_ACTION _utilGetSchemaAlterAction( const CHAR *actionName )
   {
      if ( NULL != actionName )
      {
         if ( 0 == ossStrcmp( actionName,
                              CMD_VALUE_NAME_SCHEMA_ADD_COLUMN ) )
         {
            return UTIL_SCHEMA_ADD_COLUMN ;
         }
         else if ( 0 == ossStrcmp( actionName,
                                   CMD_VALUE_NAME_SCHEMA_DROP_COLUMN ) )
         {
            return UTIL_SCHEMA_DROP_COLUMN ;
         }
         else if ( 0 == ossStrcmp( actionName,
                                   CMD_VALUE_NAME_SCHEMA_ALTER_COLUMN ) )
         {
            return UTIL_SCHEMA_ALTER_COLUMN ;
         }
         else if ( 0 == ossStrcmp( actionName,
                                   CMD_VALUE_NAME_SCHEMA_RENAME_COLUMN ) )
         {
            return UTIL_SCHEMA_RENAME_COLUMN ;
         }
         else if ( 0 == ossStrcmp( actionName,
                                   CMD_VALUE_NAME_SCHEMA_DROP_DEFAULT ) )
         {
            return UTIL_SCHEMA_DROP_DEFAULT ;
         }
         else if ( 0 == ossStrcmp( actionName,
                                   CMD_VALUE_NAME_SCHEMA_SET_ATTRIBUTES ) )
         {
            return UTIL_SCHEMA_SET_ATTRIBUTES ;
         }
      }
      return UTIL_SCHEMA_ACTION_UNKNOWN ;
   }

   static const CHAR *_utilGetSchemaAlterActionName( UTIL_SCHEMA_ALTER_ACTION actionType )
   {
      switch ( actionType )
      {
         case UTIL_SCHEMA_ADD_COLUMN :
            return CMD_VALUE_NAME_SCHEMA_ADD_COLUMN ;
         case UTIL_SCHEMA_DROP_COLUMN :
            return CMD_VALUE_NAME_SCHEMA_DROP_COLUMN ;
         case UTIL_SCHEMA_ALTER_COLUMN :
            return CMD_VALUE_NAME_SCHEMA_ALTER_COLUMN ;
         case UTIL_SCHEMA_RENAME_COLUMN :
            return CMD_VALUE_NAME_SCHEMA_RENAME_COLUMN ;
         case UTIL_SCHEMA_DROP_DEFAULT :
            return CMD_VALUE_NAME_SCHEMA_DROP_DEFAULT ;
         case UTIL_SCHEMA_SET_ATTRIBUTES :
            return CMD_VALUE_NAME_SCHEMA_SET_ATTRIBUTES ;
         default:
            break ;
      }
      return "unknown" ;
   }

   /*
      _utilSchemaAlterAction implement
    */
   _utilSchemaAlterAction::_utilSchemaAlterAction()
   : _schemaName( NULL ),
     _action( UTIL_SCHEMA_ACTION_UNKNOWN ),
     _boAction(),
     _colName( NULL ),
     _newColAttr(),
     _newSchemaAttr(),
     _alterMask( 0 )
   {
   }

   _utilSchemaAlterAction::~_utilSchemaAlterAction()
   {
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__UTILSCHEMAALTERACTION_PARSE, "_utilSchemaAlterAction::parse" )
   INT32 _utilSchemaAlterAction::parse( const BSONObj &boAction )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__UTILSCHEMAALTERACTION_PARSE ) ;

      try
      {
         BSONObj options ;
         const CHAR *actionName = NULL ;

         _boAction = boAction.getOwned() ;

         PD_LOG( PDDEBUG, "Got action [%s]", _boAction.toPoolString().c_str() ) ;

         BSONElement ele = _boAction.getField( FIELD_NAME_NAME ) ;
         PD_CHECK( String == ele.type(), SDB_INVALIDARG, error, PDERROR,
                   "Failed to get field [%s], it is not a string",
                   FIELD_NAME_NAME ) ;
         _schemaName = ele.valuestr() ;

         ele = _boAction.getField( FIELD_NAME_ACTION ) ;
         PD_CHECK( String == ele.type(), SDB_INVALIDARG, error, PDERROR,
                   "Failed to get field [%s], it is not a string",
                   FIELD_NAME_ACTION ) ;
         actionName = ele.valuestr() ;
         _action = _utilGetSchemaAlterAction( actionName ) ;

         ele = _boAction.getField( FIELD_NAME_OPTIONS ) ;
         PD_CHECK( Object == ele.type(), SDB_INVALIDARG, error, PDERROR,
                   "Failed to get field [%s], it is not an object",
                   FIELD_NAME_OPTIONS ) ;
         options = ele.embeddedObject() ;

         switch ( _action )
         {
            case UTIL_SCHEMA_ADD_COLUMN :
            {
               rc = _parseAddColumn( options ) ;
               PD_RC_CHECK( rc, PDERROR, "Failed to parse add column action, "
                            "rc: %d", rc ) ;
               break ;
            }
            case UTIL_SCHEMA_DROP_COLUMN :
            {
               rc = _parseDropColumn( options ) ;
               PD_RC_CHECK( rc, PDERROR, "Failed to parse drop column action, "
                            "rc: %d", rc ) ;
               break ;
            }
            case UTIL_SCHEMA_ALTER_COLUMN :
            {
               rc = _parseAlterColumn( options ) ;
               PD_RC_CHECK( rc, PDERROR, "Failed to parse alter column action, "
                            "rc: %d", rc ) ;
               break ;
            }
            case UTIL_SCHEMA_RENAME_COLUMN :
            {
               rc = _parseRenameColumn( options ) ;
               PD_RC_CHECK( rc, PDERROR, "Failed to parse rename column action, "
                            "rc: %d", rc ) ;
               break ;
            }
            case UTIL_SCHEMA_DROP_DEFAULT :
            {
               rc = _parseDropDefault( options ) ;
               PD_RC_CHECK( rc, PDERROR, "Failed to parse drop default action, "
                            "rc: %d", rc ) ;
               break ;
            }
            case UTIL_SCHEMA_SET_ATTRIBUTES :
            {
               rc = _parseSetAttributes( options ) ;
               PD_RC_CHECK( rc, PDERROR, "Failed to parse set attributes action, "
                            "rc: %d", rc ) ;
               break ;
            }
            default:
            {
               rc = SDB_INVALIDARG ;
               PD_LOG( PDERROR, "Failed to parse alter schema action, "
                       "action is unknown [%s]", ele.valuestr() ) ;
               goto error ;
            }
         }
      }
      catch ( exception &e )
      {
         PD_LOG( PDERROR, "Failed to parse alter schema action, "
                 "occur exception %s", e.what() ) ;
         rc = ossException2RC( &e ) ;
         goto error ;
      }

      PD_LOG( PDDEBUG, "Got alter schema [%s], action [%s/%s], mask [%u]",
              _schemaName, _utilGetSchemaAlterActionName( _action ),
              _boAction.toPoolString().c_str(), _alterMask ) ;

   done:
      PD_TRACE_EXITRC( SDB__UTILSCHEMAALTERACTION_PARSE, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__UTILSCHEMAALTERACTION__PARSEADDCOL, "_utilSchemaAlterAction::_parseAddColumn" )
   INT32 _utilSchemaAlterAction::_parseAddColumn( const BSONObj &options )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__UTILSCHEMAALTERACTION__PARSEADDCOL ) ;

      try
      {
         BSONElement ele ;
         BSONObj boDefine ;

         UINT32 parsedMask = 0 ;

         PD_CHECK( 1 == options.nFields(), SDB_INVALIDARG, error, PDERROR,
                   "Failed to parse add column options, "
                   "should be only one column" ) ;

         _colName = options.firstElementFieldName() ;

         rc = _utilCheckColumnName( _colName ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to check column name, rc: %d", rc ) ;

         ele = options.firstElement() ;
         PD_CHECK( Object == ele.type(), SDB_INVALIDARG, error, PDERROR,
                   "Failed to get column attributes, it is not an object" ) ;
         boDefine = ele.embeddedObject() ;

         rc = _newColAttr.parse( _colName, boDefine, TRUE, TRUE, parsedMask ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to parse new column attributes, "
                      "rc: %d", rc ) ;
         _boColDefine = boDefine ;

         PD_CHECK( OSS_BIT_TEST( parsedMask, UTIL_SCHEMA_ATTR_MASK_COL_TYPE ),
                   SDB_INVALIDARG, error, PDERROR,
                   "Failed to parse attributes of column [%s], unknown type",
                   _colName ) ;

         OSS_BIT_SET( _alterMask, UTIL_SCHEMA_ATTR_MASK_COL_NAME ) ;
         OSS_BIT_SET( _alterMask, parsedMask ) ;
      }
      catch ( exception &e )
      {
         PD_LOG( PDERROR, "Failed to parse add column action, "
                 "occur exception %s", e.what() ) ;
         rc = ossException2RC( &e ) ;
         goto error ;
      }

   done:
      PD_TRACE_EXITRC( SDB__UTILSCHEMAALTERACTION__PARSEADDCOL, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__UTILSCHEMAALTERACTION__PARSEDROPCOL, "_utilSchemaAlterAction::_parseDropColumn" )
   INT32 _utilSchemaAlterAction::_parseDropColumn( const BSONObj &options )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__UTILSCHEMAALTERACTION__PARSEDROPCOL ) ;

      try
      {
         PD_CHECK( 1 == options.nFields(), SDB_INVALIDARG, error, PDERROR,
                   "Failed to parse drop column options, "
                   "should be only one column" ) ;

         _colName = options.firstElementFieldName() ;
         OSS_BIT_SET( _alterMask, UTIL_SCHEMA_ATTR_MASK_COL_ALL ) ;
      }
      catch ( exception &e )
      {
         PD_LOG( PDERROR, "Failed to parse drop column action, "
                 "occur exception %s", e.what() ) ;
         rc = ossException2RC( &e ) ;
         goto error ;
      }

   done:
      PD_TRACE_EXITRC( SDB__UTILSCHEMAALTERACTION__PARSEDROPCOL, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__UTILSCHEMAALTERACTION__PARSEALTERCOL, "_utilSchemaAlterAction::_parseAlterColumn" )
   INT32 _utilSchemaAlterAction::_parseAlterColumn( const BSONObj &options )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__UTILSCHEMAALTERACTION__PARSEALTERCOL ) ;

      try
      {
         BSONElement ele ;
         BSONObj boDefine ;

         PD_CHECK( 1 == options.nFields(), SDB_INVALIDARG, error, PDERROR,
                   "Failed to parse alter column options, "
                   "should be only one column" ) ;

         ele = options.firstElement() ;
         PD_CHECK( Object == ele.type(), SDB_INVALIDARG, error, PDERROR,
                   "Failed to get alter column attributes, "
                   "it is not an object" ) ;
         boDefine = ele.embeddedObject() ;

         _colName = options.firstElementFieldName() ;

         rc = _newColAttr.parse( _colName, boDefine, TRUE, FALSE, _alterMask ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to parse new column attributes, "
                      "rc: %d", rc ) ;

         _boColDefine = boDefine ;
      }
      catch ( exception &e )
      {
         PD_LOG( PDERROR, "Failed to parse alter column action, "
                 "occur exception %s", e.what() ) ;
         rc = ossException2RC( &e ) ;
         goto error ;
      }

   done:
      PD_TRACE_EXITRC( SDB__UTILSCHEMAALTERACTION__PARSEALTERCOL, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__UTILSCHEMAALTERACTION__PARSERENAMECOL, "_utilSchemaAlterAction::_parseRenameColumn" )
   INT32 _utilSchemaAlterAction::_parseRenameColumn( const BSONObj &options )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__UTILSCHEMAALTERACTION__PARSERENAMECOL ) ;

      try
      {
         const CHAR *oldColName = NULL ;
         const CHAR *newColName = NULL ;
         BSONElement ele ;
         BSONObj boDefine ;

         PD_CHECK( 1 == options.nFields(), SDB_INVALIDARG, error, PDERROR,
                   "Failed to parse rename column options, "
                   "should be only one column" ) ;
         oldColName = options.firstElementFieldName() ;
         PD_LOG_MSG_CHECK( 0 != ossStrcmp( oldColName, FIELD_NAME_RECORD_OID ),
                           SDB_INVALIDARG, error, PDERROR,
                           "Can not rename \"_id\" column" ) ;
         _colName = oldColName ;

         ele = options.firstElement() ;
         PD_CHECK( String == ele.type(), SDB_INVALIDARG, error, PDERROR,
                   "Failed to get new column name, it is not a string" ) ;
         newColName = ele.valuestr() ;
         PD_LOG_MSG_CHECK( 0 != ossStrcmp( newColName, FIELD_NAME_RECORD_OID ),
                           SDB_INVALIDARG, error, PDERROR,
                           "Can not rename to \"_id\" column" ) ;
         rc = _utilCheckColumnName( newColName ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to check column name, rc: %d", rc ) ;
         _newColAttr.setName( newColName ) ;
         OSS_BIT_SET( _alterMask, UTIL_SCHEMA_ATTR_MASK_COL_NAME ) ;
      }
      catch ( exception &e )
      {
         PD_LOG( PDERROR, "Failed to parse rename column action, "
                 "occur exception %s", e.what() ) ;
         rc = ossException2RC( &e ) ;
         goto error ;
      }

   done:
      PD_TRACE_EXITRC( SDB__UTILSCHEMAALTERACTION__PARSERENAMECOL, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__UTILSCHEMAALTERACTION__PARSEDROPDEF, "_utilSchemaAlterAction::_parseDropDefault" )
   INT32 _utilSchemaAlterAction::_parseDropDefault( const BSONObj &options )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__UTILSCHEMAALTERACTION__PARSEDROPDEF ) ;

      try
      {
         PD_CHECK( 1 == options.nFields(), SDB_INVALIDARG, error, PDERROR,
                   "Failed to parse drop default options, "
                   "should be only one column" ) ;

         _colName = options.firstElementFieldName() ;
         OSS_BIT_SET( _alterMask, UTIL_SCHEMA_ATTR_MASK_COL_WDEF ) ;
         OSS_BIT_SET( _alterMask, UTIL_SCHEMA_ATTR_MASK_COL_RDEF ) ;
      }
      catch ( exception &e )
      {
         PD_LOG( PDERROR, "Failed to parse drop default action, "
                 "occur exception %s", e.what() ) ;
         rc = ossException2RC( &e ) ;
         goto error ;
      }

   done:
      PD_TRACE_EXITRC( SDB__UTILSCHEMAALTERACTION__PARSEDROPDEF, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__UTILSCHEMAALTERACTION__PARSESETATTR, "_utilSchemaAlterAction::_parseSetAttributes" )
   INT32 _utilSchemaAlterAction::_parseSetAttributes( const BSONObj &options )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__UTILSCHEMAALTERACTION__PARSESETATTR ) ;

      rc = _newSchemaAttr.parse( options, TRUE, _alterMask ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to parse set attributes options, "
                   "rc: %d", rc ) ;

   done:
      PD_TRACE_EXITRC( SDB__UTILSCHEMAALTERACTION__PARSESETATTR, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__UTILSCHEMAALTERACTION_TOBSON, "_utilSchemaAlterAction::toBSON" )
   INT32 _utilSchemaAlterAction::toBSON( BSONObj &boAction ) const
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__UTILSCHEMAALTERACTION_TOBSON ) ;

      try
      {
         BSONObjBuilder builder ;
         rc = toBSON( builder ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to build BSON for alter "
                      "schema action, rc: %d", rc ) ;
         boAction = builder.obj() ;
      }
      catch ( exception &e )
      {
         PD_LOG( PDERROR, "Failed to build BSON for alter schema action, "
                 "occur exception %s", e.what() ) ;
         rc = ossException2RC( &e ) ;
         goto error ;
      }

   done:
      PD_TRACE_EXITRC( SDB__UTILSCHEMAALTERACTION_TOBSON, rc ) ;
      return rc ;

   error:
      goto done ;
   }
   // PD_TRACE_DECLARE_FUNCTION ( SDB__UTILSCHEMAALTERACTION_TOBSON_BLD, "_utilSchemaAlterAction::toBSON" )
   INT32 _utilSchemaAlterAction::toBSON( BSONObjBuilder &builder ) const
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__UTILSCHEMAALTERACTION_TOBSON ) ;

      try
      {
         builder.append( FIELD_NAME_NAME,
                         _schemaName ) ;
         builder.append( FIELD_NAME_ACTION,
                         _utilGetSchemaAlterActionName( _action ) ) ;

         BSONObjBuilder optionBuilder( builder.subobjStart( FIELD_NAME_OPTIONS ) ) ;

         switch ( _action )
         {
            case UTIL_SCHEMA_ADD_COLUMN :
            {
               rc = _toBSONAddColumn( optionBuilder ) ;
               PD_RC_CHECK( rc, PDERROR, "Failed to build BSON for "
                            "add column action, rc: %d", rc ) ;
               break ;
            }
            case UTIL_SCHEMA_DROP_COLUMN :
            {
               rc = _toBSONDropColumn( optionBuilder ) ;
               PD_RC_CHECK( rc, PDERROR, "Failed to build BSON for "
                            "drop column action, rc: %d", rc ) ;
               break ;
            }
            case UTIL_SCHEMA_ALTER_COLUMN :
            {
               rc = _toBSONAlterColumn( optionBuilder ) ;
               PD_RC_CHECK( rc, PDERROR, "Failed to build BSON for "
                            "alter column action, rc: %d", rc ) ;
               break ;
            }
            case UTIL_SCHEMA_RENAME_COLUMN :
            {
               rc = _toBSONRenameColumn( optionBuilder ) ;
               PD_RC_CHECK( rc, PDERROR, "Failed to build BSON for "
                            "rename column action, rc: %d", rc ) ;
               break ;
            }
            case UTIL_SCHEMA_DROP_DEFAULT :
            {
               rc = _toBSONDropDefault( optionBuilder ) ;
               PD_RC_CHECK( rc, PDERROR, "Failed to build BSON for "
                            "drop default action, rc: %d", rc ) ;
               break ;
            }
            case UTIL_SCHEMA_SET_ATTRIBUTES :
            {
               rc = _toBSONSetAttributes( optionBuilder ) ;
               PD_RC_CHECK( rc, PDERROR, "Failed to build BSON for "
                            "set attributes action, rc: %d", rc ) ;
               break ;
            }
            default:
            {
               PD_LOG( PDERROR, "Failed to build BSON for alter action, "
                       "unknown action type [%d]", _action ) ;
               SDB_ASSERT( FALSE, "invalid action type" ) ;
               rc = SDB_SYS ;
               goto error ;
            }
         }
         optionBuilder.doneFast() ;
      }
      catch ( exception &e )
      {
         PD_LOG( PDERROR, "Failed to build BSON for action, "
                 "occur exception %s", e.what() ) ;
         rc = ossException2RC( &e ) ;
         goto error ;
      }

   done:
      PD_TRACE_EXITRC( SDB__UTILSCHEMAALTERACTION_TOBSON, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__UTILSCHEMAALTERACTION__TOBSONADDCOL, "_utilSchemaAlterAction::_toBSONAddColumn" )
   INT32 _utilSchemaAlterAction::_toBSONAddColumn( BSONObjBuilder &optionBuilder ) const
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__UTILSCHEMAALTERACTION__TOBSONADDCOL ) ;

      try
      {
         BSONObjBuilder builder( optionBuilder.subobjStart( _colName ) ) ;

         rc = _newColAttr.toBSON( builder, _alterMask ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to build BSON for attr, "
                      "rc: %d", rc ) ;

         builder.doneFast() ;
      }
      catch ( exception &e )
      {
         PD_LOG( PDERROR, "Failed to build BSON, occur exception %s",
                 e.what() ) ;
         rc = ossException2RC( &e ) ;
         goto error ;
      }

   done:
      PD_TRACE_EXITRC( SDB__UTILSCHEMAALTERACTION__TOBSONADDCOL, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__UTILSCHEMAALTERACTION__TOBSONDROPCOL, "_utilSchemaAlterAction::_toBSONDropColumn" )
   INT32 _utilSchemaAlterAction::_toBSONDropColumn( BSONObjBuilder &optionBuilder ) const
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__UTILSCHEMAALTERACTION__TOBSONDROPCOL ) ;

      try
      {
         optionBuilder.append( _colName, 1 ) ;
      }
      catch ( exception &e )
      {
         PD_LOG( PDERROR, "Failed to build BSON, occur exception %s",
                 e.what() ) ;
         rc = ossException2RC( &e ) ;
         goto error ;
      }

   done:
      PD_TRACE_EXITRC( SDB__UTILSCHEMAALTERACTION__TOBSONDROPCOL, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__UTILSCHEMAALTERACTION__TOBSONALTERCOL, "_utilSchemaAlterAction::_toBSONAlterColumn" )
   INT32 _utilSchemaAlterAction::_toBSONAlterColumn( BSONObjBuilder &optionBuilder ) const
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__UTILSCHEMAALTERACTION__TOBSONALTERCOL ) ;

      try
      {
         BSONObjBuilder builder( optionBuilder.subobjStart( _colName ) ) ;

         rc = _newColAttr.toBSON( builder, _alterMask ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to build BSON for attr, "
                      "rc: %d", rc ) ;

         builder.doneFast() ;
      }
      catch ( exception &e )
      {
         PD_LOG( PDERROR, "Failed to build BSON, occur exception %s",
                 e.what() ) ;
         rc = ossException2RC( &e ) ;
         goto error ;
      }

   done:
      PD_TRACE_EXITRC( SDB__UTILSCHEMAALTERACTION__TOBSONALTERCOL, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__UTILSCHEMAALTERACTION__TOBSONRENAMECOL, "_utilSchemaAlterAction::_toBSONRenameColumn" )
   INT32 _utilSchemaAlterAction::_toBSONRenameColumn( BSONObjBuilder &optionBuilder ) const
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__UTILSCHEMAALTERACTION__TOBSONRENAMECOL ) ;

      try
      {
         optionBuilder.append( _colName, _newColAttr.getName() ) ;
      }
      catch ( exception &e )
      {
         PD_LOG( PDERROR, "Failed to build BSON, occur exception %s",
                 e.what() ) ;
         rc = ossException2RC( &e ) ;
         goto error ;
      }

   done:
      PD_TRACE_EXITRC( SDB__UTILSCHEMAALTERACTION__TOBSONRENAMECOL, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__UTILSCHEMAALTERACTION__TOBSONDROPDEF, "_utilSchemaAlterAction::_toBSONDropDefault" )
   INT32 _utilSchemaAlterAction::_toBSONDropDefault( BSONObjBuilder &optionBuilder ) const
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__UTILSCHEMAALTERACTION__TOBSONDROPDEF ) ;

      try
      {
         optionBuilder.append( _colName, 1 ) ;
      }
      catch ( exception &e )
      {
         PD_LOG( PDERROR, "Failed to build BSON, occur exception %s",
                 e.what() ) ;
         rc = ossException2RC( &e ) ;
         goto error ;
      }

   done:
      PD_TRACE_EXITRC( SDB__UTILSCHEMAALTERACTION__TOBSONDROPDEF, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__UTILSCHEMAALTERACTION__TOBSONSETATTR, "_utilSchemaAlterAction::_toBSONSetAttributes" )
   INT32 _utilSchemaAlterAction::_toBSONSetAttributes( BSONObjBuilder &optionBuilder ) const
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__UTILSCHEMAALTERACTION__TOBSONSETATTR ) ;

      try
      {
         rc = _newSchemaAttr.toBSON( optionBuilder, _alterMask ) ;
      }
      catch ( exception &e )
      {
         PD_LOG( PDERROR, "Failed to build BSON, occur exception %s",
                 e.what() ) ;
         rc = ossException2RC( &e ) ;
         goto error ;
      }

   done:
      PD_TRACE_EXITRC( SDB__UTILSCHEMAALTERACTION__TOBSONSETATTR, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__UTILSCHEMAALTERACTION_COPY, "_utilSchemaAlterAction::copy" )
   INT32 _utilSchemaAlterAction::copy( const _utilSchemaAlterAction &action )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__UTILSCHEMAALTERACTION_COPY ) ;

      if ( action.isValid() )
      {
         BSONObj boAction ;

         rc = action.toBSON( boAction ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to build BSON for alter action, "
                      "rc: %d", rc ) ;

         rc = parse( boAction ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to parse alter action, rc: %d", rc ) ;
      }

   done:
      PD_TRACE_EXITRC( SDB__UTILSCHEMAALTERACTION_COPY, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__UTILSCHEMAALTERACTION_ADJUST, "_utilSchemaAlterAction::adjust" )
   INT32 _utilSchemaAlterAction::adjust( const bson::BSONObj &shardingKey )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__UTILSCHEMAALTERACTION_ADJUST ) ;

      switch ( _action )
      {
         case UTIL_SCHEMA_ADD_COLUMN:
         case UTIL_SCHEMA_ALTER_COLUMN:
         {
            if ( 0 == ossStrcmp( _colName, FIELD_NAME_RECORD_OID ) )
            {
               _newColAttr.adjustForOID() ;
            }
            if ( !shardingKey.isEmpty() )
            {
               BOOLEAN hasColumn = FALSE ;
               rc = checkKeyPattern( shardingKey, _colName, hasColumn ) ;
               PD_RC_CHECK( rc, PDERROR, "Failed to check sharding key, "
                            "rc: %d", rc ) ;
               if ( hasColumn )
               {
                  _newColAttr.adjustForShardingKey() ;
               }
            }
            break ;
         }
         default :
            break ;
      }

   done:
      PD_TRACE_EXITRC( SDB__UTILSCHEMAALTERACTION_ADJUST, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__UTILSCHEMAALTERACTION_CHKSCHEMA, "_utilSchemaAlterAction::checkSchema" )
   INT32 _utilSchemaAlterAction::checkSchema( const utilSchema &schema ) const
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__UTILSCHEMAALTERACTION_CHKSCHEMA ) ;

      switch ( _action )
      {
         case UTIL_SCHEMA_ADD_COLUMN :
         {
            PD_LOG_MSG_CHECK( !schema.hasColumn( _colName ),
                              SDB_INVALIDARG, error, PDERROR,
                              "Failed to add column [%s], it is duplicated",
                              _colName ) ;
            break ;
         }
         case UTIL_SCHEMA_DROP_COLUMN :
         {
            // to allow retry drop column, do not check if column exists
            break ;
         }
         case UTIL_SCHEMA_ALTER_COLUMN :
         {
            // const utilSchemaColumn *column = NULL ;

            PD_LOG_MSG_CHECK( schema.hasColumn( _colName ),
                              SDB_INVALIDARG, error, PDERROR,
                              "Failed to alter column [%s], it does not exist",
                              _colName ) ;

            if ( NULL != schema.getCollection() )
            {
               PD_CHECK( 0 == OSS_BIT_TEST( UTIL_SCHEMA_ATTR_MASK_COL_RDEF,
                                            _alterMask ),
                         SDB_OPERATION_INCOMPATIBLE, error, PDERROR,
                         "Failed to alter column [%s] on schema [%s] with "
                         "collection [%s], alter [%s] is not allowed",
                         _colName, _schemaName, schema.getCollection(),
                         FIELD_NAME_READDEFAULT ) ;
            }

//            column = schema.getColumn( _colName ) ;
//
//            if ( ( OSS_BIT_TEST( _alterMask,
//                                 UTIL_SCHEMA_ATTR_MASK_COL_TYPE ) ) &&
//                 ( EOO != _newColAttr.getType() ) )
//            {
//               // going to change type of column, check if old write and
//               // read defaults match the new type
//               if ( ( !OSS_BIT_TEST( _alterMask,
//                                     UTIL_SCHEMA_ATTR_MASK_COL_WDEF ) ) &&
//                    ( column->hasWriteDefault() ) )
//               {
//                  PD_LOG_MSG_CHECK(
//                        _newColAttr.getType() == column->getWriteDefaultType(),
//                        SDB_OPERATION_INCOMPATIBLE, error, PDERROR,
//                        "Failed to alter column [%s] on schema [%s] with "
//                        "collection [%s], type of old write default value [%s] "
//                        "is not matched new type [%s]",
//                        _colName, _schemaName, schema.getCollection(),
//                        column->getWriteDefaultTypeName(),
//                        _newColAttr.getTypeName() ) ;
//               }
//               if ( ( !OSS_BIT_TEST( _alterMask,
//                                     UTIL_SCHEMA_ATTR_MASK_COL_RDEF ) ) &&
//                    ( column->hasReadDefault() ) &&
//                    ( NULL == schema.getCollection() ) )
//               {
//                  PD_LOG_MSG_CHECK(
//                        ( ( _newColAttr.getType() == column->getReadDefaultType() ) ||
//                          ( Undefined == column->getReadDefaultType() ) ||
//                          ( jstNULL == column->getReadDefaultType() ) ),
//                        SDB_OPERATION_INCOMPATIBLE, error, PDERROR,
//                        "Failed to alter column [%s] on schema [%s] with "
//                        "collection [%s], type of old read default value [%s] "
//                        "is not matched new type [%s]",
//                        _colName, _schemaName, schema.getCollection(),
//                        column->getReadDefaultTypeName(),
//                        _newColAttr.getTypeName() ) ;
//               }
//            }
//            else if ( EOO != column->getType() )
//            {
//               // going to change write/read defaults, check if match the
//               // old type
//               if ( ( OSS_BIT_TEST( _alterMask,
//                                    UTIL_SCHEMA_ATTR_MASK_COL_WDEF ) ) &&
//                    ( _newColAttr.hasWriteDefault() ) )
//               {
//                  PD_LOG_MSG_CHECK(
//                        ( ( _newColAttr.getWriteDefaultType() == column->getType() ) ||
//                          ( Undefined == _newColAttr.getWriteDefaultType() ) ||
//                          ( jstNULL == _newColAttr.getWriteDefaultType() ) ),
//                        SDB_OPERATION_INCOMPATIBLE, error, PDERROR,
//                        "Failed to alter column [%s] on schema [%s] with "
//                        "collection [%s], type of new write default value [%s] "
//                        "is not matched old type [%s]",
//                        _colName, _schemaName, schema.getCollection(),
//                        _newColAttr.getWriteDefaultTypeName(),
//                        column->getTypeName() ) ;
//               }
//               if ( ( OSS_BIT_TEST( _alterMask,
//                                    UTIL_SCHEMA_ATTR_MASK_COL_RDEF ) ) &&
//                    ( _newColAttr.hasReadDefault() ) )
//               {
//                  PD_LOG_MSG_CHECK(
//                        _newColAttr.getReadDefaultType() == column->getType(),
//                        SDB_OPERATION_INCOMPATIBLE, error, PDERROR,
//                        "Failed to alter column [%s] on schema [%s] with "
//                        "collection [%s], type of new read default value [%s] "
//                        "is not matched old type [%s]",
//                        _colName, _schemaName, schema.getCollection(),
//                        _newColAttr.getReadDefaultTypeName(),
//                        column->getTypeName() ) ;
//               }
//            }
            break ;
         }
         case UTIL_SCHEMA_RENAME_COLUMN :
         {
            PD_LOG_MSG_CHECK( schema.hasColumn( _colName ),
                              SDB_INVALIDARG, error, PDERROR,
                              "Failed to rename column from [%s], "
                              "it does not exist", _colName ) ;
            PD_LOG_MSG_CHECK( !schema.hasColumn( _newColAttr.getName() ),
                              SDB_INVALIDARG, error, PDERROR,
                              "Failed to rename column to [%s], "
                              "it is duplicated", _newColAttr.getName() ) ;
            break ;
         }
         case UTIL_SCHEMA_DROP_DEFAULT :
         {
            PD_LOG_MSG_CHECK( schema.hasColumn( _colName ),
                              SDB_INVALIDARG, error, PDERROR,
                              "Failed to drop default of column [%s], "
                              "it does not exist", _colName ) ;
            break ;
         }
         case UTIL_SCHEMA_SET_ATTRIBUTES :
         {
            break ;
         }
         default:
         {
            rc = SDB_SYS ;
            PD_LOG( PDERROR, "Failed to apply alter action on schema, "
                    "unknown action" ) ;
            SDB_ASSERT( FALSE, "should not be here" ) ;
            goto error ;
         }
      }

   done:
      PD_TRACE_EXITRC( SDB__UTILSCHEMAALTERACTION_CHKSCHEMA, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__UTILSCHEMAALTERACTION_APPLYSCHEMA, "_utilSchemaAlterAction::applySchema" )
   INT32 _utilSchemaAlterAction::applySchema( utilSchema &schema ) const
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__UTILSCHEMAALTERACTION_APPLYSCHEMA ) ;

      switch ( _action )
      {
         case UTIL_SCHEMA_ADD_COLUMN :
         {
            utilSchemaColumn column( _newColAttr ) ;
            rc = schema.addColumn( column ) ;
            PD_RC_CHECK( rc, PDERROR, "Failed to apply add column action, "
                         "rc: %d", rc ) ;
            break ;
         }
         case UTIL_SCHEMA_DROP_COLUMN :
         {
            rc = schema.dropColumn( _colName ) ;
            PD_RC_CHECK( rc, PDERROR, "Failed to apply drop column action, "
                         "rc: %d", rc ) ;
            break ;
         }
         case UTIL_SCHEMA_ALTER_COLUMN :
         {
            rc = schema.alterColumn( _colName, _newColAttr, _alterMask ) ;
            PD_RC_CHECK( rc, PDERROR, "Failed to apply alter column action, "
                         "rc: %d", rc ) ;
            break ;
         }
         case UTIL_SCHEMA_RENAME_COLUMN :
         {
            rc = schema.renameColumn( _colName, _newColAttr.getName() ) ;
            PD_RC_CHECK( rc, PDERROR, "Failed to apply rename column action, "
                         "rc: %d", rc ) ;
            break ;
         }
         case UTIL_SCHEMA_DROP_DEFAULT :
         {
            rc = schema.dropDefault( _colName ) ;
            PD_RC_CHECK( rc, PDERROR, "Failed to apply drop default action, "
                         "rc: %d", rc ) ;
            break ;
         }
         case UTIL_SCHEMA_SET_ATTRIBUTES :
         {
            rc = schema.setAttributes( _newSchemaAttr, _alterMask ) ;
            PD_RC_CHECK( rc, PDERROR, "Failed to apply set attributes action, "
                         "rc: %d", rc ) ;
            break ;
         }
         default:
         {
            rc = SDB_SYS ;
            PD_LOG( PDERROR, "Failed to apply alter action on schema, "
                    "unknown action" ) ;
            SDB_ASSERT( FALSE, "should not be here" ) ;
            goto error ;
         }
      }

   done:
      PD_TRACE_EXITRC( SDB__UTILSCHEMAALTERACTION_APPLYSCHEMA, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__UTILSCHEMAALTERACTION_CHECKKEYPATTERN_NAME, "_utilSchemaAlterAction::checkKeyPattern" )
   INT32 _utilSchemaAlterAction::checkKeyPattern( const BSONObj &keyPattern,
                                                  const CHAR *columnName,
                                                  BOOLEAN &hasColumn ) const
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__UTILSCHEMAALTERACTION_CHECKKEYPATTERN_NAME ) ;

      rc = _utilCheckKeyPattern( keyPattern, columnName, hasColumn ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to check key pattern, rc: %d", rc ) ;

   done:
      PD_TRACE_EXITRC( SDB__UTILSCHEMAALTERACTION_CHECKKEYPATTERN_NAME, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__UTILSCHEMAALTERACTION_REBUILDKEYPATTERN, "_utilSchemaAlterAction::rebuildKeyPattern" )
   INT32 _utilSchemaAlterAction::rebuildKeyPattern( const BSONObj &keyPattern,
                                                    BSONObj &newKeyPattern,
                                                    BOOLEAN isRollback ) const
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__UTILSCHEMAALTERACTION_REBUILDKEYPATTERN ) ;

      if ( UTIL_SCHEMA_RENAME_COLUMN != _action )
      {
         newKeyPattern = keyPattern ;
         goto done ;
      }

      try
      {
         const CHAR *oldColName = isRollback ? _newColAttr.getName() : _colName ;
         const CHAR *newColName = isRollback ? _colName : _newColAttr.getName() ;
         UINT32 oldColNameLen = ossStrlen( oldColName ) ;
         BSONObjBuilder newKeyPatternBuilder ;
         BSONObjIterator iter( keyPattern ) ;
         while ( iter.more() )
         {
            BSONElement ele = iter.next() ;
            ossPoolString newKeyName ;
            const CHAR *keyName = ele.fieldName() ;

            if ( 0 == ossStrncmp( oldColName, keyName, oldColNameLen ) )
            {
               if ( '\0' == keyName[ oldColNameLen ] )
               {
                  keyName = newColName ;
               }
               else if ( '.' == keyName[ oldColNameLen ] )
               {
                  ossPoolStringStream ss ;
                  ss << newColName << "." << keyName + oldColNameLen + 1 ;
                  newKeyName = ss.str() ;
                  keyName = newKeyName.c_str() ;
               }
            }

            newKeyPatternBuilder.appendAs( ele, keyName ) ;
         }
         newKeyPattern = newKeyPatternBuilder.obj() ;
      }
      catch ( exception &e )
      {
         PD_LOG( PDERROR, "Failed to rebuild key pattern, occur exception %s",
                 e.what() ) ;
         rc = ossException2RC( &e ) ;
         goto error ;
      }

   done:
      PD_TRACE_EXITRC( SDB__UTILSCHEMAALTERACTION_REBUILDKEYPATTERN, rc ) ;
      return rc ;
   error:
      goto done ;
   }

}
