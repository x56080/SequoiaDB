/*******************************************************************************


   Copyright (C) 2011-2018 SequoiaDB Ltd.

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

   http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.

   Source File Name = sptUsrRemoteAssit.cpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          18/07/2016  WJM Initial Draft

   Last Changed =

*******************************************************************************/
#include "sptUsrRemoteAssit.hpp"
#include "client.h"
#include "client_internal.h"
#include "pd.hpp"
#include "msgDef.h"
#include "ossUtil.h"
#include "ossTypes.h"
#include "omagentDef.hpp"
#include "network.h"
#include "common.h"
#include "sptRemote.hpp"

#if defined( _LINUX ) || defined (_AIX)
#include <arpa/inet.h>
#include <netinet/tcp.h>
#endif


#define SDB_CLIENT_DFT_NETWORK_TIMEOUT 10000

namespace engine
{

/*
   define member functions
*/

   _sptUsrRemoteAssit::_sptUsrRemoteAssit()
   {
      _handle = 0 ;
   }

   _sptUsrRemoteAssit::~_sptUsrRemoteAssit()
   {
      disconnect() ;
   }

   INT32 _sptUsrRemoteAssit::connect( const CHAR *pHostName,
                                      const CHAR *pServiceName )
   {
      INT32 rc = SDB_OK ;

      // disconnect before connect
      rc = disconnect() ;
      PD_RC_CHECK( rc, PDERROR, "Failed to disconnect" ) ;

      rc = sdbConnect( pHostName, pServiceName, SDB_OMA_USER,
                       SDB_OMA_USERPASSWD, &_handle ) ;
      PD_RC_CHECK( rc, PDERROR, "Connect to %s:%s failed, rc: %d",
                   pHostName, pServiceName, rc ) ;
   done:
      return rc ;
   error:
      disconnect() ;
      goto done ;
   }

   INT32 _sptUsrRemoteAssit::disconnect()
   {
      if ( 0 != _handle )
      {
         sdbDisconnect ( (sdbConnectionHandle)_handle ) ;
         sdbReleaseConnection( (sdbConnectionHandle) _handle ) ;
         _handle = 0 ;
      }

      return SDB_OK ;
   }

   INT32 _sptUsrRemoteAssit::runCommand( string command,
                                         const CHAR *arg1,
                                         CHAR **ppRetBuffer,
                                         INT32 &retCode,
                                         BOOLEAN needRecv )
   {
      INT32 rc = SDB_OK ;

      if ( 0 == _handle )
      {
         rc = SDB_NETWORK ;
         goto error ;
      }
      rc = _remote.runCommand( _handle,
                               ( CMD_ADMIN_PREFIX + command ).c_str(),
                               0, 0, -1, -1,
                               arg1, NULL, NULL, NULL,
                               ppRetBuffer, retCode, needRecv ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "Failed to run command: %s, rc: %d",
                 command.c_str(), rc ) ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

}
