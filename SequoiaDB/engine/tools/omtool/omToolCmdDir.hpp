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

   Source File Name = omToolCmdDir.hpp

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

#define OM_TOOL_MODE_CREATE_DIR  "createdir"

namespace omTool
{
   class omToolCmdDir : public omToolCmdBase
   {
   DECLARE_OMTOOL_CMD_AUTO_REGISTER() ;

   public:
      omToolCmdDir() ;
      ~omToolCmdDir() ;

      INT32 doCommand() ;

      const CHAR *name(){ return OM_TOOL_MODE_CREATE_DIR ; }

   private:
      INT32 _check( const string &path, const string &user ) ;
      INT32 _setDirOwnership( const CHAR *path ) ;
      INT32 _setParentDirPermissions( const CHAR *path ) ;
      INT32 _mkdir( const CHAR *path ) ;
      INT32 _chmod( const CHAR *path, UINT32 iPermission ) ;
      INT32 _exist( const CHAR *path, BOOLEAN &isExist ) ;
      INT32 _isEmpty( const CHAR *path ) ;

   private:
      OSSUID _uid ;
      OSSGID _gid ;
   } ;
}


#endif /* OMT_CMD_HOST_HPP_ */