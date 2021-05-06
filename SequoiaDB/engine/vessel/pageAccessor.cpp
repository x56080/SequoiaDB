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

   Source File Name = pageAccessor.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#include "vessel/pageAccessor.h"
#include "ossLikely.hpp"
#include "vessel/requestContext.h"
#include "vessel/instanceEnv.h"
#include "vessel/storageUnit.h"
#include "vessel/vesselOptions.h"
#include "vessel/outerResource.h"
#include "vessel/IRedoLogger.h"
#include "dpsLogRecordDef.hpp"
#include "vessel/logRecordContext.h"
#include "vessel/vesselOptions.h"
#include "vessel/checkpointController.h"

namespace engine
{
namespace vessel
{
   pageAccessor::~pageAccessor()
   {
      SDB_ASSERT(!accessing(), "fini lost");
   }

   INT32 pageAccessor::initUniversally(requestContext *context,
                                       FILE_TYPE type,
                                       PAGE_ID pid,
                                       UINT32 flags,
                                       storageUnit *su,
                                       DPS_LSN_OFFSET oplist)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(ACCESSOR_STATUS_INVALID == _status, "do not reinit");
      storageUnit *obj = su;
      fini(context);

      if (OSS_UNLIKELY(NULL == context))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      else if (OSS_UNLIKELY(!context->getSpaceIDLocked()))
      { 
         rc = SDB_INVALIDARG;
         goto error;  
      }
      else if (OSS_UNLIKELY(INVALID_FILE_TYPE == type))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      else if (OSS_UNLIKELY(INVALID_PAGE_ID == pid))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      else if (OSS_BIT_TEST(flags, PAGE_ACCESSOR_FLAG_OPLIST_TAIL) &&
               DPS_INVALID_LSN_OFFSET == oplist)
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      else if (OSS_BIT_TEST(flags, PAGE_ACCESSOR_FLAG_OPLIST_HEAD) &&
               DPS_INVALID_LSN_OFFSET != oplist)
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      if (NULL == obj)
      {
         rc = context->getEnv()->csContainer.getStorageUnit(context->getSpaceID(), &obj);
         if (SDB_OK != rc)
         {
            goto error;
         }
      }
      else if (OSS_UNLIKELY(obj->getSpaceID() != context->getSpaceID()))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      rc = obj->getCoreArgs(type, &_pageSize, NULL, NULL);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to get page size:%d", rc);
         goto error;
      }

      _gpid.reset(context->getSpaceID(), type, pid);
      _flags = flags;
      if (DPS_INVALID_LSN_OFFSET != oplist)
      {
         _oplist = oplist;
      }

      rc = beginToAccess(context, obj);
      if (SDB_OK != rc)
      {
         goto error;
      }
   done:
      return rc;
   error:
      fini(context);
      goto done;
   }

   INT32 pageAccessor::initWithOptions(requestContext *context,
                                       FILE_TYPE type,
                                       PAGE_ID pid,
                                       const options &o,
                                       storageUnit *su,
                                       DPS_LSN_OFFSET oplist)
   {
      return initUniversally(context, type, pid,
                             o.getFlags(), su, oplist);
   }

   INT32 pageAccessor::initWithMMapMode(requestContext *context,
                                        FILE_TYPE type,
                                        PAGE_ID pid,
                                        UINT32 pageSize,
                                        ossValuePtr ptr,
                                        BOOLEAN pageValidation,
                                        BOOLEAN readOnly)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(ACCESSOR_STATUS_INVALID == _status, "do not reinit");
      fini(context);
      UINT32 flags = 0;

      if (OSS_UNLIKELY(NULL == context))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      else if (OSS_UNLIKELY(!context->getSpaceIDLocked()))
      { 
         rc = SDB_INVALIDARG;
         goto error;  
      }
      else if (OSS_UNLIKELY(INVALID_FILE_TYPE == type))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      else if (OSS_UNLIKELY(INVALID_PAGE_ID == pid))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      else if (!isValidPageSize(pageSize))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      else if (OSS_UNLIKELY(0 == ptr))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      if (!pageValidation)
      {
         OSS_BIT_SET(flags, PAGE_ACCESSOR_FLAG_NO_PAGE_VALIDATION);
      }
      if (!readOnly)
      {
         OSS_BIT_SET(flags, PAGE_ACCESSOR_FLAG_NON_READONLY);
      }
      _gpid.reset(context->getSpaceID(), type, pid);
      _pageSize = pageSize;
      _flags = flags;
      _ptr = ptr;
      _status = ACCESSOR_STATUS_READONLY_ACCESSING;
      if (pageValidation)
      {
         rc = validateMMapPageHeadAndTail();
         if (SDB_OK != rc)
         {
            goto error;
         }
      }
   done:
      return rc;
   error:
      fini(context);
      goto done;
   }

   void pageAccessor::abortToWrite()
   {
      SDB_ASSERT(fullAccessing(), "must be prepared");
      if (ACCESSOR_STATUS_FULL_ACCESSING == _status)
      {
         _status = ACCESSOR_STATUS_READONLY_ACCESSING;
      }
      return;
   }

   INT32 pageAccessor::prepareToWrite(requestContext *context)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(ACCESSOR_STATUS_READONLY_ACCESSING == _status, "impossible");
      BOOLEAN readOnly = (0 == OSS_BIT_TEST(_flags, PAGE_ACCESSOR_FLAG_NON_READONLY));
      BOOLEAN cacheMode =  (0 != OSS_BIT_TEST(_flags, PAGE_ACCESSOR_FLAG_CACHE_MODE));

      if (OSS_UNLIKELY(ACCESSOR_STATUS_READONLY_ACCESSING != _status))
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }
      else if (OSS_UNLIKELY(readOnly))
      {
         rc = SDB_VESSEL_OPERATOION_NOT_PERMITTED;
         goto error;
      }

      if (cacheMode)
      {
         rc = prepareToWriteByCache(context);
         if (SDB_OK != rc)
         {
            goto error;
         }
      }
      _status = ACCESSOR_STATUS_FULL_ACCESSING;
   done:
      return rc;
   error:
      goto done;
   }

   void pageAccessor::commit(requestContext *context,
                             DPS_LSN_OFFSET lsn)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(fullAccessing(), "must be prepared");
      pageHead *head = NULL;
      if (ACCESSOR_STATUS_FULL_ACCESSING != _status)
      {
         goto done;
      }
         
      rc = getWritePtrOfHead(&head);
      if (OSS_LIKELY(SDB_OK == rc))
      {
         head->lsn = lsn;
      }
      rc = writeTail(lsn);
      if (OSS_UNLIKELY(SDB_OK != rc))
      {
         PD_LOG(PDSEVERE, "failed to update page[%s] tail:%d", _gpid.toString().c_str(), rc);
      }

      if (isCacheMode())
      {
         liteCache &cache = context->getEnv()->cache;
         cache.commit(context, lsn, _lcTuple);
      }
      _status = ACCESSOR_STATUS_READONLY_ACCESSING;
   done:
      return;
   }


   void pageAccessor::fini(requestContext *context)
   {
      SDB_ASSERT(!fullAccessing(), "writing prepared but no commit or abort");
      SDB_ASSERT(NULL != context, "can not be null");
      
      if (_lcTuple.valid())
      {
         liteCache &cache = context->getEnv()->cache;
         if (_lcTuple.valid())
         {
            cache.release(context, _lcTuple);
         }
      }
      _gpid.reset();
      _pageSize = 0;
      _flags = 0;
      _status = ACCESSOR_STATUS_INVALID;
      _ptr = 0;
      _oplist = DPS_INVALID_LSN_OFFSET;

      return;
   }

   INT32 pageAccessor::validateMMapPageHeadAndTail()
   {
      INT32 rc = SDB_OK;
      const pageHead *head = NULL;
      UINT64 tail = 0;

      rc = getReadPtrOfHead(&head);
      if (SDB_OK != rc)
      {
         goto error;
      }

      rc = readTail(tail);
      if (SDB_OK != rc)
      {
         goto error;
      }

      if (head->lsn != tail ||
          INVALID_PAGE_TYPE == head->type ||
          INVALID_PAGE_VERSION == head->version ||
          getPageType() != head->type ||
          !head->inUsed())
      {
         PD_LOG(PDERROR, "page[%s] head crashed", _gpid.toString().c_str());
         rc = SDB_VESSEL_PAGE_CRASHED;
         goto error;
      }

   done:
      return rc;
   error:
      goto done;
   }

   INT32 pageAccessor::beginToAccess(requestContext *context,
                                     storageUnit *su)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(ACCESSOR_STATUS_INVALID == _status, "impossible");
      SDB_ASSERT(NULL != su, "can not be null");
      
      if (isCacheMode())
      {
         rc = beginToAccessByCache(context);
         if (SDB_OK != rc)
         {
            goto error;
         }
      }
      else
      {
         rc = beginToAccessByMMap(su);
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

   INT32 pageAccessor::beginToAccessByMMap(storageUnit *su)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(NULL != su, "can not be null");
      SDB_ASSERT(ACCESSOR_STATUS_INVALID == _status, "impossible");
      ossValuePtr tmpPtr = 0;
      BOOLEAN pageValidation = TRUE;

      if (0 == _ptr)
      {
         rc = su->getPagePtr(_gpid.type(), _gpid.page(), tmpPtr);
         if (SDB_OK != rc)
         {
            goto error;
         }
         _ptr = tmpPtr;
      }

      _status = ACCESSOR_STATUS_READONLY_ACCESSING;
      pageValidation = (0 == OSS_BIT_TEST(_flags, PAGE_ACCESSOR_FLAG_NO_PAGE_VALIDATION));
      if (pageValidation)
      {
         rc = validateMMapPageHeadAndTail();
         if (SDB_OK != rc)
         {
            goto error;
         }
      }

   done:
      return rc;
   error:
      _status = ACCESSOR_STATUS_INVALID;
      goto done;
   }

   INT32 pageAccessor::beginToAccessByCache(requestContext *context)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(isCacheMode(), "impossible");
      SDB_ASSERT(!_lcTuple.valid(), "do not reinit");
      SDB_ASSERT(ACCESSOR_STATUS_INVALID == _status, "must be invalid");

      const CHAR *head = NULL;
      liteCacheAllocateOptions options;
      options.readonly = (0 == OSS_BIT_TEST(_flags, PAGE_ACCESSOR_FLAG_NON_READONLY));
      liteCache &cache = context->getEnv()->cache;
      rc = cache.allocate(context, _gpid, options, _lcTuple);
      if (SDB_OK != rc)
      {
         goto error;
      }

      rc = _lcTuple.getReadPtr(0, PAGE_HEAD_LEN, &head);
      if (SDB_OK != rc)
      {
         goto error;
      }

      _status = ACCESSOR_STATUS_READONLY_ACCESSING;

      /// page checked when allocating tuple.
      if (getPageType() !=  ((const pageHead *)head)->type)
      {
         SDB_ASSERT(FALSE, "wrong type accessing");
         PD_LOG(PDERROR, "wrong page type. accessor type:%d, page type:%d",
                getPageType(), ((const pageHead *)head)->type);
         rc = SDB_VESSEL_INTERNAL_ERR;
         goto error;
      }
   done:
      return rc;
   error:
      if (_lcTuple.valid())
      {
         cache.release(context, _lcTuple);
      }
      _status = ACCESSOR_STATUS_INVALID;
      goto done;
   }

   INT32 pageAccessor::prepareToWriteByCache(requestContext *context)
   {
      INT32 rc = SDB_OK;
      rc = _lcTuple.prepareToWrite(context);
      if (SDB_OK != rc)
      {
         goto error;
      }
   done:
      return rc;
   error:
      goto done;
   }

   INT32 pageAccessor::initCommonPageHeadAndTail(PAGE_ID lpid)
   {
      INT32 rc = SDB_OK;
      pageHead *head = NULL;
      CHAR e0 = 0;
      CHAR e1 = 0;

      if (OSS_UNLIKELY(!fullAccessing()))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      getEyeCatcher(getPageType(), e0, e1);

      rc = getWritePtrOfHead(&head);
      if (SDB_OK != rc)
      {
         goto error;
      }

      head->eyeCatcher[0] = e0;
      head->eyeCatcher[1] = e1;
      head->version = PAGE_VERSION_1;
      head->type = getPageType();
      head->flags = 0;
      head->setInUsed();
      head->size = _pageSize;
      head->pageID = INVALID_PAGE_ID == lpid ? _gpid.page() : lpid;
      head->lsn = DPS_INVALID_LSN_OFFSET;
      head->pad = 0;
      head->pad2 = 0;

      rc = writeTail(head->lsn);
      if (SDB_OK != rc)
      {
         goto error;
      }

   done:
      return rc;
   error:
      goto done;
   }

   INT32 pageAccessor::writeTail(UINT64 v)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(fullAccessing(), "impossible");
      CHAR *tail = NULL;
      if (!isCacheMode())
      {
         rc = getMMapWritePtrOfPage(_pageSize - PAGE_TAIL_LEN, PAGE_TAIL_LEN, &tail);
         if (SDB_OK != rc)
         {
            goto error;
         }
      }
      else
      {
         rc = _lcTuple.getWritePtr(_pageSize - PAGE_TAIL_LEN, PAGE_TAIL_LEN, &tail);
         if (SDB_OK != rc)
         {
            goto error;
         }
      }
      *((UINT64 *)tail) = v;
      
   done:
      return rc;
   error:
      goto done;
   }

   INT32 pageAccessor::readTail(UINT64 &value)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(accessing(), "impossible");
      const CHAR *tail = NULL;
      if (!isCacheMode())
      {
         rc = getMMapReadPtrOfPage(_pageSize - PAGE_TAIL_LEN, PAGE_TAIL_LEN, &tail);
         if (SDB_OK != rc)
         {
            goto error;
         }
      }
      else
      {
         rc = _lcTuple.getReadPtr(_pageSize - PAGE_TAIL_LEN, PAGE_TAIL_LEN, &tail);
          if (SDB_OK != rc)
         {
            goto error;
         }
      }
      value = *((const UINT64 *)tail);
   done:
      return rc;
   error:
      goto done;
   }

   INT32 pageAccessor::getReadPtrOfHead(const pageHead **head)
   {
      INT32 rc = SDB_OK;
      const CHAR *ptr = NULL;
      if (OSS_UNLIKELY(!accessing()))
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }
      else if (OSS_UNLIKELY(NULL == head))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      else if (isCacheMode())
      {
         rc = _lcTuple.getReadPtr(0, PAGE_HEAD_LEN, &ptr);
         if (SDB_OK != rc)
         {
            goto error;
         }
      }
      else
      {
         rc = getMMapReadPtrOfPage(0, PAGE_HEAD_LEN, &ptr);
         if (SDB_OK != rc)
         {
            goto error;
         }  
      }

      *head = (const pageHead *)ptr;
      
   done:
      return rc;
   error:
      goto done;
   }

   INT32 pageAccessor::getWritePtrOfHead(pageHead **head)
   {
      INT32 rc = SDB_OK;
      CHAR *ptr = NULL;
      if (OSS_UNLIKELY(!fullAccessing()))
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }
      else if (OSS_UNLIKELY(NULL == head))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      else if (isCacheMode())
      {
         rc = _lcTuple.getWritePtr(0, PAGE_HEAD_LEN, &ptr);
         if (SDB_OK != rc)
         {
            goto error;
         }
         
      }
      else
      {
         rc = getMMapWritePtrOfPage(0, PAGE_HEAD_LEN, &ptr);
         if (SDB_OK != rc)
         {
            goto error;
         }
      }

      *head = (pageHead *)ptr;
      
   done:
      return rc;
   error:
      goto done;
   }

   INT32 pageAccessor::getWritePtrOfPageBody(UINT32 offset, UINT32 len, CHAR **ptr)
   {
      INT32 rc = SDB_OK;
      if (OSS_UNLIKELY(!fullAccessing()))
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }
      else if (OSS_UNLIKELY(NULL == ptr))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      else if (isCacheMode())
      {
         rc = _lcTuple.getWritePtr(offset + PAGE_HEAD_LEN, len, (CHAR **)ptr);
         if (SDB_OK != rc)
         {
            goto error;
         }
      }
      else
      {
         rc = getMMapWritePtrOfPage(offset + PAGE_HEAD_LEN, len + PAGE_TAIL_LEN, ptr);
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

   INT32 pageAccessor::getReadPtrOfPageBody(UINT32 offset, UINT32 len, const CHAR **ptr)
   {
      INT32 rc = SDB_OK;
      if (OSS_UNLIKELY(!accessing()))
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }
      else if (OSS_UNLIKELY(NULL == ptr))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      else if (isCacheMode())
      {
         rc = _lcTuple.getReadPtr(PAGE_HEAD_LEN + offset, len + PAGE_TAIL_LEN, ptr);
         if (SDB_OK != rc)
         {
            goto error;
         }
      }
      else
      {
         rc = getMMapReadPtrOfPage(PAGE_HEAD_LEN + offset, len + PAGE_TAIL_LEN, ptr);
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

   INT32 pageAccessor::readPageBody(UINT32 offset, UINT32 len, CHAR *buf)
   {
      INT32 rc = SDB_OK;
      if (OSS_UNLIKELY(!accessing()))
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }
      else if(OSS_UNLIKELY(NULL == buf))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      else if (isCacheMode())
      {
         rc =_lcTuple.read(offset + PAGE_HEAD_LEN, len, buf);
         if (SDB_OK != rc)
         {
            goto error;
         }
      }
      else
      {
         const CHAR *ptr = NULL;
         rc = getMMapReadPtrOfPage(offset + PAGE_HEAD_LEN, len + PAGE_TAIL_LEN, &ptr);
         if (SDB_OK != rc)
         {
            goto error;
         }

         ossMemcpy(buf, ptr, len);
      }
      
   done:
      return rc;
   error:
      goto done;
   }

   INT32 pageAccessor::writePageBody(UINT32 offset, UINT32 len, const CHAR *buf)
   {
      INT32 rc = SDB_OK;
      if (OSS_UNLIKELY(!accessing()))
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }
      else if (OSS_UNLIKELY(NULL == buf))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      else if (isCacheMode())
      {
         rc = _lcTuple.write(PAGE_HEAD_LEN + offset, len, buf);
         if (SDB_OK != rc)
         {
            goto error;
         }
      }
      else
      {
         CHAR *ptr = NULL;
         rc = getMMapWritePtrOfPage(PAGE_HEAD_LEN + offset, len + PAGE_TAIL_LEN, &ptr);
         if (SDB_OK != rc)
         {
            goto error;
         }
         ossMemcpy(ptr, buf, len);
      }
   done:
      return rc;
   error:
      goto done;
   }

   INT32 pageAccessor::getPidFromDisk(PAGE_ID &pid)
   {
      INT32 rc = SDB_OK;
      const pageHead *head = NULL;
      rc = getReadPtrOfHead(&head);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to read ptr of page head:%d", rc);
         goto error;
      }
      pid = head->pageID;
   done:
      return rc;
   error:
      goto done;
   }

   INT32 pageAccessor::getMMapWritePtrOfPage(UINT32 offset, UINT32 len, CHAR **ptr)
   {
      INT32 rc = SDB_OK;

      if (OSS_UNLIKELY(!validMMapPtr(offset, len)))
      {
         rc = SDB_VESSEL_INVALID_PTR_OFFSET;
         goto error;
      }
      else
      {
         *ptr = (CHAR *)(_ptr + offset);
      }
   done:
      return rc;
   error:
      goto done;
   }

   INT32 pageAccessor::getMMapReadPtrOfPage(UINT32 offset, UINT32 len, const CHAR **ptr)
   {
      INT32 rc = SDB_OK;
      
      if (OSS_UNLIKELY(!validMMapPtr(offset, len)))
      {
         rc = SDB_VESSEL_INVALID_PTR_OFFSET;
         goto error;
      }
      else
      {
         *ptr = (const CHAR *)(_ptr + offset);
      }
   done:
      return rc;
   error:
      goto done;
   }

   INT32 pageAccessor::memsetPageBody(CHAR v)
   {
      INT32 rc = SDB_OK;
      CHAR *ptr = NULL;
      UINT32 pageBodySize = 0;
      if (OSS_UNLIKELY(!fullAccessing()))
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }

      pageBodySize = getPageBodySize();
      if (OSS_UNLIKELY(0 == pageBodySize))
      {
         rc = SDB_VESSEL_INTERNAL_ERR;
         goto error;
      }

      if (!isCacheMode())
      {
         rc = getWritePtrOfPageBody(0, pageBodySize, &ptr);
         if (SDB_OK != rc)
         {
            goto error;
         }

         ossMemset(ptr, v, pageBodySize);
      }
      else
      {
         rc = SDB_VESSEL_OPERATOION_NOT_PERMITTED;
         goto error;
      }
   done:
      return rc;
   error:
      goto done;
   }

   BOOLEAN pageAccessor::accessing()const
   {
      return ACCESSOR_STATUS_READONLY_ACCESSING == _status ||
             ACCESSOR_STATUS_FULL_ACCESSING == _status;
   }

   BOOLEAN pageAccessor::fullAccessing()const
   {
      return ACCESSOR_STATUS_FULL_ACCESSING == _status;
   }

   UINT32 pageAccessor::getPageBodySize()const
   {
      return engine::vessel::getPageBodySize(_pageSize);
   }

   INT32 pageAccessor::prepareLogDone(requestContext *context,
                                      logRecordContext *lrc)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(accessing(), "must be accessing");
      SDB_ASSERT(fullAccessing(), "should prepare writing first");
      SDB_ASSERT(isCacheMode(), "must be cache mode");
      SDB_ASSERT(NULL != lrc, "can not be null");
      SDB_ASSERT(!lrc->needFullDump(), "can not be full dump");
      SDB_ASSERT(!lrc->prepared(), "can not be prepared");

      ISession *session = context->getSession();
      IRedoLogger *logger = context->getOuterResource()->logger;
      DPS_LSN_OFFSET lsn = DPS_INVALID_LSN_OFFSET;
      const checkpointController *checkpointer = NULL;
      const openDBOptions &options = context->getEnv()->options;
      dpsLogRecordHeader &head = lrc->getHead();

      OSS_BIT_SET(head._flags, DPS_VESSEL_LOG_FLAG_FROM_VESSEL);

      if (isOplistHead())
      {
         OSS_BIT_SET(head._flags, DPS_VESSEL_LOG_FLAG_OP_HEAD);
      }
      else if (isInOplist())
      {
         SDB_ASSERT(DPS_INVALID_LSN_OFFSET != getOplist(), "impossible");
         head._opListLSN = getOplist();
      }

      /// not else if
      if (isOplistTail())
      {
         SDB_ASSERT(DPS_INVALID_LSN_OFFSET != head._opListLSN, "impossible");
         OSS_BIT_SET(head._flags, DPS_VESSEL_LOG_FLAG_OP_TAIL);
      }

      if (options.fullDumpPageLog)
      {
         rc = _lcTuple.getMinLSN(lsn);
         if (SDB_OK != rc)
         {
            goto error;
         }

         if (DPS_INVALID_LSN_OFFSET == lsn)
         {
            dpsLogRecordHeader &head = lrc->getHead();
            setFlags(head, DPS_VESSEL_LOG_FLAG_LAST_LSN_IS_INVALID);
         }
         else
         {
            checkpointer = &(context->getEnv()->checkpointer);
            if (lsn <= checkpointer->getLastCheckpointLSN())
            {
               const CHAR *pagePtr = NULL;
               rc = _lcTuple.getReadPtr(0, _pageSize, &pagePtr);
               if (SDB_OK != rc)
               {
                  PD_LOG(PDERROR, "failed to get page ptr:%d", rc);
                  goto error;
               }

               rc = lrc->fullDumpPage(_pageSize, pagePtr);
               if (SDB_OK != rc)
               {
                  PD_LOG(PDERROR, "failed to dump page to log:%d", rc);
                  goto error;
               }

               lrc->prepush(_pageSize);
            }
         }
      }

      lrc->prepushDone();
      rc = logger->prepare(session, lrc);
      if (SDB_OK != rc)
      {
         goto error;
      }

      if (pageAccessor::isOplistHead())
      {
         SDB_ASSERT(DPS_INVALID_LSN_OFFSET != lrc->getHead()._opListLSN, "impossible");
         _oplist = lrc->getHead()._opListLSN;
      }
   done:
      return rc;
   error:
      goto done;
   }

}//namespace vessel
}//namespace engine
