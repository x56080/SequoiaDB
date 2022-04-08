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

   Source File Name = lobcFlushTaskBuilder.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#include "vessel/lobcFlushTaskBuilder.h"
#include "pdTrace.hpp"
#include "vessel/threadContext.h"
#include "vessel/instanceEnv.h"
#include "vessel/storageFileCluster.h"

namespace engine
{
namespace vessel
{
   lobcFlushTaskBuilder::lobcFlushTaskBuilder()
   {

   }

   lobcFlushTaskBuilder::~lobcFlushTaskBuilder()
   {

   }

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

   lobcFlushTaskBuilder::taskId lobcFlushTaskBuilder::getNextTask()
   {
      THREAD_CONTEXT *tc = GET_THREAD_CONTEXT();
      SDB_ASSERT(nullptr != tc, "can not be null");
      dataManagementService &dms = tc->getEnv()->dms;
      storageFileCluster *fcluster = nullptr;
      taskId tid;

      if (hasMore())
      {
         tid.offset = _next++;
         tid.size = 1;
      }

      if (hasMore())
      {
         const globalPageID &gpid = _tasks[_next - 1].gpid();
         fcluster = dms.getLobdFileCluster(gpid.space());
         SDB_ASSERT(nullptr != fcluster, "can not be null");

         do
         {
            const globalPageID &next = _tasks[_next].gpid();
            const globalPageID &current = _tasks[_next - 1].gpid();
            if (current.space() == next.space())
            {
               INT32 currentFd = fcluster->getFileSpaceId(current.page());
               SDB_ASSERT(0 <= currentFd, "impossible");
               INT32 nextFd = fcluster->getFileSpaceId(next.page());
               SDB_ASSERT(0 <= nextFd, "impossible");
               if (currentFd == nextFd)
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
