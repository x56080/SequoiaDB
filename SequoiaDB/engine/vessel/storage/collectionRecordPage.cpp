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

   Source File Name = collectionRecordPage.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#include "vessel/collectionRecordPage.h"

namespace engine
{
namespace vessel
{
   INT32 getCapacityOfCLRecordPage(UINT32 pageSize, UINT32 &capacity)
   {
      INT32 rc = SDB_OK;
      if (DMS_PAGE_SIZE32K != pageSize &&
          DMS_PAGE_SIZE64K != pageSize)
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      capacity = (pageSize - PAGE_HEAD_SIZE - PAGE_TAIL_SIZE - COLLECTION_RECORD_PAGE_HEAD_LEN) / COLLECTION_DISK_RECORD_LEN;
   done:
      return rc;
   error:
      goto done;
   }

   BOOLEAN initCollectionRecordPage(UINT32 pageSize,
                                    PAGE_ID pid,
                                    PAGE_ID lpid,
                                    PAGE_SNAPSHOT_VERION psv,
                                    void *buf)
   {
      BOOLEAN r = FALSE;
      UINT32 capacity = 0;
      collectionRecordPageHead *head = NULL;
      collectionRecord record;
      UINT32 offset = 0;
      if (SDB_OK != getCapacityOfCLRecordPage(pageSize, capacity))
      {
         goto done;
      }

      if (!initCommonPage(PAGE_TYPE_COLLECTION_RECORD, pageSize,
                          pid, lpid, psv, buf))
      {
         goto done;
      }

      head = (collectionRecordPageHead *)((ossValuePtr)buf + PAGE_HEAD_SIZE);
      head->version = COLLECTION_RECORD_PAGE_VERSION_1;
      head->flags = 0;
      head->pad0 = 0;
      head->pad1 = 0;
      
      offset = PAGE_HEAD_SIZE + COLLECTION_RECORD_PAGE_HEAD_LEN;
      for (UINT32 i = 0; i < capacity; ++i)
      {
         collectionRecord *recordPtr = (collectionRecord *)((ossValuePtr)buf + offset);
         ossMemcpy(recordPtr, &record, COLLECTION_RECORD_LEN);
         offset += COLLECTION_DISK_RECORD_LEN;
      }
      r = TRUE;
   done:
      return r;
   }
}//namespace vessel
}//namespace engine