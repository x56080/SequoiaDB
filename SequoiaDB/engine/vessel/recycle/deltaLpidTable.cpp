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

   Source File Name = deltaLpidTable.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#include "vessel/deltaLpidTable.h"
#include "pdTrace.hpp"
#include "ossLikely.hpp"

namespace engine
{
namespace vessel
{
   deltaLpidTable::~deltaLpidTable()
   {
      fini();
   }

   INT32 deltaLpidTable::init(UINT32 bucketCount, UINT32 latchCount)
   {
      INT32 rc = SDB_OK;
      fini();
      if (0 == bucketCount || !ossIsPowerOf2(bucketCount))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      if (0 == latchCount || !ossIsPowerOf2(latchCount))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      _latches = SDB_OSS_NEW ossSpinSLatch[latchCount];
      if (NULL == _latches)
      {
         PD_LOG(PDERROR, "failed to allocate mem");
         rc = SDB_OOM;
         goto error;
      }

      _mutableBuckets = SDB_OSS_NEW _cacheBucket[bucketCount];
      if (NULL == _mutableBuckets)
      {
         PD_LOG(PDERROR, "failed to allocate mem");
         rc = SDB_OOM;
         goto error;
      }

      _immutableBuckets = SDB_OSS_NEW _cacheBucket[bucketCount];
      if (NULL == _immutableBuckets)
      {
         PD_LOG(PDERROR, "failed to allocate mem");
         rc = SDB_OOM;
         goto error;
      }

      _latchCount = latchCount;
      _bucketCount = bucketCount;
   done:
      return rc;
   error:
      fini();
      goto done;
   }

   void deltaLpidTable::fini()
   {
      if (NULL != _mutableBuckets)
      {
         SDB_OSS_DEL []_mutableBuckets;
         _mutableBuckets = NULL;
      }
      if (NULL != _immutableBuckets)
      {
         SDB_OSS_DEL []_immutableBuckets;
         _immutableBuckets = NULL;
      }
      if (NULL != _latches)
      {
         SDB_OSS_DEL []_latches;
         _latches = NULL;
      }
      _latchCount = 0;
      _bucketCount = 0;
      return;
   }

   void deltaLpidTable::clearInmmutableBuckets()
   {
      SDB_ASSERT(isInitialized(), "must be inited");
      if (NULL != _immutableBuckets)
      {
         for (UINT32 i = 0; i < _bucketCount; ++i)
         {
            _immutableBuckets[i].cache.clear();
         }
      }
      
      return;
   }

   INT32 deltaLpidTable::mergeMutablePagesIntoImmutable()
   {
      INT32 rc = SDB_OK;
      if (OSS_UNLIKELY(!isInitialized()))
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }

      for (UINT32 i = 0; i < _bucketCount; ++i)
      {
         _immutableBuckets[i].merge(_mutableBuckets[i]);
         _mutableBuckets[i].cache.clear();
      }
   done:
      return rc;
   error:
      goto done;
   }

   INT32 deltaLpidTable::find(PAGE_ID lpid,
                              PAGE_ID &pid,
                              BOOLEAN &found,
                              BOOLEAN *isMutable)
   {
      INT32 rc = SDB_OK;
      pid = INVALID_PAGE_ID;
      found = FALSE;
      ossSpinSLatch *latch = NULL;

      if (OSS_UNLIKELY(!isInitialized()))
      {
         SDB_ASSERT(FALSE, "impossible");
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }
      else if (OSS_UNLIKELY(INVALID_PAGE_ID == lpid))
      {
         SDB_ASSERT(FALSE, "impossible");
         rc = SDB_INVALIDARG;
         goto error;
      }

      latch = getLatch(lpid);
      {
      ossScopedLock guard(latch, SHARED);
      if (findInMutableBuckets(lpid, pid))
      {
         found = TRUE;
         if (NULL != isMutable)
         {
            *isMutable = TRUE;
            goto done;
         }
      }
      if (findInImmutableBuckets(lpid, pid))
      {
         found = TRUE;
         if (NULL != isMutable)
         {
            *isMutable = FALSE;
         }
         goto done;
      }
      }
   done:
      return rc;
   error:
      goto done;
   }

   BOOLEAN deltaLpidTable::findInMutableBuckets(PAGE_ID lpid, PAGE_ID &pid)const
   {
      BOOLEAN r = FALSE;
      SDB_ASSERT(isInitialized(), "must be inited");
      SDB_ASSERT(INVALID_PAGE_ID != lpid, "can not be invalid");
      const _cacheBucket *bucket = getMutableBucket(lpid);
      _cacheBucket::PAGE_CACHE::const_iterator itr = bucket->cache.find(lpid);
      if (bucket->cache.end() != itr)
      {
         pid = itr->second;
         r = TRUE;
      }
      
   done:
      return r;
   }

   BOOLEAN deltaLpidTable::findInImmutableBuckets(PAGE_ID lpid, PAGE_ID &pid)const
   {
      BOOLEAN r = FALSE;
      SDB_ASSERT(isInitialized(), "must be inited");
      SDB_ASSERT(INVALID_PAGE_ID != lpid, "can not be invalid");
      const _cacheBucket *bucket = getImmutableBucket(lpid);
      _cacheBucket::PAGE_CACHE::const_iterator itr = bucket->cache.find(lpid);
      if (bucket->cache.end() != itr)
      {
         pid = itr->second;
         r = TRUE;
      }
      
   done:
      return r;
   }

   INT32 deltaLpidTable::upsert(PAGE_ID lpid, PAGE_ID pid)
   {
      INT32 rc = SDB_OK;
      _cacheBucket *mutableBucket = NULL;
      _cacheBucket *immutableBucket = NULL;
      ossSpinSLatch *latch = NULL;

      if (OSS_UNLIKELY(!isInitialized()))
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }
      else if (OSS_UNLIKELY(INVALID_PAGE_ID == lpid ||
                            INVALID_PAGE_ID == pid))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      latch = getLatch(lpid);
      mutableBucket = getMutableBucket(lpid);
      immutableBucket = getImmutableBucket(lpid);
      {
      ossScopedLock guard(latch, EXCLUSIVE);
      mutableBucket->cache[lpid] = pid;
      immutableBucket->cache.erase(lpid);
      }
      
   done:
      return rc;
   error:
      goto done;
   }

//////////////////_cacheBucket

   void deltaLpidTable::_cacheBucket::merge(const _cacheBucket &o)
   {
      PAGE_CACHE::const_iterator itr = o.cache.begin();
      for (; itr != o.cache.end(); ++itr)
      {
         this->cache[itr->first] = itr->second;
      }
      return;
   }
}//namespace vessel
}//namespace engine