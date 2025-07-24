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

   Source File Name = dmsWTPersistUnit.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          11/20/2023  HGM Initial Draft

   Last Changed =

*******************************************************************************/
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
      _dmsWTPersistUnit implement
    */
   _dmsWTPersistUnit::_dmsWTPersistUnit( dmsWTStorageEngine &engine )
   : _dmsPersistUnit(),
     _engine( engine )
   {
   }

   _dmsWTPersistUnit::~_dmsWTPersistUnit()
   {
      if ( dmsPersistUnitState::INACTIVE != _state )
      {
         INT32 tmpRC = _abortUnit( sdbGetThreadExecutor() ) ;
         PD_LOG( PDWARNING, "Failed to abort persist unit, rc: %d", tmpRC ) ;
      }
      _session.close() ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSWTPERSISTUNIT_INITUNIT, "_dmsWTPersistUnit::initUnit" )
   INT32 _dmsWTPersistUnit::initUnit( IExecutor *executor )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__DMSWTPERSISTUNIT_INITUNIT ) ;

      if ( !_session.isOpened() )
      {
         // only snapshot session can support write operations
         rc = _engine.openSession( _session, dmsWTSessIsolation::SNAPSHOT ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to open session, rc: %d", rc ) ;
      }

   done:
      PD_TRACE_EXITRC( SDB__DMSWTPERSISTUNIT_INITUNIT, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSWTPERSISTUNIT__BEGINUNIT, "_dmsWTPersistUnit::_beginUnit" )
   INT32 _dmsWTPersistUnit::_beginUnit( IExecutor *executor )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__DMSWTPERSISTUNIT__BEGINUNIT ) ;

      rc = _session.beginTrans() ;
      PD_RC_CHECK( rc, PDERROR, "Failed to begin transaction, rc: %d", rc ) ;

   done:
      PD_TRACE_EXITRC( SDB__DMSWTPERSISTUNIT__BEGINUNIT, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSWTPERSISTUNIT__PREPAREUNIT, "_dmsWTPersistUnit::_prepareUnit" )
   INT32 _dmsWTPersistUnit::_prepareUnit( IExecutor *executor )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__DMSWTPERSISTUNIT__PREPAREUNIT ) ;

      PD_CHECK( _session.isOpened(), SDB_SYS, error, PDERROR,
                "Failed to prepare transaction, session is not opened" ) ;

      rc = _session.prepareTrans() ;
      PD_RC_CHECK( rc, PDERROR, "Failed to prepare transaction, rc: %d", rc ) ;

   done:
      PD_TRACE_EXITRC( SDB__DMSWTPERSISTUNIT__PREPAREUNIT, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSWTPERSISTUNIT__COMMITUNIT, "_dmsWTPersistUnit::_commitUnit" )
   INT32 _dmsWTPersistUnit::_commitUnit( IExecutor *executor )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__DMSWTPERSISTUNIT__COMMITUNIT ) ;

      PD_CHECK( _session.isOpened(), SDB_SYS, error, PDERROR,
                "Failed to commit transaction, session is not opened" ) ;

      rc = _session.commitTrans() ;
      PD_RC_CHECK( rc, PDERROR, "Failed to commit transaction, rc: %d", rc ) ;

   done:
      PD_TRACE_EXITRC( SDB__DMSWTPERSISTUNIT__COMMITUNIT, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSWTPERSISTUNIT__ABORTUNIT, "_dmsWTPersistUnit::_abortUnit" )
   INT32 _dmsWTPersistUnit::_abortUnit( IExecutor *executor )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__DMSWTPERSISTUNIT__ABORTUNIT ) ;

      PD_CHECK( _session.isOpened(), SDB_SYS, error, PDERROR,
                "Failed to abort transaction, session is not opened" ) ;

      rc = _session.abortTrans() ;
      PD_RC_CHECK( rc, PDERROR, "Failed to abort transaction, rc: %d", rc ) ;

   done:
      PD_TRACE_EXITRC( SDB__DMSWTPERSISTUNIT__ABORTUNIT, rc ) ;
      return rc ;

   error:
      goto done ;
   }

}
}
