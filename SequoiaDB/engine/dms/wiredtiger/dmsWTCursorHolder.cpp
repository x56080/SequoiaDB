/*******************************************************************************


   Copyright (C) 2011-2018 SequoiaDB Ltd.

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
   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSWTCURSORHOLDER__OPEN_INT, "_dmsWTCursorHolder::_open" )
   INT32 _dmsWTCursorHolder::_open( dmsWTStorageEngine &engine,
                                    const ossPoolString &uri,
                                    const ossPoolString &config,
                                    dmsWTSessIsolation isolation,
                                    UINT64 startKey,
                                    BOOLEAN isAfterStartKey,
                                    BOOLEAN isForward,
                                    IExecutor *executor )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__DMSWTCURSORHOLDER__OPEN_INT ) ;

      BOOLEAN isFound = FALSE ;

      rc = engine.openSession( _session, isolation ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to open session, rc: %d", rc ) ;

      rc = _cursor.open( uri, config ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to open cursor, rc: %d", rc ) ;

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

      _isOpened = TRUE ;
      _isForward = isForward ;

   done:
      PD_TRACE_EXITRC( SDB__DMSWTCURSORHOLDER__OPEN_INT, rc ) ;
      return rc ;

   error:
      _close() ;
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSWTCURSORHOLDER__OPEN_ITEM, "_dmsWTCursorHolder::_open" )
   INT32 _dmsWTCursorHolder::_open( dmsWTStorageEngine &engine,
                                    const ossPoolString &uri,
                                    const ossPoolString &config,
                                    dmsWTSessIsolation isolation,
                                    const dmsWTItem &startKey,
                                    BOOLEAN isAfterStartKey,
                                    BOOLEAN isForward,
                                    IExecutor *executor )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__DMSWTCURSORHOLDER__OPEN_ITEM ) ;

      BOOLEAN isFound = FALSE ;

      rc = engine.openSession( _session, isolation ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to open session, rc: %d", rc ) ;

      rc = _cursor.open( uri, config ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to open cursor, rc: %d", rc ) ;

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

      _isOpened = TRUE ;
      _isForward = isForward ;

   done:
      PD_TRACE_EXITRC( SDB__DMSWTCURSORHOLDER__OPEN_ITEM, rc ) ;
      return rc ;

   error:
      _close() ;
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSWTCURSORHOLDER__OPEN, "_dmsWTCursorHolder::_open" )
   INT32 _dmsWTCursorHolder::_open( dmsWTStorageEngine &engine,
                                    const ossPoolString &uri,
                                    const ossPoolString &config,
                                    dmsWTSessIsolation isolation,
                                    BOOLEAN isForward,
                                    IExecutor *executor )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__DMSWTCURSORHOLDER__OPEN ) ;

      rc = engine.openSession( _session, isolation ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to open session, rc: %d", rc ) ;

      rc = _cursor.open( uri, config ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to open cursor, rc: %d", rc ) ;

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

      _isOpened = TRUE ;
      _isForward = isForward ;

   done:
      PD_TRACE_EXITRC( SDB__DMSWTCURSORHOLDER__OPEN, rc ) ;
      return rc ;

   error:
      _close() ;
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
      _close() ;
      goto done ;
   }

}
}
