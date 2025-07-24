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

   Source File Name = rtnCoordAuthCrt.cpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          07/12/2012  YW  Initial Draft

   Last Changed =

*******************************************************************************/
#include "rtnCoordAuthCrt.hpp"
#include "msgMessageFormat.hpp"
#include "pdTrace.hpp"
#include "rtnTrace.hpp"

using namespace bson ;

namespace engine
{
   // PD_TRACE_DECLARE_FUNCTION ( SDB_RTNCOAUTHCRT_EXECUTE, "rtnCoordAuthCrt::execute" )
   INT32 rtnCoordAuthCrt::execute( MsgHeader *pMsg,
                                   pmdEDUCB *cb,
                                   INT64 &contextID,
                                   rtnContextBuf *buf )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB_RTNCOAUTHCRT_EXECUTE ) ;
      const CHAR *pUserName = NULL ;
      const CHAR *pPassWord = NULL ;
      BSONObj options ;

      rc = forward( pMsg, cb, MSG_AUTH_CRTUSR_RES,
                    FALSE, contextID,
                    &pUserName, &pPassWord, &options ) ;
      if ( pUserName )
      {
         /// AUDIT
         PD_AUDIT_OP( AUDIT_DCL, pMsg->opCode, AUDIT_OBJ_USER,
                      pUserName, rc, "Options:%s",
                      options.toString().c_str() ) ;
      }
      if ( rc )
      {
         goto error ;
      }
      else if ( *cb->getUserName() == '\0' )
      {
         cb->setUserInfo( pUserName, pPassWord ) ;
         updateSessionByOptions( options ) ;
      }

   done:
      PD_TRACE_EXITRC ( SDB_RTNCOAUTHCRT_EXECUTE, rc ) ;
      return rc ;
   error:
      goto done ;
   }
}
