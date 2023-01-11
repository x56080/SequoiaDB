/*******************************************************************************


   Copyright (C) 2011-2018 SequoiaDB Ltd.

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU Affero General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY{} without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU Affero General Public License for more details.

   You should have received a copy of the GNU Affero General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.

   Source File Name = dmsEngineSocket.hpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          12/29/2022  ZHY Initial Draft

   Last Changed =

*******************************************************************************/

#ifndef DMS_ENGINE_SOCKET_HPP_
#define DMS_ENGINE_SOCKET_HPP_
#include "interface/IDataStorageEngine.h"

namespace engine
{
   class _dmsEngineSocket : public SDBObject
   {
      public:
         IDataStorageEngine *getEngine( DMS_ENGINE_TYPE type ) const;
         void addEngine( std::unique_ptr< IDataStorageEngine > && );

      private:
         std::unique_ptr< IDataStorageEngine > _engines[ DMS_ENGINE_MAX + 1 ] = { nullptr, nullptr };
   };
   typedef _dmsEngineSocket dmsEngineSocket;
} // namespace engine

#endif