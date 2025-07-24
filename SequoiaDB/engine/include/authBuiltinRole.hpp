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

   Source File Name = authBuiltinRole.hpp

   Descriptive Name = 

   When/how to use: this program may be used on binary and text-formatted
   version of runtime component. This file contains code logic for
   data delete request from coordinator node.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          07/27/2023  ZHY Initial Draft
   Last Changed =

*******************************************************************************/
#include "authPrivilege.hpp"

namespace engine
{
   class _authBuiltinRole
   {
   public:
      _authBuiltinRole();
      ~_authBuiltinRole();

      void set( authPrivilege *privileges, INT32 size, BOOLEAN needDelete );
      authPrivilege* getPrivileges();
      INT32 getSize();
      void toBSONObj( BOOLEAN showPrivileges, bson::BSONObjBuilder &builder );

   private:
      authPrivilege *_privileges;
      INT32 _size;
      BOOLEAN _needDelete;
   };
   typedef _authBuiltinRole authBuiltinRole;
}; // namespace engine