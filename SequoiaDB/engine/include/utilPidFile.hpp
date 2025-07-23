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

   Source File Name = utilPidFile.hpp

   Descriptive Name =

   When/how to use:

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          02/12/2019  HJW Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef UTIL_PID_FILE_HPP__
#define UTIL_PID_FILE_HPP__

#include "ossTypes.hpp"

namespace engine
{

   INT32 createPIDFile( const CHAR *pOutputPath ) ;

   INT32 checkAndCreatePIDFile( const CHAR *pOutputPath, BOOLEAN *pHasCreate = NULL ) ;

   INT32 removePIDFile( const CHAR *pFilePath ) ;

}

#endif //UTIL_PID_FILE_HPP__
