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

   Source File Name = diskIOTask.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#ifndef VESSEL_DISK_IO_TASK_H_
#define VESSEL_DISK_IO_TASK_H_

#include "vessel/globalPageID.h"
#include "ossLikely.hpp"

namespace engine
{
namespace vessel
{
   class diskIOJob;
   class liteCachePageTag;

   class diskIOTask : public SDBObject
   {
      public:
         diskIOTask(){}

         diskIOTask(UINT32 taskID,
                   UINT32 pageCount,
                   diskIOJob *job):
         _taskID(taskID),
         _pageCount(pageCount),
         _job(job)
         {}

         diskIOTask(const diskIOTask &o):
         _taskID(o._taskID),
         _pageCount(o._pageCount),
         _job(o._job)
         {}

         ~diskIOTask(){}

         diskIOTask &operator=(const diskIOTask &o)
         {
            _taskID = o._taskID;
            _pageCount = o._pageCount;
            _job = o._job;
            return *this;
         }

      public:
         ///WARNING: will release tags in job.
         void done();

         liteCachePageTag *getTag(UINT32 pos);

         GLOBAL_PAGE_ID getFirstPID()const;

         OSS_INLINE UINT32 getSize()const
         {
            return _pageCount;
         }

         OSS_INLINE BOOLEAN valid()const
         {
            return 0 < _pageCount;
         }

         OSS_INLINE const diskIOJob *getJob()const
         {
            return _job;
         }

         OSS_INLINE UINT32 getTaskID()const
         {
            return _taskID;
         }
      private:
         UINT32 _taskID = 0;
         UINT32 _pageCount = 0;
         diskIOJob *_job = NULL;
   };//class diskIOTask

}  /// end of namespace vessel 
}  /// end of namespace engine

#endif//VESSEL_DISK_IO_TASK_H_