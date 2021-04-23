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

   Source File Name = logicalPageCache.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#include "vessel/logicalPageCache.h"

namespace engine
{
namespace vessel
{
   BOOLEAN logicalPageCache::findIndexPage(ossSpinSLatch *latch, PAGE_ID lpid, idMapSlot &slot)
   {
      SDB_ASSERT(ossIsPowerOf2(LOGICAL_PAGE_CACHE_BUCKET_COUNT), "must be power of 2");
      BOOLEAN r = FALSE;
      slot.reset();
      ossScopedLock guard(latch, SHARED);
      _cacheBucket &bucket = _buckets[(lpid & (LOGICAL_PAGE_CACHE_BUCKET_COUNT - 1))];
      _cacheBucket::PAGE_CACHE::const_iterator itr = bucket.indexCache.find(lpid);
      if (bucket.indexCache.end() != itr)
      {
         r = TRUE;
         slot = itr->second;
      }
   done:
      return r;
   }

   void logicalPageCache::upsertIndexPage(ossSpinSLatch *latch, PAGE_ID lpid, const idMapSlot &slot)
   {
      SDB_ASSERT(ossIsPowerOf2(LOGICAL_PAGE_CACHE_BUCKET_COUNT), "must be power of 2");
      ossScopedLock guard(latch, EXCLUSIVE);
      _cacheBucket &bucket = _buckets[(lpid & (LOGICAL_PAGE_CACHE_BUCKET_COUNT - 1))];
      bucket.indexCache[lpid] = slot;
      return;
   }
}//namespace vessel
}//namespace engine