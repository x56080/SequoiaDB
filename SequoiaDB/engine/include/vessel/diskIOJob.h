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

   Source File Name = diskIOJob.h

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

#ifndef VESSEL_DISK_IO_JOB_H_
#define VESSEL_DISK_IO_JOB_H_

#include "vessel/diskIOTask.h"
#include "ossMemPool.hpp"

namespace engine
{
namespace vessel
{
   class liteCachePageTag;
   class diskIOJob : public SDBObject
   {
      public:
         enum TYPE
         {
            DIRTY_LIST = 0,
            LRU_LIST = 1,
         };//enum TYPE

      private:
         enum _STATUS
         {
            NONE = 0,
            PENDING = 1,
            DISPATCHING = 2,
         };//enum _STATUS

      public:
         diskIOJob();
         ~diskIOJob();

         diskIOJob(const diskIOJob &o) = delete;
         diskIOJob &operator=(const diskIOJob &) = delete;

      public:
         void reset();
         void prepare(UINT64 jobID, TYPE type, UINT32 bufSize=0);
         INT32 addPendingWriteTag(liteCachePageTag *tag);

         void prepareForDispatching(UINT32 maxIOSizePerTask=0);

         /// must be prepared for dispatching.
         /// return SDB_VESSEL_END_OF_CURSOR when no more task.
         INT32 getNextTask(diskIOTask &task);

         /// can not abort dispathed task.
         void abortUndispatchedTasks();

      public:
         /// callback by diskIOTask 
         void releaseTagsWhenTaskDone(const diskIOTask *task);

      public:
         OSS_INLINE UINT32 getTagCount()const
         {
            return _tags.size();
         }

         OSS_INLINE liteCachePageTag *getTag(UINT32 taskID)
         {
            if (OSS_LIKELY(taskID < _tags.size()))
            {
               return _tags.at(taskID);
            }
            else
            {
               return NULL;
            }
         }

         OSS_INLINE BOOLEAN needFSync()const
         {
            return DIRTY_LIST == _jobType;
         }

      private:
         void releaseTag(UINT32 i);

      private:
         typedef ossPoolVector<liteCachePageTag *> _TAG_VEC;

      private:
         _STATUS _status;
         UINT64 _jobID;
         TYPE _jobType;
         UINT32 _dispatchedCount;
         UINT32 _maxIOSizePerTask;
         _TAG_VEC _tags; 
   };//class diskIOJob

}  /// end of namespace vessel 
}  /// end of namespace engine

#endif//VESSEL_DISK_IO_JOB_H_