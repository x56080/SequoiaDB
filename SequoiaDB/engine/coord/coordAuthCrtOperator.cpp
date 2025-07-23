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

   Source File Name = coordAuthCrtOperator.cpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          04/18/2017  XJH Initial Draft

   Last Changed =

*******************************************************************************/
#include "coordAuthCrtOperator.hpp"
#include "msgMessageFormat.hpp"
#include "pdTrace.hpp"
#include "coordTrace.hpp"
#include "auth.hpp"

using namespace bson ;
using namespace boost ;

namespace engine
{
   /*
      _coordAuthCrtOperator implement
   */
   _coordAuthCrtOperator::_coordAuthCrtOperator()
   {
   }

   _coordAuthCrtOperator::~_coordAuthCrtOperator()
   {
   }

   const CHAR* _coordAuthCrtOperator::getName() const
   {
      return "AuthCreate" ;
   }

   BOOLEAN _coordAuthCrtOperator::isReadOnly() const
   {
      return FALSE ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( COORD_AUTHCRTOPR_EXE, "_coordAuthCrtOperator::execute" )
   INT32 _coordAuthCrtOperator::execute( MsgHeader *pMsg,
                                         pmdEDUCB *cb,
                                         INT64 &contextID,
                                         rtnContextBuf *buf )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( COORD_AUTHCRTOPR_EXE ) ;
      const CHAR *pUserName = NULL ;
      const CHAR *pPassWord = NULL ;
      BSONObj options ;

      if ( cb->getSession()->privilegeCheckEnabled() )
      {
         authActionSet actions;
         actions.addAction( ACTION_TYPE_createUsr );
         rc = cb->getSession()->checkPrivilegesForActionsOnCluster( actions );
         PD_RC_CHECK( rc, PDERROR, "Failed to check privileges" );
      }

      rc = forward( pMsg, cb, FALSE, contextID,
                    &pUserName, &pPassWord, &options, buf ) ;
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
      PD_TRACE_EXITRC ( COORD_AUTHCRTOPR_EXE, rc ) ;
      return rc ;
   error:
      goto done ;
   }

}
