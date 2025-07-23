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
      Encode object format:
      |  ...          | 1 Byte |    3 Byte    |
      |  Data         |  Flag  |    Version   |

      Data has two format:
      1. Orignal format, ex: { a:1, b:1, c:1 }
      2. Hex format,     ex: { 0:1, 1:1, 2:1 }
   */

   /*
      Flag define

      |   4 bit   |    4 bit   |
      |   Attr    |    Type    |
   */
   #define DMS_SCHEMA_ENCODE_ORG             0x01
   #define DMS_SCHEMA_ENCODE_HEX             0x02

   #define DMS_SCHEMA_ATTR_VER_STRICT        0x80

   #define DMS_SCHEMA_ENCODE_POSIXPTR(pData)       (((CHAR*)(pData)) + *(UINT32*)(pData) )
   #define DMS_SCHEMA_ENCODE_DATAPTR(pData)        ((CHAR*)(pData))

   #define DMS_SCHEMA_GET_ENCODE_FLAG(pData)       \
      ((*(UINT32*)DMS_SCHEMA_ENCODE_POSIXPTR(pData)) >> 24)

   #define DMS_SCHEMA_GET_ENCODE_TYPE(pData)       \
      (((UINT8)DMS_SCHEMA_GET_ENCODE_FLAG(pData)) & 0x0F )

   #define DMS_SCHEMA_GET_ENCODE_ATTR(pData)       \
      (((UINT8)DMS_SCHEMA_GET_ENCODE_FLAG(pData)) & 0xF0 )

   #define DMS_SCHEMA_SET_ENCODE_FLAG(pData,flag)  \
      ((*(UINT32*)DMS_SCHEMA_ENCODE_POSIXPTR(pData)) = \
      (((*(UINT32*)DMS_SCHEMA_ENCODE_POSIXPTR(pData))&0x00FFFFFF) |\
      (((UINT32)((UINT8)flag)) << 24 )))

   #define DMS_SCHEMA_SET_ENCODE_FLAG2(pData,attr,type)  \
      DMS_SCHEMA_SET_ENCODE_FLAG(pData, (((UINT8)(attr))&0xF0)|(((UINT8)(type))&0x0F) )

   #define DMS_SCHEMA_GET_VERSION(pData)           \
      ((*(UINT32*)DMS_SCHEMA_ENCODE_POSIXPTR(pData)) & 0x00FFFFFF)

   #define DMS_SCHEMA_SET_VERSION(pData,version)   \
      ((*(UINT32*)DMS_SCHEMA_ENCODE_POSIXPTR(pData)) = \
      (((*(UINT32*)DMS_SCHEMA_ENCODE_POSIXPTR(pData))&0xFF000000) |\
      (((UINT32)version)&0x00FFFFFF )))

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

   INT32 _dmsSchemaContainer::getColDefaultInitVersion( UINT16 columnID,
                                                        UINT32 &initVersion ) const
   {
      INT32 rc = SDB_OK ;
      const dmsSchemaColSlot *slot = _getColSlot( columnID ) ;
      if ( !slot )
      {
         rc = SDB_SYS ;
         PD_LOG( PDERROR, "Get schema column info slot of column ID [%u] failed, rc: %d",
                 columnID, rc ) ;
         goto error ;
      }
      initVersion = slot->getDefaultInitVersion() ;

   done:
      return rc ;
   error:
      goto done ;
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
                                                  BOOLEAN *hasOrigName,
                                                  BOOLEAN *isHidden,
                                                  UINT32 *defaultValInitVersion ) const
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
      if ( isHidden )
      {
         *isHidden = ( attr & DMS_SCHEMA_COL_HIDDEN ) ? TRUE : FALSE ;
      }
      if ( defaultValInitVersion )
      {
         const dmsSchemaColSlot *slot = _getColSlot( columnID ) ;
         if ( !slot )
         {
            rc = SDB_SYS ;
            PD_LOG( PDERROR, "Get schema column info slot of column ID [%u] failed, rc: %d",
                    columnID, rc ) ;
            goto error ;
         }
         *defaultValInitVersion = slot->getDefaultInitVersion() ;
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

         // 1 byte for type, 1 byte for terminating null of name.
         size = 1 + nameLen + 1 + valueLen ;
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

         // 1 byte for type, 1 byte for terminating null of name.
         size = 1 + nameLen + 1 + valueLen ;
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
            const dmsSchemaColSlot *slot = _getColSlot( columnID ) ;
            if ( !slot )
            {
               rc = SDB_INVALIDARG ;
               PD_LOG( PDERROR, "Get column slot failed" ) ;
               goto error ;
            }
            builder.append( FIELD_NAME_READDEFAULT_INIT_VERSION, slot->getDefaultInitVersion() ) ;
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
            if ( isColumnHidden( columnID ) )
            {
               builder.appendBool( FIELD_NAME_HIDDEN, TRUE ) ;
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

      _bucketNum = extent->_bucketNum ;
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

   #define DMS_SCHEMA_CONTEXT_ITEM_SZ           ( 32 )
   #define DMS_SCHEMA_KICKOUT_THRESHOLD         ( DMS_SCHEMA_CONTEXT_ITEM_SZ * 20 )

   /*
      _dmsSchemaContext implement
   */
   _dmsSchemaContext::_dmsSchemaContext()
   :_hwUseID(0), _queryBitmap( 0 )
   {
      _isWirld = TRUE ;
      _hasSetQuery = FALSE ;
      _curSchemaVersion = DMS_SCHEMA_INVALID_VERSION ;
   }

   _dmsSchemaContext::~_dmsSchemaContext()
   {
   }

   BOOLEAN _dmsSchemaContext::validateCheck( UINT32 curSchemaVersion )
   {
      if ( _curSchemaVersion != curSchemaVersion )
      {
         _clearBitInfo() ;
         _curSchemaVersion = curSchemaVersion ;
         return FALSE ;
      }
      return TRUE ;
   }

   void _dmsSchemaContext::_clearBitInfo()
   {
      _mapHisItem.clear() ;
      _queryBitmap.resetBitmap() ;
      _isWirld = TRUE ;
      _hasSetQuery = FALSE ;
      _hwUseID = 0 ;
   }

   BOOLEAN _dmsSchemaContext::_kickOutHisItem()
   {
      MAP_CTX_ITEM::iterator it = _mapHisItem.begin() ;
      while ( it != _mapHisItem.end() )
      {
         const dmsSchemaContextItem &item = it->second ;
         if ( item._useID + DMS_SCHEMA_KICKOUT_THRESHOLD < _hwUseID )
         {
            _mapHisItem.erase( it ) ;
            return TRUE ;
         }
      }
      return FALSE ;
   }

   void _dmsSchemaContext::prune( UINT32 recordVersion,
                                  UINT8 recordAttr,
                                  _utilBitmapBase &readBitmap,
                                  BOOLEAN &hitName,
                                  BOOLEAN &found )
   {
      MAP_CTX_ITEM::iterator it ;

      found = FALSE ;

      if ( recordVersion != DMS_SCHEMA_INVALID_VERSION &&
           recordVersion != _curSchemaVersion &&
           OSS_BIT_TEST( recordAttr, DMS_SCHEMA_ATTR_VER_STRICT ) )
      {
         ++_hwUseID ;

         it = _mapHisItem.find( recordVersion ) ;
         if ( it != _mapHisItem.end() )
         {
            dmsSchemaContextItem &tmpItem = it->second ;
            tmpItem._useID = _hwUseID ;

            hitName = tmpItem._hitName ;
            if ( !readBitmap.isEmpty() )
            {
               readBitmap.setBitmap( tmpItem._readBitmap ) ;
            }

            found = TRUE ;
         }
      }

      /// when has query bitmap
      if ( _hasSetQuery && !_isWirld && !readBitmap.isEmpty() && !found )
      {
         INT32 pos = 0 ;
         while( !readBitmap.isEmpty() )
         {
            pos = readBitmap.nextSetBitPos( pos ) ;
            if ( -1 == pos )
            {
               break ;
            }
            else if ( !_queryBitmap.testBit( pos ) )
            {
               readBitmap.clearBit( pos ) ;
            }
            ++pos ;
         }
      }
   }

   void _dmsSchemaContext::pushItem( UINT32 recordVersion,
                                     UINT8 recordAttr,
                                     const _utilBitmapBase &readBitmap,
                                     BOOLEAN hitName )
   {
      if ( recordVersion != DMS_SCHEMA_INVALID_VERSION &&
           recordVersion != _curSchemaVersion &&
           OSS_BIT_TEST( recordAttr, DMS_SCHEMA_ATTR_VER_STRICT ) &&
           ( _mapHisItem.size() <= DMS_SCHEMA_CONTEXT_ITEM_SZ ||
             _kickOutHisItem() ) )
      {
         try
         {
            dmsSchemaContextItem &tmpItem = _mapHisItem[ recordVersion ] ;
            if ( tmpItem._readBitmap.getSize() < readBitmap.getSize() )
            {
               if ( SDB_OK != tmpItem._readBitmap.resize( readBitmap.getSize() ) )
               {
                  goto done ;
               }
            }
            tmpItem._readBitmap.setBitmap( readBitmap ) ;
            tmpItem._hitName = hitName ;
            tmpItem._useID = ++_hwUseID ;
         }
         catch( std::exception & )
         {
         }
      }

   done:
      return ;
   }

   void _dmsSchemaContext::pushQueryBitmap( const _utilBitmapBase &readBitmap )
   {
      if ( _queryBitmap.getSize() < readBitmap.getSize() )
      {
         if ( SDB_OK == _queryBitmap.resize( readBitmap.getSize() ) )
         {
            _queryBitmap.setBitmap( readBitmap ) ;
            _hasSetQuery = TRUE ;
            _isWirld = FALSE ;
         }
      }
   }

   void _dmsSchemaContext::setQueryWirld()
   {
      _hasSetQuery = TRUE ;
      _isWirld = TRUE ;
   }

   /*
      _dmsDecodeWatchValue implement
   */
   _dmsDecodeWatchValue::_dmsDecodeWatchValue( UINT16 colID, BOOLEAN isDeleted )
   {
      _colID = colID ;
      _isDeleted = isDeleted ;
      _pName = NULL ;
      _nameLen = 0 ;
      _pOrgName = NULL ;
      _orgNameLen = 0 ;
   }

   _dmsDecodeWatchValue::~_dmsDecodeWatchValue()
   {
      if ( _pName )
      {
         SDB_THREAD_FREE( _pName ) ;
         _pName = NULL ;
      }
      _nameLen = 0 ;
      if ( _pOrgName )
      {
         SDB_THREAD_FREE( _pOrgName ) ;
         _pOrgName = NULL ;
      }
      _orgNameLen = 0 ;
   }

   INT32 _dmsDecodeWatchValue::setName( const CHAR *pName, INT32 nameLen,
                                        const CHAR *pOrgName, INT32 orgNameLen )
   {
      INT32 rc = SDB_OK ;

      if ( !pName || 0 == nameLen )
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      _pName = ( CHAR* )SDB_THREAD_ALLOC( nameLen + 1 ) ;
      if ( !_pName )
      {
         rc = SDB_OOM ;
         PD_LOG( PDERROR, "Allocate memory[%u] failed", nameLen ) ;
         goto error ;
      }

      /// copy
      ossMemcpy( _pName, pName, nameLen ) ;
      _pName[ nameLen ] = 0 ;
      _nameLen = nameLen ;

      if ( pOrgName && orgNameLen > 0 )
      {
         _pOrgName = ( CHAR* )SDB_THREAD_ALLOC( orgNameLen + 1 ) ;
         if ( !_pOrgName )
         {
            rc = SDB_OOM ;
            PD_LOG( PDERROR, "Allocate memory[%u] failed", orgNameLen ) ;
            goto error ;
         }

         /// copy
         ossMemcpy( _pOrgName, pOrgName, orgNameLen ) ;
         _pOrgName[ orgNameLen ] = 0 ;
         _orgNameLen = orgNameLen ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   /*
      _dmsInternalSchema implement
   */
   _dmsInternalSchema::_dmsInternalSchema()
   : _enabled( FALSE ),
     _forceEncode( FALSE ),
     _schemaVersion( DMS_SCHEMA_INVALID_VERSION ),
     _schemaInnerVersion( DMS_SCHEMA_INVALID_VERSION ),
     _colBitmap( 0 ),
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
      _reset() ;
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

      _reset() ;

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
      ossScopedRWLock lock( &_loadRWMutex, EXCLUSIVE ) ;
      _reset() ;
      _enabled = FALSE ;
   }

   void _dmsInternalSchema::_reset()
   {
      _schemaContainer.reset() ;
      _schemaHash.reset() ;

      _clearBitmapInfo() ;

      // _enabled = FALSE ; Don't set enable, because call enabled() when in reloading
      _schemaVersion = DMS_SCHEMA_INVALID_VERSION ;
      _schemaInnerVersion = DMS_SCHEMA_INVALID_VERSION ;

      _hasLoad = FALSE ;
   }

   void _dmsInternalSchema::_clearBitmapInfo()
   {
      _colBitmap.release() ;
      _readColBitmap.release() ;
      _writeColBitmap.release() ;

      NAME_INFO_MAP_ITR itr = _decodeWatchNames.begin() ;
      while( itr != _decodeWatchNames.end() )
      {
         SDB_OSS_DEL itr->second ;
         ++itr ;
      }
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
                                           BOOLEAN &hasNewCol,
                                           BOOLEAN isPrimalData )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__DMSINTERNALSCHEMA_ENCODERECORD ) ;

      UINT32 totalSize = 0 ;
      CHAR *encodeRecord = NULL ;
      INT32 estimateSize = 0 ;
      BOOLEAN isEmpty = FALSE ;
      UINT8 encodeType = DMS_SCHEMA_ENCODE_HEX ;
      UINT8 encodeAttr = 0 ;
      const BSONObj *pBaseObj = NULL ;
      ISession *session = cb->getSession() ;

      const CHAR *pOrgData = recordData.data() ;
      UINT32 orgDataLen    = recordData.len() ;

      dmsThreadSchemaBitmap writeBitmap( _schemaContainer.columnNum() ) ;
      dmsThreadSchemaBitmap colBitmap( _schemaContainer.columnNum() ) ;
      utilBSONRawBuilder encodeBuilder ;

      BOOLEAN hasRetry = FALSE ;

      /// check enbaled
      if ( !_enabled )
      {
         rc = SDB_INTERNAL_SCHEMA_NOT_ENABLED ;
         PD_LOG( PDERROR, "Schema is not enabled" ) ;
         goto error ;
      }
      /// check is load
      else if ( !_hasLoad )
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

      rc = colBitmap.init() ;
      PD_RC_CHECK( rc, PDERROR, "Init column bitmap failed, rc: %d", rc ) ;

      /// when Primal data don't set write bitmap
      if ( !isPrimalData && session && session->isBusinessSession() &&
           !cb->isInTransRollback() )
      {
         writeBitmap.setBitmap( _writeColBitmap ) ;
      }
      else
      {
         isPrimalData = TRUE ;
      }
      colBitmap.setBitmap( _colBitmap ) ;

   retry:
      try
      {
         pBaseObj = NULL ;
         BSONObj record( recordData.data() ) ;

         if ( !_forceEncode && _decodeWatchNames.empty() )
         {
            if ( hasRetry )
            {
               if ( DMS_SCHEMA_ENCODE_ORG == encodeType )
               {
                  pBaseObj = &record ;
               }
            }
            else
            {
               BOOLEAN hitName = FALSE ;
               BOOLEAN hitDefault = FALSE ;

               rc = _checkOrgRecord( record, writeBitmap, hitName, hitDefault,
                                     &hasNewCol, &colBitmap ) ;
               PD_RC_CHECK( rc, PDERROR, "Check record for encode failed, rc: %d", rc ) ;

               if ( hasNewCol )
               {
                  /// need update the internal schema outside and retry
                  goto done ;
               }
               else if ( !hitName )
               {
                  encodeType = DMS_SCHEMA_ENCODE_ORG ;
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

         rc = encodeBuilder.start( DMS_SCHEMA_ENCODE_DATAPTR( encodeRecord ),
                                   estimateSize, pBaseObj ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to start building record, rc: %d", rc ) ;

         /// when encode by org, don't parse and build
         if ( DMS_SCHEMA_ENCODE_HEX == encodeType )
         {
            rc = _parseRecord( encodeBuilder, writeBitmap, colBitmap, record, hasNewCol ) ;
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
            rc = _appendPrimalColumns( cb, record, writeBitmap, colBitmap, encodeType,
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

         if ( colBitmap.isEmpty() )
         {
            /// all columns include in record
            OSS_BIT_SET( encodeAttr, DMS_SCHEMA_ATTR_VER_STRICT ) ;
         }

         DMS_SCHEMA_SET_ENCODE_FLAG2( encodeRecord, encodeAttr, encodeType ) ;
         DMS_SCHEMA_SET_VERSION( encodeRecord, _schemaInnerVersion ) ;

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
                                           dmsSchemaContext *pContext,
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
      BOOLEAN colHasOrgName = FALSE ;
      BOOLEAN colIsHidden = FALSE ;
      UINT8 encodeType = DMS_SCHEMA_GET_ENCODE_TYPE( data ) ;
      UINT8 encodeAttr = DMS_SCHEMA_GET_ENCODE_ATTR( data ) ;
      UINT32 version = DMS_SCHEMA_GET_VERSION( data ) ;

      BOOLEAN hitName = FALSE ;
      BOOLEAN hasFound = FALSE ;

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
      if ( !getPrimalData && version != _schemaInnerVersion )
      {
         readBitmap.setBitmap( _readColBitmap ) ;
      }

      if ( pContext )
      {
         pContext->validateCheck( _schemaInnerVersion ) ;
         _makeSchemaContextQuery( *pContext ) ;
         pContext->prune( version, encodeAttr, readBitmap, hitName, hasFound ) ;
      }

   retry:
      try
      {
         BSONObj encodedRecord( DMS_SCHEMA_ENCODE_DATAPTR( data ) ) ;

         /// when encode format is ORG, and newest version
         if ( DMS_SCHEMA_ENCODE_ORG == encodeType )
         {
            if ( version == _schemaInnerVersion )
            {
               objRecord = encodedRecord ;
            }
            else
            {
               /// rebuild record by the orignal record
               rc = _rebuildRecord( cb, encodedRecord, version, objRecord, readBitmap,
                                    hitName, hasFound, getPrimalData ) ;
               PD_RC_CHECK( rc, PDERROR, "Rebuild record failed, rc: %d", rc ) ;

               /// save to context
               if ( pContext && !getPrimalData && !hasFound )
               {
                  pContext->pushItem( version, encodeAttr, readBitmap, hitName ) ;
               }
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

         retry_colid:
            /// clear read default bitmap
            readBitmap.clearBit( columnID ) ;
            /// get column from schema
            rc = _schemaContainer.getColumnBasicInfo( columnID, &columnName,
                                                      &nameLen, &colIsDeleted,
                                                      NULL, NULL, NULL,
                                                      &colHasOrgName,
                                                      &colIsHidden ) ;
            if ( rc )
            {
               PD_LOG( PDERROR, "Get column[%s,%u] from schema failed, rc: %d",
                       ele.fieldName(), columnID, rc ) ;
               goto error ;
            }
            else if ( colIsHidden )
            {
               columnID = _schemaHash.getColumnIDByName( columnName ) ;
               goto retry_colid ;
            }
            else if ( colIsDeleted )
            {
               // Ignore columns which has been marked as delete.
               continue ;
            }

            if ( !hitName && ( colIsDeleted || colHasOrgName ) )
            {
               hitName = TRUE ;
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
            rc = _appendColWithReadDefault( readBitmap, builder, version ) ;
            PD_RC_CHECK( rc, PDERROR, "Append column with default value to record failed, rc: %d",
                        rc ) ;
         }

         builder.done( isEmpty ) ;
         PD_RC_CHECK( rc, PDERROR, "Finish building encoded record failed, rc: %d", rc ) ;

         /// set return object
         objRecord = BSONObj( decodeBuffer ) ;

         /// save to context
         if ( pContext && !getPrimalData && !hasFound )
         {
            pContext->pushItem( version, encodeAttr, readBitmap, hitName ) ;
         }
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

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSINTERNALSCHEMA__REBUILDRECORD, "_dmsInternalSchema::_rebuildRecord" )
   INT32 _dmsInternalSchema::_rebuildRecord( _pmdEDUCB *cb,
                                             const BSONObj &record,
                                             UINT32 recordVersion,
                                             BSONObj &outRecord,
                                             _utilBitmapBase &readBitmap,
                                             BOOLEAN &hitName,
                                             BOOLEAN hasFound,
                                             BOOLEAN getPrimalData )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__DMSINTERNALSCHEMA__REBUILDRECORD ) ;

      UINT32 hitCount = 0 ;
      BOOLEAN isEmpty = FALSE ;
      BOOLEAN hitDefault = FALSE ;
      utilBSONRawBuilder builder ;
      INT32 buffSize = 0 ;
      CHAR *buff = NULL ;
      UINT16 colID = DMS_SCHEMA_INVALID_COLUMNID ;
      NAME_INFO_MAP_ITR itrInfo ;
      BOOLEAN hasRetry = FALSE ;

      const CHAR *name           = NULL ;
      INT32 nameLen              = 0 ;

      if ( getPrimalData && _decodeWatchNames.empty() )
      {
         outRecord = record ;
         goto done ;
      }

      if ( !hasFound )
      {
         rc = _checkOrgRecord( record, readBitmap, hitName, hitDefault ) ;
         PD_RC_CHECK( rc, PDERROR, "Check record need rebuild failed, rc: %d", rc ) ;
      }
      else if ( !readBitmap.isEmpty() )
      {
         hitDefault = TRUE ;
      }

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

         rc = _appendColWithReadDefault( readBitmap, builder, recordVersion ) ;
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
                  const dmsDecodeWatchValue *pItem = itrInfo->second ;
                  colID = pItem->getColID() ;
                  ++hitCount ;

                  if ( pItem->isDeleted() )
                  {
                     /// ignore
                     continue ;
                  }
                  else if ( pItem->hasOrgName() )
                  {
                     pItem->getName( &name, nameLen ) ;
                     rc = builder.appendElement( ele.type(), name, nameLen,
                                                 ele.value(), ele.valuesize() ) ;
                     PD_RC_CHECK( rc, PDERROR, "Append element when rebuilding record failed, "
                                  "rc: %d", rc ) ;
                     readBitmap.clearBit( colID ) ;
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
            rc = _appendColWithReadDefault( readBitmap, builder, recordVersion ) ;
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
      PD_TRACE_EXITRC( SDB__DMSINTERNALSCHEMA__REBUILDRECORD, rc ) ;
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

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSINTERNALSCHEMA_REBUILDRECORD, "_dmsInternalSchema::rebuildRecord" )
   INT32 _dmsInternalSchema::rebuildRecord( _pmdEDUCB *cb,
                                            const BSONObj &record,
                                            BSONObj &outRecord,
                                            dmsSchemaContext *pContext,
                                            BOOLEAN getPrimalData )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__DMSINTERNALSCHEMA_REBUILDRECORD ) ;

      BOOLEAN hitName = FALSE ;
      BOOLEAN hasFound = FALSE ;

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
      if ( !getPrimalData )
      {
         readBitmap.setBitmap( _readColBitmap ) ;
      }

      if ( pContext )
      {
         pContext->validateCheck( _schemaInnerVersion ) ;
         _makeSchemaContextQuery( *pContext ) ;
         pContext->prune( DMS_SCHEMA_INVALID_VERSION, 0, readBitmap, hitName, hasFound ) ;
      }

      rc = _rebuildRecord( cb, record, DMS_SCHEMA_INVALID_VERSION, outRecord, readBitmap,
                           hitName, hasFound, getPrimalData ) ;
      if ( rc )
      {
         goto error ;
      }

   done:
      PD_TRACE_EXITRC( SDB__DMSINTERNALSCHEMA_REBUILDRECORD, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   INT32 _dmsInternalSchema::dumpSchemaInfo( BSONObj &schema, BOOLEAN includeColumnID )
   {
      ossScopedRWLock lock( &_loadRWMutex, SHARED ) ;
      return _dumpSchemaInfo( schema, includeColumnID ) ;
   }

   /* Each column includes:
    * 1. ID( if includeColumnID is true )
    * 2. Name
    * 3. Read default value(if any)
    * 4. Write default value(if any)
    * 5. If the column is deleted
    * 6. If it's column in index
   */
   INT32 _dmsInternalSchema::_dumpSchemaInfo( BSONObj &schema, BOOLEAN includeColumnID )
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

      ossScopedRWLock lock( &_loadRWMutex, SHARED ) ;

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

      ossScopedRWLock lock( &_loadRWMutex, SHARED ) ;

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
      _enabled = FALSE ;
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSINTERNALSCHEMA__PARSERECORD, "_dmsInternalSchema::_parseRecord" )
   INT32 _dmsInternalSchema::_parseRecord( utilBSONRawBuilder &encodeBuilder,
                                           _utilBitmapBase &writeBitmap,
                                           _utilBitmapBase &allBitmap,
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
            allBitmap.clearBit( columnID ) ;
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
                                                   _utilBitmapBase &allBitmap,
                                                   UINT8 encodeType,
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

   retry:
      rc = cb->allocBuff( buffSize, &buff, NULL ) ;
      if ( rc )
      {
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
         else
         {
            SDB_ASSERT( FALSE, "Should have write default value" ) ;
            rc = SDB_SYS ;
            PD_LOG( PDERROR, "Get primal columns[%u] for record failed, rc: %d",
                    setBitPos, rc ) ;
            goto error ;
         }

         if ( DMS_SCHEMA_ENCODE_HEX == encodeType )
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

         allBitmap.clearBit( setBitPos ) ;

         ++setBitPos ;
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
         buff = NULL ;
      }
      if ( builder.isOutOfBuff() )
      {
         builder.reset() ;
         buffSize <<= 1 ;
         goto retry ;
      }
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSINTERNALSCHEMA__CHECKORGRECORD, "_dmsInternalSchema::_checkOrgRecord" )
   INT32 _dmsInternalSchema::_checkOrgRecord( const BSONObj &record,
                                              _utilBitmapBase &colBitmap,
                                              BOOLEAN &hitName,
                                              BOOLEAN &hitDefault,
                                              BOOLEAN *pHitNew,
                                              _utilBitmapBase *pAllBitmap )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__DMSINTERNALSCHEMA__CHECKORGRECORD ) ;

      NAME_INFO_MAP_ITR itrInfo ;
      UINT16 colID = DMS_SCHEMA_INVALID_COLUMNID ;

      hitName = FALSE ;
      hitDefault = FALSE ;
      if ( pHitNew )
      {
         *pHitNew = FALSE ;
      }

      try
      {
         BSONObjIterator itr( record ) ;
         while( itr.more() )
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
            else if ( colBitmap.isEmpty() && !pHitNew )
            {
               goto done ;
            }

            colID = _schemaHash.getColumnIDByName( ele.fieldName() ) ;
            if ( DMS_SCHEMA_INVALID_COLUMNID != colID )
            {
               colBitmap.clearBit( colID ) ;
               if ( pAllBitmap )
               {
                  pAllBitmap->clearBit( colID ) ;
               }
            }
            else if ( pHitNew )
            {
               *pHitNew = TRUE ;
               goto done ;
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
                                                        utilBSONRawBuilder &builder,
                                                        UINT32 recordVersion )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__DMSINTERNALSCHEMA__APPENDCOLWITHREADDFT ) ;

      const CHAR *name = NULL ;
      INT32 nameLen = 0 ;
      BSONType type ;
      const CHAR *value = NULL ;
      INT32 valueLen = 0 ;
      INT32 nextSetPos = 0 ;
      UINT32 initDefaultVersion = DMS_SCHEMA_INVALID_VERSION ;

      while ( TRUE )
      {
         nextSetPos = readBitmap.nextSetBitPos( ( UINT32 )nextSetPos ) ;
         if ( -1 == nextSetPos )
         {
            break ;
         }

         if ( DMS_SCHEMA_INVALID_VERSION != recordVersion )
         {
            rc = _schemaContainer.getColDefaultInitVersion( (UINT16)nextSetPos,
                                                            initDefaultVersion ) ;
            PD_RC_CHECK( rc, PDERROR, "Get column default version failed, rc: %d", rc ) ;

            if ( DMS_SCHEMA_INVALID_VERSION != initDefaultVersion &&
                 recordVersion >= initDefaultVersion )
            {
               /// don't add default
               ++nextSetPos ;
               continue ;
            }
         }

         /// get read default value infor
         rc = _schemaContainer.getColReadDefault( (UINT16)nextSetPos, name, nameLen,
                                                  type, value, valueLen ) ;
         PD_RC_CHECK( rc, PDERROR, "Get column read default value failed, rc: %d", rc ) ;

         /// append default
         rc = builder.appendElement( type, name, nameLen, value, valueLen ) ;
         PD_RC_CHECK( rc, PDERROR, "Append default column[%s] failed, rc: %d",
                      name, rc ) ;

         ++nextSetPos ;
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
      BOOLEAN hasOrigName        = FALSE ;
      UINT32 colReadEleSize      = 0 ;
      UINT32 colWriteEleSize     = 0 ;
      dmsDecodeWatchValue *pItem = NULL ;

      /// clear bitmap information
      _clearBitmapInfo() ;

      rc = _colBitmap.resize( _schemaContainer.columnNum() ) ;
      PD_RC_CHECK( rc, PDERROR, "Resize column bitmap failed, rc: %d", rc ) ;
      rc = _readColBitmap.resize( _schemaContainer.columnNum() ) ;
      PD_RC_CHECK( rc, PDERROR, "Resize read column bitmap failed, rc: %d", rc ) ;
      rc = _writeColBitmap.resize( _schemaContainer.columnNum() ) ;
      PD_RC_CHECK( rc, PDERROR, "Resize write column bitmap failed, rc: %d", rc ) ;

      for ( UINT16 colID = 0 ; colID < _schemaContainer.columnNum() ; ++colID )
      {
         rc = _schemaContainer.getColumnBasicInfo( colID, &name, &nameLen, &isDeleted,
                                                   &hasReadDefault, &hasWriteDefault,
                                                   NULL, &hasOrigName ) ;
         PD_RC_CHECK( rc, PDERROR, "Get column info from internal schema failed, rc: %d", rc ) ;

         if ( !isDeleted )
         {
            _colBitmap.setBit( colID ) ;

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

            _totalValidNameSize += nameLen ;
         }

         try
         {
            const CHAR *origName = NULL ;
            INT32 orgNameLen = 0 ;
            const CHAR *pKeyName = 0 ;

            if ( hasOrigName )
            {
               origName = _schemaContainer.getOrigName( colID, &orgNameLen ) ;
            }

            if ( isDeleted || hasOrigName )
            {
               pItem = SDB_OSS_NEW dmsDecodeWatchValue( colID, isDeleted ) ;
               if ( !pItem )
               {
                  PD_LOG( PDERROR, "Allocate decode watch value failed" ) ;
                  rc = SDB_OOM ;
                  goto error ;
               }
               /// set name
               rc = pItem->setName( name, nameLen, origName, orgNameLen ) ;
               if ( rc )
               {
                  goto error ;
               }
               /// set key name
               pKeyName = pItem->hasOrgName() ? pItem->getOrgName() : pItem->getName() ;

               /// insert to map
               if ( _decodeWatchNames.insert( NAME_INFO_MAP::value_type( pKeyName,
                                              pItem ) ).second )
               {
                  pItem = NULL ;
               }
               else
               {
                  /// when is exist, release pItem
                  SDB_OSS_DEL pItem ;
                  pItem = NULL ;
               }
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
      if ( pItem )
      {
         SDB_OSS_DEL pItem ;
         pItem = NULL ;
      }
      return rc ;
   error:
      goto done ;
   }

   void _dmsInternalSchema::_logSchemaInfo()
   {
      INT32 rc = SDB_OK ;
      BSONObj schemaInfo ;

      rc = _dumpSchemaInfo( schemaInfo, TRUE ) ;
      if ( rc )
      {
         PD_LOG( PDWARNING, "Dump internal schema info failed, rc: %d", rc )  ;
      }
      else
      {
         PD_LOG( PDEVENT,
                 "Internal schema[Version: %u. Inner version: %u. Column number %d]"OSS_NEWLINE"%s",
                 _schemaVersion, _schemaInnerVersion, _schemaContainer.columnNum(),
                 schemaInfo.jsonString( bson::JS, TRUE ).c_str() ) ;
      }
   }

   INT32 _dmsInternalSchema::_encodeSanityCheck( const dmsRecordData &encodedData,
                                                 BOOLEAN isPrimalData )
   {
      INT32 rc = SDB_OK ;
      UINT8 encodeType = DMS_SCHEMA_GET_ENCODE_TYPE( encodedData.data() ) ;
      UINT8 encodeAttr = DMS_SCHEMA_GET_ENCODE_ATTR( encodedData.data() ) ;
      UINT32 version = DMS_SCHEMA_GET_VERSION( encodedData.data() ) ;
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
      if ( version != _schemaInnerVersion )
      {
         SDB_ASSERT( FALSE, "The encoded version is invalid" ) ;
         PD_LOG( PDERROR, "The encoded version[%d] is not the same with current[%d]",
                 version, _schemaInnerVersion ) ;
         rc = SDB_SYS ;
         goto error ;
      }

      /// check flag
      if ( encodeType != DMS_SCHEMA_ENCODE_HEX && encodeType != DMS_SCHEMA_ENCODE_ORG )
      {
         SDB_ASSERT( FALSE, "The encoded flag is invalid" ) ;
         PD_LOG( PDERROR, "The encode flag[%d] is invalid", encodeType ) ;
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

            if ( DMS_SCHEMA_ENCODE_HEX == encodeType )
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
            else if ( DMS_SCHEMA_ENCODE_ORG == encodeType )
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

         if ( OSS_BIT_TEST( encodeAttr, DMS_SCHEMA_ATTR_VER_STRICT ) )
         {
            if ( colBitmap.validBitSize() !=  _colBitmap.validBitSize() ||
                 !colBitmap.isEqual( _colBitmap ) )
            {
               rc = SDB_SYS ;
               SDB_ASSERT( FALSE, "Has some field not include in record" ) ;
               PD_LOG( PDERROR, "Has some field not include in record, record field size: %u, "
                       "schema field size: %u", colBitmap.validBitSize(),
                       _colBitmap.validBitSize() ) ;
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

   void _dmsInternalSchema::_makeSchemaContextQuery( dmsSchemaContext &context )
   {
      if ( !context.hasSetQuery() )
      {
         const SET_CHARSTRING& setFields = context.getQueryFields() ;
         if ( setFields.empty() )
         {
            context.setQueryWirld() ;
         }
         else
         {
            dmsThreadSchemaBitmap readBitmap( _schemaContainer.columnNum() ) ;
            const CHAR *pName = NULL ;
            const CHAR *pDot = NULL ;
            ossPoolString tmp ;
            UINT16 colID = DMS_SCHEMA_INVALID_COLUMNID ;
            SET_CHARSTRING::const_iterator cit ;

            if ( SDB_OK != readBitmap.init() )
            {
               goto done ;
            }

            for ( cit = setFields.begin() ; cit != setFields.end() ; ++cit )
            {
               pName = *cit ;

               if ( !pName || !*pName )
               {
                  context.setQueryWirld() ;
                  goto done ;
               }

               pDot = ossStrchr( pName, '.' ) ;
               if ( pDot )
               {
                  try
                  {
                     tmp.assign( pName, pDot - pName ) ;
                     pName = tmp.c_str() ;
                  }
                  catch( std::exception & )
                  {
                     goto done ;
                  }
               }

               colID = _schemaHash.getColumnIDByName( pName ) ;
               if ( DMS_SCHEMA_INVALID_COLUMNID != colID )
               {
                  readBitmap.setBit( colID ) ;
               }
            }

            /// set query bit
            context.pushQueryBitmap( readBitmap ) ;
         }
      }

   done:
      return ;
   }

   BOOLEAN _dmsInternalSchema::_testColumn( const CHAR *pName,
                                            const _utilBitmapBase &bitmap )
   {
      BOOLEAN hitColumn = FALSE ;

      if ( pName && *pName )
      {
         UINT16 colID = DMS_SCHEMA_INVALID_COLUMNID ;
         const CHAR *pDot = ossStrchr( pName, '.' ) ;
         ossPoolString tmp ;

         if ( pDot )
         {
            try
            {
               tmp.assign( pName, pDot - pName ) ;
               pName = tmp.c_str() ;
            }
            catch( std::exception & )
            {
               goto done ;
            }
         }

         colID = _schemaHash.getColumnIDByName( pName ) ;
         if ( DMS_SCHEMA_INVALID_COLUMNID != colID &&
              bitmap.testBit( colID ) )
         {
            hitColumn = TRUE ;
         }
      }

   done:
      return hitColumn ;
   }

   BOOLEAN _dmsInternalSchema::testReadDefault( const CHAR *pName )
   {
      ossScopedRWLock lock( &_loadRWMutex, SHARED ) ;
      return _testColumn( pName, _readColBitmap ) ;
   }

   BOOLEAN _dmsInternalSchema::testReadDefault( const SET_CHARSTRING &setNames )
   {
      ossScopedRWLock lock( &_loadRWMutex, SHARED ) ;
      SET_CHARSTRING::const_iterator cit ;

      for ( cit = setNames.begin() ; cit != setNames.end() ; ++cit )
      {
         if ( _testColumn( *cit, _readColBitmap ) )
         {
            return TRUE ;
         }
      }
      return FALSE ;
   }

   BOOLEAN _dmsInternalSchema::testWriteDefault( const CHAR *pName )
   {
      ossScopedRWLock lock( &_loadRWMutex, SHARED ) ;
      return _testColumn( pName, _writeColBitmap ) ;
   }

   BOOLEAN _dmsInternalSchema::testWriteDefault( const SET_CHARSTRING &setNames )
   {
      ossScopedRWLock lock( &_loadRWMutex, SHARED ) ;
      SET_CHARSTRING::const_iterator cit ;

      for ( cit = setNames.begin() ; cit != setNames.end() ; ++cit )
      {
         if ( _testColumn( *cit, _writeColBitmap ) )
         {
            return TRUE ;
         }
      }
      return FALSE ;
   }

}

