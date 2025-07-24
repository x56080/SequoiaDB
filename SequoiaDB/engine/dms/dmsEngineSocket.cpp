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

   INT32 _dmsEngineSocket::closeEngines( IExecutor *executor, const dmsCloseDBOptions &options )
   {
      INT32 rc = SDB_OK;
      for( INT32 type = DMS_ENGINE_MMAP; type <= DMS_ENGINE_MAX; ++type)
      {
         if ( _engines[type] )
         {
            rc = _engines[ type ]->close( executor, options );
            PD_RC_CHECK( rc, PDERROR, "failed to close the engine[%s]",
                         _engines[ type ]->getEngineType() );
            _engines[type].reset();
         }
      }
   done:
      return rc;
   error:
      goto done;
   }
} // namespace engine