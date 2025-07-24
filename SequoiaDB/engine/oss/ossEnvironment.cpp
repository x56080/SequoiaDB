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

   Source File Name = ossEnvironment.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

*******************************************************************************/
#include "ossEnvironment.hpp"
#include "ossLikely.hpp"

namespace engine
{
   UINT32 _ossEnvironment::_FILE_FLAGS = 0;

   INT32 _ossEnvironment::extendFile(OSSFILE &file, UINT64 incrementSize)
   {
      INT32 rc = SDB_OK;
      INT64 originalSize = 0;
      if (OSS_UNLIKELY(!file.isOpened()))
      {
         PD_LOG(PDERROR, "file has been closed");
         rc = SDB_INVALIDARG;
         goto error;
      }

      rc = ossGetFileSize(&file, &originalSize);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to get file size:%d", rc);
         goto error;
      }

      ///TODO: switch to normal extending if falloc not supported
      
      rc = ossFallocate(&file, 0, originalSize, incrementSize);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to extend file[%d] space[%lld], rc:%d",
                file.fd, incrementSize, rc);
         goto error;
      } 
   done:
      return rc;
   error:
      goto done;
   }
} // namespace engine
