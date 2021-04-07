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

   When/how to use: this program may be used on binary and text-formatted
   versions of PMD component. This file contains functions for agent processing.

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
      BOOLEAN rollback = FALSE;
      if (OSS_UNLIKELY(0 == bucketCount || 0 == latchCount))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      else if (OSS_UNLIKELY(NULL != _buckets))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      rollback = TRUE;
      _buckets = SDB_OSS_NEW lcBucket[bucketCount];
      if (NULL == _buckets)
      {
         PD_LOG(PDERROR, "failed to allocate mem");
         rc = SDB_OOM;
         goto error;
      }

      _latches = SDB_OSS_NEW _ossSpinSLatch[latchCount];
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
      if (rollback)
      {
         fini();
      }
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
                                         UINT32 pageSize,
                                         lcPageTagHolder &holder,
                                         BOOLEAN &newTagInBucket)
   {
      ossSpinSLatch *latch = NULL;
      lcBucket *bucket = NULL;
      getBucketAndLatch(id, latch, bucket);
      return bucket->ensureTagAndIncUsage(id, latch,
                                          pageSize, _minRecycleCount,
                                          holder, newTagInBucket);
   }

   INT32 lcBuckets::getTagAndIncUsage(const GLOBAL_PAGE_ID &id,
                                      lcPageTagHolder &holder)
   {
      ossSpinSLatch *latch = NULL;
      lcBucket *bucket = NULL;
      getBucketAndLatch(id, latch, bucket);
      return bucket->getTagAndIncUsage(id, latch, holder);
   }

   INT32 lcBuckets::releaseRemovedTag(liteCachePageTag *tag)
   {
      INT32 rc = SDB_OK;
      ossSpinSLatch *latch = NULL;
      lcBucket *bucket = NULL;
      if (NULL == tag || tag->id().invalid())
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      getBucketAndLatch(tag->id(), latch, bucket);
      rc = bucket->releaseRemovedTag(latch, tag);
      if (SDB_OK != rc)
      {
         goto error;
      }
   done:
      return rc;
   error:
      goto done;
   }

   INT32 lcBuckets::releaseRemovedTags(UINT32 num, liteCachePageTag *tags[])
   {
      INT32 rc = SDB_OK;
      if (OSS_UNLIKELY(0 == num))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      /// TODO: batch remove tags in same bucket
      for (UINT32 i = 0; i < num; ++i)
      {
         releaseRemovedTag(tags[i]);
      }
   done:
      return rc;
   error:
      goto done;
   }   

   void lcBuckets::getBucketAndLatch(const PHY_EXTENT_ID &id,
                                    _ossSpinSLatch *&mutex,
                                    lcBucket *&bucket)
   {
      UINT32 hash = id.hash();
      UINT32 bucketNO = hash % _bucketCount;
      lcBucket &b = _buckets[bucketNO];
      UINT32 latchNO = hash % _latchCount;
      _ossSpinSLatch &m = _latches[latchNO];
      mutex = &m;
      bucket = &b;
      return;
   }
} /// end of namespace vessel
} /// end of namespace engine