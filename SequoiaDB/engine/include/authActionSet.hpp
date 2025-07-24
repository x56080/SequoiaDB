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

   Source File Name = authActionSet.hpp

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
#ifndef AUTH_ACTION_SET_HPP__
#define AUTH_ACTION_SET_HPP__

#include "ossTypes.h"
#include "../bson/bson.hpp"
#include "authRBAC.hpp"
#include <boost/shared_ptr.hpp>
#include <bitset>
#include <vector>

namespace engine
{
   class _authActionSet
   {
   public:
      static boost::shared_ptr<_authActionSet> fromBson(const bson::BSONObj &obj);
   
   public:
      _authActionSet();
      _authActionSet( const std::vector< ACTION_TYPE > &actions );
      _authActionSet( const ACTION_SET_NUMBER_ARRAY& numbers );
      ~_authActionSet();

      BOOLEAN operator==( const _authActionSet &r ) const
      {
         return this->equal( r );
      }

      friend std::ostream &operator<<( std::ostream &os, const _authActionSet &set );

      OSS_INLINE BOOLEAN operator[]( ACTION_TYPE type ) const
      {
         return _data[ type ];
      }

      boost::shared_ptr< _authActionSet > operator&( const _authActionSet &other ) const;

      void addAction( ACTION_TYPE action );
      void addAllActionsFromSet( const _authActionSet &actionSet );

      void removeAction( ACTION_TYPE action );
      void removeActionsFromSet( const _authActionSet &actionSet );

      OSS_INLINE BOOLEAN empty() const
      {
         return _data.none();
      }

      OSS_INLINE BOOLEAN equal( const _authActionSet &other ) const
      {
         return this->_data == other._data;
      }

      BOOLEAN contains( ACTION_TYPE action ) const;
      BOOLEAN contains( const _authActionSet &other ) const;
      BOOLEAN isSupersetOf( const _authActionSet &other ) const;
      void toBSONArray( bson::BSONArrayBuilder &builder ) const;

   private:
      std::bitset< ACTION_SET_SIZE > _data;
   };
   typedef _authActionSet authActionSet;
} // namespace engine

#endif