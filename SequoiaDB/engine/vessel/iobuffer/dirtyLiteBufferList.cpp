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

   Source File Name = dirtyLiteBufferList.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

*******************************************************************************/
#include "vessel/dirtyLiteBufferList.h"
#include "pdTrace.hpp"

namespace engine
{
namespace vessel
{
   void dirtyLiteBufferList::insert(SHARED_IO_BUFFER_CB &bcb)
   {
      SDB_ASSERT(!!bcb && bcb->hasDirtyLSN(), "can not be invalid");

      /// it may be still pending flush now, bu does not matter.
      /// cleaner will reset it soon.
      if (!bcb->hasDirtyFlag())
      {
         bcb->ctl().setFlag(LITE_IO_BUFFER_CTL_FLAGS::DIRTY);
         std::unique_lock<std::mutex> guard(_mutex);
         pushBackToList(bcb);
      }
   }

   void dirtyLiteBufferList::makeFlushList(UINT32 maxBufferCount,
                                           SHARED_IO_BUFFER_CB_LIST &fl,
                                           UINT64 &maxLSN)
   {
      SDB_ASSERT(DPS_INVALID_LSN_OFFSET == _minFlushLSN, "task still running");
      fl.clear();
      maxLSN = DPS_INVALID_LSN_OFFSET;

      UINT32 count = 0;
      std::unique_lock<std::mutex> guard(_mutex);

      SHARED_IO_BUFFER_CB_LIST::iterator left = _l.begin();
      SHARED_IO_BUFFER_CB_LIST::iterator right = _l.begin();
      while (_l.end() != right &&
             (0 == maxBufferCount || count++ < maxBufferCount))
      {
         if (DPS_INVALID_LSN_OFFSET == maxLSN ||
             maxLSN < (*right)->getMaxDirtyLSN())
         {
            maxLSN = (*right)->getMaxDirtyLSN();
         }

         ++right;
      }

      if (left != right)
      {
         fl.splice(fl.end(), _l, left, right);
         _minFlushLSN = fl.front()->getMinDirtyLSN();
      }

      resetMinListLSNAndSize(FALSE);

      guard.unlock();
      for (auto itr = fl.begin(); itr != fl.end(); ++itr)
      {
         (*itr)->ctl().setFlag(LITE_IO_BUFFER_CTL_FLAGS::PENDDING_FLUSH);
      }
      return;
   }

   void dirtyLiteBufferList::discard(SPACE_ID sid, SHARED_IO_BUFFER_CB_LIST &discarded)
   {
      SDB_ASSERT(INVALID_SPACE_ID != sid, "can not be invalid");
      std::unique_lock<std::mutex> guard(_mutex);

      SHARED_IO_BUFFER_CB_LIST::iterator left = _l.begin();
      SHARED_IO_BUFFER_CB_LIST::iterator right = _l.begin();
      while (_l.end() != right)
      {
         if ((*right)->getGlobalPid().getSpaceId() == sid)
         {
            ++right;
         }
         else if (left != right)
         {
            discarded.splice(discarded.end(), _l, left, right);
            ++right;
            left = right;
         }
         else
         {
            ++left;
            ++right;
         }
      }

      if (left != right)
      {
         discarded.splice(discarded.end(), _l, left, right);
      }

      resetMinListLSNAndSize(FALSE);
      return;
   }
} // namespace vessel

} // namespace engine
