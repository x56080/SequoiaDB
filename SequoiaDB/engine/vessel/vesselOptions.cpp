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

   Source File Name = vesselOptions.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#include "vessel/vesselOptions.h"
#include "vessel/storageFileDef.h"

namespace engine
{
namespace vessel
{
   BOOLEAN createCSOptions::isValid()const
   {
      BOOLEAN r = FALSE;

      if (DMS_PAGE_SIZE32K != dataPageSize &&
          DMS_PAGE_SIZE64K != dataPageSize)
      {
         goto done;
      }
      if (DMS_PAGE_SIZE64K != idxPageSize &&
          DMS_PAGE_SIZE32K != idxPageSize &&
          DMS_PAGE_SIZE16K != idxPageSize &&
          DMS_PAGE_SIZE8K != idxPageSize)
      {
         goto done;
      }
      if (DMS_PAGE_SIZE4K != lobPageSize &&
          DMS_PAGE_SIZE8K != lobPageSize &&
          DMS_PAGE_SIZE32K != lobPageSize &&
          DMS_PAGE_SIZE64K != lobPageSize &&
          DMS_PAGE_SIZE128K != lobPageSize &&
          DMS_PAGE_SIZE256K != lobPageSize &&
          DMS_PAGE_SIZE512K != lobPageSize)
      {
         goto done;
      }

      if (!isValidSegmentSize(dataSegSize) ||
          !isValidSegmentSize(idxSegSize) ||
          !isValidSegmentSize(lobSegSize))
      {
         goto done;
      }
      r = TRUE;
   done:
      return r;
   }
}//namespace vessel
}//namespace engine
