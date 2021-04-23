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

namespace engine
{
namespace vessel
{
   BOOLEAN storageCoreArgs::isValid()const
   {
      BOOLEAN r = FALSE;
      if (DMS_PAGE_SIZE4K != pageSize &&
          DMS_PAGE_SIZE8K != pageSize &&
          DMS_PAGE_SIZE16K != pageSize &&
          DMS_PAGE_SIZE32K != pageSize &&
          DMS_PAGE_SIZE64K != pageSize &&
          DMS_PAGE_SIZE256K != pageSize)
      {
         goto done;
      }

      if (0 == maxPageCountPerSeg ||
          !ossIsPowerOf2(maxPageCountPerSeg))
      {
         goto done;
      }

      if (0 == maxSegmentCountPerFile ||
          !ossIsPowerOf2(maxSegmentCountPerFile))
      {
         goto done;
      }

      r = TRUE;
   done:
      return r;
   }

   BOOLEAN isValidSegmentSize(UINT32 size)
   {
      return STORAGE_FILE_SEGMENT_SIZE_2MB == size ||
             STORAGE_FILE_SEGMENT_SIZE_32MB == size ||
             STORAGE_FILE_SEGMENT_SIZE_128MB == size ||
             STORAGE_FILE_SEGMENT_SIZE_256MB == size;
   }

}//namespace vessel
}//namespace engine