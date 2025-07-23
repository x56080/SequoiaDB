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

   Source File Name = coordCacheCleaner.hpp

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
#ifndef COORD_CACHECLEANER_HPP__
#define COORD_CACHECLEANER_HPP__

#include "coordResource.hpp"
#include "utilLightJobBase.hpp"


using namespace bson ;

namespace engine
{

   class _coordCacheCleaner : public _utilLightJob
   {
   public:
      _coordCacheCleaner( coordResource* pRes ) ;
      virtual ~_coordCacheCleaner() ;
      virtual INT32 init() ;
      virtual const CHAR* name() const ;
      virtual INT32 doit( IExecutor *pExe,
                          UTIL_LJOB_DO_RESULT &result,
                          UINT64 &sleepTime ) ;

   private:
      coordResource       *_pResource ;

   } ;
   typedef _coordCacheCleaner coordCacheCleaner ;

   INT32 coordStartCacheCleanJob( coordResource* pRes ) ;

}

#endif // COORD_CACHECLEANER_HPP__