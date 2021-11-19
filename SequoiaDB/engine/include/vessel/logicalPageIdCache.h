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
#include "vessel/partialImpCache.h"
#include "ossLatch.hpp"
#include "ossRWMutex.hpp"
#include "vessel/memoryBlock.h"

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
      public:
         ///<imp pid, pos in imp page>
         typedef std::pair<PAGE_ID, UINT32> KEY;
         struct cmp
         {
            OSS_INLINE BOOLEAN operator()(const KEY &l, const KEY &r)const
            {
               if (l.first < r.first)
               {
                  return TRUE;
               }
               else if (l.first > r.first)
               {
                  return FALSE;
               }
               else
               {
                  return l.second < r.second;
               }
            }
         };
      public:
         typedef ossPoolMap<KEY, partialImpCache *, cmp> CACHE_MAP;
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
         const partialImpCache *find(const KEY &key)const;
         INT32 insert(const KEY &key, partialImpCache *cache);
         void upsert(const KEY &key, partialImpCache *cache);
         partialImpCache *find(const KEY &key);
         void exportTo(partialImpCacheMap &o, UINT32 &replaced);
         const CACHE_MAP &get()const {return _map;} 
         CACHE_MAP &get(){return _map;}
         UINT32 getCacheSize()const
         {
            //return *((const volatile UINT32 *)(&_size));
            return _map.size() *
               (sizeof(partialImpCacheMap::KEY) + ID_MAP_PARTIAL_PAGE_CACHE_SIZE);
         }
         BOOLEAN isEmpty()const
         {
            return _map.empty();
         }

      public:
         CACHE_MAP::iterator begin() {return _map.begin();}
         CACHE_MAP::iterator end() {return _map.end();}
         CACHE_MAP::const_iterator begin()const {return _map.begin();}
         CACHE_MAP::const_iterator end()const {return _map.end();}
         static BOOLEAN isValidKey(const KEY &key);
      private:
         CACHE_MAP _map;
         //UINT32 _size = 0;
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

         /// WARNING:Will not hold any latch. The result may not be real.
         UINT64 getTotalCacheSize();

         ///WARNING: User should ensure that no one can update lpid's mapping when
         /// call put/get.
         INT32 upsert(PAGE_ID lpid, const idMapSlot &slot);

         INT32 get(PAGE_ID lpid, idMapSlot &slot, BOOLEAN &isMutable);

         /// Set slot as null if do not care about slot before removing.
         INT32 remove(PAGE_ID lpid, idMapSlot *slot=NULL);

         INT32 estimateMutablePageCount()const;

         INT32 upsertWhenRestore(const deltaLogDumpRecord *lr);

      public:         
         /// set mutableSegmentIds as null if do not care about mutable segments.
         INT32 prepareToCreateNewBase(UINT32 pageCountPerSeg,
                                      ossPoolSet<UINT32> *mutableSegmentIds);

         INT32 flushPreparedCacheToFile(idMapFile *file);

         INT32 resetBaseFileAndClearFlushedMaps(const idMapFile *file);

         INT32 dumpBufferAndSetImmutable(UINT32 pageCountPerSeg,
                                         ossPoolVector<memoryBlock> &buffers,
                                         ossPoolSet<UINT32> *mutableSegmentIds);

         const partialImpCacheMap &getImmutableMap()const
         {
            return _immutableMap;
         }

      private:
         INT32 _upsert(PAGE_ID lpid,
                       const idMapSlot &slot);
         INT32 _get(PAGE_ID lpid, idMapSlot &slot, BOOLEAN &isMutable);
         INT32 _remove(PAGE_ID lpid,
                       idMapSlot *slot);
         INT32 getFromBase(PAGE_ID lpid, idMapSlot &slot);

         INT32 createCacheFromBase(const partialImpCacheMap::KEY &key,
                                   partialImpCache **cache);

         INT32 findInMemToUpdate(UINT32 bucketPos,
                                 const partialImpCacheMap::KEY &key,
                                 partialImpCache **out);

      private:
         INT32 resetBase(const idMapFile *base);

      private:
         UINT32 getBucketAndPartialCacheIdentity(PAGE_ID lpid,
                                                 partialImpCacheMap::KEY &key,
                                                 UINT32 &slotInPartialCache)const;

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

         const partialImpCacheMap *_immutableCache = NULL;
         partialImpCacheMap _immutableMap;

         std::atomic_int _counter = {0};
   };//class logicalPageIdCache
}//namespace vessel
}//namespace engine

#endif//VESSEL_LOGICAL_PAGE_ID_CACHE_H_
