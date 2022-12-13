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

   Source File Name = ossEnvironment.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

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
