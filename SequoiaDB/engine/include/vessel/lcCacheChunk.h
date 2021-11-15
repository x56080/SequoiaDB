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

   Source File Name = lcCacheChunk.h

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
#ifndef VESSEL_LC_CACHE_CHUNK_H_
#define VESSEL_LC_CACHE_CHUNK_H_

#include "core.hpp"
#include "oss.hpp"
#include "utilBitmap.hpp"
#include "freeListPage.h"
#include "pd.hpp"

namespace engine
{
namespace vessel
{
   class lcCacheChunk : public SDBObject
   {
      public:
         lcCacheChunk();
         ~lcCacheChunk();
         lcCacheChunk(const lcCacheChunk &o) = delete;
         lcCacheChunk &operator=(const lcCacheChunk &o) = delete;

      public:
         INT32 setup(UINT32 id, UINT32 pageNum, UINT32 pageSize);
         INT32 teardown();
         BOOLEAN allocatePage(freeListPage &page);
         void releasePage(const freeListPage &page);

         OSS_INLINE BOOLEAN hasFreePage()
         {
            return _pageBitmap.isFull() ? FALSE : TRUE;
         }

         OSS_INLINE UINT32 getFreePageCount()
         {
            return _pageBitmap.freeSize();
         }

         OSS_INLINE UINT32 getPageNum()const
         {
            return _pageNum;
         }

         OSS_INLINE UINT32 getId()const
         {
            return _id;
         }

      private:
         UINT32 getPagePos(ossValuePtr pagePtr)const;
         ossValuePtr getPagePtr(UINT32 pagePos)const;
         
      private:
         UINT32 _id;
         UINT32 _pageNum;
         UINT32 _pageSize;
         CHAR *_pages;
         UINT32 _bitPos;
         utilBitmap _pageBitmap;

   }; /// end of class lcCacheChunk
} /// end of namespace vessel
} /// end of namespace engine

#endif