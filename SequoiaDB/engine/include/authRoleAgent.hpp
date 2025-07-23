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

   Source File Name = authRoleAgent.hpp

   Descriptive Name = N/A

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who         Description
   ====== =========== =========== =============================================
          23/07/2025  fangjiabin  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef AUTH_ROLE_AGENT_HPP__
#define AUTH_ROLE_AGENT_HPP__

#include "ossTypes.hpp"
#include "../bson/bson.hpp"

namespace engine
{
   typedef class _pmdEDUCB pmdEDUCB;
   class _authRoleAgent : public SDBObject
   {
   public:
      virtual ~_authRoleAgent(){};
      virtual INT32 getRole( const CHAR *roleName, bson::BSONObj &out ) = 0;
      virtual INT32 listRoles( ossPoolMap< ossPoolString, bson::BSONObj > &roles ) = 0;
      virtual INT32 createRole( const bson::BSONObj &obj ) = 0;
      virtual INT32 dropRole( const CHAR *roleName ) = 0;
      virtual INT32 updateRole( const CHAR *roleName, const bson::BSONObj &obj ) = 0;
      virtual INT32 grantRolesToRole( const CHAR *roleName, const bson::BSONObj &obj ) = 0;
      virtual INT32 revokeRolesFromRole( const CHAR *roleName, const bson::BSONObj &obj ) = 0;
      virtual INT32 grantRolesToUser( const CHAR *userName, const bson::BSONObj &obj ) = 0;
      virtual INT32 revokeRolesFromUser( const CHAR *userName, const bson::BSONObj &obj ) = 0;
      virtual INT32 getUser( const CHAR *userName, bson::BSONObj &out ) = 0;
   };
   typedef _authRoleAgent authRoleAgent;
} // namespace engine
#endif