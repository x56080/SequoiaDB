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

   Source File Name = omToolCmdHost.hpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          08/09/2019  HJW  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef OMTOOL_CMD_HOST_HPP_
#define OMTOOL_CMD_HOST_HPP_

#include "omToolCmdBase.hpp"

#define OM_TOOL_MODE_ADD_HOST  "addhost"
#define OM_TOOL_MODE_DEL_HOST  "delhost"

namespace omTool
{
   class omToolAddHost : public omToolCmdBase
   {
   DECLARE_OMTOOL_CMD_AUTO_REGISTER() ;

   public:
      omToolAddHost() ;

      ~omToolAddHost() ;

      INT32 doCommand() ;

      const CHAR *name(){ return OM_TOOL_MODE_ADD_HOST ; }
   } ;

   class omToolDelHost : public omToolCmdBase
   {
   DECLARE_OMTOOL_CMD_AUTO_REGISTER();

   public:
      omToolDelHost() ;
      ~omToolDelHost() ;

      INT32 doCommand() ;

      const CHAR *name(){ return OM_TOOL_MODE_DEL_HOST ; }
   } ;
}


#endif /* OMT_CMD_HOST_HPP_ */