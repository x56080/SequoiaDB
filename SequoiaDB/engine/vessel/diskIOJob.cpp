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
#include "vessel/liteCachePageTag.h"
#include "ossLikely.hpp"
#include <algorithm>

namespace engine
{
namespace vessel
{
   static const UINT32 DEFAULT_MAX_IO_SIZE_PER_TASK = 4 * 1024 * 1024;
   static const UINT32 DEFUALT_BUF_COUNT = 128;

   BOOLEAN compareTag(const liteCachePageTag*l, const liteCachePageTag*r)
   {
      return l->id() < r->id();
   }


   diskIOJob::diskIOJob():
   _status(NONE),
   _jobID(0),
   _jobType(DIRTY_LIST),
   _dispatchedCount(0),
   _maxIOSizePerTask(DEFAULT_MAX_IO_SIZE_PER_TASK)
   {
   }

   diskIOJob::~diskIOJob()
   {
      
   }

   void diskIOJob::reset()
   {
      if (NONE != _status)
      {
         _status = NONE;
         _jobID = 0;
         _jobType = DIRTY_LIST;
         _dispatchedCount = 0;
         _maxIOSizePerTask = DEFAULT_MAX_IO_SIZE_PER_TASK;
         _tags.clear();
      }
      return;
   }


   void diskIOJob::prepare(UINT64 jobID, TYPE type, UINT32 bufSize)
   {
      UINT32 initBufSize = 0 < bufSize ? bufSize : DEFUALT_BUF_COUNT;
      SDB_ASSERT(NONE == _status, "must be none");

      _tags.reserve(initBufSize);
      _jobID = jobID;
      _status = PENDING;
      _jobType = type;
      return;
   }

   INT32 diskIOJob::addPendingWriteTag(liteCachePageTag *tag)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(NULL != tag, "can not be null");
      SDB_ASSERT(tag->isPendingWrite(), "must be pending");

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

      _tags.push_back(tag);
   done:
      return rc;
   error:
      goto done;
   }

   void diskIOJob::abortUndispatchedTasks()
   {
      if (PENDING == _status)
      {
         for (UINT32 i = 0; i < _tags.size(); ++i)
         {
            releaseTag(i);
         }
         reset();
      }
      if (DISPATCHING == _status)
      {
         if (_dispatchedCount < _tags.size())
         {
            for (UINT32 i = _dispatchedCount; i < _tags.size(); ++i)
            {
               releaseTag(i);
            }
            _tags.resize(_dispatchedCount);
         }
      }
      return;
   }

   void diskIOJob::prepareForDispatching(UINT32 maxIOSizePerTask)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(PENDING == _status, "must be pendding");

      if (0 < maxIOSizePerTask)
      {
         _maxIOSizePerTask = maxIOSizePerTask;
      }

      if ((1 < _tags.size()) &&
          (LRU_LIST == _jobType || DIRTY_LIST == _jobType))
      {
         std::sort(_tags.begin(), _tags.end(), compareTag);
      }

      _status = DISPATCHING;
      _dispatchedCount = 0;
      return;
   }

   void diskIOJob::releaseTag(UINT32 pos)
   {
      if (pos < _tags.size())
      {
         liteCachePageTag*tag = _tags[pos];
         if (NULL != tag)
         {
            tag->setUnPendingWrite();
         }
         _tags[pos] = NULL;
      }
      return;
   }


   void diskIOJob::releaseTagsWhenTaskDone(const diskIOTask *task)
   {
      SDB_ASSERT(NULL != task && task->getJob() == this, "impossible");
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
      const liteCachePageTag*tag = NULL;
      UINT32 taskID = 0;

      if (OSS_UNLIKELY(DISPATCHING != _status))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      else if (_tags.size() == _dispatchedCount)
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
      ioSize = tag->getPageSize();

      while (_dispatchedCount < _tags.size()  &&
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
            ioSize += tag->getPageSize();
         }
         else
         {
            break;
         }
      }

      task = diskIOTask(taskID, count, this);
   done:
      return rc;
   error:
      goto done;
   }
}//namespace vessel
}//namespace engine