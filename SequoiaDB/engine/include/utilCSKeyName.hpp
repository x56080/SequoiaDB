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

   Source File Name = utilCSKeyName.hpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          05/03/2022  HGM Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef UTIL_CS_KEY_NAME_HPP_
#define UTIL_CS_KEY_NAME_HPP_

#include "oss.hpp"
#include "core.hpp"
#include "ossUtil.hpp"
#include "ossMemPool.hpp"

namespace engine
{

   /*
      _utilCSKeyName define
    */
   typedef struct _utilCSKeyName
   {
      ossPoolString        _name ;

      _utilCSKeyName( const ossPoolString &name )
      {
         _name = name ;
      }

      _utilCSKeyName( const CHAR *name )
      {
         _name.assign( name ) ;
      }

      _utilCSKeyName()
      {
      }

      bool operator< ( const _utilCSKeyName &rhs ) const
      {
         UINT32 llen = _name.length() ;
         UINT32 rlen = rhs._name.length() ;
         UINT32 len = llen <= rlen ? llen : rlen ;

         INT32 cmp = ossStrncmp( _name.c_str(), rhs._name.c_str(), len ) ;
         if ( 0 == cmp && llen == len && rlen != len &&
              '.' != rhs._name[len] )
         {
            cmp = -1 ;
         }

         return cmp < 0 ? true : false ;
      }

      bool operator== ( const _utilCSKeyName &rhs ) const
      {
         UINT32 llen = _name.length() ;
         UINT32 rlen = rhs._name.length() ;
         UINT32 len = llen <= rlen ? llen : rlen ;

         INT32 cmp = ossStrncmp( _name.c_str(), rhs._name.c_str(), len ) ;
         if ( 0 == cmp && ( llen == len || '.' == _name[len] ) &&
              ( rlen == len || '.' == rhs._name[len] ) )
         {
            return true ;
         }
         return false ;
      }
   } utilCSKeyName ;

}

#endif
