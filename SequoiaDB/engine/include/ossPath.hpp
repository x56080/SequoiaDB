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

   Source File Name = ossPath.hpp

   Descriptive Name =

   When/how to use:

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          11/28/2012  YW  Initial Draft

   Last Changed =

*******************************************************************************/

#ifndef OSSPATH_HPP__
#define OSSPATH_HPP__

#include <string>
#include <map>
#include <vector>
#include "oss.h"

using namespace std ;

INT32 ossEnumFiles( const string &dirPath,
                    multimap<string, string> &mapFiles,
                    const CHAR *filter = NULL,
                    UINT32 deep = 1 ) ;

INT32 ossEnumFiles2( const string &dirPath,
                     multimap<string, string> &mapFiles,
                     const CHAR *filter = NULL,
                     OSS_MATCH_TYPE type = OSS_MATCH_ALL,
                     UINT32 deep = 1 ) ;

INT32 ossEnumSubDirs( const string &dirPath,
                      vector< string > &subDirs,
                      UINT32 deep = 1 ) ;


#endif // OSSPATH_HPP__

