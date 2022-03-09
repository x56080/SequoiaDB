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

   Source File Name = clMetaBlockPage.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#include "vessel/clMetaBlockPage.h"

namespace engine
{
namespace vessel
{
   INT32 getCapacityOfCLMetaBlockPage(UINT32 pageSize, UINT32 &capacity)
   {
      INT32 rc = SDB_OK;
      if (DMS_PAGE_SIZE32K != pageSize &&
          DMS_PAGE_SIZE64K != pageSize)
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      capacity = (pageSize - PAGE_HEAD_SIZE - PAGE_TAIL_SIZE) / CL_DISK_META_BLOCK_LEN;
   done:
      return rc;
   error:
      goto done;
   }

   BOOLEAN initCLMetaBlockPage(UINT32 pageSize,
                               PAGE_ID pid,
                               PAGE_ID lpid,
                               PAGE_SNAPSHOT_VERION psv,
                               void *buf)
   {
      BOOLEAN r = FALSE;
      UINT32 capacity = 0;
      clMetaBlock block;
      UINT32 offset = 0;
      if (SDB_OK != getCapacityOfCLMetaBlockPage(pageSize, capacity))
      {
         goto done;
      }

      if (!initCommonPage(PAGE_TYPE_CL_META, pageSize,
                          pid, lpid, psv, buf))
      {
         goto done;
      }
      
      offset = PAGE_HEAD_SIZE;
      for (UINT32 i = 0; i < capacity; ++i)
      {
         clMetaBlock *blockPtr = (clMetaBlock *)((ossValuePtr)buf + offset);
         ossMemcpy(blockPtr, &block, CL_META_BLOCK_LEN);
         offset += CL_DISK_META_BLOCK_LEN;
      }
      r = TRUE;
   done:
      return r;
   }

   BOOLEAN getCLMetaBlockIfValid(const void *ptr,
                                 UINT32 i,
                                 clMetaBlock &record)
   {
      BOOLEAN r = FALSE;
      SDB_ASSERT(NULL != ptr, "can not be null");
      const clMetaBlock *cr = (const clMetaBlock *)
                              ((ossValuePtr)ptr +
                              PAGE_HEAD_SIZE +
                              i * CL_DISK_META_BLOCK_LEN);
   
      if (cr->isValid())
      {
         record = *cr;
         r = TRUE;
      }
      return r;             
   }

   PAGE_ID getMbpLpidOfCollection(UINT32 pageSize, CL_MB_ID mbID)
   {
      PAGE_ID lpid = INVALID_PAGE_ID;
      UINT32 capacity = 0;
      INT32 rc = getCapacityOfCLMetaBlockPage(pageSize, capacity);
      if (SDB_OK != rc)
      {
         goto done;
      }

      lpid = mbID / capacity + CL_META_BLOCK_PAGE_MIN_LPID;
   done:
      return lpid;
   }
}//namespace vessel
}//namespace engine