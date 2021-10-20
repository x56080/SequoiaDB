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

   Source File Name = indexMappingPageIniter.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#include "vessel/indexMappingPageIniter.h"
#include "vessel/runtimePageBuffer.h"

namespace engine
{
namespace vessel
{
   INT32 indexMappingPageIniter::initPage(requestContext *context,
                                          PAGE_ID lpid,
                                          PAGE_SNAPSHOT_VERION psv,
                                          runtimePageBuffer *rpb)
   {
      INT32 rc = SDB_OK;
      CHAR *buffer = NULL;
      if (NULL == context ||
          INVALID_PAGE_ID == lpid ||
          INVALID_PAGE_SNAPSHOT_VERSION == psv ||
          NULL == rpb ||
          !rpb->isWritingPrepared())
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      SDB_ASSERT(!rpb->isCacheBuffer(), "impossible");

      if (!initCommonPage(PAGE_TYPE_INDEX_MAPPING,
                          rpb->getPageSize(),
                          rpb->getGlobalPid().page(),
                          lpid, psv,
                          rpb->getWritableSlice().getWPtr()))
      {
         PD_LOG(PDERROR, "failed to init page");
         rc = SDB_VESSEL_INTERNAL_ERR;
         goto error;
      }

      buffer = rpb->getWritableBodySlice().getWPtr();
      ossMemset(buffer, 0xFF, getPageBodySize(rpb->getPageSize()));
      rpb->commit(DPS_INVALID_LSN_OFFSET);
   done:
      return rc;
   error:
      goto done;
   }
}//namespace vessel
}//namespace engine
