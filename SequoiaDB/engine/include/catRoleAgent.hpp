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

   Source File Name = catRoleAgent.hpp

   Descriptive Name = N/A

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who         Description
   ====== =========== =========== =============================================
          23/07/2025  fangjiabin  Initial Draft

   Last Changed =

*******************************************************************************/
#include "authRoleAgent.hpp"
#include "pmdEDU.hpp"
#include "rtnCB.hpp"
#include "dmsCB.hpp"

namespace engine
{
   class _catRoleAgent : public authRoleAgent
   {
   public:
      _catRoleAgent( pmdEDUCB *cb, SDB_RTNCB *rtnCB, SDB_DMSCB *dmsCB );
      ~_catRoleAgent();

      INT32 getRole( const CHAR *roleName, bson::BSONObj &out );
      INT32 listRoles( ossPoolMap< ossPoolString, bson::BSONObj > &roles );
      INT32 createRole( const bson::BSONObj &obj );
      INT32 dropRole( const CHAR *roleName );
      INT32 updateRole( const CHAR *roleName, const bson::BSONObj &obj );
      INT32 grantRolesToRole( const CHAR *roleName, const bson::BSONObj &obj );
      INT32 revokeRolesFromRole( const CHAR *roleName, const bson::BSONObj &obj );
      INT32 grantRolesToUser( const CHAR *userName, const bson::BSONObj &obj );
      INT32 revokeRolesFromUser( const CHAR *userName, const bson::BSONObj &obj );
      INT32 getUser( const CHAR *userName, bson::BSONObj &out );

   private:
      INT32 _checkRevokeLastRoot( const CHAR *userName, const bson::BSONObj &obj );

   private:
      pmdEDUCB *_pEduCB;
      SDB_RTNCB *_pRtnCB;
      SDB_DMSCB *_pDmsCB;
   };
   typedef _catRoleAgent catRoleAgent;
} // namespace engine