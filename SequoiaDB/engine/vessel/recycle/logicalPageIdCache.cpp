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
///////////////////////////partialImpCacheMap
   void partialImpCacheMap::fini()
   {
      _mutablePos = 0;
      _maps[0].clear();
      _maps[1].clear();
   }

   BOOLEAN partialImpCacheMap::find(PAGE_ID lpid,
                                    idMapSlot &value,
                                    BOOLEAN &isMutable)const
   {
      BOOLEAN r = FALSE;
      SDB_ASSERT(INVALID_PAGE_ID != lpid, "can not be invalid");
      value.reset();
      CACHE_MAP::const_iterator itr = getMutableMap().find(lpid);
      if (getMutableMap().end() != itr)
      {
         value = itr->second;
         r = TRUE;
         isMutable = TRUE;
      }
      else
      {
         itr = getImmutableMap().find(lpid);
         if (getImmutableMap().end() != itr)
         {
            value = itr->second;
            r = TRUE;
            isMutable = FALSE;
         }
      }

      return r;
   }


   BOOLEAN partialImpCacheMap::insert(PAGE_ID lpid, const idMapSlot &value)
   {
      SDB_ASSERT(INVALID_PAGE_ID != lpid, "can not be invalid");
      SDB_ASSERT(!value.isFree(), "can not be invalid");
      BOOLEAN r = FALSE;
      std::pair<CACHE_MAP::iterator, BOOLEAN> result = 
                                getMutableMap().insert(std::make_pair(lpid, value));
      if (!result.second)
      {
         if (result.first->second.isFree())
         {
            result.first->second = value;
            r = TRUE;
         }
      }
      else
      {
         r = TRUE;
      }
      return r;
   }

   void partialImpCacheMap::upsert(PAGE_ID lpid, const idMapSlot &value)
   {
      SDB_ASSERT(INVALID_PAGE_ID != lpid, "can not be invalid");
      SDB_ASSERT(!value.isFree(), "can not be invalid");
      getMutableMap()[lpid] = value;
   }

   void partialImpCacheMap::remove(PAGE_ID lpid)
   {
      SDB_ASSERT(INVALID_PAGE_ID != lpid, "can not be invalid");
      getMutableMap()[lpid] = idMapSlot();
   }

   void partialImpCacheMap::restoreToImmutableMap(PAGE_ID lpid, const idMapSlot &value)
   {
      SDB_ASSERT(INVALID_PAGE_ID != lpid, "can not be invalid");
      SDB_ASSERT(getMutableMap().empty(), "must be empty");
      _getImmutableMap()[lpid] = value;
   }

   void partialImpCacheMap::switchMap()
   {
      CACHE_MAP &mm = getMutableMap();
      CACHE_MAP &imm = _getImmutableMap();
      if (mm.size() <= imm.size())
      {
         for (CACHE_MAP::const_iterator itr = mm.begin();
            itr != mm.end(); ++itr)
         {
            imm[itr->first] = itr->second;                               
         }
         mm.clear();
      }
      else
      {
         for (CACHE_MAP::const_iterator itr = imm.begin();
            itr != imm.end(); ++itr)
         {
            /// ingore duplicated key
            mm.insert(std::make_pair(itr->first, itr->second));
         }
         imm.clear();
         _mutablePos ^= 1;
      }
   }

   void partialImpCacheMap::clearImmutableMap()
   {
      _getImmutableMap().clear();
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

      rc = resetBase(base);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to reset base file:%d", rc);
         goto error;
      }

      _latches.resize(o.bucketLatchCount);
      _buckets.resize(o.bucketCount, NULL);
      for (UINT32 i = 0; i < o.bucketCount; ++i)
      {
         partialImpCacheMap *pm = SDB_OSS_NEW partialImpCacheMap();
         if (NULL == pm)
         {
            PD_LOG(PDERROR, "failed to allocate mem.");
            rc = SDB_OOM;
            goto error;
         }

         _buckets[i] = pm;
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
      _latches.clear();
      for (UINT32 i = 0; i < _buckets.size(); ++i)
      {
         if (NULL != _buckets[i])
         {
            SDB_OSS_DEL _buckets[i];
         }
      }
      _buckets.clear();
      _counter.store(0);
      return;
   }

   UINT64 logicalPageIdCache::getFuzzyCacheSize()
   {
      SDB_ASSERT(isReady(), "can not be invalid");
      UINT64 count = 0;
      _latch.lock_r();
      for (UINT32 i = 0; i < _buckets.size(); ++i)
      {
         count += _buckets[i]->getImmutableMap().size();
      }

      count += _counter.load(std::memory_order_relaxed);
      _latch.release_r();
      return count * partialImpCacheMap::getCacheItemSize();
   }

   UINT32 logicalPageIdCache::getFuzzyMutablePageCount()const
   {
      return _counter.load(std::memory_order_relaxed);
   }

   void logicalPageIdCache::setAllPagesImmutable()
   {
      SDB_ASSERT(isReady(), "can not be invalid");
      UINT32 totalMutableCount = 0;
      UINT32 totalImmutableCount = 0;

      _latch.lock_w();

      for (UINT32 i = 0; i < _buckets.size(); ++i)
      {
         totalMutableCount += _buckets[i]->getMutableMap().size();
         _buckets[i]->switchMap();
         totalImmutableCount += _buckets[i]->getImmutableMap().size();
      }

      _counter.store(0, std::memory_order_relaxed);
      _latch.release_w();
      PD_LOG(PDDEBUG, "mutable page count:%d turned, final immutable page count:%d",
             totalMutableCount, totalImmutableCount);

      return;
   }

   INT32 logicalPageIdCache::resetBaseAndClearImmutableMaps(const idMapFile *file)
   {
      INT32 rc = SDB_OK;
      BOOLEAN locked = FALSE;

      if (OSS_UNLIKELY(NULL == file ||
                       !file->isOpen()))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      else if (OSS_UNLIKELY(!isReady()))
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }

      _latch.lock_w();
      locked = TRUE;

      rc = resetBase(file);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to reset base file:%d", rc);
         goto error;
      }

      for (UINT32 i = 0; i < _buckets.size(); ++i)
      {
         _buckets[i]->clearImmutableMap();
      }
   done:
      if (locked)
      {
         _latch.release_w();
      }
      return rc;
   error:
      goto done;
   }

   INT32 logicalPageIdCache::put(PAGE_ID lpid,
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
         rc = _put(lpid, slot);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to put lpid[%d] to cache:%d", lpid, rc);
            goto error;
         }

         _counter.fetch_add(1, std::memory_order_relaxed);
      }

   done:
      return rc;
   error:
      goto done;
   }

   INT32 logicalPageIdCache::_put(PAGE_ID lpid,
                                  const idMapSlot &slot)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(INVALID_PAGE_ID != lpid, "can not be invalid");
      SDB_ASSERT(!slot.isFree(), "can not be free");

      ossSLatch *latch = NULL;
      UINT32 bucketPos = getBucket(lpid, &latch);
      ossSLatchGuard guard(latch, EXCLUSIVE);
      if (!_buckets[bucketPos]->insert(lpid, slot))
      {
         PD_LOG(PDERROR, "failed to insert lpid[%d] to bucket", lpid);
         rc = SDB_VESSEL_DUPLICATED_KEY;
         goto error;
      }
      
   done:
      return rc;
   error:
      goto done;
   }

   void logicalPageIdCache::remove(PAGE_ID lpid)
   {
      SDB_ASSERT(isReady(), "can not be invalid");
      SDB_ASSERT(INVALID_PAGE_ID != lpid, "can not be invalid");
      _latch.lock_r();
      _remove(lpid);
      _counter.fetch_add(1, std::memory_order_relaxed);
      _latch.release_r();
      return;
   }

   void logicalPageIdCache::_remove(PAGE_ID lpid)
   {
      SDB_ASSERT(INVALID_PAGE_ID != lpid, "can not be invalid");
      ossSLatch *latch = NULL;
      UINT32 bucketPos = getBucket(lpid, &latch);
      ossSLatchGuard guard(latch, EXCLUSIVE);
      _buckets[bucketPos]->remove(lpid);
      return;
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

   INT32 logicalPageIdCache::getFromBase(PAGE_ID lpid, idMapSlot &slot)const
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

      ossSLatch *latch = NULL;
      UINT32 bucketPos = getBucket(lpid, &latch);
      slot = idMapSlot();
      isMutable = FALSE;

      ossSLatchGuard guard(latch, SHARED);
      if (_buckets[bucketPos]->find(lpid, slot, isMutable))
      {
         if (slot.isFree())
         {
            rc = SDB_VESSEL_LOGICAL_PAGE_UNMAPPED;
            goto error;
         }
         goto done;
      }

      guard.unlock();

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


   INT32 logicalPageIdCache::upsertWhenRestore(PAGE_ID lpid, const idMapSlot &slot)
   {
      INT32 rc = SDB_OK;
      UINT32 bucketPos = 0;

      if (OSS_UNLIKELY(INVALID_PAGE_ID == lpid))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      else if (OSS_UNLIKELY(!isReady()))
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }

      bucketPos = getBucket(lpid, NULL);
      _buckets[bucketPos]->restoreToImmutableMap(lpid, slot);

   done:
      return rc;
   error:
      goto done;
   }

   const partialImpCacheMap::CACHE_MAP logicalPageIdCache::getImmutableMap(UINT32 pos)const
   {
      SDB_ASSERT(isReady(), "can not be invalid");
      SDB_ASSERT(pos < _buckets.size(), "out of bound");
      return _buckets[pos]->getImmutableMap();
   }

   const partialImpCacheMap::CACHE_MAP logicalPageIdCache::getMutableMap(UINT32 pos)const
   {
      SDB_ASSERT(isReady(), "can not be invalid");
      SDB_ASSERT(pos < _buckets.size(), "out of bound");
      return _buckets[pos]->getMutableMap();
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
                base->getFullPath(), rc);
         goto error;
      }

      _basePageCount = head.totalPageCount;
      _base = base;
   done:
      return rc;
   error:
      goto done;
   }
}//namespace vessel
}//namespace engine
