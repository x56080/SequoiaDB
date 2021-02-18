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

namespace engine
{
namespace vessel
{
   class lcExtentTag;
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
         
         enum _FLAG
         {
            _FLAG_FSYNC = 0x01,
         };// enum _FLAG

      public:
         diskIOJob();
         ~diskIOJob();

      private:
         diskIOJob(const diskIOJob &o){}
         diskIOJob &operator=(const diskIOJob &)
         {
            return *this;
         }

      public:
         void reset();
         INT32 prepare(UINT64 jobID, TYPE type, UINT32 bufCount=0);
         INT32 addPendingWriteTag(lcExtentTag *tag);

         INT32 prepareForDispatching(UINT32 maxIOSizePerTask=0);

         /// must be prepared for dispatching.
         /// return SDB_VESSEL_END_OF_CURSOR when no more task.
         INT32 getNextTask(diskIOTask &task);

         /// can not abort dispathed task.
         INT32 abort();

         INT32 allTaskDone(BOOLEAN &r)const;

      public:
         /// callback by diskIOTask 
         void releaseDispatchedTask(const diskIOTask *task);

      public:
         OSS_INLINE UINT32 getPageCount()const
         {
            return _pageCount;
         }

         OSS_INLINE lcExtentTag *getTag(UINT32 taskID)
         {
            if (OSS_LIKELY(taskID < _pageCount))
            {
               return _tags[taskID];
            }
            else
            {
               return NULL;
            }
         }

         OSS_INLINE BOOLEAN needFSync()const
         {
            return OSS_BIT_TEST(_flags, _FLAG_FSYNC);
         }

      private:
         INT32 extentBufTo(UINT32 count);

         void releaseTag(UINT32 pos);

      private:
         _STATUS _status;
         UINT64 _jobID;
         TYPE _jobType;
         UINT32 _flags;
         UINT32 _bufCount;
         UINT32 _pageCount;
         UINT32 _dispatchedCount;
         UINT32 _maxIOSizePerTask;
         lcExtentTag **_tags;
         
   };//class diskIOJob

}  /// end of namespace vessel 
}  /// end of namespace engine

#endif//VESSEL_DISK_IO_JOB_H_