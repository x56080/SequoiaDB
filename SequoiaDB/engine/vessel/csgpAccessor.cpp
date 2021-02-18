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

   Source File Name = csgpAccessor.cpp

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

#include "vessel/csgpAccessor.h"
#include "vessel/collectionSpaceGlobalPage.h"
#include "pdTrace.hpp"

namespace engine
{
namespace vessel
{
   csgpAccessor::csgpAccessor()
   {}

   csgpAccessor::~csgpAccessor()
   {}
   
   INT32 csgpAccessor::initPage(const csMetaRecord &record)
   {
      INT32 rc = SDB_OK;
      CHAR * ptr = NULL;
      UINT32 flags = PAGE_ACCESSOR_FLAG_DIRECT |
                     PAGE_ACCESSOR_FLAG_NON_READONLY |
                     PAGE_ACCESSOR_FLAG_INIT_PAGE;
      SDB_ASSERT(OSS_BIT_TEST(getFlags(), flags), "impossible");

      rc = prepareToWrite();
      if (SDB_OK != rc)
      {
         goto error;
      }

      rc = initCommonPageHeadAndTail();
      if (SDB_OK != rc)
      {
         goto error;
      }

      rc = memsetPageBody(0);
      if (SDB_OK != rc)
      {
         goto error;
      }

      rc = getWritePtrOfPageBody(0, sizeof(csMetaRecord), &ptr);
      if (SDB_OK != rc)
      {
         goto error;
      }

      ossMemcpy(ptr, &record, sizeof(csMetaRecord));

      pageAccessor::commit(DPS_INVALID_LSN_OFFSET);
   done:
      return rc;
   error:
      if (fullAccessing())
      {
         abortToWrite();
      }
      goto done;
   }

   INT32 csgpAccessor::readMetaRecord(csMetaRecord &record)
   {
      INT32 rc = SDB_OK;
      const CHAR *ptr = NULL;
      const csMetaRecordOnDisk *recordPtr = NULL;
      rc = getReadPtrOfPageBody(0, CS_META_RECORD_ON_DISK_LEN, &ptr);
      if (SDB_OK != rc)
      {
         goto error;
      }

      recordPtr = (const csMetaRecordOnDisk *)ptr;
      if (!metaRecordIsValid(*recordPtr))
      {
         PD_LOG(PDERROR, "page crashed[%d, %d, %d]", getGPID().space(), getGPID().type(), getGPID().page());
         rc = SDB_VESSEL_INVALID_PAGE_CONTENT;
         goto error;
      }

      record = recordPtr->record;
   done:
      return rc;
   error:
      goto done;
   }
}//namespace vessel
}//namespace engine