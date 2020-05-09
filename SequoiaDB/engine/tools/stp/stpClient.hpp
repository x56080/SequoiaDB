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

#ifndef STP_CLIENT_HPP__
#define STP_CLIENT_HPP__

#include "oss.hpp"
#include "stpClientInternal.hpp"
#include "../bson/bson.h"

namespace engine
{

   /*
      _stpClient define
    */
   // client to send commands and handle reply with STP nodes
   // command and reply are in BSON format
   class _stpClient : public stpClientInternal
   {
   public:
      _stpClient() ;
      ~_stpClient() ;

   public:
      // run command on STP node to get back BSON object
      // input:
      // - command: command to be executed
      // - argument: argument to be executed in BSON format
      // output:
      // - returnObject: result of command in BSON format
      // - returnCode: return code of command
      // return:
      // - SDB_OK: succeed to run command
      // - other error code: failed to run command
      INT32 runCommand( const CHAR *command,
                        const bson::BSONObj &argument,
                        bson::BSONObj &returnObject,
                        INT32 &returnCode ) ;
   } ;

   typedef class _stpClient stpClient ;

}

#endif // STP_CLIENT_HPP__
