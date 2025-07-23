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
#include "utilBSON.hpp"
#include "dmsSchemaColRecord.hpp"
#include "dmsExtent.hpp"
#include "utilBitmap.hpp"
#include "ossRWMutex.hpp"

/*
   Attr define ( 1 Byte )
*/
#define DMS_SCHEMA_COL_DELETED                     0x80
#define DMS_SCHEMA_COL_IN_INDEX                    0x40
#define DMS_SCHEMA_COL_READ_DEFAULT                0x20
#define DMS_SCHEMA_COL_WRITE_DEFAULT               0x10
#define DMS_SCHEMA_COL_HAS_ORIGNAME                0x08
#define DMS_SCHEMA_COL_HIDDEN                      0x04

#define DMS_SCHEMA_EXTENT_MAX_SZ                   ( 4 << 20 )

using namespace bson ;

namespace engine
{
   class _pmdEDUCB ;

   /*
      typedef
   */
   typedef _utilThreadBitmap<DMS_SCHEMA_DFT_COLUMN_NUM>     dmsThreadSchemaBitmap ;
   typedef utilBitmap                                       dmsSchemaBitmap ;

   /*
      Container of the internal schema extent. It provides interfaces to operate on the extent.
   */
   class _dmsSchemaContainer : public SDBObject
   {
      friend class _dmsSchemaWriter ;

      public:
         _dmsSchemaContainer() ;
         virtual ~_dmsSchemaContainer() ;

         INT32 init( const dmsSchemaExtent *extent, UINT32 extentSize, UINT16 mbID ) ;

         void  reset() ;

         BOOLEAN isColumnDeleted( UINT16 columnID ) const
         {
            SDB_ASSERT( columnID < _extent->_itemNum, "Column id is invalid" ) ;
            return OSS_BIT_TEST( _getColumnAttr( columnID ), DMS_SCHEMA_COL_DELETED ) ;
         }

         BOOLEAN hasReadDefault( UINT16 columnID ) const
         {
            SDB_ASSERT( columnID < _extent->_itemNum, "Column id is invalid" ) ;
            return !isColumnDeleted( columnID ) &&
                   OSS_BIT_TEST( _getColumnAttr( columnID ), DMS_SCHEMA_COL_READ_DEFAULT ) ;
         }

         BOOLEAN hasWriteDefault( UINT16 columnID ) const
         {
            SDB_ASSERT( columnID < _extent->_itemNum, "Column id is invalid" ) ;
            return !isColumnDeleted( columnID ) &&
                   OSS_BIT_TEST( _getColumnAttr( columnID ), DMS_SCHEMA_COL_WRITE_DEFAULT ) ;
         }

         BOOLEAN isIndexColumn( UINT16 columnID ) const
         {
            SDB_ASSERT( columnID < _extent->_itemNum, "Column id is invalid" ) ;
            return !isColumnDeleted( columnID ) &&
                   OSS_BIT_TEST( _getColumnAttr( columnID ), DMS_SCHEMA_COL_IN_INDEX ) ;
         }

         BOOLEAN isColumnHidden( UINT16 columnID ) const
         {
            SDB_ASSERT( columnID < _extent->_itemNum, "Column id is invalid" ) ;
            return !isColumnDeleted( columnID ) &&
                   OSS_BIT_TEST( _getColumnAttr( columnID ), DMS_SCHEMA_COL_HIDDEN ) ;
         }

         INT32 getColDefaultInitVersion( UINT16 columnID, UINT32 &initVersion ) const ;

         INT32 getColReadDefault( UINT16 columnID, const CHAR *&name, INT32 &nameLen,
                                  BSONType &type, const CHAR *&value, INT32 &valueLen ) const ;

         INT32 getColWriteDefault( UINT16 columnID, const CHAR *&name, INT32 &nameLen,
                                   BSONType &type, const CHAR *&value, INT32 &valueLen ) const ;

         /* Getting basic information of the column, including the name and attribute.
         */
         INT32 getColumnBasicInfo( UINT16 columnID, const CHAR **name, INT32 *nameLen,
                                   BOOLEAN *isDeleted = NULL, BOOLEAN *hasReadDefault = NULL,
                                   BOOLEAN *hasWriteDefault = NULL, BOOLEAN *isIndexColumn = NULL,
                                   BOOLEAN *hasOrigName = NULL, BOOLEAN *isHidden = NULL,
                                   UINT32 *defaultValInitVersion = NULL ) const ;

         OSS_INLINE const CHAR *getColumnName( UINT16 columnID, INT32 *nameLen = NULL ) const ;

         const CHAR *getOrigName( UINT16 columnID, INT32 *nameLen = NULL ) const ;

         INT32 getColumnInfoSize( UINT16 columnID, UINT16 &size ) const ;

         INT32 getReadEleSize( UINT16 columnID, UINT32 &size ) const ;
         INT32 getWriteEleSize( UINT16 columnID, UINT32 &size ) const ;

         UINT16 columnNum() const
         {
            if ( _extent )
            {
               return _extent->_itemNum ;
            }
            SDB_ASSERT( FALSE, "Invalid extent" ) ;
            return 0 ;
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
         INT32    _columnInfo2Def( UINT16 columnID, BSONObjBuilder &builder ) const ;

         INT32    _columnInfo2Obj( UINT16 columnID,
                                   BSONObjBuilder &builder,
                                   BOOLEAN includeID = FALSE,
                                   BOOLEAN includeAttr = FALSE ) const ;

      protected:

         UINT8 _getColumnAttr( UINT16 columnID ) const
         {
            const dmsSchemaColSlot* pSlot = _getColSlot( columnID ) ;
            if ( pSlot )
            {
               return pSlot->getAttr() ;
            }
            return 0 ;
         }

         UINT32 _getColRecordOffset( UINT16 columnID ) const
         {
            const dmsSchemaColSlot* pSlot = _getColSlot( columnID ) ;
            if ( pSlot )
            {
               return pSlot->getOffset() ;
            }
            /// invliad offset
            return 0xFFFFFFFF ;
         }

         INT32  _getColAttrAndRecordOffset( UINT16 columnID,
                                            UINT8 &attr,
                                            UINT32 &valueOffset ) const
         {
            const dmsSchemaColSlot* pSlot = _getColSlot( columnID ) ;
            if ( pSlot )
            {
               attr = pSlot->getAttr() ;
               valueOffset = pSlot->getOffset() ;
               return SDB_OK ;
            }
            return SDB_INVALIDARG ;
         }

         const CHAR* _offset2Ptr( UINT32 offset ) const
         {
            if ( _extent && offset < _extentSize )
            {
               return (const CHAR *)_extent + offset ;
            }
            SDB_ASSERT( FALSE, "_extent or offset invalid" ) ;
            return NULL ;
         }

         const dmsSchemaColSlot* _getColSlot( UINT16 columnID ) const
         {
            if ( _extent && columnID < _extent->_itemNum )
            {
               return (const dmsSchemaColSlot*)_offset2Ptr( (UINT32)DMS_SCHEMAEXTENT_HEADER_SZ +
                                                            DMS_SCHEMAEXTENT_SLOT_SZ * columnID ) ;
            }
            SDB_ASSERT( FALSE, "_extent or columnID invalid" ) ;
            return NULL ;
         }

         const dmsSchemaColRecord* _getColRecord( UINT16 columnID ) const
         {
            const dmsSchemaColSlot* pSlot = _getColSlot( columnID ) ;
            if ( pSlot )
            {
               return (const dmsSchemaColRecord*)_offset2Ptr( pSlot->getOffset() ) ;
            }
            SDB_ASSERT( FALSE, "invalid _extent or columnID" ) ;
            return NULL ;
         }

      protected:
         const dmsSchemaExtent            *_extent ;     // Pointer of the schema extent.
         UINT32                            _extentSize ;

   } ;
   typedef _dmsSchemaContainer dmsSchemaContainer ;

   /*
      _dmsSchemaContainer inline functions
   */
   OSS_INLINE const CHAR *_dmsSchemaContainer::getColumnName( UINT16 columnID,
                                                              INT32 *nameLen ) const
   {
      const dmsSchemaColRecord *record = _getColRecord( columnID ) ;
      if ( record )
      {
         return record->getName( nameLen ) ;
      }
      if ( nameLen )
      {
         *nameLen = 0 ;
      }
      return "" ;
   }

   /*
      _dmsSchemaHash define
   */
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

         const CHAR *_offset2Ptr( UINT32 offset ) const
         {
            if ( _extent && offset < _extentSize )
            {
               return (const CHAR *)_extent + offset ;
            }
            SDB_ASSERT( FALSE, "_extent or offset invalid" ) ;
            return NULL ;
         }

         const dmsSchemaHashSlot* _getHashBucketSlot( UINT32 slotID ) const
         {
            if ( _extent && slotID < _bucketNum )
            {
               return &_pBucketSlot[ slotID ] ;
            }
            SDB_ASSERT( FALSE, "_extent or slotID invalid" ) ;
            return NULL ;
         }

         const dmsSchemaHashSlot* _getHashListSlot( UINT32 slotID ) const
         {
            if ( _extent && slotID < _extent->_slotNum )
            {
               return &_pListSlot[ slotID ] ;
            }
            SDB_ASSERT( FALSE, "_extent or slotID invalid" ) ;
            return NULL ;
         }

      protected:
         const dmsSchemaContainer  *_schemaContainer ;
         const dmsSchemaHashExtent *_extent ;
         UINT32                     _extentSize ;
         UINT32                     _bucketNum ;
         const dmsSchemaHashSlot   *_pBucketSlot ;
         const dmsSchemaHashSlot   *_pListSlot ;
   } ;
   typedef _dmsSchemaHash dmsSchemaHash ;

   /*
      _dmsSchemaContextItem define
   */
   struct _dmsSchemaContextItem
   {
      dmsThreadSchemaBitmap   _readBitmap ;
      BOOLEAN                 _hitName ;
      UINT64                  _useID ;

      _dmsSchemaContextItem()
      : _readBitmap( 0 ), _hitName( FALSE ), _useID( 0 )
      {
      }
   } ;
   typedef _dmsSchemaContextItem dmsSchemaContextItem ;

   /*
      _dmsSchemaContext define
   */
   class _dmsSchemaContext : public SDBObject
   {
      typedef ossPoolMap< UINT32, dmsSchemaContextItem >       MAP_CTX_ITEM ;

      public:
         _dmsSchemaContext() ;
         ~_dmsSchemaContext() ;

         const SET_CHARSTRING&  getQueryFields() const { return _setQueryFields ; }
         SET_CHARSTRING& getQueryFields() { return _setQueryFields ; }
         BOOLEAN  hasSetQuery() const { return _hasSetQuery ; }

      public:

         BOOLEAN  validateCheck( UINT32 curSchemaVersion ) ;

         void     prune( UINT32 recordVersion,
                         UINT8 recordAttr,
                         _utilBitmapBase &readBitmap,
                         BOOLEAN &hitName,
                         BOOLEAN &found ) ;

         void     pushItem( UINT32 recordVersion,
                            UINT8 recordAttr,
                            const _utilBitmapBase &readBitmap,
                            BOOLEAN hitName ) ;

         void     pushQueryBitmap( const _utilBitmapBase &readBitmap ) ;
         void     setQueryWirld() ;

      protected:
         void     _clearBitInfo() ;
         BOOLEAN  _kickOutHisItem() ;

      protected:
         UINT64                        _hwUseID ;
         MAP_CTX_ITEM                  _mapHisItem ;
         _utilBitmap                   _queryBitmap ;
         BOOLEAN                       _isWirld ;
         BOOLEAN                       _hasSetQuery ;
         SET_CHARSTRING                _setQueryFields ;
         UINT32                        _curSchemaVersion ;

   } ;
   typedef _dmsSchemaContext dmsSchemaContext ;

   /*
      _dmsDecodeWatchKey define
   */
   class _dmsDecodeWatchValue : public utilPooledObject
   {
      public:
         _dmsDecodeWatchValue( UINT16 colID = DMS_SCHEMA_INVALID_COLUMNID,
                               BOOLEAN isDeleted = FALSE ) ;
         ~_dmsDecodeWatchValue() ;

         INT32       setName( const CHAR *pName, INT32 nameLen,
                              const CHAR *pOrgName = NULL,
                              INT32 orgNameLen = 0 ) ;
         BOOLEAN     getName( const CHAR **ppName, INT32 &nameLen ) const
         {
            if ( _pName )
            {
               *ppName = _pName ;
               nameLen = _nameLen ;
               return TRUE ;
            }
            return FALSE ;
         }
         const CHAR* getName() const { return _pName ; }
         const CHAR* getOrgName() const { return _pOrgName ; }

         UINT16      getColID() const { return _colID ; }
         BOOLEAN     isDeleted() const { return _isDeleted ; }
         BOOLEAN     hasOrgName() const { return _pOrgName ? TRUE : FALSE ; }

      private:
         UINT16         _colID ;
         BOOLEAN        _isDeleted ;
         CHAR*          _pName ;
         INT32          _nameLen ;
         CHAR*          _pOrgName ;
         INT32          _orgNameLen ;
   } ;
   typedef _dmsDecodeWatchValue dmsDecodeWatchValue ;

   #define DMS_SCHEMA_ENCODE_FILL_SZ         ( 4 )
   /*
      _dmsInternalSchema define
   */
   class _dmsInternalSchema : public SDBObject
   {
      typedef ossPoolMap< const CHAR *, dmsDecodeWatchValue*, _ossCharStringCmp > NAME_INFO_MAP ;
      typedef NAME_INFO_MAP::iterator                                             NAME_INFO_MAP_ITR ;

      public:
         _dmsInternalSchema() ;
         virtual ~_dmsInternalSchema() ;

         INT32    init( const dmsSchemaExtent *schemaExtent,
                        UINT32 schemaExtentSize,
                        const dmsSchemaHashExtent *hashExtent,
                        UINT32 hashExtentSize,
                        UINT16 mbID,
                        BOOLEAN forceEncode = FALSE ) ;

         INT32    reload( const dmsSchemaExtent *schemaExtent,
                          UINT32 schemaExtentSize,
                          const dmsSchemaHashExtent *hashExtent,
                          UINT32 hashExtentSize,
                          UINT16 mbID ) ;

         void     reset() ;

         BOOLEAN  enabled() const
         {
            return _enabled ;
         }

         UINT32   getSchemaVersion() const
         {
            return _schemaVersion ;
         }
         UINT32   getSchemaInnerVersion() const
         {
            return _schemaInnerVersion ;
         }

         INT32    encodeRecord( _pmdEDUCB *cb,
                                dmsRecordData &recordData,
                                BOOLEAN &memAlloc,
                                dmsRecordData &encodeData,
                                BOOLEAN &hasNewCol,
                                BOOLEAN isPrimalData = FALSE ) ;

         // Decode a record which is encoded by the internal schema.
         INT32    decodeRecord( _pmdEDUCB *cb,
                                const CHAR *data,
                                UINT32 dataSize,
                                BSONObj &objRecord,
                                dmsSchemaContext *pContext = NULL,
                                BOOLEAN getPrimalData = FALSE ) ;

         // Rebuild a record which is not encoded by the internal schema. Possible actions
         // including:
         // 1. Change old names to new names.
         // 2. Removed fields which have been deleted in the schema.
         // 3. Add new fields with read default values which are not in the record according to the
         //    internal schema.
         INT32    rebuildRecord( _pmdEDUCB *cb,
                                 const BSONObj &record,
                                 BSONObj &outRecord,
                                 dmsSchemaContext *pContext = NULL,
                                 BOOLEAN getPrimalData = FALSE ) ;

         // Dump the internal schema information. All columns are included.
         INT32    dumpSchemaInfo( BSONObj &schema, BOOLEAN includeColumnID = TRUE ) ;

         INT32    toObj( BSONObj &obj ) ;
         INT32    toSchemaObj( const CHAR *name,
                               BSONObj &boSchema ) ;

         BOOLEAN  testReadDefault( const SET_CHARSTRING &setNames ) ;
         BOOLEAN  testWriteDefault( const SET_CHARSTRING &setNames ) ;
         BOOLEAN  testReadDefault( const CHAR *pName ) ;
         BOOLEAN  testWriteDefault( const CHAR *pName ) ;

         const dmsSchemaContainer* getSchemaContainer() const
         {
            return &_schemaContainer ;
         }

         const dmsSchemaHash* getSchemaHashTable() const
         {
            return &_schemaHash ;
         }

         ossRWMutex*          getRWMutex() ;

      private:

         INT32    _init( const dmsSchemaExtent *schemaExtent,
                         UINT32 schemaExtentSize,
                         const dmsSchemaHashExtent *hashExtent,
                         UINT32 hashExtentSize,
                         UINT16 mbID,
                         BOOLEAN isReload ) ;

         void     _reset() ;

         INT32    _parseRecord( utilBSONRawBuilder &encodeBuilder,
                                _utilBitmapBase &writeBitmap,
                                _utilBitmapBase &allBitmap,
                                const BSONObj &record,
                                BOOLEAN &hasNewCol ) ;

         // Append primal columns which do not exist in the original record.
         INT32    _appendPrimalColumns( _pmdEDUCB *cb,
                                        const BSONObj& originalRecord,
                                        const _utilBitmapBase &writeBitmap,
                                        _utilBitmapBase &allBitmap,
                                        UINT8 encodeType,
                                        utilBSONRawBuilder &encodeBuilder,
                                        dmsRecordData &recordData,
                                        BOOLEAN &memAlloc ) ;

         INT32    _checkOrgRecord( const BSONObj &record,
                                   _utilBitmapBase &colBitmap,
                                   BOOLEAN &hitName,
                                   BOOLEAN &hitDefault,
                                   BOOLEAN *pHitNew = NULL,
                                   _utilBitmapBase *pAllBitmap = NULL ) ;

         INT32    _rebuildRecord( _pmdEDUCB *cb,
                                  const BSONObj &record,
                                  UINT32 recordVersion,
                                  BSONObj &outRecord,
                                  _utilBitmapBase &readBitmap,
                                  BOOLEAN &hitName,
                                  BOOLEAN hasFound,
                                  BOOLEAN getPrimalData ) ;

         INT32    _appendColWithReadDefault( const _utilBitmapBase &readBitmap,
                                             utilBSONRawBuilder &builder,
                                             UINT32 recordVersion = DMS_SCHEMA_INVALID_VERSION ) ;

         INT32    _postLoad() ;

         void     _logSchemaInfo() ;
         INT32    _dumpSchemaInfo( BSONObj &schema, BOOLEAN includeColumnID = TRUE ) ;

         INT32    _encodeSanityCheck( const dmsRecordData &encodedData,
                                      BOOLEAN isPrimalData ) ;

         void     _clearBitmapInfo() ;

         void     _makeSchemaContextQuery( dmsSchemaContext &context ) ;

         BOOLEAN  _testColumn( const CHAR *pName,
                               const _utilBitmapBase &bitmap ) ;

      private:
         BOOLEAN                _enabled ;
         BOOLEAN                _forceEncode ;
         UINT32                 _schemaVersion ;
         UINT32                 _schemaInnerVersion ;
         dmsSchemaContainer     _schemaContainer ;
         dmsSchemaHash          _schemaHash ;

         dmsSchemaBitmap        _colBitmap ;
         dmsSchemaBitmap        _readColBitmap ;
         dmsSchemaBitmap        _writeColBitmap ;
         NAME_INFO_MAP          _decodeWatchNames ;

         UINT32                 _defaultReadMaxSize ;
         UINT32                 _defaultWriteMaxSize ;
         UINT32                 _totalValidNameSize ;

         BOOLEAN                _hasLoad ;
         ossRWMutex             _loadRWMutex ;
   } ;
   typedef _dmsInternalSchema dmsInternalSchema ;

}

#endif /* DMS_INTERNALSCHEMA_HPP__ */
