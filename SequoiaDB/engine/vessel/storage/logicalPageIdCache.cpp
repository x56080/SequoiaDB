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

   const partialImpCache *partialImpCacheMap::find(UINT32 key)const
   {
      const partialImpCache *out = NULL;
      CACHE_MAP::const_iterator itr = _map.find(key);
      if (_map.end() != itr)
      {
         out = itr->second;
      }
      return out;
   }

   partialImpCache *partialImpCacheMap::find(UINT32 key)
   {
      partialImpCache *out = NULL;
      CACHE_MAP::const_iterator itr = _map.find(key);
      if (_map.end() != itr)
      {
         out = itr->second;
      }
      return out;
   }

   INT32 partialImpCacheMap::insert(UINT32 key, partialImpCache *cache)
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

   INT32 logicalPageIdCache::_cacheBucket::add(UINT32 key, partialImpCache *cache)
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

   const partialImpCache *logicalPageIdCache::_cacheBucket::findToRead(UINT32 key)const
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

   INT32 logicalPageIdCache::_cacheBucket::findToUpdate(UINT32 key,
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
            newCache->reset(tmp->getBuffer(), 0);
            
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
      if (NULL == _mainMap)
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
         SDB_OSS_DEL _mainMap;
         _mainMap = NULL;
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

   UINT32 logicalPageIdCache::_cacheBucket::getTotalCacheSize()const
   {
      UINT32 size = 0;
      if (NULL != _mainMap)
      {
         size = _mainMap->get().size() * ID_MAP_PAGE_CACHE_SIZE;
      }
      if (NULL != _flushingMap)
      {
         size += _flushingMap->get().size() * ID_MAP_PAGE_CACHE_SIZE;
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

   INT32 logicalPageIdCache::_cacheBucket::addEmptyCache(UINT32 key,
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

   ////////////////logicalPageIdCache
   logicalPageIdCache::logicalPageIdCache()
   {
      SDB_ASSERT(64 == ID_MAP_PAGE_CACHE_SLOT_COUNT, "impossible");
      /// Should update some functions, eg: getKeyByLpid.
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
      _latches = SDB_OSS_NEW ossSpinSLatch[_latchCount];
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

      if (o.mutablePidAllowed)
      {
         setMutablePidAllowed();
      }
   done:
      return rc;
   error:
      fini();
      goto done;
   }

   void logicalPageIdCache::fini()
   {
      _flags = 0;
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
      return;
   }

   UINT32 logicalPageIdCache::getTotalCacheSize()const
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
         rc = flushBucketToFile(_buckets[i], file);
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

   INT32 logicalPageIdCache::createMutablePidListAndSetInmmutable(ossPoolVector<PAGE_ID> &mutablePids)
   {
      INT32 rc = SDB_OK;
      if (OSS_UNLIKELY(!isReady()))
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }
      else if (OSS_UNLIKELY(!isMutablePidAllowed()))
      {
         rc = SDB_VESSEL_OPERATOION_NOT_PERMITTED;
         goto error;
      }

      for (UINT32 i = 0; i < _bucketCount; ++i)
      {
         ossSpinSLatch *latch = getBucketLatch(i);
         ossScopedLock guard(latch, EXCLUSIVE);
         _buckets[i].dumpMutablePidsAndSetImmutable(mutablePids);
      }

   done:
      return rc;
   error:
      goto done;
   }

   INT32 logicalPageIdCache::upsert(PAGE_ID lpid,
                                    const idMapSlot &slot,
                                    BOOLEAN isMutable)
   {
      INT32 rc = SDB_OK;


      if (OSS_UNLIKELY(isReady()))
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
      else if(isMutable && !isMutablePidAllowed())
      {
         rc = SDB_VESSEL_OPERATOION_NOT_PERMITTED;
         goto error;
      }

      rc = _upsert(lpid, slot, isMutable);
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
      UINT32 key = getKeyByLpid(lpid);
      UINT32 bucketNo = getBucketNo(lpid);
      ossSpinSLatch *latch = getBucketLatch(bucketNo);
      partialImpCache *cache = NULL;
      partialImpCache *newCache = NULL;
      ossSpinSLatchGuard guard(latch, EXCLUSIVE, FALSE);
      
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
               PAGE_ID impPid = getImpPidByKey(key);
               if (_basePageCount <= impPid)
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
         rc = cache->upsert(partialImpCache::getSlotNoByLpid(lpid), slot, isMutable);
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
   done:
      return rc;
   error:
      goto done;
   }

   INT32 logicalPageIdCache::_remove(PAGE_ID lpid, idMapSlot *slot)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(INVALID_PAGE_ID != lpid, "can not be invalid");
      UINT32 key = getKeyByLpid(lpid);
      UINT32 bucketNo = getBucketNo(lpid);
      ossSpinSLatch *latch = getBucketLatch(bucketNo);
      partialImpCache *cache = NULL;
      partialImpCache *newCache = NULL;
      ossSpinSLatchGuard guard(latch, EXCLUSIVE, FALSE);
      
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
               PAGE_ID impPid = getImpPidByKey(key);
               if (_basePageCount <= impPid)
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
         rc = cache->drop(partialImpCache::getSlotNoByLpid(lpid), slot);
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
      const idMapSlot *tmp = NULL;

      if (_basePageCount <= pid)
      {
         PD_LOG(PDERROR, "imp pid[%d] is over total page count[%d]", pid, _basePageCount);
         rc = SDB_OUT_OF_BOUND;
         goto error;
      }

      rc = _base->getPagePtr(pid, ptr);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to get page[%d] ptr:%d", pid, rc);
         goto error;
      }

      tmp = getIdMapSlot(ptr, getIdMapSlotNo(lpid));
      if (OSS_UNLIKELY(NULL == tmp))
      {
         PD_LOG(PDERROR, "failed to get id map slot[%d]", lpid);
         rc = SDB_VESSEL_INTERNAL_ERR;
         goto error;
      }
      else if (tmp->isFree())
      {
         rc = SDB_VESSEL_LOGICAL_PAGE_UNMAPPED;
         goto error;
      }

      slot = *tmp;

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
      UINT32 key = getKeyByLpid(lpid);
      UINT32 bucketNo = getBucketNo(lpid);
      ossSpinSLatch *latch = getBucketLatch(bucketNo);
      const partialImpCache *cache = NULL;
      
      ossSpinSLatchGuard guard(latch, SHARED);
      cache = _buckets[bucketNo].findToRead(key);
      if (NULL != cache)
      {
         rc = cache->get(partialImpCache::getSlotNoByLpid(lpid), slot, isMutable);
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
         PD_LOG(PDERROR, "failed to get lpid[%d] from base file:%d", lpid, rc);
         goto error;
      }

      isMutable = FALSE;
   done:
      return rc;
   error:
      goto done;
   }

/*
   void logicalPageIdCache::incTotalCacheCount(UINT32 count)
   {
      if (0 < count)
      {
         ossFetchAndAdd32(&_totalCacheCount, count);
      }
      return;
   }

   void logicalPageIdCache::decTotalCacheCount(UINT32 count)
   {  
      if (0 < count)
      {
         INT32 cnt = 0;
         cnt -= count;
         ossFetchAndAdd32(&_totalCacheCount, cnt);
      }
      return;
   }
   */

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
         ossSpinSLatch *latch = getBucketLatch(i);
         ossScopedLock guard(latch, EXCLUSIVE);
         _buckets[i].removeFlushingMap();
      }

   done:
      return rc;
   error:
      goto done;
   }

   INT32 logicalPageIdCache::flushBucketToFile(const logicalPageIdCache::_cacheBucket &bucket,
                                               idMapFile *file)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(NULL != file, "can not be null");
      idMapFileHead head;
      const partialImpCacheMap *flushing = bucket.getFlushingMap();
      if (NULL == flushing)
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

      for (partialImpCacheMap::CONST_ITERATOR itr = flushing->begin();
           itr != flushing->end(); ++itr)
      {
         UINT32 offset = 0;
         ossValuePtr ptr = 0;
         PAGE_ID pid = getImpPidByKey(itr->first);

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

         offset = getOffsetInImpByKey(itr->first);
         ossMemcpy((void *)(ptr + offset), itr->second->getBuffer(), ID_MAP_PAGE_CACHE_SIZE);
      }
   done:
      return rc;
   error:
      goto done;
   }

   INT32 logicalPageIdCache::prepareToCreateNewBase(ossPoolVector<PAGE_ID> &mutablePids)
   {
      INT32 rc = SDB_OK;
      if (OSS_UNLIKELY(!isReady()))
      {
         rc = SDB_VESSEL_OUT_OF_RESOURCE;
         goto error;
      }
      else if (OSS_UNLIKELY(!isMutablePidAllowed()))
      {
         rc = SDB_VESSEL_OPERATOION_NOT_PERMITTED;
         goto error;
      }

      for (UINT32 i = 0; i < _bucketCount; ++i)
      {
         ossSpinSLatch *latch = getBucketLatch(i);
         ossScopedLock guard(latch, EXCLUSIVE);
         _buckets[i].dumpMutablePidsAndSetImmutable(mutablePids);
         _buckets[i].mergeToFlushingMap();
      }

   done:
      return rc;
   error:
      goto done;
   }

   INT32 logicalPageIdCache::prepareToCreateNewBase()
   {
      INT32 rc = SDB_OK;
      if (OSS_UNLIKELY(!isReady()))
      {
         rc = SDB_VESSEL_OUT_OF_RESOURCE;
         goto error;
      }
      else if (OSS_UNLIKELY(isMutablePidAllowed()))
      {
         rc = SDB_VESSEL_OPERATOION_NOT_PERMITTED;
         goto error;
      }

      for (UINT32 i = 0; i < _bucketCount; ++i)
      {
         ossSpinSLatch *latch = getBucketLatch(i);
         ossScopedLock guard(latch, EXCLUSIVE);
         _buckets[i].mergeToFlushingMap();
      }

   done:
      return rc;
   error:
      goto done;
   }

   INT32 logicalPageIdCache::createCacheFromBase(UINT32 key,
                                                 partialImpCache **cache)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(NULL != cache, "can not be null");
      ossValuePtr ptr = 0;
      partialImpCache *tmp = NULL;
      PAGE_ID impPid = getImpPidByKey(key);
      UINT32 offset = getOffsetInImpByKey(key);
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

      tmp->reset((const void *)(ptr + offset), 0);
      *cache = tmp;
      tmp = NULL;
   done:
      return rc;
   error:
      SAFE_OSS_DELETE(tmp);
      goto done;
   }
}//namespace vessel
}//namespace engine
