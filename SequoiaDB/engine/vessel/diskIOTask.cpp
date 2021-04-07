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

   Source File Name = diskIOTask.cpp

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

#include "vessel/diskIOTask.h"
#include "ossLikely.hpp"
#include "vessel/lcExtentTag.h"
#include "vessel/diskIOJob.h"

namespace engine
{
namespace vessel
{

   lcExtentTag *diskIOTask::getTag(UINT32 pos)
   {
      lcExtentTag *tag = NULL;
      if (OSS_UNLIKELY(!valid()))
      {
         goto done;
      }
      else if (OSS_UNLIKELY(_pageCount <= pos))
      {
         goto done;
      }
      else
      {
         tag = _job->getTag(_taskID + pos);
      }

   done:
      return tag;
   }

   GLOBAL_PAGE_ID diskIOTask::getFirstPID()const
   {
      GLOBAL_PAGE_ID gpid;
      lcExtentTag *tag = NULL;
      if (OSS_UNLIKELY(!valid()))
      {
         goto done;
      }

      tag = _job->getTag(_taskID);
      if (OSS_UNLIKELY(NULL == tag))
      {
         goto done;
      }

      gpid = tag->id();
   done:
      return gpid;
   }

   void diskIOTask::done()
   {
      if (!valid())
      {
         goto done;
      }

      _job->releaseTagsWhenTaskDone(this);
      _taskID = 0;
      _pageCount = 0;
      _job = NULL;
   done:
      return;
   }
}//namespace vessel
}//namespace engine