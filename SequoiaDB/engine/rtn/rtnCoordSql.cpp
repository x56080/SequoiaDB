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

   Source File Name = rtnCoordSql.cpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          07/12/2012  YW  Initial Draft

   Last Changed =

*******************************************************************************/

#include "rtnCoordSql.hpp"
#include "pmd.hpp"
#include "pmdCB.hpp"
#include "rtnCommandDef.hpp"
#include "msgMessage.hpp"

namespace engine
{
   INT32 _rtnCoordSql::execute( MsgHeader *pMsg,
                                pmdEDUCB *cb,
                                INT64 &contextID,
                                rtnContextBuf *buf )

   {
      INT32 rc          = SDB_OK ;
      SQL_CB *sqlcb     = pmdGetKRCB()->getSqlCB() ;
      CHAR *sql         = NULL ;

      contextID         = -1 ;

      rc = msgExtractSql( (CHAR*)pMsg, &sql ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to extract sql" ) ;
         rc = SDB_SYS ;
         goto error ;
      }

      // add last op info
      MON_SAVE_OP_DETAIL( cb->getMonAppCB(), pMsg->opCode,
                          "%s", sql ) ;

      rc = sqlcb->exec( sql, cb, contextID ) ;

   done:
      return rc ;
   error:
      goto done ;
   }
}
