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

   Source File Name = dirtyLiteBufferList.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#include "vessel/dirtyLiteBufferList.h"
#include "pdTrace.hpp"

namespace engine
{
namespace vessel
{
   void dirtyLiteBufferList::insert(SHARED_IO_BUFFER_CB &bcb)
   {
      SDB_ASSERT(!!bcb, "can not be invalid");
      SDB_ASSERT(bcb->isDirty(), "must be dirty");

      if (!bcb->isInDirtyList())
      {
         bcb->ctl().setFlags(LITE_IO_BUFFER_CTL_FLAGS::IN_DIRTY_LIST);
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

      resetMinListLSN(FALSE);

      return;
   }

   void dirtyLiteBufferList::discard(SPACE_ID sid, SHARED_IO_BUFFER_CB_LIST &discarded)
   {
      SDB_ASSERT(INVALID_SPACE_ID != sid, "can not be invalid");
      std::unique_lock<std::mutex> guard(_mutex);

      SHARED_IO_BUFFER_CB_LIST::iterator itr = _l.begin();
      while (itr != _l.end())
      {
         if ((*itr)->getGlobalPid().getSpaceId() == sid)
         {
            SHARED_IO_BUFFER_CB &bcb = *itr;
            bcb->ctl().setFlags(0);
            SHARED_IO_BUFFER_CB_LIST::iterator pos = itr++;
            discarded.splice(discarded.end(), _l, pos, itr);
         }
         else
         {
            ++itr;
         }
      }

      resetMinListLSN(FALSE);

      return;
   }
} // namespace vessel

} // namespace engine
