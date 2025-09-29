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

   Source File Name = coordCacheCleaner.cpp

   Descriptive Name =

   When/how to use:

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          08/30/2022  Tangtao Initial Draft

   Last Changed =

*******************************************************************************/
#include "coordCacheCleaner.hpp"
#include "coordCB.hpp"

using namespace bson ;

namespace engine
{

   #define COORD_METADATACACHE_SCAN_INTERVAL ( 600 * 1000000 )

   _coordCacheCleaner::_coordCacheCleaner( coordResource* pRes )
   {
      _pResource = pRes ;
   }

   _coordCacheCleaner::~_coordCacheCleaner()
   {
   }

   INT32 _coordCacheCleaner::init()
   {
      return SDB_OK ;
   }

   const CHAR* _coordCacheCleaner::name() const
   {
      return "metaCacheCleaner" ;
   }

   INT32 _coordCacheCleaner::doit( IExecutor *pExe,
                                   UTIL_LJOB_DO_RESULT &result,
                                   UINT64 &sleepTime )
   {
      INT32 rc = SDB_OK ;
      sleepTime = COORD_METADATACACHE_SCAN_INTERVAL ;
      result = UTIL_LJOB_DO_CONT ;

      if ( PMD_IS_DB_DOWN() || ((pmdEDUCB*)pExe)->isForced() )
      {
         result = UTIL_LJOB_DO_FINISH ;
         goto done ;
      }

      if ( _pResource->_canCleanCataInfo() )
      {
         rc = _pResource->_doCleanCataInfo() ;
         if ( rc )
         {
            PD_LOG( PDWARNING, "Failed to clean coord cataInfo cache rc = %d ",
                    rc ) ;
            goto error ;
         }
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 coordStartCacheCleanJob( coordResource* pRes )
   {
      INT32 rc = SDB_OK ;
      coordCacheCleaner *pJob = NULL ;

      pJob = SDB_OSS_NEW coordCacheCleaner( pRes ) ;

      if ( NULL == pJob )
      {
         PD_LOG( PDERROR, "Failed to allocate CacheCleanJob") ;
         rc = SDB_OOM ;
         goto error ;
      }

      rc = pJob->submit( TRUE ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "Failed to submit CacheCleanJob, "
                 "rc: %d", rc ) ;
         goto error ;
      }
      else
      {
         PD_LOG( PDDEBUG, "Submit CacheCleanJob successed" ) ;
      }

   done :
      return rc ;
   error :
      goto done ;
   }

}
