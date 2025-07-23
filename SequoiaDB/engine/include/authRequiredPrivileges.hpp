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

   Source File Name = authRequiredPrivileges.hpp

   Descriptive Name = N/A

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who         Description
   ====== =========== =========== =============================================
          23/07/2025  fangjiabin  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef AUTH_REQUIRED_PRIVILEGES_HPP__
#define AUTH_REQUIRED_PRIVILEGES_HPP__

#include "ossTypes.hpp"
#include "authResource.hpp"
#include "authActionSet.hpp"
#include "utilSharedPtrHelper.hpp"

namespace engine
{
   class _authRequiredPrivileges : public SDBObject
   {
   public:
      typedef ossPoolVector< std::pair<boost::shared_ptr< authResource >,
                             const authRequiredActionSets*> >
         DATA_TYPE;

      void addActionSetsOnSimpleType( const authRequiredActionSets *actionSets )
      {
         RESOURCE_TYPE type = actionSets->getResourceType();
         if( type == RESOURCE_TYPE_CLUSTER || type == RESOURCE_TYPE_ANY ||
             type == RESOURCE_TYPE_NON_SYSTEM )
         {
            _data.push_back(std::make_pair(authResource::forSimpleType(type), actionSets));
         }
      }

      void addActionSetsOnResource( const boost::shared_ptr< authResource >&resource,
                                    const authRequiredActionSets *actionSets )
      {
         _data.push_back( std::make_pair( resource, actionSets ) );
      }

      DATA_TYPE &getWritableData()
      {
         return _data;
      }

      const DATA_TYPE &getData() const
      {
         return _data;
      }

   private:
      DATA_TYPE _data;
   };
   typedef _authRequiredPrivileges authRequiredPrivileges;
} // namespace engine

#endif