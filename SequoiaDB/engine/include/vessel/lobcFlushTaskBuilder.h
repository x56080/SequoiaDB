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

   Source File Name = lobcFlushTaskBuilder.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#ifndef VESSEL_LOBC_FLUSH_TASK_BUILDER_H_
#define VESSEL_LOBC_FLUSH_TASK_BUILDER_H_

#include "vessel/lobcFlushList.h"
#include "vessel/bufferFlushTask.h"

namespace engine
{
namespace vessel
{
   class lobcFlushTaskBuilder : public SDBObject
   {
      public:
         lobcFlushTaskBuilder();
         ~lobcFlushTaskBuilder();
         lobcFlushTaskBuilder(const lobcFlushTaskBuilder &) = delete;
         lobcFlushTaskBuilder &operator=(const lobcFlushTaskBuilder &) = delete;

      public:
         struct taskId
         {
            OSS_INLINE BOOLEAN isValid()const {return 0 < size;}
            UINT32 offset = 0;
            UINT32 size = 0;
         };

      public:

         void build(const lobcFlushList &fl);
         BOOLEAN hasMore()const {return _next < _tasks.size();}
         taskId getNextTask();
         void clear();
         const bufferFlushTask &get(UINT32 pos)const
         {
            return _tasks.at(pos);
         }
         UINT32 getDispatchedTasks()const {return _dispatched;}
         UINT32 getTotalTaskNum()const {return _tasks.size();}

      private:
         typedef ossPoolVector<bufferFlushTask> _TASK_VEC;

      private:
         _TASK_VEC _tasks;
         UINT32 _next = 0;
         UINT32 _dispatched = 0;
   };//class lobcFlushTaskBuilder
} // namespace vessel

} // namespace engine


#endif//VESSEL_LOBC_FLUSH_TASK_BUILDER_H_