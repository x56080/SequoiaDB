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

   When/how to use: this program may be used on binary and text-formatted
   versions of PMD component. This file contains functions for agent processing.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#include "vessel/rdpAccessor.h"
#include "vessel/recordDataPage.h"
#include "utilCompression.hpp"
#include "pdTrace.hpp"
#include "ossLikely.hpp"
#include "vessel/IRedoLogger.h"
#include "vessel/logRecordContext.h"
#include "vessel/outerResource.h"
#include "dpsLogRecordDef.hpp"
#include "vessel/insertContext.h"

namespace engine
{
namespace vessel
{
   INT32 rdpAccessor::initRdp(requestContext *context,
                              PAGE_ID lpid,
                              UINT32 logicalID,
                              UINT32 sequence)
   {
      INT32 rc = SDB_OK;

      recordDataPageHead *head = NULL;
      UINT32 flags = PAGE_ACCESSOR_FLAG_INIT_PAGE | 
                     PAGE_ACCESSOR_FLAG_DIRECT |
                     PAGE_ACCESSOR_FLAG_NON_READONLY;
      SDB_ASSERT(OSS_BIT_TEST(getFlags(), flags), "impossible");

      if (OSS_UNLIKELY(INVALID_PAGE_ID == lpid ||
                       DMS_INVALID_LOGICCLID == logicalID))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      rc = prepareToWrite(context);
      if (SDB_OK != rc)
      {
         goto error;
      }

      rc = initCommonPageHeadAndTail(lpid);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to init common head:%d", rc);
         goto error;
      }

      rc = memsetPageBody(0);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to memset page body:%d", rc);
         goto error;
      }

      rc = getWritableUserHeadPtr<recordDataPageHead>(&head);
      if (SDB_OK != rc)
      {
         goto error;
      }

      head->version = RDP_VERSION;
      head->totalSlotCount = 0;
      head->freeSlotCount = 0;
      head->compressionDicSlot = INVALID_RECORD_SLOT_ID;
      head->clLogcalID = logicalID;
      head->sequenceID = sequence;
      head->totalFreeSpace = getPageBodySize() - RECORD_PAGE_HEAD_LEN;
      head->freeSpaceAfterLastSlot = head->totalFreeSpace;
      head->flags = 0;
      head->minStriping = INVALID_STRIPING_ID;
      head->maxStriping = INVALID_STRIPING_ID;
      head->transSN = 0;
      head->pad0 = 0;
      head->pad1 = 0;

      pageAccessor::commit(context, DPS_INVALID_LSN_OFFSET);
   done:
      return rc;
   error:
      if (fullAccessing())
      {
         abortToWrite();
      }
      goto done;
   }

   INT32 rdpAccessor::insert(insertContext *context)
   {
      INT32 rc = SDB_OK;
      RECORD_SLOT_ID slotID = INVALID_RECORD_SLOT_ID;
      BOOLEAN needReorg = FALSE;

      if (OSS_UNLIKELY(NULL == context))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      else if (!context->clInfoIsValid())
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      else if (!context->getRecord().isValid())
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      rc = validatePage(context->getLogicalID());
      if (SDB_OK != rc)
      {
         goto error;
      }

      if (context->getCompressionType() == UTIL_COMPRESSOR_INVALID)
      {
         rc = insertWithOutCompression(context);
         if (SDB_OK != rc)
         {
            goto error;
         }
      }

   done:
      return rc;
   error:
      goto done;
   }

   INT32 rdpAccessor::validatePage(UINT32 logicalID)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(DMS_INVALID_LOGICCLID != logicalID, "can not be invalid");
      const recordDataPageHead *head = NULL;
      rc = getReadableUserHeadPtr<recordDataPageHead>(&head);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to get rdp head:%d", rc);
         goto error;
      }
      if (INVALID_RDP_VERSION == head->version)
      {
         PD_LOG(PDERROR, "invalid record data page version");
         rc = SDB_VESSEL_PAGE_CRASHED;
         goto error;
      }
      else if (logicalID != head->clLogcalID)
      {
         PD_LOG(PDERROR, "target logical id[%d], logical id in head[%d]",
                logicalID, head->clLogcalID);
         rc = SDB_VESSEL_PAGE_CRASHED;
         goto error;
      }
      else if (getPageBodySize() < (RDP_RSLOT_SIZE * head->totalSlotCount))
      {
         PD_LOG(PDERROR, "invalid slot count:%d", head->totalSlotCount);
         rc = SDB_VESSEL_PAGE_CRASHED;
         goto error;
      }
      else
      {
         UINT32 size = RDP_RECORD_HEAD_LEN +
                       (RDP_RSLOT_SIZE * head->totalSlotCount) +
                       head->freeSpaceAfterLastSlot;
         if (getPageBodySize() < size)
         {
            PD_LOG(PDERROR, "invalid slot and freeSpaceAfterLastSlot");
            rc = SDB_VESSEL_PAGE_CRASHED;
            goto error;
         } 
      }
   done:
      return rc;
   error:
      PD_LOG(PDERROR, "page[%s] might crashed", getGPID().toString().c_str());
      goto done;
   }

   INT32 rdpAccessor::insertWithOutCompression(insertContext *context)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(NULL != context, "can not be null");
      SDB_ASSERT(DMS_INVALID_LOGICCLID != context->getLogicalID(), "can not be invalid");
      SDB_ASSERT(context->clInfoIsValid(), "can not be empty");
      SDB_ASSERT(UTIL_COMPRESSOR_INVALID == context->getCompressionType(), "must be invalid");

      const recordData &record = context->getRecord();
      recordDataPageHead backupHead;
      const recordDataPageHead *head = NULL;
      recordDataPageHead *wHead = NULL;
      BOOLEAN needReorg = FALSE;
      RECORD_SLOT_ID slotID = INVALID_RECORD_SLOT_ID;
      recordHead *recordHeadPtr = NULL;
      CHAR *recordPtr = NULL;
      recordSlot slot;
      UINT32 alignedSize = 0;
      UINT32 offset = 0;
      UINT32 sizeNeeded = 0;
      PAGE_ID lpid = INVALID_PAGE_ID;
      recordID rid;
      logRecordContext lrc;
      DPS_LSN_OFFSET lsn = DPS_INVALID_LSN_OFFSET;
      BOOLEAN rollback = FALSE;
      
      rc = getPidFromDisk(lpid);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to get lpid from disk:%d", rc);
         goto error;
      }

      rc = getReadableUserHeadPtr<recordDataPageHead>(&head);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to get rdp head:%d", rc);
         goto error;
      }

      alignedSize = getAlignedSizeOfNormalRecordAndHead(record.getSlice().len());
      sizeNeeded = alignedSize;
      if (0 == head->freeSlotCount)
      {
         sizeNeeded += RDP_RSLOT_SIZE;
      }
      if (!hasSpaceToInsert(head, sizeNeeded, needReorg))
      {
         rc = SDB_VESSEL_NOT_ENOUGH_SPACE_IN_PAGE;
         goto error;
      }
      else if (needReorg)
      {
         SDB_ASSERT(FALSE, "todo");
      }

      if (0 < head->freeSlotCount)
      {
         rc = findFreeSlot(head, slotID);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to find slot id:%d", rc);
            goto error;
         }
      }

      rc = prepareInsertWOCLog(context, &lrc, alignedSize);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to prepare log:%d", rc);
         goto error;
      }

      rc = prepareToWrite(context);
      if (SDB_OK != rc)
      {
         goto error;
      }

      /// backup page head before any writing.
      backupHead = *head;

      rc = getWritableUserHeadPtr<recordDataPageHead>(&wHead);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to get writable head:%d", rc);
         goto error;
      }

      rollback = TRUE;
      /// 1. update page head.
      updateMinMaxStriping(wHead, context->getStriping());
      if (INVALID_RECORD_SLOT_ID == slotID)
      {
         slotID = wHead->totalSlotCount++;
      }
      else
      {
         --wHead->freeSlotCount;
      }
      wHead->freeSpaceAfterLastSlot -= sizeNeeded;
      wHead->totalFreeSpace -= sizeNeeded;

      /// 2. copy record head and record
      offset = getNonFreeBeginOffet(wHead);
      rc = getWritePtrOfPageBody<recordHead>(offset, &recordHeadPtr);
      {
         PD_LOG(PDERROR, "failed to get writable record head:%d", rc);
         goto error;
      }

      rc = getWritePtrOfPageBody(offset + RDP_RECORD_HEAD_LEN,
                                 alignedSize - RDP_RECORD_HEAD_LEN,
                                 &recordPtr);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to get writable ptr:%d", rc);
         goto error;
      }

      recordHeadPtr->size = alignedSize;
      recordHeadPtr->setTypeAndFormat(RDP_R_HEAD_TYPE_NORMAL, record.getType());
      recordHeadPtr->flags = 0;
      recordHeadPtr->compressionType = context->getCompressionType();
      recordHeadPtr->transNode = context->getTransID().getNodeID();
      recordHeadPtr->transSN = context->getTransID().getSN();
      recordHeadPtr->pad = 0;
      ossMemcpy(recordPtr, record.getSlice().data(), record.getSlice().len());

      /// 3. write slot
      slot.setOffset(offset);
      slot.setType(RDP_R_HEAD_TYPE_NORMAL);

      rc = writeSlot(slotID, slot);
      if (SDB_OK != rc)
      {
         goto error;
      }

      /// 4. commit log
      rid.setPageID(lpid);
      rid.setSlotID(slotID);
      rc = commitInsertWOCLog(context, &lrc, rid, wHead, slot, recordHeadPtr);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to commit log:%d", rc);
         goto error;
      }
      
      pageAccessor::commit(context, lrc.getLsn());
      context->setRid(rid);
      context->setLsn(lrc.getLsn());
      rollback = FALSE;
      lrc.close();
   done:
      return rc;
   error:
      if (lrc.prepared())
      {
         IRedoLogger *logger = context->getOuterResource()->logger;
         logger->abort(context->getSession(), &lrc);
      }
      if (rollback)
      {
         if (NULL != recordHeadPtr)
         {
            ossMemset(recordHeadPtr, 0, alignedSize);
         }
         if (INVALID_RECORD_SLOT_ID != slotID)
         {
            writeSlot(slotID, recordSlot());
         }
         *wHead = backupHead;
      }
      if (fullAccessing())
      {
         pageAccessor::abortToWrite();
      }
      goto done;
   }

   BOOLEAN rdpAccessor::hasSpaceToInsert(const recordDataPageHead *head,
                                         UINT32 sizeNeeded,
                                         BOOLEAN &needReorg)
   {
      SDB_ASSERT(NULL != head, "can not be null");
      BOOLEAN r = FALSE;
      if (sizeNeeded <= head->freeSpaceAfterLastSlot)
      {
         r = TRUE;
      }
      else if (sizeNeeded <= head->totalFreeSpace)
      {
         r = TRUE;
         needReorg = TRUE;
      }
   done:
      return r;
   }

   UINT32 rdpAccessor::getAlignedSizeOfNormalRecordAndHead(UINT32 recordSize)
   {
      return RDP_RECORD_HEAD_LEN + ossAlign4(recordSize);
   }

   UINT32 rdpAccessor::getNonFreeBeginOffet(const recordDataPageHead *head)
   {
      SDB_ASSERT(NULL != head, "can not be null");
      UINT32 offset = RDP_RECORD_HEAD_LEN +
                      (head->totalSlotCount * RDP_RSLOT_SIZE) +
                      head->freeSpaceAfterLastSlot;
      return offset;
   }

   void rdpAccessor::updateMinMaxStriping(recordDataPageHead *head,
                                          STRIPING_ID striping)
   {
      SDB_ASSERT(NULL != head, "can not be null");
      if (INVALID_STRIPING_ID == striping)
      {
         goto done;
      }

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
   done:
      return;
   }

   INT32 rdpAccessor::findFreeSlot(const recordDataPageHead *head,
                                     RECORD_SLOT_ID &slotID)
   {
      SDB_ASSERT(NULL != head, "can not be null");
      INT32 rc = SDB_OK;
      recordSlot slot;
      UINT32 count = head->totalSlotCount;
      slotID = INVALID_RECORD_SLOT_ID;

      if (0 == head->freeSlotCount)
      {
         goto done;
      }

      if (OSS_UNLIKELY(INVALID_RECORD_SLOT_ID <= count))
      {
         PD_LOG(PDERROR, "invalid total slot count:%d", count);
         rc = SDB_VESSEL_PAGE_CRASHED;
         goto error;
      }

      for (UINT16 i = 0; i < count; ++i)
      {
         rc = getSlot(i, slot);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to get record slot[%d], rc:%d", i, rc);
            goto error;
         }

         if (slot.isFree())
         {
            slotID = i;
            break;
         }
      }

      if (INVALID_RECORD_SLOT_ID == slotID)
      {
         rc = SDB_VESSEL_INTERNAL_ERR;
         PD_LOG(PDERROR, "free count is not zero but not found");
         goto error;
      }

   done:
      return rc;
   error:
      goto done;
   }

   INT32 rdpAccessor::getSlot(RECORD_SLOT_ID slotID, recordSlot &slot)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(INVALID_RECORD_SLOT_ID != slotID, "can not be invalid");
      const recordSlot *tmp = NULL;
      rc = getReadPtrOfPageBody<recordSlot>(RECORD_PAGE_HEAD_LEN + (RDP_RSLOT_SIZE *slotID),
                                            &tmp);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to get record slot[%d], rc:%d", slotID, rc);
         goto error;
      }

      slot = *tmp;
   done:
      return rc;
   error:
      goto done;
   }

   INT32 rdpAccessor::writeSlot(RECORD_SLOT_ID slotID,
                                const recordSlot &slot)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(INVALID_RECORD_SLOT_ID != slotID, "can not be invalid");
      recordSlot *tmp = NULL;
      rc = getWritePtrOfPageBody<recordSlot>(RECORD_PAGE_HEAD_LEN + (RDP_RSLOT_SIZE *slotID),
                                            &tmp);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to get record slot[%d], rc:%d", slotID, rc);
         goto error;
      }

      *tmp = slot;
   done:
      return rc;
   error:
      goto done;
   }

   INT32 rdpAccessor::prepareInsertWOCLog(insertContext *context,
                                          logRecordContext *lrc,
                                          UINT32 rhAndbodySize)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(NULL != context, "can not be null");
      SDB_ASSERT(UTIL_COMPRESSOR_INVALID == context->getCompressionType(), "must be invalid");
      ISession *session = context->getSession();
      SDB_ASSERT(NULL != lrc, "can not be null");
      SDB_ASSERT(!lrc->prepared(), "can not be prepared");
      IRedoLogger *logger = context->getOuterResource()->logger;
      dpsLogRecordHeader *head = NULL;
      UINT32 fullNameLen = context->getCSName().strLen() +
                           context->getCLName().strLen() + 2; // one for '.', one for '\0'

      head = &(lrc->getHead());
      head->_type = LOG_TYPE_DATA_INSERT;
      OSS_BIT_SET(head->_flags, DPS_VESSEL_LOG_FLAG_FROM_VESSEL);
      lrc->prepush(fullNameLen);
      if (context->getTransID().isValid())
      {
         lrc->prepush(sizeof(DPS_TRANSID_SN));
         lrc->prepush(sizeof(DPS_TRANSID_NODEID));
      }
      lrc->prepush(sizeof(GLOBAL_PAGE_ID));
      lrc->prepush(sizeof(recordID));
      lrc->prepush(sizeof(utilCLUniqueID));
      lrc->prepush(RECORD_PAGE_HEAD_LEN);
      lrc->prepush(RDP_RSLOT_SIZE);
      lrc->prepush(rhAndbodySize);

      rc = prepareFullDumpLogWhenNecessary(context, lrc);
      if (SDB_OK != rc)
      {
         goto error;
      }
      lrc->prepushDone();

      rc = logger->prepare(session, lrc);
      if (SDB_OK != rc)
      {
         goto error;
      }
   done:
      return rc;
   error:
      goto done;
   }

   INT32 rdpAccessor::commitInsertWOCLog(insertContext *context,
                                         logRecordContext *lrc,
                                         const recordID &rid,
                                         const recordDataPageHead *head,
                                         const recordSlot &slot,
                                         const recordHead *rh)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(NULL != context, "can not be null");
      SDB_ASSERT(UTIL_COMPRESSOR_INVALID == context->getCompressionType(), "must be invalid");
      ISession *session = context->getSession();
      SDB_ASSERT(NULL != lrc, "can not be null");
      SDB_ASSERT(lrc->prepared(), "must be prepared");
      SDB_ASSERT(rid.valid(), "can not be invalid");
      SDB_ASSERT(NULL != head, "can not be null");
      SDB_ASSERT(!slot.isFree(), "can not be free");
      SDB_ASSERT(NULL != rh, "can not be null");
      utilCLUniqueID uniqueID = context->getUniqueID();
      IRedoLogger *logger = NULL;
      CHAR *nameBuffer = NULL;
      UINT32 bufferSize = context->getCSName().strLen() +
                          context->getCLName().strLen() + 2;
      nameBuffer = context->allocateBuffer(bufferSize);
      if (NULL == nameBuffer)
      {
         PD_LOG(PDERROR, "failed to allcoate mem");
         rc = SDB_OOM;
         goto error;
      }

      ossMemcpy(nameBuffer, context->getCSName().str(),
                context->getCSName().strLen());
      nameBuffer[context->getCSName().strLen()] = '.';
      ossMemcpy((CHAR *)(nameBuffer + context->getCSName().strLen() + 1),
                context->getCLName().str(),
                context->getCLName().strLen());
      nameBuffer[bufferSize] = '\0';

      logger = context->getOuterResource()->logger;

      rc = logger->pushLogRecordElement(session, lrc,
                                        DPS_LOG_PUBLIC_FULLNAME,
                                        bufferSize,
                                        nameBuffer);
      if (OSS_UNLIKELY(SDB_OK != rc))
      {
         goto error;
      }

      if (context->getTransID().isValid())
      {
         DPS_TRANSID_SN sn = context->getTransID().getSN();
         DPS_TRANSID_NODEID nodeID = context->getTransID().getNodeID();
         rc = logger->pushLogRecordElement(session, lrc,
                                          DPS_LOG_PUBLIC_TRANSID,
                                          sizeof(DPS_TRANSID_SN),
                                          &sn);
         if (OSS_UNLIKELY(SDB_OK != rc))
         {
            goto error;
         }

         rc = logger->pushLogRecordElement(session, lrc,
                                          DPS_LOG_PUBLIC_TRANSID_NODEID,
                                          sizeof(DPS_TRANSID_NODEID),
                                          &nodeID);
         if (OSS_UNLIKELY(SDB_OK != rc))
         {
            goto error;
         }
      }

      rc = logger->pushLogRecordElement(session, lrc,
                                        DPS_LOG_PUBLIC_VESSEL_GPID,
                                        sizeof(GLOBAL_PAGE_ID),
                                        &getGPID());
      if (OSS_UNLIKELY(SDB_OK != rc))
      {
         goto error;
      }

      rc = logger->pushLogRecordElement(session, lrc,
                                        DPS_LOG_INSERT_VESSEL_SLOT,
                                        sizeof(recordID),
                                        &rid);
      if (OSS_UNLIKELY(SDB_OK != rc))
      {
         goto error;
      }

      rc = logger->pushLogRecordElement(session, lrc,
                                        DPS_LOG_INSERT_VESSEL_UNIQUEID,
                                        sizeof(utilCLUniqueID),
                                        &uniqueID);
      if (OSS_UNLIKELY(SDB_OK != rc))
      {
         goto error;
      }

      rc = logger->pushLogRecordElement(session, lrc,
                                        DPS_LOG_INSERT_VESSEL_PAGE_HEAD,
                                        RECORD_PAGE_HEAD_LEN,
                                        head);
      if (OSS_UNLIKELY(SDB_OK != rc))
      {
         goto error;
      }

      rc = logger->pushLogRecordElement(session, lrc,
                                        DPS_LOG_INSERT_VESSEL_SLOT,
                                        RDP_RSLOT_SIZE,
                                        &slot);
      if (OSS_UNLIKELY(SDB_OK != rc))
      {
         goto error;
      }

      rc = logger->pushLogRecordElement(session, lrc,
                                        DPS_LOG_INSERT_VESSEL_RECORD_AND_HEAD,
                                        rh->getSize(),
                                        rh);
      if (OSS_UNLIKELY(SDB_OK != rc))
      {
         goto error;
      }

      if (lrc->needFullDump())
      {
         const CHAR *dumpBuf = getFullDumpBuffer();
         SDB_ASSERT(NULL != dumpBuf, "can not be null");
         rc = logger->pushLogRecordElement(session, lrc,
                                           DPS_LOG_PUBLIC_VESSEL_FULL_PAGE_DUMP,
                                           getPageSize(), dumpBuf);
         if (SDB_OK != rc)
         {
            goto error;
         }
      }

      rc = logger->commit(session, lrc);
      if (SDB_OK != rc)
      {
         goto error;
      }
   done:
      if (NULL != nameBuffer)
      {
         context->releaseBuffer(nameBuffer, bufferSize);
      }
      return rc;
   error:
      goto done;
   }
}//namespace vessel
}//namespace engine