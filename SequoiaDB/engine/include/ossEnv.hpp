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

   Source File Name = ossEnv.hpp

   Descriptive Name = oss Environment Info

   When/how to use:

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          22/12/2022  Tangtao Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef OSSENV_HPP_
#define OSSENV_HPP_

#include "ossIO.hpp"
#include <fcntl.h>
#include "ossMemPool.hpp"

namespace engine
{

   class _ossPathDetector
   {
   public:
      _ossPathDetector() {} ;
      ~_ossPathDetector() {} ;

      INT32 testPunchHole( const ossPoolSet<ossPoolString> &pathSet ) ;

   private:
      INT32 _tryToPunchHole( const CHAR* pFilePath ) ;
   } ;
   typedef _ossPathDetector ossPathDetector ;


   struct _ossEnvInfo
   {
      _ossEnvInfo() ;
      ~_ossEnvInfo() ;

      INT32 init( const CHAR *dataPath, const CHAR *idxPath,
                  const CHAR *lobmPath, const CHAR *lobdPath ) ;

      ossPoolSet<ossPoolString>  _filePathsSet ;
      BOOLEAN                    _supportPunchHoleMode ;
   } ;
   typedef _ossEnvInfo ossEnvInfo ;


   ossEnvInfo*       getOssEnvInfo () ;
   void              initOssEnv( ossEnvInfo* env ) ;
   BOOLEAN           ossEnvCanPunchHole() ;

}

#endif //OSSENV_HPP_

