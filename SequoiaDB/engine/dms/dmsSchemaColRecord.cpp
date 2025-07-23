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

   Source File Name = dmsSchemaColRecord.cpp

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
#include "dmsSchemaColRecord.hpp"
#include "dmsExtent.hpp"

#define SCHEMA_COLREC_MASK_NAME              0x01
#define SCHEMA_COLREC_MASK_RD_DEFAULT        0x02
#define SCHEMA_COLREC_MASK_WD_DEFAULT        0x04
#define SCHEMA_COLREC_MASK_ON                0x08
#define SCHEMA_COLREC_MASK_ALL               (SCHEMA_COLREC_MASK_NAME |          \
                                              SCHEMA_COLREC_MASK_RD_DEFAULT |    \
                                              SCHEMA_COLREC_MASK_WD_DEFAULT |    \
                                              SCHEMA_COLREC_MASK_ON)

#define SCHEMA_COLREC_BUILD_INIT_BUFFSZ      (128)

namespace engine
{
   /*
      _dmsSchemaColAssist implement
   */
   class _dmsSchemaColAssist
   {
      public:
         _dmsSchemaColAssist()
         {
            SDB_ASSERT( DMS_SCHEMAEXTENT_SLOT_SZ == 8, "Slot size invalid" ) ;
            SDB_ASSERT( DMS_HASHEXTENT_SLOT_SZ == 4, "Hash slot size invalid" ) ;

            SDB_ASSERT( ossIsPowerOf2( DMS_SCHEMA_HASH_BUCKET_SIZE, NULL ),
                        "Schema hash bucket must power of 2" ) ;
         }
   } ;
   _dmsSchemaColAssist  __tmpColValidateAssist ;

   /*
      _dmsSchemaColRecord implement
   */
   void _dmsSchemaColRecord::setLength( UINT16 length )
   {
      *(UINT16 *)this = length ;
   }

   void _dmsSchemaColRecord::setNameOffset( UINT16 offset )
   {
      *(UINT16 *)((const CHAR *)this + COL_NAME_ENTRY_OFFSET) = offset ;
   }

   void _dmsSchemaColRecord::setReadDefaultOffset( UINT16 offset )
   {
      *(UINT16 *)((const CHAR *)this + COL_RD_ENTRY_OFFSET) = offset ;
   }

   void _dmsSchemaColRecord::setWriteDefaultOffset( UINT16 offset )
   {
      *(UINT16 *)((const CHAR *)this + COL_WD_ENTRY_OFFSET) = offset ;
   }

   void _dmsSchemaColRecord::setOrigNameOffset( UINT16 offset )
   {
      *(UINT16 *)((const CHAR *)this + COL_ON_ENTRY_OFFSET) = offset ;
   }

   _dmsSchemaColRecBuilder::_dmsSchemaColRecBuilder()
   : _recordBuff( NULL ),
     _origRecord( NULL ),
     _buffSize( 0 ),
     _writeOffset( 0 ),
     _isRebuild( FALSE ),
     _finish( FALSE ),
     _rebuildMask( SCHEMA_COLREC_MASK_ALL )
   {
   }

   _dmsSchemaColRecBuilder::~_dmsSchemaColRecBuilder()
   {
      if ( _recordBuff )
      {
         SDB_OSS_FREE( _recordBuff ) ;
      }
   }

   INT32 _dmsSchemaColRecBuilder::startBuild()
   {
      INT32 rc = SDB_OK ;

      if ( _buffSize < SCHEMA_COLREC_BUILD_INIT_BUFFSZ )
      {
         rc = _extendBuff( SCHEMA_COLREC_BUILD_INIT_BUFFSZ ) ;
         PD_RC_CHECK( rc, PDERROR, "Allocate buffer of size %d for building internal schema column "
                      "info failed, rc: %d", SCHEMA_COLREC_BUILD_INIT_BUFFSZ, rc ) ;

         _buffSize = SCHEMA_COLREC_BUILD_INIT_BUFFSZ ;
      }

      ossMemset( _recordBuff, 0, _buffSize ) ;
      _writeOffset = DMS_SCHEMA_COLREC_HEAD_SZ ;      // Write after the record header
      _isRebuild = FALSE ;
      _rebuildMask = SCHEMA_COLREC_MASK_ALL ;

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _dmsSchemaColRecBuilder::addColumnName( const CHAR *name, BOOLEAN isOrigName )
   {
      INT32 rc = SDB_OK ;
      INT32 offset = _writeOffset ;

      SDB_ASSERT( name, "Name is null" ) ;

      INT32 len = ossStrlen( name ) + 1 ; // 1 byte for the terminating null.

      if ( _buffSize - _writeOffset < len + 2 )    // 2 bytes for the length of the name.
      {
         INT32 newSize = _writeOffset + len + SCHEMA_COLREC_BUILD_INIT_BUFFSZ ;
         rc = _extendBuff( newSize ) ;
         PD_RC_CHECK( rc, PDERROR, "Extend buffer to size %d for building internal schema column "
                      "info failed, rc: %d", newSize, rc ) ;
      }

      // Write the length of the name.
      *(UINT16 *)_offset2Ptr( _writeOffset ) = len - 1 ; // The terminating null not included.
      _incWriteOffset( sizeof(UINT16) ) ;

      // Write the name, including the terminating null.
      ossMemcpy( _offset2Ptr(_writeOffset), name, len ) ;
      _incWriteOffset( len ) ;

      // Write the offset in the header.
      if ( isOrigName )
      {
         _recordBuff->setOrigNameOffset( offset ) ;
         OSS_BIT_CLEAR( _rebuildMask, SCHEMA_COLREC_MASK_ON ) ;
      }
      else
      {
         _recordBuff->setNameOffset( offset ) ;
         OSS_BIT_CLEAR( _rebuildMask, SCHEMA_COLREC_MASK_NAME ) ;
      }

      SDB_ASSERT( _writeOffset <= _buffSize, "Write out of bound" ) ;

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _dmsSchemaColRecBuilder::addDefault( BSONType type, const CHAR *value, INT32 valueSize,
                                              BOOLEAN writeDefault )
   {
      INT32 rc = SDB_OK ;
      INT32 offset = _writeOffset ;

#ifdef _DEBUG
      switch ( type )
      {
        case EOO:
        case Undefined:
        case jstNULL:
        case MaxKey:
        case MinKey:
            SDB_ASSERT( 0 == valueSize, "Default value size is wrong" ) ;
            break;
         default:
            SDB_ASSERT( value && valueSize > 0 , "Default value is invalid" ) ;
      }
#endif /* _DEBUG */

      if ( _buffSize - _writeOffset < valueSize + 1 )    // 1 byte for the BSONType
      {
         INT32 newSize = _writeOffset + valueSize + SCHEMA_COLREC_BUILD_INIT_BUFFSZ ;
         rc = _extendBuff( newSize ) ;
         PD_RC_CHECK( rc, PDERROR, "Extend buffer to size %d for building internal schema column "
                      "info failed, rc: %d", newSize, rc ) ;
      }

      *(CHAR *)_offset2Ptr(_writeOffset) = (CHAR)type ;
      _incWriteOffset( 1 ) ;
      if ( valueSize > 0 )
      {
         ossMemcpy( _offset2Ptr(_writeOffset), value, valueSize ) ;
         _incWriteOffset( valueSize ) ;
      }

      if ( writeDefault )
      {
         _recordBuff->setWriteDefaultOffset( offset ) ;
         OSS_BIT_CLEAR( _rebuildMask, SCHEMA_COLREC_MASK_WD_DEFAULT ) ;
      }
      else
      {
         _recordBuff->setReadDefaultOffset( offset ) ;
         OSS_BIT_CLEAR( _rebuildMask, SCHEMA_COLREC_MASK_RD_DEFAULT ) ;
      }

      SDB_ASSERT( _writeOffset <= _buffSize, "Write out of bound" ) ;

   done:
      return rc ;
   error:
      goto done ;
   }

   void _dmsSchemaColRecBuilder::finishBuild()
   {
      if ( _recordBuff && (UINT32)_writeOffset > DMS_SCHEMA_COLREC_HEAD_SZ )
      {
         _recordBuff->setLength( _writeOffset ) ;
      }
      _finish = TRUE ;
   }

   INT32 _dmsSchemaColRecBuilder::startRebuild( const dmsSchemaColRecord *origRecord )
   {
      INT32 rc = SDB_OK ;
      INT32 len = origRecord->getLength() ;

      if ( _buffSize < len )
      {
         rc = _extendBuff( len ) ;
         PD_RC_CHECK( rc, PDERROR, "Allocate buffer of size %d for rebuilding internal schema "
                      "column info failed, rc: %d", len, rc ) ;

         _buffSize = len ;
      }

      ossMemset( _recordBuff, 0, _buffSize ) ;
      _origRecord = origRecord ;
      _writeOffset = DMS_SCHEMA_COLREC_HEAD_SZ ;      // Write after the record header
      _isRebuild = TRUE ;

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _dmsSchemaColRecBuilder::updateName( const CHAR *newName )
   {
      INT32 rc = SDB_OK ;

      // Check if it has original name. If yes, just change the current name. If not, need to store
      // the current name as the original name, and store the new name as the current name.
      const CHAR *currName = _origRecord->getName() ;
      const CHAR *origName = _origRecord->getOrigName() ;

      rc = addColumnName( newName ) ;
      PD_RC_CHECK( rc, PDERROR, "Add new column name %s into column info record in buffer failed, "
                   "rc: %d", newName, rc ) ;

      OSS_BIT_CLEAR( _rebuildMask, SCHEMA_COLREC_MASK_NAME ) ;

      if ( !origName )
      {
         // If currently no original name, the current name should set as original name.
         origName = currName ;
         rc = addColumnName( origName, TRUE ) ;
         PD_RC_CHECK( rc, PDERROR, "Add original column name %s into column info record in buffer "
                      "failed, rc: %d", origName, rc ) ;
         OSS_BIT_CLEAR( _rebuildMask, SCHEMA_COLREC_MASK_ON ) ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _dmsSchemaColRecBuilder::updateReadDefault( BSONType type, const CHAR *data,
                                                     INT32 valueSize )
   {
      INT32 rc = SDB_OK ;

      rc = addDefault( type, data, valueSize ) ;
      PD_RC_CHECK( rc, PDERROR, "Add read default for internal schema column in buffer failed, "
                   "rc: %d", rc ) ;

      OSS_BIT_CLEAR( _rebuildMask, SCHEMA_COLREC_MASK_RD_DEFAULT ) ;

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _dmsSchemaColRecBuilder::updateWriteDefault( BSONType type, const CHAR *data,
                                                      INT32 valueSize )
   {
      INT32 rc = SDB_OK ;

      rc = addDefault( type, data, valueSize, TRUE ) ;
      PD_RC_CHECK( rc, PDERROR, "Add write default for internal schema column in buffer failed, "
                   "rc: %d", rc ) ;

      OSS_BIT_CLEAR( _rebuildMask, SCHEMA_COLREC_MASK_WD_DEFAULT ) ;

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _dmsSchemaColRecBuilder::dropReadDefault()
   {
      OSS_BIT_CLEAR( _rebuildMask, SCHEMA_COLREC_MASK_RD_DEFAULT ) ;
      return SDB_OK ;
   }

   INT32 _dmsSchemaColRecBuilder::dropWriteDefault()
   {
      OSS_BIT_CLEAR( _rebuildMask, SCHEMA_COLREC_MASK_WD_DEFAULT ) ;
      return SDB_OK ;
   }

   INT32 _dmsSchemaColRecBuilder::finishRebuild ()
   {
      INT32 rc = SDB_OK ;
      const CHAR *value = NULL ;
      INT32 size = 0 ;
      BOOLEAN foundDefault = FALSE ;

      // Append remaining items.
      if ( OSS_BIT_TEST( _rebuildMask, SCHEMA_COLREC_MASK_NAME ) )
      {
         value = _origRecord->getName() ;
         SDB_ASSERT( value, "Column name in original col record is invalid" ) ;
         rc = addColumnName( value ) ;
         PD_RC_CHECK( rc, PDERROR, "Add column name %s into new column info record in buffer "
                      "failed, rc: %d", value, rc ) ;
      }

      if ( OSS_BIT_TEST( _rebuildMask, SCHEMA_COLREC_MASK_RD_DEFAULT ) )
      {
         BSONType type ;
         foundDefault = _origRecord->getDefault( type, size, value ) ;
         if ( foundDefault )
         {
            rc = addDefault( type, value, size ) ;
            PD_RC_CHECK( rc, PDERROR, "Add read default value for internal schema column in buffer "
                         "failed, rc: %d", rc ) ;
         }
      }

      if ( OSS_BIT_TEST( _rebuildMask, SCHEMA_COLREC_MASK_WD_DEFAULT ) )
      {
         BSONType type ;
         foundDefault = _origRecord->getDefault( type, size, value, FALSE ) ;
         if ( foundDefault )
         {
            rc = addDefault( type, value, size, TRUE ) ;
            PD_RC_CHECK( rc, PDERROR, "Add write default value for internal schema column in buffer "
                         "failed, rc: %d", rc ) ;
         }
      }

      if ( OSS_BIT_TEST( _rebuildMask, SCHEMA_COLREC_MASK_ON ) )
      {
         value = _origRecord->getOrigName() ;
         if ( value )
         {
            rc = addColumnName( value, TRUE ) ;
            PD_RC_CHECK( rc, PDERROR, "Add original name for internal schema column in buffer "
                         "failed, rc: %d", rc ) ;
         }
      }

      _recordBuff->setLength( _writeOffset ) ;

      _finish = TRUE ;

   done:
      return rc ;
   error:
      goto done ;
   }

   const dmsSchemaColRecord *_dmsSchemaColRecBuilder::getRecord() const
   {
      if ( _recordBuff->getLength() > 0 )
      {
         return _recordBuff ;
      }
      else
      {
         return NULL ;
      }
   }

   INT32 _dmsSchemaColRecBuilder::_extendBuff( INT32 size )
   {
      INT32 rc = SDB_OK ;

      if ( size > _buffSize )
      {
         dmsSchemaColRecord *newBuff = (dmsSchemaColRecord *)SDB_OSS_REALLOC( _recordBuff, size ) ;
         if ( !newBuff )
         {
            rc = SDB_OOM ;
            goto error ;
         }
         if ( newBuff != _recordBuff )
         {
            _recordBuff = newBuff ;
         }
         _buffSize = size ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }
}
