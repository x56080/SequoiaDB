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

   Source File Name = sptUsrTpAssit.cpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          10/30/2019  HGM Initial Draft

   Last Changed =

*******************************************************************************/

#include "sptUsrTpAssit.hpp"
#include "client.h"
#include "client_internal.h"
#include "pd.hpp"
#include "msgDef.h"
#include "ossUtil.h"
#include "tpToolCommon.hpp"
#include <string>

using namespace std ;

namespace engine
{

   /*
      _sptUsrTpAssit implement
    */
   _sptUsrTpAssit::_sptUsrTpAssit()
   : _handle( 0 )
   {
   }

   _sptUsrTpAssit::~_sptUsrTpAssit()
   {
   }

   INT32 _sptUsrTpAssit::connect( const CHAR *hostName,
                                  const CHAR *serviceName )
   {
      INT32 rc = SDB_OK ;

      // disconnect before connect
      rc = disconnect() ;
      PD_RC_CHECK( rc, PDERROR, "Failed to disconnect" ) ;

      rc = sdbConnect( hostName, serviceName, SDB_TP_USER, SDB_TP_USERPASSWD,
                       &_handle ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to connect to %s:%s, rc: %d",
                   hostName, serviceName, rc ) ;
   done:
      return rc ;

   error:
      disconnect() ;
      goto done ;
   }

   INT32 _sptUsrTpAssit::disconnect()
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

   INT32 _sptUsrTpAssit::runCommand( const CHAR *command,
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
      PD_RC_CHECK( rc, PDERROR, "Faield to run command [%s] in TP node, "
                   "rc: %d", command, rc ) ;

   done:
      return rc ;

   error:
      goto done ;
   }

}
