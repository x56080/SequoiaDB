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

   Source File Name = csMetaBlockPageIniter.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

*******************************************************************************/
#include "vessel/csMetaBlockPageIniter.h"
#include "pdTrace.hpp"
#include "vessel/csMetaBlockPage.h"
#include "vessel/requestContext.h"
#include "vessel/strictBuffer.h"
#include "vessel/runtimePageBuffer.h"
#include "vessel/csMetaBlockPage.h"

namespace engine
{
namespace vessel
{
   INT32 csMetaBlockPageIniter::initPage(requestContext *context,
                                         PAGE_ID lpid,
                                         PAGE_SNAPSHOT_VERION psv,
                                         runtimePageBuffer *rpb)
   {
      INT32 rc = SDB_OK;
      strictBuffer buffer;
      csMetaBlock *blockInBuffer = nullptr;
      SDB_ASSERT(DPS_INVALID_LSN_OFFSET != getLSN(), "can not be invalid");

      if (OSS_UNLIKELY(nullptr == context ||
                       INVALID_PAGE_ID == lpid ||
                       INVALID_PAGE_SNAPSHOT_VERSION == psv ||
                       nullptr == rpb ||
                       !rpb->isWritingPrepared()))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      else if (DPS_INVALID_LSN_OFFSET == getLSN() ||
               nullptr == _block || !_block->isValid())
      {
         SDB_ASSERT(FALSE, "lsn can not be invalid");
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }

      buffer = rpb->getWritableBuffer();
      SDB_ASSERT(buffer.isWritable(), "impossible");
      if (!initCSMetaBlockPage(rpb->getPageSize(),
                               rpb->getGlobalPid().getPageId(),
                               lpid, psv, buffer.getWPtr()))
      {
         PD_LOG(PDERROR, "faield to init gmp");
         rc = SDB_VESSEL_INTERNAL_ERR;
         goto error;
      }

      blockInBuffer = buffer.getWritableObjPtr<csMetaBlock>(PAGE_HEAD_SIZE);
      SDB_ASSERT(nullptr != blockInBuffer, "impossible");
      *blockInBuffer = *_block;
      rpb->commit(getLSN());

   done:
      return rc;
   error:
      goto done;
   }
} // namespace vessel

} // namespace engine
