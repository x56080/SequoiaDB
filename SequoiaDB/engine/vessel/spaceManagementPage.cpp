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

   Source File Name = spaceManagementPage.cpp

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

#include "vessel/spaceManagementPage.h"
#include "pdTrace.hpp"
#include "ossLikely.hpp"

namespace engine
{
namespace vessel
{
   INT32 getSMPCapacity8BytesAligned(UINT32 pageSize,
                                     UINT32 maxSegmentCount,
                                     UINT32 pageCountOfSeg,
                                     UINT32 &capacity)
   {
      INT32 rc = SDB_OK;
      UINT32 capacityOfPage = 0;
      UINT32 userDefinedCapacity = maxSegmentCount * pageCountOfSeg;
      SDB_ASSERT(0 < pageCountOfSeg && 0 < maxSegmentCount, "can not be zero");

      if (OSS_UNLIKELY(pageSize < (PAGE_HEAD_LEN + PAGE_TAIL_LEN + SMP_HEAD_LEN)))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      else if (OSS_UNLIKELY(0 == maxSegmentCount ||
                            0 == pageCountOfSeg))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      capacityOfPage = (pageSize - PAGE_HEAD_LEN - PAGE_TAIL_LEN - SMP_HEAD_LEN) & 0xfffffff8; /// 8bytes aligned.
      capacityOfPage = capacityOfPage << 3; /// capacityOfPage *= 8;

      if (capacityOfPage < userDefinedCapacity)
      {
         PD_LOG(PDERROR, "max capacity is %d when page size is:%d", capacityOfPage, pageSize);
         rc = SDB_INVALIDARG;
         goto error;
      }
      else
      {
         capacityOfPage = userDefinedCapacity;
      }

      capacity = capacityOfPage;
   done:
      return rc;
   error:
      goto done;
   }

}//namespace vessel
}//namespace engine