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

   Source File Name = sequoiaFSCommon.cpp

   Descriptive Name = N/A

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who         Description
   ====== =========== =========== =============================================
          23/07/2025  fangjiabin  Initial Draft

   Last Changed =

*******************************************************************************/
#include "sequoiaFSCommon.hpp"
#include "pmdDef.hpp"

using namespace engine;

namespace sequoiafs
{
   INT64 min64(INT64 a, INT64 b)
   {
      if(a > b)
      {
         return b;
      }
      else
      {
         return a;
      }
   }

   INT64 max64(INT64 a, INT64 b)
   {
      if( a < b)
      {
         return b;
      }
      else
      {
         return a;
      }
   }

   INT32 buildDialogPath(CHAR *diaglogPath, CHAR *diaglogPathFromCmd,
                      UINT32 bufSize)
   {
      INT32 rc = SDB_OK;
      CHAR currentPath[OSS_MAX_PATHSIZE + 1] = {0};

      if(bufSize < OSS_MAX_PATHSIZE + 1)
      {
         rc = SDB_INVALIDARG;
         ossPrintf("Path buffer size is too small: %u"OSS_NEWLINE, bufSize);
         goto error;
      }

      rc = ossGetEWD(currentPath, OSS_MAX_PATHSIZE);
      if(rc)
      {
         ossPrintf("Get working directory failed: %d"OSS_NEWLINE, rc);
         goto error;
      }

      rc = engine::utilBuildFullPath(diaglogPathFromCmd, PMD_OPTION_DIAG_PATH,
                                     OSS_MAX_PATHSIZE, diaglogPath);
      if(rc)
      {
         ossPrintf("Build log path failed: %d"OSS_NEWLINE, rc);
         goto error;
      }

      rc = ossMkdir(diaglogPath);
      if(rc)
      {
         if(SDB_FE != rc)
         {
             ossPrintf("Make diralog path [%s] faild: %d"OSS_NEWLINE,
                       diaglogPath, rc);
             goto error;
         }
         else
         {
             rc = SDB_OK;
         }
      }

   done:
      return rc;
   error:
      goto done;
   }
}
