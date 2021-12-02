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

   Source File Name = recordDataPage.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#include "vessel/recordDataPage.h"

namespace engine
{
namespace vessel
{
   BOOLEAN initRecordDataPage(UINT32 pageSize,
                              PAGE_ID pid,      
                              PAGE_ID lpid,
                              PAGE_SNAPSHOT_VERION psv,
                              UINT32 logicalID,
                              UINT32 pageSeq,
                              void *buf)
   {
      SDB_ASSERT(DMS_INVALID_LOGICCLID != logicalID, "can not be invalid");
      SDB_ASSERT(INVALID_CL_PAGE_SEQ != pageSeq, "can not be invalid");

      recordDataPageHead *head = NULL;
      CHAR *ptr = (CHAR *)buf;
      BOOLEAN r = initCommonPage(PAGE_TYPE_RECORD, pageSize,
                                 pid, lpid, psv, buf);
      if (!r)
      {
         goto done;
      }

      head = (recordDataPageHead *)(ptr + PAGE_HEAD_SIZE);
      *head = recordDataPageHead();
      head->version = RDP_VERSION;
      head->clLogcalID = logicalID;
      head->pageSeq = pageSeq;
      head->totalFreeSpace = getMaxFreeSizeOfRdp(pageSize);
      head->backOffset = getPageBodySize(pageSize);
   done:
      return r;
   }
}//namespace vessel
}//namespace engine