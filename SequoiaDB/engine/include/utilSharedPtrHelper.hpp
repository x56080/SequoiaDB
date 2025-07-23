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

   Source File Name = utilSharedPtrHelper.hpp

   Descriptive Name = N/A

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who         Description
   ====== =========== =========== =============================================
          23/07/2025  fangjiabin  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef UTIL_SHARED_PTR_HELPER__
#define UTIL_SHARED_PTR_HELPER__

#include <boost/shared_ptr.hpp>

namespace engine
{
   template < typename T > struct SHARED_TYPE_LESS
   {
      typedef boost::shared_ptr< T > SHARED_TYPE;
      bool operator()( const SHARED_TYPE &lhs, const SHARED_TYPE &rhs ) const
      {
         if ( lhs.get() == rhs.get() )
         {
            return false;
         }
         return ( *lhs ) < ( *rhs );
      }
   };

   template < typename T > class SHARED_TYPE_EQUAL
   {
      typedef boost::shared_ptr< T > SHARED_TYPE;

   public:
      SHARED_TYPE_EQUAL( const SHARED_TYPE &outer ) : _outer( outer ) {}
      bool operator()( const SHARED_TYPE &value ) const
      {
         return *value == *_outer;
      }

   private:
      SHARED_TYPE _outer;
   };
} // namespace engine

#endif