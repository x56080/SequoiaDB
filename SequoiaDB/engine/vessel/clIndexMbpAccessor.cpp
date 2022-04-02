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

   Source File Name = clIndexMbpAccessor.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          03/16/2022  LYC  Initial Draft

   Last Changed =

******************************************************************************/

#include "vessel/clIndexMbpAccessor.h"
#include "vessel/logicalPageBuffer.h"
#include "vessel/requestContext.h"
#include "vessel/indexDef.h"
#include "vessel/strictBuffer.h"
#include "vessel/runtimeMbContext.h"

namespace engine
{
namespace vessel
{
   INT32 clIndexMbpAccessor::init(logicalPageBuffer *lpb)
   {
      INT32 rc = SDB_OK;

      _lpb = nullptr;
      if (OSS_UNLIKELY(nullptr == lpb ||
                       !lpb->isValid()))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      rc = lpb->validatePage(PAGE_TYPE_INDEX_META_BLOCK);
      if (SDB_OK != rc)
      {
         goto error;
      }

      _lpb = lpb;

   done:
      return rc;
   error:
      goto done;
   }

   void clIndexMbpAccessor::fini(BOOLEAN lpbNeedClose)
   {
      if (lpbNeedClose && nullptr != _lpb)
      {
         _lpb->fini();
      }
      _lpb = nullptr;
   }

   INT32 clIndexMbpAccessor::initIndexMetaBlock(requestContext *context,
                                                INT32 blockPos)
   {
      INT32 rc = SDB_OK;
      UINT32 capacity = 0;
      strictBuffer buf;
      clIndexMetaBlock *wBlockPtr = nullptr;

      if (OSS_UNLIKELY(nullptr == context ||
                       !context->isMbContextAttached() ||
                       0 > blockPos))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      else if (OSS_UNLIKELY(nullptr == _lpb ||
                            !_lpb->isValid()))
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }
      else if (OSS_UNLIKELY(!_lpb->getLockingMode().isExclusiveOrUpgrade()))
      {
         rc = SDB_VESSEL_FORBIDDEN_OP_WLT;
         goto error;
      }

      capacity = getCapacityOfIndexMetaBlockPage(_lpb->getPageSize());
      if (OSS_UNLIKELY(0 == capacity))
      {
         rc = SDB_VESSEL_INTERNAL_ERR;
         PD_LOG(PDERROR, "failed to get capacity of index meta block page, rc:%d", rc);
         goto error;
      }
      else if (OSS_UNLIKELY(blockPos >= capacity))
      {
         rc = SDB_OUT_OF_BOUND;
         PD_LOG(PDERROR, "block position is out of bound, rc:%d", rc);
         goto error;
      }

      rc = _lpb->autoGetWritableBodyBuffer(buf);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to get writable body buffer, rc:%d", rc);
         goto error;
      }
      else if (OSS_UNLIKELY(!buf.isValid()))
      {
         rc = SDB_VESSEL_INTERNAL_ERR;
         PD_LOG(PDERROR, "failed to get invalid writable buffer, rc:%d", rc);
         goto error;
      }

      wBlockPtr = buf.getWritableObjPtr<clIndexMetaBlock>(blockPos * CL_DISK_INDEX_META_BLOCK_LEN);
      if (OSS_UNLIKELY(nullptr == wBlockPtr))
      {
         rc = SDB_VESSEL_INTERNAL_ERR;
         PD_LOG(PDERROR, "failed to get writable ptr, rc:%d", rc);
         goto error;
      }

      wBlockPtr->version = CL_INDEX_META_BLOCK_VERSION;
      wBlockPtr->clLogicalId = context->getMbContext()->getGlobalId().getCLLid();
      wBlockPtr->maxIndexLid = 0;
      for (UINT32 i = 0; i < MAX_INDEX_COUNT_PER_CL; ++i)
      {
         wBlockPtr->entryPageLpids[i] = INVALID_PAGE_ID;
      }

      _lpb->commit(context->getExecutor()->getEndLsn());

   done:
      return rc;
   error:
      goto done;
   }

   INT32 clIndexMbpAccessor::resetIndexMetaBlock(requestContext *context,
                                                 INT32 blockPos)
   {
      INT32 rc = SDB_OK;
      UINT32 capacity = 0;
      strictBuffer buf;
      const clIndexMetaBlock *rBlockPtr = nullptr;
      clIndexMetaBlock *wBlockPtr = nullptr;

      if (OSS_UNLIKELY(nullptr == context ||
                       !context->isMbContextAttached() ||
                       0 > blockPos))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      else if (OSS_UNLIKELY(nullptr == _lpb ||
                            !_lpb->isValid()))
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }
      else if (OSS_UNLIKELY(!_lpb->getLockingMode().isExclusiveOrUpgrade()))
      {
         rc = SDB_VESSEL_FORBIDDEN_OP_WLT;
         goto error;
      }

      capacity = getCapacityOfIndexMetaBlockPage(_lpb->getPageSize());
      if (OSS_UNLIKELY(0 == capacity))
      {
         rc = SDB_VESSEL_INTERNAL_ERR;
         PD_LOG(PDERROR, "failed to get capacity of index meta block page");
         goto error;
      }
      else if (OSS_UNLIKELY(blockPos >= capacity))
      {
         rc = SDB_OUT_OF_BOUND;
         PD_LOG(PDERROR, "block position is out of bound, rc:%d", rc);
         goto error;
      }

      buf = _lpb->getReadableBodyBuffer();
      if (OSS_UNLIKELY(!buf.isValid()))
      {
         rc = SDB_VESSEL_INTERNAL_ERR;
         PD_LOG(PDERROR, "failed to get readable buffer, rc:%d", rc);
         goto error;
      }

      rBlockPtr = buf.getReadableObjPtr<clIndexMetaBlock>(blockPos * CL_DISK_INDEX_META_BLOCK_LEN);
      if (OSS_UNLIKELY(nullptr == rBlockPtr))
      {
         rc = SDB_VESSEL_INTERNAL_ERR;
         PD_LOG(PDERROR, "failed to get readable ptr, rc:%d", rc);
         goto error;
      }
      else if (OSS_UNLIKELY(rBlockPtr->clLogicalId !=
                            context->getMbContext()->getGlobalId().getCLLid()))
      {
         rc = SDB_VESSEL_INTERNAL_ERR;
         PD_LOG(PDERROR, "cl logical id[%d] does not match the cl logical id[%d] on block, rc:%d",
                context->getMbContext()->getGlobalId().getCLLid(),
                rBlockPtr->clLogicalId, rc);
         goto error;
      }

      rc = _lpb->autoGetWritableBodyBuffer(buf);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to get writable body buffer, rc:%d", rc);
         goto error;
      }
      else if (OSS_UNLIKELY(!buf.isValid()))
      {
         rc = SDB_VESSEL_INTERNAL_ERR;
         PD_LOG(PDERROR, "failed to get writable body buffer, rc:%d", rc);
         goto error;
      }

      wBlockPtr = buf.getWritableObjPtr<clIndexMetaBlock>(blockPos * CL_DISK_INDEX_META_BLOCK_LEN);
      if (OSS_UNLIKELY(nullptr == wBlockPtr))
      {
         rc = SDB_VESSEL_INTERNAL_ERR;
         PD_LOG(PDERROR, "failed to get writable ptr, rc:%d", rc);
         goto error;
      }

      wBlockPtr->reset();

      _lpb->commit(context->getExecutor()->getEndLsn());

   done:
      return rc;
   error:
      goto done;
   }

   INT32 clIndexMbpAccessor::getIndexMetaBlock(requestContext *context,
                                               INT32 blockPos,
                                               clIndexMetaBlock &block)
   {
      INT32 rc = SDB_OK;
      UINT32 capacity = 0;
      strictBuffer buf;
      const clIndexMetaBlock *rBlockPtr = nullptr;

      block.reset();
      if (OSS_UNLIKELY(nullptr == context ||
                       !context->isMbContextAttached() ||
                       0 > blockPos))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      else if (OSS_UNLIKELY(nullptr == _lpb ||
                            !_lpb->isValid()))
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }
      else if (OSS_UNLIKELY(_lpb->getLockingMode().isNone()))
      {
         rc = SDB_VESSEL_FORBIDDEN_OP_WLT;
         goto error;
      }

      capacity = getCapacityOfIndexMetaBlockPage(_lpb->getPageSize());
      if (OSS_UNLIKELY(0 == capacity))
      {
         rc = SDB_VESSEL_INTERNAL_ERR;
         PD_LOG(PDERROR, "failed to get capacity of index meta block page");
         goto error;
      }
      else if (OSS_UNLIKELY(blockPos >= capacity))
      {
         rc = SDB_OUT_OF_BOUND;
         PD_LOG(PDERROR, "block position is out of bound, rc:%d", rc);
         goto error;
      }

      buf = _lpb->getReadableBodyBuffer();
      if (OSS_UNLIKELY(!buf.isValid()))
      {
         rc = SDB_VESSEL_INTERNAL_ERR;
         PD_LOG(PDERROR, "failed to get readable buffer, rc:%d", rc);
         goto error;
      }

      rBlockPtr = buf.getReadableObjPtr<clIndexMetaBlock>(blockPos * CL_DISK_INDEX_META_BLOCK_LEN);
      if (OSS_UNLIKELY(nullptr == rBlockPtr))
      {
         rc = SDB_VESSEL_INTERNAL_ERR;
         PD_LOG(PDERROR, "failed to get readable ptr, rc:%d", rc);
         goto error;
      }
      else if (OSS_UNLIKELY(rBlockPtr->clLogicalId !=
                            context->getMbContext()->getGlobalId().getCLLid()))
      {
         rc = SDB_VESSEL_INTERNAL_ERR;
         PD_LOG(PDERROR, "cl logical id[%d] does not match the cl logical id[%d] on block, rc:%d",
                context->getMbContext()->getGlobalId().getCLLid(),
                rBlockPtr->clLogicalId, rc);
         goto error;
      }

      block = *rBlockPtr;

   done:
      return rc;
   error:
      goto done;
   }

   INT32 clIndexMbpAccessor::setIndexEntryPageLpid(requestContext *context,
                                                   INT32 blockPos,
                                                   INT32 slot,
                                                   PAGE_ID lpid,
                                                   UINT32 indexLid)
   {
      INT32 rc = SDB_OK;
      UINT32 capacity = 0;
      strictBuffer buf;
      const clIndexMetaBlock *rBlockPtr = nullptr;
      clIndexMetaBlock *wBlockPtr = nullptr;

      if (OSS_UNLIKELY(nullptr == context ||
                       !context->isMbContextAttached() ||
                       0 > blockPos ||
                       !isValidIndexSlot(slot) ||
                       INVALID_PAGE_ID == lpid))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      else if (OSS_UNLIKELY(nullptr == _lpb ||
                            !_lpb->isValid()))
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }
      else if (OSS_UNLIKELY(!_lpb->getLockingMode().isExclusiveOrUpgrade()))
      {
         rc = SDB_VESSEL_FORBIDDEN_OP_WLT;
         goto error;
      }

      capacity = getCapacityOfIndexMetaBlockPage(_lpb->getPageSize());
      if (OSS_UNLIKELY(0 == capacity))
      {
         rc = SDB_VESSEL_INTERNAL_ERR;
         PD_LOG(PDERROR, "failed to get capacity of index meta block page");
         goto error;
      }
      else if (OSS_UNLIKELY(blockPos >= capacity))
      {
         rc = SDB_OUT_OF_BOUND;
         PD_LOG(PDERROR, "block position is out of bound, rc:%d", rc);
         goto error;
      }

      buf = _lpb->getReadableBodyBuffer();
      if (OSS_UNLIKELY(!buf.isValid()))
      {
         PD_LOG(PDERROR, "failed to get readable buffer, rc:%d", rc);
         goto error;
      }

      rBlockPtr = buf.getReadableObjPtr<clIndexMetaBlock>(blockPos * CL_DISK_INDEX_META_BLOCK_LEN);
      if (OSS_UNLIKELY(nullptr == rBlockPtr))
      {
         rc = SDB_VESSEL_INTERNAL_ERR;
         PD_LOG(PDERROR, "failed to get readable ptr, rc:%d", rc);
         goto error;
      }
      else if (OSS_UNLIKELY(!rBlockPtr->isValid()))
      {
         rc = SDB_VESSEL_INTERNAL_ERR;
         PD_LOG(PDERROR, "invalid cl index meta block, rc:%d", rc);
         goto error;
      }
      else if (OSS_UNLIKELY(rBlockPtr->clLogicalId !=
                            context->getMbContext()->getGlobalId().getCLLid()))
      {
         rc = SDB_VESSEL_INTERNAL_ERR;
         PD_LOG(PDERROR, "cl logical id[%d] does not match the cl logical id[%d] on block, rc:%d",
                context->getMbContext()->getGlobalId().getCLLid(),
                rBlockPtr->clLogicalId, rc);
         goto error;
      }

      rc = _lpb->autoGetWritableBodyBuffer(buf);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to get writable body buffer, rc:%d", rc);
         goto error;
      }
      else if (OSS_UNLIKELY(!buf.isValid()))
      {
         rc = SDB_VESSEL_INTERNAL_ERR;
         PD_LOG(PDERROR, "failed to get writable body buffer, rc:%d", rc);
         goto error;
      }

      wBlockPtr = buf.getWritableObjPtr<clIndexMetaBlock>(blockPos * CL_DISK_INDEX_META_BLOCK_LEN);
      if (OSS_UNLIKELY(nullptr == wBlockPtr))
      {
         PD_LOG(PDERROR, "failed to get writable ptr, rc:%d", rc);
         goto error;
      }

      wBlockPtr->entryPageLpids[slot] = lpid;
      if (wBlockPtr->maxIndexLid < indexLid)
      {
         wBlockPtr->maxIndexLid = indexLid;
      }

      _lpb->commit(context->getExecutor()->getEndLsn());

   done:
      return rc;
   error:
      goto done;
   }

   INT32 clIndexMbpAccessor::resetIndexEntryPageLpid(requestContext *context,
                                                     INT32 blockPos,
                                                     INT32 slot)
   {
      INT32 rc = SDB_OK;
      UINT32 capacity = 0;
      strictBuffer buf;
      const clIndexMetaBlock *rBlockPtr = nullptr;
      clIndexMetaBlock *wBlockPtr = nullptr;

      if (OSS_UNLIKELY(nullptr == context ||
                       !context->isMbContextAttached() ||
                       0 > blockPos ||
                       !isValidIndexSlot(slot)))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      else if (OSS_UNLIKELY(nullptr == _lpb ||
                            !_lpb->isValid()))
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }
      else if (OSS_UNLIKELY(!_lpb->getLockingMode().isExclusiveOrUpgrade()))
      {
         rc = SDB_VESSEL_FORBIDDEN_OP_WLT;
         goto error;
      }

      capacity = getCapacityOfIndexMetaBlockPage(_lpb->getPageSize());
      if (OSS_UNLIKELY(0 == capacity))
      {
         rc = SDB_VESSEL_INTERNAL_ERR;
         PD_LOG(PDERROR, "failed to get capacity of index meta block page");
         goto error;
      }
      else if (OSS_UNLIKELY(blockPos >= capacity))
      {
         rc = SDB_OUT_OF_BOUND;
         PD_LOG(PDERROR, "block position is out of bound, rc:%d", rc);
         goto error;
      }

      buf = _lpb->getReadableBodyBuffer();
      if (OSS_UNLIKELY(!buf.isValid()))
      {
         rc = SDB_VESSEL_INTERNAL_ERR;
         PD_LOG(PDERROR, "failed to get readable buffer, rc:%d", rc);
         goto error;
      }
      
      rBlockPtr = buf.getReadableObjPtr<clIndexMetaBlock>(blockPos * CL_DISK_INDEX_META_BLOCK_LEN);
      if (OSS_UNLIKELY(nullptr == rBlockPtr))
      {
         rc = SDB_VESSEL_INTERNAL_ERR;
         PD_LOG(PDERROR, "failed to get readable ptr, rc:%d", rc);
         goto error;
      }
      else if (OSS_UNLIKELY(!rBlockPtr->isValid()))
      {
         rc = SDB_VESSEL_INTERNAL_ERR;
         PD_LOG(PDERROR, "invalid cl index meta block, rc:%d", rc);
         goto error;
      }
      else if (OSS_UNLIKELY(rBlockPtr->clLogicalId !=
                            context->getMbContext()->getGlobalId().getCLLid()))
      {
         rc = SDB_VESSEL_INTERNAL_ERR;
         PD_LOG(PDERROR, "cl logical id[%d] does not match the cl logical id[%d] on block, rc:%d",
                context->getMbContext()->getGlobalId().getCLLid(),
                rBlockPtr->clLogicalId, rc);
         goto error;
      }

      rc = _lpb->autoGetWritableBodyBuffer(buf);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to get writable body buffer, rc:%d", rc);
         goto error;
      }
      else if (OSS_UNLIKELY(!buf.isValid()))
      {
         rc = SDB_VESSEL_INTERNAL_ERR;
         PD_LOG(PDERROR, "failed to get writable body buffer, rc:%d", rc);
         goto error;
      }

      wBlockPtr = buf.getWritableObjPtr<clIndexMetaBlock>(blockPos * CL_DISK_INDEX_META_BLOCK_LEN);
      if (OSS_UNLIKELY(nullptr == wBlockPtr))
      {
         PD_LOG(PDERROR, "failed to get writable ptr, rc:%d", rc);
         goto error;
      }

      wBlockPtr->entryPageLpids[slot] = INVALID_PAGE_ID;

      _lpb->commit(context->getExecutor()->getEndLsn());
   
   done:
      return rc;
   error:
      goto done;
   }

} // namespace vessel
} // namespace engine