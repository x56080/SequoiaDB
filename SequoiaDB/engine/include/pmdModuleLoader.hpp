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

   Source File Name = pmdModuleLoader.hpp

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
#ifndef _SDB_MODULE_LOADER_HPP_
#define _SDB_MODULE_LOADER_HPP_

#include "oss.hpp"
#include "ossDynamicLoad.hpp"
#include "ossTypes.h"
#include "pmdAccessProtocolBase.hpp"

#define OSS_FAP_CREATE  ( IPmdAccessProtocol*(*)() )
#define OSS_FAP_RELEASE ( void(*)(IPmdAccessProtocol *&) )
#define CREATE_FAP_NAME "createAccessProtocol"
#define RELEASE_FAP_NAME "releaseAccessProtocol"

#define FAP_MODULE_NAME_PREFIX "fap"
#define FAP_MODULE_PATH FAP_MODULE_NAME_PREFIX OSS_FILE_SEP
#define FAP_MODULE_NAME_SIZE 255
#define MONGO_MODULE_NAME "mongo"


namespace engine
{
   /*
      _pmdEDUParam define
   */
   class _pmdEDUParam : public SDBObject
   {
   public:
      _pmdEDUParam() : pSocket( NULL ), protocol( NULL )
      {}

      virtual ~_pmdEDUParam()
      {
         pSocket = NULL ;
         protocol = NULL ;
      }

      void               *pSocket ;
      IPmdAccessProtocol *protocol ;
   };
   typedef _pmdEDUParam pmdEDUParam ;

   /*
      _pmdModuleLoader define
   */
   class _pmdModuleLoader : public SDBObject
   {
   public:
      _pmdModuleLoader() ;
      virtual ~_pmdModuleLoader() ;

      INT32 getFunction( const CHAR *funcName,
                         OSS_MODULE_PFUNCTION *function ) ;
      INT32 create ( IPmdAccessProtocol *&protocol ) ;
      INT32 release( IPmdAccessProtocol *&protocol ) ;

      INT32 load( const CHAR *mudule, const CHAR *path,
                  UINT32 mode = 0 ) ;
      void  unload() ;

   protected:
      OSS_MODULE_PFUNCTION  _function ;
      ossModuleHandle      *_loadModule ;
   };

   typedef _pmdModuleLoader pmdModuleLoader ;
}

#endif // _SDB_MODULE_LOADER_HPP_
