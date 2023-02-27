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

   Source File Name = dmsInternalSchema.hpp

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

#ifndef DMS_INTERNALSCHEMA_HPP__
#define DMS_INTERNALSCHEMA_HPP__

#include "dms.hpp"
#include "dmsStorageBase.hpp"
#include "utilBSON.hpp"
#include "dmsSchemaColRecord.hpp"


#define DMS_SCHEMA_INVALID_VERSION                 (0)
#define DMS_SCHEMA_INVALID_COLUMNID                0xFFFF

#define DMS_SCHEMA_HASH_BUCKET_SIZE                (4096)

#define DMS_SCHEMA_COL_DELETED                     0x8000
#define DMS_SCHEMA_COL_IN_INDEX                    0x4000
#define DMS_SCHEMA_COL_READ_DEFAULT                0x2000
#define DMS_SCHEMA_COL_WRITE_DEFAULT               0x1000
#define DMS_SCHEMA_COL_HAS_ORIGNAME                0x0800

#define DMS_SCHEMA_HASH_ID_MASK                    0x0000FFFF
#define DMS_SCHEMA_HASH_OFFSET_MASK                0xFFFF0000
#define DMS_SCHEMA_HASH_INVALID_ITEM_VALUE         0xFFFFFFFF

#define DMS_SCHEMA_MAX_COLUMN_NUM                  65535
#define DMS_SCHEMA_INVALID_ITEM_ID                 65535
#define DMS_SCHEMA_INVALID_ITEM_OFFSET             65535

namespace engine
{
   class _pmdEDUCB ;
   class _dmsStorageDataCommon ;
   class _dmsMBContext ;

   /* Container of the internal schema extent. It provides interfaces to operate on the extent.
   */
   class _dmsSchemaContainer : public SDBObject
   {
      public:
         _dmsSchemaContainer() ;
         virtual ~_dmsSchemaContainer() ;

         INT32 init( const dmsSchemaExtent *extent, UINT32 extentSize, UINT16 mbID ) ;

         void  reset() ;

         INT32 getColIDsWithDefault( ossPoolSet<UINT16> &colsWithReadDefault,
                                     ossPoolSet<UINT16> &colsWithWriteDefault,
                                     ossPoolSet<UINT16> &colsWithReadDefaultIndexCol ) const ;

         BOOLEAN hasReadDefault( UINT16 columnID ) const ;

         BOOLEAN hasWriteDefault( UINT16 columnID ) const ;

         BOOLEAN isIndexColumn( UINT16 columnID ) const ;

         INT32 getColReadDefault( UINT16 columnID, const CHAR *&name, INT32 &nameLen,
                                  BSONType &type, const CHAR *&value, INT32 &valueLen ) const ;

         INT32 getColWriteDefault( UINT16 columnID, const CHAR *&name, INT32 &nameLen,
                                   BSONType &type, const CHAR *&value, INT32 &valueLen ) const ;

         /* Getting basic information of the column, including the name and attribute.
         */
         INT32 getColumnBasicInfo( UINT16 columnID, const CHAR **name, INT32 *nameLen,
                                   BOOLEAN *isDeleted = NULL, BOOLEAN *hasReadDefault = NULL,
                                   BOOLEAN *hasWriteDefault = NULL, BOOLEAN *isIndexColumn = NULL,
                                   BOOLEAN *hasOrigName = NULL ) const ;

         OSS_INLINE const CHAR *getColumnName( UINT16 columnID, INT32 *nameLen = NULL ) const ;

         const CHAR *getOrigName( UINT16 columnID, INT32 *nameLen = NULL ) const ;

         INT32 getColumnInfoSize( UINT16 columnID, UINT16 &size ) const ;

         UINT16 columnNum() const
         {
            return _extent->_itemNum ;
         }

         const dmsSchemaExtent* getExtent() const
         {
            return _extent ;
         }

         UINT32 getExtentSize() const
         {
            return _extentSize ;
         }

         // Fetch information of all valid columns.
         INT32 toObj( BSONObj &object ) const ;

         // Dump information of all columns in the internal schema, including those marked as
         // deleted.
         INT32 dump( BSONObj &schemaInfo, BOOLEAN includeColumnID = TRUE ) const ;

      protected:
         void                 _setColumnAttr( UINT16 columnID, INT16 flags ) ;
         void                 _clearColumnAttr( UINT16 columnID, INT16 flags ) ;

         OSS_INLINE INT16    _getColumnAttr( UINT16 columnID ) const ;
         OSS_INLINE UINT16   _getColRecordOffset( UINT16 columnID ) const ;

         void     _getColAttrAndRecordOffset( UINT16 columnID, INT16 &attr,
                                              UINT16 &valueOffset ) const ;

         OSS_INLINE const dmsSchemaColRecord *_getColRecord( UINT16 columnID ) const ;

         BOOLEAN  _isColumnDeleted( UINT16 columnID ) const ;

         BOOLEAN  _isIndexColumn( UINT16 columnID ) const ;

         INT32    _columnInfo2Def( UINT16 columnID, const CHAR **name, BSONObj &columnDef ) const ;

         INT32    _columnInfo2Obj( UINT16 columnID, BSONObj &object, BOOLEAN includeID = FALSE,
                                   BOOLEAN includeAttr = FALSE ) ;

         OSS_INLINE const CHAR* _offset2Ptr( UINT32 offset ) const ;

      protected:
         const dmsSchemaExtent            *_extent ;     // Pointer of the schema extent.
         UINT32                            _extentSize ;
   } ;
   typedef _dmsSchemaContainer dmsSchemaContainer ;

   class _dmsSchemaHash : public SDBObject
   {
      public:
         _dmsSchemaHash() ;
         virtual ~_dmsSchemaHash() ;

         INT32 init( const dmsSchemaContainer *schemaContainer,
                     const dmsSchemaHashExtent *extent, UINT32 extentSize, UINT16 mbID ) ;

         void  reset() ;

         UINT16 getColumnIDByName( const CHAR *name ) const ;

         const dmsSchemaHashExtent* getExtent() const
         {
            return _extent ;
         }

         UINT32 getExtentSize() const
         {
            return _extentSize ;
         }

      protected:
         UINT16                     _getColumnIDByItem( const INT32 *item ) const ;
         // Offset of the first free item in the conflict item area.
         INT32                      _nextFreeItemOffset() const ;
         OSS_INLINE UINT16          _getNextItemOffset( const INT32 *item ) const ;
         OSS_INLINE const CHAR     *_offset2Ptr( UINT32 offset ) const ;
         OSS_INLINE INT32           _getItemByName( const CHAR *name, INT32 *&item,
                                                    INT32 **prevItem ) ;
         OSS_INLINE INT32 *         _itemOffset2Ptr( UINT16 offset ) ;

      protected:
         const dmsSchemaContainer  *_schemaContainer ;
         const dmsSchemaHashExtent *_extent ;
         UINT32                     _extentSize ;
   } ;
   typedef _dmsSchemaHash dmsSchemaHash ;

   class _dmsInternalSchema : public SDBObject
   {
      struct _columnInfo
      {
         BOOLEAN     _isOrigName ;
         BOOLEAN     _isDeleted ;
         BOOLEAN     _hasReadDefault ;
         UINT16      _columnID ;

         _columnInfo( BOOLEAN isOrigName, UINT16 columnID,
                      BOOLEAN isDeleted = FALSE, BOOLEAN hasReadDefault = FALSE )
         : _isOrigName( isOrigName ),
           _isDeleted( isDeleted ),
           _hasReadDefault( hasReadDefault ),
           _columnID( columnID )
         {
         }
      } ;
      typedef _columnInfo columnInfo ;

      struct name_cmp
      {
         BOOLEAN operator()( const CHAR *left, const CHAR *right )
         {
            return ossStrcmp( left, right ) < 0 ;
         }
      } ;

      typedef ossPoolSet<UINT16>                      COLUMN_ID_SET ;
      typedef COLUMN_ID_SET::const_iterator           COLUMN_ID_SET_CITR ;
      typedef ossPoolMap< const CHAR *, columnInfo, name_cmp >  NAME_INFO_MAP ;
      typedef NAME_INFO_MAP::iterator                 NAME_DECODE_INFO_ITR ;

      public:
         _dmsInternalSchema() ;
         virtual ~_dmsInternalSchema() ;

         INT32    init( const dmsSchemaExtent *schemaExtent, UINT32 schemaExtentSize,
                        const dmsSchemaHashExtent *hashExtent, UINT32 hashExtentSize,
                        UINT16 mbID ) ;

         INT32    reload() ;

         void     reset() ;

         BOOLEAN  enabled() const
         {
            return _enabled ;
         }

         INT32    getVersion() const
         {
            return _version ;
         }

         INT32    encodeRecord( _dmsMBContext *context, _pmdEDUCB *cb, dmsRecordData &recordData,
                                dmsRecordData &encodeData, BOOLEAN &hasNewColumn ) ;

         // Decode a record which is encoded by the internal schema.
         INT32    decodeRecord( _pmdEDUCB *cb, const CHAR *data, UINT32 dataSize,
                                const CHAR **record, UINT32 &recordSize,
                                BOOLEAN getPrimalData = FALSE ) ;

         BOOLEAN  needRebiuldUncodedRecord() const
         {
            return _decodeWatchNames.size() > 0 ;
         }

         // Rebuild a record which is not encoded by the internal schema. Possible actions
         // including:
         // 1. Change old names to new names.
         // 2. Removed fields which have been deleted in the schema.
         // 3. Add new fields with read default values which are not in the record according to the
         //    internal schema.
         INT32    rebuildRecord( _pmdEDUCB *cb, const BSONObj &record, const CHAR **newRecord,
                                 UINT32 &newRecSize, BOOLEAN &changed,
                                 BOOLEAN getPrimalData = FALSE ) ;

         // Dump the internal schema information. All columns are included.
         INT32    dumpSchemaInfo( BSONObj &schema, BOOLEAN includeColumnID = TRUE ) ;

         INT32    toObj( BSONObj &obj ) ;
         INT32    toSchemaObj( const CHAR *name,
                               bson::BSONObj &boSchema ) ;

         const dmsSchemaContainer* getSchemaContainer() const
         {
            return &_schemaContainer ;
         }

         const dmsSchemaHash* getSchemaHashTable() const
         {
            return &_schemaHash ;
         }

      private:
         INT32    _parseRecord( _dmsMBContext *context, utilBSONRawBuilder &encodeBuilder,
                                COLUMN_ID_SET &watchIDs, const BSONObj &record,
                                BOOLEAN &hasNewColumn ) ;


         // Append primal columns which do not exist in the original record.
         INT32    _appendPrimalColumns( _dmsMBContext *context, _pmdEDUCB *cb,
                                        utilBSONRawBuilder &encodeBuilder,
                                        COLUMN_ID_SET &watchIDs, const BSONObj& originalRecord,
                                        dmsRecordData &recordData ) ;

         INT32    _checkRebuildRecord( const BSONObj &record, NAME_INFO_MAP &watchNames ) ;


         INT32    _appendColWithReadDefault( ossPoolSet<UINT16> &colIDs,
                                             utilBSONRawBuilder &builder ) ;

         INT32    _onSchemaColChanged() ;

         void     _logSchemaInfo() ;

         INT32    _encodeSanityCheck( const dmsRecordData &encodedData ) ;

      private:
         BOOLEAN                _enabled ;
         INT32                  _version ;
         dmsSchemaContainer     _schemaContainer ;
         dmsSchemaHash          _schemaHash ;
         ossPoolSet<UINT16>     _encodeWatchIDs ;   // Columns with write default value or index
                                                   // column with read default value.
         ossPoolSet<UINT16>     _decodeWatchIDs ;    // Column with read default value.

         // Code review: change to _utilSet

         NAME_INFO_MAP          _decodeWatchNames ;

         UINT16                 _defaultMaxSize ;  // Not accurate, but sure to be enough.
         UINT16                 _totalValidNameSize ;
   } ;
   typedef _dmsInternalSchema dmsInternalSchema ;

   OSS_INLINE const CHAR *_dmsSchemaContainer::getColumnName( UINT16 columnID,
                                                              INT32 *nameLen ) const
   {
      const dmsSchemaColRecord *record = _getColRecord( columnID ) ;
      return record->getName( nameLen ) ;
   }

   OSS_INLINE INT16 _dmsSchemaContainer::_getColumnAttr( UINT16 columnID ) const
   {
      const INT32* slot= (const INT32 *)
         _offset2Ptr( DMS_SCHEMAEXTENT_HEADER_SZ + DMS_SCHEMAEXTENT_SLOT_SZ * columnID ) ;
      return (INT16)(*slot >> 16) ;
   }

   OSS_INLINE UINT16 _dmsSchemaContainer::_getColRecordOffset( UINT16 columnID ) const
   {
      const INT32* slot= (const INT32 *)
         _offset2Ptr( DMS_SCHEMAEXTENT_HEADER_SZ + DMS_SCHEMAEXTENT_SLOT_SZ * columnID ) ;
      return (UINT16)( (*slot) & 0x0000FFFF ) ;
   }

   OSS_INLINE const dmsSchemaColRecord *_dmsSchemaContainer::_getColRecord( UINT16 columnID ) const
   {
      return (const dmsSchemaColRecord *)_offset2Ptr( _getColRecordOffset( columnID ) ) ;
   }

   OSS_INLINE const CHAR *_dmsSchemaContainer::_offset2Ptr( UINT32 offset ) const
   {
      return (const CHAR *)_extent + offset ;
   }

   OSS_INLINE UINT16 _dmsSchemaHash::_getNextItemOffset( const INT32 *item ) const
   {
      return (UINT16)( (*item) >> 16 ) ;
   }

   OSS_INLINE const CHAR *_dmsSchemaHash::_offset2Ptr( UINT32 offset ) const
   {
      return (const CHAR *)_extent + offset ;
   }
}

#endif /* DMS_INTERNALSCHEMA_HPP__ */
