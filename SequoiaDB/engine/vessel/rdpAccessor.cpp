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

   Source File Name = rdpAccessor.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

*******************************************************************************/
#include "vessel/rdpAccessor.h"
#include "vessel/logicalPageBuffer.h"
#include "dpsLogRecordDef.hpp"
#include "vessel/requestContext.h"
#include "vessel/modifyRecordContext.h"
#include "vessel/collectionProperties.h"
#include "vessel/dmlContext.h"
#include "vessel/rdpCompactor.h"
#include "dpsWriteReqBuilder.hpp"
#include "vessel/outerResource.h"

namespace engine
{
namespace vessel
{
   INT32 rdpAccessor::init(requestContext *context,
                           logicalPageBuffer *lpb)
   {
      INT32 rc = SDB_OK;

      _lpb = nullptr;
      if (OSS_UNLIKELY(nullptr == context ||
                       !context->isClPropertiesSet() ||
                       nullptr == lpb ||
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
                                         const slice &record)
   {
      INT32 rc = SDB_OK;
      const recordDataPageHead *head = nullptr;
      RECORD_SLOT_POS pos = INVALID_RECORD_SLOT_POS;
      UINT16 offset = 0;
      recordID rid;
      BOOLEAN ridLocked = FALSE;
      const collectionProperties *properties = nullptr;

      if (OSS_UNLIKELY(nullptr == context ||
                       !context->isClPropertiesSet() ||
                       !record.isValid()))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      else if (OSS_UNLIKELY(nullptr == _lpb))
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }
      else if (!_lpb->getLockingMode().isExclusiveOrUpgrade())
      {
         rc = SDB_VESSEL_FORBIDDEN_OP_WLT;
         goto error;
      }
      else if (isBigRecord(_lpb->getPageSize(), record.getSize()))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      head = _lpb->getReadableBodyBuffer().getReadableObjPtr<recordDataPageHead>(0);
      if (nullptr == head)
      {
         PD_LOG(PDERROR, "failed to get readble record page head");
         rc = SDB_VESSEL_INTERNAL_ERR;
         goto error;
      }

      properties = context->getClProperties();
      SDB_ASSERT(properties->clid.getLid() == head->clLogcalID,
                 "must be same");

      if (!findPositionToInsert(record.getSize(),
                                properties->getMinFreePct(),
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
      head = nullptr;

      rc = insertNormalRecordToPos(context, record, pos, offset);
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
                                              const slice &record,
                                              RECORD_SLOT_POS pos,
                                              UINT16 offset)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(nullptr != _lpb, "can not be invalid");
      SDB_ASSERT(nullptr != context, "can not be null");
      SDB_ASSERT(record.isValid(), "can not be invalid");
      SDB_ASSERT(isValidRecordSlotPosition(pos), "can not be invalid");
      SDB_ASSERT(0 < offset, "can not be invalid");

      recordDataPageHead oldHead;
      recordDataPageHead *head = nullptr;
      recordSlot *slotPtr = nullptr;
      DPS_LSN_OFFSET lsn = DPS_INVALID_LSN_OFFSET;
      recordID rid;
      strictBuffer buffer;
      CHAR *recordPtr = nullptr;
      normalRecordHead rh;
      recordSlot rs;
      DPS_TRANS_ID transID = context->getOrigTransId();
      UINT32 size = record.getSize() + NORMAL_RECORD_HEAD_SIZE;
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

      slotPtr = buffer.getWritableObjPtr<recordSlot>(RECORD_PAGE_HEAD_SIZE +
                                                     (pos * RDP_RSLOT_SIZE));
      if (nullptr == slotPtr)
      {
         PD_LOG(PDERROR, "failed to get writable slot ptr[%d]", pos);
         rc = SDB_VESSEL_INTERNAL_ERR;
         goto error;
      }

      recordPtr = buffer.getWritablePtr(offset, head->backOffset - offset);
      if (nullptr == recordPtr)
      {
         PD_LOG(PDERROR, "failed to get writable record ptr[%d,%d]",
                offset, head->backOffset - offset);
         rc = SDB_VESSEL_INTERNAL_ERR;
         goto error;
      }

      rc = writeInsertJournal(context, lsn);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to write journal:%d", rc);
         goto error;
      }

      oldHead = *head;
      *slotPtr = rs;
      *((normalRecordHead *)recordPtr) = rh;
      ossMemcpy(recordPtr + NORMAL_RECORD_HEAD_SIZE,
                record.getData(), record.getSize());
      if (0 < rs.reservedSpaceSize)
      {
         ossMemset((void *)(recordPtr + NORMAL_RECORD_HEAD_SIZE + record.getSize()),
                   0, rs.reservedSpaceSize);
      }
      updatePageHeadWhenInsert(pos, rs, transID, context->getStripingId());

      rid.setPid(_lpb->getLogicalPid());
      rid.setPos(pos);

      context->setDmlLSN(lsn);
      context->setDmlRecordInfo(head->pageSeq, rid);
      _lpb->commit(lsn);

   done:
      return rc;
   error:
      goto done;
   }

   BOOLEAN rdpAccessor::findPositionToInsert(UINT32 recordSize,
                                             FLOAT32 minFreePercent,
                                             RECORD_SLOT_POS &pos,
                                             UINT16 &offset)const
   {
      BOOLEAN r = FALSE;
      SDB_ASSERT(nullptr != _lpb && _lpb->isValid(), "can not be invalid");
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

      UINT32 realSlotSize = (0 == head->freeSlotCount) ?
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

      if (0 == head->freeSlotCount)
      {
         pos = head->totalSlotCount;
      }
      else
      {
         for (UINT16 i = 0; i < head->totalSlotCount; ++i)
         {
            const recordSlot *slot = getReadableSlot(i);
            if (!slot->isValid())
            {
               pos = static_cast<RECORD_SLOT_POS>(i);
               break;
            }
         }

         SDB_ASSERT(INVALID_RECORD_SLOT_POS != pos, "impossible");
      }

      offset = (UINT16)(backOffset - realDataSize - reservedSize);
      r = TRUE;

   done:
      return r;
   }

   void rdpAccessor::updatePageHeadWhenInsert(RECORD_SLOT_POS pos,
                                              const recordSlot &slot,
                                              const DPS_TRANS_ID &transID,
                                              const dmsStripingId &striping)
   {
      SDB_ASSERT(nullptr != _lpb && _lpb->isWritable(), "can not be null");
      SDB_ASSERT(isValidRecordSlotPosition(pos), "can not be invalid");
      SDB_ASSERT(slot.isValid(), "must be valid");

      strictBuffer buffer = _lpb->getWritableBodyBuffer();
      recordDataPageHead *head = buffer.getWritableObjPtr<recordDataPageHead>(0);
      UINT32 size = slot.size;

      if (head->totalSlotCount == pos)
      {
         ++head->totalSlotCount;
         size += RDP_RSLOT_SIZE;
      }
      else
      {
         SDB_ASSERT(0 < head->freeSlotCount, "impossible");
         --head->freeSlotCount;
      }

      if (!slot.isInvisible() && !slot.isTombstone())
      {
         ++head->totalRecordCount;
      }

      SDB_ASSERT(size <= head->totalFreeSpace, "impossible");
      head->totalFreeSpace -= size;
      head->backOffset = slot.offset;

      updateStripingInfo(head, striping);
      updateMaxTransSN(head, transID.getSN());
   }

   void rdpAccessor::updateMaxTransSN(recordDataPageHead *head,
                                      UINT64 transSN)
   {
      SDB_ASSERT(nullptr != head, "can not be null");
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
      SDB_ASSERT(nullptr != head, "can not be null");

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

   INT32 rdpAccessor::updateNormalRecord(dmlContext *context,
                                         RECORD_SLOT_POS pos,
                                         const slice &newRowData,
                                         BOOLEAN &outOfSpace)
   {
      INT32 rc = SDB_OK;
      const recordDataPageHead *head = nullptr;
      const recordSlot *rs = nullptr;
      recordID rid;

      outOfSpace = FALSE;
      if (OSS_UNLIKELY(nullptr == context ||
                       !context->isClPropertiesSet() ||
                       !isValidRecordSlotPosition(pos) ||
                       !newRowData.isValid()))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      else if (OSS_UNLIKELY(nullptr == _lpb))
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
      if (OSS_UNLIKELY(nullptr == rs))
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
         rc = inplaceUpdate(context, pos, context->getStripingId(), newRowData);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to inplace update record[%d], rc:%d", pos, rc);
            goto error;
         }
      }
      else if ((newRowData.getSize() + NORMAL_RECORD_HEAD_SIZE) <= 
               getFreeSpaceAfterLastSlot())
      {
         rc = updateByResaving(context, pos, context->getStripingId(), newRowData);
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
         rc = updateByCompaction(context, pos, context->getStripingId(), newRowData);
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

   INT32 rdpAccessor::insertInvisibleNormalRecord(dmlContext *context,
                                                  const slice &record,
                                                  recordID &rid)
   {
      INT32 rc = SDB_OK;
      const recordDataPageHead *head = nullptr;
      RECORD_SLOT_POS pos = INVALID_RECORD_SLOT_POS;
      UINT16 offset = 0;

      rid.reset();
      if (OSS_UNLIKELY(nullptr == context ||
                       !context->isClPropertiesSet() ||
                       !record.isValid()))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      else if (OSS_UNLIKELY(nullptr == _lpb))
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
                           record.getSize()))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      head = _lpb->getReadableBodyBuffer().getReadableObjPtr<recordDataPageHead>(0);
      if (nullptr == head)
      {
         PD_LOG(PDERROR, "failed to get readble record page head");
         rc = SDB_VESSEL_INTERNAL_ERR;
         goto error;
      }

      SDB_ASSERT(context->getLogicalClId() == head->clLogcalID,
                 "must be same");

      if (!findPositionToInsert(record.getSize(),
                                context->getClProperties()->getMinFreePct(),
                                pos, offset))
      {
         rc = SDB_VESSEL_NOT_ENOUGH_SPACE_IN_PAGE;
         goto error;
      }

      /// do not access old page ptr any more.
      head = nullptr;

      rc = insertInvisibleNormalRecordToPos(context, record, pos, offset);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to insert with normal record head:%d", rc);
         goto error;
      }
      rid.setPid(_lpb->getLogicalPid());
      rid.setPos(pos);

   done:
      return rc;
   error:
      goto done;
   }

   INT32 rdpAccessor::insertInvisibleNormalRecordToPos(dmlContext *context,
                                                       const slice &row,
                                                       RECORD_SLOT_POS pos,
                                                       UINT16 offset)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(nullptr != context, "can not be null");
      SDB_ASSERT(nullptr != _lpb, "can not be invlaid");
      SDB_ASSERT(row.isValid(), "can not be invalid");
      SDB_ASSERT(isValidRecordSlotPosition(pos), "can not be invalid");

      strictBuffer buffer;
      recordDataPageHead *head = nullptr;
      overflowedRecord orh;
      DPS_TRANS_ID transID = context->getOrigTransId();
      recordID rid;

      recordDataPageHead oldHead;
      strictBuffer slotBuffer;
      strictBuffer recordBuffer;
      const CHAR *recordPtr = nullptr;
      recordSlot rs;
      normalRecordHead rh;
      UINT32 size = row.getSize() + NORMAL_RECORD_HEAD_SIZE;
      UINT32 reserved = 0;

      DPS_LSN_OFFSET lsn = DPS_INVALID_LSN_OFFSET;

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
      rs.setInvisible();
      rh.setTransID(transID);

      slotBuffer = buffer.getWritableBuffer(RDP_RSLOT_SIZE, 
                                            RECORD_PAGE_HEAD_SIZE +
                                            (pos * RDP_RSLOT_SIZE));
      if (OSS_UNLIKELY(!slotBuffer.isWritable()))
      {
         rc = SDB_VESSEL_INTERNAL_ERR;
         PD_LOG(PDERROR, "failed to get writable slot buffer[%d], rc:%d", pos, rc);
         goto error;
      }

      recordBuffer = buffer.getWritableBuffer(head->backOffset - offset, offset);
      if (OSS_UNLIKELY(!recordBuffer.isWritable()))
      {
         rc = SDB_VESSEL_INTERNAL_ERR;
         PD_LOG(PDERROR, "failed to get writable record buffer[%d,%d]",
                offset, head->backOffset - offset);
         goto error;
      }

      rc = writeInsertJournal(context, lsn);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to write journal:%d", rc);
         goto error;
      }

      oldHead = *head;
      rc = slotBuffer.write(0, RDP_RSLOT_SIZE, &rs);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to write slot, rc:%d", rc);
         goto error;
      }
      recordBuffer.write(0, NORMAL_RECORD_HEAD_SIZE, &rh);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to write record head, rc:%d", rc);
         goto error;
      }
      recordBuffer.write(NORMAL_RECORD_HEAD_SIZE, row.getSize(), row.getData());
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to write record data, rc:%d", rc);
         goto error;
      }
      if (0 < rs.reservedSpaceSize)
      {
         recordBuffer.setBuffer(rs.size, rs.reservedSpaceSize, 0);
      }

      // no need to update transSN and stripingID
      updatePageHeadWhenInsert(pos, rs, DPS_TRANS_ID(), dmsStripingId());
      
      rid.setPid(_lpb->getLogicalPid());
      rid.setPos(pos);

      recordPtr = recordBuffer.getReadablePtr(0, rs.getMaxSpaceSize());
      if (OSS_UNLIKELY(nullptr == recordPtr))
      {
         rc = SDB_VESSEL_INTERNAL_ERR;
         PD_LOG(PDERROR, "failed to get readable record, rc:%d", rc);
      }

      _lpb->commit(lsn);
   done:
      return rc;
   error:
      goto done;
   }

   INT32 rdpAccessor::insertOverflowedRecord(dmlContext *context,
                                             const recordID &overflowAddr,
                                             BOOLEAN isBigRecord)
   {
      INT32 rc = SDB_OK;
      const recordDataPageHead *head = nullptr;
      RECORD_SLOT_POS pos = INVALID_RECORD_SLOT_POS;
      UINT16 offset = 0;
      recordID rid;
      BOOLEAN ridLocked = FALSE;

      if (OSS_UNLIKELY(nullptr == context ||
                       !overflowAddr.isValid()))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      else if (OSS_UNLIKELY(nullptr == _lpb))
      {
         rc = SDB_VESSEL_INTERNAL_ERR;
         goto error;
      }
      else if (!_lpb->getLockingMode().isExclusiveOrUpgrade())
      {
         rc = SDB_VESSEL_FORBIDDEN_OP_WLT;
         goto error;
      }

      head = _lpb->getReadableBodyBuffer().getReadableObjPtr<recordDataPageHead>(0);
      if (nullptr == head)
      {
         PD_LOG(PDERROR, "failed to get readble record page head");
         rc = SDB_VESSEL_INTERNAL_ERR;
         goto error;
      }

      SDB_ASSERT(context->getLogicalClId() == head->clLogcalID,
                 "must be same");

      if (!findPositionToInsert(OVERFLOWED_RECORD_SIZE,
                                context->getClProperties()->getMinFreePct(),
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
      head = nullptr;
      rc = insertOverflowedRecordToPos(context, overflowAddr, isBigRecord, pos, offset);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to insert overflowed record to pos, rc:%d", rc);
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

   INT32 rdpAccessor::insertOverflowedRecordToPos(dmlContext *context,
                                                  const recordID &overflowAddr,
                                                  BOOLEAN isBigRecord,
                                                  RECORD_SLOT_POS pos,
                                                  UINT16 offset)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(nullptr != _lpb, "can not be invalid");
      SDB_ASSERT(nullptr != context, "can not be null");
      SDB_ASSERT(overflowAddr.isValid(), "can not be invalid");
      SDB_ASSERT(isValidRecordSlotPosition(pos), "can not be invalid");
      SDB_ASSERT(0 < offset, "can not be invalid");

      strictBuffer buffer;
      strictBuffer slotBuffer;
      strictBuffer recordBuffer;
      recordDataPageHead *head = nullptr;
      recordDataPageHead oldHead;
      const CHAR *recordPtr = nullptr;
      recordSlot rs;
      overflowedRecord ofr;
      DPS_TRANS_ID transID = context->getOrigTransId();
      recordID rid;
      DPS_LSN_OFFSET lsn = DPS_INVALID_LSN_OFFSET;

      rc = _lpb->autoGetWritableBodyBuffer(buffer);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to get writable buffer, rc:%d", rc);
         goto error;
      }

      head = buffer.getWritableObjPtr<recordDataPageHead>(0);
      if (OSS_UNLIKELY(nullptr == head))
      {
         rc = SDB_VESSEL_INTERNAL_ERR;
         PD_LOG(PDERROR, "failed to get readable head, rc:%d", rc);
         goto error;
      }
      
      slotBuffer = buffer.getWritableBuffer(RDP_RSLOT_SIZE, 
                                            RECORD_PAGE_HEAD_SIZE +
                                            (pos * RDP_RSLOT_SIZE));
      if (!slotBuffer.isWritable())
      {
         rc = SDB_VESSEL_INTERNAL_ERR;
         PD_LOG(PDERROR, "failed to get writable slot buf, rc:%d", rc);
         goto error;
      }

      recordBuffer = buffer.getWritableBuffer(OVERFLOWED_RECORD_SIZE, offset);
      if (!recordBuffer.isWritable())
      {
         rc = SDB_VESSEL_INTERNAL_ERR;
         PD_LOG(PDERROR, "failed to get writable record buf, rc:%d", rc);
         goto error;
      }

      oldHead = *head;

      rs.init(RDP_RECORD_HEAD_OVERFLOW, 0, offset, OVERFLOWED_RECORD_SIZE);
      rc = slotBuffer.write(0, RDP_RSLOT_SIZE, &rs);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to write slot, rc:%d", rc);
         goto error;
      }

      ofr.lpid = overflowAddr.getPid();
      ofr.pos = overflowAddr.getPos();
      if (isBigRecord)
      {
         ofr.setAsBigRecord();
      }
      rc = recordBuffer.write(0, OVERFLOWED_RECORD_SIZE, &ofr);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to write overflowed record, rc:%d", rc);
         goto error;
      }

      updatePageHeadWhenInsert(pos, rs, transID, context->getStripingId());

      rid.setPid(_lpb->getLogicalPid());
      rid.setPos(pos);

      rc = writeInsertJournal(context, lsn);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to write journal:%d", rc);
         goto error;
      }

      recordPtr = recordBuffer.getReadablePtr(0, rs.getMaxSpaceSize());
      if (OSS_UNLIKELY(nullptr == recordPtr))
      {
         rc = SDB_VESSEL_INTERNAL_ERR;
         PD_LOG(PDERROR, "failed to get readable record, rc:%d", rc);
      }

      _lpb->commit(lsn);

      context->setDmlLSN(lsn);
      context->setDmlRecordInfo(head->pageSeq, rid);
   
   done:
      return rc;
   error:
      goto done;
   } 

   INT32 rdpAccessor::setRecordOverflowed(dmlContext *context,
                                          RECORD_SLOT_POS pos,
                                          const recordID &overflowAddr,
                                          BOOLEAN isBigRecord)
   {
      INT32 rc = SDB_OK;
      strictBuffer buffer;
      const recordDataPageHead *rhead = nullptr;
      recordDataPageHead *whead = nullptr;
      const recordSlot *rrs = nullptr;
      recordSlot *wrs = nullptr;
      overflowedRecord ofr;
      strictBuffer oldRecordBuf;
      UINT32 deltaSize = 0;
      DPS_LSN_OFFSET lsn = DPS_INVALID_LSN_OFFSET;
      DPS_TRANS_ID transID = context->getOrigTransId();

      if (OSS_UNLIKELY(nullptr == context ||
                       !isValidRecordSlotPosition(pos) ||
                       !overflowAddr.isValid()))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      rhead = getReadablePageHead();
      if (OSS_UNLIKELY(nullptr == rhead))
      {
         rc = SDB_VESSEL_INTERNAL_ERR;
         PD_LOG(PDERROR, "failed to get readable page head, rc:%d", rc);
         goto error;
      }
      else if (rhead->totalSlotCount < pos)
      {
         rc = SDB_OUT_OF_BOUND;
         PD_LOG(PDERROR, "pos is out of page total slot count, rc:%d", rc);
         goto error;
      }

      rrs = getReadableSlot(pos);
      if (OSS_UNLIKELY(nullptr == rrs))
      {
         rc = SDB_VESSEL_INTERNAL_ERR;
         PD_LOG(PDERROR, "failed to get readable slot[%d], rc:%d", pos, rc);
         goto error;
      }
      else if (!rrs->isValidAndVisible() || 
               !rrs->isNormalRecord())
      {
         rc = SDB_INVALID_OPERATION;
         PD_LOG(PDERROR, "invalid, invisible and non-normal record " 
                         "cannot be set to overflowed, rc:%d", rc);
         goto error;
      }
      else if(rrs->size < OVERFLOWED_RECORD_SIZE)
      {
         rc = SDB_VESSEL_INTERNAL_ERR;
         PD_LOG(PDERROR, "invalid slot[%d] and record, rc:%d", pos, rc);
         goto error;
      }

      rc = _lpb->autoGetWritableBodyBuffer(buffer);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to get writable buffer, rc:%d", rc);
         goto error;
      }

      whead = buffer.getWritableObjPtr<recordDataPageHead>(0);
      if (OSS_UNLIKELY(nullptr == whead))
      {
         rc = SDB_VESSEL_INTERNAL_ERR;
         PD_LOG(PDERROR, "failed to get writable page head");
         goto error;
      }

      wrs = getWritableSlot(buffer, pos);
      if (OSS_UNLIKELY(nullptr == wrs))
      {
         rc = SDB_VESSEL_INTERNAL_ERR;
         PD_LOG(PDERROR, "failed to get writable page head");
         goto error;
      }

      deltaSize = wrs->size - OVERFLOWED_RECORD_SIZE;
      ofr.lpid = overflowAddr.getPid();
      ofr.pos = overflowAddr.getPos();
      if (isBigRecord)
      {
         ofr.setAsBigRecord();
      }

      oldRecordBuf = buffer.getWritableBuffer(wrs->getMaxSpaceSize(), wrs->offset);
      if (OSS_UNLIKELY(!oldRecordBuf.isWritable()))
      {
         rc = SDB_VESSEL_INTERNAL_ERR;
         PD_LOG(PDERROR, "failed to get writable buffer, rc:%d", rc);
         goto error;
      }
      rc = oldRecordBuf.write(0, OVERFLOWED_RECORD_SIZE, &ofr);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to write overflow record head, rc:%d", rc);
         goto error;
      }
      oldRecordBuf.setBuffer(OVERFLOWED_RECORD_SIZE, 
                             wrs->getMaxSpaceSize() - OVERFLOWED_RECORD_SIZE, 
                             0);

      wrs->type = RDP_RECORD_HEAD_OVERFLOW;
      wrs->size = OVERFLOWED_RECORD_SIZE;
      wrs->reservedSpaceSize = 0;

      whead->totalFreeSpace += deltaSize;
      updateStripingInfo(whead, context->getStripingId());
      updateMaxTransSN(whead, transID.getSN());

      rc = writeInplaceUpdateJournal(context, lsn);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to write journal:%d", rc);
         goto error;
      }

      _lpb->commit(lsn);
      context->setDmlLSN(lsn);

   done:
      return rc;
   error:
      goto done;

   }
   INT32 rdpAccessor::insertBigRecordSlice(dmlContext *context,
                                           bigRecordStream &recordStream)
   {
      INT32 rc = SDB_OK;
      UINT32 realEntrySize = 0;
      const recordDataPageHead *head = nullptr;

      if (OSS_UNLIKELY(nullptr == context) ||
          !recordStream.isValid())
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      else if (OSS_UNLIKELY(nullptr == _lpb))
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }
      else if (!_lpb->getLockingMode().isExclusiveOrUpgrade())
      {
         rc = SDB_VESSEL_FORBIDDEN_OP_WLT;
         goto error;
      }
      
      realEntrySize = recordStream.getRemainingSize() +
                      RDP_RSLOT_SIZE + BIG_RECORD_ENTRY_SIZE;

      head = getReadablePageHead();
      if (OSS_UNLIKELY(nullptr == head))
      {
         rc = SDB_VESSEL_INTERNAL_ERR;
         PD_LOG(PDERROR, "failed to get readable head, rc:%d", rc);
         goto error;         
      }
      
      SDB_ASSERT(head->totalFreeSpace > (RDP_RSLOT_SIZE + BIG_RECORD_BODY_SLICE_SIZE), 
                 "impossible");
      if (getFreeSpaceAfterLastSlot() >= realEntrySize)
      {
         rc = insertBigRecordEntrySlice(context, recordStream);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to insert big record entry to page[%d], rc:%d",
                   _lpb->getLogicalPid(), rc);
            goto error;
         }
      }
      else
      {
         rc = insertBigRecordBodySlice(context, recordStream);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to insert big record body to page[%d], rc:%d",
                   _lpb->getLogicalPid(), rc);
            goto error;
         }
      }

   done:
      return rc;
   error:
      goto done;

   }

   INT32 rdpAccessor::insertBigRecordEntrySlice(dmlContext *context,
                                                bigRecordStream &recordStream)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(nullptr != context, "can not be null");
      SDB_ASSERT(nullptr != _lpb, "can not be null");
      SDB_ASSERT(recordStream.isValid(), "can not be invalid");

      RECORD_SLOT_POS pos = INVALID_RECORD_SLOT_POS;
      UINT16 offset = 0;
      UINT32 sliceSize = recordStream.getRemainingSize();
      UINT32 dataSize = sliceSize + BIG_RECORD_ENTRY_SIZE;
      BOOLEAN ridLocked = FALSE;

      recordSlot rs;
      strictBuffer buffer;
      strictBuffer slotBuf;
      strictBuffer dataBuf;
      recordDataPageHead *head = nullptr;
      recordDataPageHead oldHead;
      bigRecordEntrySlice sliceHead;
      recordID lastRid = recordStream.getLastSliceAddr();
      DPS_TRANS_ID transID = context->getOrigTransId();

      recordID rid;
      const CHAR *slicePtr = nullptr;
      const CHAR *dataPtr = nullptr;

      DPS_LSN_OFFSET lsn = DPS_INVALID_LSN_OFFSET;

      slicePtr = recordStream.reserveSlice(sliceSize);
      if (OSS_UNLIKELY(nullptr == slicePtr))
      {
         rc = SDB_VESSEL_INTERNAL_ERR;
         PD_LOG(PDERROR, "failed to get slice ptr, rc:%d", rc);
         goto error;
      }

      rc = _lpb->autoGetWritableBodyBuffer(buffer);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to get writable buffer, rc:%d", rc);
         goto error;
      }

      head = buffer.getWritableObjPtr<recordDataPageHead>(0);
      if (OSS_UNLIKELY(nullptr == head))
      {
         rc = SDB_VESSEL_INTERNAL_ERR;
         PD_LOG(PDERROR, "failed to get record page head, rc:%d", rc);
         goto error;
      }
      SDB_ASSERT((dataSize + RDP_RSLOT_SIZE) <= head->totalFreeSpace, "impossible");

      // There will not be many invalid slots on the page, 
      // new slot can be inserted directly.
      pos = head->totalSlotCount;
      offset = head->backOffset - dataSize;

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

      slotBuf = buffer.getWritableBuffer(RDP_RSLOT_SIZE, 
                                         RECORD_PAGE_HEAD_SIZE + 
                                         (pos * RDP_RSLOT_SIZE));
      if (OSS_UNLIKELY(!slotBuf.isWritable()))
      {
         rc = SDB_VESSEL_INTERNAL_ERR;
         PD_LOG(PDERROR, "failed to get writable slot buffer[%d], rc:%d", pos, rc);
         goto error;
      }

      dataBuf = buffer.getWritableBuffer(dataSize, offset);
      if (OSS_UNLIKELY(!dataBuf.isWritable()))
      {
         rc = SDB_VESSEL_INTERNAL_ERR;
         PD_LOG(PDERROR, "failed to get writable buffer, rc:%d", rc);
         goto error;
      }
      oldHead = *head;

      // write slot
      rs.init(RDP_RECORD_HEAD_BIG_RECORD_ENTRY, 0, offset, dataSize);
      rs.setInvisible();
      rc = slotBuf.write(0, RDP_RSLOT_SIZE, &rs);
      if (SDB_OK != rc)
      {
         rc = SDB_VESSEL_INTERNAL_ERR;
         PD_LOG(PDERROR, "failed to write slot[%d], rc:%d", pos, rc);
         goto error;
      }

      // write entry head
      sliceHead.setRid(lastRid);
      sliceHead.setTransID(transID);
      sliceHead.sliceCount = recordStream.getSliceCount();
      sliceHead.totalRecordSize = recordStream.getRecordSize();

      rc = dataBuf.write(0, BIG_RECORD_ENTRY_SIZE, &sliceHead);
      if (SDB_OK != rc)
      {
         rc = SDB_VESSEL_INTERNAL_ERR;
         PD_LOG(PDERROR, "failed to write big record entry head, rc:%d", rc);
         goto error;
      }

      // write entry slice
      rc = dataBuf.write(BIG_RECORD_ENTRY_SIZE, 
                         sliceSize, slicePtr);
      if (SDB_OK != rc)
      {
         rc = SDB_VESSEL_INTERNAL_ERR;
         PD_LOG(PDERROR, "failed to write big record entry slice, rc:%d", rc);
         goto error;
      }

      updatePageHeadWhenInsert(pos, rs, transID, context->getStripingId());

      // dummy log
      rc = writeInsertJournal(context, lsn);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to write journal:%d", rc);
         goto error;
      }

      dataPtr = dataBuf.getReadablePtr(0, rs.getMaxSpaceSize());
      if (OSS_UNLIKELY(nullptr == dataPtr))
      {
         rc = SDB_VESSEL_INTERNAL_ERR;
         PD_LOG(PDERROR, "failed to get readable record, rc:%d", rc);
         goto error;
      }

      recordStream.fillLastSlice(rid);
      _lpb->commit(lsn);

   done:
      return rc;
   error:
      if (ridLocked)
      {
         context->unlockRid(rid);
      }
      goto done;
   }

   INT32 rdpAccessor::insertBigRecordBodySlice(dmlContext *context,
                                               bigRecordStream &recordStream)
      {
      INT32 rc = SDB_OK;
      SDB_ASSERT(nullptr != context, "can not be null");
      SDB_ASSERT(nullptr != _lpb, "can not be null");
      SDB_ASSERT(recordStream.isValid(), "can not be invalid");

      RECORD_SLOT_POS pos = INVALID_RECORD_SLOT_POS;
      UINT16 offset = 0;
      UINT32 dataSize = getFreeSpaceAfterLastSlot() - RDP_RSLOT_SIZE;
      UINT32 sliceSize = dataSize - BIG_RECORD_BODY_SLICE_SIZE;

      recordSlot rs;
      strictBuffer buffer;
      strictBuffer slotBuf;
      strictBuffer dataBuf;
      recordDataPageHead *head = nullptr;
      recordDataPageHead oldHead;
      bigRecordBodySlice sliceHead;
      recordID lastRid = recordStream.getLastSliceAddr();

      DPS_LSN_OFFSET lsn = DPS_INVALID_LSN_OFFSET;
      recordID rid;
      const CHAR *slicePtr = nullptr;
      const CHAR *dataPtr = nullptr;

      slicePtr = recordStream.reserveSlice(sliceSize);
      if (OSS_UNLIKELY(nullptr == slicePtr))
      {
         rc = SDB_VESSEL_INTERNAL_ERR;
         PD_LOG(PDERROR, "failed to get slice ptr, rc:%d", rc);
         goto error;
      }

      rc = _lpb->autoGetWritableBodyBuffer(buffer);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to get writable buffer, rc:%d", rc);
         goto error;
      }

      head = buffer.getWritableObjPtr<recordDataPageHead>(0);
      if (OSS_UNLIKELY(nullptr == head))
      {
         rc = SDB_VESSEL_INTERNAL_ERR;
         PD_LOG(PDERROR, "failed to get record page head, rc:%d", rc);
         goto error;
      }

      // There will not be many invalid slots on the page, 
      // new slot can be inserted directly.
      pos = head->totalSlotCount;
      offset = head->backOffset - dataSize;

      slotBuf = buffer.getWritableBuffer(RDP_RSLOT_SIZE, 
                                         RECORD_PAGE_HEAD_SIZE + 
                                         (pos * RDP_RSLOT_SIZE));
      if (OSS_UNLIKELY(!slotBuf.isWritable()))
      {
         rc = SDB_VESSEL_INTERNAL_ERR;
         PD_LOG(PDERROR, "failed to get writable slot buffer[%d], rc:%d", pos, rc);
         goto error;
      }

      dataBuf = buffer.getWritableBuffer(dataSize, offset);
      if (OSS_UNLIKELY(!dataBuf.isWritable()))
      {
         rc = SDB_VESSEL_INTERNAL_ERR;
         PD_LOG(PDERROR, "failed to get writable buffer, rc:%d", rc);
         goto error;
      }
      oldHead = *head;

      // write slot
      rs.init(RDP_RECORD_HEAD_BIG_RECORD_BODY, 0, offset, dataSize);
      rs.setInvisible();
      rc = slotBuf.write(0, RDP_RSLOT_SIZE, &rs);
      if (SDB_OK != rc)
      {
         rc = SDB_VESSEL_INTERNAL_ERR;
         PD_LOG(PDERROR, "failed to write slot[%d], rc:%d", pos, rc);
         goto error;
      }

      // write body head
      sliceHead.setRid(lastRid);
      rc = dataBuf.write(0, BIG_RECORD_BODY_SLICE_SIZE, &sliceHead);
      if (SDB_OK != rc)
      {
         rc = SDB_VESSEL_INTERNAL_ERR;
         PD_LOG(PDERROR, "failed to write big record body head, rc:%d", rc);
         goto error;
      }

      // write slice
      rc = dataBuf.write(BIG_RECORD_BODY_SLICE_SIZE, sliceSize, slicePtr);
      if (SDB_OK != rc)
      {
         rc = SDB_VESSEL_INTERNAL_ERR;
         PD_LOG(PDERROR, "failed to write big record body slice, rc:%d", rc);
         goto error;
      }

      // no need to update transID and stripingID
      updatePageHeadWhenInsert(pos, rs, DPS_TRANS_ID(), dmsStripingId());
      rid.setPid(_lpb->getLogicalPid());
      rid.setPos(pos);

      rc = writeInsertJournal(context, lsn);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to write journal:%d", rc);
         goto error;
      }

      dataPtr = dataBuf.getReadablePtr(0, rs.getMaxSpaceSize());
      if (OSS_UNLIKELY(nullptr == dataPtr))
      {
         rc = SDB_VESSEL_INTERNAL_ERR;
         PD_LOG(PDERROR, "failed to get readable record, rc:%d", rc);
         goto error;
      }

      recordStream.fillLastSlice(rid);
      _lpb->commit(lsn);

   done:
      return rc;
   error:
      goto done;
   }

   INT32 rdpAccessor::updateOverflowedInfo(dmlContext *context,
                                           RECORD_SLOT_POS pos,
                                           const recordID &overflowAddr,
                                           BOOLEAN isBigRecord)
   {
      INT32 rc = SDB_OK;
      strictBuffer buffer;
      const recordDataPageHead *rhead = nullptr;
      recordDataPageHead *whead = nullptr;
      const recordSlot *rs = nullptr;
      overflowedRecord *ofr = nullptr;
      DPS_LSN_OFFSET lsn = DPS_INVALID_LSN_OFFSET;

      if (OSS_UNLIKELY(nullptr == context ||
                       !isValidRecordSlotPosition(pos) ||
                       !overflowAddr.isValid()))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      rhead = getReadablePageHead();
      if (OSS_UNLIKELY(nullptr == rhead))
      {
         rc = SDB_VESSEL_INTERNAL_ERR;
         PD_LOG(PDERROR, "failed to get readable page head, rc:%d", rc);
         goto error;
      }
      else if (rhead->totalSlotCount < pos)
      {
         rc = SDB_OUT_OF_BOUND;
         PD_LOG(PDERROR, "pos is out of page total slot count, rc:%d", rc);
         goto error;
      }

      rs = getReadableSlot(pos);
      if (OSS_UNLIKELY(nullptr == rs))
      {
         rc = SDB_VESSEL_INTERNAL_ERR;
         PD_LOG(PDERROR, "failed to get readable slot[%d], rc:%d", pos, rc);
         goto error;
      }
      else if (!rs->isValidAndVisible() || 
               !rs->isOverflowedRecord())
      {
         rc = SDB_INVALID_OPERATION;
         PD_LOG(PDERROR, "can not update invalid record info, rc:%d", rc);
         goto error;
      }

      rc = _lpb->autoGetWritableBodyBuffer(buffer);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to get writable buffer, rc:%d", rc);
         goto error;
      }

      whead = buffer.getWritableObjPtr<recordDataPageHead>(0);
      if (OSS_UNLIKELY(nullptr == whead))
      {
         rc = SDB_VESSEL_INTERNAL_ERR;
         PD_LOG(PDERROR, "failed to get writable page head, rc:%d", rc);
         goto error;
      }
      
      ofr = buffer.getWritableObjPtr<overflowedRecord>(rs->offset);
      ofr->lpid = overflowAddr.getPid();
      ofr->pos = overflowAddr.getPos();
      if (!isBigRecord)
      {
         ofr->flags = 0;
      }
      else
      {
         ofr->setAsBigRecord();
      }
      
      // dummy log
      rc = writeInplaceUpdateJournal(context, lsn);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to write journal:%d", rc);
         goto error;
      }

      _lpb->commit(lsn);
      context->setDmlLSN(lsn);

   done:
      return rc;
   error:
      goto done;

   }

   INT32 rdpAccessor::destroySlotAndData(dmlContext *context,
                                         RECORD_SLOT_POS pos)
   {
      INT32 rc = SDB_OK;
      strictBuffer buffer;
      strictBuffer recordBuf;
      DPS_LSN_OFFSET lsn = DPS_INVALID_LSN_OFFSET;
      const recordDataPageHead *rhead = nullptr;
      recordDataPageHead *whead = nullptr;
      const recordSlot *rrs = nullptr;
      recordSlot *wrs = nullptr;
      UINT32 deltaSize = 0;

      if (OSS_UNLIKELY(nullptr == context ||
                       !isValidRecordSlotPosition(pos)))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      rhead = getReadablePageHead();
      if (OSS_UNLIKELY(nullptr == rhead))
      {
         rc = SDB_VESSEL_INTERNAL_ERR;
         PD_LOG(PDERROR, "failed to get readable page head, rc:%d", rc);
         goto error;
      }
      else if (rhead->totalSlotCount < pos)
      {
         rc = SDB_OUT_OF_BOUND;
         PD_LOG(PDERROR, "pos is out of page total slot count, rc:%d", rc);
         goto error;
      }

      rrs = getReadableSlot(pos);
      if (OSS_UNLIKELY(nullptr == rrs))
      {
         rc = SDB_VESSEL_INTERNAL_ERR;
         PD_LOG(PDERROR, "failed to get readable slot");
         goto error;
      }
      else if (!rrs->isValid())
      {
         rc = SDB_VESSEL_INTERNAL_ERR;
         PD_LOG(PDERROR, "invalid slot[%d], rc:%d", pos, rc);
         goto error;
      }

      rc = _lpb->autoGetWritableBodyBuffer(buffer);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to get writable buffer, rc:%d", rc);
         goto error;
      }

      whead = buffer.getWritableObjPtr<recordDataPageHead>(0);
      if (OSS_UNLIKELY(nullptr == whead))
      {
         rc = SDB_VESSEL_INTERNAL_ERR;
         PD_LOG(PDERROR, "failed to get writable page head, rc:%d", rc);
         goto error;
      }

      wrs = getWritableSlot(buffer, pos);
      if (OSS_UNLIKELY(nullptr == wrs))
      {
         rc = SDB_VESSEL_INTERNAL_ERR;
         PD_LOG(PDERROR, "failed to get writable slot[%d], rc:%d", pos, rc);
         goto error;
      }
      SDB_ASSERT(wrs->isValid(), "can not be invalid");

      deltaSize = wrs->size;
      recordBuf = buffer.getWritableBuffer(wrs->getMaxSpaceSize(), wrs->offset);
      if (OSS_UNLIKELY(!recordBuf.isWritable()))
      {
         rc = SDB_VESSEL_INTERNAL_ERR;
         PD_LOG(PDERROR, "failed to get writable buffer, rc:%d", rc);
         goto error;
      }
      
      whead->totalFreeSpace += deltaSize;
      ++whead->freeSlotCount;
      if (!wrs->isInvisible() && !wrs->isTombstone())
      {
         SDB_ASSERT(0 < whead->totalRecordCount, "impossible");
         --whead->totalRecordCount;
      }

      recordBuf.setBuffer(0);
      wrs->reset();

      // dummy log
      rc = writeRemoveJournal(context, lsn);
      if (SDB_OK != rc)
      {
         ///TODO: rollback
         PD_LOG(PDERROR, "failed to write journal:%d", rc);
         goto error;
      }
      _lpb->commit(lsn);
      context->setDmlLSN(lsn);
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
      SDB_ASSERT(nullptr != context, "can not be null");
      SDB_ASSERT(isValidRecordSlotPosition(pos), "can not be invalid");
      SDB_ASSERT(row.isValid(), "can not be invalid");

      strictBuffer buffer;
      recordSlot *rs = nullptr;
      normalRecordHead *rh = nullptr;
      strictBuffer recordBuffer;
      recordDataPageHead *head = nullptr;
      DPS_LSN_OFFSET lsn = DPS_INVALID_LSN_OFFSET;
      DPS_TRANS_ID transID = context->getOrigTransId();
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
      if (OSS_UNLIKELY(nullptr == rs))
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
         rc = SDB_INVALID_OPERATION;
         goto error;
      }

      recordBuffer = buffer.getWritableBuffer(rs->getMaxSpaceSize(), rs->offset);
      rh = recordBuffer.getWritableObjPtr<normalRecordHead>(0);
      if (OSS_UNLIKELY(nullptr == rh))
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
         UINT32 reservedSize = (UINT32)(rs->reservedSpaceSize) + deltaSize;
         rs->size -= deltaSize;
         rs->reservedSpaceSize = (recordSlot::getMaxReservedSize() < reservedSize ) ?
                                  recordSlot::getMaxReservedSize() : reservedSize;
         head->totalFreeSpace += deltaSize;
      }
      else if (totalSize > (UINT32)(rs->size))
      {
         UINT32 delta =  totalSize - (UINT32)(rs->size);
         rs->size += delta;
         rs->reservedSpaceSize -= delta;
         head->totalFreeSpace -= delta;
      }

      if (!rs->isInvisible())
      {
         updateStripingInfo(head, striping);
         updateMaxTransSN(head, transID.getSN());
      }

      ///dummy log
      rc = writeInplaceUpdateJournal(context, lsn);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to write journal:%d", rc);
         goto error;
      }
      _lpb->commit(lsn);
      context->setDmlLSN(lsn);
   done:
      return rc;
   error:
      goto done;
   }

   INT32 rdpAccessor::updateByResaving(dmlContext *context,
                                       RECORD_SLOT_POS pos,
                                       const dmsStripingId &striping,
                                       const slice &row)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(nullptr != context, "can not be null");
      SDB_ASSERT(isValidRecordSlotPosition(pos), "can not be invalid");
      SDB_ASSERT(row.isValid(), "can not be invalid");

      strictBuffer buffer;
      CHAR* oldRecordPtr = nullptr;
      strictBuffer newRecordBuffer;
      recordSlot *rs = nullptr;
      normalRecordHead rh;
      recordDataPageHead *head = nullptr;
      UINT16 offset = 0;
      UINT16 deltaSize = 0;

      DPS_LSN_OFFSET lsn = DPS_INVALID_LSN_OFFSET;
      DPS_TRANS_ID transID = context->getOrigTransId();
      UINT32 totalSize = row.getSize() + NORMAL_RECORD_HEAD_SIZE;
      SDB_ASSERT(totalSize <= getFreeSpaceAfterLastSlot(), "not enough free space");

      rc = _lpb->autoGetWritableBodyBuffer(buffer);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to prepare to write");
         goto error;
      }

      head = buffer.getWritableObjPtr<recordDataPageHead>(0);
      if (OSS_UNLIKELY(nullptr == head))
      {
         PD_LOG(PDERROR, "failed to get page head");
         rc = SDB_VESSEL_INTERNAL_ERR;
         goto error;
      }

      rs = getWritableSlot(buffer, pos);
      if (OSS_UNLIKELY(nullptr == rs))
      {
         PD_LOG(PDERROR, "failed to get writable slot[%d]", pos);
         rc = SDB_VESSEL_INTERNAL_ERR;
         goto error;
      }
      SDB_ASSERT(rs->isValid() && rs->isNormalRecord(), "impossible");

      rh = *(buffer.getReadableObjPtr<normalRecordHead>(rs->offset));
      // reset old record
      oldRecordPtr = buffer.getWritablePtr(rs->offset, rs->getMaxSpaceSize());
      if (OSS_UNLIKELY(nullptr == oldRecordPtr))
      {
         rc = SDB_VESSEL_INTERNAL_ERR;
         PD_LOG(PDERROR, "failed to get writable old record");
         goto error;
      }
      ossMemset(oldRecordPtr, 0, rs->getMaxSpaceSize());

      // save new record
      offset = (UINT16)(head->backOffset - totalSize);
      newRecordBuffer = buffer.getWritableBuffer(totalSize, (UINT32)offset);
      if (OSS_UNLIKELY(!newRecordBuffer.isWritable()))
      {
         rc = SDB_VESSEL_INTERNAL_ERR;
         PD_LOG(PDERROR, "failed to get writable buffer, rc:%d", rc);
         goto error;
      }
      rh.setTransID(transID);

      newRecordBuffer.write(0, NORMAL_RECORD_HEAD_SIZE, &rh);
      newRecordBuffer.write(NORMAL_RECORD_HEAD_SIZE, row.getSize(), row.getData());

      // update slot and page head
      SDB_ASSERT(totalSize > rs->size, "impossible");
      deltaSize = (UINT16)(totalSize - rs->size);
      rs->offset = offset;
      rs->reservedSpaceSize = 0;
      rs->size = totalSize;

      SDB_ASSERT(deltaSize <= head->totalFreeSpace, "impossible");
      head->totalFreeSpace -= deltaSize;
      head->backOffset = offset;
      if (!rs->isInvisible())
      {
         updateStripingInfo(head, striping);
         updateMaxTransSN(head, transID.getSN());
      }

      ///dummy log
      rc = writeInplaceUpdateJournal(context, lsn);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to write journal:%d", rc);
         goto error;
      }

      _lpb->commit(lsn);
      context->setDmlLSN(lsn);

   done:
      return rc;
   error:
      goto done;
   }

   INT32 rdpAccessor::updateByCompaction(dmlContext *context,
                                         RECORD_SLOT_POS pos,
                                         const dmsStripingId &striping,
                                         const slice &row)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(nullptr != context, "can not be null");
      SDB_ASSERT(isValidRecordSlotPosition(pos), "can not be invalid");
      SDB_ASSERT(row.isValid(), "can not be invalid");

      strictBuffer buffer; 
      const recordDataPageHead *rhead = nullptr;
      recordDataPageHead *whead = nullptr;
      const recordSlot *rs = nullptr;
      normalRecordHead rh;

      DPS_LSN_OFFSET lsn = DPS_INVALID_LSN_OFFSET;
      DPS_TRANS_ID transID = context->getOrigTransId();
      rdpCompactor compactor;
      BOOLEAN isInvisible = FALSE;

      // compactor only needs slots and data
      UINT32 compactBufSize = getPageBodySize(_lpb->getPageSize())
                              - RECORD_PAGE_HEAD_SIZE;
      CHAR *compactBuf = context->allocateBuffer(compactBufSize);
      if (OSS_UNLIKELY(nullptr == compactBuf))
      {
         rc = SDB_OOM;
         PD_LOG(PDERROR, "failed to allocate compact buffer, rc:%d", rc);
         goto error;
      }
      compactor.reset(compactBuf, compactBufSize);

      buffer = _lpb->getReadableBodyBuffer();
      rhead = buffer.getReadableObjPtr<recordDataPageHead>(0);
      if (OSS_UNLIKELY(nullptr == rhead))
      {
         PD_LOG(PDERROR, "failed to get page head");
         rc = SDB_VESSEL_INTERNAL_ERR;
         goto error;
      }

      rs = getReadableSlot(pos);
      if (OSS_UNLIKELY(nullptr == rs))
      {
         rc = SDB_VESSEL_INTERNAL_ERR;
         PD_LOG(PDERROR, "failed to get readable slot");
         goto error;
      }

      SDB_ASSERT(rs->isValid() && rs->isNormalRecord(), "impossible");
      SDB_ASSERT(row.getSize() + NORMAL_RECORD_HEAD_SIZE - rs->size <= 
                 rhead->totalFreeSpace, "not enough free space");
      isInvisible = rs->isInvisible();

      for (UINT16 i = 0; i < rhead->totalSlotCount; ++i)
      {
         const recordSlot *tmpRS = getReadableSlot(i);
         if (OSS_UNLIKELY(nullptr == tmpRS))
         {
            rc = SDB_VESSEL_INTERNAL_ERR;
            PD_LOG(PDERROR, "failed to get readable slot[%d]", i);
            goto error;
         }

         if (!tmpRS->isValid())
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
            normalRecordHead compactRH = *(buffer.getReadableObjPtr<normalRecordHead>(tmpRS->offset));
            compactRH.setTransID(transID);
            rc = compactor.push(tmpRS->flags, tmpRS->type, 
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
            rc = compactor.push(tmpRS->flags, tmpRS->type, buffer.getSlice(tmpRS->offset, tmpRS->size));
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
      if (OSS_UNLIKELY(nullptr == whead))
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

      if (!isInvisible)
      {
         updateStripingInfo(whead, striping);
         updateMaxTransSN(whead, transID.getSN());
      }

      ///dummy log
      rc = writeInplaceUpdateJournal(context, lsn);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to write journal:%d", rc);
         goto error;
      }

      _lpb->commit(lsn);
      context->setDmlLSN(lsn);

   done:
      if (nullptr != compactBuf)
      {
         context->releaseBuffer(compactBuf);
      }
      return rc;
   error:
      goto done;

   }

   INT32 rdpAccessor::validatePage(requestContext *context,
                                   logicalPageBuffer *lpb)const
   {
      SDB_ASSERT(nullptr != context && context->isClPropertiesSet(), "can not be invalid");
      SDB_ASSERT(nullptr != lpb && lpb->isValid(), "can not be invalid");
      const recordDataPageHead *head = nullptr;
      UINT32 lid =  context->getLogicalClId();
      INT32 rc = lpb->validatePage(PAGE_TYPE_RECORD);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to validate page[%s], rc:%d",
                lpb->getGlobalPid().toString().c_str(), rc);
         goto error;
      }

      head = lpb->getReadableBodyBuffer().getReadableObjPtr<recordDataPageHead>(0);
      if (nullptr == head)
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
      SDB_ASSERT(nullptr != _lpb && _lpb->isValid(), "can not be invalid");
      SDB_ASSERT(isValidRecordSlotPosition(pos), "can not be invalid");
      SDB_ASSERT((UINT32)pos < getTotalSlotCount(), "out of bound");
      strictBuffer buffer = _lpb->getReadableBodyBuffer();
      return buffer.getReadableObjPtr<recordSlot>(RECORD_PAGE_HEAD_SIZE +
                                                  (RDP_RSLOT_SIZE * pos));
   }

   const recordDataPageHead *rdpAccessor::getReadablePageHead()const
   {
      SDB_ASSERT(nullptr != _lpb && _lpb->isValid(), "can not be invalid");
      return _lpb->getReadableBodyBuffer().getReadableObjPtr<recordDataPageHead>(0);
   }

   UINT32 rdpAccessor::getFrontOffset(const recordDataPageHead *head)const
   {
      SDB_ASSERT(nullptr != head, "can not be null");
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

      const recordDataPageHead *head = nullptr;
      count = 0;

      if (OSS_UNLIKELY(nullptr == _lpb))
      {
         rc = SDB_VESSEL_RECORD_NOT_FOUND;
         goto error;
      }

      head = getReadablePageHead();
      if (OSS_UNLIKELY(nullptr == head))
      {
         PD_LOG(PDERROR, "failed to get readable page header");
         rc = SDB_VESSEL_INTERNAL_ERR;
         goto error;
      }

      count = head->totalRecordCount;
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
      else if (OSS_UNLIKELY(nullptr == _lpb))
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
      const normalRecordHead *header = nullptr;

      rh = normalRecordHead();
      data.reset();

      if (OSS_UNLIKELY(INVALID_RECORD_SLOT_POS == pos))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      else if (OSS_UNLIKELY(nullptr == _lpb))
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
         rc = SDB_INVALID_OPERATION;
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
      if (OSS_UNLIKELY(nullptr == header))
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

   INT32 rdpAccessor::getOverflowedRecord(RECORD_SLOT_POS pos,
                                          overflowedRecord &ofr)const
   {
      INT32 rc = SDB_OK;
      recordSlot rs;
      strictBuffer buffer;
      strictBuffer recordBuffer;
      const overflowedRecord *recordPtr = nullptr;

      ofr = overflowedRecord();
      if (OSS_UNLIKELY(INVALID_RECORD_SLOT_POS == pos))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      else if (OSS_UNLIKELY(nullptr == _lpb))
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }

      rc = getSlot(pos, rs);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to get slot, rc:%d", rc);
         goto error;
      }

      if(!rs.isValidAndVisible() || !rs.isOverflowedRecord())
      {
         rc = SDB_INVALID_OPERATION;
         PD_LOG(PDERROR, "slot type is not overflowed, rc:%d", rc);
         goto error;
      }

      buffer = _lpb->getReadableBodyBuffer();
      if (OSS_UNLIKELY(!buffer.isValid()))
      {
         rc = SDB_VESSEL_INTERNAL_ERR;
         PD_LOG(PDERROR, "failed to get page body buffer, rc:%d", rc);
         goto error;
      }

      recordBuffer = buffer.getReadableBuffer(rs.size, rs.offset);
      if (OSS_UNLIKELY(!recordBuffer.isValid()))
      {
         rc = SDB_VESSEL_INTERNAL_ERR;
         PD_LOG(PDERROR, "failed to get record buffer[%d,%d]", rs.size, rs.offset);
         goto error;
      }

      recordPtr = recordBuffer.getReadableObjPtr<overflowedRecord>(0);
      if (OSS_UNLIKELY(nullptr == recordPtr))
      {
         rc = SDB_VESSEL_INTERNAL_ERR;
         PD_LOG(PDERROR, "failed to get record header[%d,%d]", rs.offset, rs.size);
         goto error;
      }

      ofr = *recordPtr;

   done:
      return rc;
   error:
      goto done;
   }

   INT32 rdpAccessor::getBigRecordEntrySlice(RECORD_SLOT_POS pos,
                                             bigRecordEntrySlice &entry,
                                             slice &data)const
   {
      INT32 rc = SDB_OK;
      recordSlot rs;
      strictBuffer buffer;
      strictBuffer sliceBuffer;
      const bigRecordEntrySlice *entryPtr = nullptr;
      const CHAR *dataPtr = nullptr;

      entry = bigRecordEntrySlice();
      data.reset();
      if (OSS_UNLIKELY(INVALID_RECORD_SLOT_POS == pos))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      else if (OSS_UNLIKELY(nullptr == _lpb))
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }

      rc = getSlot(pos, rs);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to get slot, rc:%d", rc);
         goto error;
      }

      if (!rs.isValid() || !rs.isBigRecordEntry())
      {
         rc = SDB_INVALID_OPERATION;
         PD_LOG(PDERROR, "slot type is not big record, rc:%d", rc);
         goto error;
      }

      buffer = _lpb->getReadableBodyBuffer();
      if (OSS_UNLIKELY(!buffer.isValid()))
      {
         rc = SDB_VESSEL_INTERNAL_ERR;
         PD_LOG(PDERROR, "failed to get page body buffer, rc:%d", rc);
         goto error;
      }

      sliceBuffer = buffer.getReadableBuffer(rs.size, rs.offset);
      if (OSS_UNLIKELY(!sliceBuffer.isValid()))
      {
         rc = SDB_VESSEL_INTERNAL_ERR;
         PD_LOG(PDERROR, "failed to get record buffer[%d,%d]", rs.size, rs.offset);
         goto error;
      }

      entryPtr = sliceBuffer.getReadableObjPtr<bigRecordEntrySlice>(0);
      if (OSS_UNLIKELY(nullptr == entryPtr))
      {
         rc = SDB_VESSEL_INTERNAL_ERR;
         PD_LOG(PDERROR, "failed to get big record entry head, rc:%d", rc);
         goto error;
      }

      dataPtr = sliceBuffer.getReadablePtr(BIG_RECORD_ENTRY_SIZE, 
                                           rs.size - BIG_RECORD_ENTRY_SIZE);
      if (OSS_UNLIKELY(nullptr == dataPtr))
      {
         rc = SDB_VESSEL_INTERNAL_ERR;
         PD_LOG(PDERROR, "failed to get big record entry data, rc:%d", rc);
         goto error;
      }

      entry = *entryPtr;
      data.reset(rs.size - BIG_RECORD_ENTRY_SIZE, dataPtr);

   done:
      return rc;
   error:
      goto done;
   }

   INT32 rdpAccessor::getBigRecordBodySlice(RECORD_SLOT_POS pos,
                                            bigRecordBodySlice &body,
                                            slice &data)const
   {
      INT32 rc = SDB_OK;
      recordSlot rs;
      strictBuffer buffer;
      strictBuffer sliceBuffer;
      const bigRecordBodySlice *bodyPtr = nullptr;
      const CHAR *dataPtr = nullptr;

      body = bigRecordBodySlice();
      data.reset();
      if (OSS_UNLIKELY(INVALID_RECORD_SLOT_POS == pos))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      else if (OSS_UNLIKELY(nullptr == _lpb))
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }

      rc = getSlot(pos, rs);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to get slot, rc:%d", rc);
         goto error;
      }

      if (!rs.isValid() || !rs.isBigRecordBody() || !rs.isInvisible())
      {
         rc = SDB_INVALID_OPERATION;
         goto error;
      }

      buffer = _lpb->getReadableBodyBuffer();
      if (OSS_UNLIKELY(!buffer.isValid()))
      {
         rc = SDB_VESSEL_INTERNAL_ERR;
         PD_LOG(PDERROR, "failed to get page body buffer, rc:%d", rc);
         goto error;
      }

      sliceBuffer = buffer.getReadableBuffer(rs.size, rs.offset);
      if (OSS_UNLIKELY(!sliceBuffer.isValid()))
      {
         rc = SDB_VESSEL_INTERNAL_ERR;
         PD_LOG(PDERROR, "failed to get record buffer[%d,%d]", rs.size, rs.offset);
         goto error;
      }

      bodyPtr = sliceBuffer.getReadableObjPtr<bigRecordBodySlice>(0);
      if (OSS_UNLIKELY(nullptr == bodyPtr))
      {
         rc = SDB_VESSEL_INTERNAL_ERR;
         PD_LOG(PDERROR, "failed to get big record body head, rc:%d", rc);
         goto error;
      }

      dataPtr = sliceBuffer.getReadablePtr(BIG_RECORD_BODY_SLICE_SIZE, 
                                           rs.size - BIG_RECORD_BODY_SLICE_SIZE);
      if (OSS_UNLIKELY(nullptr == dataPtr))
      {
         rc = SDB_VESSEL_INTERNAL_ERR;
         PD_LOG(PDERROR, "failed to get big record body data, rc:%d", rc);
         goto error;
      }

      body = *bodyPtr;
      data.reset(rs.size - BIG_RECORD_BODY_SLICE_SIZE, dataPtr);

   done:
      return rc;
   error:
      goto done;
   }

   INT32 rdpAccessor::getNextSliceAddrOfBigRecord(RECORD_SLOT_POS pos,
                                                  recordID &nextAddr)
   {
      INT32 rc = SDB_OK;
      recordSlot rs;
      strictBuffer buffer;

      nextAddr.reset();
      if (OSS_UNLIKELY(INVALID_RECORD_SLOT_POS == pos))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      else if (OSS_UNLIKELY(nullptr == _lpb))
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }

      buffer = _lpb->getReadableBodyBuffer();
      if (OSS_UNLIKELY(!buffer.isValid()))
      {
         rc = SDB_VESSEL_INTERNAL_ERR;
         PD_LOG(PDERROR, "failed to get readable buffer, rc:%d", rc);
         goto error;
      }

      rc = getSlot(pos, rs);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to get slot[%d], rc:%d", pos, rc);
         goto error;
      }
      else if (!rs.isValid() || !rs.isInvisible())
      {
         rc = SDB_VESSEL_INTERNAL_ERR;
         PD_LOG(PDERROR, "invalid slot in this pos[%d], rc:%d", pos, rc);
         goto error;
      }

      if (RDP_RECORD_HEAD_BIG_RECORD_ENTRY == rs.type)
      {
         const bigRecordEntrySlice *entry = 
               buffer.getReadableObjPtr<bigRecordEntrySlice>(rs.offset);
         if (OSS_UNLIKELY(nullptr == entry))
         {
            rc = SDB_VESSEL_INTERNAL_ERR;
            PD_LOG(PDERROR, "failed to get readable entry slice, rc:%d", rc);
            goto error;
         }
         nextAddr = recordID(entry->nextPage, entry->nextPos);

      }
      else if(RDP_RECORD_HEAD_BIG_RECORD_BODY == rs.type)
      {
         const bigRecordBodySlice *body = 
               buffer.getReadableObjPtr<bigRecordBodySlice>(rs.offset);
         if (OSS_UNLIKELY(nullptr == body))
         {
            rc = SDB_VESSEL_INTERNAL_ERR;
            PD_LOG(PDERROR, "failed to get readable entry slice, rc:%d", rc);
            goto error;
         }
         nextAddr = recordID(body->nextPage, body->nextPos);
      }
      else
      {
         rc = SDB_INVALID_OPERATION;
         PD_LOG(PDERROR, "not a big record slice, rc:%d", rc);
         goto error;
      }

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

   INT32 rdpAccessor::deleteRecord(dmlContext *context,
                                   RECORD_SLOT_POS pos)
   {
      INT32 rc = SDB_OK;
      recordSlot rs;

      if (OSS_UNLIKELY(nullptr == context ||
                       !context->isClPropertiesSet() ||
                       INVALID_RECORD_SLOT_POS == pos))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      else if (OSS_UNLIKELY(nullptr == _lpb))
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
         PD_LOG(PDERROR, "failed to get slot[%d]", pos);
         rc = SDB_VESSEL_INTERNAL_ERR;
         goto error;
      }

      if (!rs.isValidAndVisible())
      {
         PD_LOG(PDERROR, "pos[%d] not valid to be delete", pos);
         rc = SDB_VESSEL_RECORD_NOT_FOUND;
         goto error;
      }
      else if (rs.isTombstone())
      {
         PD_LOG(PDERROR, "pos[%d] already been tombstone", pos);
         rc = SDB_INVALID_OPERATION;
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
      SDB_ASSERT(nullptr != context, "can not be null");
      SDB_ASSERT(nullptr != _lpb, "can not be null");

      strictBuffer buffer, recordBuffer;
      recordSlot *rs = nullptr;
      recordDataPageHead *head = nullptr;
      DPS_LSN_OFFSET lsn = DPS_INVALID_LSN_OFFSET;
      DPS_TRANS_ID transID = context->getOrigTransId();
      UINT32 size = 0;
      normalRecordHead *rh = nullptr;
      
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
      if (OSS_UNLIKELY(nullptr == rs))
      {
         PD_LOG(PDERROR, "failed to get writable slot[%d]", pos);
         rc = SDB_VESSEL_INTERNAL_ERR;
         goto error;
      }

      SDB_ASSERT(rs->isValidAndVisible() && !rs->isTombstone(), "impossible");

      recordBuffer = buffer.getWritableBuffer(rs->size, rs->offset);
      if (!recordBuffer.isWritable())
      {
         PD_LOG(PDERROR, "failed to get writable buffer[%d,%d]",
                rs->size, rs->offset);
         rc = SDB_VESSEL_INTERNAL_ERR;
         goto error;
      }

      rh = recordBuffer.getWritableObjPtr<normalRecordHead>(0);
      rh->flags = 0;
      rh->setTransID(transID);

      size = rs->size - NORMAL_RECORD_HEAD_SIZE;
      rs->setTombstone();
      rs->size = NORMAL_RECORD_HEAD_SIZE;
      rs->reservedSpaceSize = 0;
      rs->type = RDP_RECORD_HEAD_TYPE_NORMAL;

      head->totalFreeSpace += size;
      SDB_ASSERT(0 < head->totalRecordCount, "impossible");
      --head->totalRecordCount;
      updateMaxTransSN(head, transID.getSN());

      rc = writeRemoveJournal(context, lsn);
      if (SDB_OK != rc)
      {
         ///TODO: rollback
         PD_LOG(PDERROR, "failed to write journal:%d", rc);
         goto error;
      }

      _lpb->commit(lsn);
      context->setDmlLSN(lsn);
      context->setDmlRecordInfo(head->pageSeq, recordID(_lpb->getLogicalPid(), pos));
      
   done:
      return rc;
   error:
      goto done;
   }

   INT32 rdpAccessor::writeInsertJournal(dmlContext *context,
                                         DPS_LSN_OFFSET &lsn)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(nullptr != context, "can not be null");
      IDataJournal *journal = context->getOuterResource()->journal;
      dpsWriteReqBuilder jpad;
      dpsWriteRequest jrequest;
      dpsLogRecordHeader jres;

      jpad.setType(LOG_TYPE_VESSEL_RDP_INSERT);

      jrequest = jpad.reap();
      rc = journal->write(context->getExecutor(), jrequest, dpsWriteOptions(), &jres);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to write journal:%d", rc);
         goto error;
      }
      lsn = jres._lsn;
   done:
      return rc;
   error:
      goto done;
   }

   INT32 rdpAccessor::writeInplaceUpdateJournal(dmlContext *context,
                                                DPS_LSN_OFFSET &lsn)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(nullptr != context, "can not be null");
      IDataJournal *journal = context->getOuterResource()->journal;
      dpsWriteReqBuilder jpad;
      dpsWriteRequest jrequest;
      dpsLogRecordHeader jres;

      jpad.setType(LOG_TYPE_DATA_UPDATE);

      jrequest = jpad.reap();
      rc = journal->write(context->getExecutor(), jrequest, dpsWriteOptions(), &jres);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to write journal:%d", rc);
         goto error;
      }
      lsn = jres._lsn;
   done:
      return rc;
   error:
      goto done;
   }

   INT32 rdpAccessor::writeRemoveJournal(dmlContext *context,
                                         DPS_LSN_OFFSET &lsn)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(nullptr != context, "can not be null");
      IDataJournal *journal = context->getOuterResource()->journal;
      dpsWriteReqBuilder jpad;
      dpsWriteRequest jrequest;
      dpsLogRecordHeader jres;

      jpad.setType(LOG_TYPE_DATA_DELETE);

      jrequest = jpad.reap();
      rc = journal->write(context->getExecutor(), jrequest, dpsWriteOptions(), &jres);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to write journal:%d", rc);
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