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

   Source File Name = csMetaBlockPageAccessor.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

*******************************************************************************/
#include "vessel/csMetaBlockPageAccessor.h"
#include "pdTrace.hpp"
#include "vessel/logicalPageBuffer.h"
#include "dpsLogRecordDef.hpp"
#include "dpsWriteReqBuilder.hpp"
#include "interface/IDataJournal.h"
#include "vessel/outerResource.h"
#include "vessel/requestContext.h"

namespace engine
{
namespace vessel
{
   INT32 csMetaBlockPageAccessor::read(requestContext *context,
                                       const logicalPageBuffer *lpb,
                                       csMetaBlock &cmb)
   {
      INT32 rc = SDB_OK;
      const runtimePageBuffer *rpb = NULL;
      const csMetaBlock *record = NULL;
      strictBuffer buffer;

      if (NULL == context ||
          NULL == lpb ||
          !lpb->isValid())
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      rpb = &(lpb->getRuntimeBuffer());
      buffer = lpb->getReadableBodyBuffer();
      rc = lpb->validatePage(PAGE_TYPE_CS_META);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to validate page[%s], rc:%d",
                rpb->getGlobalPid().toString().c_str(), rc);
         goto error;
      }

      record = buffer.getReadableObjPtr<csMetaBlock>(0);
      if (NULL == record)
      {
         PD_LOG(PDERROR, "failed to get readable record ptr");
         rc = SDB_VESSEL_INTERNAL_ERR;
         goto error;
      }

      if (!record->isValid())
      {
         PD_LOG(PDERROR, "collection space meta data is not valid");
         rc = SDB_VESSEL_PAGE_CRASHED;
         goto error;
      }
      cmb = *record;      
   done:
      return rc;
   error:
      goto done;
   }

   INT32 csMetaBlockPageAccessor::update(requestContext *context,
                                         logicalPageBuffer *lpb,
                                         const csMetaBlock &block,
                                         UINT64 mask)
   {
      INT32 rc = SDB_OK;
      const csMetaBlock *rBlockPtr = nullptr;
      csMetaBlock *blockPtr = nullptr;
      csMetaBlock oldBlock;
      strictBuffer buffer;
      DPS_LSN_OFFSET lsn = DPS_INVALID_LSN_OFFSET;

      if (OSS_UNLIKELY(nullptr == context ||
                       nullptr == lpb ||
                       !lpb->isValid() ||
                       !block.isValid() ||
                       0 == mask))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      buffer = lpb->getReadableBodyBuffer();
      if (OSS_UNLIKELY(!buffer.isValid()))
      {
         rc = SDB_VESSEL_INTERNAL_ERR;
         PD_LOG(PDERROR, "failed to get readable body buffer");
         goto error;
      }

      rBlockPtr = buffer.getReadableObjPtr<csMetaBlock>(0);
      if (OSS_UNLIKELY(nullptr == rBlockPtr))
      {
         rc = SDB_VESSEL_INTERNAL_ERR;
         PD_LOG(PDERROR, "failed to get readable cs meta block ptr");
         goto error;
      }

      if (OSS_UNLIKELY(!rBlockPtr->isValid()))
      {
         rc = SDB_VESSEL_INTERNAL_ERR;
         PD_LOG(PDERROR, "invalid cs meta block on disk");
         goto error;
      }

      rc = lpb->autoGetWritableBodyBuffer(buffer);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to get writable buffer, rc:%d", rc);
         goto error;
      }

      blockPtr = buffer.getWritableObjPtr<csMetaBlock>(0);
      oldBlock = *blockPtr;

      rc = writeJournal(context, lpb->getGlobalPid(),
                        oldBlock, block, mask, lsn);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to write journal:%d", rc);
         goto error;
      }

      *blockPtr = block;


      lpb->commit(lsn);

   done:
      return rc;
   error:
      goto done;
   }

   INT32 csMetaBlockPageAccessor::writeJournal(requestContext *context,
                                               const GLOBAL_PAGE_ID &gpid,
                                               const csMetaBlock &oldBlock,
                                               const csMetaBlock &block,
                                               UINT64 mask,
                                               DPS_LSN_OFFSET &lsn)
   {
      INT32 rc = SDB_OK;
      dpsWriteReqBuilder jpad;
      dpsWriteRequest jrequest;
      dpsLogRecordHeader jres;
      IDataJournal *journal = context->getOuterResource()->journal;

      jpad.setType(LOG_TYPE_VESSEL_CSMB_UPDATE);

      rc = jpad.append(DPS_LOG_PUBLIC_VESSEL_GPID,
                       sizeof(GLOBAL_PAGE_ID), &gpid);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to append gpid:%d", rc);
         goto error;
      }

      rc = jpad.appendInt64(DPS_LOG_VESSEL_CSMB_UPDATE_MASK, mask);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to append mask:%d", rc);
         goto error;
      }

      rc = jpad.append(DPS_LOG_VESSEL_CSMB_UPDATE_OLD,
                       sizeof(csMetaBlock), &oldBlock);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to append old block:%d", rc);
         goto error;
      }

      rc = jpad.append(DPS_LOG_VESSEL_CSMB_UPDATE_NEW,
                       sizeof(csMetaBlock), &block);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to append new block:%d", rc);
         goto error;
      }

      jrequest = jpad.reap();

      rc = journal->write(context->getExecutor(), jrequest, dpsWriteOptions(), &jres);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failedd to write journal:%d", rc);
         goto error;
      }

      lsn = jres._lsn;

   done:
      return rc;
   error:
      goto done;
   }
}//namespace vessel
}//namespace engine