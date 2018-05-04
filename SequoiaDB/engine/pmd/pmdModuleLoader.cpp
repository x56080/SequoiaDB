/*******************************************************************************


   Copyright (C) 2011-2018 SequoiaDB Ltd.

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

   http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.

   Source File Name = pmdModuleLoader.cpp

   Descriptive Name =

   When/how to use: this program may be used on binary and text-formatted
   versions of PMD component. This file contains functions for agent processing.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          01/27/2015  LZ  Initial Draft

   Last Changed =

*******************************************************************************/
#include "pmdModuleLoader.hpp"
#include "pd.hpp"

namespace engine
{
   /*
      _pmdModuleLoader implement
   */
   _pmdModuleLoader::_pmdModuleLoader() : _loadModule( NULL )
   {
   }

   _pmdModuleLoader::~_pmdModuleLoader()
   {
      unload() ;
   }

   INT32 _pmdModuleLoader::load( const CHAR *module,
                                 const CHAR *path,
                                 UINT32 mode )
   {
      INT32 rc = SDB_OK ;
      if ( NULL != _loadModule )
      {
         unload() ;
      }

      _loadModule = SDB_OSS_NEW ossModuleHandle( module, path, 0 ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "Failed to alloc module" ) ;
         rc = SDB_OOM ;
         goto error ;
      }

      rc = _loadModule->init() ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "Init module failed" ) ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   void _pmdModuleLoader::unload()
   {
      if ( NULL != _loadModule )
      {
         SDB_OSS_DEL _loadModule ;
         _loadModule = NULL ;
      }
   }

   INT32 _pmdModuleLoader::getFunction( const CHAR *funcName,
                                        OSS_MODULE_PFUNCTION *func )
   {
      SDB_ASSERT( NULL != funcName, "Function name cann't be NULL" ) ;

      INT32 rc = SDB_OK ;
      SDB_ASSERT( NULL != _loadModule, "module was not loaded" ) ;

      rc = _loadModule->resolveAddress( funcName, func ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "Failed to get function address" ) ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _pmdModuleLoader::create( IPmdAccessProtocol *&protocol )
   {
      INT32 rc = SDB_OK ;
      SDB_ASSERT( NULL != _loadModule, "Module handle cann't be NULL" ) ;

      rc = _loadModule->resolveAddress( CREATE_FAP_NAME, &_function ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "Failed to get export function: " ) ;
         goto error ;
      }

      protocol = (OSS_FAP_CREATE(_function))() ;
      if ( NULL == protocol )
      {
         PD_LOG( PDERROR, "Failed to create protocol" ) ;
         rc = SDB_OOM ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _pmdModuleLoader::release( IPmdAccessProtocol *&protocol )
   {
      INT32 rc = SDB_OK ;
      SDB_ASSERT( NULL != _loadModule, "Module handle cann't be NULL" ) ;

      rc = _loadModule->resolveAddress( RELEASE_FAP_NAME, &_function ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "Failed to get export function: " ) ;
         goto error ;
      }

      (OSS_FAP_RELEASE(_function))( protocol ) ;

   done:
      return rc ;
   error:
      goto done ;
   }

}
