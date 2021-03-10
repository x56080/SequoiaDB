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

   Source File Name = crpAccessor.cpp

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

#include "vessel/crpAccessor.h"
#include "vessel/collectionRecordPage.h"
#include "pdTrace.hpp"
#include "vessel/IRedoLogger.h"
#include "vessel/logRecordContext.h"
#include "vessel/requestContext.h"
#include "vessel/outerResource.h"
#include "vessel/redoLogUtil.h"
#include "dpsLogRecordDef.hpp"

namespace engine
{
namespace vessel
{
   crpAccessor::crpAccessor()
   {}

   crpAccessor::~crpAccessor()
   {}

   INT32 crpAccessor::initPage(PAGE_ID lpid)
   {
      INT32 rc = SDB_OK;
      collectionRecordPageHead *head = NULL;
      pageHead *pageHead = NULL;
      UINT32 capacity = 0;
      UINT32 flags = PAGE_ACCESSOR_FLAG_DIRECT |
                     PAGE_ACCESSOR_FLAG_NON_READONLY |
                     PAGE_ACCESSOR_FLAG_INIT_PAGE;
      SDB_ASSERT(OSS_BIT_TEST(getFlags(), flags), "impossible");

      if (OSS_UNLIKELY(INVALID_PAGE_ID == lpid))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      rc = getCapacityOfCLRecordPage(getPageSize(), capacity);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to get capacity of page size:%d", getPageSize());
         rc = SDB_VESSEL_INTERNAL_ERR;
         goto error;
      }

      /// bitmap capacity is 64
      if (OSS_UNLIKELY(64 < capacity || 0 == capacity))
      {
         PD_LOG(PDERROR, "invalid capacity:%d", capacity);
         rc = SDB_VESSEL_INTERNAL_ERR;
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

      rc = getWritePtrOfHead(&pageHead);
      if (SDB_OK != rc)
      {
         goto error;
      }

      rc = memsetPageBody(0);
      if (SDB_OK != rc)
      {
         goto error;
      }

      rc = getWritableUserHeadPtr<collectionRecordPageHead>(&head);
      if (SDB_OK != rc)
      {
         goto error;
      }

      head->version = COLLECTION_RECORD_PAGE_VERSION_1;
      head->capacity = capacity;
      head->bitmap = 1;
      head->pad = 0;
      head->minMBID = lpid * head->capacity;

      for (UINT32 i = 1; i < head->capacity; ++i)
      {
         head->bitmap = head->bitmap << 1;
         head->bitmap |= 0x01;
      }

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

   INT32 crpAccessor::createCL(const CHAR *csName,
                               const collectionRecord &record)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(NULL != csName, "can not be null");
      CL_MB_ID mbID = record.mbID;
      const collectionRecordPageHead *rHead = NULL;
      collectionRecordPageHead *wHead = NULL;
      UINT32 slot = 0;
      UINT32 bit = 1;
      logRecordContext lrContext;
      BOOLEAN rollback = FALSE;
      GLOBAL_FULL_PAGE_ID fpid;
      const pageHead *pHead = NULL;
      DPS_LSN_OFFSET lsn = DPS_INVALID_LSN_OFFSET;
      strSlice nameSlice;
      nameSlice.reset(csName);

      if (OSS_UNLIKELY(INVALID_CL_MB_ID == mbID))
      {
         goto error;
      }

      rc = getReadPtrOfHead(&pHead);
      if (SDB_OK != rc)
      {
         goto error;
      }

      rc = getReadableUserHeadPtr<collectionRecordPageHead>(&rHead);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to get collectionRecordPageHead:%d", rc);
         goto error;
      }

      if ((UINT32)mbID < rHead->minMBID)
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      else if ((rHead->minMBID + rHead->capacity) <= mbID)
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      slot = (mbID - rHead->minMBID) % rHead->capacity;

      bit = bit << slot;
      if (!OSS_BIT_TEST(rHead->bitmap, bit))
      {
         PD_LOG(PDERROR, "mbid[%d] is not free", mbID);
         rc = SDB_VESSEL_INTERNAL_ERR;
         goto error;
      }

      fpid.gpid = getGPID();
      fpid.lpid = pHead->pageID;
      

      rc = prepareCreateCLLog(&lrContext,
                              nameSlice, fpid, record);
      if (SDB_OK != rc)
      {
         goto error;
      }

      lsn = lrContext.getLsn();

      rc = prepareToWrite();
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to prepare writing:%d", rc);
         goto error;
      }

      rc = getWritableUserHeadPtr<collectionRecordPageHead>(&wHead);
      if (SDB_OK != rc)
      {
         goto error;
      }

      rollback = TRUE;
      OSS_BIT_CLEAR(wHead->bitmap, bit);
      rc = writeToSlot(slot, record);
      if (SDB_OK != rc)
      {
         goto error;
      }
 
      rc = commitCreateCLLog(&lrContext,
                             nameSlice, fpid, record);
      if (SDB_OK != rc)
      {
         goto error;
      }

      pageAccessor::commit(lsn);
      lrContext.close();
      rollback = FALSE;

   done:
      return rc;
   error:
      if (lrContext.prepared())
      {
         IRedoLogger *logger = getContext()->getOuterResource()->logger;
         logger->abort(getContext()->getSession(), &lrContext);
      }
      if (rollback)
      {
         collectionRecord tmp;
         ossMemset(&tmp, 0, COLLECTION_RECORD_LEN);
         writeToSlot(slot, tmp);
         OSS_BIT_SET(wHead->bitmap, bit);
         abortToWrite();
      }
      if (fullAccessing())
      {
         abortToWrite();
      }
      goto done;
   }

   INT32 crpAccessor::getClRecordBySlot(UINT32 slot, collectionRecord &record)
   {
      INT32 rc = SDB_OK;
      const collectionRecordPageHead *head = NULL;

      rc = getReadableUserHeadPtr<collectionRecordPageHead>(&head);
      if (SDB_OK != rc)
      {
         goto error;
      }

      if (head->capacity <= slot)
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      else if (OSS_BIT_TEST(head->bitmap, ((UINT64)1 << slot)))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      rc = readFromSlot(slot, record);
      if (SDB_OK != rc)
      {
         goto error;
      }
   done:
      return rc;
   error:
      goto done;
   }

   INT32 crpAccessor::getHeadContent(UINT32 &capacity, UINT64 &bitmap)
   {
      INT32 rc = SDB_OK;
      const collectionRecordPageHead *head = NULL;

      rc = getReadableUserHeadPtr<collectionRecordPageHead>(&head);
      if (SDB_OK != rc)
      {
         goto error;
      }

      capacity = head->capacity;
      bitmap = head->bitmap;
   done:
      return rc;
   error:
      goto done;
   }

   INT32 crpAccessor::writeToSlot(UINT32 slot, const collectionRecord &record)
   {
      INT32 rc = SDB_OK;
      UINT32 offset = COLLECTION_RECORD_PAGE_HEAD_LEN + slot * COLLECTION_DISK_RECORD_LEN;
      UINT32 len = COLLECTION_RECORD_LEN;

      rc = writePageBody(offset, len, (const CHAR *)(&record));
      if (SDB_OK != rc)
      {
         goto error;
      }
   done:
      return rc;
   error:
      goto done;
   }

   INT32 crpAccessor::readFromSlot(UINT32 slot, collectionRecord &record)
   {
      INT32 rc = SDB_OK;
      UINT32 offset = COLLECTION_RECORD_PAGE_HEAD_LEN + slot * COLLECTION_DISK_RECORD_LEN;
      UINT32 len = COLLECTION_RECORD_LEN;

      rc = readPageBody(offset, len, (CHAR *)(&record));
      if (SDB_OK != rc)
      {
         goto error;
      }
   done:
      return rc;
   error:
      goto done; 
   }

   INT32 crpAccessor::prepareCreateCLLog(logRecordContext *lrc,
                                         const strSlice &csName,
                                         const GLOBAL_FULL_PAGE_ID &id,
                                         const collectionRecord &record)
   {
      INT32 rc = SDB_OK;

      ISession *session = getContext()->getSession();
      SDB_ASSERT(NULL != lrc, "can not be null");
      SDB_ASSERT(!lrc->prepared(), "can not be prepared");
      IRedoLogger *logger = getContext()->getOuterResource()->logger;
      dpsLogRecordHeader *head = NULL;

      head = &(lrc->getHead());
      head->_type = LOG_TYPE_CL_CRT;
      OSS_BIT_SET(head->_flags, DPS_VESSEL_LOG_FLAG_FROM_VESSEL);

      lrc->prepush(sizeof(GLOBAL_FULL_PAGE_ID));
      lrc->prepush(csName.strLen() + 1);
      lrc->prepush(sizeof(collectionRecord));
      rc = prepareFullDumpLogWhenNecessary(lrc);
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

   INT32 crpAccessor::commitCreateCLLog(logRecordContext *lrc,
                                        const strSlice &csName,
                                        const GLOBAL_FULL_PAGE_ID &id,
                                        const collectionRecord &record)
   {
      INT32 rc = SDB_OK;

      ISession *session = getContext()->getSession();
      SDB_ASSERT(NULL != lrc, "can not be null");
      SDB_ASSERT(lrc->prepared(), "must be prepared");
      IRedoLogger *logger = getContext()->getOuterResource()->logger;

      rc = logger->pushLogRecordElement(session, lrc,
                                        DPS_LOG_PUBLIC_VESSEL_FULLID,
                                        sizeof(GLOBAL_FULL_PAGE_ID),
                                        &id);
      if (OSS_UNLIKELY(SDB_OK != rc))
      {
         goto error;
      }

      rc = logger->pushLogRecordElement(session, lrc,
                                        DPS_LOG_CLCRT_VESSEL_CS_NAME,
                                        csName.strLen() + 1,
                                        csName.str());
      if (OSS_UNLIKELY(SDB_OK != rc))
      {
         goto error;
      }

      rc = logger->pushLogRecordElement(session, lrc,
                                        DPS_LOG_CLCRT_VESSEL_CL_RECORD,
                                        sizeof(collectionRecord),
                                        &record);
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
         if (OSS_UNLIKELY(SDB_OK != rc))
         {
            goto error;
         }
      }

      rc = logger->commit(session, lrc);
      if (OSS_UNLIKELY(SDB_OK != rc))
      {
         goto error;
      }
   done:
      return rc;
   error:
      goto done;
   }
}//namespace vessel
}//namespace engine