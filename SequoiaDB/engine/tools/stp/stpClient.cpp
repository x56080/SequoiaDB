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

   Source File Name = stpClient.hpp

   Descriptive Name = Serial Time Protocol

   When/how to use: this program may be used on binary and text-formatted
   versions of STP component. This file contains structure for Serial Time
   Protocol.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          07/30/2019  HGM Initial Draft

   Last Changed =

*******************************************************************************/

#include "stpClient.hpp"
#include "pd.hpp"
#include "pdTrace.hpp"
#include "stpTrace.hpp"
#include "pmd.hpp"

using namespace std ;
using namespace bson ;

namespace engine
{

   #define STP_CLIENT_DFT_NETWORK_TIMEOUT ( 10000 )

   /*
      _stpClient implement
    */
   _stpClient::_stpClient()
   : _stpClientInternal()
   {
   }

   _stpClient::~_stpClient()
   {
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__STPCLIENT_RUNCOMMAND, "_stpClient::runCommand" )
   INT32 _stpClient::runCommand( const CHAR *command,
                                 const BSONObj &argument,
                                 BSONObj &returnObject,
                                 INT32 &returnCode )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__STPCLIENT_RUNCOMMAND ) ;

      CHAR *returnBuffer = NULL ;

      // run command
      rc = stpClientInternal::runCommand( command,
                                          argument.objdata(),
                                          &returnBuffer,
                                          returnCode ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to run command [%s] in STP node, "
                   "rc: %d", command, rc ) ;

      // check if an error happened
      if ( SDB_OK == returnCode && NULL != returnBuffer )
      {
         try
         {
            returnObject = BSONObj( returnBuffer ) ;
         }
         catch ( exception &e )
         {
            PD_LOG( PDERROR, "Failed to parse BSON object from result, "
                    "error: %s", e.what() ) ;
            rc = SDB_SYS ;
            goto error ;
         }
      }

   done:
      PD_TRACE_EXITRC( SDB__STPCLIENT_RUNCOMMAND, rc ) ;
      return rc ;

   error:
      goto done ;
   }

}
