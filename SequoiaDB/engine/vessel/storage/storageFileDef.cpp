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

   Source File Name = storageFileDef.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#include "vessel/storageFileDef.h"
#include "pdTrace.hpp"
#include "vessel/pageDef.h"

namespace engine
{
namespace vessel
{
   BOOLEAN storageCoreArgs::isValid()const
   {
      BOOLEAN r = FALSE;
      if (!isValidPageSize(pageSize))
      {
         goto done;
      }

      if (!ossIsPowerOf2(maxPageCountPerSeg) &&
          !ossIsAligned64(maxPageCountPerSeg) &&
          STORAGE_FILE_SEGMENT_MAX_PCNT < maxPageCountPerSeg)
      {
         goto done;
      }

      if (!ossIsAligned4(maxSegmentCountPerFile) ||
          0 == maxSegmentCountPerFile)
      {
         goto done;
      }

      r = TRUE;
   done:
      return r;
   }

}//namespace vessel
}//namespace engine