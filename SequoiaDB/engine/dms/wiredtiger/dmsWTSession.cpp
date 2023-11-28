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
   INT32 _dmsWTSession::open( WT_CONNECTION *conn )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__DMSWTSESSION_OPEN ) ;

      PD_CHECK( nullptr != conn, SDB_SYS, error, PDERROR,
                "Failed to open WiredTiger session, connection is not opened" ) ;
      PD_CHECK( nullptr == _session, SDB_SYS, error, PDERROR,
                "Failed to open WiredTiger session, already opened" ) ;

      rc = WT_CALL( conn->open_session( conn, nullptr, nullptr, &_session ),
                    nullptr ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to open WiredTiger session, "
                   "rc: %d", rc ) ;

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
         PD_LOG( PDWARNING, "Failed to close WiredTiger session, "
                  "rc: %d", rc ) ;
      }
      _session = nullptr ;

   done:
      PD_TRACE_EXITRC( SDB__DMSWTSESSION_CLOSE, rc ) ;

      return rc ;
   }

}
}
