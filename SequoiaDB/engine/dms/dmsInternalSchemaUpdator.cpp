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

   Source File Name = dmsInternalSchemaUpdator.cpp

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
#include "dmsInternalSchemaUpdator.hpp"
#include "msgDef.hpp"
#include "dmsTrace.hpp"
#include "dmsStorageDataCommon.hpp"

#define DMS_SCHEMA_INVALID_SLOT_ID              (0xFFFF)
#define DMS_SCHEMA_INVALID_SLOT_OFFSET          (0xFFFF)

namespace engine
{
   _dmsSchemaColInfoMem::_dmsSchemaColInfoMem()
   : _colRecord( NULL ),
     _attr( 0 )
   {
   }

   _dmsSchemaColInfoMem::~_dmsSchemaColInfoMem()
   {
      if ( _colRecord )
      {
         SDB_OSS_FREE( _colRecord ) ;
      }
   }

   INT32 _dmsSchemaColInfoMem::setData( UINT8 attr, const dmsSchemaColRecord *record )
   {
      INT32 rc = SDB_OK ;

      _attr = attr ;

      if ( record )
      {
         UINT32 colRecordLen = 0 ;
         dmsSchemaColRecord *newMem = NULL ;
         SDB_ASSERT( record != _colRecord, "Record if reference to this info obj" ) ;

         colRecordLen = record->getLength() ;
         if ( colRecordLen < DMS_SCHEMA_COLREC_HEAD_SZ )
         {
            rc = SDB_SYS ;
            PD_LOG( PDERROR, "Column record info is invalid, rc: %d", rc ) ;
            goto error ;
         }

         newMem = (dmsSchemaColRecord *)SDB_OSS_MALLOC( colRecordLen ) ;
         if ( !newMem )
         {
            rc = SDB_OOM ;
            PD_LOG( PDERROR, "Allocate memory of size [%u] to build schema column info record "
                    "failed, rc: %d", rc ) ;
            goto error ;
         }

         ossMemcpy( newMem, record, colRecordLen ) ;

         if ( _colRecord )
         {
            SDB_OSS_FREE( _colRecord ) ;
         }

         _colRecord = newMem ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   _dmsSchemaWriter::_dmsSchemaWriter()
   : _totalSize( 0 ),
     _hasBaseSchema( FALSE ),
     _hasChanged( FALSE ),
     _nextColumnID( 0 )
   {
   }

   _dmsSchemaWriter::~_dmsSchemaWriter()
   {
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSSCHEMAWRITER_INIT, "_dmsSchemaWriter::init" )
   INT32 _dmsSchemaWriter::init( const dmsSchemaExtent *baseSchemaExt,
                                 UINT32 extentSize, UINT16 mbID )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__DMSSCHEMAWRITER_INIT ) ;

      if ( baseSchemaExt )
      {
         rc = _baseSchemaContainer.init( baseSchemaExt, extentSize, mbID ) ;
         PD_RC_CHECK( rc, PDERROR, "Init internal schema writer with base schema extent failed, "
                      "rc: %d", rc ) ;
         _nextColumnID = _baseSchemaContainer.columnNum() ;
         _totalSize = extentSize - _baseSchemaContainer.getExtent()->_freeSpace -
                      DMS_SCHEMAEXTENT_HEADER_SZ ;
         _hasBaseSchema = TRUE ;

   #ifdef _DEBUG
         BSONObj baseSchemaInfo ;
         _baseSchemaContainer.dump( baseSchemaInfo, TRUE ) ;
         PD_LOG( PDDEBUG, "Init schema writer from existing schema: %s",
                 baseSchemaInfo.toPoolString().c_str() ) ;
   #endif /* _DEBUG */
      }
      else
      {
         _hasBaseSchema = FALSE ;
      }

   done:
      PD_TRACE_EXITRC( SDB__DMSSCHEMAWRITER_INIT, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSSCHEMAWRITER_ADDCOLUMN, "_dmsSchemaWriter::addColumn" )
   INT32 _dmsSchemaWriter::addColumn( const CHAR *name, const BSONObj *columnDef, UINT16 &columnID )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__DMSSCHEMAWRITER_ADDCOLUMN ) ;
      UINT8 attr = 0 ;
      dmsSchemaColRecBuilder builder ;

      SDB_ASSERT( name, "Column name is invalid" ) ;

      rc = builder.startBuild() ;
      PD_RC_CHECK( rc, PDERROR, "Star build internal schema column info failed, rc: %d", rc ) ;

      rc = builder.addColumnName( name ) ;
      PD_RC_CHECK( rc, PDERROR, "Add column name %s of internal schema into build buffer failed, "
                   "rc: %d", name, rc ) ;

      try
      {
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
      }
      catch ( std::exception &e )
      {
         rc = ossException2RC( &e ) ;
         PD_LOG( PDERROR, "Unexpected exception occurred: %s", e.what() ) ;
         goto error ;
      }

      builder.finishBuild() ;
      columnID = _nextColumnID ;

      rc = _addOrUpdateColumnInfo( columnID, attr, builder.getRecord() ) ;
      PD_RC_CHECK( rc, PDERROR, "Add new column info into internal schema update buffer failed, "
                   "rc: %d", rc ) ;

      // Only increase these data members at the end when everything is done.
      ++_nextColumnID ;

      _updateTotalSize( builder.getRecord()->getLength(), 0 ) ;

   done:
      PD_TRACE_EXITRC( SDB__DMSSCHEMAWRITER_ADDCOLUMN, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSSCHEMAWRITER_ALTERCOLUMN, "_dmsSchemaWriter::alterColumn" )
   INT32 _dmsSchemaWriter::alterColumn( UINT16 columnID, const BSONObj &columnInfo )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__DMSSCHEMAWRITER_ALTERCOLUMN ) ;

      SDB_ASSERT( DMS_SCHEMA_INVALID_COLUMNID != columnID, "Column id is invalid" ) ;

      BSONElement ele ;
      UINT8 attr = 0  ;
      dmsSchemaColRecBuilder builder ;
      const CHAR *name = NULL ;
      BOOLEAN changed = FALSE ;
      UINT32 oldColSize = 0 ;
      UINT32 newColSize = 0 ;
      const dmsSchemaColRecord *oldColRecord = NULL ;
      const dmsSchemaColRecord *newColRecord = NULL ;

      rc = _getColAttrAndRecord( columnID, attr, oldColRecord ) ;
      PD_RC_CHECK( rc, PDERROR, "Get column info with column id [%u] failed, rc: %d",
                   columnID, rc ) ;

      name = oldColRecord->getName() ;
      oldColSize = oldColRecord->getLength() ;
      rc = builder.startRebuild( oldColRecord ) ;
      PD_RC_CHECK( rc, PDERROR, "Build new column info for %s failed, rc: %d", name, rc ) ;

      rc = builder.addColumnName( name ) ;
      PD_RC_CHECK( rc, PDERROR, "Add column name %s to new column info failed, rc: %d",
                   name, rc ) ;

      try
      {
         // Currently only read default and write default values can be changed.
         ele = columnInfo.getField( FIELD_NAME_READDEFAULT ) ;
         if ( !ele.eoo() )
         {
            // Read default can not be changed.
            if ( OSS_BIT_TEST( attr, DMS_SCHEMA_COL_READ_DEFAULT ) )
            {
               PD_LOG( PDEVENT, "Read default value can not be change for column [%s], skip",
                       name ) ;
            }
            else
            {
               rc = builder.updateReadDefault( ele.type(), ele.value(), ele.valuesize() ) ;
               PD_RC_CHECK( rc, PDERROR, "Add read default for column %s to new column info "
                            "failed, rc: %d", name, rc ) ;
               OSS_BIT_SET( attr, DMS_SCHEMA_COL_READ_DEFAULT ) ;
               changed = TRUE ;
            }
         }

         ele = columnInfo.getField( FIELD_NAME_WRITEDEFAULT ) ;
         if ( !ele.eoo() )
         {
            BSONType type ;
            INT32 size = 0 ;
            const CHAR *value = NULL ;
            BOOLEAN same = FALSE ;
            if ( OSS_BIT_TEST( attr, DMS_SCHEMA_COL_WRITE_DEFAULT ) )
            {
               if ( !oldColRecord->getDefault( type, size, value, FALSE ) )
               {
                  rc = SDB_SYS ;
                  PD_LOG( PDERROR, "Get read default value of column [%s] failed, rc: %d",
                          name, rc ) ;
                  goto error ;
               }

               if ( ( ele.type() == type ) && ( ele.valuesize() == size ) &&
                    ( 0 == ossMemcmp( ele.value(), value, size ) ) )
               {
                  same = TRUE ;
                  PD_LOG( PDDEBUG, "New write default for column [%s] is the same with before, "
                          "nothing will be changed", name ) ;
               }
            }
            if ( !same )
            {
               rc = builder.updateWriteDefault( ele.type(), ele.value(), ele.valuesize() ) ;
               PD_RC_CHECK( rc, PDERROR, "Add write default for column %s to new column info "
                            "failed, rc: %d", name, rc ) ;
               if ( !OSS_BIT_TEST( attr, DMS_SCHEMA_COL_WRITE_DEFAULT ) )
               {
                  OSS_BIT_SET( attr, DMS_SCHEMA_COL_WRITE_DEFAULT ) ;
               }
               changed = TRUE ;
            }
         }

         if ( !changed )
         {
            goto done ;
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
         newColSize = newColRecord->getLength() ;

         rc = _addOrUpdateColumnInfo( columnID, attr, newColRecord ) ;
         PD_RC_CHECK( rc, PDERROR, "Add new column info into internal schema update buffer failed, "
                      "rc: %d", rc ) ;
      }
      catch ( std::exception &e )
      {
         rc = ossException2RC( &e ) ;
         PD_LOG( PDERROR, "Unexpected exception occurred: %s", e.what() ) ;
         goto error ;
      }

      _updateTotalSize( newColSize, oldColSize ) ;

   done:
      PD_TRACE_EXITRC( SDB__DMSSCHEMAWRITER_ALTERCOLUMN , rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSSCHEMAWRITER_DROPCOLUMN, "_dmsSchemaWriter::dropColumn" )
   INT32 _dmsSchemaWriter::dropColumn( UINT16 columnID )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__DMSSCHEMAWRITER_DROPCOLUMN ) ;
      UINT8 attr = 0 ;
      BOOLEAN attrChanged = FALSE ;
      BOOLEAN inMemory = FALSE ;
      dmsSchemaColRecBuilder builder ;
      const dmsSchemaColRecord *oldColRecord = NULL ;
      const dmsSchemaColRecord *newColRecord = NULL ;
      UINT32 oldColSize = 0 ;
      UINT32 newColSize = 0 ;

      rc = _getColAttrAndRecord( columnID, attr, oldColRecord, &inMemory ) ;
      PD_RC_CHECK( rc, PDERROR, "Get column info with column id [%u] failed, rc: %d",
                   columnID, rc ) ;
      oldColSize = oldColRecord->getLength() ;

      if ( OSS_BIT_TEST( attr, DMS_SCHEMA_COL_DELETED ) )
      {
         if ( inMemory )
         {
            // The column has been dropped already in the change round.
            goto done ;
         }
      }
      else
      {
         // Mark the column as deleted.
         OSS_BIT_SET( attr, DMS_SCHEMA_COL_DELETED ) ;
         attrChanged = TRUE ;
      }

      // Need to ignore the default values. They will not be stored in the column info any more.
      if ( OSS_BIT_TEST( attr, DMS_SCHEMA_COL_READ_DEFAULT ) ||
           OSS_BIT_TEST( attr, DMS_SCHEMA_COL_WRITE_DEFAULT ) )
      {
         const CHAR *origName = oldColRecord->getOrigName() ;

         rc = builder.startBuild() ;
         PD_RC_CHECK( rc, PDERROR, "Start build internal schema column info failed, rc: %d", rc ) ;

         rc = builder.addColumnName( oldColRecord->getName() ) ;
         PD_RC_CHECK( rc, PDERROR, "Add column name [%s] of internal schema into build buffer "
                      "failed, rc: %d", oldColRecord->getName(), rc ) ;

         if ( origName )
         {
            rc = builder.addColumnName( origName, TRUE ) ;
            PD_RC_CHECK( rc, PDERROR, "Add original name [%s] into internal schema column info of "
                         "column [%s] failed, rc: %d", origName, oldColRecord->getName(), rc ) ;
         }

         builder.finishBuild() ;

         OSS_BIT_CLEAR(  attr, DMS_SCHEMA_COL_READ_DEFAULT | DMS_SCHEMA_COL_WRITE_DEFAULT ) ;
         newColRecord = builder.getRecord() ;
         attrChanged = TRUE ;
      }
      else if ( !inMemory )
      {
         // Copy the original column record.
         newColRecord = oldColRecord ;
      }

      if ( newColRecord )
      {
         newColSize = newColRecord->getLength() ;
         rc = _addOrUpdateColumnInfo( columnID, attr, newColRecord ) ;
         PD_RC_CHECK( rc, PDERROR, "Add new column info into internal schema update buffer failed, "
                      "rc: %d", rc ) ;

         _updateTotalSize( newColSize, oldColSize ) ;
      }
      else if ( attrChanged )
      {
         rc = _addOrUpdateColumnInfo( columnID, attr, NULL ) ;
         PD_RC_CHECK( rc, PDERROR, "Update column attribute in internal schema update buffer "
                      "failed, rc: %d", rc ) ;
      }

   done:
      PD_TRACE_EXITRC( SDB__DMSSCHEMAWRITER_DROPCOLUMN, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSSCHEMAWRITER_HIDECOLUMN, "_dmsSchemaWriter::hideColumn" )
   INT32 _dmsSchemaWriter::hideColumn( UINT16 columnID )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__DMSSCHEMAWRITER_HIDECOLUMN ) ;
      UINT8 attr = 0 ;
      BOOLEAN inMemory = FALSE ;
      const dmsSchemaColRecord *record = NULL ;

      rc = _getColAttrAndRecord( columnID, attr, record, &inMemory ) ;
      PD_RC_CHECK( rc, PDERROR, "Get column info with column id [%u] failed, rc: %d",
                   columnID, rc ) ;
      if ( OSS_BIT_TEST( attr, DMS_SCHEMA_COL_HIDDEN ) )
      {
         goto done ;
      }

      OSS_BIT_SET( attr, DMS_SCHEMA_COL_HIDDEN ) ;

      rc = _addOrUpdateColumnInfo( columnID, attr, inMemory ? NULL : record ) ;
      PD_RC_CHECK( rc, PDERROR, "Mark column of id [%u] as hidden failed, rc: %d", columnID, rc ) ;

   done:
      PD_TRACE_EXITRC( SDB__DMSSCHEMAWRITER_HIDECOLUMN, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSSCHEMAWRITER_DROPCOLUMNDEFAULT, "_dmsSchemaWriter::dropColumnDefault" )
   INT32 _dmsSchemaWriter::dropColumnDefault( UINT16 columnID )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__DMSSCHEMAWRITER_DROPCOLUMNDEFAULT ) ;
      UINT8 attr = 0 ;
      dmsSchemaColRecBuilder builder ;
      const dmsSchemaColRecord *oldColRecord = NULL ;
      const dmsSchemaColRecord *newColRecord = NULL ;
      UINT32 oldColSize = 0 ;

      rc = _getColAttrAndRecord( columnID, attr, oldColRecord ) ;
      PD_RC_CHECK( rc, PDERROR, "Get column info with column id [%u] failed, rc: %d",
                   columnID, rc ) ;
      oldColSize = oldColRecord->getLength() ;

      if ( OSS_BIT_TEST( attr, DMS_SCHEMA_COL_DELETED ) )
      {
         // If the column has been deleted already, nothing need to be done.
         goto done ;
      }

      if ( !OSS_BIT_TEST( attr, DMS_SCHEMA_COL_WRITE_DEFAULT ) )
      {
         // If default value to be dropped does not exist, just ignore.
         goto done ;
      }

      rc = builder.startRebuild( oldColRecord ) ;
      PD_RC_CHECK( rc, PDERROR, "Start building schema column info failed, rc: %d", rc ) ;

      OSS_BIT_CLEAR( attr, DMS_SCHEMA_COL_WRITE_DEFAULT ) ;
      builder.dropWriteDefault() ;

      rc = builder.finishRebuild() ;
      PD_RC_CHECK( rc, PDERROR, "Build new column info when dropping default failed, rc: %d", rc ) ;

      newColRecord = builder.getRecord() ;

      SDB_ASSERT( newColRecord->getLength() <= oldColRecord->getLength(), "Length is wrong" ) ;

      rc = _addOrUpdateColumnInfo( columnID, attr, newColRecord ) ;
      PD_RC_CHECK( rc, PDERROR, "Add new column info into internal schema update buffer failed, "
                   "rc: %d", rc ) ;

      _updateTotalSize( newColRecord->getLength(), oldColSize ) ;

   done:
      PD_TRACE_EXITRC( SDB__DMSSCHEMAWRITER_DROPCOLUMNDEFAULT, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSSCHEMAWRITER_RENAMECOLUMN, "_dmsSchemaWriter::renameColumn" )
   INT32 _dmsSchemaWriter::renameColumn( UINT16 columnID, const CHAR *newName )
   {
      // Keep the id of the column unchanged, just change the column information. The size of the
      // column information may grow, and need another place to store. So the offset may change.
      // If the original space is enough, just do in-place update.

      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__DMSSCHEMAWRITER_RENAMECOLUMN ) ;
      UINT8 attr = 0 ;
      dmsSchemaColRecBuilder builder ;
      const dmsSchemaColRecord *oldColRecord = NULL ;
      const dmsSchemaColRecord *newColRecord = NULL ;
      UINT32 oldColSize = 0 ;

      rc = _getColAttrAndRecord( columnID, attr, oldColRecord ) ;
      PD_RC_CHECK( rc, PDERROR, "Get column info with column id [%u] failed, rc: %d",
                   columnID, rc ) ;
      oldColSize = oldColRecord->getLength() ;

      if ( !OSS_BIT_TEST( attr, DMS_SCHEMA_COL_HAS_ORIGNAME ) )
      {
         OSS_BIT_SET( attr, DMS_SCHEMA_COL_HAS_ORIGNAME ) ;
      }

      rc = builder.startRebuild( oldColRecord ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to start renaming column %s in internal schema, rc: %d",
                   oldColRecord->getName(), rc ) ;

      rc = builder.updateName( newName ) ;
      PD_RC_CHECK( rc, PDERROR, "Update column name to %s in column info builder failed, rc: %d",
                   newName, rc ) ;

      rc = builder.finishRebuild() ;
      PD_RC_CHECK( rc, PDERROR, "Finish rename column to %s failed, rc: %d", newName, rc ) ;

      newColRecord = builder.getRecord() ;

      rc = _addOrUpdateColumnInfo( columnID, attr, newColRecord ) ;
      PD_RC_CHECK( rc, PDERROR, "Add new column info into internal schema update buffer failed, "
                   "rc: %d", rc ) ;

      _updateTotalSize( newColRecord->getLength(), oldColSize ) ;

   done:
      PD_TRACE_EXITRC( SDB__DMSSCHEMAWRITER_RENAMECOLUMN, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSSCHEMAWRITER__GETCOLATTRANDRECORD, "_dmsSchemaWriter::_getColAttrAndRecord" )
   INT32 _dmsSchemaWriter::_getColAttrAndRecord( UINT16 columnID, UINT8 &attr,
                                                 const dmsSchemaColRecord *&record,
                                                 BOOLEAN *inMemory )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__DMSSCHEMAWRITER__GETCOLATTRANDRECORD ) ;

      COL_INFO_MAP_ITR itr = _colInfoMap.find( columnID ) ;
      if ( _colInfoMap.end() != itr )
      {
         attr = itr->second.getAttr() ;
         record = itr->second.getColRecord() ;
         if ( inMemory )
         {
            *inMemory = TRUE ;
         }
         goto done ;
      }
      else if ( _hasBaseSchema )
      {
         record = _baseSchemaContainer._getColRecord( columnID ) ;
         if ( !record )
         {
            // The column ID is generated based on the hash table, it should be valid.
            rc = SDB_SYS ;
            PD_LOG( PDERROR, "Column with id [%u] to be dropped does not exist in internal schema, "
                    "rc: %d", columnID, rc ) ;
            goto error ;
         }
         attr = _baseSchemaContainer._getColumnAttr( columnID ) ;
         if ( inMemory )
         {
            *inMemory = FALSE ;
         }
      }
      else
      {
         rc = SDB_SYS ;
         PD_LOG( PDERROR, "Internal schema column info for [%u] cannot be found, rc: %d", rc ) ;
         goto error ;
      }

   done:
      PD_TRACE_EXITRC( SDB__DMSSCHEMAWRITER__GETCOLATTRANDRECORD, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSSCHEMAWRITER__ADDORUPDATECOLUMNINFO, "_dmsSchemaWriter::_addOrUpdateColumnInfo" )
   INT32 _dmsSchemaWriter::_addOrUpdateColumnInfo( UINT16 columnID, UINT8 attr,
                                                   const dmsSchemaColRecord *record )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__DMSSCHEMAWRITER__ADDORUPDATECOLUMNINFO ) ;
      BOOLEAN hasAdd = FALSE ;

      try
      {
         dmsSchemaColInfoMem &colInfoMem = _colInfoMap[columnID] ;
         hasAdd = TRUE ;
         rc = colInfoMem.setData( attr, record ) ;
         PD_RC_CHECK( rc, PDERROR, "Save new internal schema column info in memory failed, rc: %d",
                      rc ) ;
         if ( !_hasChanged )
         {
            _hasChanged = true ;
         }
      }
      catch ( std::exception &e )
      {
         rc = ossException2RC( &e ) ;
         PD_LOG( PDERROR, "Unexpected exception occurred: %s", e.what() ) ;
         goto error ;
      }

   done:
      PD_TRACE_EXITRC( SDB__DMSSCHEMAWRITER__ADDORUPDATECOLUMNINFO, rc ) ;
      return rc ;
   error:
      if ( hasAdd )
      {
         _removeColumnInfo( columnID ) ;
      }
      goto done ;
   }

   void _dmsSchemaWriter::_removeColumnInfo( UINT16 columnID )
   {
      _colInfoMap.erase( columnID ) ;
   }

   void _dmsSchemaWriter::_updateTotalSize( UINT32 newColSize, UINT32 oldColSize )
   {
      if ( newColSize > 0 )
      {
         _totalSize += ossRoundUpToMultipleX( newColSize, 4 ) +
                       DMS_SCHEMAEXTENT_SLOT_SZ ;
      }

      if ( oldColSize > 0 )
      {
         _totalSize -= ( ossRoundUpToMultipleX( oldColSize, 4 ) +
                         DMS_SCHEMAEXTENT_SLOT_SZ ) ;
      }
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSSCHEMAWRITER_SETINDEXCOLUMN, "_dmsSchemaWriter::setIndexColumn" )
   INT32 _dmsSchemaWriter::setIndexColumn( UINT16 columnID )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__DMSSCHEMAWRITER_SETINDEXCOLUMN ) ;
      UINT8 attr = 0 ;
      BOOLEAN inMemory = FALSE ;
      dmsSchemaColRecBuilder builder ;
      const dmsSchemaColRecord *oldColRecord = NULL ;

      rc = _getColAttrAndRecord( columnID, attr, oldColRecord, &inMemory ) ;
      PD_RC_CHECK( rc, PDERROR, "Get column info with column id [%u] failed, rc: %d",
                   columnID, rc ) ;

      if ( OSS_BIT_TEST( attr, DMS_SCHEMA_COL_IN_INDEX ) )
      {
         goto done ;
      }

      OSS_BIT_SET( attr, DMS_SCHEMA_COL_IN_INDEX ) ;

      rc = _addOrUpdateColumnInfo( columnID, attr, inMemory ? NULL : oldColRecord ) ;
      PD_RC_CHECK( rc, PDERROR, "Add new column info into internal schema update buffer failed, "
                   "rc: %d", rc ) ;

   done:
      PD_TRACE_EXITRC( SDB__DMSSCHEMAWRITER_SETINDEXCOLUMN, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSSCHEMAWRITER_UNSETINDEXCOLUMN, "_dmsSchemaWriter::unsetIndexColumn" )
   INT32 _dmsSchemaWriter::unsetIndexColumn( UINT16 columnID )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__DMSSCHEMAWRITER_UNSETINDEXCOLUMN ) ;
      UINT8 attr = 0 ;
      BOOLEAN inMemory = FALSE ;
      dmsSchemaColRecBuilder builder ;
      const dmsSchemaColRecord *oldColRecord = NULL ;

      rc = _getColAttrAndRecord( columnID, attr, oldColRecord, &inMemory ) ;
      PD_RC_CHECK( rc, PDERROR, "Get column info with column id [%u] failed, rc: %d",
                   columnID, rc ) ;

      if ( !OSS_BIT_TEST( attr, DMS_SCHEMA_COL_IN_INDEX ) )
      {
         goto done ;
      }

      OSS_BIT_CLEAR( attr, DMS_SCHEMA_COL_IN_INDEX ) ;

      rc = _addOrUpdateColumnInfo( columnID, attr, inMemory ? NULL : oldColRecord ) ;
      PD_RC_CHECK( rc, PDERROR, "Add new column info into internal schema update buffer failed, "
                   "rc: %d", rc ) ;

   done:
      PD_TRACE_EXITRC( SDB__DMSSCHEMAWRITER_UNSETINDEXCOLUMN, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSSCHEMAWRITER_SAVE, "_dmsSchemaWriter::save" )
   INT32 _dmsSchemaWriter::save( dmsSchemaExtent *extent, UINT32 extentSize,
                                 UINT16 pageNum, UINT16 mbID, BOOLEAN isInnerChange )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__DMSSCHEMAWRITER_SAVE ) ;
      UINT8 attr = 0 ;
      UINT16 recordLen = 0 ;
      const dmsSchemaColRecord *record = NULL ;
      dmsSchemaColSlot *colSlot = NULL ;
      UINT32 recordOffset = extentSize ;
      UINT32 currVersion = DMS_SCHEMA_INVALID_VERSION ;
      UINT32 currInnerVersion = DMS_SCHEMA_INVALID_VERSION ;
      UINT32 readDefaultInitVer = DMS_SCHEMA_INVALID_VERSION ;
      UINT32 baseSchemaColumnNum = _hasBaseSchema ? _baseSchemaContainer.columnNum() : 0 ;

      extent->init( pageNum, mbID, extentSize ) ;
      ossMemset( (CHAR *)extent + DMS_SCHEMAEXTENT_HEADER_SZ, 0,
                  extentSize - DMS_SCHEMAEXTENT_HEADER_SZ ) ;
      colSlot = (dmsSchemaColSlot *)( (CHAR *)extent + DMS_SCHEMAEXTENT_HEADER_SZ ) ;

      if ( _hasBaseSchema )
      {
         currVersion = _baseSchemaContainer.getExtent()->_schemaVersion ;
         currInnerVersion = _baseSchemaContainer.getExtent()->_schemaInnerVersion ;
         if ( !isInnerChange )
         {
            extent->_schemaVersion = currVersion + 1 ;
         }
         extent->_schemaInnerVersion = currInnerVersion + 1 ;
      }
      else
      {
         // Internal schema is just enabled. Set its version to 1.
         extent->_schemaInnerVersion = 1 ;
         extent->_schemaVersion = 1 ;
      }

      for ( UINT16 columnID = 0; columnID < _nextColumnID; ++columnID )
      {
         readDefaultInitVer = extent->_schemaInnerVersion ;
         COL_INFO_MAP_CITR citr = _colInfoMap.find( columnID ) ;
         if ( _colInfoMap.end() != citr )
         {
            const dmsSchemaColInfoMem &colInfo = citr->second ;
            record = colInfo.getColRecord() ;
            attr = colInfo.getAttr() ;
         }
         else if ( columnID < baseSchemaColumnNum )
         {
            record = _baseSchemaContainer._getColRecord( columnID ) ;
            attr = _baseSchemaContainer._getColumnAttr( columnID ) ;
         }

         if ( columnID < baseSchemaColumnNum && OSS_BIT_TEST( attr, DMS_SCHEMA_COL_READ_DEFAULT ) )
         {
            const dmsSchemaColSlot *tmpSlot = _baseSchemaContainer._getColSlot( columnID ) ;
            if ( !tmpSlot )
            {
               rc = SDB_SYS ;
               PD_LOG( PDERROR, "Get slot in internal schema for column [ID: %u] failed, rc: %d",
                       columnID, rc ) ;
               goto error ;
            }

            readDefaultInitVer = tmpSlot->getDefaultInitVersion() ;
            SDB_ASSERT( DMS_SCHEMA_INVALID_VERSION != readDefaultInitVer, "Version invalid" ) ;
         }

         recordLen = record->getLength() ;
         recordOffset -= ossRoundUpToMultipleX( recordLen, 4 ) ;
         ossMemcpy( (CHAR *)extent + recordOffset, record, recordLen ) ;
         colSlot->setAttr( attr ) ;
         colSlot->setOffset( recordOffset ) ;
         colSlot->setDefaultInitVersion( readDefaultInitVer ) ;

         ++colSlot ;
         ++extent->_itemNum ;
      }

      extent->_freeSpace = extentSize - DMS_SCHEMAEXTENT_HEADER_SZ - _totalSize ;
      extent->_valueOffset = recordOffset ;

   done:
      PD_TRACE_EXIT( SDB__DMSSCHEMAWRITER_SAVE ) ;
      return rc ;
   error:
      goto done ;
   }

   _dmsSchemaHashWriter::_dmsSchemaHashWriter()
   : _maxListSlotNum( 0 ),
     _extent( NULL ),
     _bucketNum( 0 ),
     _pBucketSlot( NULL ) ,
     _pListSlot( NULL )
   {
   }

   _dmsSchemaHashWriter::~_dmsSchemaHashWriter()
   {
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSSCHEMAHASHWRITER_INIT, "_dmsSchemaHashWriter::init" )
   INT32 _dmsSchemaHashWriter::init( const dmsSchemaContainer *schemaContainer )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__DMSSCHEMAHASHWRITER_INIT ) ;

      if ( !schemaContainer )
      {
         // Build a new hash table from scratch.
         goto done ;
      }

      try
      {
         // Build the memory mapping of column name to id based on the base internal schema.
         const CHAR *columnName = NULL ;
         for ( UINT32 columnID = 0; columnID < schemaContainer->columnNum(); ++columnID )
         {
            if ( schemaContainer->isColumnDeleted( columnID ) )
            {
               continue ;
            }
            else
            {
               columnName = schemaContainer->getColumnName( columnID ) ;
               _nameIDMap[ columnName ] = columnID ;
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
      PD_TRACE_EXITRC( SDB__DMSSCHEMAHASHWRITER_INIT, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   INT32 _dmsSchemaHashWriter::addColumnItem( const CHAR *name, UINT16 columnID )
   {
      INT32 rc = SDB_OK ;

      try
      {
         _nameIDMap[ name ] = columnID ;
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

   UINT16 _dmsSchemaHashWriter::getColumnIDByName( const CHAR *name ) const
   {
      UINT16 columnID = DMS_SCHEMA_INVALID_COLUMNID ;
      COL_NAME_ID_MAP_CITR citr = _nameIDMap.find( name ) ;
      if ( _nameIDMap.end() != citr )
      {
         columnID = citr->second ;
      }
      return columnID ;
   }

   void _dmsSchemaHashWriter::dropColumnItemByName( const CHAR *name )
   {
      SDB_ASSERT( name, "Name of column to be dropped is null" ) ;
      _nameIDMap.erase( name ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSSCHEMAHASHWRITER__ADDCOLUMNITEM, "_dmsSchemaHashWriter::_addColumnItem" )
   INT32 _dmsSchemaHashWriter::_addColumnItem( const CHAR *name, UINT16 columnID )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__DMSSCHEMAHASHWRITER__ADDCOLUMNITEM ) ;
      UINT16 nextSlotID = DMS_SCHEMA_HASH_INVALID_SLOTID ;
      UINT32 bucketID = ossHash( name ) % DMS_SCHEMA_HASH_BUCKET_SIZE ;  // The bucket is also an item.
      dmsSchemaHashSlot* hashSlot = (dmsSchemaHashSlot *)_getHashBucketSlot( bucketID ) ;

      // The hash slot is empty, no conflict. Use it directly.
      if ( DMS_SCHEMA_INVALID_COLUMNID == hashSlot->getColumnID() )
      {
         hashSlot->setColumnID( columnID ) ;
         hashSlot->setNextSlotID( DMS_SCHEMA_HASH_INVALID_SLOTID ) ;
         goto done ;
      }

      // Conflict, need to find one slot in the list slot area, and put it at the end of the
      // conflict list.
      while ( TRUE )
      {
         nextSlotID = hashSlot->getNextSlotID() ;
         if ( DMS_SCHEMA_HASH_INVALID_SLOTID == nextSlotID )
         {
            // Reach the tail of the conflict list. Allocate a new slot, and link it to the tail.
            dmsSchemaHashSlot *newSlot = NULL ;
            INT32 freeSlotID = _extent->_slotNum ;
            newSlot = (dmsSchemaHashSlot *)_getHashListSlot( freeSlotID ) ;
            newSlot->setColumnID( columnID ) ;
            newSlot->setNextSlotID( DMS_SCHEMA_HASH_INVALID_SLOTID ) ;
            hashSlot->setNextSlotID( freeSlotID ) ;

            ++((dmsSchemaHashExtent *)_extent)->_slotNum ;

            goto done ;
         }
         else
         {
            // Haven't found the tail, continue.
            hashSlot = (dmsSchemaHashSlot *)_getHashListSlot( nextSlotID )  ;
            if ( !hashSlot )
            {
               rc = SDB_SYS ;
               PD_LOG( PDERROR, "Get internal schema hash slot for slot id[%u] failed, rc: %d",
                       nextSlotID, rc ) ;
               goto error ;
            }
         }
      }

   done:
      PD_TRACE_EXITRC( SDB__DMSSCHEMAHASHWRITER__ADDCOLUMNITEM, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSSCHEMAHASHWRITER_SAVE, "_dmsSchemaHashWriter::save" )
   INT32 _dmsSchemaHashWriter::save( dmsSchemaHashExtent *extent, UINT32 extentSize,
                                     UINT16 numPages, UINT16 mbID )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__DMSSCHEMAHASHWRITER_SAVE ) ;

      rc = _prepare4Save( extent, extentSize, numPages, mbID ) ;
      PD_RC_CHECK( rc, PDERROR, "Prepare schema hash extent for saving new internal schema hash "
                   "table failed, rc: %d", rc ) ;

      for ( COL_NAME_ID_MAP_CITR itr = _nameIDMap.begin(); itr != _nameIDMap.end(); ++itr )
      {
         rc = _addColumnItem( itr->first.c_str(), itr->second ) ;
         PD_RC_CHECK( rc, PDERROR, "Add column [%s] into internal schema hash table failed, rc: %d",
                      itr->first.c_str(), rc ) ;
      }

   done:
      PD_TRACE_EXITRC( SDB__DMSSCHEMAHASHWRITER_SAVE, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSSCHEMAHASHWRITER__PREPARE4SAVE, "_dmsSchemaHashWriter::_prepare4Save" )
   INT32 _dmsSchemaHashWriter::_prepare4Save( dmsSchemaHashExtent *extent, UINT32 extentSize,
                                              UINT16 numPages, UINT16 mbID )
   {
      PD_TRACE_ENTRY( SDB__DMSSCHEMAHASHWRITER__PREPARE4SAVE ) ;
      extent->init( numPages, mbID ) ;
      _bucketNum = extent->_bucketNum ;
      _extent = extent ;
      _maxListSlotNum =
         (extentSize - DMS_SCHEMAHASHEXTENT_HEADER_SZ - _bucketNum * DMS_HASHEXTENT_SLOT_SZ) /
         DMS_HASHEXTENT_SLOT_SZ ;

      _pBucketSlot = (dmsSchemaHashSlot*)
                     ((CHAR *)_extent + DMS_SCHEMAHASHEXTENT_HEADER_SZ) ;
      _pListSlot = &_pBucketSlot[ _bucketNum ] ;

      for ( UINT32 slotID = 0; slotID < _bucketNum; ++slotID )
      {
         ((dmsSchemaHashSlot *)&_pBucketSlot[slotID])->reset() ;
      }

      for ( UINT32 slotID = 0; slotID < _maxListSlotNum; ++slotID )
      {
         ((dmsSchemaHashSlot *)&_pListSlot[slotID])->reset() ;
      }

      PD_TRACE_EXIT( SDB__DMSSCHEMAHASHWRITER__PREPARE4SAVE ) ;
      return SDB_OK ;
   }

   _dmsInternalSchemaWriter::_dmsInternalSchemaWriter( BOOLEAN createNew )
   : _su( NULL ),
     _createNew( createNew ),
     _isInnerChange( FALSE )
   {
   }

   _dmsInternalSchemaWriter::~_dmsInternalSchemaWriter()
   {
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSINTERNALSCHEMAWRITER_INIT, "_dmsInternalSchemaWriter::init" )
   INT32 _dmsInternalSchemaWriter::init( const dmsInternalSchema *pSchema,
                                         dmsStorageDataCommon *su,
                                         dmsMBContext *context,
                                         pmdEDUCB *cb )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__DMSINTERNALSCHEMAWRITER_INIT ) ;
      const dmsSchemaExtent * baseSchemaExtent = NULL ;
      UINT32 extentSize = 0 ;

      if ( pSchema )
      {
         if ( pSchema->enabled() )
         {
            baseSchemaExtent = pSchema->getSchemaContainer()->getExtent() ;
            extentSize = pSchema->getSchemaContainer()->getExtentSize() ;
         }
         else
         {
            rc = SDB_INTERNAL_SCHEMA_NOT_ENABLED ;
            PD_LOG( PDERROR, "Internal schema for collection [%s] is not enabled, rc: %d",
                    context->mb()->_collectionName, rc ) ;
            goto error ;
         }
      }
      else
      {
         if ( !_createNew )
         {
            rc = SDB_SYS ;
            PD_LOG( PDERROR, "Internal schema for init the schema writer is invalid, rc: %d", rc ) ;
            goto error ;
         }
      }

      rc = _schemaWriter.init( baseSchemaExtent, extentSize, context->mbID() ) ;
      PD_RC_CHECK( rc, PDERROR, "Init internal schema writer for collection [%s] failed, rc: %d",
                   context->mb()->_collectionName, rc ) ;

      rc = _schemaHashWriter.init( _schemaWriter.getBaseSchemaContainer() ) ;
      PD_RC_CHECK( rc, PDERROR, "Init internal schema hash writer failed, rc: %d", rc ) ;

      _su = su ;

   done:
      PD_TRACE_EXITRC( SDB__DMSINTERNALSCHEMAWRITER_INIT, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSINTERNALSCHEMAWRITER_SAVE, "_dmsInternalSchemaWriter::save" )
   INT32 _dmsInternalSchemaWriter::save( dmsMBContext *context, pmdEDUCB *cb, BOOLEAN &hasChanged )
   {
      // If internal schema changes, update it by the following steps, to make sure everything is
      // right:
      // 1. Allocate one schema extent and one hash extent, which are big enough to hold the new
      //    schema.
      // 2. Save the data in memory into these extents, and flush them to disk.
      // 3. Change the schema and hash extent id in collection mb, and flush metadata.
      // 4. Release the original extents, if any.
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__DMSINTERNALSCHEMAWRITER_SAVE ) ;
      dmsExtentID oldSchemaExtID = DMS_INVALID_EXTENT ;
      dmsExtentID oldHashExtID = DMS_INVALID_EXTENT ;
      dmsExtentID newSchemaExtID = DMS_INVALID_EXTENT ;
      dmsExtentID newHashExtID = DMS_INVALID_EXTENT ;
      UINT16 schemaExtPageNum = 0 ;
      UINT16 hashExtPageNum = 0 ;
      UINT32 schemaExtSize = 0 ;
      UINT32 hashExtSize = 0 ;
      BOOLEAN extentChanged = TRUE ;

      // If the schema has not changed, noting need to do. For creation, need to allocate two
      // extents.
      if ( !_schemaWriter.hasChanged() && !_createNew )
      {
         PD_LOG( PDDEBUG, "Nothing changed for the internal schema of collection [%s]",
                 context->mb()->_collectionName ) ;
         hasChanged = FALSE ;
         goto done ;
      }

      hasChanged = TRUE ;

      oldSchemaExtID = context->mb()->_schemaExtentID ;
      oldHashExtID = context->mb()->_schemaHashExtentID ;

      schemaExtSize = ossRoundUpToMultipleX( _schemaWriter.totalSize() + DMS_SCHEMAEXTENT_HEADER_SZ,
                                             _su->pageSize() ) ;

      if ( schemaExtSize > DMS_SCHEMA_EXTENT_MAX_SZ )
      {
         rc = SDB_OSS_UP_TO_LIMIT ;
         PD_LOG( PDERROR, "Size of internal schema [%u] exceeds the limit[%u], rc: %d",
                 schemaExtSize, DMS_SCHEMA_EXTENT_MAX_SZ, rc ) ;
         goto error ;
      }

      schemaExtPageNum = schemaExtSize >> _su->pageSizeSquareRoot() ;
      rc = _su->_findFreeSpace( schemaExtPageNum, newSchemaExtID, context ) ;
      PD_RC_CHECK( rc, PDERROR, "Allocate extent of [%u] pages for new internal schema failed, "
                   "rc: %d", schemaExtPageNum, rc ) ;

      // We don't know how many items will be in the conflict list slot area. Allocate for the
      // worest case.
      hashExtSize = DMS_SCHEMAHASHEXTENT_HEADER_SZ +
                    ( DMS_SCHEMA_HASH_BUCKET_SIZE + _schemaHashWriter.totalSize() ) *
                    DMS_HASHEXTENT_SLOT_SZ ;

      hashExtSize = ossRoundUpToMultipleX( hashExtSize, _su->pageSize() ) ;

      hashExtPageNum = hashExtSize >> _su->pageSizeSquareRoot() ;
      rc = _su->_findFreeSpace( hashExtPageNum, newHashExtID, context ) ;
      PD_RC_CHECK( rc, PDERROR, "Allocate extent of [%u] pages for new internal schema hash table "
                   "failed, rc: %d", hashExtPageNum, rc ) ;

      PD_LOG( PDDEBUG, "Allocate new extents for internal schema of collection [%s]. Schema "
              "extent[ID:%d, size:%u], hash extent[ID: %d, size: %u]",
              context->mb()->_collectionName, newSchemaExtID,
              schemaExtSize, newHashExtID, hashExtSize ) ;

      {
         dmsSchemaContainer schemaContainer ;
         dmsExtRW schemaExtRW = _su->extent2RW( newSchemaExtID ) ;
         dmsExtRW hashExtRW = _su->extent2RW( newHashExtID ) ;
         schemaExtRW.setNothrow( TRUE ) ;
         hashExtRW.setNothrow( TRUE ) ;

         dmsSchemaExtent *schemaExt = schemaExtRW.writePtr<dmsSchemaExtent>( 0, schemaExtSize ) ;
         dmsSchemaHashExtent *hashExt = hashExtRW.writePtr<dmsSchemaHashExtent>( 0, hashExtSize ) ;
         if ( !schemaExt )
         {
            rc = SDB_SYS ;
            PD_LOG( PDERROR, "Get internal schema extent [%d] write pointer of collection [%s] "
                    "failed, rc: %d", newSchemaExtID, context->mb()->_collectionName, rc ) ;
            goto error ;
         }

         if ( !hashExt )
         {
            rc = SDB_SYS ;
            PD_LOG( PDERROR, "Get internal schema hash extent [%d] write pointer of collection "
                    "[%s] failed, rc: %d", newHashExtID, context->mb()->_collectionName, rc ) ;
            goto error ;
         }

         rc = _schemaWriter.save( schemaExt, schemaExtSize, schemaExtPageNum,
                                  context->mbID(), _isInnerChange ) ;
         PD_RC_CHECK( rc, PDERROR, "Save new internal schema for collection [%s] failed, rc: %d",
                      context->mb()->_collectionName, rc ) ;

         rc = _schemaHashWriter.save( hashExt, hashExtSize, hashExtPageNum, context->mbID() ) ;
         PD_RC_CHECK( rc, PDERROR, "Save new internal schema hash table for collection [%s] "
                      "failed, rc: %d", context->mb()->_collectionName, rc ) ;

         rc = _su->flushPages( newSchemaExtID, schemaExtPageNum ) ;
         PD_RC_CHECK( rc, PDERROR, "Flush internal schema extent for collection [%s] failed, "
                      "rc: %d", context->mb()->_collectionName, rc ) ;
         rc = _su->flushPages( newHashExtID, hashExtPageNum ) ;
         PD_RC_CHECK( rc, PDERROR, "Flush internal schema hash extent for collection [%s] failed, "
                      "rc: %d", context->mb()->_collectionName, rc ) ;

         context->mb()->_schemaExtentID = newSchemaExtID ;
         context->mb()->_schemaHashExtentID = newHashExtID ;
         if ( !OSS_BIT_TEST( context->mb()->_attributes, DMS_MB_ATTR_ENABLE_INFOSCHEMA ) )
         {
            OSS_BIT_SET( context->mb()->_attributes, DMS_MB_ATTR_ENABLE_INFOSCHEMA ) ;
         }
         extentChanged = TRUE ;

         _su->_onMBUpdated( context->mbID() ) ;
         _su->flushMME( _su->isSyncDeep() ) ;

         if ( DMS_INVALID_EXTENT != oldSchemaExtID )
         {
            dmsExtRW extRW = _su->extent2RW( oldSchemaExtID ) ;
            extRW.setNothrow( TRUE ) ;
            const dmsSchemaExtent *schemaExtent = extRW.readPtr<dmsSchemaExtent>() ;
            if ( schemaExtent )
            {
               _su->_releaseSpace( oldSchemaExtID, schemaExtent->_blockSize ) ;
               PD_LOG( PDDEBUG, "Release old internal schema extent[ID: %d, size: %u]",
                       oldSchemaExtID, schemaExtent->_blockSize << _su->pageSizeSquareRoot() ) ;
            }
            else
            {
               PD_LOG( PDERROR, "Internal schema extent[ID: %d] is invalid", oldSchemaExtID ) ;
            }
         }
         if ( DMS_INVALID_EXTENT != oldHashExtID )
         {
            dmsExtRW extRW = _su->extent2RW( oldHashExtID ) ;
            extRW.setNothrow( TRUE ) ;
            const dmsSchemaHashExtent *hashExtent = extRW.readPtr<dmsSchemaHashExtent>() ;
            if ( hashExtent )
            {
               _su->_releaseSpace( oldHashExtID, hashExtent->_blockSize ) ;
               PD_LOG( PDDEBUG, "Release old internal schema hash extent[ID: %d, size: %u]",
                       oldHashExtID, hashExtent->_blockSize << _su->pageSizeSquareRoot() ) ;
            }
            else
            {
               PD_LOG( PDERROR, "Internal schema hash extent[ID: %d] is invalid", oldHashExtID ) ;
            }
         }
      }

   done:
      PD_TRACE_EXITRC( SDB__DMSINTERNALSCHEMAWRITER_SAVE, rc ) ;
      return rc ;
   error:
      if ( extentChanged )
      {
         INT32 rcTmp = SDB_OK ;
         context->mb()->_schemaExtentID = oldSchemaExtID ;
         context->mb()->_schemaHashExtentID = oldHashExtID ;
         _su->_onMBUpdated( context->mbID() ) ;
         _su->flushMME( _su->isSyncDeep() ) ;

         rcTmp = _su->_freeExtent( newSchemaExtID, context->mbID() ) ;
         if ( rcTmp )
         {
            PD_LOG( PDWARNING, "Free new schema extent[ID: %d] of collection [%s] failed, rc: %d",
                    newSchemaExtID, context->mb()->_collectionName, rc ) ;
         }
         rcTmp = _su->_freeExtent( newHashExtID, context->mbID() ) ;
         if ( rcTmp )
         {
            PD_LOG( PDWARNING, "Free new schema hash extent[ID: %d] of collection [%s] failed, "
                    "rc: %d", newHashExtID, context->mb()->_collectionName, rc ) ;
         }
      }
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSINTERNALSCHEMAWRITER_ADDCOLUMNS, "_dmsInternalSchemaWriter::addColumns" )
   INT32 _dmsInternalSchemaWriter::addColumns( const utilSchema &schema, _pmdEDUCB *cb,
                                               const ossPoolSet<ossPoolString> *pSetIdxFields )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__DMSINTERNALSCHEMAWRITER_ADDCOLUMNS ) ;

      for ( UTIL_SCHEMA_COLUMN_LIST_CIT iter = schema.getColumns().begin() ;
            iter != schema.getColumns().end() ; ++ iter )
      {
         const utilSchemaColumn &column = *iter ;
         const CHAR *columnName = column.getName() ;
         if ( column.hasWriteDefault() || column.hasReadDefault() )
         {
            rc = addColumn( columnName, &column.getDefine(), NULL, TRUE ) ;
            PD_RC_CHECK( rc, PDERROR, "Failed to add column [%s] to internal schema, rc: %d",
                         columnName, rc ) ;
         }
      }
      if ( pSetIdxFields && !pSetIdxFields->empty() )
      {
         for ( ossPoolSet<ossPoolString>::const_iterator citr = pSetIdxFields->begin();
               citr != pSetIdxFields->end(); ++citr )
         {
            rc = setIndexColumn( citr->c_str() ) ;
            PD_RC_CHECK( rc, PDERROR, "Set schema column [%s] as index column failed, rc: %d",
                         citr->c_str(), rc ) ;
         }
      }

   done:
      PD_TRACE_EXITRC( SDB__DMSINTERNALSCHEMAWRITER_ADDCOLUMNS, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSINTERNALSCHEMAWRITER_ADDCOLUMN, "_dmsInternalSchemaWriter::addColumn" )
   INT32 _dmsInternalSchemaWriter::addColumn( const CHAR *columnName, const BSONObj *columnDef,
                                              UINT16 *columnID, BOOLEAN mergeOnExist )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__DMSINTERNALSCHEMAWRITER_ADDCOLUMN ) ;
      UINT16 id = 0 ;

      // The column may exist in the internal schema already. In that case, just return succeed.
      id = _schemaHashWriter.getColumnIDByName( columnName ) ;
      if ( DMS_SCHEMA_INVALID_COLUMNID != id )
      {
         if ( mergeOnExist )
         {
            if ( columnDef )
            {
               rc = _merge2Column( id, *columnDef ) ;
               PD_RC_CHECK( rc, PDERROR, "Merge column info into existing definition failed: %d",
                            rc ) ;
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

      rc = _schemaWriter.addColumn( columnName, columnDef, id ) ;
      PD_RC_CHECK( rc, PDERROR, "Add column info into internal schema failed, rc: %d", rc ) ;

      rc = _schemaHashWriter.addColumnItem( columnName, id ) ;
      PD_RC_CHECK( rc, PDERROR, "Add column into internal schema hash table failed, rc: %d", rc ) ;

      if ( columnID )
      {
         *columnID = id ;
      }

   done:
      PD_TRACE_EXITRC( SDB__DMSINTERNALSCHEMAWRITER_ADDCOLUMN, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSINTERNALSCHEMAWRITER_DROPCOLUMN, "_dmsInternalSchemaWriter::dropColumn" )
   INT32 _dmsInternalSchemaWriter::dropColumn( const CHAR *columnName )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__DMSINTERNALSCHEMAWRITER_DROPCOLUMN ) ;
      UINT16 columnID = DMS_SCHEMA_INVALID_COLUMNID ;

      // Get ID of the column from the hash table.
      columnID = _schemaHashWriter.getColumnIDByName( columnName ) ;
      if ( DMS_SCHEMA_INVALID_COLUMNID == columnID )
      {
         rc = _schemaWriter.addColumn( columnName, NULL, columnID )  ;
         PD_RC_CHECK( rc, PDERROR, "Auto add column [%s] into internal schema in dropping column "
                      "operation failed, rc: %d", columnName, rc ) ;
      }
      else
      {
         _schemaHashWriter.dropColumnItemByName( columnName ) ;
      }

      // mark as deleted in the schema container
      rc = _schemaWriter.dropColumn( columnID ) ;
      PD_RC_CHECK( rc, PDERROR, "Drop column information in internal schema failed, rc: %d", rc ) ;

   done:
      PD_TRACE_EXITRC( SDB__DMSINTERNALSCHEMAWRITER_DROPCOLUMN, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSINTERNALSCHEMAWRITER_RENAMECOLUMN, "_dmsInternalSchemaWriter::renameColumn" )
   INT32 _dmsInternalSchemaWriter::renameColumn( const CHAR *oldName, const CHAR *newName )
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
      PD_TRACE_ENTRY( SDB__DMSINTERNALSCHEMAWRITER_RENAMECOLUMN ) ;
      UINT16 columnID = DMS_SCHEMA_INVALID_COLUMNID ;
      UINT16 conflictColID = DMS_SCHEMA_INVALID_COLUMNID ;

      SDB_ASSERT( oldName && newName, "Name invalid" ) ;

      // 1. Check if the old name exists. If not, need to add the column information first.
      //    If yes, drop the column entry in the hash table.
      columnID = _schemaHashWriter.getColumnIDByName( oldName ) ;
      if ( DMS_SCHEMA_INVALID_COLUMNID == columnID )
      {
         // For internal schema, it's normal that the name we want to rename does not exist.
         PD_LOG( PDDEBUG, "Old name [%s] does not exist in the local internal schema when "
                 "renaming, rc: %d", oldName, rc ) ;
         rc = _schemaWriter.addColumn( oldName, NULL, columnID )  ;
         PD_RC_CHECK( rc, PDERROR, "Auto add column [%s] in renaming column operation failed, "
                      "rc: %d", oldName, rc ) ;
      }
      else
      {
         _schemaHashWriter.dropColumnItemByName( oldName ) ;
      }

      // 2. Check if column with the targe name exists. If yes, hide it.
      conflictColID = _schemaHashWriter.getColumnIDByName( newName ) ;
      if ( DMS_SCHEMA_INVALID_COLUMNID != conflictColID )
      {
         rc = _hideColumn( newName, conflictColID ) ;
         PD_RC_CHECK( rc, PDERROR, "Hide conflict target column name [%s] for renaming column [%s] "
                      "failed, rc: %d", newName, oldName, rc ) ;
      }

      // 3. Rename the column and update the column information in schema extent.
      rc = _schemaWriter.renameColumn( columnID, newName ) ;
      PD_RC_CHECK( rc, PDERROR, "Rename internal schema column name %s to new name %s failed, "
                   "rc: %d", oldName, newName, rc ) ;

      // 4. Add new entry in the hash table for the new name. The column ID should not change.
      rc = _schemaHashWriter.addColumnItem( newName, columnID ) ;
      PD_RC_CHECK( rc, PDERROR, "Add new schema hash entry for internal schema column failed, "
                   "rc: %d", rc ) ;

   done:
      PD_TRACE_EXITRC( SDB__DMSINTERNALSCHEMAWRITER_RENAMECOLUMN, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSINTERNALSCHEMAWRITER_DROPCOLUMNDEFAULT, "_dmsInternalSchemaWriter::dropColumnDefault" )
   INT32 _dmsInternalSchemaWriter::dropColumnDefault( const CHAR *name )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__DMSINTERNALSCHEMAWRITER_DROPCOLUMNDEFAULT ) ;

      UINT16 columnID = _schemaHashWriter.getColumnIDByName( name ) ;

      if ( DMS_SCHEMA_INVALID_COLUMNID == columnID )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG( PDERROR, "The column name %s does not exist in internal schema, rc: %d",
                 name, rc ) ;
         goto error ;
      }

      rc = _schemaWriter.dropColumnDefault( columnID ) ;
      PD_RC_CHECK( rc, PDERROR, "Drop write default for column %s failed, rc: %d", name, rc ) ;

   done:
      PD_TRACE_EXITRC( SDB__DMSINTERNALSCHEMAWRITER_DROPCOLUMNDEFAULT, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSINTERNALSCHEMAWRITER_ALTERCOLUMN, "_dmsInternalSchemaWriter::alterColumn" )
   INT32 _dmsInternalSchemaWriter::alterColumn( const CHAR *columnName, const BSONObj &columnDef )
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
      PD_TRACE_ENTRY( SDB__DMSINTERNALSCHEMAWRITER_ALTERCOLUMN ) ;

      UINT16 columnID = _schemaHashWriter.getColumnIDByName( columnName ) ;
      if ( DMS_SCHEMA_INVALID_COLUMNID == columnID )
      {
         // Case 1
         PD_LOG( PDDEBUG, "Column [%s] does not exist when altering", columnName ) ;
         rc = _schemaWriter.addColumn( columnName, &columnDef, columnID )  ;
         PD_RC_CHECK( rc, PDERROR, "Auto add column [%s] into internal schema in altering column "
                      "operation failed, rc: %d", columnName, rc ) ;
         rc = _schemaHashWriter.addColumnItem( columnName, columnID ) ;
         PD_RC_CHECK( rc, PDERROR, "Auto add column [%s] into internal schema hash table in "
                      "altering column operation failed, rc: %d", columnName, rc ) ;
      }

      // Case 2/3
      rc = _schemaWriter.alterColumn( columnID, columnDef ) ;
      PD_RC_CHECK( rc, PDERROR, "Alter column %s info in internal schema failed, rc: %d",
                   columnName, rc ) ;

   done:
      PD_TRACE_EXITRC( SDB__DMSINTERNALSCHEMAWRITER_ALTERCOLUMN, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSINTERNALSCHEMAWRITER_SETINDEXCOLUMN, "_dmsInternalSchemaWriter::setIndexColumn" )
   INT32 _dmsInternalSchemaWriter::setIndexColumn( const CHAR *columnName )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__DMSINTERNALSCHEMAWRITER_SETINDEXCOLUMN ) ;
      UINT16 columnID = DMS_SCHEMA_INVALID_COLUMNID ;

      SDB_ASSERT( columnName, "Column name is invalid" ) ;

      columnID = _schemaHashWriter.getColumnIDByName( columnName ) ;
      if ( DMS_SCHEMA_INVALID_COLUMNID == columnID )
      {
         // If the column does not exist in the internal schema, need to add it.
         rc = addColumn( columnName, NULL, &columnID ) ;
         PD_RC_CHECK( rc, PDERROR, "Add index column [%s] name to internal schema failed, "
                      "rc: %d", columnName, rc ) ;
      }

      _schemaWriter.setIndexColumn( columnID ) ;
      PD_LOG( PDDEBUG, "Set index column flag for column [%s]", columnName ) ;

   done:
      PD_TRACE_EXITRC( SDB__DMSINTERNALSCHEMAWRITER_SETINDEXCOLUMN, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSINTERNALSCHEMAWRITER_UNSETINDEXCOLUMN, "_dmsInternalSchemaWriter::unsetIndexColumn" )
   INT32 _dmsInternalSchemaWriter::unsetIndexColumn( const CHAR *columnName )
   {
      PD_TRACE_ENTRY( SDB__DMSINTERNALSCHEMAWRITER_UNSETINDEXCOLUMN ) ;

      UINT16 columnID = DMS_SCHEMA_INVALID_COLUMNID ;

      SDB_ASSERT( columnName, "Column name is invalid" ) ;

      columnID = _schemaHashWriter.getColumnIDByName( columnName ) ;
      if ( DMS_SCHEMA_INVALID_COLUMNID != columnID )
      {
         _schemaWriter.unsetIndexColumn( columnID ) ;
         PD_LOG( PDDEBUG, "Remove index column flag for column %s", columnName ) ;
      }
      // If the column does not exist in the internal schema, just ignore.
      PD_TRACE_EXIT( SDB__DMSINTERNALSCHEMAWRITER_UNSETINDEXCOLUMN ) ;
      return SDB_OK ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSINTERNALSCHEMAWRITER_UPDATESCHEMABYRECORD, "_dmsInternalSchemaWriter::updateSchemaByRecord" )
   INT32 _dmsInternalSchemaWriter::updateSchemaByRecord( const BSONObj &record )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__DMSINTERNALSCHEMAWRITER_UPDATESCHEMABYRECORD ) ;
      UINT16 columnID = DMS_SCHEMA_INVALID_COLUMNID ;

      try
      {
         BSONObjIterator itr( record ) ;
         while ( itr.more() )
         {
            BSONElement ele = itr.next() ;
            columnID = _schemaHashWriter.getColumnIDByName( ele.fieldName() ) ;
            if ( DMS_SCHEMA_INVALID_COLUMNID == columnID )
            {
               // New column for the schema.
               rc = addColumn( ele.fieldName(), NULL, &columnID ) ;
               PD_RC_CHECK( rc, PDERROR, "Add column %s into internal schema failed, rc: %d",
                            ele.fieldName(), rc ) ;
               if ( !_isInnerChange )
               {
                  _isInnerChange = TRUE ;
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
      PD_TRACE_EXITRC( SDB__DMSINTERNALSCHEMAWRITER_UPDATESCHEMABYRECORD, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSINTERNALSCHEMAWRITER__MERGE2COLUMN, "_dmsInternalSchemaWriter::_merge2Column" )
   INT32 _dmsInternalSchemaWriter::_merge2Column( UINT16 columnID, const BSONObj &columnDef )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__DMSINTERNALSCHEMAWRITER__MERGE2COLUMN ) ;

      try
      {
         // Currently only read and write default values can be merged. So we directly use the alter
         // column function.
         rc = _schemaWriter.alterColumn( columnID, columnDef ) ;
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
      PD_TRACE_EXITRC( SDB__DMSINTERNALSCHEMAWRITER__MERGE2COLUMN, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   INT32 _dmsInternalSchemaWriter::_hideColumn( const CHAR *columnName, UINT16 columnID )
   {
      INT32 rc = SDB_OK ;

      SDB_ASSERT( columnName, "Column name is null" ) ;

      if ( DMS_SCHEMA_INVALID_COLUMNID == columnID )
      {
         columnID = _schemaHashWriter.getColumnIDByName( columnName ) ;
         if ( DMS_SCHEMA_INVALID_COLUMNID == columnID )
         {
            rc = SDB_SYS ;
            PD_LOG( PDERROR, "The column [%s] to hide does not exist, rc: %d", columnName, rc ) ;
            goto error ;
         }
      }

      // Drop the entry of the column in the hash table.
      _schemaHashWriter.dropColumnItemByName( columnName ) ;

      rc = _schemaWriter.hideColumn( columnID ) ;
      PD_RC_CHECK( rc, PDERROR, "Hide column [Name: %s, ID: %u] in internal schema failed, rc: %d",
                   columnName, columnID, rc) ;

      PD_LOG( PDDEBUG, "Mark column [Name: %s, ID: %u] as hidden in internal schema successfully",
              columnName, columnID ) ;

   done:
      return rc ;
   error:
      goto done ;
   }

}
