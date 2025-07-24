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

   Source File Name = lobcFlushTaskBuilder.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

*******************************************************************************/
#include "vessel/lobcFlushTaskBuilder.h"
#include "pdTrace.hpp"
#include "vessel/threadContext.h"
#include "vessel/instanceEnv.h"
#include "vessel/storageFileCluster.h"

namespace engine
{
namespace vessel
{
   void lobcFlushTaskBuilder::build(const lobcFlushList &fl)
   {
      clear();
      _tasks.reserve(fl._dirtyPageCount);
      const SHARED_LOBC_BUFFER_LIST &list = fl._list;
      SHARED_LOBC_BUFFER_LIST::const_iterator itr = list.cbegin();
      for (; itr != list.cend(); ++itr)
      {
         (*itr)->exportTasks(_tasks);
      }

      if (!_tasks.empty())
      {
         std::sort(_tasks.begin(), _tasks.end(), bufferFlushTask::comp());
      }
   }

   void lobcFlushTaskBuilder::clear()
   {
      _tasks.clear();
      _tasks.shrink_to_fit();
      _next = 0;
      _dispatched = 0;
   }

   bufferFlushTaskId lobcFlushTaskBuilder::getNextTask()
   {
      THREAD_CONTEXT *tc = GET_THREAD_CONTEXT();
      SDB_ASSERT(nullptr != tc, "can not be null");
      dataManagementService &dms = tc->getEnv()->dms;
      bufferFlushTaskId tid;

      if (hasMore())
      {
         tid.offset = _next++;
         tid.size = 1;
      }

      if (hasMore())
      {
         const globalPageID &gpid = _tasks[_next - 1].gpid();
         const storageUnit *su = dms.getStorageUnit(gpid.getSpaceId());
         SDB_ASSERT(nullptr != su, "can not be null");
         UINT32 pcnt = su->getManifest().lobArgs.getMaxPageCountInFile();
         UINT32 fd = gpid.getPageId() / pcnt;

         do
         {
            const globalPageID &next = _tasks[_next].gpid();
            if (gpid.getSpaceId() == next.getSpaceId())
            {
               UINT32 nextFd = next.getPageId() / pcnt;
               if (fd == nextFd)
               {
                  ++tid.size;
                  ++_next;
                  continue;
               }
            }
            
            /// different space id or different file id.
            break;
         } while (hasMore());
      }

      if (tid.isValid())
      {
         ++_dispatched;
      }
      return tid;
   }
} // namespace vessel

} // namespace engine
