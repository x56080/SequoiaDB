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

   Source File Name = auth.hpp

   Descriptive Name = 

   When/how to use: this program may be used on backup or restore db data.
   You can specfiy some options from parameters.

   Dependencies: N/A;

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          07/18/2023  ZHY Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef AUTH_HPP__
#define AUTH_HPP__

#include "authAccessControlList.hpp"
#include "authRequiredPrivileges.hpp"
#include "authBuiltinRole.hpp"
namespace engine
{
   ossPoolString authGetBuiltinRoleFromOldRole( const CHAR *oldRoleName );
   INT32 authMeetRequiredPrivileges( const authRequiredPrivileges &requiredPrivileges,
                                     const authAccessControlList &acl );
   INT32 authMeetActionsOnCluster( const authActionSet &actions, const authAccessControlList &acl );
   INT32 authMeetActionsOnExact( const CHAR *pFullName,
                                 const authActionSet &actions,
                                 const authAccessControlList &acl );
   
   // return FALSE if the role is not a built-in role
   BOOLEAN authIsBuiltinRole( const CHAR *roleName );
   INT32 authGetBuiltinRole( const CHAR *roleName, authBuiltinRole &builtinRole );
   void authInitBuiltinRolePrivileges();
   INT32 authGetBuiltinRoleBsonVec( BOOLEAN showPrivileges, ossPoolVector< bson::BSONObj > &out );
}

#endif