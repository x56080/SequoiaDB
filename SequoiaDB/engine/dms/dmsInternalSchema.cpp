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
#include "msgDef.hpp"
#include "pmdEDU.hpp"
#include "pd.hpp"
#include "utilStr.hpp"
#include "pdTrace.hpp"
#include "dmsTrace.hpp"

#define DMS_SCHEMA_COLID_STR_MAX_SIZE              5

using namespace bson ;

namespace engine
{
   /*
      _dmsSchemaContainer implement
   */
   _dmsSchemaContainer::_dmsSchemaContainer()
   : _extent( NULL ),
     _extentSize( 0 )
   {
   }

   _dmsSchemaContainer::~_dmsSchemaContainer()
   {
      reset() ;
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
      else if ( !extent->validate( mbID ) )
      {
         rc = SDB_DMS_CORRUPTED_EXTENT ;
         PD_LOG( PDERROR, "Internal schema extent is invalid, rc: %d", rc ) ;
         goto error ;
      }
      else if ( extentSize < DMS_PAGE_SIZE4K )
      {
         rc = SDB_SYS ;
         PD_LOG( PDERROR, "Extent size[%u] less than the min[%u]",
                 extentSize, DMS_PAGE_SIZE4K ) ;
         goto error ;
      }
      else if ( extentSize > DMS_SCHEMA_EXTENT_MAX_SZ )
      {
         rc = SDB_SYS ;
         PD_LOG( PDERROR, "Extent size[%u] greater than the max[%u]",
                 extentSize, DMS_SCHEMA_EXTENT_MAX_SZ ) ;
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

   INT32 _dmsSchemaContainer::getColReadDefault( UINT16 columnID, const CHAR *&name,
                                                 INT32 &nameLen, BSONType &type,
                                                 const CHAR *&value, INT32 &valueLen ) const
   {
      INT32 rc = SDB_OK ;
      const dmsSchemaColRecord *colRecord = _getColRecord( columnID ) ;
      if ( !colRecord )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG( PDERROR, "Column info of column id %u is invalid, rc: %d", columnID, rc ) ;
         goto error ;
      }

      name = colRecord->getName( &nameLen ) ;
      SDB_ASSERT( name, "Name is invalid" ) ;

      if ( !colRecord->getDefault( type, valueLen, value, TRUE ) )
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
         rc = SDB_INVALIDARG ;
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
      UINT8 attr = 0 ;
      const dmsSchemaColRecord *record = _getColRecord( columnID ) ;

      if ( !record )
      {
         PD_LOG( PDERROR, "Get column record[%d] failed", columnID ) ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      attr = _getColumnAttr( columnID ) ;
      if ( name )
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

   done:
      return rc ;
   error:
      goto done ;
   }

   const CHAR *_dmsSchemaContainer::getOrigName( UINT16 columnID, INT32 *nameLen ) const
   {
      const dmsSchemaColRecord *record = _getColRecord( columnID ) ;
      if ( record )
      {
         return record->getOrigName( nameLen ) ;
      }

      if ( nameLen )
      {
         *nameLen = 0 ;
      }
      return NULL ;
   }

   INT32 _dmsSchemaContainer::toObj( BSONObj &object ) const
   {
      INT32 rc = SDB_OK ;

      if ( !_extent )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG( PDERROR, "Extent is invalid" ) ;
         goto error ;
      }

      try
      {
         BSONObjBuilder builder( _extentSize - _extent->_freeSpace ) ;

         for ( UINT16 columnID = 0; columnID < _extent->_itemNum ; ++columnID )
         {
            /// ignore delete columns
            if ( isColumnDeleted( columnID ) )
            {
               continue ;
            }

            rc = _columnInfo2Def( columnID, builder ) ;
            PD_RC_CHECK( rc, PDERROR, "Get column definition failed, rc: %d", rc ) ;
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
         rc = SDB_INVALIDARG ;
         PD_LOG( PDERROR, "Get column info for column id %u failed, rc: %d", columnID, rc ) ;
         goto error ;
      }

      size = record->getLength() ;

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _dmsSchemaContainer::getReadEleSize( UINT16 columnID, UINT32 &size ) const
   {
      INT32 rc = SDB_OK ;

      size = 0 ;

      if ( hasReadDefault( columnID ) )
      {
         const CHAR *name = NULL ;
         INT32 nameLen = 0 ;
         BSONType type = EOO ;
         const CHAR *value = NULL ;
         INT32 valueLen = 0 ;

         rc = getColReadDefault( columnID, name, nameLen, type, value, valueLen ) ;
         PD_RC_CHECK( rc, PDERROR, "Get column[%d] read default failed, rc: %d",
                      columnID, rc ) ;

         size = nameLen + 1 + valueLen ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _dmsSchemaContainer::getWriteEleSize( UINT16 columnID, UINT32 &size ) const
   {
      INT32 rc = SDB_OK ;

      size = 0 ;

      if ( hasWriteDefault( columnID ) )
      {
         const CHAR *name = NULL ;
         INT32 nameLen = 0 ;
         BSONType type = EOO ;
         const CHAR *value = NULL ;
         INT32 valueLen = 0 ;

         rc = getColWriteDefault( columnID, name, nameLen, type, value, valueLen ) ;
         PD_RC_CHECK( rc, PDERROR, "Get column[%d] write default failed, rc: %d",
                      columnID, rc ) ;

         size = nameLen + 1 + valueLen ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _dmsSchemaContainer::dump( BSONObj &schemaObj, BOOLEAN includeColumnID ) const
   {
      INT32 rc = SDB_OK ;

      if ( !_extent )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG( PDERROR, "Extent is invalid" ) ;
         goto error ;
      }

      try
      {
         BSONObjBuilder builder( _extentSize - _extent->_freeSpace ) ; ;
         BSONArrayBuilder columnBuilder( builder.subarrayStart( FIELD_NAME_COLUMNS ) ) ;

         for ( UINT16 columnID = 0 ; columnID < _extent->_itemNum ; ++columnID )
         {
            BSONObjBuilder subBuilder( columnBuilder.subobjStart() ) ;

            rc = _columnInfo2Obj( columnID, subBuilder, includeColumnID, TRUE ) ;
            if ( rc )
            {
               PD_LOG( PDERROR, "Get column info failed, rc: %d", rc ) ;
               goto error ;
            }

            subBuilder.done() ;
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

   INT32 _dmsSchemaContainer::_columnInfo2Def( UINT16 columnID, BSONObjBuilder &builder ) const
   {
      INT32 rc = SDB_OK ;

      BSONType type = EOO ;
      const CHAR *value = NULL ;
      INT32 valueSize = 0 ;
      const CHAR *pName = NULL ;

      const dmsSchemaColRecord *pRecord = _getColRecord( columnID ) ;
      if ( !pRecord )
      {
         PD_LOG( PDERROR, "Get column record failed" ) ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      pName = pRecord->getName() ;
      SDB_ASSERT( pName, "Column name is invalid" ) ;

      try
      {
         BSONObjBuilder subBuilder( builder.subobjStart( pName ) ) ;

         /// read default
         if ( pRecord->getDefault( type, valueSize, value, TRUE ) )
         {
            subBuilder.appendRawEle( type, StringData( FIELD_NAME_READDEFAULT,
                                                    sizeof(FIELD_NAME_READDEFAULT)-1),
                                     (const void *)value, valueSize ) ;
         }

         /// write default
         if ( pRecord->getDefault( type, valueSize, value, FALSE ) )
         {
            subBuilder.appendRawEle( type, StringData( FIELD_NAME_WRITEDEFAULT,
                                                    sizeof(FIELD_NAME_WRITEDEFAULT)-1),
                                     (const void *)value, valueSize ) ;
         }

         subBuilder.done() ;
      }
      catch( std::exception &e )
      {
         PD_LOG( PDERROR, "Occur exception: %s", e.what() ) ;
         rc = ossException2RC( &e ) ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _dmsSchemaContainer::_columnInfo2Obj( UINT16 columnID,
                                               BSONObjBuilder &builder,
                                               BOOLEAN includeID,
                                               BOOLEAN includeAttr ) const
   {
      INT32 rc = SDB_OK ;
      BSONType type = EOO ;
      INT32 valueSize = 0 ;
      const CHAR *value = NULL ;
      const CHAR *pName = NULL ;
      const CHAR *pOrgName = NULL ;

      const dmsSchemaColRecord *pRecord = _getColRecord( columnID ) ;
      if ( !pRecord )
      {
         PD_LOG( PDERROR, "Get column record failed" ) ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      pName = pRecord->getName() ;
      SDB_ASSERT( pName, "Column name is invalid" ) ;

      pOrgName = pRecord->getOrigName() ;

      try
      {
         if ( includeID )
         {
            builder.append( FIELD_NAME_ID, (INT32)columnID ) ;
         }

         builder.append( FIELD_NAME_NAME, pName ) ;
         if ( pOrgName && *pOrgName )
         {
            builder.append( FIELD_NAME_ORIGIN_NAME, pOrgName ) ;
         }

         /// read default
         if ( pRecord->getDefault( type, valueSize, value, TRUE ) )
         {
            builder.appendRawEle( type, StringData( FIELD_NAME_READDEFAULT,
                                                    sizeof(FIELD_NAME_READDEFAULT)-1),
                                  (const void *)value, valueSize ) ;
         }

         /// write default
         if ( pRecord->getDefault( type, valueSize, value, FALSE ) )
         {
            builder.appendRawEle( type, StringData( FIELD_NAME_WRITEDEFAULT,
                                                    sizeof(FIELD_NAME_WRITEDEFAULT)-1),
                                  (const void *)value, valueSize ) ;
         }

         if ( includeAttr )
         {
            if ( isColumnDeleted( columnID ) )
            {
               builder.appendBool( FIELD_NAME_DELETED, TRUE ) ;
            }
            if ( isIndexColumn( columnID ) )
            {
               builder.appendBool( FIELD_NAME_INDEX_COL, TRUE ) ;
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

   /*
      _dmsSchemaHash implement
   */
   _dmsSchemaHash::_dmsSchemaHash()
   : _schemaContainer( NULL ),
     _extent( NULL ),
     _extentSize( 0 ),
     _bucketNum( 0 ),
     _pBucketSlot( NULL ),
     _pListSlot( NULL )
   {
   }

   _dmsSchemaHash::~_dmsSchemaHash()
   {
      reset() ;
   }

   INT32 _dmsSchemaHash::init( const dmsSchemaContainer *schemaContainer,
                               const dmsSchemaHashExtent *extent, UINT32 extentSize, UINT16 mbID )
   {
      INT32 rc = SDB_OK ;

      if ( !extent || !schemaContainer )
      {
         rc = SDB_SYS ;
         PD_LOG( PDERROR, "Internal schema hash extent address is null, rc: %d", rc ) ;
         goto error ;
      }
      else if ( !extent->validate( mbID ) )
      {
         rc = SDB_DMS_CORRUPTED_EXTENT ;
         PD_LOG( PDERROR, "Internal schema hash extent is invalid, rc: %d", rc ) ;
         goto error ;
      }

      _bucketNum = _extent->_bucketNum ;
      /// check bucket num
      if ( _bucketNum < 2 || !ossIsPowerOf2( _bucketNum, NULL ) )
      {
         rc = SDB_DMS_CORRUPTED_EXTENT ;
         PD_LOG( PDERROR, "Internal schema hash extent bucket number[u%] invalid",
                 _bucketNum ) ;
         goto error ;
      }
      /// check slot num
      else if ( (UINT32)DMS_SCHEMAHASHEXTENT_HEADER_SZ +
                _bucketNum * DMS_HASHEXTENT_SLOT_SZ +
                extent->_slotNum * DMS_HASHEXTENT_SLOT_SZ > extentSize )
      {
         rc = SDB_DMS_CORRUPTED_EXTENT ;
         PD_LOG( PDERROR, "Internal schema hash extent slot number[u%] invalid",
                 extent->_slotNum ) ;
         goto error ;
      }

      _schemaContainer = schemaContainer ;
      _extent = extent ;
      _extentSize = extentSize ;

      _pBucketSlot = (const dmsSchemaHashSlot*)
                     ((const CHAR *)_extent + DMS_SCHEMAHASHEXTENT_HEADER_SZ) ;
      _pListSlot = &_pBucketSlot[ _bucketNum ] ;

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
      _pBucketSlot = NULL ;
      _pListSlot = NULL ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSSCHEMAHASH_GETCOLUMNIDBYNAME, "_dmsSchemaHash::getColumnIDByName" )
   UINT16 _dmsSchemaHash::getColumnIDByName( const CHAR *name ) const
   {
      PD_TRACE_ENTRY( SDB__DMSSCHEMAHASH_GETCOLUMNIDBYNAME ) ;
      UINT32 slotID = 0 ;
      const dmsSchemaHashSlot *pSlot = NULL ;
      UINT16 columnID = DMS_SCHEMA_INVALID_COLUMNID ;
      const CHAR *columnName = NULL ;

      if ( !name || !_schemaContainer )
      {
         SDB_ASSERT( FALSE, "Invalid name or container" ) ;
         goto done ;
      }

      slotID = ossHash( name ) & ( _bucketNum - 1 ) ;
      pSlot = _getHashBucketSlot( slotID ) ;

      while ( pSlot )
      {
         columnID = pSlot->getColumnID() ;

         if ( DMS_SCHEMA_INVALID_COLUMNID == columnID )
         {
            break ;
         }

         columnName = _schemaContainer->getColumnName( columnID ) ;
         if ( columnName && 0 == ossStrcmp( name, columnName ) )
         {
            // Found
            break ;
         }
         else
         {
            columnID = DMS_SCHEMA_INVALID_COLUMNID ;
            // Check if any conflict item.

            if ( pSlot->hasNextSlot() )
            {
               pSlot = _getHashListSlot( pSlot->getNextSlotID() ) ;
            }
            else
            {
               break ;
            }
         }
      }

   done:
      PD_TRACE_EXIT( SDB__DMSSCHEMAHASH_GETCOLUMNIDBYNAME ) ;
      return columnID ;
   }

   INT32 _dmsSchemaHash::_nextFreeItemOffset() const
   {
      if ( _pListSlot )
      {
         // Search in the conflict area for a free item.
         for ( UINT32 i = 0 ; i < _extent->_slotNum ; ++i )
         {
            if ( DMS_SCHEMA_INVALID_COLUMNID == _pListSlot[ i ].getColumnID() )
            {
               return i ;
            }
         }
      }

      return -1 ;
   }

   /*
      Encode object format:
      |  ...          | 1 Byte |    3 Byte    |
      |  Data         |  Flag  |    Version   |

      Data has two format:
      1. Orignal format, ex: { a:1, b:1, c:1 }
      2. Hex format,     ex: { 0:1, 1:1, 2:1 }
   */

   /*
      Flag define
   */
   #define DMS_SCHEMA_ENCODE_ORG             0x01
   #define DMS_SCHEMA_ENCODE_HEX             0x02

   #define DMS_SCHEMA_ENCODE_POSIXPTR(pData)       (((CHAR*)(pData)) + *(UINT32*)(pData) )
   #define DMS_SCHEMA_ENCODE_DATAPTR(pData)        ((CHAR*)(pData))

   #define DMS_SCHEMA_GET_ENCODE_FLAG(pData)       \
      ((*(UINT32*)DMS_SCHEMA_ENCODE_POSIXPTR(pData)) >> 24)

   #define DMS_SCHEMA_SET_ENCODE_FLAG(pData,flag)  \
      ((*(UINT32*)DMS_SCHEMA_ENCODE_POSIXPTR(pData)) = \
      (((*(UINT32*)DMS_SCHEMA_ENCODE_POSIXPTR(pData))&0x00FFFFFF) |\
      (((UINT32)((UINT8)flag)) << 24 )))

   #define DMS_SCHEMA_GET_VERSION(pData)           \
      ((*(UINT32*)DMS_SCHEMA_ENCODE_POSIXPTR(pData)) & 0x00FFFFFF)

   #define DMS_SCHEMA_SET_VERSION(pData,version)   \
      ((*(UINT32*)DMS_SCHEMA_ENCODE_POSIXPTR(pData)) = \
      (((*(UINT32*)DMS_SCHEMA_ENCODE_POSIXPTR(pData))&0xFF000000) |\
      (((UINT32)version)&0x00FFFFFF )))

   /*
      _dmsInternalSchema implement
   */
   _dmsInternalSchema::_dmsInternalSchema()
   : _enabled( FALSE ),
     _forceEncode( FALSE ),
     _schemaVersion( DMS_SCHEMA_INVALID_VERSION ),
     _schemaInnerVersion( DMS_SCHEMA_INVALID_VERSION ),
     _readColBitmap( 0 ),
     _writeColBitmap( 0 ),
     _defaultReadMaxSize( 0 ),
     _defaultWriteMaxSize( 0 ),
     _totalValidNameSize( 0 ),
     _hasLoad( FALSE )
   {
   }

   _dmsInternalSchema::~_dmsInternalSchema()
   {
   }

   INT32 _dmsInternalSchema::init( const dmsSchemaExtent *schemaExtent,
                                   UINT32 schemaExtentSize,
                                   const dmsSchemaHashExtent *hashExtent,
                                   UINT32 hashExtentSize,
                                   UINT16 mbID,
                                   BOOLEAN forceEncode )
   {
      _forceEncode = forceEncode ;
      return _init( schemaExtent, schemaExtentSize, hashExtent, hashExtentSize, mbID, FALSE ) ;
   }

   INT32 _dmsInternalSchema::reload( const dmsSchemaExtent *schemaExtent,
                                     UINT32 schemaExtentSize,
                                     const dmsSchemaHashExtent *hashExtent,
                                     UINT32 hashExtentSize,
                                     UINT16 mbID )
   {
      INT32 rc = SDB_OK ;

      ossScopedRWLock lock( &_loadRWMutex, EXCLUSIVE ) ;

      if ( schemaExtent == _schemaContainer.getExtent() &&
           schemaExtentSize == _schemaContainer.getExtentSize() &&
           hashExtent == _schemaHash.getExtent() &&
           hashExtentSize == _schemaHash.getExtentSize() &&
           _schemaVersion == schemaExtent->_schemaVersion &&
           _schemaInnerVersion == schemaExtent->_schemaInnerVersion )
      {
         /// don't reload
         goto done ;
      }

      reset() ;

      rc = _init( schemaExtent, schemaExtentSize, hashExtent, hashExtentSize, mbID, TRUE ) ;
      if ( rc )
      {
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   void _dmsInternalSchema::reset()
   {
      _schemaContainer.reset() ;
      _schemaHash.reset() ;

      _clearBitmapInfo() ;

      _enabled = FALSE ;
      _schemaVersion = DMS_SCHEMA_INVALID_VERSION ;
      _schemaInnerVersion = DMS_SCHEMA_INVALID_VERSION ;

      _hasLoad = FALSE ;
   }

   void _dmsInternalSchema::_clearBitmapInfo()
   {
      _readColBitmap.release() ;
      _writeColBitmap.release() ;
      _decodeWatchNames.clear() ;

      _totalValidNameSize = 0 ;
      _defaultReadMaxSize = 0 ;
      _defaultWriteMaxSize = 0 ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSINTERNALSCHEMA_ENCODERECORD, "_dmsInternalSchema::encodeRecord" )
   INT32 _dmsInternalSchema::encodeRecord( _pmdEDUCB *cb,
                                           dmsRecordData &recordData,
                                           BOOLEAN &memAlloc,
                                           dmsRecordData &encodeData,
                                           BOOLEAN &hasNewCol )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__DMSINTERNALSCHEMA_ENCODERECORD ) ;

      UINT32 totalSize = 0 ;
      CHAR *encodeRecord = NULL ;
      INT32 estimateSize = 0 ;
      BOOLEAN isEmpty = FALSE ;
      UINT8 encodeFlag = DMS_SCHEMA_ENCODE_HEX ;
      const BSONObj *pBaseObj = NULL ;
      ISession *session = cb->getSession() ;

      const CHAR *pOrgData = recordData.data() ;
      UINT32 orgDataLen    = recordData.len() ;

      dmsThreadSchemaBitmap writeBitmap( _schemaContainer.columnNum() ) ;
      utilBSONRawBuilder encodeBuilder ;

      BOOLEAN isPrimalData = FALSE ;
      BOOLEAN hasRetry = FALSE ;

      /// check is load
      if ( !_hasLoad )
      {
         ossScopedRWLock lock( &_loadRWMutex, EXCLUSIVE ) ;
         if ( !_hasLoad )
         {
            rc = _postLoad() ;
            PD_RC_CHECK( rc, PDERROR, "Load schema info failed, rc: %d", rc ) ;
         }
      }

      rc = writeBitmap.init() ;
      PD_RC_CHECK( rc, PDERROR, "Init write bitmap failed, rc: %d", rc ) ;

      /// when Primal data don't set write bitmap
      if ( session && session->isBusinessSession() && !cb->isInTransRollback() )
      {
         isPrimalData = TRUE ;
         writeBitmap.setBitmap( _writeColBitmap ) ;
      }

   retry:
      try
      {
         pBaseObj = NULL ;
         BSONObj record( recordData.data() ) ;

         if ( !_forceEncode )
         {
            if ( hasRetry )
            {
               if ( DMS_SCHEMA_ENCODE_ORG == encodeFlag )
               {
                  pBaseObj = &record ;
               }
            }
            else
            {
               BOOLEAN hitName = FALSE ;
               BOOLEAN hitDefault = FALSE ;

               rc = _checkOrgRecord( record, writeBitmap, hitName, hitDefault ) ;
               PD_RC_CHECK( rc, PDERROR, "Check record for encode failed, rc: %d", rc ) ;

               if ( !hitName )
               {
                  encodeFlag = DMS_SCHEMA_ENCODE_ORG ;
                  pBaseObj = &record ;
               }
            }
         }

         if ( !hasRetry )
         {
            estimateSize = record.objsize() + _defaultWriteMaxSize ;
         }

         encodeRecord = cb->getEncodeBuff( estimateSize + DMS_SCHEMA_ENCODE_FILL_SZ ) ;
         if ( !encodeRecord )
         {
            rc = SDB_OOM ;
            PD_LOG( PDERROR, "Get buffer of size %u for encoding record failed, rc: %d",
                    estimateSize, rc ) ;
            goto error ;
         }

         DMS_SCHEMA_SET_ENCODE_FLAG( encodeRecord, encodeFlag ) ;
         DMS_SCHEMA_SET_VERSION( encodeRecord, _schemaVersion ) ;

         rc = encodeBuilder.start( DMS_SCHEMA_ENCODE_DATAPTR( encodeRecord ),
                                   estimateSize, pBaseObj ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to start building record, rc: %d", rc ) ;

         /// when encode by org, don't parse and build
         if ( DMS_SCHEMA_ENCODE_HEX == encodeFlag )
         {
            rc = _parseRecord( encodeBuilder, writeBitmap, record, hasNewCol ) ;
            if ( rc )
            {
               PD_LOG( PDERROR, "Parse record by internal schema failed, rc: %d", rc ) ;
               goto error ;
            }
            else if ( hasNewCol )
            {
               /// need update the internal schema outside and retry
               goto done ;
            }
         }

         /// add write default columns
         if ( writeBitmap.validBitSize() > 0 )
         {
            rc = _appendPrimalColumns( cb, record, writeBitmap, encodeFlag,
                                       encodeBuilder, recordData, memAlloc ) ;
            PD_RC_CHECK( rc, PDERROR, "Append primal columns into record failed, rc: %d", rc ) ;
         }

         rc = encodeBuilder.done( isEmpty ) ;
         PD_RC_CHECK( rc, PDERROR, "Finish building encoded record failed, rc: %d", rc ) ;

         totalSize = encodeBuilder.dataSize() + DMS_SCHEMA_ENCODE_FILL_SZ ;

         // Check once again if the encoded record size exceeds the limit
         if ( totalSize + DMS_RECORD_METADATA_SZ > DMS_RECORD_USER_MAX_SZ )
         {
            rc = SDB_DMS_RECORD_TOO_BIG ;
            goto error ;
         }

         encodeData.setData( (const CHAR *)encodeRecord, totalSize,
                             UTIL_COMPRESSOR_INVALID, TRUE, TRUE ) ;

#ifdef _DEBUG
         rc = _encodeSanityCheck( encodeData, isPrimalData ) ;
         PD_RC_CHECK( rc, PDERROR, "Sanity check encode data failed, rc: %d", rc ) ;
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
      if ( memAlloc )
      {
         cb->releaseBuff( (CHAR*)recordData.data() ) ;
         /// restore
         recordData.setData( pOrgData, orgDataLen ) ;
         memAlloc = FALSE ;
      }
      if ( encodeBuilder.isOutOfBuff() )
      {
         estimateSize <<= 1 ;
         encodeBuilder.reset() ;
         hasRetry = TRUE ;
         goto retry ;
      }
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSINTERNALSCHEMA_DECODERECORD, "_dmsInternalSchema::decodeRecord" )
   INT32 _dmsInternalSchema::decodeRecord( _pmdEDUCB *cb,
                                           const CHAR *data,
                                           UINT32 dataSize,
                                           BSONObj &objRecord,
                                           BOOLEAN getPrimalData )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__DMSINTERNALSCHEMA_DECODERECORD ) ;

      UINT32 decodeSize = 0 ;
      CHAR  *decodeBuffer = NULL ;
      UINT16 columnID = 0 ;
      const CHAR *columnName = NULL ;
      INT32 nameLen = 0 ;
      BOOLEAN colIsDeleted = FALSE ;
      UINT8 encodeFlag = DMS_SCHEMA_GET_ENCODE_FLAG( data ) ;
      UINT32 version = DMS_SCHEMA_GET_VERSION( data ) ;

      dmsThreadSchemaBitmap readBitmap( _schemaContainer.columnNum() ) ;
      utilBSONRawBuilder builder ;
      BOOLEAN isEmpty = FALSE ;
      BOOLEAN hasRetry = FALSE ;

      rc = readBitmap.init() ;
      PD_RC_CHECK( rc, PDERROR, "Init bitmap failed, rc: %d", rc ) ;

      /// check is load
      if ( !_hasLoad )
      {
         ossScopedRWLock lock( &_loadRWMutex, EXCLUSIVE ) ;
         if ( !_hasLoad )
         {
            rc = _postLoad() ;
            PD_RC_CHECK( rc, PDERROR, "Load schema info failed, rc: %d", rc ) ;
         }
      }

      /// copy read bitmap
      if ( !getPrimalData && version != _schemaVersion )
      {
         readBitmap.setBitmap( _readColBitmap ) ;
      }

   retry:
      try
      {
         BSONObj encodedRecord( DMS_SCHEMA_ENCODE_DATAPTR( data ) ) ;

         /// when encode format is ORG, and newest version
         if ( DMS_SCHEMA_ENCODE_ORG == encodeFlag )
         {
            if ( version == _schemaVersion )
            {
               objRecord = encodedRecord ;
            }
            else
            {
               /// rebuild record by the orignal record
               rc = rebuildRecord( cb, encodedRecord, objRecord, getPrimalData ) ;
               PD_RC_CHECK( rc, PDERROR, "Rebuild record failed, rc: %d", rc ) ;
            }
            goto done ;
         }

         // Estimate the decode size. Column ids will be replaced by column names(deleted columns
         // not included), and columns with read default values will be added.
         if ( !hasRetry )
         {
            decodeSize = encodedRecord.objsize() + _totalValidNameSize ;
            if ( !readBitmap.isEmpty() )
            {
               decodeSize += _defaultReadMaxSize ;
            }
         }

         BSONObjIterator itr( encodedRecord ) ;
         decodeBuffer = cb->getDecodeBuff( decodeSize ) ;
         if ( !decodeBuffer )
         {
            rc = SDB_OOM ;
            PD_LOG( PDERROR, "Get buffer of size %u for decoding record failed, rc: %d",
                    decodeSize, rc ) ;
            goto error ;
         }

         rc = builder.start( decodeBuffer, decodeSize ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to start building record, rc: %d", rc ) ;

         while ( itr.more() )
         {
            BSONElement ele = itr.next() ;
            columnID = (UINT16)utilHexStrToInt( ele.fieldName() ) ;

            /// clear read default bitmap
            readBitmap.clearBit( columnID ) ;
            /// get column from schema
            rc = _schemaContainer.getColumnBasicInfo( columnID, &columnName,
                                                      &nameLen, &colIsDeleted ) ;
            if ( rc )
            {
               PD_LOG( PDERROR, "Get column[%s,%u] from schema failed, rc: %d",
                       ele.fieldName(), columnID, rc ) ;
               goto error ;
            }
            else if ( colIsDeleted )
            {
               // Ignore columns which has been marked as delete.
               continue ;
            }

            rc = builder.appendElement( ele.type(), columnName, nameLen,
                                        ele.value(), ele.valuesize() ) ;
            PD_RC_CHECK( rc, PDERROR, "Append column [%s] to record decode buffer failed, rc: %d",
                         columnName, rc ) ;
         }

         /// process default columns
         if ( readBitmap.validBitSize() > 0 )
         {
            // Append columns with default.
            rc = _appendColWithReadDefault( readBitmap, builder ) ;
            PD_RC_CHECK( rc, PDERROR, "Append column with default value to record failed, rc: %d",
                        rc ) ;
         }

         builder.done( isEmpty ) ;
         PD_RC_CHECK( rc, PDERROR, "Finish building encoded record failed, rc: %d", rc ) ;

         /// set return object
         objRecord = BSONObj( decodeBuffer ) ;
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
      if ( builder.isOutOfBuff() )
      {
         decodeSize <<= 1 ;
         builder.reset() ;
         hasRetry = TRUE ;
         goto retry ;
      }
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSINTERNALSCHEMA_REBUILDRECORD, "_dmsInternalSchema::rebuildRecord" )
   INT32 _dmsInternalSchema::rebuildRecord( _pmdEDUCB *cb,
                                            const BSONObj &record,
                                            BSONObj &outRecord,
                                            BOOLEAN getPrimalData )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__DMSINTERNALSCHEMA_REBUILDRECORD ) ;

      UINT32 hitCount = 0 ;
      BOOLEAN isEmpty = FALSE ;
      BOOLEAN hitName = FALSE ;
      BOOLEAN hitDefault = FALSE ;
      utilBSONRawBuilder builder ;
      INT32 buffSize = 0 ;
      CHAR *buff = NULL ;
      UINT16 colID = DMS_SCHEMA_INVALID_COLUMNID ;
      NAME_INFO_MAP_ITR itrInfo ;
      BOOLEAN hasRetry = FALSE ;

      const CHAR *name           = NULL ;
      INT32 nameLen              = 0 ;
      BOOLEAN isDeleted          = FALSE ;
      BOOLEAN hasOrigName        = FALSE ;

      dmsThreadSchemaBitmap readBitmap( _schemaContainer.columnNum() ) ;

      /// check is load
      if ( !_hasLoad )
      {
         ossScopedRWLock lock( &_loadRWMutex, EXCLUSIVE ) ;
         if ( !_hasLoad )
         {
            rc = _postLoad() ;
            PD_RC_CHECK( rc, PDERROR, "Load schema info failed, rc: %d", rc ) ;
         }
      }

      if ( getPrimalData && _decodeWatchNames.empty() )
      {
         outRecord = record ;
         goto done ;
      }

      rc = readBitmap.init() ;
      PD_RC_CHECK( rc, PDERROR, "Init read bitmap failed, rc: %d", rc ) ;

      rc = _checkOrgRecord( record, readBitmap, hitName, hitDefault ) ;
      PD_RC_CHECK( rc, PDERROR, "Check record need rebuild failed, rc: %d", rc ) ;

      if ( !hitName && !hitDefault )
      {
         outRecord = record ;
         goto done ;
      }
      else if ( !hitName )
      {
         /// only default value
         if ( getPrimalData )
         {
            outRecord = record ;
            goto done ;
         }

         buffSize = record.objsize() + _defaultReadMaxSize ;
         buff = cb->getDecodeBuff( buffSize ) ;
         if ( !buff )
         {
            rc = SDB_OOM ;
            PD_LOG( PDERROR, "Allocate buffer of size %d for decoding record failed, rc: %d",
                    buffSize, rc ) ;
            goto error ;
         }

         rc = builder.start( buff, buffSize, &record ) ;
         PD_RC_CHECK( rc, PDERROR, "Start rebuild record by internal schema failed, rc: %d",
                      rc ) ;

         rc = _appendColWithReadDefault( readBitmap, builder ) ;
         PD_RC_CHECK( rc, PDERROR, "Append column with default value to record failed, rc: %d",
                     rc ) ;

         rc = builder.done( isEmpty ) ;
         PD_RC_CHECK( rc, PDERROR, "Finish building encoded record failed, rc: %d", rc ) ;

         outRecord = BSONObj( buff ) ;

         goto done ;
      }

   retry:
      try
      {
         if ( !hasRetry )
         {
            buffSize = record.objsize() ;
            if ( !readBitmap.isEmpty() )
            {
               buffSize += _defaultReadMaxSize ;
            }
         }

         hitCount = 0 ;
         BSONObjIterator itr( record ) ;
         buff = cb->getDecodeBuff( buffSize ) ;
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

         while( itr.more() )
         {
            BSONElement ele = itr.next() ;
            colID = DMS_SCHEMA_INVALID_COLUMNID ;

            /// find in name map
            if ( !_decodeWatchNames.empty() && hitCount < _decodeWatchNames.size() )
            {
               itrInfo = _decodeWatchNames.find( ele.fieldName() ) ;
               if ( itrInfo != _decodeWatchNames.end() )
               {
                  colID = itrInfo->second ;
                  ++hitCount ;

                  rc = _schemaContainer.getColumnBasicInfo( colID, &name, &nameLen, &isDeleted,
                                                            NULL, NULL, NULL, &hasOrigName ) ;
                  PD_RC_CHECK( rc, PDERROR, "Get column[%d] info failed, rc: %d", colID, rc ) ;

                  if ( isDeleted )
                  {
                     /// ignore
                     continue ;
                  }
                  else if ( hasOrigName )
                  {
                     rc = builder.appendElement( ele.type(), name, nameLen,
                                                 ele.value(), ele.valuesize() ) ;
                     PD_RC_CHECK( rc, PDERROR, "Append element when rebuilding record failed, "
                                  "rc: %d", rc ) ;

                     continue ;
                  }
               }
            }

            /// not found
            colID = _schemaHash.getColumnIDByName( ele.fieldName() ) ;
            if ( DMS_SCHEMA_INVALID_COLUMNID != colID )
            {
               readBitmap.clearBit( colID ) ;
            }

            rc = builder.appendElement( ele.type(), ele.fieldName(),
                                        ossStrlen( ele.fieldName() ),
                                        ele.value(), ele.valuesize() ) ;
            PD_RC_CHECK( rc, PDERROR, "Append element when rebuilding record failed, rc: %d",
                         rc ) ;
         }

         if ( !getPrimalData )
         {
            rc = _appendColWithReadDefault( readBitmap, builder ) ;
            PD_RC_CHECK( rc, PDERROR, "Append column with default value to record failed, rc: %d",
                        rc ) ;
         }

         rc = builder.done( isEmpty ) ;
         PD_RC_CHECK( rc, PDERROR, "Finish building encoded record failed, rc: %d", rc ) ;

         outRecord = BSONObj( buff ) ;
      }
      catch( std::exception &e )
      {
         rc = ossException2RC( &e ) ;
         PD_LOG( PDERROR, "Occur exception: %s", e.what() ) ;
         goto error ;
      }

   done:
      PD_TRACE_EXITRC( SDB__DMSINTERNALSCHEMA_REBUILDRECORD, rc ) ;
      return rc ;
   error:
      if ( builder.isOutOfBuff() )
      {
         buffSize <<= 1 ;
         builder.reset() ;
         hasRetry = TRUE ;
         goto retry ;
      }
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

   ossRWMutex* _dmsInternalSchema::getRWMutex()
   {
      return &_loadRWMutex ;
   }

   INT32 _dmsInternalSchema::_init( const dmsSchemaExtent *schemaExtent,
                                    UINT32 schemaExtentSize,
                                    const dmsSchemaHashExtent *hashExtent,
                                    UINT32 hashExtentSize,
                                    UINT16 mbID,
                                    BOOLEAN isReload)
   {
      INT32 rc = SDB_OK ;

      SDB_ASSERT( schemaExtent && (schemaExtentSize > 0), "Schema extent is invalid" ) ;
      SDB_ASSERT( hashExtent && (hashExtentSize > 0), "Schema hash extent is invalid" ) ;

      rc = _schemaContainer.init( schemaExtent, schemaExtentSize, mbID ) ;
      PD_RC_CHECK( rc, PDERROR, "Init internal schema extent failed, rc: %d", rc ) ;

      rc = _schemaHash.init( &_schemaContainer, hashExtent, hashExtentSize, mbID ) ;
      PD_RC_CHECK( rc, PDERROR, "Init internal schema hash extent failed, rc: %d", rc ) ;

      /// init version
      _schemaVersion = schemaExtent->_schemaVersion ;
      _schemaInnerVersion = schemaExtent->_schemaInnerVersion ;

      rc = _postLoad() ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Load internal schema failed, rc: %d", rc ) ;

         if ( !isReload )
         {
            goto error ;
         }
         /// ignore, trigger postLoad() by encode()/decode()
         rc = SDB_OK ;
      }

      _enabled = TRUE ;

   done:
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSINTERNALSCHEMA__PARSERECORD, "_dmsInternalSchema::_parseRecord" )
   INT32 _dmsInternalSchema::_parseRecord( utilBSONRawBuilder &encodeBuilder,
                                           _utilBitmapBase &writeBitmap,
                                           const BSONObj &record,
                                           BOOLEAN &hasNewCol )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__DMSINTERNALSCHEMA__PARSERECORD ) ;

      UINT16 columnID = DMS_SCHEMA_INVALID_COLUMNID ;
      CHAR columnName[ DMS_SCHEMA_COLID_STR_MAX_SIZE + 1 ] = { 0 } ;
      INT32 colNameLength = 0 ;

      try
      {
         BSONObjIterator itr( record ) ;
         while ( itr.more() )
         {
            BSONElement ele = itr.next() ;
            columnID = _schemaHash.getColumnIDByName( ele.fieldName() ) ;
            if ( DMS_SCHEMA_INVALID_COLUMNID == columnID )
            {
               hasNewCol = TRUE ;
               goto done ;
            }

            colNameLength = utilIntToLowerHexStr( (INT32)columnID, columnName,
                                                  sizeof( columnName ) ) ;
            if ( colNameLength < 0 )
            {
               PD_LOG( PDERROR, "Convert column[%u] to field name failed, rc: %d",
                       columnID, rc ) ;
               goto error ;
            }

            rc = encodeBuilder.appendElement( ele.type(), columnName, colNameLength,
                                              ele.value(), ele.valuesize() ) ;
            PD_RC_CHECK( rc, PDERROR, "Add column [%s] to encoded record failed, rc: %d",
                         ele.fieldName(), rc ) ;

            writeBitmap.clearBit( columnID ) ;
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

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSINTERNALSCHEMA__APPENDPRIMALCOLUMNS, "_dmsInternalSchema::_appendPrimalColumns" )
   INT32 _dmsInternalSchema::_appendPrimalColumns( _pmdEDUCB *cb,
                                                   const BSONObj& originalRecord,
                                                   const _utilBitmapBase &writeBitmap,
                                                   UINT8 encodeFlag,
                                                   utilBSONRawBuilder &encodeBuilder,
                                                   dmsRecordData &recordData,
                                                   BOOLEAN &memAlloc )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__DMSINTERNALSCHEMA__APPENDPRIMALCOLUMNS ) ;

      const CHAR *name = NULL ;
      INT32 nameLen = 0 ;
      BSONType type ;
      const CHAR *value = NULL ;
      INT32 valueLen = 0 ;
      INT32 setBitPos = 0 ;

      utilBSONRawBuilder builder ;
      BOOLEAN isEmpty = FALSE ;

      CHAR *buff = NULL ;
      INT32 buffSize = originalRecord.objsize() + _defaultWriteMaxSize ;
      CHAR columnName[ DMS_SCHEMA_COLID_STR_MAX_SIZE + 1 ] = { 0 } ;
      INT32 colNameLength = 0 ;

      buff = (CHAR *)cb->getBuffer( buffSize ) ;
      if ( !buff )
      {
         rc = SDB_OOM ;
         PD_LOG( PDERROR, "Allocate memory of size[%u] failed, rc: %d", buffSize, rc ) ;
         goto error ;
      }
      memAlloc = TRUE ;

      rc = builder.start( buff, buffSize, &originalRecord ) ;
      PD_RC_CHECK( rc, PDERROR, "Start rebuild record by internal schema failed, rc: %d",
                   rc ) ;

      while( TRUE )
      {
         setBitPos = writeBitmap.nextSetBitPos( setBitPos ) ;
         if ( -1 == setBitPos )
         {
            break ;
         }

         /// get write default
         if ( _schemaContainer.hasWriteDefault( (UINT16)setBitPos ) )
         {
            rc = _schemaContainer.getColWriteDefault( (UINT16)setBitPos, name, nameLen,
                                                      type, value, valueLen ) ;
            PD_RC_CHECK( rc, PDERROR, "Get write default value of column %s failed, rc: %d",
                         name, rc ) ;
         }
         else if ( _schemaContainer.hasReadDefault( (UINT16)setBitPos ) )
         {
            // No write value, it should be a column in index with read default.
            rc = _schemaContainer.getColReadDefault( (UINT16)setBitPos, name, nameLen,
                                                     type, value, valueLen ) ;
            PD_RC_CHECK( rc, PDERROR, "Get read default value of column %s failed, rc: %d",
                         name, rc ) ;
         }
         else
         {
            SDB_ASSERT( FALSE, "Should have read or write default value" ) ;
            rc = SDB_SYS ;
            PD_LOG( PDERROR, "Get primal columns[%u] for record failed, rc: %d",
                    setBitPos, rc ) ;
            goto error ;
         }

         if ( DMS_SCHEMA_ENCODE_HEX == encodeFlag )
         {
            /// build hex name
            colNameLength = utilIntToLowerHexStr( setBitPos, columnName, sizeof( columnName ) ) ;
            if ( colNameLength < 0 )
            {
               PD_LOG( PDERROR, "Convert column[%u] to field name failed, rc: %d",
                       setBitPos, rc ) ;
               goto error ;
            }

            /// add to encode builder by hex name
            rc = encodeBuilder.appendElement( type, columnName, colNameLength,
                                              value, valueLen ) ;
            PD_RC_CHECK( rc, PDERROR, "Add column[%s] to encoded record failed, rc: %d",
                         name, rc ) ;
         }
         else
         {
            /// add to encode builder by org name
            rc = encodeBuilder.appendElement( type, name, nameLen,
                                              value, valueLen ) ;
            PD_RC_CHECK( rc, PDERROR, "Add column[%s] to encoded record failed, rc: %d",
                         name, rc ) ;
         }

         /// add to orignal builder
         rc = builder.appendElement( type, name, nameLen, value, valueLen ) ;
         PD_RC_CHECK( rc, PDERROR, "Append info for column[%s] into record failed, rc: %d",
                      name, rc ) ;
      }

      rc = builder.done( isEmpty ) ;
      PD_RC_CHECK( rc, PDERROR, "Build column info record failed, rc: %d", rc ) ;

      /// check size
      if ( *(INT32 *)buff + DMS_RECORD_METADATA_SZ > DMS_RECORD_USER_MAX_SZ )
      {
         PD_LOG( PDERROR, "Object size[%d] is more than [%d]", *(INT32 *)buff,
                 DMS_RECORD_USER_MAX_SZ ) ;
         rc = SDB_DMS_RECORD_TOO_BIG ;
         goto error ;
      }

      recordData.setData( buff, *(INT32 *)buff ) ;

   done:
      PD_TRACE_EXITRC( SDB__DMSINTERNALSCHEMA__APPENDPRIMALCOLUMNS, rc ) ;
      return rc ;
   error:
      if ( buff )
      {
         cb->releaseBuff( buff ) ;
         memAlloc = FALSE ;
      }
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSINTERNALSCHEMA__CHECKORGRECORD, "_dmsInternalSchema::_checkOrgRecord" )
   INT32 _dmsInternalSchema::_checkOrgRecord( const BSONObj &record,
                                              _utilBitmapBase &colBitmap,
                                              BOOLEAN &hitName,
                                              BOOLEAN &hitDefault )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__DMSINTERNALSCHEMA__CHECKORGRECORD ) ;

      NAME_INFO_MAP_ITR itrInfo ;
      UINT16 colID = DMS_SCHEMA_INVALID_COLUMNID ;

      hitName = FALSE ;
      hitDefault = FALSE ;

      try
      {
         BSONObjIterator itr( record ) ;
         while( itr.more() ) ;
         {
            BSONElement ele = itr.next() ;

            if ( !_decodeWatchNames.empty() )
            {
               itrInfo = _decodeWatchNames.find( ele.fieldName() ) ;
               if ( itrInfo != _decodeWatchNames.end() )
               {
                  hitName = TRUE ;
                  goto done ;
               }
            }
            else if ( colBitmap.isEmpty() )
            {
               goto done ;
            }

            colID = _schemaHash.getColumnIDByName( ele.fieldName() ) ;
            if ( DMS_SCHEMA_INVALID_COLUMNID != colID )
            {
               colBitmap.clearBit( colID ) ;
            }
         }
      }
      catch( std::exception &e )
      {
         PD_LOG( PDERROR, "Occur exception: %s", e.what() ) ;
         rc = ossException2RC( &e ) ;
         goto error ;
      }

      if ( !colBitmap.isEmpty() )
      {
         hitDefault = TRUE ;
      }

   done:
      PD_TRACE_EXITRC( SDB__DMSINTERNALSCHEMA__CHECKORGRECORD, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSINTERNALSCHEMA__APPENDCOLWITHREADDFT, "_dmsInternalSchema::_appendColWithReadDefault" )
   INT32 _dmsInternalSchema::_appendColWithReadDefault( const _utilBitmapBase &readBitmap,
                                                        utilBSONRawBuilder &builder )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__DMSINTERNALSCHEMA__APPENDCOLWITHREADDFT ) ;

      const CHAR *name = NULL ;
      INT32 nameLen = 0 ;
      BSONType type ;
      const CHAR *value = NULL ;
      INT32 valueLen = 0 ;
      INT32 nextSetPos = 0 ;

      while ( TRUE )
      {
         nextSetPos = readBitmap.nextSetBitPos( ( UINT32 )nextSetPos ) ;
         if ( -1 == nextSetPos )
         {
            break ;
         }

         /// get read default value infor
         rc = _schemaContainer.getColReadDefault( (UINT16)nextSetPos, name, nameLen,
                                                  type, value, valueLen ) ;
         PD_RC_CHECK( rc, PDERROR, "Get column read default value failed, rc: %d", rc ) ;

         /// append default
         rc = builder.appendElement( type, name, nameLen, value, valueLen ) ;
         PD_RC_CHECK( rc, PDERROR, "Append default column[%s] failed, rc: %d",
                      name, rc ) ;
      }

   done:
      PD_TRACE_EXITRC( SDB__DMSINTERNALSCHEMA__APPENDCOLWITHREADDFT, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   INT32 _dmsInternalSchema::_postLoad()
   {
      INT32 rc                   = SDB_OK ;
      const CHAR *name           = NULL ;
      INT32 nameLen              = 0 ;
      BOOLEAN isDeleted          = FALSE ;
      BOOLEAN hasReadDefault     = FALSE ;
      BOOLEAN hasWriteDefault    = FALSE ;
      BOOLEAN isIndexColumn      = FALSE ;
      BOOLEAN hasOrigName        = FALSE ;
      UINT32 colReadEleSize      = 0 ;
      UINT32 colWriteEleSize     = 0 ;

      /// clear bitmap information
      _clearBitmapInfo() ;

      rc = _readColBitmap.resize( _schemaContainer.columnNum() ) ;
      PD_RC_CHECK( rc, PDERROR, "Resize read column bitmap failed, rc: %d", rc ) ;
      rc = _writeColBitmap.resize( _schemaContainer.columnNum() ) ;
      PD_RC_CHECK( rc, PDERROR, "Resize write column bitmap failed, rc: %d", rc ) ;

      for ( UINT16 colID = 0 ; colID < _schemaContainer.columnNum() ; ++colID )
      {
         rc = _schemaContainer.getColumnBasicInfo( colID, &name, &nameLen, &isDeleted,
                                                   &hasReadDefault, &hasWriteDefault,
                                                   &isIndexColumn, &hasOrigName ) ;
         PD_RC_CHECK( rc, PDERROR, "Get column info from internal schema failed, rc: %d", rc ) ;

         if ( !isDeleted )
         {
            if ( hasReadDefault )
            {
               _readColBitmap.setBit( colID ) ;

               rc = _schemaContainer.getReadEleSize( colID, colReadEleSize ) ;
               PD_RC_CHECK( rc, PDERROR, "Get column element size failed, rc: %d", rc ) ;

               _defaultReadMaxSize += colReadEleSize ;
            }
            if ( hasWriteDefault )
            {
               _writeColBitmap.setBit( colID ) ;

               rc = _schemaContainer.getWriteEleSize( colID, colWriteEleSize ) ;
               PD_RC_CHECK( rc, PDERROR, "Get column element size failed, rc: %d", rc ) ;

               _defaultWriteMaxSize += colWriteEleSize ;
            }

            if ( isIndexColumn && hasReadDefault )
            {
               /// add to write default
               if ( !_writeColBitmap.testBit( colID ) )
               {
                  _writeColBitmap.setBit( colID ) ;
                  _defaultWriteMaxSize += colReadEleSize ;
               }
            }

            _totalValidNameSize += nameLen ;
         }

         try
         {
            if ( hasOrigName )
            {
               const CHAR *origName = _schemaContainer.getOrigName( colID ) ;
               _decodeWatchNames[ origName ] = colID ;
            }
            else if ( isDeleted )
            {
               _decodeWatchNames[ name ] = colID ;
            }
         }
         catch( std::exception &e )
         {
            rc = ossException2RC( &e ) ;
            PD_LOG( PDERROR, "Occur exception: %s", e.what() ) ;
            goto error ;
         }
      }

   #ifdef _DEBUG
      _logSchemaInfo() ;
   #endif /* _DEBUG */

      /// set has load
      _hasLoad = TRUE ;

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

   INT32 _dmsInternalSchema::_encodeSanityCheck( const dmsRecordData &encodedData,
                                                 BOOLEAN isPrimalData )
   {
      INT32 rc = SDB_OK ;
      UINT8 encodeFlag = DMS_SCHEMA_GET_ENCODE_FLAG( encodedData.data() ) ;
      UINT32 version = DMS_SCHEMA_GET_ENCODE_FLAG( encodedData.data() ) ;
      INT32 colID = 0 ;
      BOOLEAN isDeleted = FALSE ;
      NAME_INFO_MAP_ITR itrInfo ;

      dmsThreadSchemaBitmap colBitmap( _schemaContainer.columnNum() ) ;
      dmsThreadSchemaBitmap writeBitmap( _writeColBitmap.getSize() ) ;

      rc = colBitmap.init() ;
      PD_RC_CHECK( rc, PDERROR, "Init column bitmap failed, rc: %d", rc ) ;

      rc = writeBitmap.init() ;
      PD_RC_CHECK( rc, PDERROR, "Init write bitmap failed, rc: %d", rc ) ;

      if ( !isPrimalData )
      {
         writeBitmap.setBitmap( _writeColBitmap ) ;
      }

      /// check version
      if ( version != _schemaVersion )
      {
         SDB_ASSERT( FALSE, "The encoded version is invalid" ) ;
         PD_LOG( PDERROR, "The encoded version[%d] is not the same with current[%d]",
                 version, _schemaVersion ) ;
         rc = SDB_SYS ;
         goto error ;
      }

      /// check flag
      if ( encodeFlag != DMS_SCHEMA_ENCODE_HEX && encodeFlag != DMS_SCHEMA_ENCODE_ORG )
      {
         SDB_ASSERT( FALSE, "The encoded flag is invalid" ) ;
         PD_LOG( PDERROR, "The encode flag[%d] is invalid", encodeFlag ) ;
         rc = SDB_SYS ;
         goto error ;
      }

      try
      {
         const BSONObj record( DMS_SCHEMA_ENCODE_DATAPTR( encodedData.data() ) ) ;
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
            BSONElement ele = itr.next() ;

            if ( DMS_SCHEMA_ENCODE_HEX == encodeFlag )
            {
               colID = utilHexStrToInt( ele.fieldName() ) ;

               if ( colID < 0 )
               {
                  SDB_ASSERT( FALSE, "Invalid field name" ) ;
                  PD_LOG( PDERROR, "Invalid field name[s]", ele.fieldName() ) ;
                  rc = SDB_SYS ;
                  goto error ;
               }

               if ( colBitmap.testBit( colID ) )
               {
                  rc = SDB_SYS ;
                  SDB_ASSERT( FALSE, "Duplicated column ID" ) ;
                  PD_LOG( PDERROR, "Duplicate column[%d,%s]", colID, ele.fieldName() ) ;
                  goto error ;
               }

               colBitmap.setBit( colID ) ;
               writeBitmap.clearBit( colID ) ;

               rc = _schemaContainer.getColumnBasicInfo( colID, NULL, NULL, &isDeleted ) ;
               if ( rc )
               {
                  rc = SDB_SYS ;
                  PD_LOG( PDERROR, "Get column info by column ID %u failed, rc: %d", colID, rc ) ;
                  SDB_ASSERT( FALSE, "Get column info by column ID failed" ) ;
                  goto error ;
               }

               if ( isDeleted )
               {
                  rc = SDB_SYS ;
                  PD_LOG( PDERROR, "Should not include deleted column when encoding" ) ;
                  SDB_ASSERT( FALSE, "Should not include deleted column when encoding" ) ;
                  goto error ;
               }
            }
            else if ( DMS_SCHEMA_ENCODE_ORG == encodeFlag )
            {
               /// can't find in namemap
               itrInfo = _decodeWatchNames.find( ele.fieldName() ) ;
               if ( itrInfo != _decodeWatchNames.end() )
               {
                  rc = SDB_SYS ;
                  SDB_ASSERT( FALSE, "Field should not in namemap when use ORG encode" ) ;
                  PD_LOG( PDERROR, "Field[%s] should not in namemap when use ORG encode",
                          ele.fieldName() ) ;
                  goto error ;
               }

               colID = _schemaHash.getColumnIDByName( ele.fieldName() ) ;
               if ( DMS_SCHEMA_INVALID_COLUMNID != (UINT16)colID )
               {
                  if ( colBitmap.testBit( colID ) )
                  {
                     rc = SDB_SYS ;
                     SDB_ASSERT( FALSE, "Duplicated column ID" ) ;
                     PD_LOG( PDERROR, "Duplicate column[%d,%s]", colID, ele.fieldName() ) ;
                     goto error ;
                  }

                  colBitmap.setBit( colID ) ;
                  writeBitmap.clearBit( colID ) ;
               }
               else
               {
                  PD_LOG( PDINFO, "Get field[%s] column ID failed", ele.fieldName() ) ;
                  /// ignore, when use ORG encode, the new field don't add to column by data
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

}

