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

   Source File Name = dmsWTStore.hpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          11/20/2023  HGM Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef DMS_WT_STORE_HPP_
#define DMS_WT_STORE_HPP_

#include "wiredtiger/dmsWTUtil.hpp"

namespace engine
{
namespace wiredtiger
{

   /*
      _dmsWTStore define
    */
   class _dmsWTStore : public utilPooledObject
   {
   public:
      _dmsWTStore() = default ;
      ~_dmsWTStore() = default ;

      _dmsWTStore( const ossPoolString &uri )
      : _uri( uri )
      {
      }

      _dmsWTStore( const _dmsWTStore &&o )
      : _uri( std::move( o._uri ) )
      {
      }

      _dmsWTStore( const _dmsWTStore &o ) = default ;
      _dmsWTStore &operator =( const _dmsWTStore & ) = default ;

      const ossPoolString &getURI() const
      {
         return _uri ;
      }

      const ossPoolString &getStatsURI() const
      {
         return _statsURI ;
      }

      void setURI( const ossPoolString &uri )
      {
         _uri = uri ;
         _statsURI = "statistics:" + _uri ;
      }

      void setURI( const CHAR *uri )
      {
         _uri.assign( uri ) ;
         _statsURI = "statistics:" + _uri ;
      }

   protected:
      ossPoolString _uri ;
      ossPoolString _statsURI ;
   } ;

   typedef class _dmsWTStore dmsWTStore ;

}
}

#endif // DMS_WT_STORE_HPP_
