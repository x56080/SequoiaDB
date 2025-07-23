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

   Source File Name = utilSchema.hpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          02/07/2023  HGM Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef UTIL_SCHEMA_HPP_
#define UTIL_SCHEMA_HPP_

#include "oss.hpp"
#include "ossMemPool.hpp"
#include "utilMap.hpp"
#include "../bson/bson.hpp"

namespace engine
{

   #define UTIL_SCHEMA_NAME_MAX_SIZE      ( 127 )

   // masks for column restrictions
   // NOT NULL
   #define UTIL_SCHEMA_COLUMN_NOT_NULL    ( 0x00000001 )
   // NOT ARRAY
   #define UTIL_SCHEMA_COLUMN_NOT_ARRAY   ( 0x00000002 )

   #define UTIL_SCHEMA_ATTR_MASK_COL_NAME   ( 0x00000001 )
   #define UTIL_SCHEMA_ATTR_MASK_COL_TYPE   ( 0x00000002 )
   #define UTIL_SCHEMA_ATTR_MASK_COL_NNULL  ( 0x00000004 )
   #define UTIL_SCHEMA_ATTR_MASK_COL_NARRAY ( 0x00000010 )
   #define UTIL_SCHEMA_ATTR_MASK_COL_WDEF   ( 0x00000020 )
   #define UTIL_SCHEMA_ATTR_MASK_COL_RDEF   ( 0x00000040 )
   #define UTIL_SCHEMA_ATTR_MASK_COL_ALL    ( 0x0000FFFF )
   #define UTIL_SCHEMA_ATTR_MASK_NAME       ( 0x00010000 )
   #define UTIL_SCHEMA_ATTR_MASK_VERSION    ( 0x00020000 )
   #define UTIL_SCHEMA_ATTR_MASK_COLLECTION ( 0x00040000 )
   #define UTIL_SCHEMA_ATTR_MASK_STRICTMODE ( 0x00100000 )

   /*
      _utilSchemaColumnAttributes define
    */
   class _utilSchemaColAttr
   {
   public:
      _utilSchemaColAttr() ;
      _utilSchemaColAttr( const _utilSchemaColAttr &attr ) ;
      ~_utilSchemaColAttr() ;

      const CHAR *getName() const
      {
         return _name ;
      }

      void setName( const CHAR *name )
      {
         _name = name ;
      }

      bson::BSONType getType() const
      {
         return _type ;
      }

      void setType( bson::BSONType type )
      {
         _type = type ;
      }

      UINT32 getRestrict() const
      {
         return _restrictFlags ;
      }

      void setRestrict( UINT32 value )
      {
         _restrictFlags = value ;
      }

      BOOLEAN isNotNull() const
      {
         return OSS_BIT_TEST( _restrictFlags, UTIL_SCHEMA_COLUMN_NOT_NULL ) ? TRUE : FALSE ;
      }

      BOOLEAN isNotArray() const
      {
         return OSS_BIT_TEST( _restrictFlags, UTIL_SCHEMA_COLUMN_NOT_ARRAY ) ? TRUE : FALSE ;
      }

      void setNotNull( BOOLEAN flag )
      {
         if ( flag )
         {
            OSS_BIT_SET( _restrictFlags, UTIL_SCHEMA_COLUMN_NOT_NULL ) ;
         }
         else
         {
            OSS_BIT_CLEAR( _restrictFlags, UTIL_SCHEMA_COLUMN_NOT_NULL ) ;
         }
      }

      void setNotArray( BOOLEAN flag )
      {
         if ( flag )
         {
            OSS_BIT_SET( _restrictFlags, UTIL_SCHEMA_COLUMN_NOT_ARRAY ) ;
         }
         else
         {
            OSS_BIT_CLEAR( _restrictFlags, UTIL_SCHEMA_COLUMN_NOT_ARRAY ) ;
         }
      }

      const bson::BSONElement &getWriteDefault() const
      {
         return _writeDefault ;
      }

      bson::BSONType getWriteDefaultType() const
      {
         return _writeDefault.type() ;
      }

      void setWriteDefault( const bson::BSONElement &writeDefault )
      {
         _writeDefault = writeDefault ;
      }

      const bson::BSONElement &getReadDefault() const
      {
         return _readDefault ;
      }

      bson::BSONType getReadDefaultType() const
      {
         return _readDefault.type() ;
      }

      void setReadDefault( const bson::BSONElement &readDefault )
      {
         _readDefault = readDefault ;
      }

      BOOLEAN hasWriteDefault() const
      {
         return _writeDefault.eoo() ? FALSE : TRUE ;
      }

      BOOLEAN hasReadDefault() const
      {
         return _readDefault.eoo() ? FALSE : TRUE ;
      }

      BOOLEAN hasSameWriteDefault( const _utilSchemaColAttr &attr ) const
      {
         return 0 == _writeDefault.woCompare( attr.getWriteDefault(), FALSE ) ;
      }

      BOOLEAN hasSameReadDefault( const _utilSchemaColAttr &attr ) const
      {
         return 0 == _readDefault.woCompare( attr.getReadDefault(), FALSE ) ;
      }

      INT32 parse( const CHAR *name,
                   const bson::BSONObj &boDefine,
                   BOOLEAN fromUser,
                   BOOLEAN isNewAdded,
                   UINT32 &parsedMask ) ;
      INT32 toBSON( bson::BSONObjBuilder &builder,
                    UINT32 mask = UTIL_SCHEMA_ATTR_MASK_COL_ALL ) const ;

      const CHAR *getTypeName() const ;
      const CHAR *getWriteDefaultTypeName() const ;
      const CHAR *getReadDefaultTypeName() const ;

      BOOLEAN adjustForOID() ;
      BOOLEAN adjustForShardingKey() ;

   protected:
      void _reset()
      {
         _name = NULL ;
         _type = bson::EOO ;
         _restrictFlags = 0 ;
         _writeDefault = bson::BSONElement() ;
         _readDefault = bson::BSONElement() ;
      }

   protected:
      const CHAR *      _name ;
      bson::BSONType    _type ;
      UINT32            _restrictFlags ;
      bson::BSONElement _writeDefault ;
      bson::BSONElement _readDefault ;
   } ;
   typedef class _utilSchemaColAttr utilSchemaColAttr ;

   /*
      _utilSchemaColumn define
    */
   class _utilSchemaColumn : public _utilPooledObject,
                             public _utilSchemaColAttr
   {
   public:
      _utilSchemaColumn() ;
      _utilSchemaColumn( const _utilSchemaColAttr &attr ) ;
      ~_utilSchemaColumn() ;

      INT32 parse( const CHAR *name,
                   const bson::BSONObj &boDefine,
                   BOOLEAN fromUser,
                   BOOLEAN needGetOwned ) ;

      INT32 toBSON( bson::BSONObjBuilder &builder ) const ;
      INT32 toBSON( bson::BSONObj &boColumn ) const ;

      const bson::BSONObj &getDefine() const
      {
         return _define ;
      }

      _utilSchemaColumn &operator =( const _utilSchemaColumn &column )
      {
         _define = column._define ;
         _name = column._name ;
         _type = column._type ;
         _restrictFlags = column._restrictFlags ;
         _writeDefault = column._writeDefault ;
         _readDefault = column._readDefault ;
         return *this ;
      }

   protected:
      void _reset()
      {
         _utilSchemaColAttr::_reset() ;
         _define = bson::BSONObj() ;
      }

   protected:
      bson::BSONObj _define ;
   } ;

   typedef class _utilSchemaColumn utilSchemaColumn ;

   /*
      UTIL_SCHEMA_COLUMN_MAP define
    */
   typedef ossPoolVector< utilSchemaColumn > UTIL_SCHEMA_COLUMN_LIST ;
   typedef UTIL_SCHEMA_COLUMN_LIST::iterator UTIL_SCHEMA_COLUMN_LIST_IT ;
   typedef UTIL_SCHEMA_COLUMN_LIST::const_iterator UTIL_SCHEMA_COLUMN_LIST_CIT ;

   typedef _utilStringMap< utilSchemaColumn * > UTIL_SCHEMA_COLUMN_MAP ;
   typedef UTIL_SCHEMA_COLUMN_MAP::iterator UTIL_SCHEMA_COLUMN_MAP_IT ;
   typedef UTIL_SCHEMA_COLUMN_MAP::const_iterator UTIL_SCHEMA_COLUMN_MAP_CIT ;

   /*
      _utilSchemaAttributes
    */
   class _utilSchemaAttr
   {
   public:
      _utilSchemaAttr() ;
      ~_utilSchemaAttr() ;

      BOOLEAN isStrictMode() const
      {
         return _strictMode ;
      }

      void setStrictMode( BOOLEAN strictMode )
      {
         _strictMode = strictMode ;
      }

      INT32 parse( const bson::BSONObj &boOptions,
                   BOOLEAN fromUser,
                   UINT32 &parsedMask ) ;
      INT32 toBSON( bson::BSONObjBuilder &builder,
                    UINT32 mask ) const ;

   protected:
      void _reset()
      {
         _strictMode = FALSE ;
      }

      INT32 _parse( const bson::BSONElement &ele,
                    BOOLEAN fromUser,
                    UINT32 &parsedMask ) ;

   protected:
      BOOLEAN _strictMode ;
   } ;

   typedef class _utilSchemaAttr utilSchemaAttr ;

   /*
      _utilSchema define
    */
   class _utilSchema : public _utilPooledObject, public _utilSchemaAttr
   {
   public:
      _utilSchema() ;
      ~_utilSchema() ;

      INT32 parse( const bson::BSONObj &boDefine,
                   BOOLEAN fromUser,
                   BOOLEAN needGetOwned ) ;

      INT32 toBSON( bson::BSONObj &boSchema,
                    UINT32 mask = 0xFFFFFFFF ) const ;
      INT32 toBSON( bson::BSONObjBuilder &builder,
                    UINT32 mask = 0xFFFFFFFF ) const ;

      BOOLEAN isValid() const
      {
         return !( _define.isEmpty() ) ;
      }

      BOOLEAN hasBind() const
      {
         return NULL != _collection ;
      }

      const bson::BSONObj &getDefine() const
      {
         return _define ;
      }

      const CHAR *getName() const
      {
         return _name ;
      }

      UINT32 getVersion() const
      {
         return _version ;
      }

      const UTIL_SCHEMA_COLUMN_LIST &getColumns() const
      {
         return _columnList ;
      }

      const CHAR *getCollection() const
      {
         return _collection ;
      }

      ossPoolString toString() const
      {
         return _define.toPoolString() ;
      }

      utilSchemaColumn *getColumn( const CHAR *columnName ) ;
      const utilSchemaColumn *getColumn( const CHAR *columnName ) const ;
      BOOLEAN hasColumn( const CHAR *columnName ) const ;

      INT32 addColumn( const utilSchemaColumn &column ) ;
      INT32 dropColumn( const CHAR *columnName ) ;
      INT32 alterColumn( const CHAR *columnName,
                         const utilSchemaColAttr &attr,
                         UINT32 alterMask ) ;
      INT32 renameColumn( const CHAR *columnName,
                          const CHAR *newColumnName ) ;
      INT32 dropDefault( const CHAR *columnName ) ;
      INT32 setAttributes( const utilSchemaAttr &attr, UINT32 alterMask ) ;

      INT32 checkDefaultKeys( const bson::BSONObj &keyPattern,
                              BOOLEAN checkWriteDefault,
                              BOOLEAN checkReadDefault,
                              const CHAR *&conflictColumnName,
                              const _utilSchema *oldSchema = NULL ) const ;

      INT32 copy( const _utilSchema &schema ) ;
      INT32 adjustOID( BOOLEAN needRebuild ) ;
      INT32 adjustShardingKey( const bson::BSONObj &shardingKey,
                               BOOLEAN needRebuild ) ;
      INT32 getDefaultKeys( const bson::BSONObj &keyPattern,
                            bson::BSONObj &keys ) ;

   protected:
      INT32 _parseColumns( const bson::BSONObj &boColumns, BOOLEAN fromUser ) ;
      INT32 _columnsToBSON( bson::BSONObjBuilder &builder ) const ;
      INT32 _rebuild() ;

      void _reset()
      {
         _utilSchemaAttr::_reset() ;
         _define = bson::BSONObj() ;
         _name = NULL ;
         _version = 0 ;
         _columnList.clear() ;
         _columnMap.clear() ;
         _collection = NULL ;
      }

   protected:
      bson::BSONObj           _define ;
      const CHAR *            _name ;
      UINT32                  _version ;
      UTIL_SCHEMA_COLUMN_LIST _columnList ;
      UTIL_SCHEMA_COLUMN_MAP  _columnMap ;
      const CHAR *            _collection ;
   } ;

   typedef class _utilSchema utilSchema ;

   /*
      UTIL_SCHEMA_ALTER_ACTION define
    */
   enum UTIL_SCHEMA_ALTER_ACTION
   {
      UTIL_SCHEMA_ACTION_UNKNOWN = 0,
      UTIL_SCHEMA_ADD_COLUMN = 1,
      UTIL_SCHEMA_DROP_COLUMN = 2,
      UTIL_SCHEMA_ALTER_COLUMN = 3,
      UTIL_SCHEMA_RENAME_COLUMN = 4,
      UTIL_SCHEMA_DROP_DEFAULT = 5,
      UTIL_SCHEMA_SET_ATTRIBUTES = 6
   } ;

   /*
      _utilSchemaAlterAction define
    */
   class _utilSchemaAlterAction : public _utilPooledObject
   {
   public:
      _utilSchemaAlterAction() ;
      ~_utilSchemaAlterAction() ;

      const CHAR *getSchemaName() const
      {
         return _schemaName ;
      }

      UTIL_SCHEMA_ALTER_ACTION getAction() const
      {
         return _action ;
      }

      BOOLEAN isValid() const
      {
         return UTIL_SCHEMA_ACTION_UNKNOWN != _action ;
      }

      UINT32 getAlterMask() const
      {
         return _alterMask ;
      }

      const bson::BSONObj getActionObject() const
      {
         return _boAction ;
      }

      const utilSchemaColAttr getNewColAttr() const
      {
         return _newColAttr ;
      }

      const CHAR *getColumnName() const
      {
         return _colName ;
      }

      const bson::BSONObj &getColDefine() const
      {
         return _boColDefine ;
      }

      INT32 parse( const bson::BSONObj &boAction ) ;
      INT32 toBSON( bson::BSONObj &boAction ) const ;
      INT32 toBSON( bson::BSONObjBuilder &builder ) const ;

      INT32 copy( const _utilSchemaAlterAction &action ) ;
      INT32 adjust( const bson::BSONObj &shardingKey ) ;
      INT32 checkSchema( const utilSchema &schema ) const ;
      INT32 applySchema( utilSchema &schema ) const ;

      INT32 checkKeyPattern( const bson::BSONObj &keyPattern,
                             const CHAR *columnName,
                             BOOLEAN &hasColumn ) const ;
      INT32 rebuildKeyPattern( const bson::BSONObj &keyPattern,
                               bson::BSONObj &newKeyPattern,
                               BOOLEAN isRollback = FALSE ) const ;

   protected:
      INT32 _parseAddColumn( const bson::BSONObj &options ) ;
      INT32 _parseDropColumn( const bson::BSONObj &options ) ;
      INT32 _parseAlterColumn( const bson::BSONObj &options ) ;
      INT32 _parseRenameColumn( const bson::BSONObj &options ) ;
      INT32 _parseDropDefault( const bson::BSONObj &options ) ;
      INT32 _parseSetAttributes( const bson::BSONObj &options ) ;

      INT32 _toBSONAddColumn( bson::BSONObjBuilder &optionBuilder ) const ;
      INT32 _toBSONDropColumn( bson::BSONObjBuilder &optionBuilder ) const ;
      INT32 _toBSONAlterColumn( bson::BSONObjBuilder &optionBuilder ) const ;
      INT32 _toBSONRenameColumn( bson::BSONObjBuilder &optionBuilder ) const ;
      INT32 _toBSONDropDefault( bson::BSONObjBuilder &optionBuilder ) const ;
      INT32 _toBSONSetAttributes( bson::BSONObjBuilder &optionBuilder ) const ;

   protected:
      const CHAR *               _schemaName ;
      UTIL_SCHEMA_ALTER_ACTION   _action ;
      bson::BSONObj              _boAction ;
      const CHAR *               _colName ;
      bson::BSONObj              _boColDefine ;
      utilSchemaColAttr          _newColAttr ;
      utilSchemaAttr             _newSchemaAttr ;
      UINT32                     _alterMask ;
   } ;
   typedef class _utilSchemaAlterAction utilSchemaAlterAction ;

}

#endif /* UTIL_SCHEMA_HPP_ */
