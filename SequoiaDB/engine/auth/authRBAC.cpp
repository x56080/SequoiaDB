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

   Source File Name = authRBAC.cpp

   Descriptive Name = Role based access control file.

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
#include "authRBAC.hpp"
#include "ossUtil.hpp"
#include "msgDef.hpp"
#include "authDef.hpp"

namespace engine
{
   // Prefixes/Complete commands for monitor operations. For a user of role
   // monitor, all these operations are allowed.
   // Note: Keep a blank after some prefix to distinguish from some possible
   // commands starting with the same prefix.
   static const CHAR *gAuthMonCmdPrefix[] = { CMD_ADMIN_PREFIX "snapshot ",
                                              CMD_ADMIN_PREFIX "list ",
                                              CMD_ADMIN_PREFIX "test ",
                                              CMD_ADMIN_PREFIX "get ",
                                              CMD_ADMIN_PREFIX "SNAPSHOT_",
                                              CMD_ADMIN_PREFIX "LIST_",

                                              CMD_ADMIN_PREFIX CMD_NAME_TRACE_STATUS,
                                              CMD_ADMIN_PREFIX CMD_NAME_SETSESS_ATTR,
                                              CMD_ADMIN_PREFIX CMD_NAME_EVAL };

#define AUTH_MON_CMD_PREFIX_NUM ( sizeof( gAuthMonCmdPrefix ) / sizeof( const CHAR * ) )

   namespace oldRole
   {
      UINT32 authGetBuiltinRoleID( const CHAR *roleName )
      {
         UINT32 id = AUTH_INVALID_ROLE_ID;

         if ( roleName )
         {
            if ( ossStrlen( roleName ) == 0 )
            {
               id = AUTH_NULL_ROLE_ID;
            }
            else if ( 0 == ossStrcmp( roleName, AUTH_ROLE_ADMIN_NAME ) )
            {
               id = AUTH_ROLE_ADMIN;
            }
            else if ( 0 == ossStrcmp( roleName, AUTH_ROLE_MONITOR_NAME ) )
            {
               id = AUTH_ROLE_MONITOR;
            }
         }

         return id;
      }
   } // namespace oldRole

   BOOLEAN authIsMonCmd( const CHAR *cmdName )
   {
      BOOLEAN result = FALSE;

      if ( cmdName )
      {
         for ( UINT16 i = 0; i < AUTH_MON_CMD_PREFIX_NUM; ++i )
         {
            if ( 0 == ossStrncasecmp( cmdName, gAuthMonCmdPrefix[ i ],
                                      ossStrlen( gAuthMonCmdPrefix[ i ] ) ) )
            {
               result = TRUE;
               break;
            }
         }
      }

      return result;
   }
} // namespace engine
