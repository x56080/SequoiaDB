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

