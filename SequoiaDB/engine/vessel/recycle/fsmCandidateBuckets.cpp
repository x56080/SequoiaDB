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

   Source File Name = fsmCandidateBuckets.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#include "vessel/fsmCandidateBuckets.h"

namespace engine
{
namespace vessel
{
   fsmCandidateBuckets::fsmCandidateBuckets():
   _bucketCount(0),
   _bucketCapacity(0),
   _buckets(NULL),
   _latchCount(0),
   _latches(NULL)
   {

   }

   fsmCandidateBuckets::~fsmCandidateBuckets()
   {
      fini();
   }

   void fsmCandidateBuckets::fini()
   {
      _bucketCount = 0;
      _bucketCapacity = 0;
      _latchCount = 0;
      if (NULL != _buckets)
      {
         SDB_OSS_DEL []_buckets;
         _buckets = NULL;
      }
      if (NULL != _latches)
      {
         SDB_OSS_DEL []_latches;
         _latches = NULL;
      }
      return;
   }

   INT32 fsmCandidateBuckets::init(UINT16 bucketCount,
                                   UINT16 bucketCapacity,
                                   UINT32 latchCount)
   {
      INT32 rc = SDB_OK;
      fini();

      if (!ossIsPowerOf2(bucketCount) ||
          !ossIsPowerOf2(bucketCapacity) ||
          !ossIsPowerOf2(latchCount))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      _buckets = SDB_OSS_NEW fsmCandidateBucket[bucketCount];
      if (NULL == _buckets)
      {
         PD_LOG(PDERROR, "failed to allocate mem");
         rc = SDB_OOM;
         goto error;
      }
      
      for (UINT16 i = 0; i < bucketCount; ++i)
      {
         rc = _buckets[i].init(bucketCapacity);
         if (SDB_OK != rc)
         {
            goto error;
         }
      }

      _latches = SDB_OSS_NEW ossSpinXLatch[latchCount];
      if (NULL == _latches)
      {
         PD_LOG(PDERROR, "failed to allocate mem");
         rc = SDB_OOM;
         goto error;
      }

      _bucketCount = bucketCount;
      _bucketCapacity = bucketCapacity;
      _latchCount = latchCount;
   done:
      return rc;
   error:
      fini();
      goto done;
   }

   BOOLEAN fsmCandidateBuckets::upsert(UINT32 bucketNo,
                                       const fsmCandidate &candidate,
                                       fsmCandidate *replaced)
   {
      SDB_ASSERT(bucketNo < _bucketCount, "out of bound");
      SDB_ASSERT(INVALID_CL_PAGE_SEQ != candidate.seq, "can not be invalid");
      BOOLEAN r = FALSE;
      if (OSS_UNLIKELY(_bucketCount <= bucketNo ||
                       !candidate.isValid()))
      {
         goto done;
      }

      {
      ossScopedLock guard(getLatch(bucketNo));
      r = _buckets[bucketNo].upsert(_bucketCapacity, candidate, replaced);
      }
   done:
      return r;
   }

   BOOLEAN fsmCandidateBuckets::findAndAutoRemoving(UINT32 bucketNo,
                                                    UINT16 size,
                                                    UINT16 minFreeSize,
                                                    fsmCandidate &candidate)
   {
      BOOLEAN r = FALSE;
      SDB_ASSERT(bucketNo < _bucketCount, "out of bound");
      SDB_ASSERT(0 < size, "can not be zero");
      if (OSS_UNLIKELY(_bucketCount <= bucketNo ||
                       0 == size))
      {
         goto done;
      }

      {
      ossScopedLock guard(getLatch(bucketNo));
      if (_buckets[bucketNo].findAndAutoRemoving(_bucketCapacity, size,
                                                 minFreeSize, candidate))
      {
         candidate.setBucketNo(bucketNo);
         r = TRUE;
      }
      }
   done:
      return r;
   }

   BOOLEAN fsmCandidateBuckets::tryToIncBucket(UINT32 seq,
                                                PAGE_ID lpid,
                                                UINT32 bucketBegin,
                                                UINT16 maxFreeSize,
                                                UINT16 newFreeSize,
                                                UINT16 delta)
   {
      BOOLEAN r = FALSE;
      SDB_ASSERT(0 != _bucketCount, "must be inited");
      for (UINT32 i = 0; i < _bucketCount; ++i)
      {
         UINT32 bucketNo = (bucketBegin + i) & (_bucketCount - 1);
         ossScopedLock guard(getLatch(bucketNo));
         if (_buckets[bucketNo].tryToInc(_bucketCapacity, seq, lpid, maxFreeSize, newFreeSize, delta))
         {
            r = TRUE;
            goto done;
         }
      }
   done:
      return r;
   }

   BOOLEAN fsmCandidateBuckets::tryToDecBucket(UINT32 seq,
                                                PAGE_ID lpid,
                                                UINT32 bucketBegin,
                                                UINT16 minFreeSize,
                                                UINT16 newFreeSize,
                                                UINT16 delta)
   {
      BOOLEAN r = FALSE;
      SDB_ASSERT(0 != _bucketCount, "must be inited");
      for (UINT32 i = 0; i < _bucketCount; ++i)
      {
         UINT32 bucketNo = (bucketBegin + i) & (_bucketCount - 1);
         ossScopedLock guard(getLatch(bucketNo));
         if (_buckets[bucketNo].tryToDec(_bucketCapacity, seq, lpid, minFreeSize, newFreeSize, delta))
         {
            r = TRUE;
            goto done;
         }
      }
   done:
      return r;
   }

   BOOLEAN fsmCandidateBuckets::updateCandidate(UINT32 bucketNo,
                                                UINT32 seq,
                                                PAGE_ID lpid,
                                                UINT16 minFreeSize,
                                                UINT16 freeSizeFromBucket,
                                                UINT16 currentFreeSize,
                                                BOOLEAN failure)
   {
      BOOLEAN r = FALSE;
      if (_bucketCount <= bucketNo)
      {
         goto done;
      }
      {
      ossScopedLock guard(getLatch(bucketNo));
      r = _buckets[bucketNo].updateCandidate(_bucketCapacity, seq, lpid,
                                             minFreeSize, freeSizeFromBucket,
                                             currentFreeSize, failure);
      }
   done:
      return r;
   }

   UINT32 fsmCandidateBuckets::getFreeSize(UINT32 bucketNo)
   {
      SDB_ASSERT(bucketNo < _bucketCount, "out of bound");
      if (OSS_UNLIKELY(_bucketCount <= bucketNo))
      {
         return 0;
      }
      {
      ossScopedLock guard(getLatch(bucketNo));
      return _bucketCapacity - _buckets[bucketNo].getSize(_bucketCapacity);
      }
   }

   UINT64 fsmCandidateBuckets::getReqCount(UINT32 bucketNo)
   {
      SDB_ASSERT(bucketNo < _bucketCount, "out of bound");
      if (OSS_UNLIKELY(_bucketCount <= bucketNo))
      {
         return 0;
      }
      return _buckets[bucketNo].getReqCnt();
   }

   void fsmCandidateBuckets::dumpBucket(UINT32 i,
                                        fsmCandidate *candidates,
                                        UINT32 &count)
   {
      SDB_ASSERT(i < _bucketCount, "out of bound");
      SDB_ASSERT(NULL != candidates, "can not be null");
      ossScopedLock guard(getLatch(i));
      _buckets[i].dump(_bucketCapacity, candidates, count);
   }
}//namespace vessel
}//namespace engine