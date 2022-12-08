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
