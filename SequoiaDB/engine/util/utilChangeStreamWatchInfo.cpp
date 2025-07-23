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

   Source File Name = utilChangeStreamWatchInfo.cpp

   Descriptive Name = Change Stream Watch Info

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          06/01/2023  HGM Initial Draft

   Last Changed =

*******************************************************************************/
#include "utilChangeStreamWatchInfo.hpp"
#include "dms.hpp"
#include "dpsDef.hpp"
#include "msgDef.hpp"
#include "pdTrace.hpp"
#include "utilTrace.hpp"
#include "utilUniqueID.hpp"

using namespace bson ;
using namespace std ;

namespace engine
{

   #define UTIL_WATCH_TYPE_NAME_UNKNOWN "unknown"
   #define UTIL_WATCH_TYPE_NAME_COLLECTION "collection"
   #define UTIL_WATCH_TYPE_NAME_COLLECTION_SPACE "collection space"
   #define UTIL_WATCH_TYPE_NAME_ALL "all"

   // helper functions
   const CHAR *utilGetWatchTypeName( utilWatchType watchType )
   {
      const CHAR *result = UTIL_WATCH_TYPE_NAME_UNKNOWN ;

      switch ( watchType )
      {
      case UTIL_WATCH_COLLECTION:
         result = UTIL_WATCH_TYPE_NAME_COLLECTION ;
         break ;
      case UTIL_WATCH_COLLECTION_SPACE:
         result = UTIL_WATCH_TYPE_NAME_COLLECTION_SPACE ;
         break ;
      case UTIL_WATCH_ALL:
         result = UTIL_WATCH_TYPE_NAME_ALL ;
         break ;
      default:
         result = UTIL_WATCH_TYPE_NAME_UNKNOWN ;
         break ;
      }

      return result ;
   }

   /*
      _utilWatchInfo implement
    */
   // PD_TRACE_DECLARE_FUNCTION ( SDB_UTILCHANGESTREAMWATCHINFO_WATCHCS, "_utilChangeStreamWatchInfo::watchCS")
   INT32 _utilChangeStreamWatchInfo::watchCS( const CHAR *csName,
                                              utilCSUniqueID csUniqueID )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_UTILCHANGESTREAMWATCHINFO_WATCHCS ) ;

      try
      {
         if ( UTIL_IS_VALID_CSUNIQUEID( csUniqueID ) )
         {
            _watchedCSByName.insert( csName ) ;
         }
         else
         {
            _watchedSysCSByName.insert( csName ) ;
         }
         _watchedCSByUID.insert( csUniqueID ) ;
         _watchedCSBitmap.watch( csUniqueID ) ;
      }
      catch ( exception &e )
      {
         PD_LOG( PDERROR, "Failed to save watched collection space, "
                 "occurred exception %s", e.what() ) ;
         rc = ossException2RC( &e ) ;
         goto error ;
      }

   done:
      PD_TRACE_EXITRC( SDB_UTILCHANGESTREAMWATCHINFO_WATCHCS, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_UTILCHANGESTREAMWATCHINFO_WATCHCL, "_utilChangeStreamWatchInfo::watchCL")
   INT32 _utilChangeStreamWatchInfo::watchCL( const CHAR *clName,
                                              utilCLUniqueID clUniqueID )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_UTILCHANGESTREAMWATCHINFO_WATCHCL ) ;

      try
      {
         if ( UTIL_IS_VALID_CSUNIQUEID( utilGetCSUniqueID( clUniqueID ) ) &&
              UTIL_IS_VALID_CLUNIQUEID( clUniqueID ) )
         {
            _watchedCLByName.insert( clName ) ;
         }
         else
         {
            _watchedSysCLByName.insert( clName ) ;
         }
         _watchedCLByUID.insert( clUniqueID ) ;
         _watchedCSBitmap.watch( clUniqueID ) ;
      }
      catch ( exception &e )
      {
         PD_LOG( PDERROR, "Failed to save watched collection, "
                 "occurred exception %s", e.what() ) ;
         rc = ossException2RC( &e ) ;
         goto error ;
      }

   done:
      PD_TRACE_EXITRC( SDB_UTILCHANGESTREAMWATCHINFO_WATCHCL, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_UTILCHANGESTREAMWATCHINFO_WATCHLOGTYPES, "_utilChangeStreamWatchInfo::watchLogTypes" )
   INT32 _utilChangeStreamWatchInfo::watchLogTypes( UINT8 changeTypeMask )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_UTILCHANGESTREAMWATCHINFO_WATCHLOGTYPES ) ;

      if ( OSS_BIT_TEST( changeTypeMask, UTIL_CHANGE_TYPE_DML_RECORD ) )
      {
         _watchedLogTypeBitmap.setBit( LOG_TYPE_DATA_INSERT ) ;
         _watchedLogTypeBitmap.setBit( LOG_TYPE_DATA_UPDATE ) ;
         _watchedLogTypeBitmap.setBit( LOG_TYPE_DATA_DELETE ) ;
         _watchedLogTypeBitmap.setBit( LOG_TYPE_CL_TRUNC ) ;
         _watchedLogTypeBitmap.setBit( LOG_TYPE_DATA_POP ) ;
      }
      if ( OSS_BIT_TEST( changeTypeMask, UTIL_CHANGE_TYPE_DML_LOB ) )
      {
         _watchedLogTypeBitmap.setBit( LOG_TYPE_CL_TRUNC ) ;
         _watchedLogTypeBitmap.setBit( LOG_TYPE_LOB_WRITE ) ;
         _watchedLogTypeBitmap.setBit( LOG_TYPE_LOB_UPDATE ) ;
         _watchedLogTypeBitmap.setBit( LOG_TYPE_LOB_REMOVE ) ;
         _watchedLogTypeBitmap.setBit( LOG_TYPE_LOB_TRUNCATE ) ;
      }
      if ( OSS_BIT_TEST( changeTypeMask, UTIL_CHANGE_TYPE_DDL ) )
      {
         _watchedLogTypeBitmap.setBit( LOG_TYPE_CS_CRT ) ;
         _watchedLogTypeBitmap.setBit( LOG_TYPE_CS_DELETE ) ;
         _watchedLogTypeBitmap.setBit( LOG_TYPE_CL_CRT ) ;
         _watchedLogTypeBitmap.setBit( LOG_TYPE_CL_DELETE ) ;
         _watchedLogTypeBitmap.setBit( LOG_TYPE_IX_CRT ) ;
         _watchedLogTypeBitmap.setBit( LOG_TYPE_IX_DELETE ) ;
         _watchedLogTypeBitmap.setBit( LOG_TYPE_CL_RENAME ) ;
         _watchedLogTypeBitmap.setBit( LOG_TYPE_INVALIDATE_CATA ) ;
         _watchedLogTypeBitmap.setBit( LOG_TYPE_CS_RENAME ) ;
         _watchedLogTypeBitmap.setBit( LOG_TYPE_ALTER ) ;
         _watchedLogTypeBitmap.setBit( LOG_TYPE_ADDUNIQUEID ) ;
         _watchedLogTypeBitmap.setBit( LOG_TYPE_RETURN ) ;
      }
      if ( OSS_BIT_TEST( changeTypeMask, UTIL_CHANGE_TYPE_TRANS ) )
      {
         _watchedLogTypeBitmap.setBit( LOG_TYPE_TS_COMMIT ) ;
         _watchedLogTypeBitmap.setBit( LOG_TYPE_TS_ROLLBACK ) ;
      }

      PD_TRACE_EXITRC( SDB_UTILCHANGESTREAMWATCHINFO_WATCHLOGTYPES, rc ) ;

      return rc ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_UTILCHANGESTREAMWATCHINFO_COMPLETE, "_utilChangeStreamWatchInfo::complete")
   void _utilChangeStreamWatchInfo::complete()
   {
      PD_TRACE_ENTRY( SDB_UTILCHANGESTREAMWATCHINFO_COMPLETE ) ;

      if ( _watchedCLByName.empty() &&
           _watchedSysCLByName.empty() &&
           _watchedCSByName.empty() &&
           _watchedSysCSByName.empty() )
      {
         _watchLevel = UTIL_WATCH_ALL ;
      }
      else if ( _watchedCSByName.empty() &&
                _watchedSysCSByName.empty() )
      {
         _watchLevel = UTIL_WATCH_COLLECTION ;
      }
      else
      {
         _watchLevel = UTIL_WATCH_COLLECTION_SPACE ;
      }

      // we need to check collection space drop and rename
      if ( !_watchedCSByUID.empty() )
      {
         _errorLogTypeBitmap.setBit( LOG_TYPE_CS_DELETE ) ;
         _errorLogTypeBitmap.setBit( LOG_TYPE_CS_RENAME ) ;
      }
      // we need to check collection space drop and rename, collection drop and rename
      if ( !_watchedCLByUID.empty() )
      {
         _errorLogTypeBitmap.setBit( LOG_TYPE_CS_DELETE ) ;
         _errorLogTypeBitmap.setBit( LOG_TYPE_CS_RENAME ) ;
         _errorLogTypeBitmap.setBit( LOG_TYPE_CL_DELETE ) ;
         _errorLogTypeBitmap.setBit( LOG_TYPE_CL_RENAME ) ;
      }

      PD_TRACE_EXIT( SDB_UTILCHANGESTREAMWATCHINFO_COMPLETE ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_UTILCHANGESTREAMWATCHINFO_ISWATCHINGCS, "_utilChangeStreamWatchInfo::isWatchingCS")
   BOOLEAN _utilChangeStreamWatchInfo::isWatchingCS( utilCSUniqueID csUniqueID,
                                                     BOOLEAN &needCheckName ) const
   {
      BOOLEAN result = FALSE ;

      PD_TRACE_ENTRY( SDB_UTILCHANGESTREAMWATCHINFO_ISWATCHINGCS ) ;

      if ( UTIL_WATCH_ALL == _watchLevel )
      {
         // watching all collection spaces
         result = TRUE ;
         needCheckName = FALSE ;
      }
      else if ( _watchedCSBitmap.isWatching( csUniqueID ) )
      {
         // pass the bitmap check for unique ID
         if ( UTIL_WATCH_COLLECTION_SPACE == _watchLevel )
         {
            // check if watching collection space explicitly
            result = _watchedCSByUID.count( csUniqueID ) > 0 ;
         }

         if ( !result )
         {
            // check if watching collection space by watching its collection
            utilCLUniqueID tmpCLUniqueID = utilBuildCLUniqueID( csUniqueID, 0 ) ;
            utilWatchCLUIDSetCIter iter = _watchedCLByUID.lower_bound( tmpCLUniqueID ) ;
            result = ( iter != _watchedCLByUID.end() ) &&
                     ( utilGetCSUniqueID( *iter ) == csUniqueID ) ;
         }

         if ( result )
         {
            // need check name for further if it is system collection space
            needCheckName = ( !UTIL_IS_VALID_CSUNIQUEID( csUniqueID ) ) &&
                            ( !_watchedSysCSByName.empty() ) &&
                            ( !_watchedSysCLByName.empty() ) ;
         }
      }

      PD_TRACE_EXIT( SDB_UTILCHANGESTREAMWATCHINFO_ISWATCHINGCS ) ;

      return result ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_UTILCHANGESTREAMWATCHINFO_ISWATCHINGCL, "_utilChangeStreamWatchInfo::isWatchingCL")
   BOOLEAN _utilChangeStreamWatchInfo::isWatchingCL( utilCLUniqueID clUniqueID,
                                                     BOOLEAN &needCheckName ) const
   {
      BOOLEAN result = FALSE ;

      PD_TRACE_ENTRY( SDB_UTILCHANGESTREAMWATCHINFO_ISWATCHINGCL ) ;

      if ( UTIL_WATCH_ALL == _watchLevel )
      {
         // watching all collections
         result = TRUE ;
         needCheckName = FALSE ;
      }
      else if ( _watchedCSBitmap.isWatching( clUniqueID ) )
      {
         // pass the bitmap check for unique ID
         utilCSUniqueID csUniqueID = utilGetCSUniqueID( clUniqueID ) ;

         if ( UTIL_WATCH_COLLECTION_SPACE == _watchLevel )
         {
            // check if watching collection by watching its collection space
            result = _watchedCSByUID.count( csUniqueID ) > 0 ;
         }

         if ( !result )
         {
            // check if watching collection explicitly
            result = _watchedCLByUID.count( clUniqueID ) > 0 ;
         }

         if ( result )
         {
            // need check name for further if it is system collection space
            needCheckName = ( ( UTIL_IS_VALID_CSUNIQUEID( csUniqueID ) ) ||
                              ( UTIL_IS_VALID_CLUNIQUEID( clUniqueID ) ) ) &&
                           ( !_watchedSysCSByName.empty() ) &&
                           ( !_watchedSysCLByName.empty() ) ;
         }
      }

      PD_TRACE_EXIT( SDB_UTILCHANGESTREAMWATCHINFO_ISWATCHINGCL ) ;

      return result ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_UTILCHANGESTREAMWATCHINFO_ISWATCHINGCS_NAME, "_utilChangeStreamWatchInfo::isWatchingCS")
   BOOLEAN _utilChangeStreamWatchInfo::isWatchingCS( const CHAR *csName ) const
   {
      BOOLEAN result = FALSE ;

      PD_TRACE_ENTRY( SDB_UTILCHANGESTREAMWATCHINFO_ISWATCHINGCS_NAME ) ;

      if ( UTIL_WATCH_ALL == _watchLevel )
      {
         // watching all collection spaces
         result = TRUE ;
      }
      else
      {
         if ( UTIL_WATCH_COLLECTION_SPACE == _watchLevel )
         {
            // check if watching collection space by name explicitly
            result = isWatchingCSExplicitly( csName ) ;
         }

         if ( !result )
         {
            // check if watching collection space by name implicitly
            result = isWatchingCSImplicitly( csName ) ;
         }
      }

      PD_TRACE_EXIT( SDB_UTILCHANGESTREAMWATCHINFO_ISWATCHINGCS_NAME ) ;

      return result ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_UTILCHANGESTREAMWATCHINFO_ISWATCHINGCL_NAME, "_utilChangeStreamWatchInfo::isWatchingCL")
   BOOLEAN _utilChangeStreamWatchInfo::isWatchingCL( const CHAR *clName ) const
   {
      BOOLEAN result = FALSE ;

      PD_TRACE_ENTRY( SDB_UTILCHANGESTREAMWATCHINFO_ISWATCHINGCL_NAME ) ;

      if ( UTIL_WATCH_ALL == _watchLevel )
      {
         // watching all collections
         result = TRUE ;
      }
      else
      {
         if ( UTIL_WATCH_COLLECTION_SPACE == _watchLevel )
         {
            // check if watching collection by name implicitly
            result = isWatchingCLImplicitly( clName ) ;
         }

         if ( !result )
         {
            // check if watching collection by name explicitly
            result = isWatchingCLExplicitly( clName ) ;
         }
      }

      PD_TRACE_EXIT( SDB_UTILCHANGESTREAMWATCHINFO_ISWATCHINGCL_NAME ) ;

      return result ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_UTILCHANGESTREAMWATCHINFO_ISWATCHINGCSEXT_NAME, "_utilChangeStreamWatchInfo::isWatchingCSExplicitly")
   BOOLEAN _utilChangeStreamWatchInfo::isWatchingCSExplicitly( const CHAR *csName ) const
   {
      BOOLEAN result = FALSE ;

      PD_TRACE_ENTRY( SDB_UTILCHANGESTREAMWATCHINFO_ISWATCHINGCSEXT_NAME ) ;

      result = _watchedCSByName.isCSWatched( csName ) ||
               _watchedSysCSByName.isCSWatched( csName ) ;

      PD_TRACE_EXIT( SDB_UTILCHANGESTREAMWATCHINFO_ISWATCHINGCSEXT_NAME ) ;

      return result ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_UTILCHANGESTREAMWATCHINFO_ISWATCHINGCLEXT_NAME, "_utilChangeStreamWatchInfo::isWatchingCLExplicitly")
   BOOLEAN _utilChangeStreamWatchInfo::isWatchingCLExplicitly( const CHAR *clName ) const
   {
      BOOLEAN result = FALSE ;

      PD_TRACE_ENTRY( SDB_UTILCHANGESTREAMWATCHINFO_ISWATCHINGCLEXT_NAME ) ;

      result = _watchedCLByName.isCLWatched( clName ) ||
               _watchedSysCLByName.isCLWatched( clName ) ;

      PD_TRACE_EXIT( SDB_UTILCHANGESTREAMWATCHINFO_ISWATCHINGCLEXT_NAME ) ;

      return result ;
   }

      // PD_TRACE_DECLARE_FUNCTION ( SDB_UTILCHANGESTREAMWATCHINFO_ISWATCHINGCSIMT_NAME, "_utilChangeStreamWatchInfo::isWatchingCSImplicitly")
   BOOLEAN _utilChangeStreamWatchInfo::isWatchingCSImplicitly( const CHAR *csName ) const
   {
      BOOLEAN result = FALSE ;

      PD_TRACE_ENTRY( SDB_UTILCHANGESTREAMWATCHINFO_ISWATCHINGCSIMT_NAME ) ;

      result = _watchedCLByName.isCSWatched( csName ) ||
               _watchedSysCLByName.isCSWatched( csName ) ;

      PD_TRACE_EXIT( SDB_UTILCHANGESTREAMWATCHINFO_ISWATCHINGCSIMT_NAME ) ;

      return result ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_UTILCHANGESTREAMWATCHINFO_ISWATCHINGCLIMT_NAME, "_utilChangeStreamWatchInfo::isWatchingCLImplicitly")
   BOOLEAN _utilChangeStreamWatchInfo::isWatchingCLImplicitly( const CHAR *clName ) const
   {
      BOOLEAN result = FALSE ;

      PD_TRACE_ENTRY( SDB_UTILCHANGESTREAMWATCHINFO_ISWATCHINGCLIMT_NAME ) ;

      result = _watchedCSByName.isCLWatched( clName ) ||
               _watchedSysCSByName.isCLWatched( clName ) ;

      PD_TRACE_EXIT( SDB_UTILCHANGESTREAMWATCHINFO_ISWATCHINGCLIMT_NAME ) ;

      return result ;
   }

}
