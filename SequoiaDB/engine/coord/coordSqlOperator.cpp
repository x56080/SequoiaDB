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

   Source File Name = coordSqlOperator.cpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          05/02/2017  XJH Initial Draft

   Last Changed =

*******************************************************************************/
#include "coordSqlOperator.hpp"
#include "pmd.hpp"
#include "sqlCB.hpp"
#include "msgMessage.hpp"
#include "rtnCommandDef.hpp"
#include "pdTrace.hpp"
#include "coordTrace.hpp"

using namespace bson ;

namespace engine
{

   /*
      _coordSqlOperator implement
   */
   _coordSqlOperator::_coordSqlOperator()
   {
      _needRollback = FALSE ;
   }

   _coordSqlOperator::~_coordSqlOperator()
   {
   }

   const CHAR* _coordSqlOperator::getName() const
   {
      return "Sql" ;
   }

   BOOLEAN _coordSqlOperator::needRollback() const
   {
      return _needRollback ;
   }

   INT32 _coordSqlOperator::execute( MsgHeader *pMsg,
                                     pmdEDUCB *cb,
                                     INT64 &contextID,
                                     rtnContextBuf *buf )

   {
      INT32 rc          = SDB_OK ;
      SQL_CB *sqlcb     = pmdGetKRCB()->getSqlCB() ;
      const CHAR *sql   = NULL ;
      BSONObjBuilder retBuilder ;

      contextID         = -1 ;

      rc = msgExtractSql( (const CHAR*)pMsg, &sql ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to extract sql" ) ;
         rc = SDB_SYS ;
         goto error ;
      }

      MONQUERY_SET_NAME( cb, sql ) ;

      // add last op info
      MON_SAVE_OP_DETAIL( cb->getMonAppCB(), pMsg->opCode,
                          "%s", sql ) ;

      MONQUERY_SET_QUERY_TEXT( cb, cb->getMonAppCB()->_lastOpDetail ) ;

      rc = sqlcb->exec( sql, cb, contextID, _needRollback, &retBuilder ) ;
      if ( rc )
      {
         goto error ;
      }

   done:
      if ( !retBuilder.isEmpty() )
      {
         *buf = rtnContextBuf( retBuilder.obj() ) ;
      }
      return rc ;
   error:
      if ( !retBuilder.isEmpty() )
      {
         utilBuildErrorBson( retBuilder, rc, cb->getInfo( EDU_INFO_ERROR ) ) ;
      }
      goto done ;
   }

}

