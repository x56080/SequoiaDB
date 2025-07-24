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

   Source File Name = stpToolUtil.hpp

   Descriptive Name = Serial Time Protocol

   When/how to use: this program may be used on binary and text-formatted
   versions of STP component. This file contains structure for Serial Time
   Protocol.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/01/2019  HGM Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef STP_TOOL_UTIL_HPP__
#define STP_TOOL_UTIL_HPP__

#include "core.hpp"
#include "ossUtil.hpp"

#include <string>

namespace engine
{

   // start a STP node with specified config path and additional options
   INT32 stpStartNode( const CHAR *rootPath,
                       std::string configPath,
                       const std::string &options ) ;

   // remove STP related files
   INT32 stpRemoveFiles( const CHAR *stpPath ) ;

   // get local ip address
   INT32 stpGetLocalIP( UINT32 &ipAddress ) ;

   // get ip address from hostname
   INT32 stpGetIP( UINT32 &ipAddress, const CHAR *hostname ) ;

}

#endif // STP_TOOL_UTIL_HPP__
