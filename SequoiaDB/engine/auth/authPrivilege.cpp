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

   Source File Name = authPrivilege.cpp

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
#include "authPrivilege.hpp"
#include "authDef.hpp"

namespace engine
{
   using namespace bson;
   using namespace boost;
   static const authActionSet ACTION_SET_MASK__INVALID( RESOURCE_TYPE__INVALID_BITSET_NUMBERS );
   static const authActionSet ACTION_SET_MASK_COLLECTION_SPACE(
      RESOURCE_TYPE_COLLECTION_SPACE_BITSET_NUMBERS );
   static const authActionSet ACTION_SET_MASK_EXACT_COLLECTION(
      RESOURCE_TYPE_EXACT_COLLECTION_BITSET_NUMBERS );
   static const authActionSet ACTION_SET_MASK_COLLECTION_NAME(
      RESOURCE_TYPE_COLLECTION_NAME_BITSET_NUMBERS );
   static const authActionSet ACTION_SET_MASK_NON_SYSTEM( RESOURCE_TYPE_NON_SYSTEM_BITSET_NUMBERS );
   static const authActionSet ACTION_SET_MASK_CLUSTER( RESOURCE_TYPE_CLUSTER_BITSET_NUMBERS );
   static const authActionSet ACTION_SET_MASK_ANY( RESOURCE_TYPE_ANY_BITSET_NUMBERS );

   const authActionSet *authGetActionSetMask( RESOURCE_TYPE type )
   {
      switch ( type )
      {
      case RESOURCE_TYPE__INVALID :
         return &ACTION_SET_MASK__INVALID;
      case RESOURCE_TYPE_COLLECTION_SPACE :
         return &ACTION_SET_MASK_COLLECTION_SPACE;
      case RESOURCE_TYPE_EXACT_COLLECTION :
         return &ACTION_SET_MASK_EXACT_COLLECTION;
      case RESOURCE_TYPE_COLLECTION_NAME :
         return &ACTION_SET_MASK_COLLECTION_NAME;
      case RESOURCE_TYPE_NON_SYSTEM :
         return &ACTION_SET_MASK_NON_SYSTEM;
      case RESOURCE_TYPE_CLUSTER :
         return &ACTION_SET_MASK_CLUSTER;
      case RESOURCE_TYPE_ANY :
         return &ACTION_SET_MASK_ANY;
      }
      SDB_ASSERT( FALSE, "Impossible" );
      return &ACTION_SET_MASK__INVALID;
   }

   _authPrivilege::_authPrivilege( const boost::shared_ptr< authResource > &resource,
                                   const boost::shared_ptr< authActionSet > &actions )
   : _resource( resource )
   {
      SDB_ASSERT( resource && actions, "can not be null" );
      const authActionSet *mask = authGetActionSetMask( _resource->getType() );
      _actionSet = ( *actions ) & ( *mask );
   }
   _authPrivilege::~_authPrivilege() {}

   const boost::shared_ptr< authResource > &_authPrivilege::getResource() const
   {
      return _resource;
   }
   const boost::shared_ptr< authActionSet > &_authPrivilege::getActionSet() const
   {
      return _actionSet;
   }
} // namespace engine