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

   Source File Name = tpServerSession.cpp

   Descriptive Name = SequoiaDB Time Protocol Service

   When/how to use: this program may be used on binary and text-formatted
   versions of Runtime component. This file contains structure for SequoiaDB
   Time Protocol Service.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          07/30/2019  HGM Initial Draft

   Last Changed =

*******************************************************************************/

#include "tpServerSession.hpp"
#include "tpCB.hpp"
#include "pdTrace.hpp"
#include "tpTrace.hpp"

namespace engine
{

   /*
      _tpServerSession implement
    */
   _tpServerSession::_tpServerSession( SDB_TPCB *tpCB )
   : _catalogManager( tpCB->getCatalogManager() )
   {
      _sourceRID.value = MSG_INVALID_ROUTEID ;
   }

   _tpServerSession::~_tpServerSession()
   {
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__TPSERVERSESSION_GETPRIMARYRID, "_tpServerSession::getPrimaryRID" )
   INT32 _tpServerSession::getPrimaryRID( MsgRouteID &routeID )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__TPSERVERSESSION_GETPRIMARYRID ) ;

      rc = _catalogManager->getServerRID( _sourceRID, TRUE ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to get primary server route ID, "
                   "rc: %d", rc ) ;

      routeID.value = _sourceRID.value ;

   done :
      PD_TRACE_EXITRC( SDB__TPSERVERSESSION_GETPRIMARYRID, rc ) ;
      return rc ;

   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__TPSERVERSESSION_GETSERVERRID, "_tpServerSession::getServerRID" )
   INT32 _tpServerSession::getServerRID( MsgRouteID &routeID )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__TPSERVERSESSION_GETSERVERRID ) ;

      rc = _catalogManager->getServerRID( _sourceRID, FALSE ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to get server route ID, "
                   "rc: %d", rc ) ;

   done :
      PD_TRACE_EXITRC( SDB__TPSERVERSESSION_GETSERVERRID, rc ) ;
      return rc ;

   error :
      goto done ;
   }

}
