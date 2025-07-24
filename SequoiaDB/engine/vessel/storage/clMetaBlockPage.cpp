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

   Source File Name = clMetaBlockPage.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

*******************************************************************************/
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