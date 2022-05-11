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

   Source File Name = ioBufferFlushJob.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#ifndef VESSEL_IO_BUFFER_FLUSH_JOB_H_
#define VESSEL_IO_BUFFER_FLUSH_JOB_H_

#include "vessel/ioBufferControlBlock.h"
#include "ossMemPool.hpp"
#include "vessel/bufferFlushDef.h"

namespace engine
{
namespace vessel
{
   struct ioBufferFlushTask : public SDBObject
   {
      explicit ioBufferFlushTask(ioBufferControlBlock *b):
      bcb(b){}

      ioBufferControlBlock *bcb = nullptr;

      struct comp
      {
         BOOLEAN operator()(const ioBufferFlushTask &l,
                            const ioBufferFlushTask &r)const
         {
            return l.bcb->getGlobalPid() < r.bcb->getGlobalPid();
         }
      };
   };

   class ioBufferFlushJob : public SDBObject
   {
      public:
          ioBufferFlushJob() = default;
          ~ioBufferFlushJob() = default;
          ioBufferFlushJob(const ioBufferFlushJob &) = delete;
          ioBufferFlushJob &operator=(const ioBufferFlushJob &) = delete;

      private:
         typedef ossPoolVector<ioBufferFlushTask> _TASK_POOL;

      public:
         /// do not destroy l until job done.
         void build(const SHARED_IO_BUFFER_CB_LIST &l);
         void reset();
         OSS_INLINE BOOLEAN hasMoreTasks()const {return _pos < _tasks.size();}
         bufferFlushTaskId getNextTask();
         const ioBufferFlushTask &get(UINT32 pos)const {return _tasks.at(pos);}

         OSS_INLINE BOOLEAN isRunning()const {return !_tasks.empty();}
         OSS_INLINE UINT32 getDispatchedTaskNum()const {return _dispatched;}
         OSS_INLINE UINT32 getTotalTaskNum()const {return _tasks.size();}

      private:
         _TASK_POOL _tasks;
         UINT32 _pos = 0;

         /// task num dispatched, one task may have more than one buffer.
         UINT32 _dispatched = 0;
   };//class ioBufferFlushJob
} // namespace vessel

} // namespace engine


#endif//VESSEL_IO_BUFFER_FLUSH_JOB_H_
