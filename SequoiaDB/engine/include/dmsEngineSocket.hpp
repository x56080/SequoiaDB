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
         INT32 closeEngines(IExecutor *executor, const dmsCloseDBOptions &options);

      private:
         std::unique_ptr< IDataStorageEngine > _engines[ DMS_ENGINE_MAX + 1 ] = { nullptr, nullptr };
   };
   typedef _dmsEngineSocket dmsEngineSocket;
} // namespace engine

#endif