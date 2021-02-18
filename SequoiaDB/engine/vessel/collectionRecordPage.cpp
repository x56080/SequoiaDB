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

      capacity = (pageSize - PAGE_HEAD_LEN - PAGE_TAIL_LEN - COLLECTION_RECORD_PAGE_HEAD_LEN) / COLLECTION_DISK_RECORD_LEN;
   done:
      return rc;
   error:
      goto done;
   }
}//namespace vessel
}//namespace engine