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

   Source File Name = lcFreeList.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/
#ifndef VESSEL_LC_FREE_LIST_H_
#define VESSEL_LC_FREE_LIST_H_

#include "vessel/liteCacheDef.h"
#include "vessel/lcCacheChunk.h"
#include "vessel/freeListPage.h"
#include "vessel/vesselOptions.h"
#include "ossLatch.hpp"
#include "ossMemPool.hpp"

#include <list>

namespace engine
{
namespace vessel
{
   class liteCacheChunk;

   class lcFreeList : public SDBObject
   {
      public:
         lcFreeList();
         ~lcFreeList();

      private:
         lcFreeList(const lcFreeList &) = delete;
         lcFreeList &operator=(const lcFreeList &) = delete;

      public:
         OSS_INLINE UINT32 getMaxPageCount()const
         {
            return _options.maxChunkCount * _options.pageCountInChunk;
         }

         OSS_INLINE UINT32 getPageSize()const
         {
            return _pageSize;
         }

      public:
         INT32 init(UINT32 pageSize, 
                    const liteCacheOptions::freeListOptions &options);
         void fini();


         INT32 allocate(freeListPage &page);

         void releasePage(const freeListPage &page);

         void releasePages(UINT32 size, const freeListPage *pages);

         BOOLEAN fastCheckIfHasFreePage()const;

      private:
         INT32 pushNewChunkIntoFreeList();

         INT32 initChunkArray(UINT32 size);

      private:
         UINT32 _pageSize;
         ossSpinXLatch _latch;
         liteCacheOptions::freeListOptions _options;
         lcCacheChunk *_chunks;
         UINT32 _size;
         ossPoolList<lcCacheChunk*> _free;
   };
}/// end of namespace vessel
} /// end of namespace engine

#endif 