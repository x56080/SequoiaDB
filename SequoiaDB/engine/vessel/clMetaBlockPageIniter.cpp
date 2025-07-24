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

   Source File Name = clMetaBlockPageIniter.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

*******************************************************************************/
#include "vessel/clMetaBlockPageIniter.h"
#include "vessel/runtimePageBuffer.h"
#include "vessel/clMetaBlockPage.h"
#include "vessel/outerResource.h"
#include "vessel/requestContext.h"
#include "dpsLogRecordDef.hpp"

namespace engine
{
namespace vessel
{
   INT32 clMetaBlockPageIniter::initPage(requestContext *context,
                                         PAGE_ID lpid,
                                         PAGE_SNAPSHOT_VERION psv,
                                         runtimePageBuffer *rpb)
   {
      INT32 rc = SDB_OK;
      DPS_LSN_OFFSET lsn = DPS_INVALID_LSN_OFFSET;

      if (OSS_UNLIKELY(NULL == context ||
                       INVALID_PAGE_ID == lpid ||
                       INVALID_PAGE_SNAPSHOT_VERSION == psv ||
                       NULL == rpb ||
                       !rpb->isValid()))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      else if (!rpb->isCacheBuffer() ||
               !rpb->isWritingPrepared())
      {
         SDB_ASSERT(FALSE, "impossible");
         rc = SDB_INVALIDARG;
         goto error;
      }

      if (!initCLMetaBlockPage(rpb->getPageSize(),
                               rpb->getGlobalPid().page(),
                               lpid, psv, rpb->getWritableBuffer().getWPtr()))
      {
         PD_LOG(PDERROR, "failed to init cl meta block page");
         rc = SDB_VESSEL_INTERNAL_ERR;
         goto error;
      }

      rc = pageInitializer::writeJournal(context, rpb->getGlobalPid(),
                                         PAGE_TYPE_CL_META, slice(), lsn);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to write journal:%d", rc);
         goto error;
      }
   
      rpb->commit(lsn);
   done:
      return rc;
   error:
      goto done;
   }
}//namespace vessel
}//namespace engine