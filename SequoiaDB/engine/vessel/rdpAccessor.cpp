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

namespace engine
{
namespace vessel
{
   INT32 rdpAccessor::initRdp(PAGE_ID lpid,
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

      rc = prepareToWrite();
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
      head->compressionFlags = 0;
      head->compressionDicSlot = RDP_INVALID_COMPRESSION_SLOT;
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

      pageAccessor::commit(DPS_INVALID_LSN_OFFSET);
   done:
      return rc;
   error:
      if (fullAccessing())
      {
         abortToWrite();
      }
      goto done;
   }

   INT32 rdpAccessor::setCLInfo(UINT32 logicalID,
                                utilCLUniqueID uniqueID,
                                const CHAR *csName,
                                const CHAR *clName)
   {
      INT32 rc = SDB_OK;
      if (OSS_UNLIKELY(DMS_INVALID_LOGICCLID == logicalID ||
                       NULL == csName ||
                       NULL == clName))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      _clLogicalID = logicalID;
      _uniqueID = uniqueID;
      _csName.reset(csName);
      _clName.reset(clName);

      if (OSS_UNLIKELY(_csName.empty() ||
                       _clName.empty()))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
   done:
      return rc;
   error:
      goto done;
   }

   INT32 rdpAccessor::insert(const recordData &record,
                             UTIL_COMPRESSOR_TYPE compressionType,
                             const DPS_TRANS_ID &transID,
                             STRIPING_ID striping,
                             const insertOptions &options,
                             recordID *rid)
   {
      INT32 rc = SDB_OK;
      const recordDataPageHead *rHead = NULL;
      RECORD_SLOT_ID slotID = INVALID_RECORD_SLOT_ID;
      BOOLEAN needReorg = FALSE;

      if (OSS_UNLIKELY(!record.isValid() ||
                        DMS_INVALID_LOGICCLID == _clLogicalID))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      rc = getReadableUserHeadPtr<recordDataPageHead>(&rHead);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to get rdp head:%d", rc);
         goto error;
      }

      if (INVALID_RDP_VERSION == rHead->version)
      {
         PD_LOG(PDERROR, "invalid record data page version, gpid:%s", getGPID().toString().c_str());
         rc = SDB_VESSEL_PAGE_CRASHED;
         goto error;
      }

      if (_clLogicalID != rHead->clLogcalID)
      {
         PD_LOG(PDERROR, "target logical id[%d], logical id in head[%d]",
                _clLogicalID, rHead->clLogcalID);
         rc = SDB_VESSEL_PAGE_CRASHED;
         goto error;
      }

      if (compressionType != UTIL_COMPRESSOR_LZ4)
      {
         rc = insertWithOutInPageCompression(record, compressionType,
                                             transID, striping, options);
         if (SDB_OK != rc)
         {
            goto error;
         }
      }

      rc = findFreeSlot(rHead, slotID);
      if (SDB_OK != rc)
      {
         goto error;
      }
   done:
      return rc;
   error:
      goto done;
   }

   INT32 rdpAccessor::insertWithOutInPageCompression(const recordDataPageHead *head,
                                                     const recordData &record,
                                                     UTIL_COMPRESSOR_TYPE compressionType,
                                                     const DPS_TRANS_ID &transID,
                                                     STRIPING_ID striping,
                                                     const insertOptions &options,
                                                     recordID *rid)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(NULL != head, "can not be null");
      SDB_ASSERT(DMS_INVALID_LOGICCLID != _clLogicalID, "can not be invalid");
      SDB_ASSERT(!_csName.empty() && !_clName.empty(), "can not be empty");
      SDB_ASSERT(UTIL_COMPRESSOR_LZ4 != compressionType, "can not be lz4");
   done:
      return rc;
   error:
      goto done;
   }

   BOOLEAN rdpAccessor::hasSpaceToInsertNormalRecord(const recordDataPageHead *head,
                                                     UINT32 originalRecordSize,
                                                     BOOLEAN &needReorg)
   {
      SDB_ASSERT(NULL != head, "can not be null");
      BOOLEAN r = FALSE;
      UINT32 alignedSize = getAlignedSizeOfNormalRecordAndHead(originalRecordSize);
      if (0 == head->freeSlotCount)
      {
         alignedSize += RDP_RSLOT_SIZE;
      }

      if (alignedSize <= head->freeSpaceAfterLastSlot)
      {
         r = TRUE;
      }
      else if (alignedSize <= head->totalFreeSpace)
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

   INT32 rdpAccessor::findFreeSlot(const recordDataPageHead *head,
                                     RECORD_SLOT_ID &slotID)
   {
      SDB_ASSERT(NULL != head, "can not be null");
      INT32 rc = SDB_OK;
      recordSlot slot;
      UINT32 count = head->totalSlotCount;

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
                                            RDP_RSLOT_SIZE, &tmp);
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
}//namespace vessel
}//namespace engine