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

   Source File Name = authRBAC.hpp

   Descriptive Name = Role based access control header file.

   When/how to use: this program may be used on binary and text-formatted
   versions of data management component. This file contains structure for
   DMS storage unit and its methods.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          19/09/2022  YSD Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef AUTH_RBAC_HPP__
#define AUTH_RBAC_HPP__

#include "core.hpp"
#include "authRBACGen.hpp"
namespace engine
{
   namespace oldRole
   {
      UINT32 authGetBuiltinRoleID( const CHAR *roleName ) ;
   }
   BOOLEAN authIsMonCmd( const CHAR *cmdName ) ;

   typedef ACTION_TYPE_ENUM ACTION_TYPE;
   typedef RESOURCE_TYPE_ENUM RESOURCE_TYPE;
   const int ACTION_SET_SIZE = ACTION_TYPE_VALID_NUM_GEN;
}

#endif /* AUTH_RBAC_HPP__ */
