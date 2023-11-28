/*******************************************************************************


   Copyright (C) 2011-2018 SequoiaDB Ltd.

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU Affero General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU Affero General Public License for more details.

   You should have received a copy of the GNU Affero General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.

   Source File Name = dmsWTSession.hpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          11/20/2023  HGM Initial Draft

   Last Changed =

*******************************************************************************/

#ifndef DMS_WT_SESSION_HPP_
#define DMS_WT_SESSION_HPP_

#include "interface/IStorageService.hpp"
#include "wiredtiger/dmsWTEngineOptions.hpp"
#include "wiredtiger/dmsWTUtil.hpp"

#include <wiredtiger.h>

namespace engine
{
namespace wiredtiger
{

   /*
      _dmsWTSession define
    */
   class _dmsWTSession : public SDBObject
   {
   public:
      _dmsWTSession() ;
      ~_dmsWTSession() ;
      _dmsWTSession( const _dmsWTSession &o ) = delete ;
      _dmsWTSession &operator =( const _dmsWTSession & ) = delete ;

      INT32 open( WT_CONNECTION *conn ) ;
      INT32 close() ;

      WT_SESSION *getSession()
      {
         return _session ;
      }

      BOOLEAN isOpened() const
      {
         return nullptr != _session ;
      }

   protected:
      WT_SESSION *_session = nullptr ;
   } ;

   typedef class _dmsWTSession dmsWTSession ;

}
}

#endif // DMS_WT_ENGINE_HPP_
