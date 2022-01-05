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
#include "vessel/logicalPageBuffer.h"
#include "vessel/logRecordContext.h"
#include "dpsLogRecordDef.hpp"
#include "vessel/fsmCandidate.h"
#include "vessel/redoLogUtil.h"
#include "vessel/requestContext.h"
#include "vessel/modifyRecordContext.h"
#include "vessel/runtimeMbContext.h"
#include "vessel/dmlContext.h"
#include "vessel/rdpCompactor.h"

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
                       !context->isMbContextAttached() ||
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

   INT32 rdpAccessor::insertNormalRecord(dmlContext *context,
                                         const dmlInsertRequest &request)
   {
      INT32 rc = SDB_OK;
      const recordDataPageHead *head = NULL;
      RECORD_SLOT_POS pos = INVALID_RECORD_SLOT_POS;
      UINT16 offset = 0;
      recordID rid;
      BOOLEAN ridLocked = FALSE;
      const runtimeMbContext *mbContext = NULL;

      if (OSS_UNLIKELY(NULL == context ||
                       !context->isMbContextAttached() ||
                       !request.isValid()))
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
      else if (isBigRecord(_lpb->getPageSize(),
                           request.record.getSize()))
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

      mbContext = context->getMbContext();
      SDB_ASSERT(mbContext->getGlobalId().getCLLid() == head->clLogcalID,
                 "must be same");

      if (!findPositionToInsert(request.record.getSize(),
                                mbContext->getFloatMinFreePercent(),
                                pos, offset))
      {
         rc = SDB_VESSEL_NOT_ENOUGH_SPACE_IN_PAGE;
         goto error;
      }

      rid.setPid(_lpb->getLogicalPid());
      rid.setPos(pos);

      if (0 < context->getIndexReqCount())
      {
         ossSharedLatchMode mode(OSS_SHARED_LATCH_MODE_ENUM_EXCLUSIVE);
         rc = context->tryLockRid(rid, mode, ridLocked);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to lock rid[%d,%d], rc:%d",
                     rid.getPid(), rid.getPos(), rc);
            goto error;
         }
         else if (!ridLocked)
         {
            PD_LOG(PDERROR, "failed to lock free rid[%d,%d]",
                     rid.getPid(), rid.getPos());
            rc = SDB_VESSEL_INTERNAL_ERR;
            goto error;
         }
      }
      
      /// do not access old page ptr any more.
      head = NULL;

      rc = insertNormalRecordToPos(context, request, pos, offset);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to insert with normal record head:%d", rc);
         goto error;
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

   INT32 rdpAccessor::insertNormalRecordToPos(dmlContext *context,
                                              const dmlInsertRequest &request,
                                              RECORD_SLOT_POS pos,
                                              UINT16 offset)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(NULL != _lpb, "can not be invlaid");
      SDB_ASSERT(NULL != context, "can not be null");
      SDB_ASSERT(request.isValid(), "can not be invalid");
      SDB_ASSERT(isValidRecordSlotPosition(pos), "can not be invalid");
      SDB_ASSERT(0 < offset, "can not be invalid");

      recordDataPageHead oldHead;
      recordDataPageHead *head = NULL;
      recordSlot *slotPtr = NULL;

      logRecordContext lrc;
      recordID rid;
      strictBuffer buffer;
      CHAR *recordPtr = NULL;
      normalRecordHead rh;
      recordSlot rs;
      DPS_TRANS_ID transID = context->getTransIDWithoutTag();
      UINT32 size = request.record.getSize() + NORMAL_RECORD_HEAD_SIZE;
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
      reserved = head->backOffset - offset - size;
      SDB_ASSERT(reserved <= recordSlot::getMaxReservedSize(),
                 "invalid reserved size");
      rs.init(RDP_RECORD_HEAD_TYPE_NORMAL, reserved, offset, size);
      rh.setTransID(transID);

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
      *((normalRecordHead *)recordPtr) = rh;
      ossMemcpy(recordPtr + NORMAL_RECORD_HEAD_SIZE,
                request.record.getData(), request.record.getSize());
      if (0 < rs.reserved)
      {
         ossMemset((void *)(recordPtr + NORMAL_RECORD_HEAD_SIZE + request.record.getSize()),
                   0, rs.reserved);
      }
      updatePageHeadWhenInsert(pos, rs, rh, request.o.stripingId);

      rid.setPid(_lpb->getLogicalPid());
      rid.setPos(pos);
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
      context->setDmlRecordInfo(head->pageSeq, rid);
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
                                             RECORD_SLOT_POS &pos,
                                             UINT16 &offset)const
   {
      BOOLEAN r = FALSE;
      SDB_ASSERT(NULL != _lpb && _lpb->isValid(), "can not be invalid");
      SDB_ASSERT(0.0f <= minFreePercent && minFreePercent <= 50.0f, "out of range");
      SDB_ASSERT(0 < recordSize, "can not be zero");

      constexpr UINT32 _MIN_RESERVED_SIZE = 8;
      constexpr FLOAT32 _OVERSIZE_TOLERANCE = 0.9f;
      const recordDataPageHead *head = getReadablePageHead();

      pos = INVALID_RECORD_SLOT_POS;
      offset = 0;

      UINT32 realDataSize = recordSize + NORMAL_RECORD_HEAD_SIZE;
      UINT32 reservedSize = ossAlign4(recordSize + (UINT32)(recordSize * minFreePercent)) -
                            recordSize;
      if (reservedSize < _MIN_RESERVED_SIZE)
      {
         /// may be not aligned any more
         reservedSize = _MIN_RESERVED_SIZE;
      }
      else if (reservedSize > recordSlot::getMaxReservedSize())
      {
         /// may be not aligned any more
         reservedSize = recordSlot::getMaxReservedSize();
      }

      UINT32 realSlotSize = (INVALID_RECORD_SLOT_POS == head->firstFreeSlot) ?
                             RDP_RSLOT_SIZE : 0;

      UINT32 frontOffset = getFrontOffset(head);
      UINT32 backOffset = head->backOffset;
      UINT32 sizeNeed = realDataSize + reservedSize + realSlotSize;
      UINT32 realSizeAllocating = realDataSize + realSlotSize;
      if (backOffset < (frontOffset + sizeNeed))
      {
         goto done;
      }
      else
      {
         UINT32 remainedSize = head->totalFreeSpace - realSizeAllocating;
         UINT32 minFreeSize = _lpb->getPageSize() * minFreePercent * _OVERSIZE_TOLERANCE;
         if (remainedSize < minFreeSize)
         {
            goto done;
         }
      }

      pos = (INVALID_RECORD_SLOT_POS == head->firstFreeSlot) ?
            head->totalSlotCount : head->firstFreeSlot;
      offset = (UINT16)(backOffset - realDataSize - reservedSize);
      r = TRUE;

   done:
      return r;
   }

   void rdpAccessor::updatePageHeadWhenInsert(RECORD_SLOT_POS pos,
                                              const recordSlot &slot,
                                              const normalRecordHead &rh,
                                              const dmsStripingId &striping)
   {
      SDB_ASSERT(NULL != _lpb && _lpb->isWritable(), "can not be null");
      SDB_ASSERT(isValidRecordSlotPosition(pos), "can not be invalid");
      SDB_ASSERT(slot.isValid() && RDP_RECORD_HEAD_TYPE_NORMAL == slot.type,
                 "must be valid");

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
         head->firstFreeSlot = INVALID_RECORD_SLOT_POS;
         for (INT32 i = pos + 1; i < (INT32)head->totalSlotCount; ++i)
         {
            const recordSlot *tmp = buffer.getReadableObjPtr<recordSlot>
                                    (NORMAL_RECORD_HEAD_SIZE + (i * RDP_RSLOT_SIZE));
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

      updateStripingInfo(head, striping);

      updateMaxTransSN(head, rh.transSN);
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
                                        const dmsStripingId &striping)
   {
      SDB_ASSERT(NULL != head, "can not be null");

      if (striping.isValid())
      {
         dmsStripingId min(head->minStriping);
         if (!min.isValid() || striping < min)
         {
            head->minStriping = striping.getValue();
         }

         dmsStripingId max(head->maxStriping);
         if (!max.isValid() || max < striping)
         {
            head->maxStriping = striping.getValue();
         } 
      }
   }

   INT32 rdpAccessor::prepareInsertLog(dmlContext *context,
                                       UINT32 recordHeadAndBodySize,
                                       const runtimePageBuffer *rpb,
                                       logRecordContext *lrc)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(0 < recordHeadAndBodySize, "can not be zero");
      DPS_TRANS_ID transID = context->getTransIDWithoutTag();
      rc = pageAccessor::prepareLog(context, rpb,
                                    LOG_TYPE_VESSEL_RDP_INSERT,
                                    FALSE, lrc);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to prepare dps log:%d", rc);
         goto error;
      }

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

   INT32 rdpAccessor::prepareInplaceUpdateLog(dmlContext *context,
                                              const runtimePageBuffer *rpb,
                                              logRecordContext *lrc)
   {
      INT32 rc = SDB_OK;
      rc = pageAccessor::prepareLog(context, rpb,
                                    LOG_TYPE_DATA_UPDATE,
                                    FALSE, lrc);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to prepare dps log:%d", rc);
         goto error;
      }

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

   INT32 rdpAccessor::prepareDeleteLog(dmlContext *context,
                                       const runtimePageBuffer *rpb,
                                       logRecordContext *lrc)
   {
      INT32 rc = SDB_OK;
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

   INT32 rdpAccessor::commitInsertLog(dmlContext *context,
                                       const recordID &rid,
                                       const recordSlot &slot,
                                       const void *record,
                                       const recordDataPageHead *oldHead,
                                       const recordDataPageHead *newHead,
                                       const runtimePageBuffer *rpb,
                                       logRecordContext *lrc)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(rid.isValid(), "must be valid");
      SDB_ASSERT(slot.isValid(), "must be valid");
      SDB_ASSERT(NULL != record, "can not be null");
      SDB_ASSERT(NULL != oldHead, "can not be null");
      SDB_ASSERT(NULL != newHead, "can not be null");
      SDB_ASSERT(NULL != lrc, "can not be null");
      SDB_ASSERT(lrc->prepared(), "must be prepared");
      
      DPS_TRANS_ID transID = context->getTransIDWithoutTag();

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

   INT32 rdpAccessor::updateNormalRecord(dmlContext *context,
                                         RECORD_SLOT_POS pos,
                                         const dmsStripingId &striping,
                                         const slice &newRowData,
                                         BOOLEAN &outOfSpace)
   {
      INT32 rc = SDB_OK;
      const recordDataPageHead *head = NULL;
      const recordSlot *rs = NULL;
      recordID rid;

      outOfSpace = FALSE;
      if (OSS_UNLIKELY(NULL == context ||
                       !context->isMbContextAttached() ||
                       !isValidRecordSlotPosition(pos) ||
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
      else if (isBigRecord(_lpb->getPageSize(), newRowData.getSize()))
      {
         PD_LOG(PDERROR, "new row size ouf of size:%d", newRowData.getSize());
         rc = SDB_INVALIDARG;
         goto error;
      }

      head = getReadablePageHead();
      if (head->totalSlotCount <= pos)
      {
         PD_LOG(PDERROR, "pos[%d] out of total slot count[%d]",
                rid.getPos(), head->totalSlotCount);
         rc = SDB_OUT_OF_BOUND;
         goto error;
      }

      rs = getReadableSlot(pos);
      if (OSS_UNLIKELY(NULL == rs))
      {
         PD_LOG(PDERROR, "failed to get slot ptr[%d]", pos);
         rc = SDB_VESSEL_INTERNAL_ERR;
         goto error;
      }

      if (!rs->isValid() || !rs->isNormalRecord())
      {
         PD_LOG(PDERROR, "pos[%d] not valid to be updated", pos);
         rc = SDB_VESSEL_RECORD_NOT_FOUND;
         goto error;
      }

      if ((newRowData.getSize() + NORMAL_RECORD_HEAD_SIZE) <= rs->getMaxSpaceSize())
      {
         rc = inplaceUpdate(context, pos, striping, newRowData);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to inplace update record[%d], rc:%d", pos, rc);
            goto error;
         }
      }
      else if ((newRowData.getSize() + NORMAL_RECORD_HEAD_SIZE) <= 
               getFreeSpaceAfterLastSlot())
      {
         rc = updateByResaving(context, pos, striping, newRowData);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to update record[%d] by resaving, rc:%d", 
                   pos, rc);
            goto error;
         }
      }
      else if ((newRowData.getSize() + NORMAL_RECORD_HEAD_SIZE - rs->size) <= 
               head->totalFreeSpace)
      {
         rc = updateByCompaction(context, pos, striping, newRowData);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to update record[%d] by compacting, rc:%d", 
                  pos, rc);
            goto error;
         }
      }
      else
      {
         outOfSpace = TRUE;
      }
   done:
      return rc;
   error:
      goto done;
   }

   INT32 rdpAccessor::inplaceUpdate(dmlContext *context,
                                    RECORD_SLOT_POS pos,
                                    const dmsStripingId &striping,
                                    const slice &row)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(NULL != context, "can not be null");
      SDB_ASSERT(isValidRecordSlotPosition(pos), "can not be invalid");
      SDB_ASSERT(row.isValid(), "can not be invalid");

      strictBuffer buffer;
      recordSlot *rs = NULL;
      normalRecordHead *rh = NULL;
      strictBuffer recordBuffer;
      recordDataPageHead *head = NULL;
      logRecordContext lrc;
      DPS_TRANS_ID transID = context->getTransIDWithoutTag();
      UINT32 totalSize = row.getSize() + NORMAL_RECORD_HEAD_SIZE;

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

      SDB_ASSERT(rs->isValid() && rs->isNormalRecord(), "impossible");

      if (rs->getMaxSpaceSize() < totalSize)
      {
         PD_LOG(PDERROR, "record size[%d] out of valid range[%d]",
                row.getSize(), rs->getMaxSpaceSize());
         rc = SDB_VESSEL_OPERATOION_NOT_PERMITTED;
         goto error;
      }

      recordBuffer = buffer.getWritableBuffer(rs->getMaxSpaceSize(), rs->offset);
      rh = recordBuffer.getWritableObjPtr<normalRecordHead>(0);
      if (OSS_UNLIKELY(NULL == rh))
      {
         PD_LOG(PDERROR, "failed to get writable record header");
         rc = SDB_VESSEL_INTERNAL_ERR;
         goto error;
      }

      rh->setTransID(transID);
      recordBuffer.write(NORMAL_RECORD_HEAD_SIZE, row.getSize(), row.getData());
      if (totalSize < (UINT32)(rs->size))
      {
         UINT32 deltaSize = (UINT32)(rs->size) - totalSize;
         UINT32 reservedSize = (UINT32)(rs->reserved) + deltaSize;
         rs->size -= deltaSize;
         rs->reserved = (recordSlot::getMaxReservedSize() < reservedSize ) ?
                         recordSlot::getMaxReservedSize() : reservedSize;
         head->totalFreeSpace += deltaSize;
      }
      else if (totalSize > (UINT32)(rs->size))
      {
         UINT32 delta =  totalSize - (UINT32)(rs->size);
         rs->size += delta;
         rs->reserved -= delta;
         head->totalFreeSpace -= delta;
      }

      updateStripingInfo(head, striping);
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
      context->setDmlLSN(lrc.getLsn());
      context->setDmlRecordInfo(head->pageSeq, recordID(_lpb->getLogicalPid(), pos));
   done:
      return rc;
   error:
      if (lrc.prepared())
      {
         pageAccessor::abortLog(context, &lrc);
      }
      goto done;
   }

   INT32 rdpAccessor::updateByResaving(dmlContext *context,
                                       RECORD_SLOT_POS pos,
                                       const dmsStripingId &striping,
                                       const slice &row)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(NULL != context, "can not be null");
      SDB_ASSERT(isValidRecordSlotPosition(pos), "can not be invalid");
      SDB_ASSERT(row.isValid(), "can not be invalid");

      strictBuffer buffer;
      CHAR* oldRecordPtr = NULL;
      strictBuffer newRecordBuffer;
      recordSlot *rs = NULL;
      normalRecordHead rh;
      recordDataPageHead *head = NULL;
      UINT16 offset = 0;
      UINT16 deltaSize = 0;

      logRecordContext lrc;
      DPS_TRANS_ID transID = context->getTransIDWithoutTag();
      UINT32 totalSize = row.getSize() + NORMAL_RECORD_HEAD_SIZE;
      SDB_ASSERT(totalSize <= getFreeSpaceAfterLastSlot(), "not enough free space");

      rc = _lpb->autoGetWritableBodyBuffer(buffer);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to prepare to write");
         goto error;
      }

      head = buffer.getWritableObjPtr<recordDataPageHead>(0);
      if (OSS_UNLIKELY(NULL == head))
      {
         PD_LOG(PDERROR, "failed to get page head");
         rc = SDB_VESSEL_INTERNAL_ERR;
         goto error;
      }

      rs = getWritableSlot(buffer, pos);
      if (OSS_UNLIKELY(NULL == rs))
      {
         PD_LOG(PDERROR, "failed to get writable slot[%d]", pos);
         rc = SDB_VESSEL_INTERNAL_ERR;
         goto error;
      }
      SDB_ASSERT(rs->isValid() && rs->isNormalRecord(), "impossible");

      rh = *(buffer.getReadableObjPtr<normalRecordHead>(rs->offset));
      // reset old record
      oldRecordPtr = buffer.getWritablePtr(rs->offset, rs->getMaxSpaceSize());
      if (OSS_UNLIKELY(NULL == oldRecordPtr))
      {
         rc = SDB_VESSEL_INTERNAL_ERR;
         PD_LOG(PDERROR, "failed to get writable old record");
         goto error;
      }
      ossMemset(oldRecordPtr, 0, rs->getMaxSpaceSize());

      // save new record
      offset = (UINT16)(head->backOffset - totalSize);
      newRecordBuffer = buffer.getWritableBuffer(totalSize, (UINT32)offset);
      rh.setTransID(transID);

      newRecordBuffer.write(0, NORMAL_RECORD_HEAD_SIZE, &rh);
      newRecordBuffer.write(NORMAL_RECORD_HEAD_SIZE, row.getSize(), row.getData());

      // update slot and page head
      SDB_ASSERT(totalSize > rs->size, "impossible");
      deltaSize = (UINT16)(totalSize - rs->size);
      rs->offset = offset;
      rs->reserved = 0;
      rs->size = totalSize;

      SDB_ASSERT(deltaSize <= head->totalFreeSpace, "impossible");
      head->totalFreeSpace -= deltaSize;
      head->backOffset = offset;
      updateStripingInfo(head, striping);
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
      context->setDmlLSN(lrc.getLsn());
      context->setDmlRecordInfo(head->pageSeq, 
                                recordID(_lpb->getLogicalPid(), pos));

   done:
      return rc;
   error:
      if (lrc.prepared())
      {
         pageAccessor::abortLog(context, &lrc);
      }
      goto done;
   }

   INT32 rdpAccessor::updateByCompaction(dmlContext *context,
                                         RECORD_SLOT_POS pos,
                                         const dmsStripingId &striping,
                                         const slice &row)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(NULL != context, "can not be null");
      SDB_ASSERT(isValidRecordSlotPosition(pos), "can not be invalid");
      SDB_ASSERT(row.isValid(), "can not be invalid");

      strictBuffer buffer; 
      const recordDataPageHead *rhead = NULL;
      recordDataPageHead *whead = NULL;
      const recordSlot *rs = getReadableSlot(pos);
      normalRecordHead rh;

      logRecordContext lrc;
      DPS_TRANS_ID transID = context->getTransIDWithoutTag();
      rdpCompactor compactor;

      // compactor only needs slots and data
      UINT32 compactBufSize = getPageBodySize(_lpb->getPageSize())
                              - RECORD_PAGE_HEAD_SIZE;
      CHAR *compactBuf = context->allocateBuffer(compactBufSize);
      if (OSS_UNLIKELY(NULL == compactBuf))
      {
         rc = SDB_VESSEL_INTERNAL_ERR;
         PD_LOG(PDERROR, "failed to allocate compact buffer");
         goto error;
      }
      compactor.reset(compactBuf, compactBufSize);

      buffer = _lpb->getReadableBodyBuffer();
      rhead = buffer.getReadableObjPtr<recordDataPageHead>(0);
      if (OSS_UNLIKELY(NULL == rhead))
      {
         PD_LOG(PDERROR, "failed to get page head");
         rc = SDB_VESSEL_INTERNAL_ERR;
         goto error;
      }
      SDB_ASSERT(row.getSize() + NORMAL_RECORD_HEAD_SIZE - rs->size <= 
                 rhead->totalFreeSpace, "not enough free space");

      for (UINT16 i = 0; i < rhead->totalSlotCount; ++i)
      {
         rs = getReadableSlot(i);
         if (OSS_UNLIKELY(NULL == rs))
         {
            rc = SDB_VESSEL_INTERNAL_ERR;
            PD_LOG(PDERROR, "failed to get readable slot[%d]", i);
            goto error;
         }

         if (!rs->isValid())
         {
            rc = compactor.pushEmptySlot();
            if (SDB_OK != rc)
            {
               PD_LOG(PDERROR, "failed to compact the page");
               goto error;
            }
         }
         else if (i == pos)
         {
            normalRecordHead compactRH = *(buffer.getReadableObjPtr<normalRecordHead>(rs->offset));
            compactRH.setTransID(transID);
            rc = compactor.push(rs->flags, rs->type, 
                                slice(NORMAL_RECORD_HEAD_SIZE, &compactRH), 
                                row);
            if (SDB_OK != rc)
            {
               PD_LOG(PDERROR, "failed to compact the page");
               goto error;
            }
         }
         else
         {
            rc = compactor.push(rs->flags, rs->type, buffer.getSlice(rs->offset, rs->size));
            if (SDB_OK != rc)
            {
               PD_LOG(PDERROR, "failed to compact the page");
               goto error;
            }
         }
      }

      rc = _lpb->autoGetWritableBodyBuffer(buffer);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to prepare to write");
         goto error;
      }
      whead = buffer.getWritableObjPtr<recordDataPageHead>(0);
      if (OSS_UNLIKELY(NULL == whead))
      {
         rc = SDB_VESSEL_INTERNAL_ERR;
         PD_LOG(PDERROR, "failed to get writable page head");
         goto error;
      }

      // write the compact buf back
      rc = buffer.write(RECORD_PAGE_HEAD_SIZE, compactBufSize, 
                        compactor.getBuffer());
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to write the buffer");
         goto error;
      }

      // update head
      whead->totalFreeSpace = compactor.getFreeSpace();
      whead->backOffset = compactor.getCorrectBackOffset();
      whead->totalSlotCount = compactor.getSlotCount();
      updateStripingInfo(whead, striping);
      updateMaxTransSN(whead, transID.getSN());

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
      context->setDmlLSN(lrc.getLsn());
      context->setDmlRecordInfo(rhead->pageSeq, 
                                recordID(_lpb->getLogicalPid(), pos));

   done:
      context->releaseBuffer(compactBuf);
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
      SDB_ASSERT(NULL != context && context->isMbContextAttached(), "can not be invalid");
      SDB_ASSERT(NULL != lpb && lpb->isValid(), "can not be invalid");
      const recordDataPageHead *head = NULL;
      UINT32 lid = context->getMbContext()->getGlobalId().getCLLid();
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

      if (head->clLogcalID != lid)
      {
         PD_LOG(PDERROR, "logical id[%d] in context does not match the one[%d] in page",
                lid, head->clLogcalID);
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

   const recordSlot *rdpAccessor::getReadableSlot(RECORD_SLOT_POS pos)const
   {
      SDB_ASSERT(NULL != _lpb && _lpb->isValid(), "can not be invalid");
      SDB_ASSERT(isValidRecordSlotPosition(pos), "can not be invalid");
      SDB_ASSERT((UINT32)pos < getTotalSlotCount(), "out of bound");
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

   UINT32 rdpAccessor::getFreeSpaceAfterLastSlot()const
   {
      const recordDataPageHead *head = getReadablePageHead();
      return (UINT32)(head->backOffset) - getFrontOffset(head);
   }

   INT32 rdpAccessor::getRecordCount(UINT32 &count)const
   {
      INT32 rc = SDB_OK;

      const recordDataPageHead *head = NULL;
      count = 0;

      if (OSS_UNLIKELY(NULL == _lpb))
      {
         rc = SDB_VESSEL_RECORD_NOT_FOUND;
         goto error;
      }

      head = getReadablePageHead();
      if (OSS_UNLIKELY(NULL == head))
      {
         PD_LOG(PDERROR, "failed to get readable page header");
         rc = SDB_VESSEL_INTERNAL_ERR;
         goto error;
      }

      for (RECORD_SLOT_POS i = 0; i < head->totalSlotCount; ++i)
      {
         const recordSlot *slot = getReadableSlot(i);
         if (slot->isValidAndVisible() && !slot->isTombstoneRecord())
         {
            ++count;
         }
      }
   done:
      return rc;
   error:
      goto done;
   }


   FLOAT32 rdpAccessor::getFreeSpacePercent()const
   {
      const recordDataPageHead *head = getReadablePageHead();
      return (FLOAT32)(head->totalFreeSpace) / _lpb->getPageSize();
   }

   INT32 rdpAccessor::getSlot(RECORD_SLOT_POS pos, recordSlot &rs)const
   {
      INT32 rc = SDB_OK;
      rs.reset();

      if (OSS_UNLIKELY(INVALID_RECORD_SLOT_POS == pos))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      else if (OSS_UNLIKELY(NULL == _lpb))
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }
      else if (OSS_UNLIKELY((INT16)(getReadablePageHead()->totalSlotCount) <= pos))
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

   INT32 rdpAccessor::getNormalRecord(RECORD_SLOT_POS pos,
                                      normalRecordHead &rh,
                                      slice &data)const
   {
      INT32 rc = SDB_OK;
      recordSlot rs;
      strictBuffer buffer;
      strictBuffer recordBuffer;
      const normalRecordHead *header = NULL;

      rh = normalRecordHead();
      data.reset();

      if (OSS_UNLIKELY(INVALID_RECORD_SLOT_POS == pos))
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

      if (!rs.isValid() || !rs.isNormalRecord())
      {
         PD_LOG(PDERROR, "slot type is not normal:%d", rs.type);
         rc = SDB_VESSEL_OPERATOION_NOT_PERMITTED;
         goto error;
      }

      buffer = _lpb->getReadableBodyBuffer();
      recordBuffer = buffer.getReadableBuffer(rs.size, rs.offset);
      if (OSS_UNLIKELY(!recordBuffer.isValid()))
      {
         PD_LOG(PDERROR, "failed to get record buffer[%d,%d]", rs.size, rs.offset);
         rc = SDB_VESSEL_INTERNAL_ERR;
         goto error;
      }

      header = recordBuffer.getReadableObjPtr<normalRecordHead>(0);
      if (OSS_UNLIKELY(NULL == header))
      {
         PD_LOG(PDERROR, "failed to get record header[%d,%d]", rs.offset, rs.size);
         rc = SDB_VESSEL_INTERNAL_ERR;
         goto error;
      }

      rh = *header;
      data.reset(rs.size - NORMAL_RECORD_HEAD_SIZE,
                 recordBuffer.getReadablePtr(NORMAL_RECORD_HEAD_SIZE,
                                             rs.size - NORMAL_RECORD_HEAD_SIZE));
   done:
      return rc;
   error:
      goto done;
   }

   recordSlot *rdpAccessor::getWritableSlot(strictBuffer &buffer,
                                            RECORD_SLOT_POS pos)
   {
      SDB_ASSERT(buffer.isWritable(), "must be writable");
      SDB_ASSERT(INVALID_RECORD_SLOT_POS != pos, "can not be invalid");
      return buffer.getWritableObjPtr<recordSlot>(RECORD_PAGE_HEAD_SIZE +
                                                  (pos * RDP_RSLOT_SIZE));

   }

   INT32 rdpAccessor::deleteNormalRecord(dmlContext *context,
                                         RECORD_SLOT_POS pos)
   {
      INT32 rc = SDB_OK;
      recordSlot rs;

      if (OSS_UNLIKELY(NULL == context ||
                       !context->isMbContextAttached() ||
                       INVALID_RECORD_SLOT_POS == pos))
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

      rc = getSlot(pos, rs);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to get slot ptr[%d]", pos);
         rc = SDB_VESSEL_INTERNAL_ERR;
         goto error;
      }

      if (!rs.isValid() || !rs.isNormalRecord())
      {
         PD_LOG(PDERROR, "pos[%d] not valid to be delete", pos);
         rc = SDB_VESSEL_RECORD_NOT_FOUND;
         goto error;
      }

      rc = createTombstone(context, pos);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to delete record[%d], rc:%d", pos);
         goto error;
      }
   done:
      return rc;
   error:
      goto done;
   }

   INT32 rdpAccessor::createTombstone(dmlContext *context,
                                      RECORD_SLOT_POS pos)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(NULL != context, "can not be null");
      SDB_ASSERT(NULL != _lpb, "can not be null");

      strictBuffer buffer, recordBuffer;
      recordSlot *rs = NULL;
      recordDataPageHead *head = NULL;
      tombstoneRecord *tr = NULL;
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

      SDB_ASSERT(rs->isValid() && rs->isNormalRecord(), "impossible");

      

      recordBuffer = buffer.getWritableBuffer(rs->size, rs->offset);
      if (!recordBuffer.isWritable())
      {
         PD_LOG(PDERROR, "failed to get writable buffer[%d,%d]",
                rs->size, rs->offset);
         rc = SDB_VESSEL_INTERNAL_ERR;
         goto error;
      }

      tr = recordBuffer.getWritableObjPtr<tombstoneRecord>(0);
      if (OSS_UNLIKELY(NULL == tr))
      {
         PD_LOG(PDERROR, "failed to get writable record header");
         rc = SDB_VESSEL_INTERNAL_ERR;
         goto error;
      }

      size = rs->size - TOMBSTONE_RECORD_SIZE;
      *tr = tombstoneRecord();
      tr->setTransID(transID);
      rs->size = TOMBSTONE_RECORD_SIZE;
      rs->reserved = 0;
      rs->type = RDP_RECORD_HEAD_TOMBSTONE;

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
      context->setDmlLSN(lrc.getLsn());
      context->setDmlRecordInfo(head->pageSeq, recordID(_lpb->getLogicalPid(), pos));
      
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