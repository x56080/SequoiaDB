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

   Source File Name = utilFullNameParser.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

*******************************************************************************/
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

   ossPoolString utilFullNameParser::buildFullName(const CHAR *csName,
                                                   const CHAR *clName)
   {
      SDB_ASSERT(NULL != csName && NULL != clName, "can not be null");
      ossPoolString fullName;
      fullName.reserve(128);
      fullName.assign(csName).append(".").append(clName);
      return std::move(fullName);
   }
} // namespace engine
