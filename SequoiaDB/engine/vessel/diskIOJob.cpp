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

   Source File Name = diskIOJob.cpp

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

#include "vessel/diskIOJob.h"
#include "pdTrace.hpp"
#include "vessel/lcExtentTag.h"
#include "ossLikely.hpp"
#include <algorithm>

namespace engine
{
namespace vessel
{
   static const UINT32 DEFAULT_MAX_IO_SIZE_PER_TASK = 4 * 1024 * 1024;
   static const UINT32 DEFUALT_BUF_COUNT = 128;

   BOOLEAN compareTag(const lcExtentTag *l, const lcExtentTag *r)
   {
      return l->id() < r->id();
   }


   diskIOJob::diskIOJob():
   _status(NONE),
   _jobID(0),
   _jobType(DIRTY_LIST),
   _bufCount(0),
   _pageCount(0),
   _dispatchedCount(0),
   _maxIOSizePerTask(DEFAULT_MAX_IO_SIZE_PER_TASK),
   _tags(NULL)
   {
   }

   diskIOJob::~diskIOJob()
   {
      SAFE_OSS_FREE(_tags);
   }

   void diskIOJob::reset()
   {
      if (NONE != _status)
      {
         _status = NONE;
         _jobID = 0;
         _jobType = DIRTY_LIST;
         _bufCount = 0;
         _pageCount = 0;
         _dispatchedCount = 0;
         _maxIOSizePerTask = DEFAULT_MAX_IO_SIZE_PER_TASK;
         SAFE_OSS_FREE(_tags);
      }
      return;
   }


   INT32 diskIOJob::prepare(UINT64 jobID, TYPE type, UINT32 bufCount)
   {
      INT32 rc = SDB_OK;
      UINT32 initBufCount = 0 < bufCount ? bufCount : DEFUALT_BUF_COUNT;
      if (OSS_UNLIKELY(NONE != _status))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      rc = extentBufTo(initBufCount);
      if (SDB_OK != rc)
      {
         goto error;
      }

      _jobID = jobID;
      _status = PENDING;
      _jobType = type;
   done:
      return rc;
   error:
      goto done;
   }

   INT32 diskIOJob::addPendingWriteTag(lcExtentTag *tag)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(NULL != tag, "can not be null");
      SDB_ASSERT(tag->pendingWrite(), "must be pending");

      if (OSS_UNLIKELY(NULL == tag))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      else if (OSS_UNLIKELY(PENDING != _status))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      else if (OSS_UNLIKELY(DIRTY_LIST != _jobType &&
                            LRU_LIST != _jobType))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      else if (_bufCount == _pageCount)
      {
         rc = extentBufTo(2 * _bufCount);
         if (SDB_OK != rc)
         {
            goto error;
         }
      }

      _tags[_pageCount++] = tag;
   done:
      return rc;
   error:
      goto done;
   }

   INT32 diskIOJob::abort()
   {
      INT32 rc = SDB_OK;
      if (PENDING == _status &&
          (DIRTY_LIST == _jobType || LRU_LIST == _jobType))
      {
         for (UINT32 i = 0; i < _pageCount; ++i)
         {
            releaseTag(i);
         }
         reset();
      }
      else if (DISPATCHING == _status &&
               (DIRTY_LIST == _jobType || LRU_LIST == _jobType))
      {
         for (UINT32 i = _dispatchedCount; i < _pageCount; ++i)
         {
            releaseTag(i);
         }
         _pageCount = _dispatchedCount;
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

   INT32 diskIOJob::allTaskDone(BOOLEAN &r)const
   {
      INT32 rc = SDB_OK;
      r = FALSE;
      if (OSS_UNLIKELY(DISPATCHING != _status))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      if (_dispatchedCount < _pageCount)
      {
         goto done;
      }

      for (UINT32 i = 0; i < _pageCount; ++i)
      {
         if (NULL != _tags[i])
         {
            goto done;
         }
      }

      r = TRUE;
      
   done:
      return rc;
   error:
      goto done;
   }

   INT32 diskIOJob::prepareForDispatching(UINT32 maxIOSizePerTask)
   {
      INT32 rc = SDB_OK;
      if (OSS_UNLIKELY(PENDING != _status))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      if (0 < maxIOSizePerTask)
      {
         _maxIOSizePerTask = maxIOSizePerTask;
      }

      if ((1 < _pageCount) &&
          (LRU_LIST == _jobType || DIRTY_LIST == _jobType))
      {
         std::sort(_tags, _tags + _pageCount, compareTag);
      }

      _status = DISPATCHING;
      _dispatchedCount = 0;
   done:
      return rc;
   error:
      goto done;
   }

   void diskIOJob::releaseTag(UINT32 pos)
   {
      if (pos < _pageCount)
      {
         lcExtentTag *tag = _tags[pos];
         if (NULL != tag)
         {
            tag->setUnPendingWrite();
         }
         _tags[pos] = NULL;
      }
      return;
   }

   void diskIOJob::releaseDispatchedTask(const diskIOTask *task)
   {
      if (OSS_LIKELY(NULL != task && task->getJob() == this))
      {
         UINT32 taskID = task->getTaskID();
         UINT32 count = task->getPageCount();
         SDB_ASSERT(taskID + count <= _dispatchedCount, "impossible");
         for (UINT32 i = 0; i < count && (i+taskID) < _dispatchedCount; ++i)
         {
            releaseTag(taskID + i);
         }
      }

      return;
   }

   INT32 diskIOJob::getNextTask(diskIOTask &task)
   {
      INT32 rc = SDB_OK;
      GLOBAL_PAGE_ID gpid;
      GLOBAL_PAGE_ID pre;
      UINT32 count = 0;
      UINT32 ioSize = 0;
      const lcExtentTag *tag = NULL;
      UINT32 taskID = 0;

      if (OSS_UNLIKELY(DISPATCHING != _status))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      else if (_pageCount == _dispatchedCount)
      {
         rc = SDB_VESSEL_END_OF_CURSOR;
         goto error;
      }
      
      taskID = _dispatchedCount;
      tag = _tags[_dispatchedCount++];
      gpid = tag->id();
      SDB_ASSERT(!gpid.invalid(), "can not be invalid");
      count = 1;
      pre = gpid;
      ioSize = tag->getDiskPageSize();

      while (_dispatchedCount < _pageCount &&
             ioSize < _maxIOSizePerTask)
      {
         tag = _tags[_dispatchedCount];
         if (tag->id().space() == pre.space() &&
             tag->id().type() == pre.type() &&
             tag->id().page() == pre.page() + 1)
         {
            ++count;
            pre = tag->id();
            ++_dispatchedCount;
            ioSize += tag->getDiskPageSize();
         }
         else
         {
            break;
         }
      }

      task.setup(this, taskID, count);
   done:
      return rc;
   error:
      goto done;
   }

   INT32 diskIOJob::extentBufTo(UINT32 count)
   {
      INT32 rc = SDB_OK;
      if (_bufCount < count)
      {
         UINT32 totalSize = sizeof(lcExtentTag *) * (count + 1);
         lcExtentTag **tmp = (lcExtentTag **)SDB_OSS_MALLOC(totalSize);
         if (NULL == tmp)
         {
            PD_LOG(PDERROR, "failed to allocate mem");
            rc = SDB_OOM;
            goto error;
         }

         if (0 < _bufCount)
         {
            ossMemcpy(tmp, _tags, sizeof(lcExtentTag*)*_bufCount);
            SDB_OSS_FREE(_tags);
            _tags = NULL;
         }

         _tags = tmp;
         _bufCount = count;
      }
   done:
      return rc;
   error:
      goto done;
   }

}//namespace vessel
}//namespace engine