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

   Source File Name = ixmUtil.cpp

   Descriptive Name =

   When/how to use: this program may be used on binary and text-formatted
   versions of PMD component. This file contains common function of ixm
   component.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who      Description
   ====== =========== ======== ==============================================
          2019/09/20  Ting YU  Initial Draft

   Last Changed =

*******************************************************************************/
#include "ixm.hpp"

namespace engine
{
   static void _appendString( CHAR * pBuffer, INT32 bufSize,
                              const CHAR *flagStr )
   {
      if ( 0 != *pBuffer )
      {
         ossStrncat( pBuffer, " | ", bufSize - ossStrlen( pBuffer ) ) ;
      }
      ossStrncat( pBuffer, flagStr, bufSize - ossStrlen( pBuffer ) ) ;
   }

   BOOLEAN ixmIsTextIndex( const BSONObj& indexDef )
   {
      UINT16 type = 0 ;
      BOOLEAN isText = FALSE ;

      if ( SDB_OK == ixmGetIndexType( indexDef, type ) )
      {
         if ( OSS_BIT_TEST( type, IXM_EXTENT_TYPE_TEXT ) )
         {
            isText = TRUE ;
         }
      }
      else
      {
         // ignore error, so clear info
         IExecutor *cb = sdbGetThreadExecutor() ;
         if ( cb )
         {
            cb->resetInfo ( EDU_INFO_ERROR ) ;
         }
      }

      return isText ;
   }

   INT32 ixmGetIndexType( const BSONObj& indexDef, UINT16 &type )
   {
      INT32 rc = SDB_OK ;
      UINT16 indexType = IXM_EXTENT_TYPE_NONE ;
      rc = ixmIndexCB::generateIndexType( indexDef, indexType ) ;
      PD_RC_CHECK( rc, PDERROR,
                   "Failed to get index type from definition[%s], rc: %d",
                   indexDef.toString().c_str(), rc ) ;

      if ( IXM_EXTENT_HAS_TYPE( indexType, IXM_EXTENT_TYPE_POSITIVE ) )
      {
         type |= IXM_EXTENT_TYPE_POSITIVE ;
      }
      if ( IXM_EXTENT_HAS_TYPE( indexType, IXM_EXTENT_TYPE_REVERSE ) )
      {
         type |= IXM_EXTENT_TYPE_REVERSE ;
      }
      if ( IXM_EXTENT_HAS_TYPE( indexType, IXM_EXTENT_TYPE_2D ) )
      {
         type |= IXM_EXTENT_TYPE_2D ;
      }
      if ( IXM_EXTENT_HAS_TYPE( indexType, IXM_EXTENT_TYPE_TEXT ) )
      {
         type |= IXM_EXTENT_TYPE_TEXT ;
      }
      if ( IXM_EXTENT_HAS_TYPE( indexType, IXM_EXTENT_TYPE_GLOBAL ) )
      {
         type |= IXM_EXTENT_TYPE_GLOBAL ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   #define IXM_INDEXTYPE_TMP_STR_SZ 63

   ossPoolString ixmGetIndexTypeDesp( UINT16 type )
   {
      CHAR szTmp[ IXM_INDEXTYPE_TMP_STR_SZ + 1 ] = {0} ;
      if ( IXM_EXTENT_TYPE_NONE == type )
      {
         return "None" ;
      }

      if ( IXM_EXTENT_HAS_TYPE( type, IXM_EXTENT_TYPE_POSITIVE ) )
      {
         ossStrncat( szTmp, "Positive", IXM_INDEXTYPE_TMP_STR_SZ ) ;
         OSS_BIT_CLEAR( type, IXM_EXTENT_TYPE_POSITIVE ) ;
      }
      if ( IXM_EXTENT_HAS_TYPE( type, IXM_EXTENT_TYPE_REVERSE ) )
      {
         _appendString( szTmp, IXM_INDEXTYPE_TMP_STR_SZ, "Reverse" ) ;
         OSS_BIT_CLEAR( type, IXM_EXTENT_TYPE_REVERSE ) ;
      }
      if ( IXM_EXTENT_HAS_TYPE( type, IXM_EXTENT_TYPE_2D ) )
      {
         _appendString( szTmp, IXM_INDEXTYPE_TMP_STR_SZ, "2d" ) ;
         OSS_BIT_CLEAR( type, IXM_EXTENT_TYPE_2D ) ;
      }
      if ( IXM_EXTENT_HAS_TYPE( type, IXM_EXTENT_TYPE_TEXT ) )
      {
         _appendString( szTmp, IXM_INDEXTYPE_TMP_STR_SZ, "Text" ) ;
         OSS_BIT_CLEAR( type, IXM_EXTENT_TYPE_TEXT ) ;
      }
      if ( IXM_EXTENT_HAS_TYPE( type, IXM_EXTENT_TYPE_GLOBAL ) )
      {
         _appendString( szTmp, IXM_INDEXTYPE_TMP_STR_SZ, "Global" ) ;
         OSS_BIT_CLEAR( type, IXM_EXTENT_TYPE_GLOBAL ) ;
      }

      if ( type )
      {
         _appendString( szTmp, IXM_INDEXTYPE_TMP_STR_SZ, "Unknown" ) ;
      }

      return szTmp ;
   }

   INT32 ixmBuildExtDataName( UINT64 clUniqID, const CHAR *idxName,
                              CHAR *extName, UINT32 buffSize )
   {
      INT32 rc = SDB_OK ;
      INT32 length = 0 ;

      SDB_ASSERT( idxName, "Index name is NULL") ;
      SDB_ASSERT( extName, "Buffer is empty" ) ;
      SDB_ASSERT( buffSize >= DMS_MAX_EXT_NAME_SIZE + 1, "buffer too small" ) ;

      length = ossSnprintf( extName, DMS_MAX_EXT_NAME_SIZE + 1,
                            SYS_PREFIX"_%llu_", clUniqID ) ;
      if ( length + ossStrlen( idxName ) > DMS_MAX_EXT_NAME_SIZE )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG( PDERROR, "Index name size[%u] too long for external data",
                 ossStrlen( idxName ) ) ;
         goto error ;
      }

      ossSnprintf( extName + length, DMS_MAX_EXT_NAME_SIZE + 1 - length,
                   idxName ) ;

   done:
      return rc ;
   error:
      goto done ;
   }

   BOOLEAN ixmIsSameDef( const BSONObj &defObj1,
                         const BSONObj &defObj2,
                         BOOLEAN strict )
   {
      BOOLEAN rs = TRUE;
      BOOLEAN lValue = FALSE;
      BOOLEAN rValue = FALSE;
      BSONElement lEle ;
      BSONElement rEle ;

      try
      {
         lEle = defObj1.getField( IXM_KEY_FIELD ) ;
         rEle = defObj2.getField( IXM_KEY_FIELD ) ;
         if ( 0 != lEle.woCompare( rEle, false ) )
         {
            rs = FALSE;
            goto done;
         }

         lValue = defObj1.getBoolField( IXM_UNIQUE_FIELD ) ;
         rValue = defObj2.getBoolField( IXM_UNIQUE_FIELD ) ;
         if ( !strict )
         {
            if ( lValue )
            {
               /// it is useless to create any same defined index
               /// when an unique index exists.
               rs = TRUE ;
               goto done ;
            }
            else if ( lValue != rValue )
            {
               rs = FALSE;
               goto done;
            }
            else
            {
               /// do nothing.
            }
         }
         else
         {
            if ( lValue != rValue )
            {
                rs = FALSE ;
                goto done ;
            }
         }

         lValue = defObj1.getBoolField( IXM_ENFORCED_FIELD ) ;
         rValue = defObj2.getBoolField( IXM_ENFORCED_FIELD ) ;
         if ( lValue != rValue )
         {
            rs = FALSE ;
            goto done ;
         }

         lValue = defObj1.getBoolField( IXM_NOTNULL_FIELD ) ;
         rValue = defObj2.getBoolField( IXM_NOTNULL_FIELD ) ;
         if ( lValue != rValue )
         {
            rs = FALSE ;
            goto done ;
         }

         lEle = defObj1.getField( IXM_NAME_FIELD ) ;
         rEle = defObj2.getField( IXM_NAME_FIELD ) ;
         if ( 0 == ossStrcmp( lEle.valuestrsafe(), IXM_ID_KEY_NAME ) )
         {
            lValue = TRUE ;
         }
         else
         {
            lValue = defObj1.getBoolField( IXM_NOTARRAY_FIELD ) ;
         }
         if ( 0 == ossStrcmp( rEle.valuestrsafe(), IXM_ID_KEY_NAME ) )
         {
            rValue = TRUE ;
         }
         else
         {
            rValue = defObj2.getBoolField( IXM_NOTARRAY_FIELD ) ;
         }
         if( lValue != rValue )
         {
            rs = FALSE ;
            goto done ;
         }
      }
      catch( std::exception &e )
      {
         rs = FALSE ;
         PD_LOG( PDERROR, "occur unexpected error(%s)", e.what() ) ;
      }

   done:
      return rs;
   }

   INT32 ixmCheckMappingsFields( const BSONObj &fields, const BSONObj &indexKey )
   {
      INT32 rc = SDB_OK ;

      try
      {
         BSONObjIterator itr( fields ) ;
         while( itr.more() )
         {
            BSONElement ele = itr.next() ;
            const CHAR* fieldName = ele.fieldName() ;
            UINT32 fieldNameLen = ossStrlen( fieldName ) ;
            BOOLEAN existInIndexKey = FALSE ;

            if ( Object != ele.type() )
            {
               rc = SDB_INVALIDARG ;
               PD_LOG_MSG( PDERROR, "The elements in Fileds must be Object" ) ;
               goto error ;
            }

            {
               BSONObjIterator i( indexKey ) ;
               while( i.more() )
               {
                  BSONElement e = i.next() ;
                  UINT32 keyNameLen = ossStrlen( e.fieldName() ) ;

                  if ( 0 == ossStrcmp( fieldName, e.fieldName() ) )
                  {
                     existInIndexKey = TRUE ;
                     break ;
                  }

                  // May be the fieldName is a nested field
                  if (
                       // May be the fieldName = "abc.d", the keyName = "abc.dd"
                       fieldNameLen > keyNameLen &&
                       // May be the fieldName = "abcd.d", the keyName = "abc"
                       0 == ossStrncmp( fieldName, e.fieldName(), keyNameLen ) &&
                       // May be the fieldName = "abcd.d", the keyName = "abcd"
                       '.' == fieldName[keyNameLen]
                     )
                  {
                     existInIndexKey = TRUE ;
                  }
               }
            }

            if ( !existInIndexKey )
            {
               rc = SDB_INVALIDARG ;
               PD_LOG_MSG( PDERROR, "Index keys must contain all fields definded in "
                           "Mappings.Fields. Field \"%s\" doesn't exist in index key, "
                           "rc: %d", fieldName, rc ) ;
               goto error ;
            }

            {
               BSONObjIterator i( ele.embeddedObject() ) ;
               while( i.more() )
               {
                  BSONElement e = i.next() ;
                  if ( 0 == ossStrcmp( e.fieldName(), FIELD_ES_NAME_TYPE ) )
                  {
                     const CHAR* typeStr = NULL ;
                     if ( String != e.type() )
                     {
                        rc = SDB_INVALIDARG ;
                        PD_LOG_MSG( PDERROR, "The Type field in Fields must be string" ) ;
                        goto error ;
                     }
                     typeStr = e.valuestr() ;

                     if ( 0 != ossStrcmp( typeStr, VALUE_ES_TYPE_NAME_TEXT ) &&
                          0 != ossStrcmp( typeStr, VALUE_ES_TYPE_NAME_KEYWORD ) &&
                          0 != ossStrcmp( typeStr, VALUE_ES_TYPE_NAME_WILDCARD ) &&
                          0 != ossStrcmp( typeStr, VALUE_ES_TYPE_NAME_INT ) &&
                          0 != ossStrcmp( typeStr, VALUE_ES_TYPE_NAME_LONG ) &&
                          0 != ossStrcmp( typeStr, VALUE_ES_TYPE_NAME_FLOAT ) &&
                          0 != ossStrcmp( typeStr, VALUE_ES_TYPE_NAME_DOUBLE ) &&
                          0 != ossStrcmp( typeStr, VALUE_ES_TYPE_NAME_DATE ) &&
                          0 != ossStrcmp( typeStr, VALUE_ES_TYPE_NAME_BOOLEAN ) )
                     {
                        rc = SDB_INVALIDARG ;
                        PD_LOG_MSG( PDERROR, "Invalid Type[%s]", typeStr ) ;
                     }
                  }
                  else if ( 0 == ossStrcmp( e.fieldName(), FIELD_ES_NAME_INDEX ) )
                  {
                     if ( Bool != e.type() )
                     {
                        rc = SDB_INVALIDARG ;
                        PD_LOG_MSG( PDERROR, "The Type field in Fields must be bool" ) ;
                        goto error ;
                     }
                  }
                  else
                  {
                     rc = SDB_INVALIDARG ;
                     PD_LOG_MSG( PDERROR, "Unrecognized field[%s] in Fields",
                                 ele.fieldName() ) ;
                     goto error ;
                  }
               }
            }
         }
      }
      catch ( std::exception &e )
      {
         rc = ossException2RC( &e ) ;
         PD_LOG( PDERROR, "An exception occurred when checking fields in fulltext index maapings: "
                 "%s, rc: %d", e.what(), rc ) ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 ixmCheckFulltextIdxMappings( const BSONObj &indexMappings,
                                      const BSONObj &indexKey )
   {
      INT32 rc = SDB_OK ;

      /*

      eg:

      mappings =
      {
         Fields:
         {
            fieldName1: { Type: "keyword" },
            fieldName2: { Index: false },
            ...
         }
      }

      */

      try
      {
         BSONObjIterator itr( indexMappings ) ;
         while( itr.more() )
         {
            BSONElement ele = itr.next() ;
            if ( 0 == ossStrcmp( ele.fieldName(), FIELD_ES_NAME_FIELDS ) )
            {
               if ( Object != ele.type() )
               {
                  rc = SDB_INVALIDARG ;
                  PD_LOG_MSG( PDERROR, "%s in fulltext index mappings should be Object",
                              FIELD_NAME_FIELDS ) ;
                  goto error ;
               }

               rc = ixmCheckMappingsFields( ele.embeddedObject(), indexKey ) ;
               if ( rc )
               {
                  goto error ;
               }
            }
            else
            {
               rc = SDB_INVALIDARG ;
               PD_LOG_MSG( PDERROR, "Unrecognized field[%s] in fulltext index mappings",
                           ele.fieldName() ) ;
               goto error ;
            }
         }
      }
      catch ( std::exception &e )
      {
         rc = ossException2RC( &e ) ;
         PD_LOG( PDERROR, "An exception occurred when checking fulltext index mappings: %s, "
                 "rc: %d", e.what(), rc ) ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

}

