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

   Source File Name = deltaLogFileScanner.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#include "vessel/deltaLogFileScanner.h"
#include "pdTrace.hpp"

namespace engine
{
namespace vessel
{
   INT32 deltaLogFileScanner::open(const deltaLogFile *file)
   {
      INT32 rc = SDB_OK;
      close();

      if (NULL == file || !file->isOpen())
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      rc = _file->getDeltaLogFileHead(_header);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to get file header:%d", rc);
         goto error;
      }
   done:
      return rc;
   error:
      goto done;
   }

   void deltaLogFileScanner::close()
   {
      _file = NULL;
      _header = deltaLogFileHead();
      _pos = 0;
   }
} // namespace vessel

} // namespace engine