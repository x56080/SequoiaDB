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
