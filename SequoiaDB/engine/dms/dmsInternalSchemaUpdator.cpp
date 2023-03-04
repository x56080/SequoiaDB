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
   _dmsSchemaWriter::_dmsSchemaWriter()
   : _extent( NULL )
   {
   }

   _dmsSchemaWriter::~_dmsSchemaWriter()
   {
   }

   INT32 _dmsSchemaWriter::init( dmsSchemaExtent *extent, UINT32 extentSize, UINT16 mbID,
                                 BOOLEAN create )
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

      if ( create )
      {
         ossMemset( (CHAR *)_offset2Ptr( DMS_SCHEMAEXTENT_HEADER_SZ ), 0,
                    extentSize - DMS_SCHEMAEXTENT_HEADER_SZ ) ;
      }

      rc = _dmsSchemaContainer::init( _extent, extentSize, mbID ) ;
      PD_RC_CHECK( rc, PDERROR, "Init internal schema container failed, rc: %d", rc ) ;

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _dmsSchemaWriter::addColumn( const CHAR *name, const BSONObj *columnDef,
                                         UINT16 &columnID, const CHAR *origName )
   {
      INT32 rc = SDB_OK ;
      UINT32 recOffset = 0 ;
      UINT16 recLength = 0 ;
      UINT16 allocSize = 0 ;
      UINT8 attr = 0 ;
      dmsSchemaColRecBuilder builder ;
      dmsSchemaColSlot* slot = NULL ;
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

      ossMemcpy( (CHAR *)_offset2Ptr(recOffset), (CHAR *)colInfoRec, recLength ) ;

      columnID = _extent->_itemNum ;

      // Increase the item number at last and flush immediately.
      _extent->_valueOffset = recOffset ;
      ++_extent->_itemNum ;
      _extent->_freeSpace -= allocSize ;

      slot = (dmsSchemaColSlot *)_getColSlot( columnID ) ;
      slot->setAttr( attr ) ;
      slot->setOffset( recOffset ) ;

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _dmsSchemaWriter::alterColumn( UINT16 columnID, const BSONObj &columnInfo )
   {
      INT32 rc = SDB_OK ;

      SDB_ASSERT( DMS_SCHEMA_INVALID_COLUMNID != columnID, "Column id is invalid" ) ;

      BSONElement ele ;
      dmsSchemaColRecBuilder builder ;
      UINT8 attr = _getColumnAttr( columnID ) ;
      UINT8 newAttr = attr ;
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
            setColumnAttr( columnID, newAttr, FALSE ) ;
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

   INT32 _dmsSchemaWriter::dropColumn( UINT16 columnID )
   {
      setColumnAttr( columnID, DMS_SCHEMA_COL_DELETED, FALSE ) ;
      return SDB_OK ;
   }

   INT32 _dmsSchemaWriter::dropColumnDefault( UINT16 columnID, BOOLEAN dropWrite,
                                                 BOOLEAN dropRead )
   {
      INT32 rc = SDB_OK ;
      UINT8 attr = 0 ;
      UINT32 offset = 0 ;
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

      offset = _getColRecordOffset( columnID ) ;
      ossMemcpy( (CHAR *)_offset2Ptr(offset), newRecord, newRecord->getLength() ) ;

      unsetColumnAttr( columnID, attr ) ;

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _dmsSchemaWriter::renameColumn( UINT16 columnID, const CHAR *newName )
   {
      // Keep the id of the column unchanged, just change the column information. The size of the
      // column information may grow, and need another place to store. So the offset may change.
      // If the original space is enough, just do in-place update.

      INT32 rc = SDB_OK ;
      UINT8 attr = _getColumnAttr( columnID ) ;
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
         setColumnAttr( columnID, DMS_SCHEMA_COL_HAS_ORIGNAME, FALSE ) ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _dmsSchemaWriter::_allocSpace4ColRecord( UINT16 slotSize, UINT16 valueSize,
                                                  UINT32 &offset, UINT16 *allocSize )
   {
      INT32 rc = SDB_OK ;
      UINT16 requiredSize = ossRoundUpToMultipleX( valueSize, 4 ) + slotSize ;

      if ( _freeSpace() < requiredSize )
      {
         // TODO: YSD add another extent ;
         // Try to compact

         rc = SDB_SYS ;
         PD_LOG( PDERROR, "Too much column info in internal schema, rc: %d", rc ) ;
         goto error ;
      }

      if ( 0 == _extent->_valueOffset )
      {
         offset = _extentSize - requiredSize ;
      }
      else
      {
         offset = _extent->_valueOffset - requiredSize ;
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

   INT32 _dmsSchemaWriter::_updateColRecord( UINT16 columnID,
                                                const dmsSchemaColRecord *oldRecord,
                                                const dmsSchemaColRecord *newRecord )
   {
      INT32 rc = SDB_OK ;

      SDB_ASSERT( oldRecord && newRecord, "Record is null" ) ;

      if ( newRecord->getLength() <= oldRecord->getLength() )
      {
         ossMemcpy( (CHAR *)oldRecord, newRecord, newRecord->getLength() ) ;
         if ( newRecord->getLength() < oldRecord->getLength() )
         {
            _extent->_freeSpace += ( oldRecord->getLength() - newRecord->getLength() ) ;
         }
      }
      else
      {
         // Need to allocate new space to store the new record.
         // 1. Allocate new space.
         // 2. Copy the new record to the space.
         // 3. Update the offset in the column slot, and flush immediately.
         UINT32 newOffset = 0 ;
         UINT16 allocSize = 0 ;
         dmsSchemaColSlot *slot = (dmsSchemaColSlot *)_getColSlot( columnID ) ;

         rc = _allocSpace4ColRecord( DMS_SCHEMAEXTENT_SLOT_SZ, newRecord->getLength(),
                                     newOffset, &allocSize ) ;
         PD_RC_CHECK( rc, PDERROR, "Allocate space for new column infor in internal schema extent "
                      "failed, rc: %d", rc ) ;

         ossMemcpy( (CHAR *)_offset2Ptr(newOffset), (CHAR *)newRecord, newRecord->getLength() ) ;

         slot->setOffset( newOffset ) ;

         _extent->_valueOffset = newOffset ;
         _extent->_freeSpace += oldRecord->getLength() - allocSize ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   void _dmsSchemaWriter::setIndexColumn( UINT16 columnID )
   {
      setColumnAttr( columnID, DMS_SCHEMA_COL_IN_INDEX, FALSE ) ;
   }

   void _dmsSchemaWriter::unsetIndexColumn( UINT16 columnID )
   {
      unsetColumnAttr( columnID, DMS_SCHEMA_COL_IN_INDEX ) ;
   }

   void _dmsSchemaWriter::setColumnAttr( UINT16 columnID, UINT8 attr, BOOLEAN replace )
   {
      dmsSchemaColSlot* pSlot = (dmsSchemaColSlot *)_getColSlot( columnID ) ;
      if ( pSlot )
      {
         if ( replace )
         {
            pSlot->setAttr( attr ) ;
         }
         else
         {
            pSlot->setAttrBits( attr ) ;
         }
      }
   }

   void _dmsSchemaWriter::unsetColumnAttr( UINT16 columnID, UINT8 attr )
   {
      dmsSchemaColSlot* pSlot = (dmsSchemaColSlot *)_getColSlot( columnID ) ;
      if ( pSlot )
      {
         pSlot->clearAttrBits( attr ) ;
      }
   }

   UINT32 _dmsSchemaWriter::_freeSpace() const
   {
      UINT32 freeSpace = 0 ;

      if ( 0 == _extent->_itemNum )
      {
         // No item yet
         freeSpace = _extentSize - DMS_SCHEMAEXTENT_HEADER_SZ ;
      }
      else
      {
         freeSpace = _extent->_valueOffset - DMS_SCHEMAEXTENT_HEADER_SZ -
                     DMS_SCHEMAEXTENT_SLOT_SZ * _extent->_itemNum ;
      }
      return freeSpace ;
   }

   _dmsSchemaHashWriter::_dmsSchemaHashWriter()
   : _extent( NULL ),
     _schemaWriter( NULL )
   {
   }

   _dmsSchemaHashWriter::~_dmsSchemaHashWriter()
   {
   }

   INT32 _dmsSchemaHashWriter::init( dmsSchemaWriter *schemaWriter, dmsSchemaHashExtent *extent,
                                     UINT32 extentSize, UINT16 mbID, BOOLEAN create )
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

      _extent = extent ;
      _schemaWriter = schemaWriter ;

      if ( create )
      {
         ossMemset( (CHAR *)_offset2Ptr( DMS_SCHEMAHASHEXTENT_HEADER_SZ ), 0xFF,
                    extentSize - DMS_SCHEMAHASHEXTENT_HEADER_SZ ) ;
      }

      rc = _dmsSchemaHash::init( schemaWriter, extent, extentSize, mbID ) ;
      PD_RC_CHECK( rc, PDERROR, "Init internal schema hash table failed, rc: %d", rc ) ;

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _dmsSchemaHashWriter::addColumnItem( const CHAR *name, UINT16 columnID )
   {
      INT32 rc = SDB_OK ;
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
            INT32 freeSlotID = _nextFreeListSlotID() ;
            if ( -1 == freeSlotID )
            {
               rc = SDB_SYS ;
               PD_LOG( PDERROR, "Find a free slot in internal schema hash table failed, rc: %d",
                       rc ) ;
               goto error ;
            }

            newSlot = (dmsSchemaHashSlot *)_getHashListSlot( freeSlotID ) ;
            newSlot->setColumnID( columnID ) ;
            newSlot->setNextSlotID( DMS_SCHEMA_HASH_INVALID_SLOTID ) ;
            hashSlot->setNextSlotID( freeSlotID ) ;
            if ( freeSlotID >= _extent->_slotNum )
            {
               _extent->_slotNum = freeSlotID + 1 ;
            }
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
      return rc ;
   error:
      goto done ;
   }

   INT32 _dmsSchemaHashWriter::dropColumnItemByName( const CHAR *name )
   {
      INT32 rc = SDB_OK ;
      INT32 *item = NULL ;
      INT32 *prevItem = NULL ;
      const CHAR *columnName = NULL ;
      UINT16 slotID = DMS_SCHEMA_INVALID_SLOT_ID ;
      UINT16 columnID = DMS_SCHEMA_INVALID_COLUMNID ;
      BOOLEAN found = FALSE ;

      UINT32 bucketID = ossHash( name ) % DMS_SCHEMA_HASH_BUCKET_SIZE ;
      dmsSchemaHashSlot* slot = (dmsSchemaHashSlot *)_getHashBucketSlot( bucketID ) ;
      dmsSchemaHashSlot* lastSlot = NULL ;

      while ( slot )
      {
         columnID = slot->getColumnID() ;
         if ( DMS_SCHEMA_INVALID_COLUMNID == columnID )
         {
            // Not found
            goto done ;
         }

         columnName = _schemaContainer->getColumnName( columnID ) ;
         if ( 0 == ossStrcmp( name, columnName ) )
         {
            // Found
            found = TRUE ;
            break ;
         }
         else
         {
            slotID = slot->getNextSlotID() ;
            if ( DMS_SCHEMA_INVALID_SLOT_ID == slotID )
            {
               // Not found
               goto done ;
            }

            lastSlot = slot ;
            slot = (dmsSchemaHashSlot *)_getHashListSlot( slotID ) ;
         }
      }

      if ( !found )
      {
         goto done ;
      }

      if ( lastSlot )
      {
         lastSlot->setNextSlotID( slot->getNextSlotID() ) ;
         slot->reset() ;
      }
      else
      {
         // Delete the slot in the bucket. Need to move the next item(if any) to the bucket.
         // Otherwise we are not able to find the entry of this bucket.
         slotID = slot->getNextSlotID() ;
         

      }


      // while ( DMS_SCHEMA_INVALID_SLOT_OFFSET != itemOffset )
      // {
      //    item = (INT32 *)_offset2Ptr( itemOffset ) ;
      //    columnID = _getColumnIDByItem( item ) ;
      //    columnName = _schemaWriter->getColumnName( columnID ) ;
      //    if ( 0 == ossStrcmp( name, columnName ) )
      //    {
      //       // Found
      //       break ;
      //    }
      //    else
      //    {
      //       prevItem = item ;
      //       itemOffset = _getNextItemOffset( item ) ;
      //    }
      // }

      // if ( DMS_SCHEMA_INVALID_SLOT_OFFSET == itemOffset )
      // {
      //    // Not found
      //    goto done ;
      // }
      // else
      // {
      //    // Check if the current item has a next item.
      //    UINT16 nextItemOffset = _getNextItemOffset( item ) ;
      //    if ( prevItem )
      //    {
      //       _setNextItemOffset( prevItem, _getNextItemOffset( item ) ) ;
      //       _resetItem( item ) ;
      //    }
      //    else if ( DMS_SCHEMA_INVALID_SLOT_OFFSET == nextItemOffset )
      //    {
      //       // No next, only this one.
      //       _resetItem( item ) ;
      //    }
      //    else
      //    {
      //       INT32 *nextItem = (INT32 *)_offset2Ptr( nextItemOffset ) ;
      //       *item = *nextItem ;
      //       _resetItem( nextItem ) ;
      //    }
      // }

   done:
      return rc ;
   }

   _dmsInternalSchemaWriter::_dmsInternalSchemaWriter( BOOLEAN createNew )
   : _origSchemaExtent( NULL ),
     _origHashExtent( NULL ),
     _newSchemaExtent( NULL ),
     _newHashExtent( NULL ),
     _changed( FALSE )
   {
   }

   _dmsInternalSchemaWriter::~_dmsInternalSchemaWriter()
   {
      SAFE_OSS_FREE( _newSchemaExtent ) ;
      SAFE_OSS_FREE( _newHashExtent ) ;
   }

   INT32 _dmsInternalSchemaWriter::init( const dmsInternalSchema *pSchema,
                                         dmsStorageDataCommon *su,
                                         dmsMBContext *context,
                                         pmdEDUCB *cb )
   {
      INT32 rc = SDB_OK ;

      if ( !pSchema )
      {
         rc = SDB_SYS ;
         PD_LOG( PDERROR, "Internal schema for init the schema writer failed, rc: %d", rc ) ;
         goto error ;
      }

      // Get the existing extent from the schema. If both of them are empty, build the schema from
      // scratch.
      _origSchemaExtent = pSchema->getSchemaContainer()->getExtent() ;
      _origHashExtent = pSchema->getSchemaHashTable()->getExtent() ;

      // if ( _origSchemaExtent && !_origHashExtent || ( !_origSchemaExtent && _origHashExtent ) )









   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _dmsInternalSchemaWriter::save( dmsMBContext *context, pmdEDUCB *cb )
   {
      INT32 rc = SDB_OK ;

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _dmsInternalSchemaWriter::init( dmsStorageDataCommon *su, _dmsMBContext *context,
                                         dmsSchemaExtent *schemaExtent,
                                         dmsSchemaHashExtent *hashExtent,
                                         BOOLEAN create )
   {
      INT32 rc = SDB_OK ;

      UINT32 schemaExtentSize = schemaExtent->_blockSize << su->pageSizeSquareRoot() ;
      UINT32 hashExtentSize = hashExtent->_blockSize << su->pageSizeSquareRoot() ;

      _newSchemaExtent = (dmsSchemaExtent *)SDB_OSS_MALLOC( schemaExtentSize ) ;
      if ( !_newSchemaExtent)
      {
         rc = SDB_OOM ;
         PD_LOG( PDERROR, "Allocate memory of size [%u] for updating internal schema failed, "
                 "rc: %d", schemaExtentSize, rc ) ;
         goto error ;
      }

      _newHashExtent = (dmsSchemaHashExtent *)SDB_OSS_MALLOC( hashExtentSize ) ;
      if ( !_newHashExtent )
      {
         rc = SDB_OOM ;
         PD_LOG( PDERROR, "Allocate memory of size [%u] for updating internal schema hash failed, "
                 "rc",  hashExtentSize, rc ) ;
         goto error ;
      }

      ossMemcpy( (CHAR *)_newSchemaExtent, (CHAR *)schemaExtent, schemaExtentSize ) ;
      ossMemcpy( (CHAR *)_newHashExtent, (CHAR *)hashExtent, hashExtentSize ) ;

      // Init writers to update the internal schema.
      rc = _schemaWriter.init( _newSchemaExtent, schemaExtentSize, context->mbID(), create ) ;
      PD_RC_CHECK( rc, PDERROR, "Init internal schema writer failed, rc: %d", rc ) ;
      rc = _schemaHashWriter.init( &_schemaWriter, _newHashExtent, hashExtentSize,
                                   context->mbID(), create ) ;
      PD_RC_CHECK( rc, PDERROR, "Init internal schema hash writer failed, rc: %d", rc ) ;

      _origSchemaExtent = schemaExtent ;
      _origHashExtent = hashExtent ;

      rc = dmsInternalSchema::init( _newSchemaExtent, schemaExtentSize, _newHashExtent,
                                    hashExtentSize, context->mbID() ) ;
      PD_RC_CHECK( rc, PDERROR, "Init internal schema failed, rc: %d", rc ) ;

   done:
      return rc ;
   error:
      SAFE_OSS_FREE( _newSchemaExtent ) ;
      SAFE_OSS_FREE( _newHashExtent ) ;
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSINTERNALSCHEMAWRITER_ADDCOLUMN, "_dmsInternalSchemaWriter::addColumn" )
   INT32 _dmsInternalSchemaWriter::addColumn( const CHAR *columnName,
                                              const BSONObj *columnDef, UINT16 *columnID,
                                              BOOLEAN mergeOnExist, const CHAR *origName )
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

      rc = _schemaWriter.addColumn( columnName, columnDef, id, origName ) ;
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

   INT32 _dmsInternalSchemaWriter::dropColumn( const CHAR *columnName, BOOLEAN *colNotFound )
   {
      INT32 rc = SDB_OK ;
      UINT16 columnID = DMS_SCHEMA_INVALID_COLUMNID ;

      // Get ID of the column from the hash table.
      columnID = _schemaHashWriter.getColumnIDByName( columnName ) ;
      if ( DMS_SCHEMA_INVALID_COLUMNID == columnID )
      {
         if ( colNotFound )
         {
            *colNotFound = TRUE ;
         }
         PD_LOG( PDDEBUG, "Column %s does not exist when dropping", columnName ) ;
         goto done ;
      }

      if ( colNotFound )
      {
         *colNotFound = FALSE ;
      }

      rc = _schemaHashWriter.dropColumnItemByName( columnName ) ;
      PD_RC_CHECK( rc, PDERROR, "Drop column item in schema hash table for column %s failed, "
                   "rc: %d", columnName, rc ) ;

      // mark as deleted in the schema container
      rc = _schemaWriter.dropColumn( columnID ) ;
      PD_RC_CHECK( rc, PDERROR, "Drop column information in internal schema failed, rc: %d", rc ) ;

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _dmsInternalSchemaWriter::renameColumn( const CHAR *oldName, const CHAR *newName,
                                                 BOOLEAN *colNotFound )
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
      columnID = _schemaHashWriter.getColumnIDByName( oldName ) ;
      if ( DMS_SCHEMA_INVALID_COLUMNID == columnID )
      {
         // For internal schema, it's normal that the name we want to rename does not exist.
         PD_LOG( PDDEBUG, "Old name [%s] does not exist in the local internal schema when "
                 "renaming, rc: %d", oldName, rc ) ;
         if ( colNotFound )
         {
            *colNotFound = TRUE ;
         }
         goto done ;
      }

      if ( colNotFound )
      {
         *colNotFound = FALSE ;
      }

      if ( DMS_SCHEMA_INVALID_COLUMNID != _schemaHashWriter.getColumnIDByName( newName ) )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG( PDERROR, "Can not rename %s to %s as the targe name already exist, rc: %d",
                 oldName, newName, rc ) ;
         goto error ;
      }

      // 2. Drop the entry in the hash table.
      rc = _schemaHashWriter.dropColumnItemByName( oldName ) ;
      PD_RC_CHECK( rc, PDERROR, "Drop schema hash entry when rename internal schema column %s "
                  "failed, rc: %d", oldName, rc ) ;

      // 3. Rename the column and update the column information in schema extent.
      rc = _schemaWriter.renameColumn( columnID, newName ) ;
      PD_RC_CHECK( rc, PDERROR, "Rename internal schema column name %s to new name %s failed, "
                   "rc: %d", oldName, newName, rc ) ;

      // 4. Add new entry in the hash table for the new name. The column ID should not change.
      rc = _schemaHashWriter.addColumnItem( newName, columnID ) ;
      PD_RC_CHECK( rc, PDERROR, "Add new schema hash entry for internal schema column failed, "
                   "rc: %d", rc ) ;

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _dmsInternalSchemaWriter::dropColumnDefault( const CHAR *name )
   {
      INT32 rc = SDB_OK ;

      UINT16 columnID = _schemaHashWriter.getColumnIDByName( name ) ;

      if ( DMS_SCHEMA_INVALID_COLUMNID == columnID )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG( PDERROR, "The column name %s does not exist in internal schema, rc: %d",
                 name, rc ) ;
         goto error ;
      }

      rc = _schemaWriter.dropColumnDefault( columnID, TRUE ) ;
      PD_RC_CHECK( rc, PDERROR, "Drop write default for column %s failed, rc: %d", name, rc ) ;

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _dmsInternalSchemaWriter::alterColumn( const CHAR *columnName,
                                                const BSONObj &columnDef,
                                                BOOLEAN *colNotFound )
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

      UINT16 columnID = _schemaHashWriter.getColumnIDByName( columnName ) ;
      if ( DMS_SCHEMA_INVALID_COLUMNID == columnID )
      {
         // Case 1
         PD_LOG( PDDEBUG, "Column [%s] does not exist when altering", columnName ) ;
         if ( colNotFound )
         {
            *colNotFound = TRUE ;
         }
         goto done ;
      }

      if ( colNotFound )
      {
         *colNotFound = FALSE ;
      }

      // Case 2/3
      rc = _schemaWriter.alterColumn( columnID, columnDef ) ;
      PD_RC_CHECK( rc, PDERROR, "Alter column %s info in internal schema failed, rc: %d",
                   columnName, rc ) ;

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _dmsInternalSchemaWriter::setIndexColumn( const CHAR *columnName )
   {
      INT32 rc = SDB_OK ;
      UINT16 columnID = DMS_SCHEMA_INVALID_COLUMNID ;

      SDB_ASSERT( columnName, "Column name is invalid" ) ;

      columnID = _schemaHashWriter.getColumnIDByName( columnName ) ;
      if ( DMS_SCHEMA_INVALID_COLUMNID == columnID )
      {
         // If the column does not exist in the internal schema, need to add it.
         rc = addColumn( columnName, NULL, &columnID ) ;
         PD_RC_CHECK( rc, PDERROR, "Add index column %s column name to internal schema failed, "
                      "rc: %d", columnName, rc ) ;
      }

      _schemaWriter.setIndexColumn( columnID ) ;
      PD_LOG( PDDEBUG, "Set index column flag for column %s", columnName ) ;

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _dmsInternalSchemaWriter::unsetIndexColumn( const CHAR *columnName )
   {
      UINT16 columnID = DMS_SCHEMA_INVALID_COLUMNID ;

      SDB_ASSERT( columnName, "Column name is invalid" ) ;

      columnID = _schemaHashWriter.getColumnIDByName( columnName ) ;
      if ( DMS_SCHEMA_INVALID_COLUMNID != columnID )
      {
         _schemaWriter.unsetIndexColumn( columnID ) ;
         PD_LOG( PDDEBUG, "Remove index column flag for column %s", columnName ) ;
      }
      // If the column does not exist in the internal schema, just ignore.

      return SDB_OK ;
   }

   INT32 _dmsInternalSchemaWriter::updateSchemaByRecord( const BSONObj &record )
   {
      INT32 rc = SDB_OK ;
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

   INT32 _dmsInternalSchemaWriter::save( dmsInternalSchema *schema, dmsMBContext *context )
   {
      INT32 rc = SDB_OK ;

      if ( _newSchemaExtent && _newHashExtent )
      {
         if ( !context->isMBLock( EXCLUSIVE ) )
         {
            rc = SDB_SYS ;
            PD_LOG( PDERROR, "Collection mb latch should be taken in EXCLUSIVE mode when save new "
                    "internal schema, rc: %d", rc ) ;
            goto error ;
         }

         ++_newSchemaExtent->_schemaVersion ;

         ossMemcpy( (CHAR *)_origSchemaExtent, _newSchemaExtent, _schemaWriter.getExtentSize() ) ;
         ossMemcpy( (CHAR *)_origHashExtent, _newHashExtent, _schemaHashWriter.getExtentSize() ) ;

         rc = schema->reload( _origSchemaExtent, _schemaWriter.getExtentSize(),
                              _origHashExtent, _schemaHashWriter.getExtentSize(),
                              context->mbID() ) ;
         PD_RC_CHECK( rc, PDERROR, "Reload internal schema for collection[%s] failed, rc: %d",
                      context->mb()->_collectionName, rc ) ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _dmsInternalSchemaWriter::_merge2Column( UINT16 columnID, const BSONObj &columnDef )
   {
      INT32 rc = SDB_OK ;

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
      return rc ;
   error:
      goto done ;
   }

}
