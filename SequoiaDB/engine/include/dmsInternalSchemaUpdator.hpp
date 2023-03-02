#ifndef DMS_INTSCHEMA_UPDATOR_HPP__
#define DMS_INTSCHEMA_UPDATOR_HPP__

#include "dmsInternalSchema.hpp"

namespace engine
{
   class _dmsStorageDataCommon ;
   class _dmsMBContext ;

   class _dmsSchemaWriter : public _dmsSchemaContainer
   {
      public:
         _dmsSchemaWriter() ;
         ~_dmsSchemaWriter() ;

         INT32 init( dmsSchemaExtent *extent, UINT32 extentSize,
                     UINT16 mbID, BOOLEAN create = FALSE ) ;

         INT32 addColumn( const CHAR *name, const BSONObj *columnDef, UINT16 &columnID,
                          const CHAR *origName = NULL ) ;

         INT32 alterColumn( UINT16 columnID, const BSONObj &columnInfo ) ;
         INT32 dropColumn( UINT16 columnID ) ;

         INT32 dropColumnDefault( UINT16 columnID, BOOLEAN dropWrite, BOOLEAN dropRead = FALSE ) ;

         INT32 renameColumn( UINT16 columnID, const CHAR *newName ) ;

         void  setIndexColumn( UINT16 columnID ) ;

         void  unsetIndexColumn( UINT16 columnID ) ;

         const dmsSchemaExtent *getExtent( UINT32 *size ) const
         {
            return _extent ;
         }

         UINT32 getExtentSize() const
         {
            return _extentSize ;
         }

      private:

         INT32    _allocSpace4ColRecord( UINT16 slotSize, UINT16 vaueSize, UINT16 &offset,
                                         UINT16 *allocSize = NULL ) ;

         INT32    _updateColRecord( UINT16 columnID, const dmsSchemaColRecord *oldRecord,
                                    const dmsSchemaColRecord *newRecord ) ;

         void     _setColRecordOffset( UINT16 columnID, UINT16 offset ) ;

         void     _initColAttrAndRecordOffset( UINT16 columnID, INT16 attr, UINT16 valOffset ) ;

         UINT32   _freeSpace() const ;

         INT32    _compact() ;

         OSS_INLINE CHAR *_offset2Ptr( UINT32 offset ) const
         {
            return (CHAR *)_extent + offset ;
         }

         void     _setColumnAttr( UINT16 columnID, UINT8 attr ) {}
         void     _clearColumnAttr( UINT16 columnID, UINT8 attr ) {}

      private:
         dmsSchemaExtent        *_extent ;
   } ;
   typedef _dmsSchemaWriter dmsSchemaWriter ;

   class _dmsSchemaHashWriter : public _dmsSchemaHash
   {
      public:
         _dmsSchemaHashWriter() ;
         ~_dmsSchemaHashWriter() ;

         INT32 init( dmsSchemaWriter *schemaWriter, dmsSchemaHashExtent *extent, UINT32 extentSize,
                     UINT16 mbID, BOOLEAN create = FALSE ) ;

         INT32  addColumnItem( const CHAR *name, UINT16 columnID ) ;
         INT32  dropColumnItemByName( const CHAR *name ) ;

         const dmsSchemaHashExtent *getExtent( UINT32 *size ) const
         {
            return _extent ;
         }

         UINT32 getExtentSize() const
         {
            return _extentSize ;
         }

      private:
         void   _setColumnIDInItem( INT32 *item, UINT16 columnID ) ;
         void   _setNextItemOffset( INT32 *item, UINT16 id ) ;
         void   _resetItem( INT32 *item ) ;

         UINT16 _getColumnIDByItem( INT32 *item ) { return 0 ; }
         UINT16 _getNextItemOffset( INT32 *item ) { return 0 ; }

      private:
         dmsSchemaHashExtent    *_extent ;
         dmsSchemaWriter        *_schemaWriter ;
   } ;
   typedef _dmsSchemaHashWriter dmsSchemaHashWriter ;

   class _dmsInternalSchemaWriter : public dmsInternalSchema
   {
      public:
         _dmsInternalSchemaWriter() ;
         ~_dmsInternalSchemaWriter() ;

         INT32 init( _dmsStorageDataCommon *su, _dmsMBContext *context,
                     dmsSchemaExtent *schemaExtent, dmsSchemaHashExtent *hashExtent,
                     BOOLEAN create = FALSE ) ;

         INT32 addColumn( const CHAR *columnName, const BSONObj *columnDef = NULL,
                          UINT16 *columnID = NULL,
                          BOOLEAN mergeOnExist = FALSE, const CHAR *origName = NULL ) ;

         INT32 dropColumn( const CHAR *columnName ) ;

         INT32 renameColumn( const CHAR *oldName, const CHAR *newName, BOOLEAN *oldColFound ) ;

         /**
          * Drop default value of a column. Only the write default can be dropped.
         */
         INT32 dropColumnDefault( const CHAR *name ) ;

         INT32 alterColumn( const CHAR *columnName,
                            const BSONObj &columnDef ) ;

         INT32 setIndexColumn( const CHAR *columnName ) ;

         INT32 unsetIndexColumn( const CHAR *columnName ) ;

         INT32 updateSchemaByRecord( const BSONObj &record ) ;

         INT32 save( _dmsMBContext *context ) ;

      private:
         INT32 _merge2Column( UINT16 columnID, const BSONObj &columnDef ) ;

      private:
         dmsSchemaExtent        *_origSchemaExtent ;
         dmsSchemaHashExtent    *_origHashExtent ;
         dmsSchemaExtent        *_newSchemaExtent ;
         dmsSchemaHashExtent    *_newHashExtent ;
         dmsSchemaWriter         _schemaWriter ;
         dmsSchemaHashWriter     _schemaHashWriter ;
   } ;
   typedef _dmsInternalSchemaWriter dmsInternalSchemaWriter ;
}

#endif /* DMS_INTSCHEMA_UPDATOR_HPP__ */
