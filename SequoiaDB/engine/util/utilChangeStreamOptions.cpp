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

   Source File Name = utilChangeStreamOptions.cpp

   Descriptive Name = Change Stream Options

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          06/01/2023  HGM Initial Draft

   Last Changed =

*******************************************************************************/
#include "utilChangeStreamOptions.hpp"
#include "dms.hpp"
#include "msgDef.hpp"
#include "ossUtil.h"
#include "pdTrace.hpp"
#include "utilTrace.hpp"

using namespace bson ;
using namespace std ;

namespace engine
{

   /*
      _utilChangeStreamOptions implement
    */
   void _utilChangeStreamOptions::reset()
   {
      _boOptions = BSONObj() ;
      _collectionSpaces.clear() ;
      _collections.clear() ;
      _changeTypeMask = UTIL_CHANGE_TYPE_DFT ;
      _maxWaitTimeMS = UTIL_CHANGE_STREAM_MAX_WAIT_TIME_MS_DEF ;
      _cacheSizeB = UTIL_CHANGE_STREAM_CACHE_SIZE_B_DEF ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_UTILCHANGESTREAMOPTS_FROMBSON, "_utilChangeStreamOptions::fromBSON")
   INT32 _utilChangeStreamOptions::fromBSON( const BSONObj &options )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_UTILCHANGESTREAMOPTS_FROMBSON ) ;

      reset() ;

      try
      {
         BSONElement element ;

         // copy options
         _boOptions = options.copy() ;
         _token.fromString( "" ) ;

         BSONObjIterator iter( _boOptions ) ;
         while ( iter.more() )
         {
            element = iter.next() ;
            const CHAR *fieldName = element.fieldName() ;

            if ( 0 == ossStrcmp( fieldName, FIELD_NAME_TOKEN ) )
            {
               // get token
               PD_LOG_MSG_CHECK( String == element.type(),
                                 SDB_INVALIDARG, error, PDERROR,
                                 "Failed to get field [%s], it is not a string",
                                 FIELD_NAME_TOKEN ) ;
               // check string size
               // NOTE: string size in BSONElment includes the trailing terminator
               PD_LOG_MSG_CHECK( ( ( 1 == element.valuestrsize() ) ||
                                 ( MSG_STREAM_TOKEN_STING_SIZE + 1 ==
                                                      element.valuestrsize() ) ),
                                 SDB_INVALIDARG, error, PDERROR,
                                 "Failed to get field [%s], "
                                 "invalid string size [%d], "
                                 "expected empty or [%d]",
                                 FIELD_NAME_TOKEN, element.valuestrsize() - 1,
                                 MSG_STREAM_TOKEN_STING_SIZE ) ;
               _token.fromString( element.valuestr() ) ;
            }
            else if ( 0 == ossStrcmp( fieldName, FIELD_NAME_COLLECTION_SPACES ) )
            {
               // get collection spaces
               if ( Array == element.type() )
               {
                  BSONObjIterator iter( element.embeddedObject() ) ;
                  while ( iter.more() )
                  {
                     BSONElement nameElement = iter.next() ;
                     PD_LOG_MSG_CHECK( String == nameElement.type(),
                                       SDB_INVALIDARG, error, PDERROR,
                                       "Failed to get string element from field [%s]",
                                       FIELD_NAME_COLLECTION_SPACES ) ;
                     const CHAR *name = nameElement.valuestr() ;
                     rc = dmsCheckCSName( name, TRUE ) ;
                     if ( SDB_OK != rc )
                     {
                        PD_LOG_MSG( PDERROR, "Failed to check collection space "
                                    "name [%s], rc: %d", name, rc ) ;
                        goto error ;
                     }
                     _collectionSpaces.insert( name ) ;
                  }
               }
               else if ( String == element.type() )
               {
                  const CHAR *name = element.valuestr() ;
                  rc = dmsCheckCSName( name, TRUE ) ;
                  if ( SDB_OK != rc )
                  {
                     PD_LOG_MSG( PDERROR, "Failed to check collection space "
                                 "name [%s], rc: %d", name, rc ) ;
                     goto error ;
                  }
                  _collectionSpaces.insert( name ) ;
               }
               else if ( EOO != element.type() )
               {
                  PD_LOG_MSG_CHECK( FALSE, SDB_INVALIDARG, error, PDERROR,
                                    "Failed to get field [%s], "
                                    "it is not an array or a string",
                                    FIELD_NAME_COLLECTION_SPACES ) ;
               }
            }
            else if ( 0 == ossStrcmp( fieldName, FIELD_NAME_COLLECTIONS ) )
            {
               if ( Array == element.type() )
               {
                  BSONObjIterator iter( element.embeddedObject() ) ;
                  while ( iter.more() )
                  {
                     BSONElement nameElement = iter.next() ;
                     PD_LOG_MSG_CHECK( String == nameElement.type(),
                                       SDB_INVALIDARG, error, PDERROR,
                                       "Failed to get string element from field [%s]",
                                       FIELD_NAME_COLLECTIONS ) ;
                     const CHAR *name = nameElement.valuestr() ;
                     rc = dmsCheckFullCLName( name, TRUE ) ;
                     if ( SDB_OK != rc )
                     {
                        PD_LOG_MSG( PDERROR, "Failed to check collection "
                                    "name [%s], rc: %d", name, rc ) ;
                        goto error ;
                     }
                     _collections.insert( name ) ;
                  }
               }
               else if ( String == element.type() )
               {
                  const CHAR *name = element.valuestr() ;
                  rc = dmsCheckFullCLName( name, TRUE ) ;
                  if ( SDB_OK != rc )
                  {
                     PD_LOG_MSG( PDERROR, "Failed to check collection "
                                 "name [%s], rc: %d", name, rc ) ;
                     goto error ;
                  }
                  _collections.insert( name ) ;
               }
               else if ( EOO != element.type() )
               {
                  PD_LOG_MSG_CHECK( FALSE, SDB_INVALIDARG, error, PDERROR,
                                    "Failed to get field [%s], "
                                    "it is not an array or a string",
                                    FIELD_NAME_COLLECTIONS ) ;
               }
            }
            else if ( 0 == ossStrcmp( fieldName, FIELD_NAME_MAX_WAIT_TIME ) )
            {
               PD_LOG_MSG_CHECK( element.isNumber(),
                                 SDB_INVALIDARG, error, PDERROR,
                                 "Failed to get field [%s], it is not a number",
                                 FIELD_NAME_MAX_WAIT_TIME ) ;
               // try roundup between -1 and 2^31-1
               INT64 tmpTimeMS = (INT64)( element.numberInt() ) * OSS_ONE_SEC ;
               if ( tmpTimeMS >= OSS_SINT32_MAX_LL )
               {
                  _maxWaitTimeMS = OSS_SINT32_MAX ;
               }
               else if ( tmpTimeMS < 0 )
               {
                  _maxWaitTimeMS = -1 ;
               }
               else
               {
                  _maxWaitTimeMS = (INT32)tmpTimeMS ;
               }
            }
            else if ( 0 == ossStrcmp( fieldName, FIELD_NAME_CACHE_SIZE ) )
            {
               PD_LOG_MSG_CHECK( element.isNumber(),
                                 SDB_INVALIDARG, error, PDERROR,
                                 "Failed to get field [%s], it is not a number",
                                 FIELD_NAME_CACHE_SIZE ) ;

               INT32 tmpCacheSize = element.numberInt() ;
               PD_LOG_MSG_CHECK( ( tmpCacheSize >= UTIL_CHANGE_STREAM_CACHE_SIZE_MIN ) &&
                                 ( tmpCacheSize <= UTIL_CHANGE_STREAM_CACHE_SIZE_MAX ),
                                 SDB_INVALIDARG, error, PDERROR,
                                 "Failed to get field [%s], it must between [ %u, %u ]",
                                 FIELD_NAME_CACHE_SIZE,
                                 UTIL_CHANGE_STREAM_CACHE_SIZE_MIN,
                                 UTIL_CHANGE_STREAM_CACHE_SIZE_MAX ) ;
               _cacheSizeB = tmpCacheSize * 1024 * 1024 ;
            }
            else if ( 0 == ossStrcmp( fieldName, FIELD_NAME_CHANGE_TYPES ) )
            {
               const CHAR *changeTypes = NULL ;
               PD_LOG_MSG_CHECK( String == element.type(),
                                 SDB_INVALIDARG, error, PDERROR,
                                 "Failed to get field [%s], it is not a string",
                                 FIELD_NAME_CHANGE_TYPES ) ;
               changeTypes = element.valuestr() ;
               rc = _parseChangeTypeMask( changeTypes ) ;
               PD_RC_CHECK( rc, PDERROR, "Failed to parse change types [%s], rc: %d",
                           changeTypes, rc ) ;
            }
            else if ( 0 == ossStrcmp( fieldName, FIELD_NAME_GROUPS ) )
            {
               // ignore groups
               continue;
            }
            else
            {
               PD_LOG_MSG( PDERROR, "Failed to parse change stream options, "
                           "[%s] is not supported", fieldName ) ;
               rc = SDB_INVALIDARG ;
               goto error ;
            }
         }
      }
      catch ( exception &e )
      {
         PD_LOG( PDERROR, "Failed to parse change stream conf from BSON, "
                 "occurred exception %s", e.what() ) ;
         rc = ossException2RC( &e ) ;
         goto error ;
      }

   done:
      PD_TRACE_EXITRC( SDB_UTILCHANGESTREAMOPTS_FROMBSON, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_UTILCHANGESTREAMOPTS__PARSECHANGETYPE, "_utilChangeStreamOptions::_parseChangeTypeMask")
   INT32 _utilChangeStreamOptions::_parseChangeTypeMask( const CHAR *changeTypes )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_UTILCHANGESTREAMOPTS__PARSECHANGETYPE ) ;

      const CHAR *p = changeTypes ;
      BOOLEAN gotNone = FALSE ;
      UINT8 tmpChangeTypeMask = UTIL_CHANGE_TYPE_NONE ;

      _changeTypeMask = UTIL_CHANGE_TYPE_NONE ;

      // change type split by '|'
      while ( NULL != p && '\0' != *p )
      {
         if ( ' ' == *p || '\t' == *p )
         {
            ++ p ;
            continue ;
         }
         const CHAR *pEnd = (const CHAR *)ossStrchr( p, '|' ) ;
         const CHAR *p1 = pEnd ;
         UINT32 length = 0 ;
         if ( p1 )
         {
            length = p1 - p ;
            -- p1 ;
         }
         else
         {
            length = ossStrlen( p ) ;
            p1 = p + length - 1 ;
         }
         while ( length > 0 && ( ' ' == *p1 || '\t' == *p1 ) )
         {
            -- p1 ;
            -- length ;
         }
         if ( length > 0 )
         {
            // case-insensitive compare
            if ( 0== ossStrncasecmp( p, UTIL_CHANGE_TYPE_NAME_NONE, length ) &&
                 length == ossStrlen( UTIL_CHANGE_TYPE_NAME_NONE ) )
            {
               // NONE
               gotNone = TRUE ;
            }
            else if ( 0 == ossStrncasecmp( p, UTIL_CHANGE_TYPE_NAME_ALL, length ) &&
                      length == ossStrlen( UTIL_CHANGE_TYPE_NAME_ALL ) )
            {
               // ALL
               tmpChangeTypeMask = UTIL_CHANGE_TYPE_ALL ;
            }
            else if ( 0 == ossStrncasecmp( p, UTIL_CHANGE_TYPE_NAME_DDL, length ) &&
                      length == ossStrlen( UTIL_CHANGE_TYPE_NAME_DDL ) )
            {
               // DDL
               OSS_BIT_SET( tmpChangeTypeMask, UTIL_CHANGE_TYPE_DDL ) ;
            }
            else if ( 0 == ossStrncasecmp( p, UTIL_CHANGE_TYPE_NAME_DML_RECORD, length ) &&
                      length == ossStrlen( UTIL_CHANGE_TYPE_NAME_DML_RECORD ) )
            {
               // DML for BSON record
               OSS_BIT_SET( tmpChangeTypeMask, UTIL_CHANGE_TYPE_DML_RECORD ) ;
            }
            else if ( 0 == ossStrncasecmp( p, UTIL_CHANGE_TYPE_NAME_DML_LOB, length ) &&
                      length == ossStrlen( UTIL_CHANGE_TYPE_NAME_DML_LOB ) )
            {
               // DML for LOB
               OSS_BIT_SET( tmpChangeTypeMask, UTIL_CHANGE_TYPE_DML_LOB ) ;
            }
            else if ( 0 == ossStrncasecmp( p, UTIL_CHANGE_TYPE_NAME_TRANS, length ) &&
                      length == ossStrlen( UTIL_CHANGE_TYPE_NAME_TRANS ) )
            {
               // TRANSACTION
               OSS_BIT_SET( tmpChangeTypeMask, UTIL_CHANGE_TYPE_TRANS ) ;
            }
            else
            {
               PD_LOG_MSG_CHECK( FALSE, SDB_INVALIDARG, error, PDERROR,
                                 "Failed to parse change types [%s]",
                                 changeTypes ) ;
            }
         }
         p = pEnd ? pEnd + 1 : NULL ;
      }

      if ( gotNone )
      {
         PD_LOG_MSG_CHECK( _allowNoneChangeTypes, SDB_INVALIDARG, error, PDERROR,
                           "Failed to parse change types [%s], \"%s\" is not allowed",
                           changeTypes, UTIL_CHANGE_TYPE_NAME_NONE ) ;
         PD_LOG_MSG_CHECK( UTIL_CHANGE_TYPE_NONE == tmpChangeTypeMask,
                           SDB_INVALIDARG, error, PDERROR,
                           "Failed to parse change types [%s], can not specify "
                           "\"%s\" with other change types", changeTypes,
                           UTIL_CHANGE_TYPE_NAME_NONE ) ;
      }

      _changeTypeMask = tmpChangeTypeMask ;

   done:
      PD_TRACE_EXITRC( SDB_UTILCHANGESTREAMOPTS__PARSECHANGETYPE, rc ) ;
      return rc ;

   error:
      goto done ;
   }

}
