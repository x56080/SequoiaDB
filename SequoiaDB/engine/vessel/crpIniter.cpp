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

   Source File Name = crpIniter.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#include "vessel/crpIniter.hpp"
#include "ossLikely.hpp"
#include "vessel/runtimePageBuffer.h"
#include "vessel/vesselOptions.h"
#include "vessel/logRecordContext.h"

namespace engine
{
namespace vessel
{
   INT32 crpIniter::initPage(requestContext *context,
                             PAGE_ID lpid,
                             PAGE_SNAPSHOT_VERION psv,
                             runtimePageBuffer *rpb)
   {
      INT32 rc = SDB_OK;
      logRecordContext lrc;
      csMetaRecord *ptr = NULL;

      if (OSS_UNLIKELY(NULL == context ||
                       INVALID_PAGE_ID == lpid ||
                       INVALID_PAGE_SNAPSHOT_VERSION == psv ||
                       NULL == rpb ||
                       !rpb->isValid()))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      else if (NULL == _record ||
               !_record->isValid() ||
               NULL == _options ||
               !_options->isValid())
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }

      SDB_ASSERT(rpb->isWritingPrepared(), "must be prepared");
      SDB_ASSERT(rpb->isCacheBuffer(), "must be cache buffer");
      
      if (!initGmp(rpb->getPageSize(),
                   rpb->getGlobalPid().page(),
                   lpid, psv, rpb->getBuffer()))
      {
         PD_LOG(PDERROR, "failed to init cs meta page");
         rc = SDB_VESSEL_INTERNAL_ERR;
         goto error;
      }

      ptr = rpb->getWritablePtrOfBody<csMetaRecord>(0);
      if (NULL == ptr)
      {
         PD_LOG(PDERROR, "failed to get write ptr");
         rc = SDB_VESSEL_INTERNAL_ERR;
         goto error;
      }

      ossMemcpy(ptr, _record, CS_META_RECORD_LEN);

      rc = pageAccessor::prepareLog(context, rpb,
                                    LOG_TYPE_VESSEL_UPDATE_CS_META,
                                    TRUE, &lrc);
      if (SDB_OK != rc)
      {
         
      }
   done:
      return rc;
   error:
      goto done;
   }
}//namespace vessel
}//namespace engine
