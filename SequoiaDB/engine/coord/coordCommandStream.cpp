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

   Source File Name = coordCommandStream.cpp

   Descriptive Name = Coord Stream Commands

   When/how to use: this program may be used on binary and text-formatted
   versions of runtime component. This file contains code logic for
   common functions for coordinator node.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          06/01/2023  HGM Initial Draft
   Last Changed =

*******************************************************************************/

#include "coordCommandStream.hpp"
#include "ossErr.h"
#include "pdTrace.hpp"
#include "coordTrace.hpp"
#include "rtnCB.hpp"
#include "coordContextChangeStream.hpp"

namespace engine
{

   /*
      _coordCMDWatch implement
    */
   COORD_IMPLEMENT_CMD_AUTO_REGISTER( _coordCMDWatch,
                                      CMD_NAME_WATCH,
                                      TRUE ) ;
   _coordCMDWatch::_coordCMDWatch()
   : _coordCommandBase()
   {
   }

   _coordCMDWatch::~_coordCMDWatch()
   {
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_COORDCMDWATCH_EXECUTE, "_coordCMDWatch::execute" )
   INT32 _coordCMDWatch::execute( MsgHeader *pMsg,
                                  pmdEDUCB *cb,
                                  INT64 &contextID,
                                  rtnContextBuf *buf )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_COORDCMDWATCH_EXECUTE ) ;

      SDB_RTNCB *rtnCB = pmdGetKRCB()->getRTNCB() ;
      rtnCoordContextChangeStream::sharePtr pContext ;
      contextID = -1 ;

      rc = rtnCB->contextNew( RTN_CONTEXT_COORD_CHANGE_STREAM, pContext, contextID, cb ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to create coord change stream context, rc: %d", rc ) ;

      rc = pContext->open( pMsg, _pResource, cb ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to open coord change stream context, rc: %d", rc ) ;

   done:
      PD_TRACE_EXITRC( SDB_COORDCMDWATCH_EXECUTE, rc ) ;
      return rc ;

   error:
      if ( -1 != contextID )
      {
         rtnCB->contextDelete( contextID, cb ) ;
         contextID = -1 ;
      }
      goto done ;
   }

}
