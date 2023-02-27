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

   Source File Name = dmsInternalSchema.cpp

   Descriptive Name =

   When/how to use:

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          01/11/2023  YSD Initial Draft

   Last Changed =

*******************************************************************************/

#include "dmsInternalSchema.hpp"
#include "dmsTrace.hpp"
#include "dmsStorageDataCommon.hpp"

#define DMS_SCHEMA_INVALID_VERSION                 (0)
#define DMS_SCHEMA_COLID_STR_MAX_SIZE              5

namespace engine
{
   _dmsSchemaContainer::_dmsSchemaContainer()
   : _extent( NULL ),
     _extentSize( 0 )
   {
   }

   _dmsSchemaContainer::~_dmsSchemaContainer()
   {
   }

   INT32 _dmsSchemaContainer::init( const dmsSchemaExtent *extent, UINT32 extentSize, UINT16 mbID )
   {
      INT32 rc = SDB_OK ;

      if ( !extent )
      {
         rc = SDB_SYS ;
         PD_LOG( PDERROR, "Internal schema extent address is null, rc: %d", rc ) ;
         goto error ;
      }

      if ( !extent->validate( mbID ) )
      {
         rc = SDB_DMS_CORRUPTED_EXTENT ;
         PD_LOG( PDERROR, "Internal schema extent is invalid, rc: %d", rc ) ;
         goto error ;
      }

      _extent = extent ;
      _extentSize = extentSize ;

   done:
      return rc ;
   error:
      goto done ;
   }

   void _dmsSchemaContainer::reset()
   {
      _extent = NULL ;
      _extentSize = 0 ;
   }

   INT32 _dmsSchemaContainer::getColIDsWithDefault( ossPoolSet<UINT16> &colsWithReadDefault,
                                                    ossPoolSet<UINT16> &colsWithWriteDefault,
                                                    ossPoolSet<UINT16> &colsWithReadDefaultIndexCol ) const
   {
      INT32 rc = SDB_OK ;
      INT16 attr = 0 ;

      colsWithReadDefault.clear() ;
      colsWithWriteDefault.clear() ;
      colsWithReadDefaultIndexCol.clear() ;

      try
      {
         for ( UINT16 id = 0; id < _extent->_itemNum; ++id )
         {
            attr = _getColumnAttr( id ) ;
            if ( !( attr & DMS_SCHEMA_COL_DELETED ) )
            {
               if ( attr & DMS_SCHEMA_COL_READ_DEFAULT )
               {
                  colsWithReadDefault.insert( id ) ;
                  if ( !(attr & DMS_SCHEMA_COL_WRITE_DEFAULT ) && (attr & DMS_SCHEMA_COL_IN_INDEX) )
                  {
                     // Columns in index, have read default but no write default. They will be
                     // treated as primal columns.
                     colsWithReadDefaultIndexCol.insert( id ) ;
                  }
               }
               if ( attr & DMS_SCHEMA_COL_WRITE_DEFAULT )
               {
                  colsWithWriteDefault.insert( id ) ;
               }
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

   BOOLEAN _dmsSchemaContainer::hasReadDefault( UINT16 columnID ) const
   {
      SDB_ASSERT( columnID < _extent->_itemNum, "Column id is invalid" ) ;
      return OSS_BIT_TEST( _getColumnAttr( columnID ), DMS_SCHEMA_COL_READ_DEFAULT ) ;
   }

   BOOLEAN _dmsSchemaContainer::hasWriteDefault( UINT16 columnID ) const
   {
      SDB_ASSERT( columnID < _extent->_itemNum, "Column id is invalid" ) ;
      return OSS_BIT_TEST( _getColumnAttr( columnID ), DMS_SCHEMA_COL_WRITE_DEFAULT ) ;
   }

   BOOLEAN _dmsSchemaContainer::isIndexColumn( UINT16 columnID ) const
   {
      SDB_ASSERT( columnID < _extent->_itemNum, "Column id is invalid" ) ;
      return OSS_BIT_TEST( _getColumnAttr( columnID ), DMS_SCHEMA_COL_IN_INDEX ) ;
   }

   INT32 _dmsSchemaContainer::getColReadDefault( UINT16 columnID, const CHAR *&name,
                                                 INT32 &nameLen, BSONType &type,
                                                 const CHAR *&value, INT32 &valueLen ) const
   {
      INT32 rc = SDB_OK ;
      const dmsSchemaColRecord *colRecord = _getColRecord( columnID ) ;
      if ( !colRecord )
      {
         rc = SDB_DMS_CORRUPTED_EXTENT ;
         PD_LOG( PDERROR, "Column info of column id %u is invalid, rc: %d", columnID, rc ) ;
         goto error ;
      }

      name = colRecord->getName( &nameLen ) ;
      SDB_ASSERT( name, "Name is invalid" ) ;

      if ( !colRecord->getDefault( type, valueLen, value ) )
      {
         SDB_ASSERT( FALSE, "Default value should exist" ) ;
         rc = SDB_SYS ;
         PD_LOG( PDERROR, "Read default value not found for column %s, rc: %d", name, rc ) ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _dmsSchemaContainer::getColWriteDefault( UINT16 columnID, const CHAR *&name,
                                                  INT32 &nameLen, BSONType &type,
                                                  const CHAR *&value, INT32 &valueLen ) const
   {
      INT32 rc = SDB_OK ;
      const dmsSchemaColRecord *colRecord = _getColRecord( columnID ) ;
      if ( !colRecord )
      {
         rc = SDB_DMS_CORRUPTED_EXTENT ;
         PD_LOG( PDERROR, "Column info of column id %u is invalid, rc: %d", columnID, rc ) ;
         goto error ;
      }

      name = colRecord->getName( &nameLen ) ;
      SDB_ASSERT( name, "Name is invalid" ) ;

      if ( !colRecord->getDefault( type, valueLen, value, FALSE ) )
      {
         SDB_ASSERT( FALSE, "Default value should exist" ) ;
         rc = SDB_SYS ;
         PD_LOG( PDERROR, "Write default value not found for column %s, rc: %d", name, rc ) ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _dmsSchemaContainer::getColumnBasicInfo( UINT16 columnID, const CHAR **name,
                                                  INT32 *nameLen, BOOLEAN *isDeleted,
                                                  BOOLEAN *hasReadDefault,
                                                  BOOLEAN *hasWriteDefault,
                                                  BOOLEAN *isIndexColumn,
                                                  BOOLEAN *hasOrigName ) const
   {
      INT32 rc = SDB_OK ;
      INT16 attr = 0 ;
      const dmsSchemaColRecord *record = _getColRecord( columnID ) ;

      attr = _getColumnAttr( columnID ) ;
      if ( name && nameLen )
      {
         *name = record->getName( nameLen ) ;
      }

      if ( isDeleted )
      {
         *isDeleted = ( attr & DMS_SCHEMA_COL_DELETED ) ? TRUE : FALSE ;
      }
      if ( hasReadDefault )
      {
         *hasReadDefault = ( attr & DMS_SCHEMA_COL_READ_DEFAULT ) ? TRUE : FALSE ;
      }
      if ( hasWriteDefault )
      {
         *hasWriteDefault = ( attr & DMS_SCHEMA_COL_WRITE_DEFAULT ) ? TRUE : FALSE ;
      }
      if ( isIndexColumn )
      {
         *isIndexColumn = ( attr & DMS_SCHEMA_COL_IN_INDEX ) ? TRUE : FALSE ;
      }
      if ( hasOrigName )
      {
         *hasOrigName = ( attr & DMS_SCHEMA_COL_HAS_ORIGNAME ) ? TRUE : FALSE ;
      }

      return rc ;
   }

   const CHAR *_dmsSchemaContainer::getOrigName( UINT16 columnID, INT32 *nameLen ) const
   {
      const dmsSchemaColRecord *record = _getColRecord( columnID ) ;
      const CHAR *name = record->getOrigName( nameLen ) ;
      return name ;
   }

   INT32 _dmsSchemaContainer::toObj( BSONObj &object ) const
   {
      INT32 rc = SDB_OK ;
      try
      {
         const CHAR *name = NULL ;
         BSONObjBuilder builder ;

         for ( UINT16 columnID = 0; columnID < _extent->_itemNum; ++columnID )
         {
            BSONObj columnDef ;
            if ( _isColumnDeleted( columnID ) )
            {
               continue ;
            }

            rc = _columnInfo2Def( columnID, &name, columnDef ) ;
            PD_RC_CHECK( rc, PDERROR, "Get column definition failed, rc: %d", rc ) ;

            builder.append( name, columnDef ) ;
         }
         object = builder.obj() ;
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

   INT32 _dmsSchemaContainer::getColumnInfoSize( UINT16 columnID, UINT16 &size ) const
   {
      INT32 rc = SDB_OK ;
      const dmsSchemaColRecord *record = _getColRecord( columnID ) ;

      SDB_ASSERT( record, "Column info record invalid" ) ;

      if ( !record )
      {
         rc = SDB_SYS ;
         PD_LOG( PDERROR, "Get column info for column id %u failed, rc: %d", columnID, rc ) ;
         goto error ;
      }

      size = record->getLength() ;

   done:
      return rc ;
   error:
      goto done ;
   }


   INT32 _dmsSchemaContainer::dump( BSONObj &schemaObj, BOOLEAN includeColumnID ) const
   {
      INT32 rc = SDB_OK ;
      try
      {
         const CHAR *name = NULL ;
         const CHAR *origName = NULL ;
         BSONObjBuilder builder ;
         BSONObjBuilder subBuilder ;
         BSONArrayBuilder columnBuilder( builder.subarrayStart( FIELD_NAME_COLUMNS ) ) ;

         for ( UINT16 columnID = 0; columnID < _extent->_itemNum; ++columnID )
         {
            BSONObj columnDef ;
            INT16 attr = _getColumnAttr( columnID ) ;
            rc = _columnInfo2Def( columnID, &name, columnDef ) ;
            PD_RC_CHECK( rc, PDERROR, "Get column definition failed, rc: %d", rc ) ;
            origName = getOrigName( columnID ) ;

            if ( includeColumnID )
            {
               subBuilder.append( FIELD_NAME_ID, columnID ) ;
            }
            subBuilder.append( FIELD_NAME_NAME, name ) ;
            if ( origName )
            {
               subBuilder.append( FIELD_NAME_ORIGIN_NAME, origName ) ;
               origName = NULL ;
            }

            subBuilder.appendElements( columnDef ) ;
            if ( OSS_BIT_TEST( attr, DMS_SCHEMA_COL_DELETED ) )
            {
               subBuilder.append( FIELD_NAME_DELETED, true ) ;
            }
            if ( OSS_BIT_TEST( attr, DMS_SCHEMA_COL_IN_INDEX ))
            {
               subBuilder.append( FIELD_NAME_INDEX_COL, true ) ;
            }
            columnBuilder.append( subBuilder.done() ) ;
            subBuilder.reset() ;
         }

         columnBuilder.done() ;
         schemaObj = builder.obj() ;
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

   INT32 _dmsSchemaContainer::_columnInfo2Def( UINT16 columnID, const CHAR **name,
                                               BSONObj &columnDef ) const
   {
      INT32 rc = SDB_OK ;
      BSONType type ;
      const CHAR *value = NULL ;
      INT32 valueSize = 0 ;
      BOOLEAN isEmpty = FALSE ;
      BOOLEAN foundDefault = FALSE ;
      utilBSONRawBuilder rawBuilder ;
      const dmsSchemaColRecord *record = _getColRecord( columnID ) ;
      UINT32 buffSize = record->getLength() + 64 ;  // Extra space for field names(ReadDefault, WriteDefault).

      CHAR *buffer = (CHAR *)SDB_OSS_MALLOC( buffSize ) ;
      if ( !buffer )
      {
         rc = SDB_OOM ;
         PD_LOG( PDERROR, "Allocate memory of size [%d] for building internal schema column object "
                 "failed, rc: %d", record->getLength(), rc ) ;
         goto error ;
      }

      if ( name )
      {
         *name = record->getName() ;
         SDB_ASSERT( *name, "Column name is invalid" ) ;
      }

      rawBuilder.start( buffer, buffSize ) ;

      foundDefault = record->getDefault( type, valueSize, value ) ;
      if ( foundDefault )
      {
         rc = rawBuilder.appendElement( type, FIELD_NAME_READDEFAULT,
                                        ossStrlen( FIELD_NAME_READDEFAULT ),
                                        value, valueSize ) ;
         PD_RC_CHECK( rc, PDERROR, "Append read default to column info builder failed, rc: %d",
                      rc ) ;
      }

      foundDefault = record->getDefault( type, valueSize, value, FALSE ) ;
      if ( foundDefault )
      {
         rc = rawBuilder.appendElement( type, FIELD_NAME_WRITEDEFAULT,
                                        ossStrlen( FIELD_NAME_WRITEDEFAULT ),
                                        value, valueSize ) ;
         PD_RC_CHECK( rc, PDERROR, "Append write default to column info builder failed, rc: %d",
                      rc ) ;
      }

      rc = rawBuilder.done( isEmpty ) ;
      PD_RC_CHECK( rc, PDERROR, "Generate schema column info failed, rc: %d", rc ) ;

      try
      {
         if ( isEmpty )
         {
            columnDef = BSONObj() ;
         }
         else
         {
            columnDef = BSONObj( buffer ).copy() ;
         }
      }
      catch ( std::exception &e )
      {
         rc = ossException2RC( &e ) ;
         PD_LOG( PDERROR, "Unexpected exception occurred: %s", e.what() ) ;
         goto error ;
      }

   done:
      if ( buffer )
      {
         SDB_OSS_FREE( buffer ) ;
      }
      return rc ;
   error:
      goto done ;
   }

   INT32 _dmsSchemaContainer::_columnInfo2Obj( UINT16 columnID, BSONObj &object, BOOLEAN includeID,
                                               BOOLEAN includeAttr )
   {
      INT32 rc = SDB_OK ;
      BSONType type ;
      INT32 valueSize = 0 ;
      const CHAR *value = NULL ;
      utilBSONRawBuilder rawBuilder ;
      BSONObjBuilder builder ;
      BOOLEAN isEmpty = FALSE ;
      BOOLEAN foundDefault = FALSE ;

      const dmsSchemaColRecord *record = _getColRecord( columnID ) ;
      UINT32 bufferSize = record->getLength() + 64 ;
      CHAR *buffer = (CHAR *)SDB_OSS_MALLOC( bufferSize ) ;
      if ( !buffer )
      {
         rc = SDB_OOM ;
         PD_LOG( PDERROR, "Allocate memory of size [%d] for building internal schema column object "
                 "failed, rc: %d", record->getLength(), rc ) ;
         goto error ;
      }

      try
      {
         if ( includeID )
         {
            builder.append( FIELD_NAME_ID, columnID ) ;
         }

         builder.append( FIELD_NAME_NAME, record->getName() ) ;
         if ( includeAttr )
         {
            builder.append( FIELD_NAME_DELETED, _isColumnDeleted( columnID ) ? true : false ) ;
            builder.append( FIELD_NAME_INDEX_COL, _isIndexColumn( columnID ) ? true : false ) ;
         }

         rawBuilder.start( buffer, bufferSize ) ;
         value = record->getOrigName() ;
         if ( value )
         {
            rc = rawBuilder.appendElement( bson::String, FIELD_NAME_ORIGIN_NAME,
                                           ossStrlen( FIELD_NAME_ORIGIN_NAME ),
                                           value, valueSize ) ;
            PD_RC_CHECK( rc, PDERROR, "Append original name to column info builder failed, rc: %d",
                         rc ) ;
         }

         foundDefault = record->getDefault( type, valueSize, value ) ;
         if ( foundDefault )
         {
            rc = rawBuilder.appendElement( type, FIELD_NAME_READDEFAULT,
                                           ossStrlen( FIELD_NAME_READDEFAULT ),
                                           value, valueSize ) ;
            PD_RC_CHECK( rc, PDERROR, "Append read default to column info builder failed, rc: %d",
                         rc ) ;
         }

         foundDefault = record->getDefault( type, valueSize, value, FALSE ) ;
         if ( foundDefault )
         {
            rc = rawBuilder.appendElement( type, FIELD_NAME_WRITEDEFAULT,
                                           ossStrlen( FIELD_NAME_WRITEDEFAULT ),
                                           value, valueSize ) ;
            PD_RC_CHECK( rc, PDERROR, "Append write default to column info builder failed, rc: %d",
                         rc ) ;
         }

         rc = rawBuilder.done( isEmpty ) ;
         PD_RC_CHECK( rc, PDERROR, "Generate schema column info failed, rc: %d", rc ) ;

         if ( isEmpty )
         {
            builder.appendElements( BSONObj() ) ;
         }
         else
         {
            builder.appendElements( BSONObj(buffer) ) ;
         }

         object = builder.obj() ;
      }
      catch ( std::exception &e )
      {
         rc = ossException2RC( &e ) ;
         PD_LOG( PDERROR, "Unexpected exception occurred: %s", e.what() ) ;
         goto error ;
      }

   done:
      if ( buffer )
      {
         SDB_OSS_FREE( buffer ) ;
      }
      return rc ;
   error:
      goto done ;
   }

   void _dmsSchemaContainer::_setColumnAttr( UINT16 columnID, INT16 flags )
   {
      INT32* slot= (INT32 *)
         _offset2Ptr( DMS_SCHEMAEXTENT_HEADER_SZ + DMS_SCHEMAEXTENT_SLOT_SZ * columnID ) ;
      OSS_BIT_SET( *slot, ((INT32)flags) << 16 ) ;
   }

   void _dmsSchemaContainer::_clearColumnAttr( UINT16 columnID, INT16 flags )
   {
      INT32* slot= (INT32 *)
         _offset2Ptr( DMS_SCHEMAEXTENT_HEADER_SZ + DMS_SCHEMAEXTENT_SLOT_SZ * columnID ) ;
      OSS_BIT_CLEAR( *slot, ((INT32)flags << 16 ) ) ;
   }

   void _dmsSchemaContainer::_getColAttrAndRecordOffset( UINT16 columnID, INT16 &attr,
                                                         UINT16 &valueOffset ) const
   {
      const INT32* slot= (const INT32 *)
         _offset2Ptr( DMS_SCHEMAEXTENT_HEADER_SZ + DMS_SCHEMAEXTENT_SLOT_SZ * columnID ) ;
      attr = (INT16)( *slot >> 16 ) ;
      valueOffset = (UINT16)( (*slot) & 0x0000FFFF ) ;
   }

   BOOLEAN _dmsSchemaContainer::_isColumnDeleted( UINT16 columnID ) const
   {
      INT16 attr = _getColumnAttr( columnID ) ;
      return (attr & DMS_SCHEMA_COL_DELETED) ;
   }

   BOOLEAN _dmsSchemaContainer::_isIndexColumn( UINT16 columnID ) const
   {
      INT16 attr = _getColumnAttr( columnID ) ;
      return (attr & DMS_SCHEMA_COL_IN_INDEX) ;
   }


   _dmsSchemaHash::_dmsSchemaHash()
   : _schemaContainer( NULL ),
     _extent( NULL ),
     _extentSize( 0 )
   {
   }

   _dmsSchemaHash::~_dmsSchemaHash()
   {
   }

   INT32 _dmsSchemaHash::init( const dmsSchemaContainer *schemaContainer,
                               const dmsSchemaHashExtent *extent, UINT32 extentSize, UINT16 mbID )
   {
      INT32 rc = SDB_OK ;

      if ( !extent )
      {
         rc = SDB_SYS ;
         PD_LOG( PDERROR, "Internal schema hash extent address is null, rc: %d", rc ) ;
         goto error ;
      }

      if ( !extent->validate( mbID ) )
      {
         rc = SDB_DMS_CORRUPTED_EXTENT ;
         PD_LOG( PDERROR, "Internal schema hash extent is invalid, rc: %d", rc ) ;
         goto error ;
      }

      _schemaContainer = schemaContainer ;
      _extent = extent ;
      _extentSize = extentSize ;

   done:
      return rc ;
   error:
      goto done ;
   }

   void _dmsSchemaHash::reset()
   {
      _schemaContainer = NULL ;
      _extent = NULL ;
      _extentSize = 0 ;
   }

   UINT16 _dmsSchemaHash::getColumnIDByName( const CHAR *name ) const
   {
      INT32 *currItem = NULL ;
      UINT16 columnID = DMS_SCHEMA_INVALID_COLUMNID ;
      const CHAR *columnName = NULL ;

      UINT32 itemID = ossHash( name ) % DMS_SCHEMA_HASH_BUCKET_SIZE ;   // Bucket is also an item.
      UINT16 itemOffset = DMS_SCHEMAHASHEXTENT_HEADER_SZ + DMS_SCHEMAHASHEXTENT_ITEM_SZ * itemID ;

      SDB_ASSERT( name, "Column name is null" ) ;

      while ( DMS_SCHEMA_INVALID_ITEM_OFFSET != itemOffset )
      {
         currItem = (INT32 *)_offset2Ptr( itemOffset ) ;
         columnID = _getColumnIDByItem( currItem ) ;
         if ( DMS_SCHEMA_INVALID_COLUMNID == columnID )
         {
            break ;
         }

         columnName = _schemaContainer->getColumnName( columnID ) ;
         if ( 0 == ossStrcmp( name, columnName ) )
         {
            // Found
            break ;
         }
         else
         {
            columnID = DMS_SCHEMA_INVALID_COLUMNID ;
            // Check if any conflict item.
            itemOffset = _getNextItemOffset( currItem ) ;
         }
      }

      return columnID ;
   }

   UINT16 _dmsSchemaHash::_getColumnIDByItem( const INT32 *item ) const
   {
      return (UINT16)( (*item) & DMS_SCHEMA_HASH_ID_MASK ) ;
   }

   INT32 _dmsSchemaHash::_nextFreeItemOffset() const
   {
      // Search in the conflict area for a free item.
      INT32 itemNum =
         (_extentSize - DMS_SCHEMAHASHEXTENT_HEADER_SZ) / DMS_SCHEMAHASHEXTENT_ITEM_SZ -
         DMS_SCHEMA_HASH_BUCKET_SIZE ;

      INT32 offset = DMS_SCHEMAHASHEXTENT_HEADER_SZ +
                     DMS_SCHEMAHASHEXTENT_ITEM_SZ * DMS_SCHEMA_HASH_BUCKET_SIZE ;

      for ( INT32 i = 0; i < itemNum; ++i )
      {
         if ( DMS_SCHEMA_HASH_INVALID_ITEM_VALUE == *(UINT32 *)_offset2Ptr(offset) )
         {
            return offset ;
         }
      }

      return -1 ;
   }

   _dmsInternalSchema::_dmsInternalSchema()
   : _enabled( FALSE ),
     _version( DMS_SCHEMA_INVALID_VERSION ),
     _defaultMaxSize( 0 ),
     _totalValidNameSize( 0 )
   {
   }

   _dmsInternalSchema::~_dmsInternalSchema()
   {
   }

   INT32 _dmsInternalSchema::init( const dmsSchemaExtent *schemaExtent, UINT32 schemaExtentSize,
                                   const dmsSchemaHashExtent *hashExtent, UINT32 hashExtentSize,
                                   UINT16 mbID )
   {
      INT32 rc = SDB_OK ;

      SDB_ASSERT( schemaExtent && (schemaExtentSize > 0), "Schema extent is invalid" ) ;
      SDB_ASSERT( hashExtent && (hashExtentSize > 0), "Schema hash extent is invalid" ) ;

      rc = _schemaContainer.init( schemaExtent, schemaExtentSize, mbID ) ;
      PD_RC_CHECK( rc, PDERROR, "Init internal schema extent failed, rc: %d", rc ) ;

      rc = _schemaHash.init( &_schemaContainer, hashExtent, hashExtentSize, mbID ) ;
      PD_RC_CHECK( rc, PDERROR, "Init internal schema hash extent failed, rc: %d", rc ) ;

      _onSchemaColChanged() ;
      _enabled = TRUE ;

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _dmsInternalSchema::reload()
   {
      INT32 rc = SDB_OK ;

      rc = _onSchemaColChanged() ;
      PD_RC_CHECK( rc, PDERROR, "Refresh internal schema failed, rc: %d", rc ) ;

   done:
      return rc ;
   error:
      goto done ;
   }

   void _dmsInternalSchema::reset()
   {
      _schemaContainer.reset() ;
      _schemaHash.reset() ;
      _encodeWatchIDs.clear() ;
      _decodeWatchIDs.clear() ;
      _enabled = FALSE ;
      _version = DMS_SCHEMA_INVALID_VERSION ;
      _defaultMaxSize = 0 ;
      _totalValidNameSize = 0 ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSINTERNALSCHEMA_ENCODERECORD, "_dmsInternalSchema::encodeRecord" )
   INT32 _dmsInternalSchema::encodeRecord( dmsMBContext *context, _pmdEDUCB *cb,
                                           dmsRecordData &recordData, dmsRecordData &encodeData,
                                           BOOLEAN &hasNewColumn )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__DMSINTERNALSCHEMA_ENCODERECORD ) ;
      UINT32 totalSize = 0 ;
      CHAR *encodeRecord = NULL ;
      COLUMN_ID_SET watchIDs( _encodeWatchIDs ) ;
      INT32 estimateSize = 0 ;
      utilBSONRawBuilder encodeBuilder ;
      BOOLEAN isEmpty = FALSE ;
      hasNewColumn = FALSE ;

      if ( recordData.len() + DMS_RECORD_METADATA_SZ > DMS_RECORD_USER_MAX_SZ )
      {
         rc = SDB_DMS_RECORD_TOO_BIG ;
         goto error ;
      }

      try
      {
         BSONObj record( recordData.data() ) ;
         // Estimate by the worest case: Each column name is 1 byte and all columns with default
         // will be added into the record.
         estimateSize = record.objsize() + _defaultMaxSize + _schemaContainer.columnNum() ;
         encodeRecord = cb->getEncodeBuff( estimateSize ) ;
         if ( !encodeRecord )
         {
            rc = SDB_OOM ;
            PD_LOG( PDERROR, "Get buffer of size %u for encoding record failed, rc: %d",
                    estimateSize, rc ) ;
            goto error ;
         }

         encodeBuilder.start( encodeRecord, estimateSize ) ;

         rc = _parseRecord( context, encodeBuilder, watchIDs, record, hasNewColumn ) ;
         if ( SDB_OK == rc && hasNewColumn )
         {
            // Need to update the internal schema by the record, and encode again.
            goto done ;
         }
         else if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "Parse record by internal schema failed, rc: %d", rc ) ;
            goto error ;
         }

         if ( !watchIDs.empty() )
         {
            ISession *session = cb->getSession() ;
            if ( session && session->isBusinessSession() )
            {
               rc = _appendPrimalColumns( context, cb, encodeBuilder, watchIDs,
                                          record, recordData ) ;
               PD_RC_CHECK( rc, PDERROR, "Append primal columns into record failed, rc: %d", rc ) ;
            }
         }

         rc = encodeBuilder.done( isEmpty ) ;
         PD_RC_CHECK( rc, PDERROR, "Finish building encoded record failed, rc: %d", rc ) ;

         totalSize = encodeBuilder.dataSize() ;

         // Check once again if the encoded record size exceeds the limit
         if ( totalSize + DMS_RECORD_METADATA_SZ > DMS_RECORD_USER_MAX_SZ )
         {
            rc = SDB_DMS_RECORD_TOO_BIG ;
            goto error ;
         }

         encodeData.setData( (const CHAR *)encodeRecord, totalSize,
                             UTIL_COMPRESSOR_INVALID, TRUE, TRUE ) ;

#ifdef _DEBUG
         _encodeSanityCheck( encodeData )  ;
#endif /* _DEBUG */

      }
      catch ( std::exception &e )
      {
         rc = ossException2RC( &e ) ;
         PD_LOG( PDERROR, "Unexpected exception occurred: %s", e.what() ) ;
         goto error ;
      }

   done:
      PD_TRACE_EXITRC( SDB__DMSINTERNALSCHEMA_ENCODERECORD, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSINTERNALSCHEMA_DECODERECORD, "_dmsInternalSchema::decodeRecord" )
   INT32 _dmsInternalSchema::decodeRecord( _pmdEDUCB *cb, const CHAR *data, UINT32 dataSize,
                                           const CHAR **record, UINT32 &recordSize,
                                           BOOLEAN getPrimalData )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__DMSINTERNALSCHEMA_DECODERECORD ) ;
      const CHAR *columnName = NULL ;
      INT32 nameLen = 0 ;
      BOOLEAN colIsDeleted = FALSE ;
      ossPoolSet<UINT16> readDefaultIDs ;
      BOOLEAN hasReadDefault = _decodeWatchIDs.size() > 0 ? TRUE : FALSE ;
      UINT32 decodeSize = 0 ;
      // Get three pointers: ID pointer, type pointer and value pointer, to build each BSON element.
      CHAR *decodeBuffer = NULL ;
      utilBSONRawBuilder builder ;
      BOOLEAN isEmpty = FALSE ;

      try
      {
         BSONObj encodedRecord( data ) ;
         BSONObjIterator itr( encodedRecord ) ;
         // Estimate the decode size. Column ids will be replaced by column names(deleted columns
         // not included), and columns with write default values will be added.
         decodeSize = encodedRecord.objsize() + _totalValidNameSize + _defaultMaxSize ;
         decodeBuffer = cb->getDecodeBuff( decodeSize ) ;
         if ( !decodeBuffer )
         {
            rc = SDB_OOM ;
            PD_LOG( PDERROR, "Get buffer of size %u for decoding record failed, rc: %d",
                    decodeSize, rc ) ;
            goto error ;
         }

         // Make a coyp of the ids, try to erase below
         if ( hasReadDefault )
         {
            readDefaultIDs.insert( _decodeWatchIDs.begin(), _decodeWatchIDs.end() ) ;
         }

         rc = builder.start( decodeBuffer, decodeSize ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to start building record, rc: %d", rc ) ;

         while ( itr.more() )
         {
            BSONElement ele = itr.next() ;
            UINT16 columnID = ossAtoi( ele.fieldName() ) ;
            // This column is included in the record, so we need not to add its read default any more.
            if ( hasReadDefault )
            {
               readDefaultIDs.erase( columnID ) ;
            }

            _schemaContainer.getColumnBasicInfo( columnID, &columnName, &nameLen, &colIsDeleted ) ;
            if ( colIsDeleted )
            {
               // Ignore columns which has been marked as delete.
               continue ;
            }

            rc = builder.appendElement( ele.type(), columnName, nameLen,
                                        ele.value(), ele.valuesize() ) ;
            PD_RC_CHECK( rc, PDERROR, "Append column [%s] to record decode buffer failed, rc: %d",
                         columnName, rc ) ;
         }

         if ( readDefaultIDs.size() > 0 && !getPrimalData )
         {
            // Append columns with default.
            rc = _appendColWithReadDefault( readDefaultIDs, builder ) ;
            PD_RC_CHECK( rc, PDERROR, "Append column with default value to record failed, rc: %d",
                        rc ) ;
         }

         builder.done( isEmpty ) ;

         *record = decodeBuffer ;
         recordSize = *(INT32 *)decodeBuffer ;
      }
      catch ( std::exception &e )
      {
         rc = ossException2RC( &e ) ;
         PD_LOG( PDERROR, "Unexpected exception occurred: %s", e.what() ) ;
         goto error ;
      }

   done:
      PD_TRACE_EXITRC( SDB__DMSINTERNALSCHEMA_DECODERECORD, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   INT32 _dmsInternalSchema::rebuildRecord( _pmdEDUCB *cb, const BSONObj &record,
                                            const CHAR **newRecord, UINT32 &newRecSize,
                                            BOOLEAN &changed, BOOLEAN getPrimalData )
   {
      // For records that are not encoded, we need to check:

      INT32 rc = SDB_OK ;

      try
      {
         NAME_INFO_MAP watchNames ;  // Including column names which have been deleted, or renamed.
         watchNames.insert( _decodeWatchNames.begin(), _decodeWatchNames.end() ) ;

         rc = _checkRebuildRecord( record, watchNames ) ;
         PD_RC_CHECK( rc, PDERROR, "Check rebuild record by internal schema failed, rc: %d", rc ) ;

         if ( watchNames.empty() )
         {
            // No columns need to be modified. Just use the original record.
            changed = FALSE ;
            goto done ;
         }

         changed = TRUE ;

         {
            BOOLEAN isEmpty = FALSE ;
            utilBSONRawBuilder builder ;
            NAME_DECODE_INFO_ITR nameItr ;
            BSONObjIterator itr( record ) ;
            INT32 buffSize = record.objsize() +  _defaultMaxSize ;      // TODO: YSD need to calculate the size.
            CHAR *buff = cb->getDecodeBuff( buffSize ) ;
            if ( !buff )
            {
               rc = SDB_OOM ;
               PD_LOG( PDERROR, "Allocate buffer of size %d for decoding record failed, rc: %d",
                       buffSize, rc ) ;
               goto error ;
            }

            rc = builder.start( buff, buffSize ) ;
            PD_RC_CHECK( rc, PDERROR, "Start rebuild record by internal schema failed, rc: %d",
                         rc ) ;

            while ( itr.more() )
            {
               BSONElement ele = itr.next() ;
               nameItr = watchNames.find( ele.fieldName() ) ;
               if ( watchNames.end() == nameItr )
               {
                  rc = builder.appendElement( ele.type(), ele.fieldName(),
                                              ossStrlen( ele.fieldName() ),
                                              ele.value(), ele.valuesize() ) ;
                  PD_RC_CHECK( rc, PDERROR, "Append element when rebuilding record failed, rc: %d",
                               rc ) ;
                  watchNames.erase( ele.fieldName() ) ;
               }
               else
               {
                  // Found the name in the watch names.
                  if ( nameItr->second._isDeleted )
                  {
                     // If the column is deleted in the schema, do not add it to the result.
                     watchNames.erase( ele.fieldName() ) ;  // Code Review: Remove by iterator. Use a counter instead of erase
                     continue ;
                  }
                  else if ( nameItr->second._isOrigName )
                  {
                     // If the column has been renamed in the schema, get the current name.
                     const CHAR *name =
                        _schemaContainer.getColumnName( nameItr->second._columnID ) ;
                     rc = builder.appendElement( ele.type(), name, ossStrlen( name ),
                                                 ele.value(), ele.valuesize() ) ;
                     PD_RC_CHECK( rc, PDERROR, "Append element when rebuilding record failed, "
                                  "rc: %d", rc ) ;
                     watchNames.erase( ele.fieldName() ) ;
                  }
                  else
                  {
                     // If the column is not deleted, and has not been renamed, it's in the watch
                     // list just because it has a read default value. Keep the value of the
                     // original record here.
                     rc = builder.appendElement( ele.type(), ele.fieldName(),
                                                 ossStrlen( ele.fieldName() ),
                                                 ele.value(), ele.valuesize() ) ;
                     PD_RC_CHECK( rc, PDERROR, "Append element when rebuilding record failed, "
                                  "rc: %d", rc ) ;
                     watchNames.erase( ele.fieldName() ) ;
                  }
               }
            }

            if ( !getPrimalData )
            {
               for ( NAME_DECODE_INFO_ITR itr = watchNames.begin(); itr != watchNames.end(); )
               {
                  const CHAR *name = NULL ;
                  INT32 nameLen = 0 ;
                  BSONType type ;
                  const CHAR *value = NULL ;
                  INT32 valueLen = 0 ;
                  rc = _schemaContainer.getColReadDefault( itr->second._columnID, name, nameLen,
                                                           type, value, valueLen ) ;
                  PD_RC_CHECK( rc, PDERROR, "Get read default of column %s failed, rc: %d",
                               itr->first, rc ) ;
                  rc = builder.appendElement( type, name, nameLen, value, valueLen ) ;
                  PD_RC_CHECK( rc, PDERROR, "Append element when rebuilding record failed, "
                               "rc: %d", rc ) ;
                  watchNames.erase( itr++ ) ;
               }
            }

            rc = builder.done( isEmpty ) ;
            PD_RC_CHECK( rc, PDERROR, "Rebuild record by internal schema failed, rc: %d", rc ) ;

            *newRecord = buff ;
            newRecSize = *(INT32 *)buff ;
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

   /* Each column includes:
    * 1. ID( if includeColumnID is true )
    * 2. Name
    * 3. Read default value(if any)
    * 4. Write default value(if any)
    * 5. If the column is deleted
    * 6. If it's column in index
   */
   INT32 _dmsInternalSchema::dumpSchemaInfo( BSONObj &schema, BOOLEAN includeColumnID )
   {
      INT32 rc = SDB_OK ;

      rc = _schemaContainer.dump( schema , includeColumnID ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to dump schema info from schema extent, rc: %d", rc ) ;

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _dmsInternalSchema::toObj( BSONObj &obj )
   {
      INT32 rc = SDB_OK ;

      rc = _schemaContainer.toObj( obj ) ;
      PD_RC_CHECK( rc, PDERROR, "Get internal schema object failed, rc: %d", rc ) ;

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _dmsInternalSchema::toSchemaObj( const CHAR *name,
                                          BSONObj &boSchema )
   {
      INT32 rc = SDB_OK ;

      BSONObj boColDefine ;

      rc = _schemaContainer.toObj( boColDefine ) ;
      PD_RC_CHECK( rc, PDERROR, "Get internal schema object failed, rc: %d", rc ) ;

      try
      {
         BSONObjBuilder builder ;
         builder.append( FIELD_NAME_NAME, name ) ;
         builder.append( FIELD_NAME_COLUMNS, boColDefine ) ;
         boSchema = builder.obj() ;
      }
      catch ( exception &e )
      {
         PD_LOG( PDERROR, "Failed to build internal schema, "
                 "occur exception %s", e.what() ) ;
         rc = ossException2RC( &e ) ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSINTERNALSCHEMA__PARSERECORD, "_dmsInternalSchema::_parseRecord" )
   INT32 _dmsInternalSchema::_parseRecord( dmsMBContext *context,
                                           utilBSONRawBuilder &encodeBuilder,
                                           COLUMN_ID_SET &watchIDs,
                                           const BSONObj& record,
                                           BOOLEAN &hasNewColumn )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__DMSINTERNALSCHEMA__PARSERECORD ) ;
      UINT16 columnID = DMS_SCHEMA_INVALID_COLUMNID ;
      CHAR columnName[ DMS_SCHEMA_COLID_STR_MAX_SIZE + 1 ] = { 0 } ;

      BOOLEAN hasWriteDefault = _encodeWatchIDs.size() > 0 ? TRUE : FALSE ;
      if ( hasWriteDefault )
      {
         watchIDs.insert( _encodeWatchIDs.begin(), _encodeWatchIDs.end() ) ;
      }

      try
      {
         BSONObjIterator itr( record ) ;
         while ( itr.more() )
         {
            BSONElement ele = itr.next() ;
            columnID = _schemaHash.getColumnIDByName( ele.fieldName() ) ;
            if ( DMS_SCHEMA_INVALID_COLUMNID == columnID )
            {
               hasNewColumn = TRUE ;
               goto done ;
            }

            ossItoa( columnID, columnName, sizeof(columnName) ) ;
            rc = encodeBuilder.appendElement( ele.type(), columnName, ossStrlen(columnName),
                                              ele.value(), ele.valuesize() ) ;
            PD_RC_CHECK( rc, PDERROR, "Add column [%s] to encoded record failed, rc: %d",
                         ele.fieldName(), rc ) ;

            if ( hasWriteDefault )
            {
               watchIDs.erase( columnID ) ;
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
      PD_TRACE_EXITRC( SDB__DMSINTERNALSCHEMA__PARSERECORD, rc ) ;
      return rc ;
   error:
      goto done ;
   }


   INT32 _dmsInternalSchema::_appendPrimalColumns( _dmsMBContext *context,
                                                   pmdEDUCB *cb,
                                                   utilBSONRawBuilder &encodeBuilder,
                                                   COLUMN_ID_SET &watchIDs,
                                                   const BSONObj& originalRecord,
                                                   dmsRecordData &recordData )
   {
      INT32 rc = SDB_OK ;
      const CHAR *name = NULL ;
      INT32 nameLen = 0 ;
      BSONType type ;
      const CHAR *value = NULL ;
      INT32 valueLen = 0 ;
      utilBSONRawBuilder builder ;
      BOOLEAN isEmpty = FALSE ;
      CHAR *buff = NULL ;
      INT32 buffSize = originalRecord.objsize() + _defaultMaxSize ;
      CHAR columnName[ DMS_SCHEMA_COLID_STR_MAX_SIZE + 1 ] = { 0 } ;

      rc = cb->allocBuff( buffSize, &buff ) ;
      PD_RC_CHECK( rc, PDERROR, "Allocate memory of size [%d] failed, rc: %d", buffSize, rc ) ;

      rc = builder.start( buff, buffSize, &originalRecord ) ;
      PD_RC_CHECK( rc, PDERROR, "Starting adding columns with default values to record failed, "
                   "rc: %d", rc ) ;

      try
      {
         for ( COLUMN_ID_SET_CITR citr = watchIDs.begin(); citr != watchIDs.end(); ++citr )
         {
            if ( _schemaContainer.hasWriteDefault( *citr ) )
            {
               rc = _schemaContainer.getColWriteDefault( *citr, name, nameLen,
                                                         type, value, valueLen ) ;
               PD_RC_CHECK( rc, PDERROR, "Get write default value of column %s failed, rc: %d",
                            name, rc ) ;
            }
            else if ( _schemaContainer.hasReadDefault( *citr ) )
            {
               // No write value, it should be a column in index with read default.
               rc = _schemaContainer.getColReadDefault( *citr, name, nameLen,
                                                        type, value, valueLen ) ;
               PD_RC_CHECK( rc, PDERROR, "Get read default value of column %s failed, rc: %d",
                            name, rc ) ;
            }
            else
            {
               SDB_ASSERT( FALSE, "Should have read or write default value" ) ;
               rc = SDB_SYS ;
               PD_LOG( PDERROR, "Get primal columns for record failed, rc: %d", rc ) ;
               goto error ;

            }

            ossItoa( *citr, columnName, sizeof(columnName) ) ;
            rc = encodeBuilder.appendElement( type, columnName, ossStrlen(columnName),
                                              value, valueLen ) ;
            PD_RC_CHECK( rc, PDERROR, "Add column [%s] to encoded record failed, rc: %d",
                         name, rc ) ;

            rc = builder.appendElement( type, name, nameLen, value, valueLen ) ;
            PD_RC_CHECK( rc, PDERROR, "Append info for column %s into record failed, rc: %d",
                         name, rc ) ;
         }

         rc = builder.done( isEmpty ) ;
         PD_RC_CHECK( rc, PDERROR, "Build column info record failed, rc: %d", rc ) ;

         recordData.setData( buff, *(INT32 *)buff ) ;
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
      if ( buff )
      {
         cb->releaseBuff( buff ) ;
      }
      goto done ;
   }

   INT32 _dmsInternalSchema::_checkRebuildRecord( const BSONObj &record,
                                                  NAME_INFO_MAP &watchNames )
   {
      INT32 rc = SDB_OK ;

      for ( NAME_DECODE_INFO_ITR itr = watchNames.begin(); itr != watchNames.end(); )
      {
         if ( record.hasField( itr->first ) )
         {
            // If we find the column in the original record, and it's not deleted, and has not
            // been renamed, we should keep the field in the record.
            if ( itr->second._isDeleted || itr->second._isOrigName )
            {
               // If the column is deleted in the schema, we need to delete it from the record.
               // If the name is original name, it has been renamed. We need to rebuild that
               // column with the new name.
               ++itr ;
            }
            else
            {
               watchNames.erase( itr++ ) ;
            }
         }
         else
         {
            // The column is not found in the original record. If the record is deleted, we can
            // ignore it. Otherwise if it has read default value, we should keep it.
            if ( itr->second._isDeleted || !itr->second._hasReadDefault )
            {
               watchNames.erase( itr++ ) ;
            }
            else
            {
               ++itr ;
            }
         }
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _dmsInternalSchema::_appendColWithReadDefault( ossPoolSet<UINT16> &colIDs,
                                                        utilBSONRawBuilder &builder )
   {
      INT32 rc = SDB_OK ;

      const CHAR *name = NULL ;
      INT32 nameLen = 0 ;
      BSONType type ;
      const CHAR *value = NULL ;
      INT32 valueLen = 0 ;

      for ( ossPoolSet<UINT16>::const_iterator citr = colIDs.begin(); citr != colIDs.end(); ++citr )
      {
         rc = _schemaContainer.getColReadDefault( *citr, name, nameLen, type, value, valueLen ) ;
         PD_RC_CHECK( rc, PDERROR, "Get column read default value failed, rc: %d", rc ) ;
         rc = builder.appendElement( type, name, nameLen, value, valueLen ) ;
         if ( rc )
         {
            rc = SDB_CORRUPTED_RECORD ;
            PD_LOG( PDERROR, "Record is corrupted, rc: %d", rc ) ;
            goto error ;
         }
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _dmsInternalSchema::_onSchemaColChanged()
   {
      INT32 rc = SDB_OK ;

      ++_version ;

      try
      {
         ossPoolSet<UINT16> rdDefaultIDs ;
         ossPoolSet<UINT16> wtDefaultIDs ;
         ossPoolSet<UINT16> rdDefaultIndexColIDs ;
         const CHAR *name = NULL ;
         INT32 nameLen = 0 ;
         BOOLEAN isDeleted = FALSE ;
         BOOLEAN hasReadDefault = FALSE ;
         BOOLEAN hasWriteDefault = FALSE ;
         BOOLEAN isIndexColumn = FALSE ;
         BOOLEAN hasOrigName = FALSE ;

         _decodeWatchNames.clear() ;
         _totalValidNameSize = 0 ;

         for ( UINT16 columnID = 0; columnID < _schemaContainer.columnNum(); ++columnID )
         {
            name = NULL ;
            nameLen = 0 ;
            hasReadDefault = FALSE ;
            hasWriteDefault = FALSE ;
            isIndexColumn = FALSE ;
            hasOrigName = FALSE ;
            rc = _schemaContainer.getColumnBasicInfo( columnID, &name, &nameLen, &isDeleted,
                                                      &hasReadDefault, &hasWriteDefault,
                                                      &isIndexColumn, &hasOrigName ) ;
            PD_RC_CHECK( rc, PDERROR, "Get column info in internal schema failed, rc: %d", rc ) ;

            if ( hasOrigName )
            {
               const CHAR *origName = _schemaContainer.getOrigName( columnID ) ;
               _decodeWatchNames.insert(
                  std::make_pair( origName, columnInfo( TRUE, columnID,
                                                        isDeleted, hasReadDefault ) ) ) ;
            }
            else
            {
               _decodeWatchNames.insert(
                  std::make_pair( name, columnInfo( FALSE, columnID,
                                                    isDeleted, hasReadDefault ) ) ) ;
            }
            if ( !isDeleted )
            {
               _totalValidNameSize += nameLen ;
            }
         }

         rc = _schemaContainer.getColIDsWithDefault( rdDefaultIDs, wtDefaultIDs,
                                                     rdDefaultIndexColIDs ) ;
         PD_RC_CHECK( rc, PDERROR, "Get columns with default values in internal schema failed, "
                      "rc: %d", rc ) ;

         _encodeWatchIDs.clear() ;
         _decodeWatchIDs.clear() ;

         _encodeWatchIDs.insert( wtDefaultIDs.begin(), wtDefaultIDs.end() ) ;
         _encodeWatchIDs.insert( rdDefaultIndexColIDs.begin(), rdDefaultIndexColIDs.end() ) ;
         _decodeWatchIDs.insert( rdDefaultIDs.begin(), rdDefaultIDs.end() ) ;

         // Calcuate the max size of all columns with default value.
         {
            COLUMN_ID_SET columnIDsWithDefault ;
            UINT16 columnInfoSize = 0 ;

            _defaultMaxSize = 0 ;

            columnIDsWithDefault.insert( _encodeWatchIDs.begin(), _encodeWatchIDs.end() ) ;
            columnIDsWithDefault.insert( _decodeWatchIDs.begin(), _decodeWatchIDs.end() ) ;
            for ( COLUMN_ID_SET_CITR citr = columnIDsWithDefault.begin();
                  citr != columnIDsWithDefault.end(); ++citr )
            {
               rc = _schemaContainer.getColumnInfoSize( *citr, columnInfoSize ) ;
               PD_RC_CHECK( rc, PDERROR, "Get column info size failed, rc: %d", rc ) ;
               _defaultMaxSize += columnInfoSize ;
            }

#ifdef _DEBUG
            SDB_ASSERT( _defaultMaxSize >= DMS_SCHEMA_COLREC_HEAD_SZ * columnIDsWithDefault.size(),
                        "Total default max size wrong" ) ;
            _logSchemaInfo() ;
#endif /* _DEBUG */
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

   void _dmsInternalSchema::_logSchemaInfo()
   {
      INT32 rc = SDB_OK ;
      BSONObj schemaInfo ;

      rc = dumpSchemaInfo( schemaInfo, TRUE ) ;
      if ( rc )
      {
         PD_LOG( PDWARNING, "Dump internal schema info failed, rc: %d", rc )  ;
      }
      else
      {
         PD_LOG( PDEVENT, "Internal schema: column number %d"OSS_NEWLINE"%s",
                 _schemaContainer.columnNum(),
                 schemaInfo.jsonString( bson::JS, TRUE ).c_str() ) ;
      }
   }

   INT32 _dmsInternalSchema::_encodeSanityCheck( const dmsRecordData &encodedData )
   {
      INT32 rc = SDB_OK ;
      ossPoolSet<UINT16> columnIDs ;

      try
      {
         const BSONObj record( encodedData.data() ) ;
         if ( !record.isValid() )
         {
            SDB_ASSERT( FALSE, "The encoded record is invalid" ) ;
            rc = SDB_SYS ;
            PD_LOG( PDERROR, "The encoded record is invalid, rc: %d", rc ) ;
            goto error ;
         }

         BSONObjIterator itr( record ) ;
         while ( itr.more() )
         {
            UINT16 columnID = ossAtoi( itr.next().fieldName() ) ;
            BOOLEAN isDeleted = FALSE ;
            SDB_ASSERT( DMS_SCHEMA_INVALID_COLUMNID != columnID, "Column ID is invalid" ) ;
            // Should NEVER have duplicated ID in the encoded record.
            if ( FALSE == columnIDs.insert( columnID ).second )
            {
               rc = SDB_SYS ;
               PD_LOG( PDERROR, "Duplicated column ID %u in the encoded record, rc: %d",
                       columnID, rc ) ;
               SDB_ASSERT( FALSE, "Duplicated column ID" ) ;
               goto error ;
            }

            rc = _schemaContainer.getColumnBasicInfo( columnID, NULL, NULL, &isDeleted ) ;
            if ( SDB_OK != rc )
            {
               rc = SDB_SYS ;
               PD_LOG( PDERROR, "Get column info by column ID %u failed, rc: %d", columnID, rc ) ;
               SDB_ASSERT( FALSE, "Get column info by column ID failed" ) ;
               goto error ;
            }

            if ( isDeleted )
            {
               rc = SDB_SYS ;
               PD_LOG( PDERROR, "Should not include deleted column when encoding, rc: %d", rc ) ;
               SDB_ASSERT( FALSE, "Should not include deleted column when encoding" ) ;
               goto error ;
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
}
