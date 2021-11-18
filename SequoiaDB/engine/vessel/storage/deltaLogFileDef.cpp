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

   Source File Name = deltaLogFileDef.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#include "vessel/deltaLogFileDef.h"
#include "pdTrace.hpp"

namespace engine
{
namespace vessel
{
   BOOLEAN deltaLogFile::validateUserDefinedHead(const void *head)const
   {
      SDB_ASSERT(NULL != head, "can not be null");
      return ((const deltaLogFileHead *)head)->version == deltaLogFile::VERSION;
   }


   INT32 deltaLogFile::getDeltaLogFileHead(deltaLogFileHead &h)const
   {
      INT32 rc = SDB_OK;
      ossValuePtr ptr = 0;

      if (OSS_UNLIKELY(!isOpen()))
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }

      rc = getUserDefinedHeadPtr(ptr);
      if (SDB_OK != rc)
      {
         goto error;
      }

      h = *((const deltaLogFileHead *)ptr);
   done:
      return rc;
   error:
      goto done;
   }
} // namespace vessel

} // namespace engine
