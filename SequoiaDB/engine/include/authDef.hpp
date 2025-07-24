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

   Source File Name = authDef.hpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          07/12/2012  YW  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef AUTHDEF_HPP_
#define AUTHDEF_HPP_

#include "core.hpp"

namespace engine
{
   #define AUTH_SPACE                     "SYSAUTH"
   #define AUTH_USR_COLLECTION            AUTH_SPACE ".SYSUSRS"
   /// AUTH_USR_COLLECTION SCHEMA
   /// {User:"", Passwd:""}

   #define AUTH_USR_INDEX_NAME            "usrindex"

   #define AUTH_INVALID_ROLE_ID           0xFFFFFFFF

   // User has no role
   #define AUTH_NULL_ROLE_ID              0xFFFFFFFE

   #define AUTH_ROLE_NAME_SZ              127
   #define AUTH_ROLE_ADMIN_NAME           "admin"
   #define AUTH_ROLE_MONITOR_NAME         "monitor"

   // Builtin role ids
   // Note: NEVER change the value of existing enum item, as they are used as
   //       the ids of builtin roles.
   enum _authBuiltinRoleID
   {
      AUTH_ROLE_ADMIN   = 0,           // can do everything
      AUTH_ROLE_MONITOR = 1            // can do nothing except snapshot and list
   } ;
   typedef _authBuiltinRoleID authBuiltinRoleID ;
}

#endif

