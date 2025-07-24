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

   Source File Name = btreeEntryPageIniter.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

*******************************************************************************/
#include "vessel/btreeEntryPageIniter.h"
#include "vessel/runtimePageBuffer.h"
#include "vessel/btreeEntryPage.h"

namespace engine
{
namespace vessel
{
   btreeEntryPageIniter::btreeEntryPageIniter(UINT32 logicalIndexId):
   _logicalIndexId(logicalIndexId)
   {
      SDB_ASSERT(INVALID_LOGICAL_INDEX_ID != _logicalIndexId, "can not be invalid");
   }

   INT32 btreeEntryPageIniter::initPage(requestContext *context,
                                       PAGE_ID lpid,
                                       PAGE_SNAPSHOT_VERION psv,
                                       runtimePageBuffer *rpb)
   {
      INT32 rc = SDB_OK;

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

      if (!initBtreeEntryPage(rpb->getPageSize(),
                              rpb->getGlobalPid().page(),
                              lpid, psv, _logicalIndexId,
                              rpb->getWritableBuffer().getWPtr()))
      {
         PD_LOG(PDERROR, "failed to init index def page[%s]",
                rpb->getGlobalPid().toString().c_str());
         rc = SDB_VESSEL_INTERNAL_ERR;
         goto error;
      }

      rpb->commit(DPS_INVALID_LSN_OFFSET);
   done:
      return rc;
   error:
      goto done;
   }
}//namespace vessel
}//namespace engine
