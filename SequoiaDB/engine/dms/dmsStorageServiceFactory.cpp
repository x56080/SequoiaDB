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

   Source File Name = dmsStorageServiceFactory.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          11/20/2023  HGM Initial Draft

   Last Changed =

*******************************************************************************/
#include "dmsStorageServiceFactory.hpp"
#include "dmsDef.hpp"
#include "ossErr.h"
#include "ossMem.hpp"
#include "wiredtiger/dmsWTStorageService.hpp"
#include "pdTrace.hpp"
#include "dmsTrace.hpp"

namespace engine
{

   /*
       _dmsStorageServiceFactory implement
    */
   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSSTORAGESERVICEFACTORY_CREATE, "_dmsStorageServiceFactory::create" )
   INT32 _dmsStorageServiceFactory::create( DMS_STORAGE_ENGINE_TYPE engineType,
                                            IStorageService *&service )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__DMSSTORAGESERVICEFACTORY_CREATE ) ;

      SDB_ASSERT( nullptr == service, "service should not be created" ) ;
      PD_CHECK( nullptr == service, SDB_INVALIDARG, error, PDERROR,
                "Failed to create service, already created" ) ;

      switch ( engineType )
      {
         case DMS_STORAGE_ENGINE_WIREDTIGER:
         {
            service = SDB_OSS_NEW wiredtiger::dmsWTStorageService() ;
            PD_CHECK( nullptr != service, SDB_OOM, error, PDERROR,
                      "Failed to create storage service for engine [%s], "
                      "out of memory",
                      dmsGetStorageEngineName(engineType) ) ;
            break ;
         }
         default:
         {
            PD_CHECK( FALSE, SDB_OPTION_NOT_SUPPORT, error, PDERROR,
                      "Failed to create storage service for engine [%s], "
                      "it is not supported",
                      dmsGetStorageEngineName(engineType) ) ;
            break ;
         }
      }

   done:
      PD_TRACE_EXITRC( SDB__DMSSTORAGESERVICEFACTORY_CREATE, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSSTORAGESERVICEFACTORY_RELEASE, "_dmsStorageServiceFactory::release" )
   void _dmsStorageServiceFactory::release( IStorageService *engine )
   {
      PD_TRACE_ENTRY( SDB__DMSSTORAGESERVICEFACTORY_RELEASE ) ;

      if ( nullptr != engine )
      {
         SDB_OSS_DEL engine ;
      }

      PD_TRACE_EXIT( SDB__DMSSTORAGESERVICEFACTORY_RELEASE ) ;
   }

}
