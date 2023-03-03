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

         void  setColumnAttr( UINT16 columnID, UINT8 attr, BOOLEAN replace ) ;

         void  unsetColumnAttr( UINT16 columnID, UINT8 attr ) ;

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

         INT32 dropColumn( const CHAR *columnName, BOOLEAN *colNotFound ) ;

         INT32 renameColumn( const CHAR *oldName, const CHAR *newName, BOOLEAN *colNotFound ) ;

         /**
          * Drop default value of a column. Only the write default can be dropped.
         */
         INT32 dropColumnDefault( const CHAR *name ) ;

         INT32 alterColumn( const CHAR *columnName, const BSONObj &columnDef,
                            BOOLEAN *colNotFound ) ;

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
