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

   Source File Name = routePageAccessor.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#include "vessel/routePageAccessor.h"
#include "ossLikely.hpp"
#include "vessel/routePage.h"

namespace engine
{
namespace vessel
{
   INT32 routePageAccessor::initPage(requestContext *context,
                                     PAGE_ID lpid,
                                     UINT32 logicalId)
   {
      INT32 rc = SDB_OK;
      UINT32 flags = PAGE_ACCESSOR_FLAG_INIT_PAGE | 
                     PAGE_ACCESSOR_FLAG_DIRECT |
                     PAGE_ACCESSOR_FLAG_NON_READONLY;
      SDB_ASSERT(OSS_BIT_TEST(getFlags(), flags), "impossible");
      UINT32 capacity;
      routePageHead *head = NULL;
      CHAR *buffer = NULL;

      if (OSS_UNLIKELY(INVALID_PAGE_ID == lpid ||
                       DMS_INVALID_LOGICCLID == logicalId))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      capacity = getCapacityOfRoutePage(pageAccessor::getPageSize());
      if (OSS_UNLIKELY(0 == capacity))
      {
         PD_LOG(PDERROR, "failed to get capacity of route page");
         rc = SDB_VESSEL_INTERNAL_ERR;
         goto error;
      }

      rc = prepareToWrite(context);
      if (SDB_OK != rc)
      {
         goto error;
      }

      rc = initCommonPageHeadAndTail(lpid);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to init common head:%d", rc);
         goto error;
      }

      rc = memsetPageBody(0);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to memset page body:%d", rc);
         goto error;
      }

      rc = getWritableUserHeadPtr<routePageHead>(&head);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to get page head ptr:%d", rc);
         goto error;
      }

      head->version = ROUTE_PAGE_VERSION;
      head->logicalId = logicalId;
      head->count = 0;
      head->pad = 0;

      buffer = (CHAR *)head + ROUTE_PAGE_HEAD_LEN;
      ossMemset(buffer, 0xFF, capacity << 2);
      pageAccessor::commit(context, DPS_INVALID_LSN_OFFSET);
   done:
      return rc;
   error:
      if (fullAccessing())
      {
         abortToWrite();
      }
      goto done;
   }
}//namespace vessel
}//namespace engine