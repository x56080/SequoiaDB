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

   Source File Name = dmsWTCursorHolder.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          11/20/2023  HGM Initial Draft

   Last Changed =

*******************************************************************************/
#include "wiredtiger/dmsWTCursorHolder.hpp"
#include "pdTrace.hpp"
#include "dmsTrace.hpp"
#include "pd.hpp"

using namespace std ;

namespace engine
{
namespace wiredtiger
{

   /*
      _dmsWTCursorHolder implement
    */
   _dmsWTCursorHolder::_dmsWTCursorHolder( dmsWTSession &session )
   : _cursor( session )
   {
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSWTCURSORHOLDER__OPEN_INT, "_dmsWTCursorHolder::_open" )
   INT32 _dmsWTCursorHolder::_open( dmsWTStorageEngine &engine,
                                    const ossPoolString &uri,
                                    const ossPoolString &config,
                                    UINT64 startKey,
                                    BOOLEAN isAfterStartKey,
                                    BOOLEAN isForward,
                                    UINT64 snapshotID,
                                    IExecutor *executor )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__DMSWTCURSORHOLDER__OPEN_INT ) ;

      BOOLEAN isFound = FALSE ;

      _isForward = isForward ;
      _isSample = FALSE ;
      _snapshotID = snapshotID ;

      rc = _cursor.open( uri, config ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to open cursor, rc: %d", rc ) ;
      _isOpened = TRUE ;

      if ( isForward )
      {
         rc = _cursor.searchNext( startKey, isAfterStartKey, isFound ) ;
      }
      else
      {
         rc = _cursor.searchPrev( startKey, isAfterStartKey, isFound ) ;
      }
      if ( SDB_OK != rc )
      {
         if ( SDB_DMS_EOC == rc )
         {
            _isEOF = TRUE ;
         }
         else
         {
            PD_LOG( PDERROR, "Failed to search key, rc: %d", rc ) ;
         }
         goto error ;
      }

   done:
      PD_TRACE_EXITRC( SDB__DMSWTCURSORHOLDER__OPEN_INT, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSWTCURSORHOLDER__OPEN_ITEM, "_dmsWTCursorHolder::_open" )
   INT32 _dmsWTCursorHolder::_open( dmsWTStorageEngine &engine,
                                    const ossPoolString &uri,
                                    const ossPoolString &config,
                                    const dmsWTItem &startKey,
                                    BOOLEAN isAfterStartKey,
                                    BOOLEAN isForward,
                                    UINT64 snapshotID,
                                    IExecutor *executor )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__DMSWTCURSORHOLDER__OPEN_ITEM ) ;

      BOOLEAN isFound = FALSE ;

      _isForward = isForward ;
      _isSample = FALSE ;
      _snapshotID = snapshotID ;

      rc = _cursor.open( uri, config ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to open cursor, rc: %d", rc ) ;
      _isOpened = TRUE ;

      if ( isForward )
      {
         rc = _cursor.searchNext( startKey, isAfterStartKey, isFound ) ;
      }
      else
      {
         rc = _cursor.searchPrev( startKey, isAfterStartKey, isFound ) ;
      }
      if ( SDB_OK != rc )
      {
         if ( SDB_DMS_EOC == rc )
         {
            _isEOF = TRUE ;
         }
         else
         {
            PD_LOG( PDERROR, "Failed to search key, rc: %d", rc ) ;
         }
         goto error ;
      }

   done:
      PD_TRACE_EXITRC( SDB__DMSWTCURSORHOLDER__OPEN_ITEM, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSWTCURSORHOLDER__OPEN, "_dmsWTCursorHolder::_open" )
   INT32 _dmsWTCursorHolder::_open( dmsWTStorageEngine &engine,
                                    const ossPoolString &uri,
                                    const ossPoolString &config,
                                    BOOLEAN isForward,
                                    UINT64 snapshotID,
                                    IExecutor *executor )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__DMSWTCURSORHOLDER__OPEN ) ;

      _isForward = isForward ;
      _isSample = FALSE ;
      _snapshotID = snapshotID ;

      rc = _cursor.open( uri, config ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to open cursor, rc: %d", rc ) ;
      _isOpened = TRUE ;

      if ( isForward )
      {
         rc = _cursor.next() ;
      }
      else
      {
         rc = _cursor.prev() ;
      }
      if ( SDB_OK != rc )
      {
         if ( SDB_DMS_EOC == rc )
         {
            _isEOF = TRUE ;
         }
         else
         {
            PD_LOG( PDERROR, "Failed to move cursor [%s], rc: %d",
                    isForward ? "forward" : "backward", rc ) ;
         }
         goto error ;
      }

   done:
      PD_TRACE_EXITRC( SDB__DMSWTCURSORHOLDER__OPEN, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSWTCURSORHOLDER__OPEN_SAMPLE, "_dmsWTCursorHolder::_open" )
   INT32 _dmsWTCursorHolder::_open( dmsWTStorageEngine &engine,
                                    const ossPoolString &uri,
                                    const ossPoolString &config,
                                    UINT64 sampleNum,
                                    UINT64 snapshotID,
                                    IExecutor *executor )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__DMSWTCURSORHOLDER__OPEN_SAMPLE ) ;

      ossPoolString sampleConfig ;

      _isForward = TRUE ;
      _isSample = TRUE ;
      _snapshotID = snapshotID ;

      try
      {
         ossPoolStringStream ss ;
         // ss << "next_random=true,next_random_sample_size=" << sampleNum ;
         ss << "next_random=true" ;
         if ( !config.empty() )
         {
            ss << "," << config ;
         }
         sampleConfig = ss.str() ;
      }
      catch ( exception &e )
      {
         rc = ossException2RC( &e ) ;
         PD_LOG( PDERROR, "Failed to build config string, occurred exception: %s",
                 e.what() ) ;
         goto error ;
      }

      rc = _cursor.open( uri, sampleConfig ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to open cursor, rc: %d", rc ) ;
      _isOpened = TRUE ;

      rc = _cursor.next() ;
      if ( SDB_OK != rc )
      {
         if ( SDB_DMS_EOC == rc )
         {
            _isEOF = TRUE ;
         }
         else
         {
            PD_LOG( PDERROR, "Failed to move cursor forward, rc: %d", rc ) ;
            goto error ;
         }
      }

   done:
      PD_TRACE_EXITRC( SDB__DMSWTCURSORHOLDER__OPEN_SAMPLE, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSWTCURSORHOLDER__CLOSE, "_dmsWTCursorHolder::_close" )
   INT32 _dmsWTCursorHolder::_close()
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__DMSWTCURSORHOLDER__CLOSE ) ;

      rc = _cursor.close() ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "Failed to close cursor, rc: %d", rc ) ;
      }
      _isClosed = TRUE ;

      PD_TRACE_EXITRC( SDB__DMSWTCURSORHOLDER__CLOSE, rc ) ;

      return rc ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSWTCURSORHOLDER__ADVANCE, "_dmsWTCursorHolder::_advance" )
   INT32 _dmsWTCursorHolder::_advance( IExecutor *executor )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__DMSWTCURSORHOLDER__ADVANCE ) ;

      if ( _isEOF )
      {
         rc = SDB_DMS_EOC ;
         goto error ;
      }

      PD_CHECK( _isOpened, SDB_SYS, error, PDERROR,
                "Failed to move cursor [%s], cursor is not opened",
                _isForward ? "forward" : "backward" ) ;
      PD_CHECK( !_isClosed, SDB_SYS, error, PDERROR,
                "Failed to move cursor [%s], cursor is not opened",
                _isForward ? "forward" : "backward" ) ;

      if ( _isForward )
      {
         rc = _cursor.next() ;
      }
      else
      {
         rc = _cursor.prev() ;
      }
      if ( SDB_OK != rc )
      {
         if ( SDB_DMS_EOC == rc )
         {
            _isEOF = TRUE ;
         }
         else
         {
            PD_LOG( PDERROR, "Failed to move cursor [%s], rc: %d",
                    _isForward ? "forward" : "backward", rc ) ;
         }
         goto error ;
      }

   done:
      PD_TRACE_EXITRC( SDB__DMSWTCURSORHOLDER__ADVANCE, rc ) ;
      return rc ;

   error:
      goto done ;
   }

}
}
