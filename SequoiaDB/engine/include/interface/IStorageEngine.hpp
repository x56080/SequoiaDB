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

   Source File Name = IStorageEngine.hpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef SDB_I_STORAGE_ENGINE_HPP_
#define SDB_I_STORAGE_ENGINE_HPP_

#include "sdbInterface.hpp"
#include "dmsMetadata.hpp"
#include "dmsOprtOptions.hpp"
#include "../bson/bson.hpp"

namespace engine
{

   /*
      IStorageEngine define
    */
   class IStorageEngine : public SDBObject
   {
   public:
      IStorageEngine() = default ;
      virtual ~IStorageEngine() = default ;
      IStorageEngine( const IStorageEngine &o ) = delete ;
      IStorageEngine &operator =( const IStorageEngine & ) = delete ;

   public:
      virtual DMS_STORAGE_ENGINE_TYPE getEngineType() const = 0 ;
   } ;

}

#endif // SDB_I_STORAGE_ENGINE_HPP_