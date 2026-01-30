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

   Source File Name = msgAuth.cpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          07/12/2012  YW  Initial Draft

   Last Changed =

*******************************************************************************/
#include "core.hpp"
#include "msgAuth.hpp"
#include "msgMessage.hpp"
#include "pd.hpp"
#include "pdTrace.hpp"
#include "msgTrace.hpp"
#include "msgDef.h"

using namespace bson ;

namespace engine
{
   // PD_TRACE_DECLARE_FUNCTION ( SDB_EXTRACTAUTHMSG, "extractAuthMsg" )
   INT32 extractAuthMsg( MsgHeader *header, BSONObj &obj )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB_EXTRACTAUTHMSG );
      CHAR *offset = NULL ;
      if ( NULL == header )
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }
      offset = ( CHAR *)header + sizeof(MsgHeader) ;
      try
      {
         BSONObj tmp( offset ) ;
         obj = tmp ;
      }
      catch (std::exception &e)
      {
         PD_LOG( PDERROR, "unexpected err:%s", e.what() ) ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }
   done:
      PD_TRACE_EXITRC ( SDB_EXTRACTAUTHMSG, rc );
      return rc ;
   error:
      goto done ;
   }

   INT32 msgBuildAuthMsg( CHAR **ppBuffer, INT32 *bufferSize,
                          const CHAR *username,
                          const CHAR *password,
                          UINT64 reqID,
                          IExecutor *cb )
   {
      INT32 rc = SDB_OK ;
      INT32 msgLen = 0 ;
      BSONObj obj ;
      MsgAuthentication *msg ;
      if ( NULL == username || NULL == password )
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }
      obj = BSON( SDB_AUTH_USER << username <<
                  SDB_AUTH_PASSWD << password ) ;

      msgLen = sizeof( MsgAuthentication ) + obj.objsize() ;

      rc = msgCheckBuffer( ppBuffer, bufferSize, msgLen, cb ) ;
      if ( rc )
      {
         PD_LOG ( PDERROR, "Failed to check buffer, rc: %d", rc ) ;
         goto error ;
      }

      msg                       = ( MsgAuthentication * )(*ppBuffer) ;
      msg->header.eye           = MSG_COMM_EYE_DEFAULT ;
      msg->header.requestID     = reqID ;
      msg->header.opCode        = MSG_AUTH_VERIFY_REQ ;
      msg->header.messageLength = msgLen ;
      msg->header.routeID.value = 0 ;
      msg->header.TID           = ossGetCurrentThreadID() ;
      msg->header.version       = SDB_PROTOCOL_VER_CUR ;
      msg->header.flags         = 0 ;
      ossMemset( &(msg->header.globalID), 0, sizeof(msg->header.globalID) ) ;
      ossMemset( msg->header.reserve, 0, sizeof(msg->header.reserve) ) ;

      ossMemcpy( *ppBuffer + sizeof(MsgAuthentication),
                 obj.objdata(), obj.objsize() ) ;

   done:
      return rc ;
   error:
      goto done ;
   }

}
