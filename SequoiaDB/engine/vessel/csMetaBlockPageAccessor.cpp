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

   Source File Name = csMetaBlockPageAccessor.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#include "vessel/csMetaBlockPageAccessor.h"
#include "pdTrace.hpp"
#include "vessel/logicalPageBuffer.h"
#include "vessel/logRecordContext.h"
#include "dpsLogRecordDef.hpp"

namespace engine
{
namespace vessel
{
   static const UINT32 UPDATE_CS_META_TYPE_LID = 0;

   csMetaBlockPageAccessor::csMetaBlockPageAccessor()
   {}

   csMetaBlockPageAccessor::~csMetaBlockPageAccessor()
   {}

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
      logRecordContext lrc;

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

      rc = prepareUpdateLog(context, &(lpb->getRuntimeBuffer()), &lrc);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to prepare dps log, rc:%d", rc);
         goto error;
      }

      *blockPtr = block;
      rc = commitUpdateLog(context, lpb->getRuntimeBuffer().getGlobalPid(),
                           oldBlock, *blockPtr, 
                           mask, &lrc);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to commit dps log, rc:%d");
         *blockPtr = oldBlock;
         goto error;
      }

      lpb->commit(lrc.getLsn());

   done:
      return rc;
   error:
      goto done;
   }

   INT32 csMetaBlockPageAccessor::prepareUpdateLog(requestContext *context,
                                                   const runtimePageBuffer *rpb,
                                                   logRecordContext *lrc)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(nullptr != context, "can not be null");
      SDB_ASSERT(nullptr != rpb, "can not be null");
      SDB_ASSERT(nullptr != lrc, "can not be null");
      SDB_ASSERT(!lrc->prepared(), "can not be prepared");

      rc = pageAccessor::prepareLog(context, rpb,
                                    LOG_TYPE_VESSEL_CSMB_UPDATE,
                                    FALSE, lrc);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to prepare log:%d", rc);
         goto error;
      }

      lrc->prepush(sizeof(GLOBAL_PAGE_ID));
      lrc->prepush(sizeof(UINT64));
      lrc->prepush(sizeof(CS_META_BLOCK_LEN));
      lrc->prepush(sizeof(CS_META_BLOCK_LEN));

      rc = pageAccessor::prepareLogDone(context, lrc);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to prepare log done:%d", rc);
         goto error;
      }
   done:
      return rc;
   error:
      goto done;
   }

   INT32 csMetaBlockPageAccessor::commitUpdateLog(requestContext *context,
                                                  const GLOBAL_PAGE_ID &gpid,
                                                  const csMetaBlock &oldBlock,
                                                  const csMetaBlock &block,
                                                  UINT64 mask,
                                                  logRecordContext *lrc)
   {                   
      INT32 rc = SDB_OK;
      SDB_ASSERT(nullptr != context, "can not be null");
      SDB_ASSERT(gpid.isValid(), "can not be invalid");
      SDB_ASSERT(oldBlock.isValid(), "can not be invalid");
      SDB_ASSERT(block.isValid(), "can not be invalid");
      SDB_ASSERT(0 != mask, "can not be zero");
      SDB_ASSERT(nullptr != lrc, "can not be null");
      SDB_ASSERT(lrc->prepared(), "must be prepared");

      rc = pageAccessor::pushElement(context, DPS_LOG_PUBLIC_VESSEL_GPID,
                                     sizeof(GLOBAL_PAGE_ID), &gpid, lrc);
      if (SDB_OK != rc)
      {
         goto error;
      }
      
      rc = pageAccessor::pushElement(context, DPS_LOG_VESSEL_CSMB_UPDATE_MASK,
                                     sizeof(UINT64), &mask, lrc);
      if (SDB_OK != rc)
      {
         goto error;
      }

      rc = pageAccessor::pushElement(context, DPS_LOG_VESSEL_CSMB_UPDATE_OLD,
                                     sizeof(CS_META_BLOCK_LEN), &oldBlock, lrc);
      if (SDB_OK != rc)
      {
         goto error;
      }

      rc = pageAccessor::pushElement(context, DPS_LOG_VESSEL_CSMB_UPDATE_NEW,
                                     sizeof(CS_META_BLOCK_LEN), &block, lrc);
      if (SDB_OK != rc)
      {
         goto error;
      }

      rc = pageAccessor::commitLog(context, lrc);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to commit log[%lld], rc:%d", lrc->getLsn(), rc);
         goto error;
      }
   done:
      return rc;
   error:
      goto done;
   }
}//namespace vessel
}//namespace engine