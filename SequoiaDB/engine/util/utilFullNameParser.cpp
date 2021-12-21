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

   Source File Name = utilFullNameParser.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#include "utilFullNameParser.hpp"
#include "pdTrace.hpp"
#include "dms.hpp"

namespace engine
{
   BOOLEAN utilFullNameParser::parse(const CHAR *fullName,
                                     const CHAR **clName)
   {
      SDB_ASSERT(NULL != fullName, "can not be null");
      BOOLEAN r = FALSE;

      UINT32 size = 0;
      const CHAR *dot = ossStrchr(fullName, '.');
      if (NULL == dot || fullName == dot || '\0' == *(dot + 1))
      {
         goto done;
      }

      size = dot - fullName;
      if (DMS_COLLECTION_SPACE_NAME_SZ < size)
      {
         goto done;
      }
      ossMemcpy(_csName, fullName, size);
      _csName[size] = '\0';

      if (SDB_OK != dmsCheckCSName(_csName, TRUE))
      {
         goto done;
      }
      if (SDB_OK != dmsCheckCLName(dot + 1, TRUE))
      {
         goto done;
      }

      if (NULL != clName)
      {
         *clName = dot + 1;
      }

      r = TRUE;

   done:
      return r;
   }
} // namespace engine
