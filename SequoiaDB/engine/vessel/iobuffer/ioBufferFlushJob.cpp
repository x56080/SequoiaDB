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

   Source File Name = ioBufferFlushJob.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

*******************************************************************************/
#include "vessel/ioBufferFlushJob.h"
#include "pdTrace.hpp"
#include "vessel/threadContext.h"
#include "vessel/instanceEnv.h"

namespace engine
{
namespace vessel
{
   void ioBufferFlushJob::build(const SHARED_IO_BUFFER_CB_LIST &l)
   {
      SDB_ASSERT(!isRunning(), "can not be running");
      reset();
      if (!l.empty())
      {
         _tasks.reserve(l.size());
         for (auto itr = l.cbegin(); itr != l.cend(); ++itr)
         {
            SDB_ASSERT(nullptr != itr->get(), "can not be null");
            _tasks.emplace_back(itr->get());
         }

         std::sort(_tasks.begin(), _tasks.end(), ioBufferFlushTask::comp());
      }
   }

   void ioBufferFlushJob::reset()
   {
      _tasks.clear();
      _tasks.shrink_to_fit();
      _pos = 0;
      _dispatched = 0;
   }

   bufferFlushTaskId ioBufferFlushJob::getNextTask()
   {
      THREAD_CONTEXT *tc = GET_THREAD_CONTEXT();
      SDB_ASSERT(nullptr != tc, "can not be null");
      dataManagementService &dms = tc->getEnv()->dms;
      bufferFlushTaskId tid;

      if (hasMoreTasks())
      {
         tid.offset = _pos++;
         tid.size = 1;
         ++_dispatched;
      }

      if (hasMoreTasks())
      {
         GLOBAL_PAGE_ID firstPid = _tasks[_pos - 1].bcb->getGlobalPid();
         const storageUnit *su = dms.getStorageUnit(firstPid.getSpaceId());
         SDB_ASSERT(nullptr != su, "can not be null");

         SDB_ASSERT(SPACE_TYPE_MAIN_DATA == firstPid.getSpaceType(), "must be data");
         SDB_ASSERT(FILE_TYPE_DATA_STORAGE == firstPid.getFileType(), "must be storage file");
         UINT32 pcnt = su->getManifest().dataArgs.getMaxPageCountInFile();
         UINT32 fd = firstPid.getPageId() / pcnt;

         do
         {
            GLOBAL_PAGE_ID nextPid = _tasks[_pos].bcb->getGlobalPid();
            if (firstPid.getSpaceId() != nextPid.getSpaceId() ||
                firstPid.getSpaceType() != nextPid.getSpaceType() ||
                firstPid.getFileType() != nextPid.getFileType())
            {
               break;
            }
            else if ((nextPid.getPageId() / pcnt) != fd)
            {
               break;
            }
            else
            {
               ++tid.size;
               ++_pos;
               continue;
            }
         } while(hasMoreTasks());
      }

   done:
      return tid;
   }
} // namespace vessel

} // namespace engine
