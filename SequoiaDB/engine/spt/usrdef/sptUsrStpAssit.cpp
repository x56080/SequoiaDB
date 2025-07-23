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

   Source File Name = sptUsrStpAssit.cpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          10/30/2019  HGM Initial Draft

   Last Changed =

*******************************************************************************/
#include "sptUsrStpAssit.hpp"
#include "client.h"
#include "client_internal.h"
#include "pd.hpp"
#include "msgDef.h"
#include "ossUtil.h"
#include "stpToolCommon.hpp"
#include <string>

using namespace std ;

namespace engine
{

   /*
      _sptUsrStpAssit implement
    */
   _sptUsrStpAssit::_sptUsrStpAssit()
   : _handle( 0 )
   {
   }

   _sptUsrStpAssit::~_sptUsrStpAssit()
   {
   }

   INT32 _sptUsrStpAssit::connect( const CHAR *hostName,
                                   const CHAR *serviceName )
   {
      INT32 rc = SDB_OK ;

      // disconnect before connect
      rc = disconnect() ;
      PD_RC_CHECK( rc, PDERROR, "Failed to disconnect" ) ;

      rc = sdbConnect( hostName, serviceName, STP_USER, STP_USERPASSWD,
                       &_handle ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to connect to %s:%s, rc: %d",
                   hostName, serviceName, rc ) ;
   done:
      return rc ;

   error:
      disconnect() ;
      goto done ;
   }

   INT32 _sptUsrStpAssit::disconnect()
   {
      INT32 rc = SDB_OK ;

      if ( 0 != _handle )
      {
         sdbDisconnect( (sdbConnectionHandle)_handle ) ;
         sdbReleaseConnection( (sdbConnectionHandle) _handle ) ;
         _handle = 0 ;
      }

      return rc ;
   }

   INT32 _sptUsrStpAssit::runCommand( const CHAR *command,
                                      const CHAR *argument,
                                      CHAR **returnBuffer,
                                      INT32 &returnCode,
                                      BOOLEAN needResult )
   {
      INT32 rc = SDB_OK ;

      string commandString ;

      PD_CHECK( NULL != command, SDB_INVALIDARG, error, PDERROR,
                "Failed to run command, command name is invalid" ) ;

      PD_CHECK( 0 != _handle, SDB_NETWORK, error, PDERROR,
                "Failed to run command [%s] on invalid handle", command ) ;

      commandString += CMD_ADMIN_PREFIX ;
      commandString += command ;

      rc = _remote.runCommand( _handle,
                               commandString.c_str(),
                               0, 0, -1, -1,
                               argument, NULL, NULL, NULL,
                               returnBuffer, returnCode, needResult ) ;
      PD_RC_CHECK( rc, PDERROR, "Faield to run command [%s] in STP node, "
                   "rc: %d", command, rc ) ;

   done:
      return rc ;

   error:
      goto done ;
   }

}
