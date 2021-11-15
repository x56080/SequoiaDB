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

   Source File Name = logicalPageIdCache.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#include "vessel/logicalPageIdCache.h"
#include "ossLikely.hpp"
#include "vessel/idMapFile.h"
#include "ossLatchGuard.hpp"

namespace engine
{
namespace vessel
{

///////////////////////////partialImpCacheMap
   void partialImpCacheMap::fini()
   {
      partialImpCacheMap::CACHE_MAP::const_iterator itr = _map.begin();
      for (; itr != _map.end(); ++itr)
      {
         SDB_OSS_DEL itr->second;
      }
      _map.clear();
   }

   const partialImpCache *partialImpCacheMap::find(const KEY &key)const
   {
      const partialImpCache *out = NULL;
      CACHE_MAP::const_iterator itr = _map.find(key);
      if (_map.end() != itr)
      {
         out = itr->second;
      }
      return out;
   }

   partialImpCache *partialImpCacheMap::find(const KEY &key)
   {
      partialImpCache *out = NULL;
      CACHE_MAP::const_iterator itr = _map.find(key);
      if (_map.end() != itr)
      {
         out = itr->second;
      }
      return out;
   }

   INT32 partialImpCacheMap::insert(const KEY &key, partialImpCache *cache)
   {
      INT32 rc = SDB_OK;
      if (OSS_UNLIKELY(NULL == cache))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      if (!_map.insert(std::make_pair(key, cache)).second)
      {
         PD_LOG(PDERROR, "duplicated key[%d]", key);
         rc = SDB_VESSEL_DUPLICATED_KEY;
         goto error;
      }
   done:
      return rc;
   error:
      goto done;
   }

   void partialImpCacheMap::exportTo(partialImpCacheMap &o, UINT32 &replaced)
   {
      replaced = 0;
      CACHE_MAP::const_iterator itr = _map.begin();
      for (; itr != _map.end(); ++itr)
      {
         CACHE_MAP::iterator dstItr = o._map.find(itr->first);
         if (o._map.end() == dstItr)
         {
            o._map.insert(std::make_pair(itr->first, itr->second));
         }
         else
         {
            partialImpCache *tmp = dstItr->second;
            dstItr->second = itr->second;
            SDB_OSS_DEL tmp;
            ++replaced;
         }
      }
      _map.clear();
      return;
   }

///////////////////////////partialImpCacheMap
   logicalPageIdCache::_cacheBucket::_cacheBucket()
   {}

   logicalPageIdCache::_cacheBucket::~_cacheBucket()
   {
      reset();
   }

   void logicalPageIdCache::_cacheBucket::reset()
   {
      SAFE_OSS_DELETE(_mainMap);
      SAFE_OSS_DELETE(_flushingMap);
      return;
   }

   INT32 logicalPageIdCache::_cacheBucket::add(const partialImpCacheMap::KEY &key,
                                               partialImpCache *cache)
   {
      INT32 rc = SDB_OK;
      if (OSS_UNLIKELY(NULL == cache))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      if (NULL == _mainMap)
      {
         _mainMap = SDB_OSS_NEW partialImpCacheMap();
         if (OSS_UNLIKELY(NULL == _mainMap))
         {
            PD_LOG(PDERROR, "failed to allocate mem");
            rc = SDB_OOM;
            goto error;
         }
      }

      rc = _mainMap->insert(key, cache);
      if (SDB_OK != rc)
      {
         goto error;
      }

   done:
      return rc;
   error:
      goto done;
   }

   const partialImpCache *logicalPageIdCache::_cacheBucket::findToRead(const partialImpCacheMap::KEY &key)const
   {
      const partialImpCache *cache = NULL;
      if (NULL != _mainMap)
      {
         cache = _mainMap->find(key);
         if (NULL != cache)
         {
            goto done;
         }
      }

      if (NULL != _flushingMap)
      {
         cache = _flushingMap->find(key);
      }

   done:
      return cache;
   }

   INT32 logicalPageIdCache::_cacheBucket::findToUpdate(const partialImpCacheMap::KEY &key,
                                                        partialImpCache **cache)
   {
      INT32 rc = SDB_OK;
      partialImpCache *tmp = NULL;
      partialImpCache *newCache = NULL;

      if (OSS_UNLIKELY(NULL == cache))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      *cache = NULL;
      if (NULL != _mainMap)
      {
         tmp = _mainMap->find(key);
         if (NULL != tmp)
         {
            *cache = tmp;
            goto done;
         }
      }

      if (NULL != _flushingMap)
      {
         tmp = _flushingMap->find(key);
         if (NULL != tmp)
         {
            newCache = SDB_OSS_NEW partialImpCache();
            if (NULL == newCache)
            {
               PD_LOG(PDERROR, "failed to allocate mem");
               rc = SDB_OOM;
               goto error;
            }
            newCache->copy(tmp->getBuffer(), 0);
            
            rc = ensureMainMap();
            if (SDB_OK != rc)
            {
               PD_LOG(PDERROR, "failed to ensure main map:%d", rc);
               goto error;
            }

            rc = _mainMap->insert(key, newCache);
            if (OSS_UNLIKELY(SDB_OK != rc))
            {
               PD_LOG(PDERROR, "failed to add cache:%d", rc);
               goto error;
            }

            *cache = newCache;
         }
      }

   done:
      return rc;
   error:
      SAFE_OSS_DELETE(newCache);
      goto done;
   }

   void logicalPageIdCache::_cacheBucket::mergeMainMapToFlushingMap()
   {
      if (NULL == _mainMap || _mainMap->get().empty())
      {
         goto done;
      }

      if (NULL == _flushingMap)
      {
         _flushingMap = _mainMap;
         _mainMap = NULL;
      }
      else
      {
         UINT32 replaced = 0;
         _mainMap->exportTo(*_flushingMap, replaced);
         SDB_ASSERT(_mainMap->get().empty(), "must be empty");
      }
   done:
      return ;
   }

   void logicalPageIdCache::_cacheBucket::removeFlushingMap()
   {
      if (NULL != _flushingMap)
      {
         SDB_OSS_DEL _flushingMap;
         _flushingMap = NULL;
      }
      return;
   }

   UINT64 logicalPageIdCache::_cacheBucket::getTotalCacheSize()const
   {
      UINT64 size = 0;
      if (NULL != _mainMap)
      {
         size = (UINT64)(_mainMap->get().size()) * ID_MAP_PAGE_CACHE_SIZE;
      }
      if (NULL != _flushingMap)
      {
         size += (UINT64)(_flushingMap->get().size()) * ID_MAP_PAGE_CACHE_SIZE;
      }
      return size;
   }

   INT32 logicalPageIdCache::_cacheBucket::ensureMainMap()
   {
      INT32 rc = SDB_OK;
      if (NULL != _mainMap)
      {
         goto done;
      }

      _mainMap = SDB_OSS_NEW partialImpCacheMap();
      if (NULL == _mainMap)
      {
         PD_LOG(PDERROR, "failed to allocate mem");
         rc = SDB_OOM;
         goto error;
      }
   done:
      return rc;
   error:
      goto done;
   }

   INT32 logicalPageIdCache::_cacheBucket::addEmptyCache(const partialImpCacheMap::KEY &key,
                                                         partialImpCache **cache)
   {
      INT32 rc = SDB_OK;
      partialImpCache *tmp = SDB_OSS_NEW partialImpCache();
      if (OSS_UNLIKELY(NULL == tmp))
      {
         PD_LOG(PDERROR, "failed to allocate mem");
         rc = SDB_OOM;
         goto error;
      }

      rc = add(key, tmp);
      if (SDB_OK != rc)
      {
         goto error;
      }

      if (NULL != cache)
      {
         *cache = tmp;
      }

      tmp = NULL;
   done:
      return rc;
   error:
      SAFE_OSS_DELETE(tmp);
      goto done;
   }

   void logicalPageIdCache::_cacheBucket::setPagesImmutable(UINT32 pageCountPerSeg,
                                                            ossPoolSet<UINT32> *mutableSegmentIds)
   {
      if (NULL == _mainMap)
      {
         goto done;
      }

      for (partialImpCacheMap::CACHE_MAP::iterator itr = _mainMap->get().begin();
           itr != _mainMap->get().end(); ++itr)
      {
         if (0 ==  itr->second->getMutablePageCount())
         {
            continue;
         }

         itr->second->setAllPageImmutable(pageCountPerSeg, mutableSegmentIds);
      }
   done:
      return;
   }

   ////////////////logicalPageIdCache
   logicalPageIdCache::logicalPageIdCache()
   {

   }

   logicalPageIdCache::~logicalPageIdCache()
   {
      fini();
   }

   INT32 logicalPageIdCache::init(const logicalPageIdCache::options &o,
                                  const idMapFile *base)
   {
      INT32 rc = SDB_OK;
      idMapFileHead head;
      fini();

      if (OSS_UNLIKELY(!o.isValid() ||
                       NULL == base ||
                       !base->isOpen()))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      _base = base;
      rc = _base->getIdMapFileHead(head);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to get id map file head from file[%s], rc:%d",
                _base->getFullPath(), rc);
         goto error;
      }

      _basePageCount = head.totalPageCount;

      _latchCount = o.bucketLatchCount;
      _latches = SDB_OSS_NEW _ossSpinSLatchPOSIX[_latchCount];
      if (NULL == _latches)
      {
         PD_LOG(PDERROR, "failed to allocate mem");
         rc = SDB_OOM;
         goto error;
      }

      _bucketCount = o.bucketCount;
      _buckets = SDB_OSS_NEW _cacheBucket[_bucketCount];
      if (NULL == _buckets)
      {
         PD_LOG(PDERROR, "failed to allocate mem");
         rc = SDB_OOM;
         goto error;
      }
   done:
      return rc;
   error:
      fini();
      goto done;
   }

   void logicalPageIdCache::fini()
   {
      _base = NULL;
      _basePageCount = 0;
      _latchCount = 0;
      if (NULL != _latches)
      {
         SDB_OSS_DEL []_latches;
         _latches = NULL;
      }
      _bucketCount = 0;
      if (NULL != _buckets)
      {
         SDB_OSS_DEL []_buckets;
         _buckets = NULL;
      }

      _counter.store(0);

      return;
   }

   UINT64 logicalPageIdCache::getTotalCacheSize()const
   {
      UINT32 size = 0;
      if (OSS_LIKELY(isReady()))
      {
         for (UINT32 i = 0; i < _bucketCount; ++i)
         {
            size += _buckets[i].getTotalCacheSize();
         }
      }

      return size;
   }

   INT32 logicalPageIdCache::estimateMutablePageCount()const
   {
      return _counter.load(std::memory_order_relaxed);
   }

   INT32 logicalPageIdCache::flushPreparedCacheToFile(idMapFile *file)
   {
      INT32 rc = SDB_OK;
      if (OSS_UNLIKELY(!isReady()))
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }
      else if (OSS_UNLIKELY(NULL == file ||
                            !file->isOpen()))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      for (UINT32 i = 0; i < _bucketCount; ++i)
      {
         /// No need to hold latch here.
         rc = flushPreparedMapToFile(_buckets[i], file);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to dump cache to file, bucket no[%d], rc:%d",
                   i, rc);
            goto error;
         }
      }
   done:
      return rc;
   error:
      goto done;
   }

   INT32 logicalPageIdCache::setPagesImmutable(UINT32 pageCountPerSeg,
                                               ossPoolSet<UINT32> *mutableSegmentIds)
   {
      INT32 rc = SDB_OK;
     
      if (OSS_UNLIKELY(!isReady()))
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }

      /// clear modified count first
      for (UINT32 i = 0; i < _bucketCount; ++i)
      {
         ossSLatch *latch = getBucketLatch(i);
         ossSLatchGuard guard(latch, EXCLUSIVE);
         _buckets[i].setPagesImmutable(pageCountPerSeg, mutableSegmentIds);
      }

      _counter.store(0);
   done:
      return rc;
   error:
      goto done;
   }

   INT32 logicalPageIdCache::upsert(PAGE_ID lpid,
                                    const idMapSlot &slot)
   {
      INT32 rc = SDB_OK;
      if (OSS_UNLIKELY(!isReady()))
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }
      else if (OSS_UNLIKELY(INVALID_PAGE_ID == lpid ||
                            slot.isFree()))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      rc = _upsert(lpid, slot, TRUE);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to put lpid[%d] to cache:%d", lpid, rc);
         goto error;
      }

      ++_counter;

   done:
      return rc;
   error:
      goto done;
   }

   INT32 logicalPageIdCache::upsertAsImmutable(PAGE_ID lpid, const idMapSlot &slot)
   {
      INT32 rc = SDB_OK;
      if (OSS_UNLIKELY(!isReady()))
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }
      else if (OSS_UNLIKELY(INVALID_PAGE_ID == lpid ||
                            slot.isFree()))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      rc = _upsert(lpid, slot, FALSE);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to put lpid[%d] to cache:%d", lpid, rc);
         goto error;
      }

   done:
      return rc;
   error:
      goto done;
   }

   INT32 logicalPageIdCache::_upsert(PAGE_ID lpid,
                                     const idMapSlot &slot,
                                     BOOLEAN isMutable)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(INVALID_PAGE_ID != lpid, "can not be invalid");
      SDB_ASSERT(!slot.isFree(), "can not be free");
      partialImpCacheMap::KEY key;
      UINT32 slotInCache = 0;
      UINT32 bucketNo = getBucketAndPartialCacheIdentity(lpid, key, slotInCache);
      ossSLatch *latch = getBucketLatch(bucketNo);
      partialImpCache *cache = NULL;
      partialImpCache *newCache = NULL;
      ossSLatchGuard guard(latch, EXCLUSIVE, FALSE);
      
      do
      {
         guard.lock();
         rc = _buckets[bucketNo].findToUpdate(key, &cache);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to find cache to update:%d", rc);
            goto error;
         }

         if (NULL == cache)
         {
            if (NULL != newCache)
            {
               rc = _buckets[bucketNo].add(key, newCache);
               if (OSS_UNLIKELY(SDB_OK != rc))
               {
                  PD_LOG(PDERROR, "failed to add new cache[%d] to bucket:%d", key, rc);
                  goto error;
               }
               cache = newCache;
               newCache = NULL;
            }
            else
            {
               if (_basePageCount <= key.first)
               {
                  rc = _buckets[bucketNo].addEmptyCache(key, &cache);
                  if (SDB_OK != rc)
                  {
                     PD_LOG(PDERROR, "failed to add empty cache to bucket:%d", rc);
                     goto error;
                  }
               }
               else
               {
                  guard.unlock();
                  rc = createCacheFromBase(key, &newCache);
                  if (SDB_OK != rc)
                  {
                     PD_LOG(PDERROR, "failed to create new cache from base:%d", rc);
                     goto error;
                  }
                  continue;
               }
            }
         }

         SDB_ASSERT(NULL != cache, "impossible");
         rc = cache->upsert(slotInCache, slot, isMutable);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to put lpid[%d] to cache:%d", lpid, rc);
            goto error;
         }
         break;
      } while (TRUE);
      
   done:
      SAFE_OSS_DELETE(newCache);
      return rc;
   error:
      goto done;
   }

   INT32 logicalPageIdCache::remove(PAGE_ID lpid, idMapSlot *slot)
   {
      INT32 rc = SDB_OK;
      if (OSS_UNLIKELY(!isReady()))
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }
      else if (OSS_UNLIKELY(INVALID_PAGE_ID == lpid))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      rc = _remove(lpid, slot);
      if (SDB_OK != rc)
      {
         goto error;
      }

      ++_counter;

   done:
      return rc;
   error:
      goto done;
   }

   INT32 logicalPageIdCache::_remove(PAGE_ID lpid, idMapSlot *slot)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(INVALID_PAGE_ID != lpid, "can not be invalid");
      partialImpCacheMap::KEY key;
      UINT32 slotInCache = 0;
      UINT32 bucketNo = getBucketAndPartialCacheIdentity(lpid, key, slotInCache);
      ossSLatch *latch = getBucketLatch(bucketNo);
      partialImpCache *cache = NULL;
      partialImpCache *newCache = NULL;
      ossSLatchGuard guard(latch, EXCLUSIVE, FALSE);
      
      do
      {
         guard.lock();
         rc = _buckets[bucketNo].findToUpdate(key, &cache);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to find cache to update:%d", rc);
            goto error;
         }

         if (NULL == cache)
         {
            if (NULL != newCache)
            {
               rc = _buckets[bucketNo].add(key, newCache);
               if (OSS_UNLIKELY(SDB_OK != rc))
               {
                  PD_LOG(PDERROR, "failed to add new cache[%d] to bucket:%d", key, rc);
                  goto error;
               }
               cache = newCache;
               newCache = NULL;
            }
            else
            {
               if (_basePageCount <= key.first)
               {
                  PD_LOG(PDERROR, "lpid[%d] not mapped yet", lpid);
                  rc = SDB_VESSEL_LOGICAL_PAGE_UNMAPPED;
                  goto error;
               }
               else
               {
                  guard.unlock();
                  rc = createCacheFromBase(key, &newCache);
                  if (SDB_OK != rc)
                  {
                     PD_LOG(PDERROR, "failed to create new cache from base:%d", rc);
                     goto error;
                  }
                  continue;
               }
            }
         }

         SDB_ASSERT(NULL != cache, "impossible");
         rc = cache->drop(slotInCache, slot);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to drop lpid[%d] in cache:%d", lpid, rc);
            goto error;
         }
         break;
      } while (TRUE);
      
   done:
      SAFE_OSS_DELETE(newCache);
      return rc;
   error:
      goto done;
   }

   INT32 logicalPageIdCache::get(PAGE_ID lpid,
                                 idMapSlot &slot,
                                 BOOLEAN &isMutable)
   {
      INT32 rc = SDB_OK;
      slot.reset();

      if (OSS_UNLIKELY(!isReady()))
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }
      else if (OSS_UNLIKELY(INVALID_PAGE_ID == lpid))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      rc = _get(lpid, slot, isMutable);
      if (SDB_OK != rc)
      {
         goto error;
      }
   done:
      return rc;
   error:
      goto done;
   }

   INT32 logicalPageIdCache::getFromBase(PAGE_ID lpid, idMapSlot &slot)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(NULL != _base, "can not be null");
      SDB_ASSERT(INVALID_PAGE_ID != lpid, "can not be invalid");
      SDB_ASSERT(slot.isFree(), "must be free");
      PAGE_ID pid = getImpPidOfLpid(lpid);
      ossValuePtr ptr = 0;
      idMapSlot tmp;

      if (_basePageCount <= pid)
      {
         PD_LOG(PDDEBUG, "imp pid[%d] is over total page count[%d]", pid, _basePageCount);
         rc = SDB_VESSEL_LOGICAL_PAGE_UNMAPPED;
         goto error;
      }

      rc = _base->getPagePtr(pid, ptr);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to get page[%d] ptr:%d", pid, rc);
         goto error;
      }

      tmp = getIdMapSlot(ptr, getIdMapSlotNo(lpid));
      if (tmp.isFree())
      {
         rc = SDB_VESSEL_LOGICAL_PAGE_UNMAPPED;
         goto error;
      }

      slot = tmp;

   done:
      return rc;
   error:
      goto done;
   }

   INT32 logicalPageIdCache::_get(PAGE_ID lpid,
                                  idMapSlot &slot,
                                  BOOLEAN &isMutable)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(slot.isFree(), "must be free");
      partialImpCacheMap::KEY key;
      UINT32 slotInCache = 0;
      UINT32 bucketNo = getBucketAndPartialCacheIdentity(lpid, key, slotInCache);
      ossSLatch *latch = getBucketLatch(bucketNo);
      const partialImpCache *cache = NULL;
      
      ossSLatchGuard guard(latch, SHARED);
      cache = _buckets[bucketNo].findToRead(key);
      if (NULL != cache)
      {
         rc = cache->get(slotInCache, slot, isMutable);
         if (SDB_OK != rc)
         {
            goto error;
         }
         goto done;
      }

      guard.unlock();

      rc = getFromBase(lpid, slot);
      if (SDB_OK != rc)
      {
         PD_LOG(PDDEBUG, "failed to get lpid[%d] from base file:%d", lpid, rc);
         goto error;
      }

      isMutable = FALSE;
   done:
      return rc;
   error:
      goto done;
   }

   INT32 logicalPageIdCache::resetBaseFileAndClearFlushedMaps(const idMapFile *file)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(NULL != file, "can not be null");
      SDB_ASSERT(file->isOpen(), "can not be closed");

      idMapFileHead head;

      if (OSS_UNLIKELY(!isReady()))
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }
      else if (OSS_UNLIKELY(NULL == file ||
                            !file->isOpen()))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      rc = file->getIdMapFileHead(head);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to get id map file head:%d", rc);
         goto error;
      }

      _base = file;
      _basePageCount = head.totalPageCount;

      for (UINT32 i = 0; i < _bucketCount; ++i)
      {
         ossSLatch *latch = getBucketLatch(i);
         ossSLatchGuard guard(latch, EXCLUSIVE);
         _buckets[i].removeFlushingMap();
      }

   done:
      return rc;
   error:
      goto done;
   }

   INT32 logicalPageIdCache::flushPreparedMapToFile(const logicalPageIdCache::_cacheBucket &bucket,
                                                    idMapFile *file)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(NULL != file, "can not be null");
      idMapFileHead head;
      const partialImpCacheMap *flushing = bucket.getFlushingMap();
      if (NULL == flushing || flushing->get().empty())
      {
         goto done;
      }

      rc = file->getIdMapFileHead(head);
      if (OSS_UNLIKELY(SDB_OK != rc))
      {
         PD_LOG(PDERROR, "failed to get id map file head in file[%s], rc:%d",
                file->getFullPath(), rc);
         goto error;
      }

      for (partialImpCacheMap::CACHE_MAP::const_iterator itr = flushing->get().begin();
           itr != flushing->get().end(); ++itr)
      {
         UINT32 offset = 0;
         ossValuePtr ptr = 0;
         PAGE_ID pid = itr->first.first;

         if (head.totalPageCount <= pid)
         {
            PD_LOG(PDERROR, "pid[%d] over max page count in head[%d]",
                   pid, head.totalPageCount);
            rc = SDB_OUT_OF_BOUND;
            goto error;
         }

         rc = file->getPagePtr(pid, ptr);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to get page[%d] ptr, rc:%d", pid, rc);
            goto error;
         }

         offset = itr->first.second * ID_MAP_PAGE_CACHE_SIZE;
         ossMemcpy((void *)(ptr + offset), itr->second->getBuffer(), ID_MAP_PAGE_CACHE_SIZE);
      }
   done:
      return rc;
   error:
      goto done;
   }

   INT32 logicalPageIdCache::prepareToCreateNewBase(UINT32 pageCountPerSeg,
                                                    ossPoolSet<UINT32> *mutableSegmentIds)
   {
      INT32 rc = SDB_OK;
      if (OSS_UNLIKELY(!isReady()))
      {
         rc = SDB_VESSEL_OUT_OF_RESOURCE;
         goto error;
      }

      for (UINT32 i = 0; i < _bucketCount; ++i)
      {
         ossSLatch *latch = getBucketLatch(i);
         ossScopedLock guard(latch, EXCLUSIVE);
         _buckets[i].setPagesImmutable(pageCountPerSeg, mutableSegmentIds);
         _buckets[i].mergeMainMapToFlushingMap();
      }

      _counter.store(0);

   done:
      return rc;
   error:
      goto done;
   }

   INT32 logicalPageIdCache::createCacheFromBase(const partialImpCacheMap::KEY &key,
                                                 partialImpCache **cache)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(NULL != cache, "can not be null");
      ossValuePtr ptr = 0;
      partialImpCache *tmp = NULL;
      PAGE_ID impPid = key.first;
      UINT32 offset = key.second * ID_MAP_PAGE_CACHE_SIZE;
      SDB_ASSERT(impPid < _basePageCount, "impossible");

      rc = _base->getPagePtr(impPid, ptr);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to get page[%d] ptr:%d", impPid, rc);
         goto error;
      }

      tmp = SDB_OSS_NEW partialImpCache();
      if (OSS_UNLIKELY(NULL == tmp))
      {
         PD_LOG(PDERROR, "failed to allocate mem");
         rc = SDB_OOM;
         goto error;
      }

      tmp->copy((const void *)(ptr + offset), 0);
      *cache = tmp;
      tmp = NULL;
   done:
      return rc;
   error:
      SAFE_OSS_DELETE(tmp);
      goto done;
   }

   UINT32 logicalPageIdCache::getBucketAndPartialCacheIdentity(PAGE_ID lpid,
                                                               partialImpCacheMap::KEY &key,
                                                               UINT32 &slotInPartialCache)const
   {
      SDB_ASSERT(INVALID_PAGE_ID != lpid, "can not be invalid");
      PAGE_ID impPid = getImpPidOfLpid(lpid);
      UINT32 bucket = (impPid & (_bucketCount - 1));
      UINT32 offset = (lpid % ID_MAP_PAGE_CAPACITY) /
                      ID_MAP_PAGE_CACHE_SLOT_COUNT;

      slotInPartialCache = (lpid % ID_MAP_PAGE_CAPACITY) %
                           ID_MAP_PAGE_CACHE_SLOT_COUNT;
      key.first = impPid;
      key.second = offset;
      return bucket;
   }
}//namespace vessel
}//namespace engine
