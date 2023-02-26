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

#define DMS_SCHEMA_INVALID_VERSION        (0)
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
         ~_dmsSchemaContainer() ;

         // Initialize the internal schema, including the internal schema container and internal
         // schema hash table.
         INT32 init( _dmsStorageDataCommon *su, _dmsMBContext *context,
                     dmsExtentID extentID, BOOLEAN isLoad = TRUE ) ;
         void  reset() ;

         INT32 addColumn( const CHAR *name, const BSONObj *columnDef, UINT16 &columnID,
                          const CHAR *origName = NULL ) ;
         INT32 alterColumn( UINT16 columnID, const BSONObj &columnInfo ) ;
         INT32 dropColumn( UINT16 columnID ) ;

         INT32 dropColumnDefault( UINT16 columnID, BOOLEAN dropWrite, BOOLEAN dropRead = FALSE ) ;

         INT32 renameColumn( UINT16 columnID, const CHAR *newName ) ;

         INT32 getColIDsWithDefault( ossPoolSet<UINT16> &colsWithReadDefault,
                                     ossPoolSet<UINT16> &colsWithWriteDefault,
                                     ossPoolSet<UINT16> &colsWithReadDefaultIndexCol ) const ;

         BOOLEAN hasReadDefault( UINT16 columnID ) const ;

         BOOLEAN hasWriteDefault( UINT16 columnID ) const ;

         BOOLEAN isIndexColumn( UINT16 columnID ) const ;

         void    setIndexColumn( UINT16 columnID ) ;

         void    unsetIndexColumn( UINT16 columnID ) ;

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

         const CHAR *getColumnName( UINT16 columnID, INT32 *nameLen = NULL ) const ;

         const CHAR *getOrigName( UINT16 columnID, INT32 *nameLen = NULL ) const ;

         INT32 getColumnInfoSize( UINT16 columnID, UINT16 &size ) const ;

         UINT16 columnNum() const
         {
            return _extent->_itemNum ;
         }

         // Fetch information of all valid columns.
         INT32 toObj( BSONObj &object ) const ;

         // Dump information of all columns in the internal schema, including those marked as
         // deleted.
         INT32 dump( BSONObj &schemaInfo, BOOLEAN includeColumnID = TRUE ) const ;

         INT32 flush() ;

      private:
         INT32    _allocSpace4ColRecord( UINT16 slotSize, UINT16 vaueSize, UINT16 &offset,
                                         UINT16 *allocSize = NULL ) ;

         void     _setColumnAttr( UINT16 columnID, INT16 flags ) ;
         INT16    _getColumnAttr( UINT16 columnID ) const ;
         void     _clearColumnAttr( UINT16 columnID, INT16 flags ) ;

         void     _setColRecordOffset( UINT16 columnID, UINT16 offset ) ;
         UINT16   _getColRecordOffset( UINT16 columnID ) const ;

         void     _initColAttrAndRecordOffset( UINT16 columnID, INT16 attr, UINT16 valOffset ) ;
         void     _getColAttrAndRecordOffset( UINT16 columnID, INT16 &attr,
                                              UINT16 &valueOffset ) const ;

         const dmsSchemaColRecord *_getColRecord( UINT16 columnID ) const ;

         INT32    _updateColRecord( UINT16 columnID, const dmsSchemaColRecord *oldRecord,
                                    const dmsSchemaColRecord *newRecord ) ;

         BOOLEAN  _isColumnDeleted( UINT16 columnID ) const ;
         BOOLEAN  _isIndexColumn( UINT16 columnID ) const ;
         UINT32   _freeSpace() const ;


         INT32    _columnInfo2Def( UINT16 columnID, const CHAR **name, BSONObj &columnDef ) const ;

         INT32    _columnInfo2Obj( UINT16 columnID, BSONObj &object, BOOLEAN includeID = FALSE,
                                   BOOLEAN includeAttr = FALSE ) ;

      private:
         // TODO: YSD compact the extent
         INT32    _compact() ;

      private:
         _dmsStorageDataCommon           *_su ;
         UINT16                           _mbID ;
         dmsExtentID                      _extentID ;
         const dmsSchemaExtent           *_extent ;     // Read pointer of the schema extent.
         dmsExtRW                         _extRW ;       // Review: 不能作为成员变量存储，可能存在可靠性问题，使用 beginFixAddr ?
   } ;
   typedef _dmsSchemaContainer dmsSchemaContainer ;

   class _dmsSchemaHash : public SDBObject
   {
      public:
         _dmsSchemaHash( dmsSchemaContainer *schemaContainer ) ;
         ~_dmsSchemaHash() ;

         INT32 init( _dmsStorageDataCommon *su, _dmsMBContext *context,
                     dmsExtentID extentID, BOOLEAN isLoad = TRUE ) ;
         void  reset() ;

         UINT16 getColumnIDByName( const CHAR *name ) ;
         INT32  addColumnItem( const CHAR *name, UINT16 columnID ) ;
         INT32  dropColumnItemByName( const CHAR *name ) ;
         INT32  flush() ;

      private:
         void   _setColumnIDInItem( INT32 *item, UINT16 columnID ) ;
         UINT16 _getColumnIDByItem( INT32 *item ) ;

         INT32  _getItemByName( const CHAR *name, INT32 *&item, INT32 **prevItem ) ;
         void   _setNextItemOffset( INT32 *item, UINT16 id ) ;
         UINT16 _getNextItemOffset( INT32 *item ) ;
         void   _resetItem( INT32 *item )
         {
            *item = 0xFFFFFFFF ;
         }

         OSS_INLINE INT32 *_itemOffset2Ptr( UINT16 offset ) ;

      private:
         dmsSchemaContainer        *_schemaContainer ;
         _dmsStorageDataCommon     *_su ;
         UINT16                     _mbID ;
         dmsExtentID                _extentID ;
         const dmsSchemaHashExtent *_extent ;
         dmsExtRW                   _extRW ;
         INT32                      _nextFreeItemOffset ;
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
         ~_dmsInternalSchema() ;

         INT32    init( _dmsStorageDataCommon *su, _dmsMBContext *context,
                        dmsExtentID schemaExtentID, dmsExtentID schemaHashExtentID,
                        BOOLEAN isLoad = TRUE ) ;

         void     reset() ;

         BOOLEAN  enabled() const
         {
            return _enabled ;
         }

         INT32    getVersion() const
         {
            return _version ;
         }

         INT32    addColumn( _dmsMBContext *context, const CHAR *columnName,
                             const BSONObj *columnDef = NULL, UINT16 *columnID = NULL,
                             BOOLEAN mergeOnExist = FALSE, const CHAR *origName = NULL ) ;

         INT32    dropColumn( _dmsMBContext *context,  const CHAR *columnName ) ;

         INT32    renameColumn( _dmsMBContext *context, const CHAR *oldName, const CHAR *newName,
                                BOOLEAN *oldColFound ) ;

         /**
          * Drop default value of a column. Only the write default can be dropped.
         */
         INT32    dropColumnDefault( _dmsMBContext *context, const CHAR *name ) ;

         INT32    alterColumn( _dmsMBContext *context, const CHAR *columnName,
                               const BSONObj &columnDef ) ;

         INT32    setIndexColumn( _dmsMBContext *context, const CHAR *columnName ) ;

         INT32    unsetIndexColumn( _dmsMBContext *context, const CHAR *columnName ) ;

         INT32    encodeRecord( _dmsMBContext *context, _pmdEDUCB *cb, dmsRecordData &recordData,
                                dmsRecordData &encodeData ) ;

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

      private:
         INT32    _parseRecord( _dmsMBContext *context, utilBSONRawBuilder &encodeBuilder,
                                COLUMN_ID_SET &watchIDs, const BSONObj &record,
                                BOOLEAN &hasNewColumn ) ;

         INT32    _updateSchemaByRecord( _dmsMBContext *context, const BSONObj &record ) ;

         // Append primal columns which do not exist in the original record.
         INT32    _appendPrimalColumns( _dmsMBContext *context, _pmdEDUCB *cb,
                                        utilBSONRawBuilder &encodeBuilder,
                                        COLUMN_ID_SET &watchIDs, const BSONObj& originalRecord,
                                        dmsRecordData &recordData ) ;

         INT32    _checkRebuildRecord( const BSONObj &record, NAME_INFO_MAP &watchNames ) ;

         INT32    _merge2Column( UINT16 columnID, const BSONObj &columnDef ) ;

         INT32    _appendColWithReadDefault( ossPoolSet<UINT16> &colIDs,
                                             utilBSONRawBuilder &builder ) ;

         INT32    _onSchemaColChanged() ;

         void     _logSchemaInfo() ;

         INT32    _flush() ;

         INT32    _encodeSanityCheck( const dmsRecordData &encodedData ) ;

      private:
         BOOLEAN                _enabled ;
         INT32                  _version ;
         dmsSchemaContainer     _schemaContainer ;
         dmsSchemaHash          _schemaHash ;
         ossPoolSet<UINT16>     _readDefaultIDsOfIndexCol ;

         ossPoolSet<UINT16>     _encodeWatchIDs ;   // Columns with write default value or index
                                                   // column with read default value.
         ossPoolSet<UINT16>     _decodeWatchIDs ;    // Column with read default value.

         // Code review: change to _utilSet

         NAME_INFO_MAP          _decodeWatchNames ;

         UINT16                 _defaultMaxSize ;  // Not accurate, but sure to be enough.
         UINT16                 _totalValidNameSize ;
   } ;
   typedef _dmsInternalSchema dmsInternalSchema ;

   OSS_INLINE INT32 *_dmsSchemaHash::_itemOffset2Ptr( UINT16 offset )
   {
      return (INT32 *)_extRW.readPtr( offset, DMS_SCHEMAHASHEXTENT_ITEM_SZ ) ;
   }
}

#endif /* DMS_INTERNALSCHEMA_HPP__ */
