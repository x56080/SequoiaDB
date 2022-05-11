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

   Source File Name = ioBufferFlushJob.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

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
