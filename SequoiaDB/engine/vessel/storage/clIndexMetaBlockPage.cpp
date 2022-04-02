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

   Source File Name = clIndexMetaBlockPage.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          03/16/2022  LYC  Initial Draft

   Last Changed =

******************************************************************************/

#include "vessel/clIndexMetaBlockPage.h"
#include "vessel/pageDef.h"
#include "pd.hpp"
#include "vessel/strictBuffer.h"

namespace engine
{
namespace vessel
{
   UINT32 getCapacityOfIndexMetaBlockPage(UINT32 pageSize)
   {
      UINT32 capacity = 0;
      if (!isValidPageSize(pageSize))
      {
         goto error;
      }

      capacity = getPageBodySize(pageSize) / CL_DISK_INDEX_META_BLOCK_LEN;

   done:
      return capacity;
   error:
      goto done;
   }

   BOOLEAN initCLIndexMetaBlockPage(UINT32 pageSize,
                                    PAGE_ID pid,
                                    PAGE_ID lpid,
                                    PAGE_SNAPSHOT_VERION psv,
                                    void *buf)
   {
      BOOLEAN r = FALSE;
      UINT32 capacity = 0;
      clIndexMetaBlock block;
      UINT32 offset = 0;
      strictBuffer buffer;

      if (OSS_UNLIKELY(!isValidPageSize(pageSize) ||
                       INVALID_PAGE_ID == pid ||
                       INVALID_PAGE_ID == lpid ||
                       INVALID_PAGE_SNAPSHOT_VERSION == psv ||
                       nullptr == buf))
      {
         goto error;
      }

      capacity = getCapacityOfIndexMetaBlockPage(pageSize);
      if (OSS_UNLIKELY(0 == capacity))
      {
         PD_LOG(PDERROR, "failed to get capacity");
         goto error;
      }

      if (!initCommonPage(PAGE_TYPE_INDEX_META_BLOCK, pageSize, 
                          pid, lpid, psv, buf))
      {
         PD_LOG(PDERROR, "failed to init common page");
         goto error;
      }

      buffer.makeWritable(pageSize, buf);
      offset = PAGE_HEAD_SIZE;

      for (UINT32 i = 0; i < capacity; ++i)
      {
         buffer.write(offset, CL_INDEX_META_BLOCK_LEN, &block);
         offset += CL_DISK_INDEX_META_BLOCK_LEN;
      }
      
      r = TRUE;
   done:
      return r;
   error:
      goto done;
   }

   PAGE_ID getIndexMetaBlockPageLpid(UINT32 pageSize, 
                                     CL_MB_ID mbID)
   {
      PAGE_ID lpid = INVALID_PAGE_ID;
      UINT32 capacity = 0;
      if (INVALID_CL_MB_ID == mbID ||
          !isValidPageSize(pageSize))
      {
         goto error;
      }

      capacity = getCapacityOfIndexMetaBlockPage(pageSize);
      if (0 == capacity)
      {
         goto error;
      }

      lpid = mbID / capacity;

   done:
      return lpid;
   error:
      goto done;
   }

   INT32 getIndexMetaBlockPos(UINT32 pageSize, 
                              CL_MB_ID mbID)
   {
      INT32 blockPos = -1;
      UINT32 capacity = 0;
      if (INVALID_CL_MB_ID == mbID ||
          !isValidPageSize(pageSize))
      {
         goto error;
      }

      capacity = getCapacityOfIndexMetaBlockPage(pageSize);
      if (0 == capacity)
      {
         goto error;
      }

      blockPos = mbID % capacity;

   done:
      return blockPos;
   error:
      goto done;
   }

} // namespace vessel
} // namespace engine
