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

   Source File Name = csMetaBlockPageIniter.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

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
