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

   Source File Name = indexDefPageAccessor.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#include "vessel/indexDefPageAccessor.h"
#include "vessel/indexDefPage.h"
#include "vessel/requestContext.h"
#include "vessel/indexUtils.h"

namespace engine
{
namespace vessel
{
   INT32 indexDefPageAccessor::testIndexDef(requestContext *context,
                                            const strSlice &indexName,
                                            const indexKeyPattern &pattern,
                                            const logicalPageBuffer *lpb,
                                            BOOLEAN &duplicated)const
   {
      INT32 rc = SDB_OK;
      const runtimePageBuffer *rpb = NULL;
      const indexDefHead *head = NULL;
      ossValuePtr ptr = 0;
      indexKeyPattern indexPattern;
      strSlice nameSlice;
      bson::BSONObj defObj;

      if (OSS_UNLIKELY(NULL == context ||
                       indexName.empty() ||
                       NULL == lpb || !lpb->isValid()))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      duplicated = FALSE;
      rpb = &(lpb->getRuntimeBuffer());

      rc = lpb->validatePage(PAGE_TYPE_INDEX_DEF);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to validate page[%s], rc:%d",
                rpb->getGlobalPid().toString().c_str(), rc);
         goto error;
      }

      head = rpb->getReadablePtrOfBody<indexDefHead>(0);
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

      if (context->getCLLid() != head->clLogicalID)
      {
         PD_LOG(PDERROR, "collection logical id in context[%d] does match the one[%d] in head",
                context->getCLLid(), head->clLogicalID);
         rc = SDB_VESSEL_PAGE_HEAD_NOT_MATCH;
         goto error;
      }

      rc = rpb->getReadablePtrOfBodyWithRc(INDEX_DEF_HEAD_SIZE, head->defObjSize, ptr);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to get obj ptr:%d", rc);
         goto error;
      }

      defObj = bson::BSONObj((const CHAR *)ptr);
      rc = indexUtils::parseIndexDefObj(defObj, &nameSlice, &indexPattern, NULL);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to parse index def obj:%d", rc);
         goto error;
      }

      duplicated = (indexName == nameSlice) ||
                    pattern.isCoveredBy(indexPattern);
   done:
      return rc;
   error:
      goto done;
   }

   INT32 indexDefPageAccessor::createIndex(requestContext *context,
                                           UINT32 indexId,
                                           const slice &defObj,
                                           logicalPageBuffer *lpb)const
   {
      INT32 rc = SDB_OK;
      indexDefHead head;
      indexDefHead *headPtr = NULL;

      if (OSS_UNLIKELY(NULL == context ||
                       INVALID_LOGICAL_INDEX_ID == indexId ||
                       !defObj.isValid() ||
                       NULL == lpb ||
                       !lpb->isValid()))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      else if (getPageBodySize(lpb->getRuntimeBuffer().getPageSize()) < 
              (INDEX_DEF_HEAD_SIZE + defObj.len()))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      rc = lpb->validatePage(PAGE_TYPE_INDEX_DEF);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to validate page[%s], rc:%d",
                lpb->getRuntimeBuffer().getGlobalPid().toString().c_str(), rc);
         goto error;
      }

      head.version = INDEX_DEF_RECORD_VERSION;
      head.indexLogicalID = indexId;
      head.clLogicalID = context->getCLLid();
      head.createdTime = ossGetCurrentMilliseconds();
      head.alteredTime = head.createdTime;
      head.status = INDEX_STATUS_BUILDING;
      head.defObjSize = defObj.len();

      rc = lpb->getRuntimeBuffer().prepareToWrite(context);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to get rpb ready to write:%d", rc);
         goto error;
      }

      headPtr = lpb->getRuntimeBuffer().getWritablePtrOfBody<indexDefHead>(0);
      if (NULL == headPtr)
      {
         PD_LOG(PDERROR, "failed to get writable head ptr");
         rc = SDB_VESSEL_INTERNAL_ERR;
         goto error;
      }

      *headPtr = head;
      ossMemcpy((void *)((CHAR *)headPtr + INDEX_DEF_HEAD_SIZE),
                 defObj.data(), defObj.len());
      lpb->getRuntimeBuffer().commit(context->getSession()->getLastLSN());

   done:
      return rc;
   error:
      if (NULL != lpb)
      {
         lpb->getRuntimeBuffer().abort();
      }
      goto done;
   }

   INT32 indexDefPageAccessor::dump(requestContext *context,
                                    const logicalPageBuffer *lpb,
                                    bson::BSONObjBuilder &builder)const
   {
      INT32 rc = SDB_OK;
      const indexDefHead *head = NULL;
      bson::BSONObj defObj;
      ossValuePtr ptr = 0;

      if (OSS_UNLIKELY(NULL == context ||
                       NULL == lpb ||
                       !lpb->isValid()))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      rc = lpb->validatePage(PAGE_TYPE_INDEX_DEF);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to validate page[%s], rc:%d",
                lpb->getRuntimeBuffer().getGlobalPid().toString().c_str(), rc);
         goto error;
      }

      head = lpb->getRuntimeBuffer().getReadablePtrOfBody<indexDefHead>(0);
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

      if (context->getCLLid() != head->clLogicalID)
      {
         PD_LOG(PDERROR, "collection logical id in context[%d] does match the one[%d] in head",
                context->getCLLid(), head->clLogicalID);
         rc = SDB_VESSEL_PAGE_HEAD_NOT_MATCH;
         goto error;
      }

      rc = lpb->getRuntimeBuffer().getReadablePtrOfBodyWithRc(INDEX_DEF_HEAD_SIZE,
                                                              head->defObjSize, ptr);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to get readble ptr of def obj:%d", rc);
         goto error;
      }

      defObj = bson::BSONObj((const CHAR *)ptr);
      builder.append(VESSEL_INDEX_FIELD_NAME_INDEX_ID, head->indexLogicalID);
      builder.append(VESSEL_INDEX_FIELD_NAME_STATUS, head->status);
      builder.appendIntOrLL(VESSEL_INDEX_FIELD_NAME_CREATED_TIME, head->createdTime);
      builder.appendIntOrLL(VESSEL_INDEX_FIELD_NAME_ALTERED_TIME, head->alteredTime);
      builder.appendElements(defObj);
   done:
      return rc;
   error:
      goto done;
   }

   INT32 indexDefPageAccessor::updateIndexStatus(requestContext *context,
                                                 INDEX_STATUS newStatus,
                                                 logicalPageBuffer *lpb)const
   {
      INT32 rc = SDB_OK;
      const indexDefHead *readableHead = NULL;
      indexDefHead *head = NULL;

      if (NULL == context ||
          NULL == lpb ||
          !lpb->isValid())
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      rc = lpb->validatePage(PAGE_TYPE_INDEX_DEF);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to validate page[%s], rc:%d",
                lpb->getRuntimeBuffer().getGlobalPid().toString().c_str(), rc);
         goto error;
      }

      readableHead = lpb->getRuntimeBuffer().getReadablePtrOfBody<indexDefHead>(0);
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
      else if (context->getCLLid() != readableHead->clLogicalID)
      {
         PD_LOG(PDERROR, "collection logical id in context[%d] does match the one[%d] in head",
                context->getCLLid(), readableHead->clLogicalID);
         rc = SDB_VESSEL_PAGE_HEAD_NOT_MATCH;
         goto error;
      }

      rc = lpb->prepareToWrite(context);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to get buffer ready to write:%d", rc);
         goto error;
      }
      readableHead = NULL;

      head = lpb->getRuntimeBuffer().getWritablePtrOfBody<indexDefHead>(0);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to get writable ptr of head");
         rc = SDB_VESSEL_INTERNAL_ERR;
         goto error;
      }

      head->status = newStatus;
      head->alteredTime = ossGetCurrentMilliseconds();
      lpb->getRuntimeBuffer().commit(context->getSession()->getLastLSN());
   done:
      return rc;
   error:
      if (NULL != lpb && lpb->getRuntimeBuffer().isWritingPrepared())
      {
         lpb->getRuntimeBuffer().abort();
      }
      goto done;
   }

   INT32 indexDefPageAccessor::getIndexObject(requestContext *context,
                                              const logicalPageBuffer *lpb,
                                              indexObject &obj,
                                              BOOLEAN getOwned,
                                              indexDefHead *out)const
   {
      INT32 rc = SDB_OK;
      const indexDefHead *readableHead = NULL;
      ossValuePtr ptr = 0;
      bson::BSONObj defObj;
      indexKeyPattern pattern;
      indexParameters params;
      strSlice indexName;
      obj.fini();

      if (NULL == context ||
          NULL == lpb ||
          !lpb->isValid())
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      rc = lpb->validatePage(PAGE_TYPE_INDEX_DEF);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to validate page[%s], rc:%d",
                lpb->getRuntimeBuffer().getGlobalPid().toString().c_str(), rc);
         goto error;
      }

      readableHead = lpb->getRuntimeBuffer().getReadablePtrOfBody<indexDefHead>(0);
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
      else if (context->getCLLid() != readableHead->clLogicalID)
      {
         PD_LOG(PDERROR, "collection logical id in context[%d] does match the one[%d] in head",
                context->getCLLid(), readableHead->clLogicalID);
         rc = SDB_VESSEL_PAGE_HEAD_NOT_MATCH;
         goto error;
      }

      rc = lpb->getRuntimeBuffer().getReadablePtrOfBodyWithRc(INDEX_DEF_HEAD_SIZE,
                                                              readableHead->defObjSize,
                                                              ptr);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to get def obj ptr:%d", rc);
         goto error;
      }

      defObj = bson::BSONObj((const CHAR *)ptr);
      rc = indexUtils::parseIndexDefObj(defObj, &indexName, &pattern, &params);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to parse def obj:%d", rc);
         goto error;
      }

      rc = obj.shallowInit(readableHead->indexLogicalID,
                           indexName,
                           pattern, params);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to init index obj:%d", rc);
         goto error;
      }

      if (getOwned)
      {
         obj.getOwned();
      }

      if (NULL != out)
      {
         *out = *readableHead;
      }
      
   done:
      return rc;
   error:
      obj.fini();
      goto done;
   }      
}//namespace vessel
}//namespace engine