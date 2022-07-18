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

   Source File Name = indexEntryPageAccessor.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#include "vessel/indexEntryPageAccessor.h"
#include "vessel/indexEntryPage.h"
#include "vessel/requestContext.h"
#include "vessel/indexUtils.h"


namespace engine
{
namespace vessel
{
   INT32 indexEntryPageAccessor::createIndex(requestContext *context,
                                           UINT32 indexId,
                                           const slice &defObj,
                                           logicalPageBuffer *lpb)const
   {
      INT32 rc = SDB_OK;
      indexEntryPageHead head;
      indexEntryPageHead *headPtr = NULL;

      if (OSS_UNLIKELY(NULL == context ||
                       !context->isClPropertiesSet() ||
                       INVALID_LOGICAL_INDEX_ID == indexId ||
                       !defObj.isValid() ||
                       NULL == lpb ||
                       !lpb->isValid()))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      else if (getPageBodySize(lpb->getRuntimeBuffer().getPageSize()) < 
              (INDEX_ENTRY_PAGE_HEAD_SIZE + defObj.getSize()))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      rc = lpb->validatePage(PAGE_TYPE_INDEX_ENTRY);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to validate page[%s], rc:%d",
                lpb->getRuntimeBuffer().getGlobalPid().toString().c_str(), rc);
         goto error;
      }

      head.version = INDEX_DEF_RECORD_VERSION;
      head.indexLogicalID = indexId;
      head.clLogicalID = context->getLogicalClId();
      head.createdTime = ossGetCurrentMilliseconds();
      head.alteredTime = head.createdTime;
      head.status = INDEX_STATUS_BUILDING;
      head.defObjSize = defObj.getSize();

      rc = lpb->prepareToWrite();
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to get rpb ready to write:%d", rc);
         goto error;
      }

      headPtr = lpb->getWritableBodyBuffer().getWritableObjPtr<indexEntryPageHead>(0);
      if (NULL == headPtr)
      {
         PD_LOG(PDERROR, "failed to get writable head ptr");
         rc = SDB_VESSEL_INTERNAL_ERR;
         goto error;
      }

      *headPtr = head;
      ossMemcpy((void *)((CHAR *)headPtr + INDEX_ENTRY_PAGE_HEAD_SIZE),
                 defObj.data(), defObj.getSize());
      lpb->commit(context->getExecutor()->getEndLsn());

   done:
      return rc;
   error:
      goto done;
   }

   INT32 indexEntryPageAccessor::updateIndexStatus(requestContext *context,
                                                 UINT32 indexId,
                                                 INDEX_STATUS status,
                                                 logicalPageBuffer *lpb)const
   {
      INT32 rc = SDB_OK;
      const indexEntryPageHead *readableHead = NULL;
      indexEntryPageHead *head = NULL;

      if (OSS_UNLIKELY(NULL == context ||
                       !context->isClPropertiesSet() ||
                       INVALID_LOGICAL_INDEX_ID == indexId ||
                       INDEX_STATUS_INVALID == status ||
                       NULL == lpb ||
                       !lpb->isValid()))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      rc = lpb->validatePage(PAGE_TYPE_INDEX_ENTRY);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to validate page[%s], rc:%d",
                lpb->getRuntimeBuffer().getGlobalPid().toString().c_str(), rc);
         goto error;
      }

      readableHead = lpb->getReadableBodyBuffer().getReadableObjPtr<indexEntryPageHead>(0);
      if (NULL == readableHead)
      {
         PD_LOG(PDERROR, "failed to get readable ptr of head");
         rc = SDB_VESSEL_INTERNAL_ERR;
         goto error;
      }

      if (!readableHead->isValid())
      {
         PD_LOG(PDERROR, "index def page head is not valid");
         rc = SDB_IXM_NOTEXIST;
         goto error;
      }
      else if (context->getLogicalClId() !=
               readableHead->clLogicalID)
      {
         PD_LOG(PDERROR, "collection logical id in context[%d] does match the one[%d] in head",
                context->getLogicalClId(),
                readableHead->clLogicalID);
         rc = SDB_VESSEL_PAGE_HEAD_NOT_MATCH;
         goto error;
      }
      else if (indexId != readableHead->indexLogicalID)
      {
         PD_LOG(PDERROR, "index logical id [%d] does match the one[%d] in head",
                indexId, readableHead->indexLogicalID);
         rc = SDB_VESSEL_PAGE_HEAD_NOT_MATCH;
         goto error;
      }

      rc = lpb->prepareToWrite();
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to get buffer ready to write:%d", rc);
         goto error;
      }
      readableHead = NULL;

      head = lpb->getWritableBodyBuffer().getWritableObjPtr<indexEntryPageHead>(0);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to get writable ptr of head");
         rc = SDB_VESSEL_INTERNAL_ERR;
         goto error;
      }

      head->status = status;
      head->alteredTime = ossGetCurrentMilliseconds();
      lpb->commit(context->getExecutor()->getEndLsn());
   done:
      return rc;
   error:
      goto done;
   }

   INT32 indexEntryPageAccessor::updateBtreeRoot(requestContext *context,
                                                 UINT32 indexId,
                                                 PAGE_ID root,
                                                 logicalPageBuffer *lpb)const
   {
      INT32 rc = SDB_OK;
      const indexEntryPageHead *readableHead = NULL;
      indexEntryPageHead *head = NULL;

      if (OSS_UNLIKELY(NULL == context ||
                       !context->isClPropertiesSet() ||
                       INVALID_LOGICAL_INDEX_ID == indexId ||
                       INVALID_PAGE_ID == root ||
                       NULL == lpb ||
                       !lpb->isValid()))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      rc = lpb->validatePage(PAGE_TYPE_INDEX_ENTRY);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to validate page[%s], rc:%d",
                lpb->getRuntimeBuffer().getGlobalPid().toString().c_str(), rc);
         goto error;
      }

      readableHead = lpb->getReadableBodyBuffer().getReadableObjPtr<indexEntryPageHead>(0);
      if (NULL == readableHead)
      {
         PD_LOG(PDERROR, "failed to get readable ptr of head");
         rc = SDB_VESSEL_INTERNAL_ERR;
         goto error;
      }

      if (!readableHead->isValid())
      {
         PD_LOG(PDERROR, "index def page head is not valid");
         rc = SDB_IXM_NOTEXIST;
         goto error;
      }
      else if (context->getLogicalClId() !=
               readableHead->clLogicalID)
      {
         PD_LOG(PDERROR, "collection logical id in context[%d] does match the one[%d] in head",
                context->getLogicalClId(),
                readableHead->clLogicalID);
         rc = SDB_VESSEL_PAGE_HEAD_NOT_MATCH;
         goto error;
      }
      else if (indexId != readableHead->indexLogicalID)
      {
         PD_LOG(PDERROR, "index logical id [%d] does match the one[%d] in head",
                indexId, readableHead->indexLogicalID);
         rc = SDB_VESSEL_PAGE_HEAD_NOT_MATCH;
         goto error;
      }

      rc = lpb->prepareToWrite();
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to get buffer ready to write:%d", rc);
         goto error;
      }
      readableHead = NULL;

      head = lpb->getWritableBodyBuffer().getWritableObjPtr<indexEntryPageHead>(0);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to get writable ptr of head");
         rc = SDB_VESSEL_INTERNAL_ERR;
         goto error;
      }

      head->btreeRoot = root;
      lpb->commit(context->getExecutor()->getEndLsn());
   done:
      return rc;
   error:
      goto done;
   }

   INT32 indexEntryPageAccessor::getIndexDefPageHead(requestContext *context,
                                                   UINT32 indexId,
                                                   const logicalPageBuffer *lpb,
                                                   const indexEntryPageHead **out)const
   {
      INT32 rc = SDB_OK;
      const indexEntryPageHead *head = NULL;

      if (OSS_UNLIKELY(NULL == context ||
                       !context->isClPropertiesSet() ||
                       INVALID_LOGICAL_INDEX_ID == indexId ||
                       NULL == lpb ||
                       !lpb->isValid() ||
                       NULL == out))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      rc = lpb->validatePage(PAGE_TYPE_INDEX_ENTRY);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to validate page[%s], rc:%d",
                lpb->getRuntimeBuffer().getGlobalPid().toString().c_str(), rc);
         goto error;
      }

      head = lpb->getReadableBodyBuffer().getReadableObjPtr<indexEntryPageHead>(0);
      if (NULL == head)
      {
         PD_LOG(PDERROR, "failed to get readable head ptr:%d", rc);
         goto error;
      }

      if (!head->isValid())
      {
         PD_LOG(PDERROR, "index def page head is not valid");
         rc = SDB_IXM_NOTEXIST;
         goto error;
      }

      if (context->getLogicalClId() != head->clLogicalID)
      {
         PD_LOG(PDERROR, "collection logical id in context[%d] does match the one[%d] in head",
                context->getLogicalClId(), head->clLogicalID);
         rc = SDB_VESSEL_PAGE_HEAD_NOT_MATCH;
         goto error;
      }

      if (indexId != head->indexLogicalID)
      {
         PD_LOG(PDERROR, "index logical id[%d] does match the one[%d] in head",
                indexId, head->indexLogicalID);
         rc = SDB_VESSEL_PAGE_HEAD_NOT_MATCH;
         goto error;
      }

      *out = head;
   done:
      return rc;
   error:
      goto done;
   }

   INT32 indexEntryPageAccessor::removeBtreeRoot(requestContext *context,
                                                 logicalPageBuffer *lpb,
                                                 PAGE_ID &oldValue)
   {
      INT32 rc = SDB_OK;

      const indexEntryPageHead *header = NULL;
      oldValue = INVALID_PAGE_ID;

      if (OSS_UNLIKELY(NULL == context ||
                       NULL == lpb ||
                       !lpb->isValid()))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      rc = lpb->validatePage(PAGE_TYPE_INDEX_ENTRY);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to validate page[%s], rc:%d",
                lpb->getRuntimeBuffer().getGlobalPid().toString().c_str(), rc);
         goto error;
      }

      header = lpb->getReadableBodyBuffer().getReadableObjPtr<indexEntryPageHead>(0);
      if (INVALID_PAGE_ID == header->btreeRoot)
      {
         goto done;
      }

      oldValue = header->btreeRoot;
      header = NULL;
      rc = lpb->prepareToWrite();
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to get buffer ready to write:%d", rc);
         goto error;
      }

      
      lpb->getWritableBodyBuffer().getWritableObjPtr<indexEntryPageHead>(0)->btreeRoot = INVALID_PAGE_ID;
      lpb->commit(context->getExecutor()->getEndLsn());
   done:
      return rc;
   error:
      oldValue = INVALID_PAGE_ID;
      goto done;
   }
}//namespace vessel
}//namespace engine