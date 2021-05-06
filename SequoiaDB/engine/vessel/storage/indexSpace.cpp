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

   Source File Name = indexSpace.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#include "vessel/indexSpace.h"
#include "vessel/storageUnit.h"

namespace engine
{
namespace vessel
{
   indexSpace::~indexSpace()
   {}

   INT32 indexSpace::allocateIdMapPagesOnDisk(requestContext *context,
                                              PAGE_ID first,
                                              UINT32 count)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(isOpen(), "must be open");
      SDB_ASSERT(INVALID_PAGE_ID != first, "can not be invalid");
      SDB_ASSERT(0 < count, "can not be zero");

      storageUnit *su = getSU();
      SDB_ASSERT(NULL != su, "can not be null");
      UINT32 maxPageCount = 0;
      UINT32 maxSegCount = 0;
      UINT32 pageSize = 0;
      PAGE_ID smpPid = INVALID_PAGE_ID;

      rc = su->getCoreArgs(FILE_TYPE_IDX_M, &pageSize,
                           &maxPageCount, &maxSegCount);
      if (SDB_OK != rc)
      {
         goto error;
      }

      if (maxPageCount * maxSegCount <= (first + count))
      {
         PD_LOG(PDERROR, "imp pids[%d,%d] out of range", first, count);
         rc = SDB_VESSEL_FS_UPPER_LIMIT;
         goto error;
      }

      smpPid = logicalPageSpace::getSMPPIdOfImp(first);
      if (INVALID_PAGE_ID == smpPid)
      {
         PD_LOG(PDERROR, "failed to get smp pid of pid[%d]", first);
         rc = SDB_VESSEL_INTERNAL_ERR;
         goto error;
      }

      for (UINT32 i = 1; i < count; ++i)
      {
         PAGE_ID nextSMP = logicalPageSpace::getSMPPIdOfImp(first + i);
         if (INVALID_PAGE_ID == nextSMP)
         {
            PD_LOG(PDERROR, "failed to get smp pid of pid[%d]", first + i);
            rc = SDB_VESSEL_INTERNAL_ERR;
            goto error;
         }
         if (smpPid != nextSMP)
         {
            PD_LOG(PDERROR, "allocating must be in one smp");
            rc = SDB_VESSEL_OPERATOION_NOT_PERMITTED;
            goto error;
         }
      }

   done:
      return rc;
   error:
      goto done;
   }

}//namespace vessel
}//namespace engine