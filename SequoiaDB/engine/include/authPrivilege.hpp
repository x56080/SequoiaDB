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

   Source File Name = authPrivilege.hpp

   Descriptive Name = 

   When/how to use: this program may be used on backup or restore db data.
   You can specfiy some options from parameters.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          07/14/2023  ZHY Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef AUTH_PRIVILEGE_HPP__
#define AUTH_PRIVILEGE_HPP__

#include "authResource.hpp"
#include "authActionSet.hpp"
#include <vector>

namespace engine
{
   class _authPrivilege : public SDBObject
   {
      public:
         _authPrivilege() {}
         _authPrivilege( const boost::shared_ptr< authResource > &resource,
                         const boost::shared_ptr< authActionSet > &actions );
         ~_authPrivilege();

         const boost::shared_ptr<authResource> &getResource() const;
         const boost::shared_ptr<authActionSet> &getActionSet() const;

      private:
         boost::shared_ptr<authResource> _resource;
         boost::shared_ptr<authActionSet> _actionSet;
   };
   typedef _authPrivilege authPrivilege;

   const authActionSet *authGetActionSetMask( RESOURCE_TYPE type );
} // namespace engine

#endif