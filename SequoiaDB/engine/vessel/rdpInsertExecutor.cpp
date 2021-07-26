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

   Source File Name = rdpInsertExecutor.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#include "vessel/rdpInsertExecutor.h"
#include "vessel/insertContext.h"
#include "vessel/logicalPageBuffer.h"
#include "vessel/logRecordContext.h"
#include "dpsLogRecordDef.hpp"
#include "vessel/fsmCandidate.h"

namespace engine
{
namespace vessel
{
   INT32 rdpInsertExecutor::insertNormalRecord(insertContext *context,
                                               logicalPageBuffer *lpb)
   {
      INT32 rc = SDB_OK;
      slice record;
      const runtimePageBuffer *rpb = NULL;
      const recordDataPageHead *head = NULL;

      RECORD_SLOT_ID slotId = INVALID_RECORD_SLOT_ID;
      recordSlot rs;
      UINT32 alignedSize = 0;
      UINT32 totalSize = 0;
      UINT16 offset = 0;
      recordHead rh;
      recordID rid;
      INT32 oldLvl = FSM_INVALID_SPACE_LVL;
      INT32 newLvl = FSM_INVALID_SPACE_LVL;
      UINT32 newFreeSize = 0;

      if (OSS_UNLIKELY(NULL == context ||
                       NULL == lpb ||
                       !lpb->isValid()))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      else if (OSS_UNLIKELY(!context->clInfoIsValid()))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      record = context->getRecordToInsert();
      if (!record.isValid())
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      else if (isBigRecord(lpb->getRuntimeBuffer().getPageSize(),
                           record.len()))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      rpb = &(lpb->getRuntimeBuffer());
      rc = validatePage((ossValuePtr)(rpb->getReadOnlyBuffer()),
                         PAGE_TYPE_COLLECTION_RECORD,
                         rpb->getPageSize(),
                         rpb->getGlobalPid().page(),
                         lpb->getLogicalPid(),
                         lpb->getCowTrigger().getPsv());
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to validate page[%s], rc:%d",
                rpb->getGlobalPid().toString().c_str(), rc);
         goto error;
      }

      head = rpb->getReadablePtrOfBody<recordDataPageHead>(0);
      if (NULL == head)
      {
         PD_LOG(PDERROR, "failed to get readble record page head");
         rc = SDB_VESSEL_INTERNAL_ERR;
         goto error;
      }

      if (head->clLogcalID != context->getCLLid())
      {
         PD_LOG(PDERROR, "logical id[%d] in context does not match the one[%d] in page",
                context->getCLLid(), head->clLogcalID);
         rc = SDB_VESSEL_PAGE_HEAD_NOT_MATCH;
         goto error;
      }

      if (context->getCandidate().isValid() &&
          head->pageSeq == context->getCandidate().getSeq())
      {
         if (INVALID_PAGE_ID == context->getCandidate().getLpid())
         {
            context->getCandidate().setLpid(lpb->getLogicalPid());
         }
         oldLvl = getFsmSpaceLvl(lpb->getRuntimeBuffer().getPageSize(),
                                 head->freeSpaceAfterLastSlot);
         if (oldLvl != context->getCandidate().getSpaceLvl())
         {
            context->getCandidate().getInfoPtr()->_lvl = oldLvl;
         }
      }

      alignedSize = getAlignedSizeOfNormalRecordAndHead(record.len());

      rc = getPosToInsert(head, alignedSize, slotId, offset, totalSize);
      if (SDB_OK != rc)
      {
         PD_LOG(PDDEBUG, "failed to get position to insert:%d", rc);
         goto error;
      }

      newFreeSize = head->freeSpaceAfterLastSlot - totalSize;
      rs.setType(RDP_SLOT_TYPE_NORMAL);
      rs.setOffset(offset);

      rh.setSize(alignedSize);
      rh.setCompressionType(context->getCompressionType());
      rh.setTransInfo(context->getTransID().getNodeID(),
                      context->getTransID().getSN());

      rc = insertWithNormalHead(context, slotId,
                                rs, rh, lpb);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to insert with normal record head:%d", rc);
         goto error;
      }

      /// do not access old page ptr any more.
      head = NULL;
      rid.setPageID(lpb->getLogicalPid());
      rid.setSlotID(slotId);
      context->setRid(rid);

      if (context->getCandidate().isValid() &&
          head->pageSeq == context->getCandidate().getSeq())
      {
         if (newFreeSize < context->getMinFreeSize())
         {
            context->getCandidate().getInfoPtr()->_lvl = FSM_INVALID_SPACE_LVL;
         }
         else
         {
            newLvl = getFsmSpaceLvl(lpb->getRuntimeBuffer().getPageSize(),
                                    newFreeSize);
            if (newLvl != oldLvl)
            {
               context->getCandidate().getInfoPtr()->_lvl = newLvl;
            }
         }
      }
      
   done:
      return rc;
   error:
      goto done;
   }

   INT32 rdpInsertExecutor::insertWithNormalHead(insertContext *context,
                                                 RECORD_SLOT_ID slotId,
                                                 const recordSlot &slot,
                                                 const recordHead &rh,
                                                 logicalPageBuffer *lpb)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(NULL != context, "can not be null");
      SDB_ASSERT(INVALID_RECORD_SLOT_ID != slotId, "can not be invalid");
      SDB_ASSERT(slot.isValid(), "must be valid");
      SDB_ASSERT(0 != rh.getSize(), "can not be zero");
      runtimePageBuffer *rpb = NULL;
      recordDataPageHead oldHead;
      recordDataPageHead *head = NULL;
      recordSlot *slotPtr = NULL;
      ossValuePtr recordPtr = 0;
      slice record = context->getRecordToInsert();
      SDB_ASSERT(0 < record.len(), "can not be empty");
      logRecordContext lrc;
      recordID rid;

      rc = lpb->prepareToWrite(context);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to prepare to write:%d", rc);
         goto error;
      }

      rpb = &(lpb->getRuntimeBuffer());

      rc = prepareInsertLog(context, rh.getSize(), rpb, &lrc);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to prepare log:%d", rc);
         goto error;
      }

      head = rpb->getWritablePtrOfBody<recordDataPageHead>(0);
      if (NULL == head)
      {
         PD_LOG(PDERROR, "failed to get writable page head");
         rc = SDB_VESSEL_INTERNAL_ERR;
         goto error;
      }

      slotPtr = rpb->getWritablePtrOfBody<recordSlot>(RECORD_PAGE_HEAD_LEN +
                                                      (slotId * RDP_RSLOT_SIZE));
      if (NULL == slotPtr)
      {
         PD_LOG(PDERROR, "failed to get writable slot ptr");
         rc = SDB_VESSEL_INTERNAL_ERR;
         goto error;
      }

      rc = rpb->getWritablePtrOfBodyWithRc(slot.getOffset(), rh.getSize(), recordPtr);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to get writable record ptr:%d", rc);
         goto error;
      }

      oldHead = *head;
      *slotPtr = slot;
      ossMemcpy((void *)recordPtr, &rh, RDP_RECORD_HEAD_LEN);
      ossMemcpy((void *)(recordPtr + RDP_RECORD_HEAD_LEN),
                record.data(), record.len());
      if ((RDP_RECORD_HEAD_LEN + record.len()) < rh.getSize())
      {
         ossMemset((void *)(recordPtr + RDP_RECORD_HEAD_LEN + record.len()),
                   0, (rh.getSize() - RDP_RECORD_HEAD_LEN - record.len()));
      }
      updatePageHead(head, slotId, slot, rh, context->getStriping());

      rid.setPageID(lpb->getLogicalPid());
      rid.setSlotID(slotId);
      rc = commitInsertLog(context, rid, slot,
                           (const recordHead *)recordPtr,
                           &oldHead, head, rpb, &lrc);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to commit log:%d", rc);
         *head = oldHead;
         *slotPtr = recordSlot();
         ossMemset((void *)recordPtr, 0, rh.getSize());
         rpb->abort();
         goto error;
      }

      rpb->commit(lrc.getLsn());

   done:
      return rc;
   error:
      if (lrc.prepared())
      {
         pageAccessor::abortLog(context, &lrc);
      }
      goto done;
   }

   INT32 rdpInsertExecutor::getPosToInsert(const recordDataPageHead *head,
                                           UINT32 alignedHeadAndBodySize,
                                           RECORD_SLOT_ID &slotId,
                                           UINT16 &offset,
                                           UINT32 &totalSize)const
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(NULL != head, "can not be null");
      SDB_ASSERT(0 < alignedHeadAndBodySize, "can not be zero");

      UINT32 currentSlotCount = head->totalSlotCount;
      UINT32 high = 0;
      totalSize = alignedHeadAndBodySize;
      if (INVALID_RECORD_SLOT_ID == head->firstFreeSlot)
      {
         totalSize += RDP_RSLOT_SIZE;
         slotId = head->totalSlotCount;
      }
      else
      {
         slotId = head->firstFreeSlot;
      }

      if (head->freeSpaceAfterLastSlot < totalSize)
      {
         PD_LOG(PDDEBUG, "avalible free space[%d] not enough for size[%d]",
                head->freeSpaceAfterLastSlot, totalSize);
         rc = SDB_VESSEL_NOT_ENOUGH_SPACE_IN_PAGE;
         goto error;
      }

      high = (currentSlotCount * RDP_RSLOT_SIZE) + head->freeSpaceAfterLastSlot;
      offset = high - alignedHeadAndBodySize;
   done:
      return rc;
   error:
      slotId = INVALID_RECORD_SLOT_ID;
      offset = 0;
      goto done;
   }

   void rdpInsertExecutor::updatePageHead(recordDataPageHead *head,
                                          RECORD_SLOT_ID slotId,
                                          const recordSlot &slot,
                                          const recordHead &rh,
                                          STRIPING_ID striping)
   {
      SDB_ASSERT(NULL != head, "can not be null");
      SDB_ASSERT(INVALID_RECORD_SLOT_ID != slotId, "can not be invalid");
      SDB_ASSERT(slot.isValid(), "must be valid");
      SDB_ASSERT(0 != rh.getSize(), "can not be zero");
      UINT32 size = rh.getSize();

      if (head->totalSlotCount == slotId)
      {
         ++head->totalSlotCount;
         size += RDP_RSLOT_SIZE;
      }
      else if (slotId == head->firstFreeSlot)
      {
         head->firstFreeSlot = INVALID_RECORD_SLOT_ID;
         for (UINT32 i = slotId + 1; i < head->totalSlotCount; ++i)
         {
            const recordSlot *tmp = (const recordSlot *)
                                     ((ossValuePtr)head + RECORD_PAGE_HEAD_LEN +
                                     (i * RDP_RSLOT_SIZE));
            if (tmp->isValid())
            {
               continue;
            }
            head->firstFreeSlot = i;
            break;
         }
      }
      else
      {
         SDB_ASSERT(FALSE, "impossible");
      }

      SDB_ASSERT(size <= head->totalFreeSpace, "impossible");
      SDB_ASSERT(size <= head->freeSpaceAfterLastSlot, "impossible");
      head->totalFreeSpace -= size;
      head->freeSpaceAfterLastSlot -= size;

      if (!slot.isInvisible())
      {
         ++head->recordCount;
      }

      if (INVALID_STRIPING_ID != striping)
      {
         if (INVALID_STRIPING_ID == head->minStriping)
         {
            head->minStriping = striping;
         }
         else if (striping < head->minStriping)
         {
            head->minStriping = striping;
         }

         if (INVALID_STRIPING_ID == head->maxStriping)
         {
            head->maxStriping = striping;
         }
         else if (head->maxStriping < striping)
         {
            head->maxStriping = striping;
         }
      }

      if (DPS_INVALID_TRANSID_SN != rh.getTransSN())
      {
         if (DPS_INVALID_TRANSID_SN == head->transSN)
         {
            head->transSN = rh.getTransSN();
         }
         else if (head->transSN < rh.getTransSN())
         {
            head->transSN = rh.getTransSN();
         }
      }
   }

   INT32 rdpInsertExecutor::prepareInsertLog(insertContext *context,
                                             UINT32 recordHeadAndBodySize,
                                             const runtimePageBuffer *rpb,
                                             logRecordContext *lrc)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(0 < recordHeadAndBodySize, "can not be zero");
      UINT32 fullNameSize = 0;
      rc = pageAccessor::prepareLog(context, rpb,
                                    LOG_TYPE_VESSEL_RDP_INSERT,
                                    FALSE, lrc);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to prepare dps log:%d", rc);
         goto error;
      }

      SDB_ASSERT(!context->getCSName().empty(), "can not be empty");
      SDB_ASSERT(!context->getCLName().empty(), "can not be empty");
      fullNameSize = context->getCSName().strLen() +
                     context->getCLName().strLen() + 2;

      lrc->prepush(fullNameSize);
      if (context->getTransID().isValid())
      {
         lrc->prepush(sizeof(DPS_TRANS_ID));
      }
      if (0 < context->getUniqueKeyCount())
      {
         lrc->prepush(context->getUniqueKeyCount() << 1);
      }
      lrc->prepush(sizeof(GLOBAL_PAGE_ID));
      lrc->prepush(sizeof(recordID));
      lrc->prepush(RECORD_PAGE_HEAD_LEN);
      lrc->prepush(RECORD_PAGE_HEAD_LEN);
      lrc->prepush(RDP_RSLOT_SIZE);
      lrc->prepush(recordHeadAndBodySize);
      if (context->isCompressed())
      {
         lrc->prepush(context->getOriginalRecord().len());
      }
      if (INVALID_STRIPING_ID != context->getStriping())
      { 
         lrc->prepush(sizeof(UINT16));
      }

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

   INT32 rdpInsertExecutor::commitInsertLog(insertContext *context,
                                            const recordID &rid,
                                            const recordSlot &slot,
                                            const recordHead *record,
                                            const recordDataPageHead *oldHead,
                                            const recordDataPageHead *newHead,
                                            const runtimePageBuffer *rpb,
                                            logRecordContext *lrc)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(rid.valid(), "must be valid");
      SDB_ASSERT(slot.isValid(), "must be valid");
      SDB_ASSERT(NULL != record, "can not be null");
      SDB_ASSERT(NULL != oldHead, "can not be null");
      SDB_ASSERT(NULL != newHead, "can not be null");
      SDB_ASSERT(NULL != lrc, "can not be null");
      SDB_ASSERT(!lrc->prepared(), "can not be prepared");
      
      ossPoolString fullName;
      SDB_ASSERT(!context->getCSName().empty(), "can not be empty");
      SDB_ASSERT(!context->getCLName().empty(), "can not be empty");
      fullName.reserve(context->getCSName().strLen() + 
                       context->getCLName().strLen() + 2);
      fullName.append(context->getCSName().str());
      fullName.append(".");
      fullName.append(context->getCLName().str());

      rc = pageAccessor::pushElement(context, DPS_LOG_PUBLIC_FULLNAME,
                                     fullName.size() + 1,
                                     fullName.c_str(), lrc);
      if (SDB_OK != rc)
      {
         goto error;
      }

      if (context->getTransID().isValid())
      {
         rc = pageAccessor::pushElement(context, DPS_LOG_PUBLIC_TRANSID,
                                        sizeof(DPS_TRANS_ID),
                                        &(context->getTransID()), lrc);
         if (SDB_OK != rc)
         {
            goto error;
         }
      }
      if (0 < context->getUniqueKeyCount())
      {
         rc = pageAccessor::pushElement(context, DPS_LOG_PUBLIC_NEW_UNQIDX_HASH,
                                        (context->getUniqueKeyCount() << 1),
                                        context->getUniqueKeys(), lrc);
         if (SDB_OK != rc)
         {
            goto error;
         }
      }

      rc = pageAccessor::pushElement(context, DPS_LOG_PUBLIC_VESSEL_GPID,
                                     sizeof(GLOBAL_PAGE_ID),
                                     &(rpb->getGlobalPid()), lrc);
      if (SDB_OK != rc)
      {
         goto error;
      }

      rc = pageAccessor::pushElement(context, DPS_LOG_VESSEL_RDP_INSERT_RID,
                                     sizeof(recordID),
                                     &rid, lrc);
      if (SDB_OK != rc)
      {
         goto error;
      }

      rc = pageAccessor::pushElement(context, DPS_LOG_VESSEL_RDP_INSERT_PAGE_HEAD,
                                     RECORD_PAGE_HEAD_LEN,
                                     newHead, lrc);
      if (SDB_OK != rc)
      {
         goto error;
      }

      rc = pageAccessor::pushElement(context, DPS_LOG_VESSEL_RDP_INSERT_OLD_PAGE_HEAD,
                                     RECORD_PAGE_HEAD_LEN,
                                     oldHead, lrc);
      if (SDB_OK != rc)
      {
         goto error;
      }

      rc = pageAccessor::pushElement(context, DPS_LOG_VESSEL_RDP_INSERT_SLOT,
                                     RDP_RSLOT_SIZE,
                                     &slot, lrc);
      if (SDB_OK != rc)
      {
         goto error;
      }

      rc = pageAccessor::pushElement(context, DPS_LOG_VESSEL_RDP_INSERT_RECORD_AND_HEAD,
                                     record->getSize(),
                                     record, lrc);
      if (SDB_OK != rc)
      {
         goto error;
      }

      if (context->isCompressed())
      {
         rc = pageAccessor::pushElement(context, DPS_LOG_VESSEL_RDP_INSERT_UNCOMPRESSED_RECORD,
                                        context->getOriginalRecord().len(),
                                        context->getOriginalRecord().data(), lrc);
         if (SDB_OK != rc)
         {
            goto error;
         }
      }

      if (INVALID_STRIPING_ID != context->getStriping())
      {
         UINT16 striping = context->getStriping();
         rc = pageAccessor::pushElement(context,
                                        DPS_LOG_VESESL_RDP_INSERT_STRIPING,
                                        sizeof(UINT16),
                                        &striping, lrc);
         if (SDB_OK != rc)
         {
            goto error;
         }
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