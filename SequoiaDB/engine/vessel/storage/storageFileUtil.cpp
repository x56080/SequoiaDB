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

   Source File Name = storageFileUtil.cpp

   Descriptive Name =

   When/how to use: this program may be used on binary and text-formatted
   versions of PMD component. This file contains functions for agent processing.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#include "vessel/storageFileUtil.h"
#include "utilStr.hpp"

namespace engine
{
namespace vessel
{
   /*
   INT32 buildStorageUnitDir(SPACE_ID space, UINT32 maxBufSize, CHAR *buf)
   {
      INT32 rc = SDB_OK;
      UINT32 prefixLen = EXTENT_SU_NAME_PREFIX_LEN;
      INT32 itoaLen = 0;
      if (NULL == buf || maxBufSize < MAX_EXTENT_SU_DIR_LEN + 1)
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      ossSnprintf(buf, maxBufSize, "%s%d", EXTENT_SU_NAME_PREFIX, space);
   done:
      return rc;
   error:
      goto done;
   }
   */

   BOOLEAN parseStorageUnitDir(const strSlice &name, SPACE_ID *space)
   {
      BOOLEAN r = FALSE;
      UINT32 digit = 0;
      
      if (name.strLen() <= SU_NAME_PREFIX_LEN)
      {
         goto done;
      }
      else if (SU_FILE_NAME_LEN < name.strLen())
      {
         goto done;
      }

      if (0 != ossStrncmp(name.str(), SU_FILE_NAME_PREFIX, SU_NAME_PREFIX_LEN))
      {
         goto done;
      }

      if (!utilStrIsDigit(name.str() + SU_NAME_PREFIX_LEN))
      {
         goto done;
      }

      digit = ossAtoi(name.str() + SU_NAME_PREFIX_LEN);
      if (MAX_SPACE_ID < digit)
      {
         goto done;
      }

      r = TRUE;

      if (NULL != space)
      {
         *space = digit;
      }

   done:
      return r;
   }
}//namespace vessel
}//namespace engine