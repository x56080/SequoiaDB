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

   Source File Name = dmsSchemaColRecord.hpp

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
#ifndef DMS_SCHEMARECORD_HPP__
#define DMS_SCHEMARECORD_HPP__

#include "oss.hpp"
#include "dms.hpp"
#include "utilBSON.hpp"

namespace engine
{

   /*
      _dmsSchemaColSlot define

      slot struct:
      |  1 Byte  |  3 Byte  |
      |   Attr   |  Offset  |
   */
   class _dmsSchemaColSlot : public SDBObject
   {
      public:
         _dmsSchemaColSlot()
         {
            _data = 0 ;
         }
         ~_dmsSchemaColSlot() {}

         UINT8    getAttr() const
         {
            return (UINT8)( _data >> 24 ) ;
         }
         void     clearAttr()
         {
            _data &= 0x00FFFFFF ;
         }
         void     clearAttrBits( UINT8 attr )
         {
            _data &= ~((UINT32)attr << 24 ) ;
         }
         void     setAttr( UINT8 attr )
         {
            _data = ( _data & 0x00FFFFFF ) | ( (UINT32)attr << 24 ) ;
         }
         void     setAttrBits( UINT8 attr )
         {
            _data |= ( (UINT32)attr << 24 ) ;
         }
         UINT32   getOffset() const
         {
            return _data & 0x00FFFFFF ;
         }
         void     setOffset( UINT32 offset )
         {
            _data = ( _data & 0xFF000000 ) | ( offset & 0x00FFFFFF ) ;
         }

      private:
         UINT32   _data ;
   } ;
   typedef _dmsSchemaColSlot dmsSchemaColSlot ;

   #define DMS_SCHEMAEXTENT_SLOT_SZ       (sizeof(dmsSchemaColSlot))
   #define DMS_SCHEMA_HASH_INVALID_SLOTID (0xFFFF)

   /*
      _dmsSchemaHashSlot define

      slot struct:
      |  2 Byte     |   2 Byte    |
      | Next slotID |   columnID  |
   */
   class _dmsSchemaHashSlot : public SDBObject
   {
      public:
         _dmsSchemaHashSlot()
         {
            _data = 0 ;
         }
         ~_dmsSchemaHashSlot() {}

         UINT16   getNextSlotID() const
         {
            return (UINT16)( _data >> 16 ) ;
         }
         BOOLEAN  hasNextSlot() const
         {
            return getNextSlotID() == (UINT16)DMS_SCHEMA_HASH_INVALID_SLOTID ? FALSE : TRUE ;
         }
         void     setNextSlotID( UINT16 nextSlotID )
         {
            _data = ( _data & 0x0000FFFF ) | ( (UINT32)nextSlotID << 16 ) ;
         }
         UINT16   getColumnID() const
         {
            return _data & 0x0000FFFF ;
         }
         void     setColumnID( UINT16 columnID )
         {
            _data = ( _data & 0xFFFF0000 ) | columnID ;
         }
         void     reset()
         {
            setNextSlotID( DMS_SCHEMA_HASH_INVALID_SLOTID ) ;
            setColumnID( DMS_SCHEMA_INVALID_COLUMNID ) ;
         }

      private:
         UINT32   _data ;
   } ;
   typedef _dmsSchemaHashSlot dmsSchemaHashSlot ;

   #define DMS_HASHEXTENT_SLOT_SZ         (sizeof(dmsSchemaHashSlot))


   /* Column record format is as below. It may contain the following four items:
    *  Name -- Current column name
    *  RD   -- Read Default
    *  WD   -- Write Default
    *  ON   -- Original Name(Generated when a column is renamed for the first time)
    *
    *  The header(first 10 bytes) structure is fixed. Column name will always be there, while other
    *  items may not.
    *  ____________________________________________________________________________________________________________________________________________________
    *  |        |           |         |         |         |         |         |       |                  |       |                    |      |             |
    *  |totalLen|Name offset|RD Offset|WD Offset|ON Offset|nameLen  |   name  |RD type|read default value|WD type| Write default value|ON len|original name|
    *  |________|___________|_________|_________|_________|_________|_________|_______|__________________|_______|____________________|______|_____________|
    *  |<--------Record Header(10 bytes)------->|
   */

   #define COL_NAME_ENTRY_OFFSET           (sizeof(UINT16))
   #define COL_RD_ENTRY_OFFSET             (sizeof(UINT16)*2)
   #define COL_WD_ENTRY_OFFSET             (sizeof(UINT16)*3)
   #define COL_ON_ENTRY_OFFSET             (sizeof(UINT16)*4)

   class _dmsSchemaColRecord : public SDBObject
   {
      public:
         OSS_INLINE UINT16         getLength() const ;
         OSS_INLINE const CHAR*    getName( INT32 *size = NULL ) const ;
         OSS_INLINE const CHAR*    getOrigName( INT32 *size = NULL ) const ;
         OSS_INLINE const BOOLEAN  getDefault( BSONType &type, INT32 &size, const CHAR *&value,
                                               BOOLEAN readDefault = TRUE ) const ;

         void setLength( UINT16 length ) ;
         void setNameOffset( UINT16 offset ) ;
         void setReadDefaultOffset( UINT16 offset ) ;
         void setWriteDefaultOffset( UINT16 offset ) ;
         void setOrigNameOffset( UINT16 offset ) ;

      private:
         UINT16      _totalLen ;
         UINT16      _nameOffset ;
         UINT16      _reaDefaultOffset ;
         UINT16      _writeDefaultOffset ;
         UINT16      _orignalNameOffset ;
   } ;
   typedef _dmsSchemaColRecord dmsSchemaColRecord ;
   #define DMS_SCHEMA_COLREC_HEAD_SZ      sizeof(_dmsSchemaColRecord)

   OSS_INLINE UINT16 _dmsSchemaColRecord::getLength() const
   {
      return _totalLen ;
   }

   OSS_INLINE const CHAR* _dmsSchemaColRecord::getName( INT32 *size ) const
   {
      UINT16 nameLenOffset = *(UINT16 *)((const CHAR *)this + COL_NAME_ENTRY_OFFSET) ;
      if ( 0 == nameLenOffset )
      {
         return NULL ;
      }
      else
      {
         if ( size )
         {
            *size = (INT32)(*(UINT16 *)((const CHAR *)this + nameLenOffset)) ;
         }
         return (const CHAR *)this + nameLenOffset + sizeof(UINT16) ;
      }
   }

   OSS_INLINE const CHAR* _dmsSchemaColRecord::getOrigName( INT32 *size ) const
   {
      UINT16 nameLenOffset = *(UINT16 *)((const CHAR *)this + COL_ON_ENTRY_OFFSET ) ;
      if ( 0 == nameLenOffset )
      {
         return NULL ;
      }
      else
      {
         if ( size )
         {
            *size = (INT32)(*(UINT16 *)((const CHAR *)this + nameLenOffset)) ;
         }
         return (const CHAR *)this + nameLenOffset + sizeof(UINT16) ;
      }
   }

   OSS_INLINE const BOOLEAN _dmsSchemaColRecord::getDefault( BSONType &type, INT32 &size,
                                                             const CHAR *&value,
                                                             BOOLEAN readDefault ) const
   {
      BOOLEAN hasDefault = FALSE ;
      UINT16 typeOffset = *(UINT16 *)((const CHAR *)this +
         ( readDefault ? COL_RD_ENTRY_OFFSET : COL_WD_ENTRY_OFFSET ) ) ;

      size = 0 ;
      value = NULL ;

      if ( 0 != typeOffset )
      {
         hasDefault = TRUE ;
         type = (BSONType)(*(CHAR *)((const CHAR *)this + typeOffset)) ;
         if ( !utilBSONRawBuilder::emptyValType( type ) )
         {
            value = (const CHAR *)this + typeOffset + 1 ;
            size = utilBSONRawBuilder::getEleValSize( type, value ) ;
         }
      }

      return hasDefault ;
   }

   class _dmsSchemaColRecBuilder : public utilPooledObject
   {
      public:
         _dmsSchemaColRecBuilder() ;
         ~_dmsSchemaColRecBuilder() ;

         // Build a column item from scratch.
         INT32 startBuild() ;
         void finishBuild() ;

         // Build a new column record based on an existing record. We can keep, modify or delete
         // the columns which exist in the original record.
         INT32 startRebuild( const dmsSchemaColRecord *origRecord ) ;
         INT32 finishRebuild() ;

         INT32 addColumnName( const CHAR *name, BOOLEAN isOrigName = FALSE ) ;
         INT32 addDefault( BSONType type, const CHAR *value, INT32 valueSize,
                           BOOLEAN writeDefault = FALSE ) ;

         INT32 updateName( const CHAR *newName ) ;
         INT32 updateReadDefault( BSONType type, const CHAR *data, INT32 valueSize ) ;
         INT32 updateWriteDefault( BSONType type, const CHAR *data, INT32 valueSize ) ;
         INT32 dropReadDefault() ;
         INT32 dropWriteDefault() ;

         const dmsSchemaColRecord *getRecord() const ;

      private:
         INT32    _extendBuff( INT32 size ) ;

         CHAR *   _offset2Ptr( INT32 offset )
         {
            return ((CHAR *)_recordBuff + offset) ;
         }

         void     _incWriteOffset( INT32 inc )
         {
            _writeOffset += inc ;
         }


      private:
         dmsSchemaColRecord           *_recordBuff ;
         const dmsSchemaColRecord     *_origRecord ;     // Used when rebuilding record.
         INT32                         _buffSize ;
         INT32                         _writeOffset ;
         BOOLEAN                       _isRebuild ;
         BOOLEAN                       _finish ;
         CHAR                          _rebuildMask ;
   } ;
   typedef _dmsSchemaColRecBuilder dmsSchemaColRecBuilder ;
}

#endif /* DMS_SCHEMARECORD_HPP__ */
