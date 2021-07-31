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
         lcLRUList(const lcLRUList &) = delete;
         lcLRUList &operator=(const lcLRUList &) = delete;

      public:
         INT32 init(lcFreeList *fl,
                    const liteCacheOptions::lruOptions &options);
         void fini();

         UINT32 getSize(BOOLEAN lock);

         /// for user threads
         /// tag under w lock
         INT32 insert(lcPageTagHolder &holder,
                      UINT32 beginTouchCount = 1);

         /// for user threads
         /// tag under lock
         INT32 tryToUpdate(lcPageTagHolder &holder);

         /// for user threads
         /// should always check free list first.
         /// page scan num will be max(pageCount, lruOptions.lruPageScanNum) when scanUntilHitMax is false
         /// pageBuf may be invalid when return ok
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
                                       freeListPage &page);
      private:
         lcFreeList *_fl = NULL;
         ossSpinXLatch _latch;

         /// options
         liteCacheOptions::lruOptions _options;

         /// runtime
         UINT32 _size = 0;
         UINT32 _coldSize = 0;
         liteCachePageTag *_head = NULL;
         liteCachePageTag *_middle = NULL;
         liteCachePageTag *_tail = NULL;
         liteCachePageTag *_evictBegin = NULL;
   }; /// end of class lcLRUList
} /// end of namespace vessel
} /// end of namespace engine

#endif