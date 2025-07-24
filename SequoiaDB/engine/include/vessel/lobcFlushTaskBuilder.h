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

   Source File Name = lobcFlushTaskBuilder.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef VESSEL_LOBC_FLUSH_TASK_BUILDER_H_
#define VESSEL_LOBC_FLUSH_TASK_BUILDER_H_

#include "vessel/lobcFlushList.h"
#include "vessel/bufferFlushTask.h"
#include "vessel/bufferFlushDef.h"

namespace engine
{
namespace vessel
{
   class lobcFlushTaskBuilder : public SDBObject
   {
      public:
         lobcFlushTaskBuilder() = default;
         ~lobcFlushTaskBuilder() = default;
         lobcFlushTaskBuilder(const lobcFlushTaskBuilder &) = delete;
         lobcFlushTaskBuilder &operator=(const lobcFlushTaskBuilder &) = delete;

      public:

         void build(const lobcFlushList &fl);
         BOOLEAN hasMore()const {return _next < _tasks.size();}
         bufferFlushTaskId getNextTask();
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