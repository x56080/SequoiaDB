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

   Source File Name = authAccessControlList.hpp

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
#ifndef AUTH_ACL_HPP__
#define AUTH_ACL_HPP__

#include "ossTypes.hpp"
#include "authPrivilege.hpp"
#include "utilSharedPtrHelper.hpp"

namespace engine
{
   class _authAccessControlList : public SDBObject
   {
      typedef ossPoolMap< boost::shared_ptr< authResource >,
                          boost::shared_ptr< authActionSet >,
                          SHARED_TYPE_LESS< authResource > >
         DATA_TYPE;

   public:
      _authAccessControlList() {}
      ~_authAccessControlList() {}

      INT32 addPrivilege( const authPrivilege &p );
      INT32 addPrivilege( const bson::BSONObj & );

      INT32 removePrivilege( const authPrivilege &p );
      INT32 removePrivilege( const bson::BSONObj & );

      BOOLEAN isAuthorizedForPrivilege( const authPrivilege &p ) const;
      BOOLEAN isAuthorizedForActionsOnResource( const authResource &res,
                                                const authActionSet &actions ) const;

      void toBSONArray( bson::BSONArrayBuilder &builder ) const;

   private:
      DATA_TYPE _data;
   };
   typedef _authAccessControlList authAccessControlList;
} // namespace engine

#endif