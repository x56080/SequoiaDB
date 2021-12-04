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

   Source File Name = rdpAccessor.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#include "vessel/rdpAccessor.h"
#include "vessel/insertContext.h"
#include "vessel/logicalPageBuffer.h"
#include "vessel/logRecordContext.h"
#include "dpsLogRecordDef.hpp"
#include "vessel/fsmCandidate.h"
#include "vessel/redoLogUtil.h"
#include "vessel/requestContext.h"
#include "vessel/modifyRecordContext.h"

namespace engine
{
namespace vessel
{
   INT32 rdpAccessor::init(requestContext *context,
                           logicalPageBuffer *lpb)
   {
      INT32 rc = SDB_OK;

      _lpb = NULL;
      if (OSS_UNLIKELY(NULL == context ||
                       !context->isMbInfoCached() ||
                       NULL == lpb ||
                       !lpb->isValid()))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      rc = validatePage(context, lpb);
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

   INT32 rdpAccessor::insertNormalRecord(insertContext *context)
   {
      INT32 rc = SDB_OK;
      slice record;
      const recordDataPageHead *head = NULL;
      RECORD_SLOT_ID pos = INVALID_RECORD_SLOT_ID;
      UINT16 offset = 0;
      recordID rid;
      BOOLEAN ridLocked = FALSE;

      if (OSS_UNLIKELY(NULL == context))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      else if (OSS_UNLIKELY(NULL == _lpb))
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }
      else if (!_lpb->getLockingMode().isExclusive() &&
               !_lpb->getLockingMode().isUpgrade())
      {
         rc = SDB_VESSEL_FORBIDDEN_OP_WLT;
         goto error;
      }

      record = context->getOriginalRecord();
      if (!record.isValid())
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      else if (isBigRecord(_lpb->getPageSize(),
                           record.getSize()))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      head = _lpb->getReadableBodyBuffer().getReadableObjPtr<recordDataPageHead>(0);
      if (NULL == head)
      {
         PD_LOG(PDERROR, "failed to get readble record page head");
         rc = SDB_VESSEL_INTERNAL_ERR;
         goto error;
      }

      SDB_ASSERT(context->getLogicalCLID() == head->clLogcalID, "must be same");

      if (context->getCandidate().isValid())
      {
         if (context->getCandidate().getSeq() != head->pageSeq)
         {
            PD_LOG(PDERROR, "candidate seq[%d] in context does not match the one[%d] in page",
                context->getCandidate().getSeq(), head->pageSeq);
            rc = SDB_VESSEL_PAGE_HEAD_NOT_MATCH;
            goto error;
         }

         if (INVALID_PAGE_ID == context->getCandidate().getLpid())
         {
            context->getCandidate().setLpid(_lpb->getLogicalPid());
         }
      }

      if (!findPositionToInsert(record.getSize(),
                                context->getMinFreePercent(),
                                pos, offset))
      {
         if (context->getCandidate().isValid())
         {
            context->getCandidate().getInfoPtr()->_lvl = FSM_INVALID_SPACE_LVL;
         }

         rc = SDB_VESSEL_NOT_ENOUGH_SPACE_IN_PAGE;
         goto error;
      }

      rid.setPageID(_lpb->getLogicalPid());
      rid.setSlotID(pos);

      if (context->keepRidLocked())
      {
         ossSharedLatchMode mode(OSS_SHARED_LATCH_MODE_ENUM_EXCLUSIVE);
         rc = context->tryLockRid(rid, mode, ridLocked);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to lock rid[%d,%d], rc:%d",
                     rid.getPageID(), rid.getSlotID(), rc);
            goto error;
         }
         else if (!ridLocked)
         {
            PD_LOG(PDERROR, "failed to lock free rid[%d,%d]",
                     rid.getPageID(), rid.getSlotID());
            rc = SDB_VESSEL_INTERNAL_ERR;
            goto error;
         }
      }
      
      /// do not access old page ptr any more.
      head = NULL;

      rc = insertNormalRecordToPos(context, pos, offset);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to insert with normal record head:%d", rc);
         goto error;
      }

      if (context->getCandidate().isValid())
      {
         head = getReadablePageHead();
         INT32 newLvl = getFsmSpaceLvl(_lpb->getPageSize(),
                                       head->backOffset - getFrontOffset(head));

         if (newLvl != context->getCandidate().getInfoPtr()->_lvl)
         {
            context->getCandidate().getInfoPtr()->_lvl = newLvl;
         }
      }
      
   done:
      return rc;
   error:
      if (ridLocked)
      {
         context->unlockRid(rid);
      }
      goto done;
   }

   INT32 rdpAccessor::insertNormalRecordToPos(insertContext *context,
                                              RECORD_SLOT_ID pos,
                                              UINT16 offset)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(NULL != _lpb, "can not be invlaid");
      SDB_ASSERT(NULL != context, "can not be null");
      SDB_ASSERT(INVALID_RECORD_SLOT_ID != pos, "can not be invalid");
      SDB_ASSERT(0 < offset, "can not be invalid");

      recordDataPageHead oldHead;
      recordDataPageHead *head = NULL;
      recordSlot *slotPtr = NULL;

      logRecordContext lrc;
      recordID rid;
      strictBuffer buffer;
      CHAR *recordPtr = NULL;
      recordHead rh;
      recordSlot rs;
      DPS_TRANS_ID transID = context->getTransIDWithoutTag();
      slice record = context->getOriginalRecord();
      UINT32 size = record.getSize() + RDP_RECORD_HEAD_SIZE;
      UINT32 reserved = 0;

      rc = _lpb->prepareToWrite();
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to prepare to write:%d", rc);
         goto error;
      }

      buffer = _lpb->getWritableBodyBuffer();
      head = buffer.getWritableObjPtr<recordDataPageHead>(0);
      SDB_ASSERT((size + offset) <= head->backOffset, "invalid offset");
      SDB_ASSERT((head->backOffset - offset - size) <= 0xFF, "invalid reserved size");
      reserved = head->backOffset - offset - size;
      rh.format.normal.transNode = transID.getNodeID();
      rh.format.normal.transSN = transID.getSN();
      rh.format.normal.compressionType = UTIL_COMPRESSOR_INVALID;
      rs.init(RDP_RECORD_HEAD_TYPE_NORMAL, reserved, offset, size);

      rc = prepareInsertLog(context, size, &(_lpb->getRuntimeBuffer()), &lrc);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to prepare log:%d", rc);
         goto error;
      }

      slotPtr = buffer.getWritableObjPtr<recordSlot>(RECORD_PAGE_HEAD_SIZE +
                                                     (pos * RDP_RSLOT_SIZE));
      if (NULL == slotPtr)
      {
         PD_LOG(PDERROR, "failed to get writable slot ptr[%d]", pos);
         rc = SDB_VESSEL_INTERNAL_ERR;
         goto error;
      }

      recordPtr = buffer.getWritablePtr(offset, head->backOffset - offset);
      if (NULL == recordPtr)
      {
         PD_LOG(PDERROR, "failed to get writable record ptr[%d,%d]",
                offset, head->backOffset - offset);
         rc = SDB_VESSEL_INTERNAL_ERR;
         goto error;
      }

      oldHead = *head;
      *slotPtr = rs;
      *((recordHead *)recordPtr) = rh;
      ossMemcpy(recordPtr + RDP_RECORD_HEAD_SIZE,
                record.getData(), record.getSize());
      if (0 < rs.reserved)
      {
         ossMemset((void *)(recordPtr + RDP_RECORD_HEAD_SIZE + record.getSize()),
                   0, rs.reserved);
      }
      updatePageHeadWhenInsert(pos, rs, rh, context->getRecordStripingId());

      rid.setPageID(_lpb->getLogicalPid());
      rid.setSlotID(pos);
      rc = commitInsertLog(context, rid, rs,
                           recordPtr,&oldHead, head,
                           &(_lpb->getRuntimeBuffer()), &lrc);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to commit log:%d", rc);
         *head = oldHead;
         *slotPtr = recordSlot();
         ossMemset((void *)recordPtr, 0, rs.size + rs.reserved);
         goto error;
      }

      context->setDmlLSN(lrc.getLsn());
      context->setRid(rid);
      context->setPageSeq(head->pageSeq);
      _lpb->commit(lrc.getLsn());

   done:
      return rc;
   error:
      if (lrc.prepared())
      {
         pageAccessor::abortLog(context, &lrc);
      }
      goto done;
   }

   BOOLEAN rdpAccessor::findPositionToInsert(UINT32 recordSize,
                                             FLOAT32 minFreePercent,
                                             RECORD_SLOT_ID &pos,
                                             UINT16 &offset)const
   {
      BOOLEAN r = FALSE;
      SDB_ASSERT(NULL != _lpb && _lpb->isValid(), "can not be invalid");
      SDB_ASSERT(0.0 <= minFreePercent && minFreePercent <= 1.0, "can not be invalid");
      const recordDataPageHead *head = getReadablePageHead();

      pos = INVALID_RECORD_SLOT_ID;
      offset = 0;

      UINT32 size = recordSize + RDP_RECORD_HEAD_SIZE;
      INT32 frontOffset = (INT32)(getFrontOffset(head));
      if (INVALID_RECORD_SLOT_ID == head->firstFreeSlot)
      {
         frontOffset += RDP_RSLOT_SIZE;
      }
      INT32 backOffset = (INT32)(head->backOffset);
      SDB_ASSERT(ossIsAligned4(frontOffset), "impossible");
      SDB_ASSERT(ossIsAligned4(backOffset), "impossible");
      INT32 freeSize = backOffset - frontOffset;
      INT32 reservedSize = 0;
      INT32 totalSize = 0;
      
      FLOAT32 totalFreePercent = 0.0;

      if (freeSize < (INT32)(ossAlign4(size)))
      {
         goto done;
      }

      totalFreePercent = (FLOAT32)(head->totalFreeSpace - size) /
                         getPageBodySize(_lpb->getPageSize());
      if (totalFreePercent < minFreePercent)
      {
         goto done;
      }

      reservedSize = recordSize * minFreePercent;
      reservedSize = recordSlot::trimReservedSize(reservedSize);
      totalSize = (INT32)ossAlign4(size + (UINT32)reservedSize);
      if (freeSize < totalSize)
      {
         totalSize = freeSize;
      }

      pos = (INVALID_RECORD_SLOT_ID == head->firstFreeSlot) ?
            head->totalSlotCount : head->firstFreeSlot;
      SDB_ASSERT(totalSize < backOffset, "impossible");
      offset = (UINT16)(backOffset - totalSize);
      SDB_ASSERT(0 == (offset & 0x03), "must be aligned");
      r = TRUE;

   done:
      return r;
   }

   void rdpAccessor::updatePageHeadWhenInsert(RECORD_SLOT_ID pos,
                                              const recordSlot &slot,
                                              const recordHead &rh,
                                              STRIPING_ID striping)
   {
      SDB_ASSERT(NULL != _lpb && _lpb->isWritable(), "can not be null");
      SDB_ASSERT(INVALID_RECORD_SLOT_ID != pos, "can not be invalid");
      SDB_ASSERT(slot.isValid(), "must be valid");
      SDB_ASSERT(!slot.isOverflow(), "can not be invalid");
      SDB_ASSERT(!slot.isTombstone(), "can not be invalid");
      SDB_ASSERT(RDP_RECORD_HEAD_TYPE_NORMAL == slot.type, "must be normal");

      strictBuffer buffer = _lpb->getWritableBodyBuffer();
      recordDataPageHead *head = buffer.getWritableObjPtr<recordDataPageHead>(0);
      UINT32 size = slot.size;

      if (head->totalSlotCount == pos)
      {
         ++head->totalSlotCount;
         size += RDP_RSLOT_SIZE;
      }
      else if (pos == head->firstFreeSlot)
      {
         head->firstFreeSlot = INVALID_RECORD_SLOT_ID;
         for (UINT32 i = pos + 1; i < head->totalSlotCount; ++i)
         {
            const recordSlot *tmp = buffer.getReadableObjPtr<recordSlot>
                                    (RDP_RECORD_HEAD_SIZE + (i * RDP_RSLOT_SIZE));
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
      head->totalFreeSpace -= size;
      head->backOffset = slot.offset;

      if (!slot.isInvisible())
      {
         ++head->recordCount;
      }

      updateStripingInfo(head, striping);

      updateMaxTransSN(head, rh.format.normal.transSN);
   }

   void rdpAccessor::updateMaxTransSN(recordDataPageHead *head,
                                      UINT64 transSN)
   {
      SDB_ASSERT(NULL != head, "can not be null");
      if (DPS_INVALID_TRANSID_SN != transSN)
      {
         if (DPS_INVALID_TRANSID_SN == head->transSN ||
             head->transSN < transSN)
         {
            head->transSN = transSN;
         }
      }
   }

   void rdpAccessor::updateStripingInfo(recordDataPageHead *head,
                                        STRIPING_ID striping)
   {
      SDB_ASSERT(NULL != head, "can not be null");

      if (INVALID_STRIPING_ID != striping)
      {
         if (INVALID_STRIPING_ID == head->minStriping ||
             striping < head->minStriping)
         {
            head->minStriping = striping;
         }

         if (INVALID_STRIPING_ID == head->maxStriping ||
             striping > head->maxStriping)
         {
            head->maxStriping = striping;
         } 
      }
   }

   INT32 rdpAccessor::prepareInsertLog(insertContext *context,
                                       UINT32 recordHeadAndBodySize,
                                       const runtimePageBuffer *rpb,
                                       logRecordContext *lrc)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(0 < recordHeadAndBodySize, "can not be zero");
      DPS_TRANS_ID transID = context->getTransIDWithoutTag();
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
      if (transID.isValid())
      {
         lrc->prepush(sizeof(DPS_TRANS_ID));
      }
      if (0 < context->getUniqueKeyHashSize())
      {
         lrc->prepush(context->getUniqueKeyHashSize() << 2);
      }
      lrc->prepush(sizeof(GLOBAL_PAGE_ID));
      lrc->prepush(sizeof(recordID));
      lrc->prepush(RECORD_PAGE_HEAD_SIZE);
      lrc->prepush(RECORD_PAGE_HEAD_SIZE);
      lrc->prepush(RDP_RSLOT_SIZE);
      lrc->prepush(recordHeadAndBodySize);
      if (INVALID_STRIPING_ID != context->getRecordStripingId())
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

   INT32 rdpAccessor::prepareInplaceUpdateLog(modifyRecordContext *context,
                                              const runtimePageBuffer *rpb,
                                              logRecordContext *lrc)
   {
      INT32 rc = SDB_OK;
      UINT32 fullNameSize = 0;
      rc = pageAccessor::prepareLog(context, rpb,
                                    LOG_TYPE_DATA_UPDATE,
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
      if (0 < context->getUniqueKeyHashSize())
      {
         lrc->prepush(context->getUniqueKeyHashSize() << 2);
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

   INT32 rdpAccessor::prepareDeleteLog(modifyRecordContext *context,
                                       const runtimePageBuffer *rpb,
                                       logRecordContext *lrc)
   {
      INT32 rc = SDB_OK;
      UINT32 fullNameSize = 0;
      rc = pageAccessor::prepareLog(context, rpb,
                                    LOG_TYPE_DATA_DELETE,
                                    FALSE, lrc);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to prepare dps log:%d", rc);
         goto error;
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

   INT32 rdpAccessor::commitInsertLog(insertContext *context,
                                            const recordID &rid,
                                            const recordSlot &slot,
                                            const void *record,
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
      SDB_ASSERT(lrc->prepared(), "must be prepared");
      
      SDB_ASSERT(!context->getCSName().empty(), "can not be empty");
      SDB_ASSERT(!context->getCLName().empty(), "can not be empty");
      DPS_TRANS_ID transID = context->getTransIDWithoutTag();
      
      ossPoolString fullName;
      fullName.reserve(context->getCSName().strLen() + 
                       context->getCLName().strLen() + 1);
      fullName.append(context->getCSName().str()).
               append(".").
               append(context->getCLName().str());

      rc = pageAccessor::pushElement(context, DPS_LOG_PUBLIC_FULLNAME,
                                     fullName.size(),
                                     fullName.c_str(), lrc);
      if (SDB_OK != rc)
      {
         goto error;
      }

      if (transID.isValid())
      {
         rc = pageAccessor::pushElement(context, DPS_LOG_PUBLIC_TRANSID,
                                        sizeof(DPS_TRANS_ID),
                                        &transID, lrc);
         if (SDB_OK != rc)
         {
            goto error;
         }
      }
      if (0 < context->getUniqueKeyHashSize())
      {
         const UINT32 *hash = context->getUniqueKeyHashes();
         SDB_ASSERT(NULL != hash, "impossible");
         rc = pageAccessor::pushElement(context, DPS_LOG_PUBLIC_NEW_UNQIDX_HASH,
                                        (context->getUniqueKeyHashSize() << 2),
                                        hash, lrc);
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
                                     RECORD_PAGE_HEAD_SIZE,
                                     newHead, lrc);
      if (SDB_OK != rc)
      {
         goto error;
      }

      rc = pageAccessor::pushElement(context, DPS_LOG_VESSEL_RDP_INSERT_OLD_PAGE_HEAD,
                                     RECORD_PAGE_HEAD_SIZE,
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
                                     slot.size,
                                     record, lrc);
      if (SDB_OK != rc)
      {
         goto error;
      }

      if (INVALID_STRIPING_ID != context->getRecordStripingId())
      {
         UINT16 striping = context->getRecordStripingId();
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

   INT32 rdpAccessor::updateNormalRecord(modifyRecordContext *context,
                                         const slice &newRowData,
                                         BOOLEAN &outOfSpace)
   {
      INT32 rc = SDB_OK;
      const recordDataPageHead *head = NULL;
      const recordSlot *rs = NULL;
      recordID rid;

      outOfSpace = FALSE;
      if (OSS_UNLIKELY(NULL == context ||
                       !context->isMbInfoCached() ||
                       !newRowData.isValid()))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      else if (OSS_UNLIKELY(NULL == _lpb))
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }
      else if (!_lpb->getLockingMode().isExclusiveOrUpgrade())
      {
         rc = SDB_VESSEL_FORBIDDEN_OP_WLT;
         goto error;
      }

      rid = context->getRid();
      if (!rid.isValid())
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      else if (isBigRecord(_lpb->getPageSize(), newRowData.getSize()))
      {
         PD_LOG(PDERROR, "new row size ouf of size:%d", newRowData.getSize());
         rc = SDB_INVALIDARG;
         goto error;
      }

      SDB_ASSERT(rid.getPageID() == _lpb->getLogicalPid(), "must be same");

      head = getReadablePageHead();
      if (head->totalSlotCount <= rid.getSlotID())
      {
         PD_LOG(PDERROR, "pos[%d] out of total slot count[%d]",
                rid.getSlotID(), head->totalSlotCount);
         rc = SDB_OUT_OF_BOUND;
         goto error;
      }

      rs = getReadableSlot(rid.getSlotID());
      if (OSS_UNLIKELY(NULL == rs))
      {
         PD_LOG(PDERROR, "failed to get slot ptr[%d]", rid.getSlotID());
         rc = SDB_VESSEL_INTERNAL_ERR;
         goto error;
      }

      if (!rs->isValid() || rs->isTombstone())
      {
         PD_LOG(PDERROR, "pos[%d] not valid to be updated", rid.getSlotID());
         rc = SDB_VESSEL_RECORD_NOT_FOUND;
         goto error;
      }
      else if (rs->isOverflow() || !rs->isNormalRecordHead())
      {
         PD_LOG(PDERROR, "record at[%d] can not be update inplace", rid.getSlotID());
         rc = SDB_VESSEL_OPERATOION_NOT_PERMITTED;
         goto error;
      }

      if ((newRowData.getSize() + RDP_RECORD_HEAD_SIZE) <= (rs->size + rs->reserved))
      {
         rc = inplaceUpdate(context, newRowData);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to inplace update record[%d], rc:%d", rid.getSlotID(), rc);
            goto error;
         }
      }
      else
      {
         SDB_ASSERT(FALSE, "TODO");
      }
   done:
      return rc;
   error:
      goto done;
   }

   INT32 rdpAccessor::inplaceUpdate(modifyRecordContext *context,
                                    const slice &row)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(NULL != context, "can not be null");
      SDB_ASSERT(row.isValid(), "can not be invalid");

      recordID rid = context->getRid();
      SDB_ASSERT(rid.isValid(), "can not be invalid");
      RECORD_SLOT_ID pos = rid.getSlotID();

      strictBuffer buffer;
      recordSlot *rs = NULL;
      recordHead *rh = NULL;
      strictBuffer recordBuffer;
      recordDataPageHead *head = NULL;
      logRecordContext lrc;
      DPS_TRANS_ID transID = context->getTransIDWithoutTag();
      UINT32 totalSize = row.getSize() + RDP_RECORD_HEAD_SIZE;

      rc = _lpb->autoGetWritableBodyBuffer(buffer);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to prepare to write:%d", rc);
         goto error;
      }

      head = buffer.getWritableObjPtr<recordDataPageHead>(0);
      if (head->totalSlotCount <= pos)
      {
         rc = SDB_OUT_OF_BOUND;
         goto error;
      }

      rs = getWritableSlot(buffer, pos);
      if (OSS_UNLIKELY(NULL == rs))
      {
         PD_LOG(PDERROR, "failed to get writable slot[%d]", pos);
         rc = SDB_VESSEL_INTERNAL_ERR;
         goto error;
      }

      SDB_ASSERT(rs->isValid() && rs->isNormalRecordHead(), "impossible");
      SDB_ASSERT(!rs->isOverflow() && !rs->isTombstone(), "impossible");

      if ((rs->size + rs->reserved) < totalSize)
      {
         PD_LOG(PDERROR, "record size[%d] out of valid range[%d]",
                row.getSize(), rs->size + rs->reserved);
         rc = SDB_VESSEL_OPERATOION_NOT_PERMITTED;
         goto error;
      }

      recordBuffer = buffer.getWritableBuffer(rs->size + rs->reserved, rs->offset);
      rh = recordBuffer.getWritableObjPtr<recordHead>(0);
      if (OSS_UNLIKELY(NULL == rh))
      {
         PD_LOG(PDERROR, "failed to get writable record header");
         rc = SDB_VESSEL_INTERNAL_ERR;
         goto error;
      }

      rh->format.normal.transSN = transID.getSN();
      rh->format.normal.transNode = transID.getNodeID();
      recordBuffer.write(RDP_RECORD_HEAD_SIZE, row.getSize(), row.getData());
      if (totalSize < (UINT32)(rs->size))
      {
         UINT32 delta = (UINT32)(rs->size) - totalSize;
         UINT32 reservedSize = rs->reserved;
         reservedSize += delta;
         rs->size -= delta;
         rs->reserved = recordSlot::trimReservedSize(reservedSize);
         head->totalFreeSpace += delta;
      }
      else if (totalSize > (UINT32)(rs->size))
      {
         UINT32 delta =  totalSize - (UINT32)(rs->size);
         rs->size += delta;
         rs->reserved -= delta;
         head->totalFreeSpace -= delta;
      }

      updateStripingInfo(head, context->getRecordStripingId());
      updateMaxTransSN(head, transID.getSN());

      ///dummy log
      rc = prepareInplaceUpdateLog(context, &(_lpb->getRuntimeBuffer()), &lrc);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to prepare redo log:%d", rc);
         goto error;
      }

      rc = pageAccessor::commitLog(context, &lrc);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to commit log[%lld], rc:%d", lrc.getLsn(), rc);
         goto error;
      }

      _lpb->commit(lrc.getLsn());
   done:
      return rc;
   error:
      if (lrc.prepared())
      {
         pageAccessor::abortLog(context, &lrc);
      }
      goto done;
   }

   INT32 rdpAccessor::validatePage(requestContext *context,
                                 logicalPageBuffer *lpb)const
   {
      SDB_ASSERT(NULL != context && context->isOpen(), "can not be invalid");
      SDB_ASSERT(NULL != lpb && lpb->isValid(), "can not be invalid");
      const recordDataPageHead *head = NULL;
      INT32 rc = lpb->validatePage(PAGE_TYPE_RECORD);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to validate page[%s], rc:%d",
                lpb->getGlobalPid().toString().c_str(), rc);
         goto error;
      }

      head = lpb->getReadableBodyBuffer().getReadableObjPtr<recordDataPageHead>(0);
      if (NULL == head)
      {
         PD_LOG(PDERROR, "failed to get readble record page head");
         rc = SDB_VESSEL_INTERNAL_ERR;
         goto error;
      }

      if (head->clLogcalID != context->getLogicalCLID())
      {
         PD_LOG(PDERROR, "logical id[%d] in context does not match the one[%d] in page",
                context->getLogicalCLID(), head->clLogcalID);
         rc = SDB_VESSEL_PAGE_HEAD_NOT_MATCH;
         goto error;
      }

      if (head->backOffset < (head->totalSlotCount * RDP_RSLOT_SIZE + RECORD_PAGE_HEAD_SIZE))
      {
         PD_LOG(PDERROR, "front offset over back offset");
         rc = SDB_VESSEL_PAGE_CRASHED;
         goto error;
      }

   done:
      return rc;
   error:
      goto done;
   }

   const recordSlot *rdpAccessor::getReadableSlot(RECORD_SLOT_ID pos)const
   {
      SDB_ASSERT(NULL != _lpb && _lpb->isValid(), "can not be invalid");
      SDB_ASSERT(INVALID_RECORD_SLOT_ID != pos, "can not be invalid");
      SDB_ASSERT(pos < getTotalSlotCount(), "out of bound");
      strictBuffer buffer = _lpb->getReadableBodyBuffer();
      return buffer.getReadableObjPtr<recordSlot>(RECORD_PAGE_HEAD_SIZE +
                                                  (RDP_RSLOT_SIZE * pos));
   }

   const recordDataPageHead *rdpAccessor::getReadablePageHead()const
   {
      SDB_ASSERT(NULL != _lpb && _lpb->isValid(), "can not be invalid");
      return _lpb->getReadableBodyBuffer().getReadableObjPtr<recordDataPageHead>(0);
   }

   UINT32 rdpAccessor::getFrontOffset(const recordDataPageHead *head)const
   {
      SDB_ASSERT(NULL != head, "can not be null");
      return RECORD_PAGE_HEAD_SIZE + (head->totalSlotCount * RDP_RSLOT_SIZE);
   }

   UINT32 rdpAccessor::getTotalSlotCount()const
   {
      return getReadablePageHead()->totalSlotCount;
   }

   INT32 rdpAccessor::getSlot(RECORD_SLOT_ID pos, recordSlot &rs)const
   {
      INT32 rc = SDB_OK;
      rs.reset();

      if (OSS_UNLIKELY(INVALID_RECORD_SLOT_ID == pos))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      else if (OSS_UNLIKELY(NULL == _lpb))
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }
      else if (OSS_UNLIKELY(getReadablePageHead()->totalSlotCount <= pos))
      {
         rc = SDB_OUT_OF_BOUND;
         goto error;
      }

      rs = *getReadableSlot(pos);
   done:
      return rc;
   error:
      goto done;
   }

   INT32 rdpAccessor::getRecord(RECORD_SLOT_ID pos,
                                recordHead &rh,
                                slice &data)const
   {
      INT32 rc = SDB_OK;
      recordSlot rs;
      strictBuffer buffer;
      const CHAR *ptr = NULL;

      rh.reset();
      data.reset();

      if (OSS_UNLIKELY(INVALID_RECORD_SLOT_ID == pos))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      else if (OSS_UNLIKELY(NULL == _lpb))
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }

      rc = getSlot(pos, rs);
      if (SDB_OK != rc)
      {
         goto error;
      }

      if (RDP_RECORD_HEAD_TYPE_NORMAL != rs.type)
      {
         PD_LOG(PDERROR, "slot type is not normal:%d", rs.type);
         rc = SDB_VESSEL_OPERATOION_NOT_PERMITTED;
         goto error;
      }
      else if (OSS_UNLIKELY(rs.size <= RDP_RECORD_HEAD_SIZE))
      {
         PD_LOG(PDERROR, "invalid size[%d] found in slot", rs.size);
         rc = SDB_VESSEL_INTERNAL_ERR;
         goto error;
      }

      buffer = _lpb->getReadableBodyBuffer();
      ptr = buffer.getReadablePtr(rs.offset, rs.size);
      if (OSS_UNLIKELY(NULL == ptr))
      {
         PD_LOG(PDERROR, "failed to get ptr[%d,%d]", rs.offset, rs.size);
         rc = SDB_VESSEL_INTERNAL_ERR;
         goto error;
      }

      rh = *((const recordHead *)ptr);
      if (!rs.isTombstone() && !rs.isOverflow())
      {
         SDB_ASSERT(RDP_RECORD_HEAD_SIZE < rs.size, "impossible");
         data.reset(rs.size - RDP_RECORD_HEAD_SIZE, ptr + RDP_RECORD_HEAD_SIZE);
      }
   done:
      return rc;
   error:
      goto done;
   }

   recordSlot *rdpAccessor::getWritableSlot(strictBuffer &buffer,
                                            RECORD_SLOT_ID pos)
   {
      SDB_ASSERT(buffer.isWritable(), "must be writable");
      SDB_ASSERT(INVALID_RECORD_SLOT_ID != pos, "can not be invalid");
      return buffer.getWritableObjPtr<recordSlot>(RECORD_PAGE_HEAD_SIZE +
                                                  (pos * RDP_RSLOT_SIZE));

   }

   INT32 rdpAccessor::deleteRecord(modifyRecordContext *context)
   {
      INT32 rc = SDB_OK;
      recordID rid;
      const recordSlot *rs = NULL;

      if (OSS_UNLIKELY(NULL == context ||
                       !context->isMbInfoCached() ||
                       !context->getRid().isValid()))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      else if (OSS_UNLIKELY(NULL == _lpb))
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }
      else if (!_lpb->getLockingMode().isExclusiveOrUpgrade())
      {
         rc = SDB_VESSEL_FORBIDDEN_OP_WLT;
         goto error;
      }

      rid = context->getRid();
      SDB_ASSERT(rid.getPageID() == _lpb->getLogicalPid(), "must be same");

      if (getTotalSlotCount() <= rid.getSlotID())
      {
         rc = SDB_OUT_OF_BOUND;
         goto error;
      }

      rs = getReadableSlot(rid.getSlotID());
      if (OSS_UNLIKELY(NULL == rs))
      {
         PD_LOG(PDERROR, "failed to get slot ptr[%d]", rid.getSlotID());
         rc = SDB_VESSEL_INTERNAL_ERR;
         goto error;
      }

      if (!rs->isValid() || rs->isTombstone() || !rs->isNormalRecordHead())
      {
         PD_LOG(PDERROR, "pos[%d] not valid to be delete", rid.getSlotID());
         rc = SDB_VESSEL_RECORD_NOT_FOUND;
         goto error;
      }

      rc = createTombstone(context);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to delete record[%d], rc:%d", rid.getSlotID());
         goto error;
      }
   done:
      return rc;
   error:
      goto done;
   }

   INT32 rdpAccessor::createTombstone(modifyRecordContext *context)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(NULL != context, "can not be null");
      SDB_ASSERT(NULL != _lpb, "can not be null");

      recordID rid = context->getRid();
      SDB_ASSERT(rid.isValid(), "can not be invalid");
      RECORD_SLOT_ID pos = rid.getSlotID();

      strictBuffer buffer, recordBuffer;
      recordSlot *rs = NULL;
      recordDataPageHead *head = NULL;
      recordHead *rh = NULL;
      logRecordContext lrc;
      DPS_TRANS_ID transID = context->getTransIDWithoutTag();
      UINT32 size = 0;
      
      rc = _lpb->autoGetWritableBodyBuffer(buffer);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to prepare to write:%d", rc);
         goto error;
      }

      head = buffer.getWritableObjPtr<recordDataPageHead>(0);
      if (head->totalSlotCount <= pos)
      {
         rc = SDB_OUT_OF_BOUND;
         goto error;
      }

      rs = getWritableSlot(buffer, pos);
      if (OSS_UNLIKELY(NULL == rs))
      {
         PD_LOG(PDERROR, "failed to get writable slot[%d]", pos);
         rc = SDB_VESSEL_INTERNAL_ERR;
         goto error;
      }

      SDB_ASSERT(rs->isValid() && rs->isNormalRecordHead(), "impossible");
      SDB_ASSERT(!rs->isTombstone(), "impossible");

      size = rs->size - RECORD_PAGE_HEAD_SIZE;

      recordBuffer = buffer.getWritableBuffer(rs->size, rs->offset);
      if (!recordBuffer.isWritable())
      {
         PD_LOG(PDERROR, "failed to get writable buffer[%d,%d]",
                rs->size, rs->offset);
         rc = SDB_VESSEL_INTERNAL_ERR;
         goto error;
      }

      rh = recordBuffer.getWritableObjPtr<recordHead>(0);
      if (OSS_UNLIKELY(NULL == rh))
      {
         PD_LOG(PDERROR, "failed to get writable record header");
         rc = SDB_VESSEL_INTERNAL_ERR;
         goto error;
      }

      rh->format.normal.transNode = transID.getNodeID();
      rh->format.normal.transSN = transID.getSN();
      rs->size = RDP_RECORD_HEAD_SIZE;
      rs->reserved = 0;
      rs->setTombstone();

      --head->recordCount;
      head->totalFreeSpace += size;
      updateMaxTransSN(head, transID.getSN());

      rc = prepareDeleteLog(context, &(_lpb->getRuntimeBuffer()), &lrc);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to prepare log:%d", rc);
         goto error;
      }

      rc = pageAccessor::commitLog(context, &lrc);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to commit log[%lld], rc:%d", lrc.getLsn(), rc);
         goto error;
      }

      _lpb->commit(lrc.getLsn());
   done:
      return rc;
   error:
      if (lrc.prepared())
      {
         pageAccessor::abortLog(context, &lrc);
      }
      goto done;
   }
}//namespace vessel
}//namespace engine