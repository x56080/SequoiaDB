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
#include "vessel/deltaLogFileDef.h"

namespace engine
{
namespace vessel
{

#pragma pack(4)
   struct _partialCacheDumpEle
   {
      static UINT32 getKeySize()
      {
         return sizeof(UINT32) + sizeof(UINT32);
      }
      UINT32 imp;
      UINT32 offset;
      CHAR cache[ID_MAP_PARTIAL_PAGE_CACHE_SIZE];
   };

   static const UINT32 _PARTIAL_CACHE_DUMP_ELE_SIZE = sizeof(_partialCacheDumpEle);
   static const UINT32 _PARTIAL_CACHE_DUMP_ELE_COUNT_PER_PAGE = 
   deltaLogFile::PAGE_SIZE / _PARTIAL_CACHE_DUMP_ELE_SIZE;

#pragma pack()

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
      SDB_ASSERT(isValidKey(key), "can not be invalid");
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
      else if (OSS_UNLIKELY(!isValidKey(key)))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      if (!_map.insert(std::make_pair(key, cache)).second)
      {
         PD_LOG(PDERROR, "duplicated key[%d, %d]", key.first, key.second);
         rc = SDB_VESSEL_DUPLICATED_KEY;
         goto error;
      }
   done:
      return rc;
   error:
      goto done;
   }

   BOOLEAN partialImpCacheMap::isValidKey(const KEY &key)
   {
      return INVALID_PAGE_ID != key.first &&
             0 == key.second % ID_MAP_PARTIAL_PAGE_CACHE_SIZE &&
             key.second < ID_MAP_FILE_PAGE_SIZE;
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

///////////////////////////partialImpCacheMap end

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

      _latches.resize(o.bucketLatchCount);
      _buckets.resize(o.bucketCount);
      for (UINT32 i = 0; i < o.bucketCount; ++i)
      {
         partialImpCacheMap *cm = SDB_OSS_NEW partialImpCacheMap();
         if (OSS_UNLIKELY(NULL == cm))
         {
            PD_LOG(PDERROR, "failed to allocate mem.");
            rc = SDB_OOM;
            goto error;
         }
         _buckets[i] = cm;
      }

      rc = resetBase(base);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to reset base file:%d", rc);
         goto error;
      }
   
   done:
      return rc;
   error:
      fini();
      goto done;
   }

   INT32 logicalPageIdCache::restore(const deltaLogFile *delta)
   {
      INT32 rc = SDB_OK;
      if (OSS_UNLIKELY(!isReady()))
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }
      else if (OSS_UNLIKELY(NULL == delta ||
                            !delta->isOpen()))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      rc = restoreDeltaCache(delta);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to restore delta cache:%d", rc);
         goto error;
      }
   done:
      return rc;
   error:
      goto done;
   }

   void logicalPageIdCache::fini()
   {
      _base = NULL;
      _basePageCount = 0;
      _latches.clear();
      _immutableMap.fini();
      _immutableCache = NULL;
      _counter.store(0);

      for (UINT32 i = 0; i < _buckets.size(); ++i)
      {
         if (NULL != _buckets[i])
         {
            SDB_OSS_DEL _buckets[i];
         }
      }
      _buckets.clear();

      return;
   }

   UINT64 logicalPageIdCache::getTotalCacheSize()const
   {
      UINT64 size = 0;
      for (UINT32 i = 0; i < _buckets.size(); ++i)
      {
         size += _buckets[i]->getCacheSize();
      }

      return size;
   }

   INT32 logicalPageIdCache::estimateMutablePageCount()const
   {
      return _counter.load();
   }

   INT32 logicalPageIdCache::flushPreparedCacheToFile(idMapFile *file)
   {
      INT32 rc = SDB_OK;
      idMapFileHead head;
      BOOLEAN locked = FALSE;

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

      if (_immutableMap.isEmpty())
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

      _latch.lock_r();
      locked = TRUE;

      for (partialImpCacheMap::CACHE_MAP::const_iterator itr = _immutableMap.begin();
           itr != _immutableMap.end(); ++itr)
      {
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

         ossMemcpy((void *)(ptr + itr->first.second),
                   itr->second->getBuffer(),
                   ID_MAP_PARTIAL_PAGE_CACHE_SIZE);
      }


   done:
      if (locked)
      {
         _latch.release_r();
      }
      return rc;
   error:
      goto done;
   }

   INT32 logicalPageIdCache::dumpBufferAndSetImmutable(UINT32 pageCountPerSeg,
                                                       ossPoolVector<memoryBlock> &buffers,
                                                       UINT32 &itemCount,
                                                       ossPoolSet<UINT32> *mutableSegmentIds)
   {
      INT32 rc = SDB_OK;
      UINT32 pushed = 0;
      UINT32 segmentSize = deltaLogFile::FILE_SEGMENT_SIZE;
      memoryBlock buffer;
      BOOLEAN locked = FALSE;
      UINT32 mutableCount = 0;

      buffers.clear();
      itemCount = 0;

      SDB_ASSERT(0 < pageCountPerSeg, "invalid");
     
      if (OSS_UNLIKELY(!isReady()))
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }

      rc = buffer.reserve(segmentSize);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to reserve mb size:%d", rc);
         goto error;
      }

      _latch.lock_w();
      locked = TRUE;

      /// clear modified count first
      for (UINT32 i = 0; i < getBucketCount(); ++i)
      {
         partialImpCacheMap &cacheMap = *_buckets[i];
         partialImpCacheMap::CACHE_MAP::const_iterator itr = cacheMap.begin();
         for (; itr != cacheMap.end(); ++itr)
         {
            UINT32 cnt = 0;
            UINT32 freeSize = segmentSize - buffer.getSize();
            if (freeSize < _PARTIAL_CACHE_DUMP_ELE_SIZE)
            {
               buffers.push_back(std::move(buffer));
               rc = buffer.reserve(segmentSize);
               if (SDB_OK != rc)
               {
                  PD_LOG(PDERROR, "failed to reserve mb size:%d", rc);
                  goto error;
               }
            }

            buffer.append(sizeof(UINT32), &(itr->first.first));
            buffer.append(sizeof(UINT32), &(itr->first.second));
            buffer.append(ID_MAP_PARTIAL_PAGE_CACHE_SIZE, itr->second->getBuffer());
            itr->second->setAllPageImmutable(pageCountPerSeg, mutableSegmentIds, &cnt);
            ++pushed;
            mutableCount += cnt;
         }
      }

      _latch.release_w();
      locked = FALSE;

      if (0 < buffer.getSize())
      {
         buffers.push_back(std::move(buffer));
      }

      _counter.store(0);
      itemCount = pushed;

      PD_LOG(PDDEBUG, "dump mutable buffer:%d, item count:%d, mutable pages:%d",
             buffers.size(), pushed, mutableCount);
   done:
      if (locked)
      {
         _latch.release_w();
      }
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

      {
         ossScopedRWLock guard(&_latch, SHARED);
         rc = _upsert(lpid, slot);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to put lpid[%d] to cache:%d", lpid, rc);
            goto error;
         }
      }

      ++_counter;

   done:
      return rc;
   error:
      goto done;
   }

   INT32 logicalPageIdCache::_upsert(PAGE_ID lpid,
                                     const idMapSlot &slot)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(INVALID_PAGE_ID != lpid, "can not be invalid");
      SDB_ASSERT(!slot.isFree(), "can not be free");
      partialImpCacheMap::KEY key;
      UINT32 slotInCache = 0;
      UINT32 bucketPos = getBucketAndPartialCacheIdentity(lpid, key, slotInCache);
      ossSLatch *latch = getBucketLatch(bucketPos);
      partialImpCache *cache = NULL;
      partialImpCache *newCache = NULL;
      ossSLatchGuard guard(latch, EXCLUSIVE, FALSE);
      
      do
      {
         guard.lock();
         rc = findInMemToUpdate(bucketPos, key, &cache);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to find cache to update:%d", rc);
            goto error;
         }

         if (NULL != cache)
         {
            break;
         }
         else if (NULL != newCache)
         {
            rc = _buckets[bucketPos]->insert(key, newCache);
            if (OSS_UNLIKELY(SDB_OK != rc))
            {
               PD_LOG(PDERROR, "failed to add new cache[%d, %d] to bucket:%d",
                        key.first, key.second, rc);
               goto error;
            }
            cache = newCache;
            newCache = NULL;
            break;
         }
         else if (_basePageCount <= key.first)
         {
            newCache = SDB_OSS_NEW partialImpCache();
            if (OSS_UNLIKELY(NULL == newCache))
            {
               PD_LOG(PDERROR, "failed to allocate mem.");
               rc = SDB_OOM;
               goto error;
            }

            rc = _buckets[bucketPos]->insert(key, newCache);
            if (OSS_UNLIKELY(SDB_OK != rc))
            {
               PD_LOG(PDERROR, "failed to add new cache[%d, %d] to bucket:%d",
                        key.first, key.second, rc);
               goto error;
            }
            cache = newCache;
            newCache = NULL;
            break;
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
      } while (TRUE);

      SDB_ASSERT(NULL != cache, "impossible");
      rc = cache->upsert(slotInCache, slot);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to put lpid[%d] to cache:%d", lpid, rc);
         goto error;
      }
      
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

      {
         ossScopedRWLock guard(&_latch, SHARED);
         rc = _remove(lpid, slot);
         if (SDB_OK != rc)
         {
            goto error;
         }
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
      UINT32 bucketPos = getBucketAndPartialCacheIdentity(lpid, key, slotInCache);
      ossSLatch *latch = getBucketLatch(bucketPos);
      partialImpCache *cache = NULL;
      partialImpCache *newCache = NULL;
      ossSLatchGuard guard(latch, EXCLUSIVE, FALSE);
      
      do
      {
         guard.lock();
         rc = findInMemToUpdate(bucketPos, key, &cache);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to find cache to update:%d", rc);
            goto error;
         }

         if (NULL != cache)
         {
            break;
         }
         else if (NULL != newCache)
         {
            rc = _buckets[bucketPos]->insert(key, newCache);
            if (OSS_UNLIKELY(SDB_OK != rc))
            {
               PD_LOG(PDERROR, "failed to add new cache[%d, %d] to bucket:%d",
                        key.first, key.second, rc);
               goto error;
            }
            cache = newCache;
            newCache = NULL;
            break;
         }
         else if (_basePageCount <= key.first)
         {
            newCache = SDB_OSS_NEW partialImpCache();
            if (OSS_UNLIKELY(NULL == newCache))
            {
               PD_LOG(PDERROR, "failed to allocate mem.");
               rc = SDB_OOM;
               goto error;
            }

            rc = _buckets[bucketPos]->insert(key, newCache);
            if (OSS_UNLIKELY(SDB_OK != rc))
            {
               PD_LOG(PDERROR, "failed to add new cache[%d, %d] to bucket:%d",
                        key.first, key.second, rc);
               goto error;
            }
            cache = newCache;
            newCache = NULL;
            break;
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
      } while (TRUE);

      SDB_ASSERT(NULL != cache, "can not be null");
      rc = cache->remove(slotInCache, slot);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to remove lpid[%d] rc:%d", lpid, rc);
         goto error;
      }
      
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

      {
         ossScopedRWLock guard(&_latch, SHARED);
         rc = _get(lpid, slot, isMutable);
         if (SDB_OK != rc)
         {
            goto error;
         }
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
         //PD_LOG(PDDEBUG, "imp pid[%d] is over total page count[%d]", pid, _basePageCount);
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
      SDB_ASSERT(INVALID_PAGE_ID != lpid, "can not be invalid");

      partialImpCacheMap::KEY key;
      UINT32 slotInCache = 0;
      UINT32 bucketPos = getBucketAndPartialCacheIdentity(lpid, key, slotInCache);
      ossSLatch *latch = getBucketLatch(bucketPos);
      const partialImpCache *cache = NULL;
      const idMapSlot *slotPtr = NULL; 

      ossSLatchGuard guard(latch, SHARED);
      cache = _buckets[bucketPos]->find(key);
      if (NULL != cache)
      {
         slotPtr = cache->get(slotInCache, isMutable);
         if (OSS_UNLIKELY(NULL == slotPtr))
         {
            PD_LOG(PDERROR, "may be ouf of bound");
            rc = SDB_VESSEL_INTERNAL_ERR;
            goto error;
         }

         if (slotPtr->isFree())
         {
            rc = SDB_VESSEL_LOGICAL_PAGE_UNMAPPED;
            goto error;
         }
         else
         {
            slot = *slotPtr;
            goto done;
         }
      }

      guard.unlock();

      /// user should holding lpid lock now.
      /// once we can not find it in mutable cache,
      /// just release the bucket latch.

      if (NULL != _immutableCache)
      {
         cache = _immutableCache->find(key);
         if (NULL != cache)
         {
            slotPtr = cache->get(slotInCache, isMutable);
            if (OSS_UNLIKELY(NULL == slotPtr))
            {
               PD_LOG(PDERROR, "may be ouf of bound");
               rc = SDB_VESSEL_INTERNAL_ERR;
               goto error;
            }

            if (slotPtr->isFree())
            {
               rc = SDB_VESSEL_LOGICAL_PAGE_UNMAPPED;
               goto error;
            }
            else
            {
               slot = *slotPtr;
               goto done;
            }
         }
      }

      rc = getFromBase(lpid, slot);
      if (SDB_OK != rc)
      {
         //PD_LOG(PDDEBUG, "failed to get lpid[%d] from base file:%d", lpid, rc);
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
      BOOLEAN locked = FALSE;

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

      _latch.lock_w();
      locked = TRUE;
      _base = file;
      _basePageCount = head.totalPageCount;
      _immutableCache = NULL;
      _latch.release_w();
      locked = FALSE;
      _immutableMap.fini();
      
   done:
      if (locked)
      {
         _latch.release_w();
      }
      return rc;
   error:
      goto done;
   }

   INT32 logicalPageIdCache::prepareToCreateNewBase(UINT32 pageCountPerSeg,
                                                    ossPoolSet<UINT32> *mutableSegmentIds)
   {
      INT32 rc = SDB_OK;
      BOOLEAN locked = FALSE;

      if (OSS_UNLIKELY(!isReady()))
      {
         rc = SDB_VESSEL_OUT_OF_RESOURCE;
         goto error;
      }

      SDB_ASSERT(_immutableMap.isEmpty(), "must be empty");

      _latch.lock_w();
      locked = TRUE;

      for (UINT32 i = 0; i < getBucketCount(); ++i)
      {
         UINT32 replaced = 0;
         partialImpCacheMap &cacheMap = *_buckets[i];
         cacheMap.exportTo(_immutableMap, replaced);
      }

      if (!_immutableMap.isEmpty())
      {
         _immutableCache = &_immutableMap;
      }

      _latch.release_w();
      locked = FALSE;

      _counter.store(0);

   done:
      if (locked)
      {
         _latch.release_w();
      }
      return rc;
   error:
      if (NULL != mutableSegmentIds)
      {
         mutableSegmentIds->clear();
      }
      goto done;
   }

   INT32 logicalPageIdCache::createCacheFromBase(const partialImpCacheMap::KEY &key,
                                                 partialImpCache **cache)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(partialImpCacheMap::isValidKey(key), "can not be invalid");
      SDB_ASSERT(NULL != cache, "can not be null");
      ossValuePtr ptr = 0;
      partialImpCache *tmp = NULL;
      SDB_ASSERT( key.first < _basePageCount, "impossible");

      rc = _base->getPagePtr(key.first, ptr);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to get page[%d] ptr:%d",  key.first, rc);
         goto error;
      }

      tmp = SDB_OSS_NEW partialImpCache();
      if (OSS_UNLIKELY(NULL == tmp))
      {
         PD_LOG(PDERROR, "failed to allocate mem");
         rc = SDB_OOM;
         goto error;
      }

      tmp->copy((const void *)(ptr + key.second), 0);
      *cache = tmp;
      tmp = NULL;
   done:
      return rc;
   error:
      SAFE_OSS_DELETE(tmp);
      goto done;
   }

   INT32 logicalPageIdCache::_insertWhenRestore(const partialImpCacheMap::KEY &key,
                                                const CHAR *cache)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(isReady(), "can not be invalid");
      SDB_ASSERT(INVALID_PAGE_ID != key.first && key.second < ID_MAP_FILE_PAGE_SIZE, 
                 "can not be invalid");
      SDB_ASSERT(NULL != cache, "can not be invalid");

      partialImpCache *cacheObj = SDB_OSS_NEW partialImpCache();
      if (OSS_UNLIKELY(NULL == cacheObj))
      {
         PD_LOG(PDERROR, "failed to allocate mem.");
         rc = SDB_OOM;
         goto error;
      }

      cacheObj->copy(cache, 0);
      rc = _immutableMap.insert(key, cacheObj);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to insert into immutable map:%d", rc);
         goto error;
      }

   done:
      return rc;
   error:
      SAFE_OSS_DELETE(cacheObj);
      goto done;
   }

   UINT32 logicalPageIdCache::getBucketAndPartialCacheIdentity(PAGE_ID lpid,
                                                               partialImpCacheMap::KEY &key,
                                                               UINT32 &slotInPartialCache)const
   {
      SDB_ASSERT(INVALID_PAGE_ID != lpid, "can not be invalid");
      SDB_ASSERT(!_buckets.empty(), "can not be empty");

      PAGE_ID impPid = getImpPidOfLpid(lpid);
      UINT32 pos = lpid / ID_MAP_PARTIAL_CACHE_SLOT_COUNT;
      UINT32 bucket =  pos & (getBucketCount() - 1);
      UINT32 offset = (pos % ID_MAP_PARTIAL_CACHE_COUNT_PER_IMP) *
                      ID_MAP_PARTIAL_PAGE_CACHE_SIZE;

      slotInPartialCache = lpid % ID_MAP_PARTIAL_CACHE_SLOT_COUNT;
      key.first = impPid;
      key.second = offset;
      return bucket;
   }

   INT32 logicalPageIdCache::resetBase(const idMapFile *base)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(NULL != base && base->isOpen(), "can not be invalid");
      idMapFileHead head;
      rc = base->getIdMapFileHead(head);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to get id map file head from file[%s], rc:%d",
                _base->getFullPath(), rc);
         goto error;
      }

      _basePageCount = head.totalPageCount;
      _base = base;
   done:
      return rc;
   error:
      goto done;
   }

   INT32 logicalPageIdCache::restoreDeltaCache(const deltaLogFile *delta)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(NULL != _base, "init base first");
      SDB_ASSERT(NULL != delta && delta->isOpen(), "can not be invalid");

      UINT32 totalEleCount = 0;
      UINT32 read = 0;
      deltaLogFileHead head;
      UINT32 segmentSize = 0;

      rc = delta->getDeltaLogFileHead(head);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to get delta log file head:%d", rc);
         goto error;
      }
     
      SDB_ASSERT(_base->getCommonHeadInMem().sequence <= head.baseIdMapFile,
                 "not valid delta file");

      segmentSize = delta->getCommonHeadInMem().getSegmentSize();
      totalEleCount = (UINT32)head.elementCount;

      PD_LOG(PDINFO, "total ele count[%d] in delta file[%s]",
             totalEleCount, delta->getFullPath());

      for (UINT32 i = 0; i < delta->getSegmentCount(); ++i)
      {
         UINT32 offset = 0;
         ossValuePtr ptr;
         rc = delta->getSegmentPtr(i, ptr);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to get segment[%d] ptr:%d", i, rc);
            goto error;
         }

         while (read < totalEleCount)
         {
            const _partialCacheDumpEle *ele = (const _partialCacheDumpEle *)(ptr + offset);
            partialImpCacheMap::KEY key;
            key.first = ele->imp;
            key.second = ele->offset;
            rc = _insertWhenRestore(key, ele->cache);
            if (SDB_OK != rc)
            {
               PD_LOG(PDERROR, "failed to restore delta log file[%s]at segment[%d], offset[%d], rc:%d",
                      delta->getFullPath(), i, offset, rc);
               goto error;
            }

            ++read;
            offset += _PARTIAL_CACHE_DUMP_ELE_SIZE;
            if (segmentSize <= (offset + _PARTIAL_CACHE_DUMP_ELE_SIZE))
            {
               break;
            }
         }
      }

      if (read != totalEleCount)
      {
         PD_LOG(PDERROR, "unexpected count[%d] from file [%s]", read, delta->getFullPath());
         rc = SDB_VESSEL_INTERNAL_ERR;
         goto error;
      }

      if (!_immutableMap.isEmpty())
      {
         _immutableCache = &_immutableMap;
      }
   done:
      return rc;
   error:
      _immutableMap.fini();
      goto done;
   }

   INT32 logicalPageIdCache::findInMemToUpdate(UINT32 bucketPos,
                                               const partialImpCacheMap::KEY &key,
                                               partialImpCache **out)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(bucketPos < getBucketCount(), "out of bound");
      SDB_ASSERT(partialImpCacheMap::isValidKey(key), "can not be invalid");
      SDB_ASSERT(NULL != out, "can not be null");

      partialImpCache *cache = _buckets[bucketPos]->find(key);
      if (NULL != cache)
      {
         *out = cache;
         goto done;
      }

      if (NULL != _immutableCache)
      {
         const partialImpCache *immutableCache = _immutableCache->find(key);
         if (NULL != immutableCache)
         {
            cache = SDB_OSS_NEW partialImpCache();
            if (OSS_UNLIKELY(NULL == cache))
            {
               PD_LOG(PDERROR, "failed to allocate mem.");
               rc = SDB_OOM;
               goto error;
            }

            cache->copy(immutableCache->getBuffer(), 0);
            _buckets[bucketPos]->insert(key, cache);
            *out = cache;
         }
      }
   done:
      return rc;
   error:
      goto done;
   }
}//namespace vessel
}//namespace engine
