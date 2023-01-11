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

   Source File Name = dmsEngineSocket.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          12/29/2022  ZHY Initial Draft

   Last Changed =

*******************************************************************************/

#include "dmsEngineSocket.hpp"

namespace engine
{
   IDataStorageEngine *dmsEngineSocket::getEngine( DMS_ENGINE_TYPE type ) const
   {
      SDB_ASSERT( type <= DMS_ENGINE_MAX, "dms engine type must be valid" );
      IDataStorageEngine *engine = _engines[ type ].get();
      SDB_ASSERT( engine, "can not be nullptr" );
      return engine;
   }

   void dmsEngineSocket::addEngine( std::unique_ptr< IDataStorageEngine > &&ptr )
   {
      SDB_ASSERT( ptr, "can not be nullptr" );
      DMS_ENGINE_TYPE type = ptr->getEngineType();
      SDB_ASSERT( !_engines[ type ], "an instance of the engine already exists" );
      _engines[ type ] = std::move( ptr );
   }
} // namespace engine