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

   Source File Name = logicalPageCache.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#ifndef VESSEL_LOGICAL_PAGE_CACHE_H_
#define VESSEL_LOGICAL_PAGE_CACHE_H_

#include "pageDef.h"
#include "ossLatch.hpp"
#include "ossMemPool.hpp"
#include "vessel/idMapPage.h"

namespace engine
{
namespace vessel
{
   static const UINT32 LOGICAL_PAGE_CACHE_BUCKET_COUNT = 256;

   class logicalPageCache : public SDBObject
   {
      public:
         logicalPageCache(){}
         ~logicalPageCache(){}
         logicalPageCache(const logicalPageCache &) = delete;
         logicalPageCache &operator=(const logicalPageCache &) = delete;

      public:
         BOOLEAN findIndexPage(ossSpinSLatch *latch, PAGE_ID lpid, idMapSlot &slot);
         BOOLEAN findLobPage(ossSpinSLatch *latch, PAGE_ID lpid, idMapSlot &slot);
         void upsertIndexPage(ossSpinSLatch *latch, PAGE_ID lpid, const idMapSlot &slot);

      private:
         class _cacheBucket
         {
            public:
               _cacheBucket(){}
               ~_cacheBucket(){}
            public:
               typedef ossPoolMap<PAGE_ID, idMapSlot> PAGE_CACHE;
               PAGE_CACHE indexCache;
               PAGE_CACHE lobCache;

         };//class _cacheBucket
      
      private:
         _cacheBucket _buckets[LOGICAL_PAGE_CACHE_BUCKET_COUNT];
   };//class logicalPageCache
}//namespace vessel
}//namespace engine

#endif//VESSEL_LOGICAL_PAGE_CACHE_H_