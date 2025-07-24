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

   Source File Name = dmsWTSession.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          11/20/2023  HGM Initial Draft

   Last Changed =

*******************************************************************************/
#include "wiredtiger/dmsWTSession.hpp"
#include "wiredtiger/dmsWTPersistUnit.hpp"
#include "wiredtiger/dmsWTUtil.hpp"
#include "pdTrace.hpp"
#include "dmsTrace.hpp"
#include "pd.hpp"

using namespace std ;

namespace engine
{
namespace wiredtiger
{

   /*
      _dmsWTSession implement
    */
   _dmsWTSession::_dmsWTSession()
   : _session( nullptr )
   {
   }

   _dmsWTSession::~_dmsWTSession()
   {
      close() ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSWTSESSION_OPEN, "_dmsWTSession::open" )
   INT32 _dmsWTSession::open( WT_CONNECTION *conn,
                              dmsWTSessIsolation isolation )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__DMSWTSESSION_OPEN ) ;

      const CHAR *config = nullptr ;
      switch ( isolation )
      {
      case dmsWTSessIsolation::READ_UNCOMMITTED:
         config = "isolation=read-uncommitted" ;
         break ;
      case dmsWTSessIsolation::READ_COMMITTED:
         config = "isolation=read-committed" ;
         break ;
      case dmsWTSessIsolation::SNAPSHOT:
         config = "isolation=snapshot" ;
         break ;
      default:
         PD_LOG( PDERROR, "Invalid session isolation: %d", isolation ) ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      PD_CHECK( nullptr != conn, SDB_SYS, error, PDERROR,
                "Failed to open session, connection is not opened" ) ;
      PD_CHECK( nullptr == _session, SDB_SYS, error, PDERROR,
                "Failed to open session, already opened" ) ;

      rc = WT_CALL( conn->open_session( conn, nullptr, config, &_session ), nullptr ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to open session, rc: %d", rc ) ;

   done:
      PD_TRACE_EXITRC( SDB__DMSWTSESSION_OPEN, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSWTSESSION_CLOSE, "_dmsWTSession::close" )
   INT32 _dmsWTSession::close()
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__DMSWTSESSION_CLOSE ) ;

      if ( nullptr == _session )
      {
         goto done ;
      }

      rc = WT_CALL( _session->close( _session, nullptr ), nullptr ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDWARNING, "Failed to close session, rc: %d", rc ) ;
      }
      _session = nullptr ;

   done:
      PD_TRACE_EXITRC( SDB__DMSWTSESSION_CLOSE, rc ) ;

      return rc ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSWTSESSION_BEGINTRANS, "_dmsWTSession::beginTrans" )
   INT32 _dmsWTSession::beginTrans()
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__DMSWTSESSION_BEGINTRANS ) ;

      PD_CHECK( nullptr != _session, SDB_SYS, error, PDERROR,
                "Failed to begin transaction, session is not opened" ) ;

      rc = WT_CALL( _session->begin_transaction( _session, nullptr ), nullptr ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to begin transaction, rc: %d", rc ) ;

   done:
      PD_TRACE_EXITRC( SDB__DMSWTSESSION_BEGINTRANS, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSWTSESSION_PREPARETRANS, "_dmsWTSession::prepareTrans" )
   INT32 _dmsWTSession::prepareTrans()
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__DMSWTSESSION_PREPARETRANS ) ;

      PD_CHECK( nullptr != _session, SDB_SYS, error, PDERROR,
                "Failed to prepare transaction, session is not opened" ) ;

      rc = WT_CALL( _session->prepare_transaction( _session, nullptr ), nullptr ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to prepare transaction, rc: %d", rc ) ;

   done:
      PD_TRACE_EXITRC( SDB__DMSWTSESSION_PREPARETRANS, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSWTSESSION_COMMITTRANS, "_dmsWTSession::commitTrans" )
   INT32 _dmsWTSession::commitTrans()
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__DMSWTSESSION_COMMITTRANS ) ;

      PD_CHECK( nullptr != _session, SDB_SYS, error, PDERROR,
                "Failed to commit transaction, session is not opened" ) ;

      rc = WT_CALL( _session->commit_transaction( _session, nullptr ), nullptr ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to commit transaction, rc: %d", rc ) ;

   done:
      PD_TRACE_EXITRC( SDB__DMSWTSESSION_COMMITTRANS, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSWTSESSION_ABORTTRANS, "_dmsWTSession::abortTrans" )
   INT32 _dmsWTSession::abortTrans()
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__DMSWTSESSION_ABORTTRANS ) ;

      PD_CHECK( nullptr != _session, SDB_SYS, error, PDERROR,
                "Failed to rollback transaction, session is not opened" ) ;

      rc = WT_CALL( _session->rollback_transaction( _session, nullptr ), nullptr ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to rollback transaction, rc: %d", rc ) ;

   done:
      PD_TRACE_EXITRC( SDB__DMSWTSESSION_ABORTTRANS, rc ) ;
      return rc ;

   error:
      goto done ;
   }

}
}
