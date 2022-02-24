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

   Source File Name = logicalPageIdCache.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#ifndef VESSEL_LOGICAL_PAGE_ID_CACHE_H_
#define VESSEL_LOGICAL_PAGE_ID_CACHE_H_

#include "vessel/pageIdentifier.h"
#include "ossMemPool.hpp"
#include "ossLatch.hpp"
#include "ossRWMutex.hpp"
#include "vessel/memoryBlock.h"
#include "vessel/idMapPage.h"

#include <atomic> // c++11


namespace engine
{
namespace vessel
{
   class idMapFile;
   class deltaLogFile;
   class deltaLogDumpRecord;

   class partialImpCacheMap : public SDBObject
   {
      private:
         struct _hash
         {
            size_t operator()(const PAGE_ID &v)const
            {
               return v;
            }
         };//struct _hash
      public:
         typedef ossPoolUnorderedMap<PAGE_ID, idMapSlot, _hash> CACHE_MAP;

      public:
         partialImpCacheMap(){}
         ~partialImpCacheMap()
         {
            fini();
         }
         partialImpCacheMap(const partialImpCacheMap &) = delete;
         partialImpCacheMap &operator=(const partialImpCacheMap &) = delete;

      public:
         void fini();
         BOOLEAN find(PAGE_ID lpid, idMapSlot &value, BOOLEAN &isMutable)const;
         BOOLEAN insert(PAGE_ID lpid, const idMapSlot &value);
         void upsert(PAGE_ID lpid, const idMapSlot &value);
         void restoreToImmutableMap(PAGE_ID lpid, const idMapSlot &value);
         void remove(PAGE_ID lpid);
         void switchMap();
         void clearImmutableMap();
         OSS_INLINE UINT32 getMutableMapSize()const
         {
            return getMutableMap().size();
         }
         OSS_INLINE UINT32 getImmutableMapSize()const
         {
            return getImmutableMap().size();
         }

         static constexpr UINT32 getCacheItemSize()
         {
            return sizeof(PAGE_ID) + sizeof(idMapSlot);
         }

      public:
         OSS_INLINE CACHE_MAP &getMutableMap()
         {
            return _maps[_mutablePos];
         }
         OSS_INLINE const CACHE_MAP &getMutableMap()const
         {
            return _maps[_mutablePos];
         }
         OSS_INLINE const CACHE_MAP &getImmutableMap()const
         {
            return _maps[_mutablePos ^ 1];
         }
      private:
         OSS_INLINE CACHE_MAP &_getImmutableMap()
         {
            return _maps[_mutablePos ^ 1];
         }
      private:
         UINT32 _mutablePos = 0;
         CACHE_MAP _maps[2];
   };//class partialImpCacheMap
   
   class logicalPageIdCache : public SDBObject
   {
      public:
         logicalPageIdCache();
         ~logicalPageIdCache();
         logicalPageIdCache(const logicalPageIdCache &) = delete;
         logicalPageIdCache &operator=(const logicalPageIdCache &) = delete;

      public:
         class options : public SDBObject
         {
            public:
               OSS_INLINE options(){}
               OSS_INLINE ~options(){}
               OSS_INLINE options(const options &o):
               bucketCount(o.bucketCount),
               bucketLatchCount(o.bucketLatchCount)
               {}

               OSS_INLINE options &operator=(const options &o)
               {
                  bucketCount = o.bucketCount;
                  bucketLatchCount = o.bucketLatchCount;
                  return *this;
               }

            public:
               BOOLEAN isValid()const
               {
                  return ossIsPowerOf2(bucketCount) &&
                         ossIsPowerOf2(bucketLatchCount) &&
                         bucketLatchCount <= bucketCount;
               }

            public:
               /// Must be power of 2
               UINT32 bucketCount = 0;

               /// Must be power of 2 and not greater than bucketCount.
               UINT32 bucketLatchCount = 0;
         };//class options

      public:
         OSS_INLINE BOOLEAN isReady()const
         {
            return NULL != _base;
         }

         OSS_INLINE UINT32 getBucketCount()const
         {
            return _buckets.size();
         }
      public:
   
         INT32 init(const options &o,
                    const idMapFile *base);

         void fini();

         /// WARNING:The result may not be real.
         UINT64 getFuzzyCacheSize();

         UINT32 getFuzzyMutablePageCount()const;

         ///WARNING: User should ensure that no one can update lpid's mapping when
         /// call put/get.
         INT32 put(PAGE_ID lpid, const idMapSlot &slot);

         INT32 get(PAGE_ID lpid, idMapSlot &slot, BOOLEAN &isMutable);

         void remove(PAGE_ID lpid);

      public:         
         INT32 resetBaseAndClearImmutableMaps(const idMapFile *file);

         void setAllPagesImmutable();

      public:
         /// WARNING: not thread-safe
         const partialImpCacheMap::CACHE_MAP getImmutableMap(UINT32 pos)const;

         /// WARNING: not thread-safe
         const partialImpCacheMap::CACHE_MAP getMutableMap(UINT32 pos)const;

         /// WARNING: can be used only when startup
         INT32 upsertWhenRestore(PAGE_ID lpid, const idMapSlot &slot);

      private:
         INT32 _put(PAGE_ID lpid,
                    const idMapSlot &slot);
         INT32 _get(PAGE_ID lpid, idMapSlot &slot, BOOLEAN &isMutable);
         void _remove(PAGE_ID lpid);
         INT32 getFromBase(PAGE_ID lpid, idMapSlot &slot)const;
         INT32 resetBase(const idMapFile *base);

      private:
         OSS_INLINE UINT32 getBucket(PAGE_ID lpid, ossSLatch **latch)
         {
            UINT32 pos = (lpid & (_buckets.size() - 1));
            if (NULL != latch)
            {
               *latch = getBucketLatch(pos);
            }
            return pos;
         }

         OSS_INLINE ossSLatch *getBucketLatch(UINT32 bucketNo)
         {
            return &(_latches[bucketNo & (_latches.size() - 1)]);
         }

      private:
         ossRWMutex _latch;
         const idMapFile *_base = NULL;
         UINT32 _basePageCount = 0;

         ossPoolVector<_ossSpinSLatchPOSIX> _latches;
         ossPoolVector<partialImpCacheMap *> _buckets;

         /// fuzzy counter
         std::atomic_int _counter = {0};
   };//class logicalPageIdCache
}//namespace vessel
}//namespace engine

#endif//VESSEL_LOGICAL_PAGE_ID_CACHE_H_
