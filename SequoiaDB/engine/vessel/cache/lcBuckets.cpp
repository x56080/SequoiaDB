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

   Source File Name = lcBuckets.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#include "vessel/lcBuckets.h"
#include "ossErr.h"
#include "ossMem.hpp"
#include "ossLatch.hpp"
#include "pdTrace.hpp"

namespace engine
{
namespace vessel
{
   lcBuckets::lcBuckets()
   :_minRecycleCount(0),
   _bucketCount(0), _buckets(NULL),
    _latchCount(0), _latches(NULL)
   {

   }

   lcBuckets::~lcBuckets()
   {
      fini();
   }

   INT32 lcBuckets::init(UINT32 bucketCount,
                          UINT32 latchCount,
                          UINT32 minRecycleCount)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(NULL == _buckets, "do not reinit");

      if (OSS_UNLIKELY(0 == bucketCount || 0 == latchCount))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      else if (OSS_UNLIKELY(!ossIsPowerOf2(bucketCount) ||
                            !ossIsPowerOf2(latchCount)))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      _buckets = SDB_OSS_NEW lcBucket[bucketCount];
      if (NULL == _buckets)
      {
         PD_LOG(PDERROR, "failed to allocate mem");
         rc = SDB_OOM;
         goto error;
      }

      _latches = SDB_OSS_NEW _ossSpinXLatch[latchCount];
      if (NULL == _latches)
      {
         PD_LOG(PDERROR, "failed to allocate mem");
         rc = SDB_OOM;
         goto error;
      }

      _bucketCount = bucketCount;
      _latchCount = latchCount;
      _minRecycleCount = minRecycleCount;
   done:
      return rc;
   error:
      fini();
      goto done;
   }

   INT32 lcBuckets::fini()
   {
      if (NULL != _buckets)
      {
         SDB_OSS_DEL []_buckets;
         _buckets = NULL;
         _bucketCount = 0;
      }

      if (NULL != _latches)
      {
         SDB_OSS_DEL []_latches;
         _latches = NULL;
         _latchCount = 0;
      }

      _minRecycleCount = 0;

      return SDB_OK;
   }


   INT32 lcBuckets::ensureTagAndIncUsage(const GLOBAL_PAGE_ID &id,
                                         const mmapPagePointer &ptr,
                                         lcPageTagHolder &holder,
                                         BOOLEAN &isNewTag)
   {
      ossSpinXLatch *latch = NULL;
      lcBucket *bucket = NULL;
      getBucketAndLatch(id, latch, bucket);
      ossScopedLock guard(latch);
      return bucket->ensureTagAndIncUsage(id, _minRecycleCount,
                                          ptr, holder, isNewTag);
   }

   BOOLEAN lcBuckets::getTagAndIncUsage(const GLOBAL_PAGE_ID &id,
                                        lcPageTagHolder &holder)
   {
      ossSpinXLatch *latch = NULL;
      lcBucket *bucket = NULL;
      getBucketAndLatch(id, latch, bucket);
      ossScopedLock guard(latch);
      return bucket->getTagAndIncUsage(id, holder);
   }

   INT32 lcBuckets::releaseRemovedTag(liteCachePageTag *tag)
   {
      INT32 rc = SDB_OK;
      ossSpinXLatch *latch = NULL;
      lcBucket *bucket = NULL;
      if (NULL == tag || !tag->id().isValid())
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      {
      getBucketAndLatch(tag->id(), latch, bucket);
      rc = bucket->releaseRemovedTag(tag);
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

   void lcBuckets::getBucketAndLatch(const GLOBAL_PAGE_ID &id,
                                    _ossSpinXLatch *&mutex,
                                    lcBucket *&bucket)
   {
      SDB_ASSERT(NULL != _buckets && NULL != _latches, "can not be null");
      UINT32 hash = id.hash();
      UINT32 bucketNO = hash & (_bucketCount - 1);
      lcBucket &b = _buckets[bucketNO];
      UINT32 latchNO = hash & (_latchCount - 1);
      _ossSpinXLatch &m = _latches[latchNO];
      mutex = &m;
      bucket = &b;
      return;
   }
} /// end of namespace vessel
} /// end of namespace engine