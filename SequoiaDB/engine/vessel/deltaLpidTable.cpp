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

      _buckets0 = SDB_OSS_NEW _cacheBucket[bucketCount];
      if (NULL == _buckets0)
      {
         PD_LOG(PDERROR, "failed to allcoate mem");
         rc = SDB_OOM;
         goto error;
      }
      _buckets1 = SDB_OSS_NEW _cacheBucket[bucketCount];
      if (NULL == _buckets1)
      {
         PD_LOG(PDERROR, "failed to allcoate mem");
         rc = SDB_OOM;
         goto error;
      }
      _latches = SDB_OSS_NEW ossSpinSLatch[latchCount];
      if (NULL == _latches)
      {
         PD_LOG(PDERROR, "failed to allocate mem");
         rc = SDB_OOM;
         goto error;
      }
      _bucketCount = bucketCount;
      _mutable = 0;
   done:
      return rc;
   error:
      fini();
      goto done;
   }

   void deltaLpidTable::fini()
   {
      if (NULL != _buckets0)
      {
         SDB_OSS_DEL []_buckets0;
         _buckets0 = NULL;
      }
      if (NULL != _buckets1)
      {
         SDB_OSS_DEL []_buckets1;
         _buckets1 = NULL;
      }
      if (NULL != _latches)
      {
         SDB_OSS_DEL []_latches;
         _latches = NULL;
      }
      _latchCount = 0;
      _bucketCount = 0;
      _mutable = 0;
      return;
   }

   void deltaLpidTable::clearInmmutablePages()
   {
      if (OSS_LIKELY(isInitialized()))
      {
         _cacheBucket *buckets = getImmutableBuckets();
         for (UINT32 i = 0; i < _bucketCount; ++i)
         {
            buckets[i].cache.clear();
         }
      }
      
      return;
   }

   void deltaLpidTable::changeMutablePagesIntoImmutable()
   {
      if (OSS_LIKELY(isInitialized()))
      {
         clearInmmutablePages();
         changeMutableBuckets();
      }
      
      return;
   }

   BOOLEAN deltaLpidTable::find(PAGE_ID lpid, idMapSlot &slot)
   {
      BOOLEAN r = FALSE;
      slot.reset();
      const _cacheBucket *bucket = NULL;
      ossSpinSLatch *latch = NULL;
      _cacheBucket::PAGE_CACHE::const_iterator itr;
      UINT32 hash = 0;
      if (OSS_UNLIKELY(!isInitialized()))
      {
         goto done;
      }

      latch = getLatch(lpid);
      hash = getBucketHash(lpid);
      bucket = &(getMutableBuckets()[hash]);
      {
      ossScopedLock guard(latch, SHARED);
      itr = bucket->cache.find(lpid);
      if (bucket->cache.end() != itr)
      {
         slot = itr->second;
         r = TRUE;
         goto done;
      }
      }

      bucket = &(getImmutableBuckets()[hash]);
      itr = bucket->cache.find(lpid);
      if (bucket->cache.end() != itr)
      {
         r = TRUE;
         slot = itr->second;
      }
   done:
      return r;
   }

   BOOLEAN deltaLpidTable::insert(PAGE_ID lpid, const idMapSlot &slot)
   {
      BOOLEAN r = FALSE;
      _cacheBucket *bucket = NULL;
      ossSpinSLatch *latch = NULL;
      UINT32 hash = 0;
      if (OSS_UNLIKELY(!isInitialized() ||
                       INVALID_PAGE_ID == lpid ||
                       slot.free()))
      {
         goto done;
      }

      hash = getBucketHash(lpid);
      latch = getLatch(lpid);
      bucket = &(getMutableBuckets()[hash]);
      {
      ossScopedLock guard(latch, EXCLUSIVE);
      r = bucket->cache.insert(std::make_pair(lpid, slot)).second;
      }
      
   done:
      return r;
   }
}//namespace vessel
}//namespace engine