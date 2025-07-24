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

   Source File Name = dmsStorageServiceFactory.hpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          11/20/2023  HGM Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef SDB_DMS_STORAGE_SERVICE_FACTORY_HPP_
#define SDB_DMS_STORAGE_SERVICE_FACTORY_HPP_

#include "dmsDef.hpp"
#include "interface/IStorageService.hpp"

namespace engine
{

   /*
      _dmsStorageServiceFactory define
    */
   class _dmsStorageServiceFactory : public SDBObject
   {
   public:
      _dmsStorageServiceFactory() = default ;
      virtual ~_dmsStorageServiceFactory() = default ;
      _dmsStorageServiceFactory( const _dmsStorageServiceFactory &o ) = delete ;
      _dmsStorageServiceFactory &operator =( const _dmsStorageServiceFactory & ) = delete ;

   public:
      static INT32 create( DMS_STORAGE_ENGINE_TYPE engineType,
                           IStorageService *&service ) ;
      static void release( IStorageService *service ) ;
   } ;

   typedef class _dmsStorageServiceFactory dmsStorageServiceFactory ;

}

#endif // SDB_DMS_STORAGE_SERVICE_FACTORY_HPP_