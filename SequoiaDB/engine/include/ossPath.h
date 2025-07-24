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

   Source File Name = ossPath.h

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
#ifndef OSS_PATH_H
#define OSS_PATH_H

#include "ossTypes.h"

#if defined (_WINDOWS)
#define OSS_PATH_SEP_CHAR '\\'
#define OSS_PATH_SEP_SET "\\"
#else
#define OSS_PATH_SEP_CHAR '/'
#define OSS_PATH_SEP_SET "/"
#endif

/**
 * locate an executable by providing the path of another executable in the
 * same directory and its own name.
 * On Windows, refPath=C:\mypath\a.exe, exeName=b.exe, then path will contain
 * C:\mypath\b.exe
 * On Linux, refPath=/home/a, exeName=b, then path will contain /home/b
 */
INT32 ossLocateExecutable ( const CHAR * refPath ,
                            const CHAR * exeName ,
                            CHAR * buf ,
                            UINT32 bufSize ) ;
#endif // OSS_PATH_H

