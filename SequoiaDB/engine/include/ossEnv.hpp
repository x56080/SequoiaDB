/*******************************************************************************
   Copyright (C) 2011-2018 SequoiaDB Ltd.

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU Affero General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU Affero General Public License for more details.

   You should have received a copy of the GNU Affero General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.

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

