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

   Source File Name = dmsInternalSchemaUpdator.hpp

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

#ifndef DMS_INTSCHEMA_UPDATOR_HPP__
#define DMS_INTSCHEMA_UPDATOR_HPP__

#include "dmsInternalSchema.hpp"
#include "utilSchema.hpp"

namespace engine
{
   class _dmsStorageDataCommon ;
   class _dmsMBContext ;

   class _dmsSchemaColInfoMem : public SDBObject
   {
      public:
         _dmsSchemaColInfoMem() ;
         ~_dmsSchemaColInfoMem() ;

         INT32 setData( UINT8 attr, const dmsSchemaColRecord *record ) ;

         UINT8 getAttr() const
         {
            return _attr ;
         }

         const dmsSchemaColRecord* getColRecord() const
         {
            return _colRecord ;
         }

      private:
         dmsSchemaColRecord           *_colRecord ;
         UINT8                         _attr ;
   } ;
   typedef _dmsSchemaColInfoMem dmsSchemaColInfoMem ;


   class _dmsSchemaWriter : public SDBObject
   {
      typedef ossPoolMap< UINT16, dmsSchemaColInfoMem >           COL_INFO_MAP ;
      typedef COL_INFO_MAP::iterator                              COL_INFO_MAP_ITR ;
      typedef COL_INFO_MAP::const_iterator                        COL_INFO_MAP_CITR ;

      public:
         _dmsSchemaWriter() ;
         ~_dmsSchemaWriter() ;

         INT32 init( const dmsSchemaExtent *baseSchemaExt, UINT32 extentSize, UINT16 mbID ) ;

         INT32 addColumn( const CHAR *name, const BSONObj *columnDef, UINT16 &columnID ) ;

         INT32 alterColumn( UINT16 columnID, const BSONObj &columnInfo ) ;

         INT32 dropColumn( UINT16 columnID ) ;

         INT32 dropColumnDefault( UINT16 columnID, BOOLEAN dropWrite, BOOLEAN dropRead = FALSE ) ;

         INT32 renameColumn( UINT16 columnID, const CHAR *newName ) ;

         INT32 setIndexColumn( UINT16 columnID ) ;

         INT32 unsetIndexColumn( UINT16 columnID ) ;

         INT32 save( dmsSchemaExtent *extent, UINT32 extentSize, UINT16 pageNum,
                     UINT16 mbID, BOOLEAN isInnerChange = FALSE ) ;

         const dmsSchemaContainer* getBaseSchemaContainer() const
         {
            return _hasBaseSchema ? &_baseSchemaContainer : NULL ;
         }

         // Total size of all column info.
         UINT32 totalSize() const
         {
            return _totalSize ;
         }

         BOOLEAN  hasChanged() const
         {
            return _hasChanged ;
         }

      private:
         INT32    _getColAttrAndRecord( UINT16 columnID, UINT8 &attr,
                                        const dmsSchemaColRecord *&record,
                                        BOOLEAN &inMemory ) ;

         INT32    _addNewColumnInfo( UINT16 columnID, UINT8 attr,
                                     const dmsSchemaColRecord *record,
                                     BOOLEAN replace ) ;

         void     _removeColumnInfo( UINT16 columnID ) ;

         void     _updateTotalSize( const dmsSchemaColRecord *newRecord,
                                    const dmsSchemaColRecord *oldRecord ) ;

      private:
         dmsSchemaContainer      _baseSchemaContainer ;
         COL_INFO_MAP            _colInfoMap ;
         UINT32                  _totalSize ;
         BOOLEAN                 _hasBaseSchema ;
         BOOLEAN                 _hasChanged ;
         UINT16                  _nextColumnID ;
   } ;
   typedef _dmsSchemaWriter dmsSchemaWriter ;

   class _dmsSchemaHashWriter : public SDBObject
   {
      typedef ossPoolMap< ossPoolString, UINT16 >              COL_NAME_ID_MAP ;
      typedef COL_NAME_ID_MAP::iterator                        COL_NAME_ID_MAP_ITR ;
      typedef COL_NAME_ID_MAP::const_iterator                  COL_NAME_ID_MAP_CITR ;

      public:
         _dmsSchemaHashWriter() ;
         ~_dmsSchemaHashWriter() ;

         INT32 init( const dmsSchemaContainer *schemaContainer ) ;

         UINT16 getColumnIDByName( const CHAR *name ) const ;

         INT32 addColumnItem( const CHAR *name, UINT16 columnID ) ;

         void dropColumnItemByName( const CHAR *name ) ;

         INT32  save( dmsSchemaHashExtent *extent, UINT32 extentSize,
                      UINT16 pageNums, UINT16 mdID ) ;

         UINT32 totalSize() const
         {
            return _nameIDMap.size() * DMS_HASHEXTENT_SLOT_SZ ;
         }

      private:
         INT32 _prepare4Save( dmsSchemaHashExtent *extent, UINT32 extentSize,
                              UINT16 numPages, UINT16 mbID ) ;
         INT32 _addColumnItem( const CHAR *name, UINT16 columnID ) ;

         dmsSchemaHashSlot* _getHashBucketSlot( UINT32 slotID ) const
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

      private:
         COL_NAME_ID_MAP            _nameIDMap ;
         UINT32                     _maxListSlotNum ;
         dmsSchemaHashExtent       *_extent ;
         UINT32                     _bucketNum ;
         dmsSchemaHashSlot         *_pBucketSlot ;
         dmsSchemaHashSlot         *_pListSlot ;
   } ;
   typedef _dmsSchemaHashWriter dmsSchemaHashWriter ;

   /*
      _dmsInternalSchemaWriter define
   */
   class _dmsInternalSchemaWriter : public dmsInternalSchema
   {
      public:
         _dmsInternalSchemaWriter( BOOLEAN createNew = FALSE ) ;
         virtual ~_dmsInternalSchemaWriter() ;

         INT32 init( const dmsInternalSchema *pSchema, _dmsStorageDataCommon *su,
                     _dmsMBContext *context, _pmdEDUCB *cb ) ;

         INT32 save( _dmsMBContext *context, _pmdEDUCB *cb, BOOLEAN &hasChanged ) ;

         INT32 addColumns( const utilSchema &schema, _pmdEDUCB *cb,
                           const ossPoolSet<ossPoolString> *pSetIdxFields = NULL ) ;

         INT32 addColumn( const CHAR *columnName, const BSONObj *columnDef = NULL,
                          UINT16 *columnID = NULL, BOOLEAN mergeOnExist = FALSE ) ;

         INT32 dropColumn( const CHAR *columnName ) ;

         INT32 renameColumn( const CHAR *oldName, const CHAR *newName ) ;

         /**
          * Drop default value of a column. Only the write default can be dropped.
         */
         INT32 dropColumnDefault( const CHAR *name ) ;

         INT32 alterColumn( const CHAR *columnName, const BSONObj &columnDef ) ;

         INT32 setIndexColumn( const CHAR *columnName ) ;

         INT32 unsetIndexColumn( const CHAR *columnName ) ;

         INT32 updateSchemaByRecord( const BSONObj &record ) ;

      private:
         INT32 _merge2Column( UINT16 columnID, const BSONObj &columnDef ) ;

      private:
         _dmsStorageDataCommon        *_su ;
         dmsSchemaWriter               _schemaWriter ;
         dmsSchemaHashWriter           _schemaHashWriter ;
         BOOLEAN                       _createNew ;
         BOOLEAN                       _isInnerChange ;
   } ;
   typedef _dmsInternalSchemaWriter dmsInternalSchemaWriter ;
}

#endif /* DMS_INTSCHEMA_UPDATOR_HPP__ */
