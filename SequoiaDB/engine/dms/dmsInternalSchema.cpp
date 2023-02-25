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

#define SCHEMA_COL_TYPE                            CHAR
#define DMS_SCHEMA_INVALID_COLUMNID                0xFFFF

#define DMS_SCHEMA_HASH_BUCKET_SIZE                (4096)

#define DMS_SCHEMA_COL_DELETED                     0x8000
#define DMS_SCHEMA_COL_IN_INDEX                    0x4000
#define DMS_SCHEMA_COL_READ_DEFAULT                0x2000
#define DMS_SCHEMA_COL_WRITE_DEFAULT               0x1000
#define DMS_SCHEMA_COL_HAS_ORIGNAME                0x0800

#define DMS_SCHEMA_HASH_ID_MASK                    0x0000FFFF
#define DMS_SCHEMA_HASH_OFFSET_MASK                0xFFFF0000

#define DMS_SCHEMA_MAX_COLUMN_NUM                  65535
#define DMS_SCHEMA_INVALID_ITEM_ID                 65535
#define DMS_SCHEMA_INVALID_ITEM_OFFSET             65535

#define DMS_SCHEMA_COLID_STR_MAX_SIZE              5

namespace engine
{
   _dmsSchemaContainer::_dmsSchemaContainer()
   : _su( NULL ),
     _mbID( DMS_INVALID_MBID ),
     _extentID( DMS_INVALID_EXTENT ),
     _extent( NULL )
   {
   }

   _dmsSchemaContainer::~_dmsSchemaContainer()
   {
   }

   INT32 _dmsSchemaContainer::init( dmsStorageDataCommon *su, dmsMBContext *context,
                                    dmsExtentID extentID, BOOLEAN isLoad )
   {
      INT32 rc = SDB_OK ;

      _extRW = su->extent2RW( extentID, context->mbID() ) ;
      _extRW.setNothrow( TRUE ) ;
      _extent = _extRW.readPtr< dmsSchemaExtent >() ;
      if ( !_extent || !_extent->validate( context->mbID() ) )
      {
         rc = SDB_DMS_CORRUPTED_EXTENT ;
         PD_LOG( PDERROR, "Internal schema extent is corrupted, extentID: %d, rc: %d",
                 extentID, rc ) ;
         goto error ;
      }

      _su = su ;
      _mbID = context->mbID() ;
      _extentID = extentID ;

      if ( !isLoad )
      {
         UINT32 dataAreaSize = _extent->_blockSize * su->pageSize() - DMS_SCHEMAEXTENT_HEADER_SZ ;
         CHAR *dataAreaPtr = _extRW.writePtr( DMS_SCHEMAEXTENT_HEADER_SZ, dataAreaSize ) ;
         ossMemset( dataAreaPtr, 0, dataAreaSize ) ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   void _dmsSchemaContainer::reset()
   {
      _su = NULL ;
      _mbID = DMS_INVALID_MBID ;
      _extentID = DMS_INVALID_EXTENT ;
      _extent = NULL ;
      _extRW = dmsExtRW() ;
   }

   INT32 _dmsSchemaContainer::alterColumn( UINT16 columnID, const BSONObj &columnInfo )
   {
      INT32 rc = SDB_OK ;

      SDB_ASSERT( DMS_SCHEMA_INVALID_COLUMNID != columnID, "Column id is invalid" ) ;

      BSONElement ele ;
      dmsSchemaColRecBuilder builder ;
      INT16 attr = _getColumnAttr( columnID ) ;
      INT16 newAttr = attr ;
      const dmsSchemaColRecord *oldColRecord = _getColRecord( columnID ) ;
      const dmsSchemaColRecord *newColRecord = NULL ;
      const CHAR *name = oldColRecord->getName() ;
      SDB_ASSERT( oldColRecord, "Original column info record is null" ) ;

      rc = builder.startRebuild( oldColRecord ) ;
      PD_RC_CHECK( rc, PDERROR, "Build new column info for %s failed, rc: %d", name, rc ) ;

      rc = builder.addColumnName( oldColRecord->getName() ) ;
      PD_RC_CHECK( rc, PDERROR, "Add column name %s to new column info failed, rc: %d",
                   name, rc ) ;

      try
      {
         ele = columnInfo.getField( FIELD_NAME_READDEFAULT ) ;
         if ( !ele.eoo() )
         {
            // Read default can not be changed.
            if ( OSS_BIT_TEST( attr, DMS_SCHEMA_COL_READ_DEFAULT ) )
            {
//               BSONType readDefaultType = EOO ;
//               INT32 readDefaultSize = 0 ;
//               const CHAR *readDefaultValue = NULL ;
//               if ( ( oldColRecord->getDefault( readDefaultType,
//                                                readDefaultSize,
//                                                readDefaultValue,
//                                                TRUE ) ) &&
//                    ( readDefaultType == ele.type() ) &&
//                    ( readDefaultSize == ele.valuesize() ) &&
//                    ( 0 == ossMemcmp( readDefaultValue,
//                                      ele.value(),
//                                      readDefaultSize ) ) )
//               {
//                  // skip
//                  PD_LOG( PDDEBUG, "Got the same read default of column %s",
//                          name ) ;
//               }
//               else
//               {
//                  rc = SDB_OPERATION_INCOMPATIBLE ;
//                  PD_LOG( PDERROR, "Can not change read default of column %s, "
//                          "rc: %d", name, rc ) ;
//                  goto error ;
//               }
               // skip
               PD_LOG( PDEVENT, "Skip the read default of column %s",
                       name ) ;
            }
            else
            {
               rc = builder.updateReadDefault( ele.type(), ele.value(), ele.valuesize() ) ;
               PD_RC_CHECK( rc, PDERROR, "Add read default for column %s to new column info "
                            "failed, rc: %d", name, rc ) ;
               newAttr |= DMS_SCHEMA_COL_READ_DEFAULT ;
            }
         }

         ele = columnInfo.getField( FIELD_NAME_WRITEDEFAULT ) ;
         if ( !ele.eoo() )
         {
            rc = builder.updateWriteDefault( ele.type(), ele.value(), ele.valuesize() ) ;
            PD_RC_CHECK( rc, PDERROR, "Add write default for column %s to new column info "
                         "failed, rc: %d", name, rc ) ;
            if ( !OSS_BIT_TEST( attr, DMS_SCHEMA_COL_WRITE_DEFAULT ) )
            {
               newAttr |= DMS_SCHEMA_COL_WRITE_DEFAULT ;
            }
         }

         name = oldColRecord->getOrigName() ;
         if ( name )
         {
            rc = builder.addColumnName( name, TRUE ) ;
            PD_RC_CHECK( rc, PDERROR, "Add original name %s to new column info failed, rc: %d",
                         rc ) ;
         }

         rc = builder.finishRebuild() ;
         PD_RC_CHECK( rc, PDERROR, "Build new info record for column %s failed, rc: %d",
                      name, rc ) ;

         newColRecord = builder.getRecord() ;

         // Do not use oldColRecord once the update is done.
         rc = _updateColRecord( columnID, oldColRecord, newColRecord ) ;
         PD_RC_CHECK( rc, PDERROR, "Update info of column %s in internal schema failed, rc: %d",
                      newColRecord->getName(), rc ) ;

         if ( newAttr != attr )
         {
            _setColumnAttr( columnID, newAttr ) ;
         }

         flush() ;
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

   INT32 _dmsSchemaContainer::dropColumn( UINT16 columnID )
   {
      _setColumnAttr( columnID, DMS_SCHEMA_COL_DELETED ) ;
      flush() ;

      return SDB_OK ;
   }

   INT32 _dmsSchemaContainer::dropColumnDefault( UINT16 columnID, BOOLEAN dropWrite,
                                                 BOOLEAN dropRead )
   {
      INT32 rc = SDB_OK ;
      INT16 attr = 0 ;
      dmsSchemaColRecBuilder builder ;
      const dmsSchemaColRecord *newRecord = NULL ;
      const dmsSchemaColRecord *oldRecord = _getColRecord( columnID ) ;

      rc = builder.startRebuild( oldRecord ) ;
      PD_RC_CHECK( rc, PDERROR, "Start building schema column info failed, rc: %d", rc ) ;

      // Set the flag bits we want to clear.
      if ( dropWrite )
      {
         OSS_BIT_SET( attr, DMS_SCHEMA_COL_WRITE_DEFAULT ) ;
         builder.dropWriteDefault() ;
      }

      if ( dropRead )
      {
         OSS_BIT_SET( attr, DMS_SCHEMA_COL_READ_DEFAULT ) ;
         builder.dropReadDefault() ;
      }

      rc = builder.finishRebuild() ;
      PD_RC_CHECK( rc, PDERROR, "Build new column info when dropping default failed, rc: %d", rc ) ;

      newRecord = builder.getRecord() ;

      SDB_ASSERT( newRecord->getLength() <= oldRecord->getLength(), "Length is wrong" ) ;

      {
         UINT16 offset = _getColRecordOffset( columnID ) ;
         CHAR *writePtr = _extRW.writePtr( offset, newRecord->getLength() ) ;
         ossMemcpy( writePtr, newRecord, newRecord->getLength() ) ;
      }

      _clearColumnAttr( columnID, attr ) ;

      flush() ;

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _dmsSchemaContainer::renameColumn( UINT16 columnID, const CHAR *newName )
   {
      // Keep the id of the column unchanged, just change the column information. The size of the
      // column information may grow, and need another place to store. So the offset may change.
      // If the original space is enough, just do in-place update.

      INT32 rc = SDB_OK ;
      INT16 attr = _getColumnAttr( columnID ) ;
      dmsSchemaColRecBuilder builder ;
      const dmsSchemaColRecord *newRecord = NULL ;
      const dmsSchemaColRecord *oldRecord = _getColRecord( columnID ) ;

      rc = builder.startRebuild( oldRecord ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to start renaming column %s in internal schema, rc: %d",
                   oldRecord->getName(), rc ) ;

      rc = builder.updateName( newName ) ;
      PD_RC_CHECK( rc, PDERROR, "Update column name to %s in column info builder failed, rc: %d",
                   newName, rc ) ;

      rc = builder.finishRebuild() ;
      PD_RC_CHECK( rc, PDERROR, "Finish rename column to %s failed, rc: %d", newName, rc ) ;

      newRecord = builder.getRecord() ;

      rc = _updateColRecord( columnID, oldRecord, newRecord ) ;
      PD_RC_CHECK( rc, PDERROR, "Update column info in internal schema failed, rc: %d", rc ) ;

      if ( !OSS_BIT_TEST( attr, DMS_SCHEMA_COL_HAS_ORIGNAME ) )
      {
         _setColumnAttr( columnID, DMS_SCHEMA_COL_HAS_ORIGNAME ) ;
      }

      flush() ;

   done:
      return rc ;
   error:
      goto done ;
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

   void _dmsSchemaContainer::setIndexColumn( UINT16 columnID )
   {
      _setColumnAttr( columnID, DMS_SCHEMA_COL_IN_INDEX ) ;
   }

   void _dmsSchemaContainer::unsetIndexColumn( UINT16 columnID )
   {
      _clearColumnAttr( columnID, DMS_SCHEMA_COL_IN_INDEX ) ;
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

   INT32 _dmsSchemaContainer::addColumn( const CHAR *name, const BSONObj *columnDef,
                                         UINT16 &columnID, const CHAR *origName )
   {
      INT32 rc = SDB_OK ;
      UINT16 recOffset = 0 ;
      UINT16 recLength = 0 ;
      UINT16 allocSize = 0 ;
      INT16 attr = 0 ;
      CHAR *writePtr = NULL ;
      dmsSchemaExtent *schemaExtent = NULL ;
      dmsSchemaColRecBuilder builder ;
      const dmsSchemaColRecord *colInfoRec = NULL ;

      SDB_ASSERT( name, "Column name is invalid" ) ;

      rc = builder.startBuild() ;
      PD_RC_CHECK( rc, PDERROR, "Star build internal schema column info failed, rc: %d", rc ) ;

      rc = builder.addColumnName( name ) ;
      PD_RC_CHECK( rc, PDERROR, "Add column name %s of internal schema into builde buffer failed, "
                   "rc: %d", name, rc ) ;

      // When the internal schema is evolved because of data inserting, the column definition is
      // empty.
      if ( columnDef && !columnDef->isEmpty() )
      {
         BSONElement ele = columnDef->getField( FIELD_NAME_READDEFAULT ) ;
         if ( !ele.eoo() )
         {
            rc = builder.addDefault( ele.type(), ele.value(), ele.valuesize() ) ;
            PD_RC_CHECK( rc, PDERROR, "Add read default value of column into builder buffer "
                         "failed, rc: %d", rc ) ;
            OSS_BIT_SET( attr, DMS_SCHEMA_COL_READ_DEFAULT ) ;
         }

         ele = columnDef->getField( FIELD_NAME_WRITEDEFAULT ) ;
         if ( !ele.eoo() )
         {
            rc = builder.addDefault( ele.type(), ele.value(), ele.valuesize(), TRUE ) ;
            PD_RC_CHECK( rc, PDERROR, "Add write default value of column into builder buffer "
                         "failed, rc: %d", rc ) ;
            OSS_BIT_SET( attr, DMS_SCHEMA_COL_WRITE_DEFAULT ) ;
         }
      }

      if ( origName )
      {
         builder.addColumnName( origName, TRUE ) ;
         OSS_BIT_SET( attr, DMS_SCHEMA_COL_HAS_ORIGNAME ) ;
      }

      builder.finishBuild() ;
      colInfoRec = builder.getRecord() ;
      recLength = colInfoRec->getLength() ;

      rc = _allocSpace4ColRecord( DMS_SCHEMAEXTENT_SLOT_SZ, recLength, recOffset, &allocSize ) ;
      PD_RC_CHECK( rc, PDERROR, "Allocate space for column infor in internal schema extent failed, "
                   "rc: %d", rc ) ;

      writePtr = _extRW.writePtr( recOffset, recLength ) ;
      ossMemcpy( writePtr, (CHAR *)colInfoRec, recLength ) ;

      columnID = _extent->_itemNum ;
      _initColAttrAndRecordOffset( columnID, attr, recOffset ) ;

      // Increase the item number at last and flush immediately.
      schemaExtent = _extRW.writePtr<dmsSchemaExtent>() ;
      schemaExtent->_valueOffset = recOffset ;
      ++schemaExtent->_itemNum ;
      schemaExtent->_freeSpace -= allocSize ;

      flush() ;

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

   const CHAR *_dmsSchemaContainer::getColumnName( UINT16 columnID, INT32 *nameLen ) const
   {
      const dmsSchemaColRecord *record = _getColRecord( columnID ) ;
      const CHAR *name = record->getName( nameLen ) ;
      return name ;
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

   INT32 _dmsSchemaContainer::flush()
   {
      INT32 rc = SDB_OK ;

      rc = _su->flushPages( _extentID, _extent->_blockSize, TRUE ) ;
      PD_RC_CHECK( rc, PDERROR, "Flush internal schema extent failed, rc: %d", rc ) ;

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
      UINT32 buffSize = record->getLength() + 64 ;  // Extra space for field names.

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

   INT16 _dmsSchemaContainer::_getColumnAttr( UINT16 columnID ) const
   {
      const INT32* attr =
         (INT32 *)_extRW.readPtr( DMS_SCHEMAEXTENT_HEADER_SZ + DMS_SCHEMAEXTENT_SLOT_SZ * columnID,
                                  DMS_SCHEMAEXTENT_HEADER_SZ ) ;

      return (INT16)(*attr >> 16) ;
   }

   void _dmsSchemaContainer::_setColRecordOffset( UINT16 columnID, UINT16 offset )
   {
      INT32* slot =
         (INT32 *)_extRW.writePtr( DMS_SCHEMAEXTENT_HEADER_SZ + DMS_SCHEMAEXTENT_SLOT_SZ * columnID,
                                   DMS_SCHEMAEXTENT_HEADER_SZ ) ;
      *slot = ( (*slot) & 0xFFFF0000 ) | (INT32)offset ;
   }

   UINT16 _dmsSchemaContainer::_getColRecordOffset( UINT16 columnID ) const
   {
      const INT32* slot =
         (INT32 *)_extRW.readPtr( DMS_SCHEMAEXTENT_HEADER_SZ + DMS_SCHEMAEXTENT_SLOT_SZ * columnID,
                                  DMS_SCHEMAEXTENT_HEADER_SZ ) ;
      return (UINT16)( (*slot) & 0x0000FFFF ) ;
   }

   void _dmsSchemaContainer::_initColAttrAndRecordOffset( UINT16 columnID, INT16 attr,
                                                         UINT16 valOffset )
   {
      INT32* slot =
         (INT32 *)_extRW.readPtr( DMS_SCHEMAEXTENT_HEADER_SZ + DMS_SCHEMAEXTENT_SLOT_SZ * columnID,
                                  DMS_SCHEMAEXTENT_HEADER_SZ ) ;
      *slot = 0 ;
      *slot |= (((INT32)attr) << 16 ) ;
      *slot = ((*slot) & 0xFFFF0000) | (INT32)valOffset ;
   }

   void _dmsSchemaContainer::_getColAttrAndRecordOffset( UINT16 columnID, INT16 &attr,
                                                         UINT16 &valueOffset ) const
   {
      const INT32* slot =
         (INT32 *)_extRW.readPtr( DMS_SCHEMAEXTENT_HEADER_SZ + DMS_SCHEMAEXTENT_SLOT_SZ * columnID,
                                  DMS_SCHEMAEXTENT_HEADER_SZ ) ;
      attr = (INT16)( *slot >> 16 ) ;
      valueOffset = (UINT16)( (*slot) & 0x0000FFFF ) ;
   }

   INT32 _dmsSchemaContainer::_allocSpace4ColRecord( UINT16 slotSize, UINT16 valueSize,
                                                     UINT16 &offset, UINT16 *allocSize )
   {
      INT32 rc = SDB_OK ;
      UINT16 requiredSize = ossRoundUpToMultipleX( valueSize, 4 ) + slotSize ;
      const dmsSchemaExtent *schemaExtent = _extRW.readPtr<dmsSchemaExtent>() ;

      if ( _freeSpace() < requiredSize )
      {
         // TODO: YSD add another extent ;
         // Try to compact

         rc = SDB_SYS ;
         PD_LOG( PDERROR, "Too much column info in internal schema, rc: %d", rc ) ;
         goto error ;
      }

      if ( 0 == schemaExtent->_valueOffset )
      {
         offset = ( schemaExtent->_blockSize << _su->pageSizeSquareRoot() ) - requiredSize ;
      }
      else
      {
         offset = schemaExtent->_valueOffset - requiredSize ;
      }

      if ( allocSize )
      {
         *allocSize = requiredSize ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   void _dmsSchemaContainer::_setColumnAttr( UINT16 columnID, INT16 flags )
   {
      INT32* slot =
         (INT32 *)_extRW.writePtr( DMS_SCHEMAEXTENT_HEADER_SZ + DMS_SCHEMAEXTENT_SLOT_SZ * columnID,
                                   DMS_SCHEMAEXTENT_HEADER_SZ ) ;
      OSS_BIT_SET( *slot, ((INT32)flags) << 16 ) ;
   }

   void _dmsSchemaContainer::_clearColumnAttr( UINT16 columnID, INT16 flags )
   {
      INT32* slot =
         (INT32 *)_extRW.writePtr( DMS_SCHEMAEXTENT_HEADER_SZ + DMS_SCHEMAEXTENT_SLOT_SZ * columnID,
                                   DMS_SCHEMAEXTENT_HEADER_SZ ) ;
      OSS_BIT_CLEAR( *slot, ((INT32)flags << 16 ) ) ;
   }

   const dmsSchemaColRecord *_dmsSchemaContainer::_getColRecord( UINT16 columnID ) const
   {
      const INT32* slot =
         (INT32 *)_extRW.readPtr( DMS_SCHEMAEXTENT_HEADER_SZ + DMS_SCHEMAEXTENT_SLOT_SZ * columnID,
                                  DMS_SCHEMAEXTENT_HEADER_SZ ) ;

      return (const dmsSchemaColRecord *)_extRW.readPtr( (UINT16)((*slot) & 0x0000FFFF), 0 ) ;      // TODO: YSD change the length???
   }

   INT32 _dmsSchemaContainer::_updateColRecord( UINT16 columnID,
                                                const dmsSchemaColRecord *oldRecord,
                                                const dmsSchemaColRecord *newRecord )
   {
      INT32 rc = SDB_OK ;

      SDB_ASSERT( oldRecord && newRecord, "Record is null" ) ;

      if ( newRecord->getLength() <= oldRecord->getLength() )
      {
         ossMemcpy( (CHAR *)oldRecord, newRecord, newRecord->getLength() ) ;
         dmsSchemaExtent *extent = _extRW.writePtr<dmsSchemaExtent>() ;
         if ( newRecord->getLength() < oldRecord->getLength() )
         {
            extent->_freeSpace += ( oldRecord->getLength() - newRecord->getLength() ) ;
         }
      }
      else
      {
         // Need to allocate new space to store the new record.
         // 1. Allocate new space.
         // 2. Copy the new record to the space.
         // 3. Update the offset in the column slot, and flush immediately.
         UINT16 newOffset = 0 ;
         CHAR *writePtr = NULL ;
         UINT16 allocSize = 0 ;

         rc = _allocSpace4ColRecord( DMS_SCHEMAEXTENT_SLOT_SZ, newRecord->getLength(),
                                     newOffset, &allocSize ) ;
         PD_RC_CHECK( rc, PDERROR, "Allocate space for new column infor in internal schema extent "
                      "failed, rc: %d", rc ) ;

         writePtr = _extRW.writePtr( newOffset, newRecord->getLength() ) ;

         ossMemcpy( writePtr, (CHAR *)newRecord, newRecord->getLength() ) ;
         _setColRecordOffset( columnID, newOffset ) ;

         {
            // 修改为内存模式后，这个应该放到 _allocSpace4ColRecord 里面
            dmsSchemaExtent *extent = _extRW.writePtr< dmsSchemaExtent >() ;
            extent->_valueOffset = newOffset ;
            extent->_freeSpace += oldRecord->getLength() - allocSize ;
         }
      }

   done:
      return rc ;
   error:
      goto done ;
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

   UINT32 _dmsSchemaContainer::_freeSpace() const
   {
      UINT32 freeSpace = 0 ;

      if ( 0 == _extent->_itemNum )
      {
         // No item yet
         freeSpace = _extent->_blockSize * _su->pageSize() - DMS_SCHEMAEXTENT_HEADER_SZ ;
      }
      else
      {
         freeSpace = _extent->_valueOffset - DMS_SCHEMAEXTENT_HEADER_SZ -
                     DMS_SCHEMAEXTENT_SLOT_SZ * _extent->_itemNum ;
      }
      return freeSpace ;
   }

   INT32 _dmsSchemaContainer::_compact()
   {
      INT32 rc = SDB_OK ;

   done:
      return rc ;
   error:
      goto done ;
   }

   _dmsSchemaHash::_dmsSchemaHash( dmsSchemaContainer *schemaContainer )
   : _schemaContainer( schemaContainer ),
     _su( NULL ),
     _mbID( DMS_INVALID_MBID ),
     _extentID( DMS_INVALID_EXTENT ),
     _extent( NULL ),
     _nextFreeItemOffset( DMS_SCHEMA_HASH_BUCKET_SIZE )
   {
   }

   _dmsSchemaHash::~_dmsSchemaHash()
   {
   }

   INT32 _dmsSchemaHash::init( _dmsStorageDataCommon *su, _dmsMBContext *context,
                               dmsExtentID extentID, BOOLEAN isLoad )
   {
      INT32 rc = SDB_OK ;
      const dmsSchemaHashExtent *extent = NULL ;

      _extRW = su->extent2RW( extentID, context->mbID() ) ;
      _extRW.setNothrow( TRUE ) ;
      extent = _extRW.readPtr< dmsSchemaHashExtent >() ;
      if ( !extent || !extent->validate( context->mbID() ) )
      {
         rc = SDB_DMS_CORRUPTED_EXTENT ;
         PD_LOG( PDERROR, "Schema hash extent is corrupted, extentID: %d, rc: %d", extentID, rc ) ;
         goto error ;
      }

      _su = su ;
      _mbID = context->mbID() ;
      _extentID = extentID ;
      _extent = extent ;

      if ( !isLoad )
      {
         UINT32 dataAreaSize = _extent->_blockSize * su->pageSize() -
                               DMS_SCHEMAHASHEXTENT_HEADER_SZ ;
         CHAR *dataAreaPtr = _extRW.writePtr( DMS_SCHEMAHASHEXTENT_HEADER_SZ, dataAreaSize ) ;
         ossMemset( dataAreaPtr, 0xFF, dataAreaSize ) ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   void _dmsSchemaHash::reset()
   {
      _su = NULL ;
      _mbID = DMS_INVALID_MBID ;
      _extentID = DMS_INVALID_EXTENT ;
      _extent = NULL ;
      _extRW = dmsExtRW() ;
      _nextFreeItemOffset = DMS_SCHEMA_HASH_BUCKET_SIZE ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSSCHEMAHASH_GETCOLUMNIDBYNAME, "_dmsSchemaHash::getColumnIDByName" )
   UINT16 _dmsSchemaHash::getColumnIDByName( const CHAR *name )
   {
      PD_TRACE_ENTRY( SDB__DMSSCHEMAHASH_GETCOLUMNIDBYNAME ) ;
      INT32 *currItem = NULL ;
      UINT16 columnID = DMS_SCHEMA_INVALID_COLUMNID ;
      const CHAR *columnName = NULL ;

      UINT32 itemID = ossHash( name ) % DMS_SCHEMA_HASH_BUCKET_SIZE ;   // Bucket is also an item.
      UINT16 itemOffset = DMS_SCHEMAHASHEXTENT_HEADER_SZ + DMS_SCHEMAHASHEXTENT_ITEM_SZ * itemID ;

      SDB_ASSERT( name, "Column name is null" ) ;

      while ( DMS_SCHEMA_INVALID_ITEM_OFFSET != itemOffset )
      {
         currItem = _itemOffset2Ptr( itemOffset ) ;
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

      PD_TRACE_EXIT( SDB__DMSSCHEMAHASH_GETCOLUMNIDBYNAME ) ;
      return columnID ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSSCHEMAHASH_ADDCOLUMNITEM, "_dmsSchemaHash::addColumnItem" )
   INT32 _dmsSchemaHash::addColumnItem( const CHAR *name, UINT16 columnID )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__DMSSCHEMAHASH_ADDCOLUMNITEM ) ;
      INT32 *item = NULL ;
      INT32 *prevItem = NULL ;

      SDB_ASSERT( DMS_INVALID_EXTENT != _extentID, "Schema hash extent id is invalid" ) ;

      INT32 itemID = ossHash( name ) % DMS_SCHEMA_HASH_BUCKET_SIZE ;  // The bucket is also an item.
      UINT16 nextItemOffset = DMS_SCHEMAHASHEXTENT_HEADER_SZ + DMS_SCHEMAHASHEXTENT_ITEM_SZ * itemID ;

      while ( TRUE )
      {
         item = (INT32 *)_extRW.writePtr( nextItemOffset, DMS_SCHEMAHASHEXTENT_ITEM_SZ ) ;
         if ( DMS_SCHEMA_INVALID_COLUMNID == _getColumnIDByItem( item ) )
         {
            // Not used.
            _setColumnIDInItem( item, columnID ) ;
            _setNextItemOffset( item, DMS_SCHEMA_INVALID_ITEM_ID ) ;
            if ( prevItem )
            {
               _setNextItemOffset( prevItem, nextItemOffset ) ;
            }
            break ;
         }
         else
         {
            nextItemOffset = _getNextItemOffset( item ) ;
            if ( DMS_SCHEMA_INVALID_ITEM_ID == nextItemOffset )
            {
               // Reach
               prevItem = item ;
               nextItemOffset = _nextFreeItemOffset ;
               _nextFreeItemOffset += DMS_SCHEMAHASHEXTENT_ITEM_SZ ;
            }
         }
      }

   done:
      PD_TRACE_EXITRC( SDB__DMSSCHEMAHASH_ADDCOLUMNITEM, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   INT32 _dmsSchemaHash::dropColumnItemByName( const CHAR *name )
   {
      INT32 rc = SDB_OK ;
      INT32 *item = NULL ;
      INT32 *prevItem = NULL ;
      const CHAR *columnName = NULL ;
      UINT16 columnID = DMS_SCHEMA_INVALID_COLUMNID ;

      UINT32 itemID = ossHash( name ) % DMS_SCHEMA_HASH_BUCKET_SIZE ;
      UINT16 itemOffset = DMS_SCHEMAHASHEXTENT_HEADER_SZ + DMS_SCHEMAHASHEXTENT_ITEM_SZ * itemID ;

      while ( DMS_SCHEMA_INVALID_ITEM_OFFSET != itemOffset )
      {
         item = _itemOffset2Ptr( itemOffset ) ;
         columnID = _getColumnIDByItem( item ) ;
         columnName = _schemaContainer->getColumnName( columnID ) ;
         if ( 0 == ossStrcmp( name, columnName ) )
         {
            // Found
            break ;
         }
         else
         {
            prevItem = item ;
            itemOffset = _getNextItemOffset( item ) ;
         }
      }

      if ( DMS_SCHEMA_INVALID_ITEM_OFFSET == itemOffset )
      {
         // Not found
         goto done ;
      }
      else
      {
         // Check if the current item has a next item.
         UINT16 nextItemOffset = _getNextItemOffset( item ) ;
         if ( prevItem )
         {
            _setNextItemOffset( prevItem, _getNextItemOffset( item ) ) ;
            _resetItem( item ) ;
         }
         else if ( DMS_SCHEMA_INVALID_ITEM_OFFSET == nextItemOffset )
         {
            // No next, only this one.
            _resetItem( item ) ;
         }
         else
         {
            INT32 *nextItem = _itemOffset2Ptr( nextItemOffset ) ;
            *item = *nextItem ;
            _resetItem( nextItem ) ;
         }
      }

   done:
      return rc ;
   }

   INT32 _dmsSchemaHash::flush()
   {
      INT32 rc = SDB_OK ;

      if ( _su )
      {
         rc = _su->flushPages( _extentID, _extent->_blockSize, TRUE ) ;
         PD_RC_CHECK( rc, PDERROR, "Flush internal schema hash table extent failed, rc: %d", rc ) ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   void _dmsSchemaHash::_setColumnIDInItem( INT32 *item, UINT16 columnID )
   {
      *item = ( (*item) & DMS_SCHEMA_HASH_OFFSET_MASK ) | (UINT32)columnID ;
   }

   UINT16 _dmsSchemaHash::_getColumnIDByItem( INT32 *item )
   {
      return (UINT16)( (*item) & DMS_SCHEMA_HASH_ID_MASK ) ;
   }

   INT32 _dmsSchemaHash::_getItemByName( const CHAR *name, INT32 *&item, INT32 **prevItem )
   {
      INT32 rc = SDB_OK ;

      INT32 itemID = ossHash( name ) % DMS_SCHEMA_HASH_BUCKET_SIZE ;
      INT32 offset = DMS_SCHEMAEXTENT_HEADER_SZ + DMS_SCHEMAHASHEXTENT_ITEM_SZ * itemID ;

      while ( TRUE )
      {
         item = (INT32 *)_extRW.readPtr( offset, DMS_SCHEMAHASHEXTENT_ITEM_SZ ) ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   void _dmsSchemaHash::_setNextItemOffset( INT32 *item, UINT16 offset )
   {
      *item = ( (*item) & DMS_SCHEMA_HASH_ID_MASK ) | ( ((INT32)offset) << 16 ) ;
   }

   UINT16 _dmsSchemaHash::_getNextItemOffset( INT32 *item )
   {
      return (UINT16)( (*item) >> 16 ) ;
   }

   _dmsInternalSchema::_dmsInternalSchema()
   : _enabled( FALSE ),
     _version( DMS_SCHEMA_INVALID_VERSION ),
     _schemaHash( &_schemaContainer ),
     _defaultMaxSize( 0 ),
     _totalValidNameSize( 0 )
   {
   }

   _dmsInternalSchema::~_dmsInternalSchema()
   {
   }

   INT32 _dmsInternalSchema::init( _dmsStorageDataCommon *su, dmsMBContext *context,
                                   dmsExtentID schemaExtentID, dmsExtentID schemaHashExtentID,
                                   BOOLEAN isLoad )
   {
      INT32 rc = SDB_OK ;

      SDB_ASSERT( su, "su is NULL" ) ;
      SDB_ASSERT( context, "context is NULL" ) ;

      if ( !context->isMBLock( EXCLUSIVE ) )
      {
         rc = SDB_SYS ;
         PD_LOG( PDERROR, "Collection mb latch should be taken when initializing internal schema, "
                 "rc: %d", rc ) ;
         goto error ;
      }

      rc = _schemaContainer.init( su, context, schemaExtentID, isLoad ) ;
      PD_RC_CHECK( rc, PDERROR, "Initialize schema container failed, rc: %d", rc ) ;

      rc = _schemaHash.init( su, context, schemaHashExtentID, isLoad ) ;
      PD_RC_CHECK( rc, PDERROR, "Initialize schema hash table failed, rc: %d", rc ) ;

      _onSchemaColChanged() ;
      _enabled = TRUE ;

   done:
      return rc ;
   error:
      goto done ;
   }

   void _dmsInternalSchema::reset()
   {
      _schemaContainer.reset() ;
      _schemaHash.reset() ;
      _readDefaultIDsOfIndexCol.clear() ;
      _encodeWatchIDs.clear() ;
      _decodeWatchIDs.clear() ;
      _enabled = FALSE ;
      _version = DMS_SCHEMA_INVALID_VERSION ;
      _defaultMaxSize = 0 ;
      _totalValidNameSize = 0 ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSINTERNALSCHEMA_ADDCOLUMN, "_dmsInternalSchema::addColumn" )
   INT32 _dmsInternalSchema::addColumn( dmsMBContext *context, const CHAR *columnName,
                                        const BSONObj *columnDef, UINT16 *columnID,
                                        BOOLEAN mergeOnExist, const CHAR *origName )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__DMSINTERNALSCHEMA_ADDCOLUMN ) ;
      UINT16 id = 0 ;

      // The column may exist in the internal schema already. In that case, just return succeed.
      id = _schemaHash.getColumnIDByName( columnName ) ;
      if ( DMS_SCHEMA_INVALID_COLUMNID != id )
      {
         if ( mergeOnExist )
         {
            if ( columnDef )
            {
               rc = _merge2Column( id, *columnDef ) ;
               PD_RC_CHECK( rc, PDERROR, "Merge column info into existing definition failed: %d",
                            rc ) ;
               _onSchemaColChanged() ;
            }
            goto done ;
         }
         else
         {
            rc = SDB_SCHEMA_EXIST ;
            PD_LOG( PDDEBUG, "Column [%s] exists when trying to add it", columnName ) ;
            goto error ;
         }
      }

      // Only add columns with read or/and write default values into internal schema.
      if ( columnDef &&
           !( columnDef->hasField( FIELD_NAME_READDEFAULT ) ||
              columnDef->hasField( FIELD_NAME_WRITEDEFAULT ) ) )
      {
         PD_LOG( PDDEBUG, "Column %s has no default values and will not be added into internal "
                 "schema", columnName ) ;
         goto done ;
      }

      rc = _schemaContainer.addColumn( columnName, columnDef, id, origName ) ;
      PD_RC_CHECK( rc, PDERROR, "Add column info into internal schema failed, rc: %d", rc ) ;

      rc = _schemaHash.addColumnItem( columnName, id ) ;
      PD_RC_CHECK( rc, PDERROR, "Add column into internal schema hash table failed, rc: %d", rc ) ;

      _onSchemaColChanged() ;

      if ( columnID )
      {
         *columnID = id ;
      }

   done:
      PD_TRACE_EXITRC( SDB__DMSINTERNALSCHEMA_ADDCOLUMN, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   INT32 _dmsInternalSchema::dropColumn( dmsMBContext *context, const CHAR *columnName )
   {
      INT32 rc = SDB_OK ;
      UINT16 columnID = DMS_SCHEMA_INVALID_COLUMNID ;

      // Get ID of the column from the hash table.
      columnID = _schemaHash.getColumnIDByName( columnName ) ;
      if ( DMS_SCHEMA_INVALID_COLUMNID == columnID )
      {
         PD_LOG( PDDEBUG, "Column %s does not exist when dropping", columnName ) ;
         goto done ;
      }

      rc = _schemaHash.dropColumnItemByName( columnName ) ;
      PD_RC_CHECK( rc, PDERROR, "Drop column item in schema hash table for column %s failed, "
                   "rc: %d", columnName, rc ) ;

      // mark as deleted in the schema container
      rc = _schemaContainer.dropColumn( columnID ) ;
      PD_RC_CHECK( rc, PDERROR, "Drop column information in internal schema failed, rc: %d", rc ) ;

      _onSchemaColChanged() ;

   done:
      return rc ;
   error:
      goto done ;
   }

   // TODO: YSD 如果记录中本来有 a，且是开启内部模式前就有的数据，定义模式时定义了字段 b，然后改成 a，这
   // 个 a 与记录中的 a 的关系???
   INT32 _dmsInternalSchema::renameColumn( dmsMBContext *context, const CHAR *oldName,
                                           const CHAR *newName, BOOLEAN *oldColumnFound )
   {
      // If the column has been renamed before, directly change the current name.
      // If not, need to store the original name, to handle the records which are not encoded and
      // contain the old column name.

      // 1. Check if the old column name exists in the internal schema. If not, return SDB_OK.
      //    Otherwise, goto step 2.
      // 2. Check if the new column name exists in the internal schema already. If yes, return
      //    error. Otherwise, goto step 3.
      // 3. Get the information of the old column name. If the new name will cause the length of the
      //    column information getting bigger, we need to allocate new space for it, and update the
      //    offset in the slot. Otherwise, we can do in-place update.
      //    3.1) If the column has origin name(had been renamed before), and the new name is shorter
      //       or equal to the old name, do in-place update.
      //    3.2) Otherwise, allocate new space.
      //         a) Calculate the new length of the column information.
      //         b) Allocate new space for the new information.
      //         c) Write the new information in the new allocate space.
      //         d) Update the offset in the slot.
      // 4. Update the entry for the new column in the hash table.
      //

      INT32 rc = SDB_OK ;
      UINT16 columnID = DMS_SCHEMA_INVALID_COLUMNID ;

      SDB_ASSERT( oldName && newName, "Name invalid" ) ;

      // 1. Check if the column with the old name exists, and the column with the new name does not
      //    exist.
      columnID = _schemaHash.getColumnIDByName( oldName ) ;
      if ( DMS_SCHEMA_INVALID_COLUMNID == columnID )
      {
         // For internal schema, it's normal that the name we want to rename does not exist.
         PD_LOG( PDDEBUG, "Old name [%s] does not exist in the local internal schema when "
                 "renaming, rc: %d", oldName, rc ) ;
         if ( oldColumnFound )
         {
            *oldColumnFound = FALSE ;
         }
         goto done ;
      }

      if ( oldColumnFound )
      {
         *oldColumnFound = TRUE ;
      }

      if ( DMS_SCHEMA_INVALID_COLUMNID != _schemaHash.getColumnIDByName( newName ) )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG( PDERROR, "Can not rename %s to %s as the targe name already exist, rc: %d",
                 oldName, newName, rc ) ;
         goto error ;
      }

      // 2. Drop the entry in the hash table.
      rc = _schemaHash.dropColumnItemByName( oldName ) ;
      PD_RC_CHECK( rc, PDERROR, "Drop schema hash entry when rename internal schema column %s "
                  "failed, rc: %d", oldName, rc ) ;

      // 3. Rename the column and update the column information in schema extent.
      rc = _schemaContainer.renameColumn( columnID, newName ) ;
      PD_RC_CHECK( rc, PDERROR, "Rename internal schema column name %s to new name %s failed, "
                   "rc: %d", oldName, newName, rc ) ;

      // 4. Add new entry in the hash table for the new name. The column ID should not change.
      rc = _schemaHash.addColumnItem( newName, columnID ) ;
      PD_RC_CHECK( rc, PDERROR, "Add new schema hash entry for internal schema column failed, "
                   "rc: %d", rc ) ;

      _onSchemaColChanged() ;

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _dmsInternalSchema::dropColumnDefault( _dmsMBContext *context, const CHAR *name )
   {
      INT32 rc = SDB_OK ;

      UINT16 columnID = _schemaHash.getColumnIDByName( name ) ;

      if ( DMS_SCHEMA_INVALID_COLUMNID == columnID )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG( PDERROR, "The column name %s does not exist in internal schema, rc: %d",
                 name, rc ) ;
         goto error ;
      }

      rc = _schemaContainer.dropColumnDefault( columnID, TRUE ) ;
      PD_RC_CHECK( rc, PDERROR, "Drop write default for column %s failed, rc: %d", name, rc ) ;

      _onSchemaColChanged() ;

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _dmsInternalSchema::alterColumn( _dmsMBContext *context, const CHAR *columnName,
                                          const BSONObj &columnDef )
   {
      // There are several scenarios:
      // Case 1: The column does not exist in the internal schema before(No default value).
      //    In this case, we need to add the column into the internal schema, if any default value
      //    is set.
      // Case 2: The column exists in the internal schema, but neither read nor write default value is
      //    available. In this case, both the read and write default values can be set.
      // Case 3: The column exists in the internal schema, and read default is avaliable. Then the read
      //    default can not be changed, but the write default can be changed.

      INT32 rc = SDB_OK ;

      UINT16 columnID = _schemaHash.getColumnIDByName( columnName ) ;

      if ( DMS_SCHEMA_INVALID_COLUMNID == columnID )
      {
         // Case 1
         rc = addColumn( context, columnName, &columnDef, &columnID ) ;
         PD_RC_CHECK( rc, PDERROR, "Add column %s into internal schema failed, rc: %d",
                      columnName, rc ) ;
         goto done ;
      }
      else
      {
         // Case 2/3
         rc = _schemaContainer.alterColumn( columnID, columnDef ) ;
         PD_RC_CHECK( rc, PDERROR, "Alter column %s info in internal schema failed, rc: %d",
                      columnName, rc ) ;
         _onSchemaColChanged() ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _dmsInternalSchema::setIndexColumn( _dmsMBContext *context, const CHAR *columnName )
   {
      INT32 rc = SDB_OK ;
      UINT16 columnID = DMS_SCHEMA_INVALID_COLUMNID ;

      SDB_ASSERT( columnName, "Column name is invalid" ) ;

      if ( !context->isMBLock( EXCLUSIVE ) )
      {
         rc = SDB_SYS ;
         PD_LOG( PDERROR, "Collection mb latch is not locked in exclusive mode, rc: %d", rc ) ;
         goto error ;
      }

      columnID = _schemaHash.getColumnIDByName( columnName ) ;
      if ( DMS_SCHEMA_INVALID_COLUMNID == columnID )
      {
         // If the column does not exist in the internal schema, need to add it.
         rc = addColumn( context, columnName, NULL, &columnID ) ;
         PD_RC_CHECK( rc, PDERROR, "Add index column %s column name to internal schema failed, "
                      "rc: %d", columnName, rc ) ;
      }

      if ( !_schemaContainer.isIndexColumn( columnID ) )
      {
         _schemaContainer.setIndexColumn( columnID ) ;
         PD_LOG( PDDEBUG, "Set index column flag for column %s", columnName ) ;
         _onSchemaColChanged() ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _dmsInternalSchema::unsetIndexColumn( _dmsMBContext *context, const CHAR *columnName )
   {
      INT32 rc = SDB_OK ;
      UINT16 columnID = DMS_SCHEMA_INVALID_COLUMNID ;

      SDB_ASSERT( columnName, "Column name is invalid" ) ;

      if ( !context->isMBLock( EXCLUSIVE ) )
      {
         rc = SDB_SYS ;
         PD_LOG( PDERROR, "Collection mb latch is not locked in exclusive mode, rc: %d", rc ) ;
         goto error ;
      }

      columnID = _schemaHash.getColumnIDByName( columnName ) ;
      if ( ( DMS_SCHEMA_INVALID_COLUMNID == columnID ) ||
           !_schemaContainer.isIndexColumn( columnID ) )
      {
         // The column may not be in the internal schema. Nothing need to do.
         goto done ;
      }

      _schemaContainer.unsetIndexColumn( columnID ) ;
      _onSchemaColChanged() ;

      PD_LOG( PDDEBUG, "Remove index column flag for column %s", columnName ) ;

   done:
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSINTERNALSCHEMA_ENCODERECORD, "_dmsInternalSchema::encodeRecord" )
   INT32 _dmsInternalSchema::encodeRecord( dmsMBContext *context, _pmdEDUCB *cb,
                                           dmsRecordData &recordData, dmsRecordData &encodeData )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__DMSINTERNALSCHEMA_ENCODERECORD ) ;
      UINT32 totalSize = 0 ;
      CHAR *encodeRecord = NULL ;
      BOOLEAN hasNewColumn = FALSE ;
      COLUMN_ID_SET watchIDs( _encodeWatchIDs ) ;
      INT32 estimateSize = 0 ;
      utilBSONRawBuilder encodeBuilder ;
      BOOLEAN isEmpty = FALSE ;

      if ( recordData.len() + DMS_RECORD_METADATA_SZ > DMS_RECORD_USER_MAX_SZ )
      {
         rc = SDB_DMS_RECORD_TOO_BIG ;
         goto error ;
      }

      try
      {
         BSONObj record( recordData.data() ) ;
retry:
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
            // Switch to EXCLUSIVE lock, update the internal schema.
            rc = _updateSchemaByRecord( context, record ) ;
            PD_RC_CHECK( rc, PDERROR, "Update internal schema when inserting record failed, rc: %d",
                         rc ) ;

            hasNewColumn = FALSE ;
            watchIDs.clear() ;
            watchIDs.insert( _encodeWatchIDs.begin(), _encodeWatchIDs.end() ) ;
            encodeBuilder.reset() ;
            goto retry ;
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

   INT32 _dmsInternalSchema::_updateSchemaByRecord( _dmsMBContext *context, const BSONObj &record )
   {
      INT32 rc = SDB_OK ;
      UINT16 columnID = DMS_SCHEMA_INVALID_COLUMNID ;
      INT32 oldLockType = context->mbLockType() ;
      BOOLEAN lockByMe = FALSE ;

      if ( EXCLUSIVE != oldLockType )
      {
         rc = context->mbLock( EXCLUSIVE ) ;
         PD_RC_CHECK( rc, PDERROR, "Take collection mb latch in exclusive mode failed, rc: %d",
                      rc ) ;
         lockByMe = TRUE ;
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
               // New column for the schema.
               rc = addColumn( context, ele.fieldName(), NULL, &columnID ) ;
               PD_RC_CHECK( rc, PDERROR, "Add column %s into internal schema failed, rc: %d",
                            ele.fieldName(), rc ) ;
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
      if ( lockByMe )
      {
         // Resume the original kind of lock, or release the lock.
         if ( SHARED == oldLockType )
         {
            rc = context->mbLock( SHARED ) ;
            PD_RC_CHECK( rc, PDERROR, "Take collection mb latch in shared mode failed, rc: %d",
                         rc ) ;
         }
         else
         {
            context->mbUnlock() ;
         }
      }

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

      buff = (CHAR *)cb->getBuffer( buffSize ) ;
      if ( !buff )
      {
         rc = SDB_OOM ;
         PD_LOG( PDERROR, "Allocate memory of size %u failed, rc: %d", rc ) ;
         goto error ;
      }

      rc = builder.start( buff, buffSize, &originalRecord ) ;

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

   INT32 _dmsInternalSchema::_merge2Column( UINT16 columnID, const BSONObj &columnDef )
   {
      INT32 rc = SDB_OK ;

      try
      {
         // Currently only read and write default values can be merged. So we directly use the alter
         // column function.
         rc = _schemaContainer.alterColumn( columnID, columnDef ) ;
         PD_RC_CHECK( rc, PDERROR, "Update column info with given information failed, rc: %d",
                      rc ) ;
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

   INT32 _dmsInternalSchema::_flush()
   {
      _schemaContainer.flush() ;
      _schemaHash.flush() ;

      return SDB_OK ;
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
