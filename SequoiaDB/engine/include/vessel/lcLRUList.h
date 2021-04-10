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

   Source File Name = lcLRUList.h

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

#ifndef VESSEL_LC_LRU_LIST_H_
#define VESSEL_LC_LRU_LIST_H_

#include "ossTypes.h"
#include "vessel/lcPageTagHolder.h"
#include "vessel/liteCacheDef.h"
#include "vessel/vesselOptions.h"
#include "ossLatch.hpp"

namespace engine
{
namespace vessel
{
   class lcBuckets;
   class lcFreeList;
   class diskIOJob;
   class requestContext;

   class lcLRUList : public SDBObject
   {
      public:
         lcLRUList();
         ~lcLRUList();

      public:
         INT32 init(lcBuckets *buckets,
                     lcFreeList *fl,
                     const liteCacheOptions::lruOptions &options);
         INT32 fini();

         /// for user threads
         /// tag under w lock
         INT32 insert(lcPageTagHolder &holder, const freeListPage &page);

         /// for user threads
         /// tag under lock
         INT32 tryToUpdate(lcPageTagHolder &holder);

         /// for user threads
         /// should always check free list first.
         /// page scan num will be max(pageCount, lruOptions.lruPageScanNum) when scanUntilHitMax is false
         INT32 evict(requestContext *context,
                     BOOLEAN scanUntilHitMax,
                     freeListPage &pageBuf);

         /// for background threads
         INT32 setPendingWriteOrEvict(requestContext *context,
                                      UINT32 scanDepth,
                                      diskIOJob *job,
                                      UINT32 *involvedMemPageCount);

         /// for background threads
         /// reset evict begin prt after flush done
         void resetEvictBegin();

      private:
         OSS_INLINE BOOLEAN splited()const
         {
            return NULL != _middle;
         }

         void splitLRU();
         void cancelSplit();

         void tryToTuneRightMiddle();
         void tryToTuneLeftMiddle();
         void removeTagAndTuneMiddle(liteCachePageTag *tag);

         void moveToHead(liteCachePageTag *tag);
         void insertToMiddle(liteCachePageTag *tag);
         void insertToHead(liteCachePageTag *tag);
         void removeFromList(liteCachePageTag *tag);

         BOOLEAN tryToEvictTagFromList(liteCachePageTag *tag,
                                       freeListPage &page,
                                       BOOLEAN &removeFromBucket);

      
      private:
         lcBuckets *_buckets;
         lcFreeList *_fl;
         ossSpinXLatch _latch;

         /// options
         liteCacheOptions::lruOptions _options;

         /// real time
         UINT32 _size;
         UINT32 _coldSize;
         liteCachePageTag *_head;
         liteCachePageTag *_middle;
         liteCachePageTag *_tail;
         liteCachePageTag *_evictBegin;
   }; /// end of class lcLRUList
} /// end of namespace vessel
} /// end of namespace engine

#endif