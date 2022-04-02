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

   Source File Name = clMetaBlockPageIniter.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#include "vessel/clMetaBlockPageIniter.h"
#include "vessel/runtimePageBuffer.h"
#include "vessel/clMetaBlockPage.h"
#include "vessel/logRecordContext.h"
#include "vessel/outerResource.h"
#include "vessel/IRedoLogger.h"
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
      logRecordContext lrc;

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

      rc = pageInitializer::prepareInitLog(context, 0, rpb, &lrc);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to prepare log:%d", rc);
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

      rc = pageInitializer::commitInitLog(context, rpb->getGlobalPid(),
                                          lpid, psv, PAGE_TYPE_CL_META,
                                          slice(), &lrc);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to commit log[%lld], rc:%d", lrc.getLsn(), rc);
         goto error;
      }

      rpb->commit(lrc.getLsn());
   done:
      return rc;
   error:
      goto done;
   }
}//namespace vessel
}//namespace engine