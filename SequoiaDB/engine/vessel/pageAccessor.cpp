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

#include "vessel/pageAccessor.h"
#include "ossLikely.hpp"
#include "vessel/requestContext.h"
#include "vessel/instanceEnv.h"
#include "vessel/extentStorageUnit.h"
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
   UINT32 ACCESSOR_STATUS_INVALID = 0;
   UINT32 ACCESSOR_STATUS_SETUP = 1;
   UINT32 ACCESSOR_STATUS_READONLY_ACCESSING = 2;
   UINT32 ACCESSOR_STATUS_FULL_ACCESSING = 4;

   pageAccessor::~pageAccessor()
   {
      teardown();
   }

   INT32 pageAccessor::setup(requestContext *context,
                             SPACE_TYPE type,
                             PAGE_ID pid,
                             UINT32 flags,
                             extentStorageUnit *su)
   {
      INT32 rc = SDB_OK;
      extentStorageUnit *obj = su;
      UINT32 size = 0;
      BOOLEAN rollback = FALSE;

      if (OSS_UNLIKELY(ACCESSOR_STATUS_INVALID != _status))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      else if (OSS_UNLIKELY(INVALID_PAGE_ID == pid))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      else if (OSS_UNLIKELY(INVALID_SPACE_TYPE == type))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      else if (OSS_UNLIKELY(NULL == context))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      else if (OSS_UNLIKELY(!context->getSpaceIDLocked()))
      { 
         rc = SDB_INVALIDARG;
         goto error;  
      }

      if (NULL == obj)
      {
         rc = context->getEnv()->suContainer.getSUByContext(context, &obj);
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

      rc = obj->getCoreArgs(type, &size, NULL, NULL);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to get page size:%d", rc);
         goto error;
      }

      rollback = TRUE;
      _gpid.reset(context->getSpaceID(), type, pid);
      _context = context;
      _size = size;
      _status = ACCESSOR_STATUS_SETUP;
      _su = obj;

      rc = beginToAccess(flags);
      if (SDB_OK != rc)
      {
         goto error;
      }

      /// if some one add new code here,
      /// should release resources got in beginToAccess
   done:
      return rc;
   error:
      if (rollback)
      {
         teardown();
      }
      goto done;
   }

   INT32 pageAccessor::setup(requestContext *context,
                             SPACE_TYPE type,
                             PAGE_ID pid,
                             UINT32 pageSize,
                             ossValuePtr ptr,
                             BOOLEAN pageTypeCheck,
                             BOOLEAN readOnly)
   {
      INT32 rc = SDB_OK;
      BOOLEAN rollback = FALSE;
      UINT32 flags = PAGE_ACCESSOR_FLAG_DIRECT;

      if (OSS_UNLIKELY(ACCESSOR_STATUS_INVALID != _status))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      else if (OSS_UNLIKELY(INVALID_PAGE_ID == pid))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      else if (OSS_UNLIKELY(INVALID_SPACE_TYPE == type))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      else if (OSS_UNLIKELY(NULL == context))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      else if (OSS_UNLIKELY(!context->getSpaceIDLocked()))
      { 
         rc = SDB_INVALIDARG;
         goto error;  
      }
      else if (OSS_UNLIKELY(0 == pageSize))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      else if (OSS_UNLIKELY(0 == ptr))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      rollback = TRUE;
      _gpid.reset(context->getSpaceID(), type, pid);
      _context = context;
      _size = pageSize;
      _status = ACCESSOR_STATUS_SETUP;
      _ptr = ptr;

      if (!pageTypeCheck)
      {
         flags |= PAGE_ACCESSOR_FLAG_INIT_PAGE;
      }
      if (!readOnly)
      {
         flags |= PAGE_ACCESSOR_FLAG_NON_READONLY;
      }
      rc = beginToAccess(flags);
      if (SDB_OK != rc)
      {
         goto error;
      }
   done:
      return rc;
   error:
      if (rollback)
      {
         teardown();
      }
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

   INT32 pageAccessor::prepareToWrite()
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(ACCESSOR_STATUS_READONLY_ACCESSING == _status, "impossible");

      if (OSS_UNLIKELY(ACCESSOR_STATUS_READONLY_ACCESSING != _status))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      else if (OSS_UNLIKELY(!OSS_BIT_TEST(_flags, PAGE_ACCESSOR_FLAG_NON_READONLY)))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      if (!OSS_BIT_TEST(_flags, PAGE_ACCESSOR_FLAG_DIRECT))
      {
         rc = prepareToWriteByCache();
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

   void pageAccessor::endToAccess()
   {
      if (accessing())
      {
         if (OSS_BIT_TEST(_flags, PAGE_ACCESSOR_FLAG_DIRECT))
         {
            endToAccessByMMap();
         }
         else
         {
            endToAccessByCache();
         }
         _status = ACCESSOR_STATUS_SETUP;
      }

      return;
   }

   void pageAccessor::endToAccessByMMap()
   {
      SDB_ASSERT(OSS_BIT_TEST(_flags, PAGE_ACCESSOR_FLAG_DIRECT), "must be direct");
      return;
   }

   void pageAccessor::endToAccessByCache()
   {
      SDB_ASSERT(!OSS_BIT_TEST(_flags, PAGE_ACCESSOR_FLAG_DIRECT), "can not be direct");
      liteCache &cache = getContext()->getEnv()->cache;
      if (_lcTuple.valid())
      {
         cache.release(getContext(), _lcTuple);
      }
      return;
   }

   void pageAccessor::commit(DPS_LSN_OFFSET lsn)
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

      if (!OSS_BIT_TEST(_flags, PAGE_ACCESSOR_FLAG_DIRECT))
      {
         liteCache &cache = getContext()->getEnv()->cache;
         cache.commit(getContext(), lsn, _lcTuple);
      }
      else
      {
         /// do nothing.
      }

      _status = ACCESSOR_STATUS_READONLY_ACCESSING;
   done:
      return;
   }

   void pageAccessor::teardown()
   {
      SDB_ASSERT(!fullAccessing(), "writing prepared but no commit or abort");
      endToAccess();

      if (ACCESSOR_STATUS_INVALID != _status)
      {
         if (NULL != _fullDumpBuf)
         {
            _context->releaseBuffer(_fullDumpBuf, _size);
         }
         _gpid.reset();
         _size = 0;
         _flags = 0;
         _status = ACCESSOR_STATUS_INVALID;
         _ptr = 0;
         _su = NULL;
         _fullDumpBuf = NULL;
         _context = NULL;
      }

      return;
   }

   INT32 pageAccessor::validateMMapPageHeadAndTail()
   {
      INT32 rc = SDB_OK;
      const pageHead *head = NULL;
      UINT64 tail = 0;--

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

   INT32 pageAccessor::beginToAccess(UINT32 flags)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(ACCESSOR_STATUS_SETUP == _status, "impossible");

      if (OSS_UNLIKELY(ACCESSOR_STATUS_SETUP != _status))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      _flags = flags;
      if (OSS_BIT_TEST(_flags, PAGE_ACCESSOR_FLAG_DIRECT))
      {
         rc = beginToAccessByMMap();
         if (SDB_OK != rc)
         {
            goto error;
         }
      }
      else
      {
         rc = beginToAccessByCache();
         if (SDB_OK != rc)
         {
            goto error;
         }
      }
   done:
      return rc;
   error:
      _flags = 0;
      goto done;
   }

   INT32 pageAccessor::beginToAccessByMMap()
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(ACCESSOR_STATUS_SETUP == _status, "impossible");
      SDB_ASSERT(OSS_BIT_TEST(_flags, PAGE_ACCESSOR_FLAG_DIRECT), "must be direct");
      ossValuePtr tmpPtr = 0;
      if (0 == _ptr)
      {
         rc = _su->getPagePtr(_gpid.type(), _gpid.page(), tmpPtr);
         if (SDB_OK != rc)
         {
            goto error;
         }
         _ptr = tmpPtr;
      }

      _status = ACCESSOR_STATUS_READONLY_ACCESSING;
      if (!OSS_BIT_TEST(_flags, PAGE_ACCESSOR_FLAG_INIT_PAGE))
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
      _status = ACCESSOR_STATUS_SETUP;
      _ptr = 0;
      goto done;
   }

   INT32 pageAccessor::beginToAccessByCache()
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(!OSS_BIT_TEST(_flags, PAGE_ACCESSOR_FLAG_DIRECT), "can not be direct");
      const pageHead *head = NULL;
      liteCacheAllocateOptions options;
      options.readonly = !OSS_BIT_TEST(_flags, PAGE_ACCESSOR_FLAG_NON_READONLY);
      liteCache &cache = getContext()->getEnv()->cache;
      rc = cache.allocate(getContext(), _gpid, options, _lcTuple);
      if (SDB_OK != rc)
      {
         goto error;
      }

      _status = ACCESSOR_STATUS_READONLY_ACCESSING;

      rc = getReadPtrOfHead(&head);
      if (SDB_OK != rc)
      {
         goto error;
      }

      /// page checked when allocating tuple.
      if (getPageType() !=  head->type)
      {
         SDB_ASSERT(FALSE, "wrong type accessing");
         PD_LOG(PDERROR, "wrong page type. accessor type:%d, page type:%d", getPageType(), head->type);
         rc = SDB_VESSEL_INTERNAL_ERR;
         goto error;
      }
   done:
      return rc;
   error:
      _status = ACCESSOR_STATUS_SETUP;
      if (_lcTuple.valid())
      {
         cache.release(getContext(), _lcTuple);
      }
      goto done;
   }

   INT32 pageAccessor::prepareToWriteByCache()
   {
      INT32 rc = SDB_OK;
      rc = _lcTuple.prepareToWrite(getContext());
      if (SDB_OK != rc)
      {
         goto error;
      }
   done:
      return rc;
   error:
      goto done;
   }

   INT32 pageAccessor::initCommonPageHeadAndTail()
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

      getPageEyeCatcher(_gpid.type(), e0, e1);

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
      head->size = _size;
      head->pageID = _gpid.page();
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
      if (OSS_BIT_TEST(_flags, PAGE_ACCESSOR_FLAG_DIRECT))
      {
         CHAR *tmp = NULL;
         rc = getMMapWritePtrOfPage(_size - PAGE_TAIL_LEN, PAGE_TAIL_LEN, &tmp);
         if (SDB_OK != rc)
         {
            goto error;
         }
         *((UINT64 *)tmp) = v;
      }
      else
      {
         
         rc = _lcTuple.write(_size - PAGE_TAIL_LEN, PAGE_TAIL_LEN, (const CHAR *)&v);
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

   INT32 pageAccessor::readTail(UINT64 &value)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(accessing(), "impossible");
      if (OSS_BIT_TEST(_flags, PAGE_ACCESSOR_FLAG_DIRECT))
      {
         const CHAR *tmp = NULL;
         rc = getMMapReadPtrOfPage(_size - PAGE_TAIL_LEN, PAGE_TAIL_LEN, &tmp);
         if (SDB_OK != rc)
         {
            goto error;
         }
         value = *((const UINT64 *)tmp);
      }
      else
      {
         UINT64 tmp = 0;
         rc = _lcTuple.read(_size - PAGE_TAIL_LEN, PAGE_TAIL_LEN, (CHAR*)&tmp);
          if (SDB_OK != rc)
         {
            goto error;
         }
         value = tmp;
      }
      
   done:
      return rc;
   error:
      goto done;
   }

   INT32 pageAccessor::getReadPtrOfHead(const pageHead **head)
   {
      INT32 rc = SDB_OK;
      const CHAR *ptr = NULL;
      if (OSS_UNLIKELY(!accessing() || NULL == head))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      else if (OSS_BIT_TEST(_flags, PAGE_ACCESSOR_FLAG_DIRECT))
      {
         rc = getMMapReadPtrOfPage(0, PAGE_HEAD_LEN, &ptr);
         if (SDB_OK != rc)
         {
            goto error;
         }
      }
      else
      {
         rc = _lcTuple.getReadPtr(0, PAGE_HEAD_LEN, &ptr);
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
      if (OSS_UNLIKELY(!fullAccessing() || NULL == head))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      else if (OSS_BIT_TEST(_flags, PAGE_ACCESSOR_FLAG_DIRECT))
      {
         
         rc = getMMapWritePtrOfPage(0, PAGE_HEAD_LEN, &ptr);
         if (SDB_OK != rc)
         {
            goto error;
         }
      }
      else
      {
         rc = _lcTuple.getWritePtr(0, PAGE_HEAD_LEN, &ptr);
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
      if (OSS_UNLIKELY(!fullAccessing() || NULL == ptr))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      else if (OSS_BIT_TEST(_flags, PAGE_ACCESSOR_FLAG_DIRECT))
      {
         rc = getMMapWritePtrOfPage(offset + PAGE_HEAD_LEN, len + PAGE_TAIL_LEN, ptr);
         if (SDB_OK != rc)
         {
            goto error;
         }
      }
      else
      {
         rc = _lcTuple.getWritePtr(offset + PAGE_HEAD_LEN, len, (CHAR **)ptr);
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
      if (OSS_UNLIKELY(!accessing() || NULL == ptr))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      else if (OSS_BIT_TEST(_flags, PAGE_ACCESSOR_FLAG_DIRECT))
      {
         rc = getMMapReadPtrOfPage(PAGE_HEAD_LEN + offset, len + PAGE_TAIL_LEN, ptr);
         if (SDB_OK != rc)
         {
            goto error;
         }
      }
      else
      {
         rc = _lcTuple.getReadPtr(PAGE_HEAD_LEN + offset, len + PAGE_TAIL_LEN, ptr);
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
         rc = SDB_INVALIDARG;
         goto error;
      }
      else if (OSS_BIT_TEST(_flags, PAGE_ACCESSOR_FLAG_DIRECT))
      {
         const CHAR *ptr = NULL;
         rc = getMMapReadPtrOfPage(offset + PAGE_HEAD_LEN, len + PAGE_TAIL_LEN, &ptr);
         if (SDB_OK != rc)
         {
            goto error;
         }

         ossMemcpy(buf, ptr, len);
      }
      else
      {
         rc =_lcTuple.read(offset + PAGE_HEAD_LEN, len, buf);
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

   INT32 pageAccessor::writePageBody(UINT32 offset, UINT32 len, const CHAR *buf)
   {
      INT32 rc = SDB_OK;
      if (OSS_UNLIKELY(!accessing()))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      else if (OSS_BIT_TEST(_flags, PAGE_ACCESSOR_FLAG_DIRECT))
      {
         CHAR *ptr = NULL;
         rc = getMMapWritePtrOfPage(PAGE_HEAD_LEN + offset, len + PAGE_TAIL_LEN, &ptr);
         if (SDB_OK != rc)
         {
            goto error;
         }
         ossMemcpy(ptr, buf, len);
      }
      else
      {
         rc = _lcTuple.write(PAGE_HEAD_LEN + offset, len, buf);
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
         rc = SDB_INVALIDARG;
         goto error;
      }

      pageBodySize = getPageBodySize();

      if (OSS_BIT_TEST(_flags, PAGE_ACCESSOR_FLAG_DIRECT))
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
         rc = SDB_INVALIDARG;
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

   INT32 pageAccessor::prepareFullDumpLogWhenNecessary(logRecordContext *lrc)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(accessing(), "must be accessing");
      SDB_ASSERT(!fullAccessing(), "can not write any thing before fulldump");
      SDB_ASSERT(!OSS_BIT_TEST(_flags, PAGE_ACCESSOR_FLAG_DIRECT), "must be cache");
      SDB_ASSERT(NULL != lrc, "can not be null");
      SDB_ASSERT(!lrc->needFullDump(), "can not be full dump");
      SDB_ASSERT(!lrc->prepared(), "can not be prepared");
      SDB_ASSERT(NULL == _fullDumpBuf, "must be null");

      DPS_LSN_OFFSET lsn = DPS_INVALID_LSN_OFFSET;
      const checkpointController *checkpointer = NULL;
      const openDBOptions &options = getContext()->getEnv()->options;
      if (!options.fullDumpPageLog)
      {
         goto done;
      }
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
         checkpointer = &(getContext()->getEnv()->checkpointer);
         if (lsn <= checkpointer->getLastCheckpointLSN())
         {
            _fullDumpBuf = getContext()->allocateBuffer(_size);
            if (NULL == _fullDumpBuf)
            {
               rc = SDB_OOM;
               goto error;
            }
            rc = _lcTuple.read(0, _size, _fullDumpBuf);
            if (SDB_OK != rc)
            {
               goto error;
            }
            lrc->setNeedFullDump();
            lrc->prepush(_size);
         }
      }
      
   done:
      return rc;
   error:
      goto done;
   }

}//namespace vessel
}//namespace engine