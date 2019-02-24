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

   Source File Name = rtnCoordAuthDel.cpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          07/12/2012  YW  Initial Draft

   Last Changed =

*******************************************************************************/

#include "rtnCoordAuthDel.hpp"
#include "msgMessageFormat.hpp"
#include "pdTrace.hpp"
#include "rtnTrace.hpp"

namespace engine
{
   // PD_TRACE_DECLARE_FUNCTION ( SDB_RTNCOAUTHDEL_EXECUTE, "rtnCoordAuthDel::execute" )
   INT32 rtnCoordAuthDel::execute( MsgHeader *pMsg,
                                   pmdEDUCB *cb,
                                   INT64 &contextID,
                                   rtnContextBuf *buf )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB_RTNCOAUTHDEL_EXECUTE ) ;
      const CHAR *pUserName = NULL ;

      rc = forward( pMsg, cb, MSG_AUTH_DELUSR_RES,
                    FALSE, contextID,
                    &pUserName ) ;
      if ( pUserName )
      {
         /// AUDIT
         PD_AUDIT_OP( AUDIT_DCL, pMsg->opCode, AUDIT_OBJ_USER,
                      pUserName, rc, "" ) ;
      }
      if ( rc )
      {
         goto error ;
      }
      else if ( 0 == ossStrcmp( pUserName, cb->getUserName() ) )
      {
         cb->setUserInfo( "", "" ) ;
      }

   done:
      PD_TRACE_EXITRC ( SDB_RTNCOAUTHDEL_EXECUTE, rc ) ;
      return rc ;
   error:
      goto done ;
   }
}
