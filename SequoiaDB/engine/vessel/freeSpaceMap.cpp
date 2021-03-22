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

   Source File Name = freeSpaceMap.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/
#include "vessel/freeSpaceMap.h"
#include "vessel/fsmCandidateBucket.h"
#include "vessel/fsmSizeLvl.h"

namespace engine
{
namespace vessel
{
   freeSpaceMap::freeSpaceMap():
   _minStriping(INVALID_STRIPING_ID),
   _maxStriping(INVALID_STRIPING_ID),
   _pageSize(0),
   _maxFreeSize(0),
   _minFreeSize(getMinSizeOfRecordInRdp()),
   _bucketCount(0),
   _buckets(NULL)
   {
      
   }

   freeSpaceMap::~freeSpaceMap()
   {
      fini();
   }

   void freeSpaceMap::fini()
   {
      _minStriping = INVALID_STRIPING_ID;
      _maxStriping = INVALID_STRIPING_ID;
      _pageSize = 0;
      _maxFreeSize = 0;
      _minFreeSize = getMinSizeOfRecordInRdp();
      _bucketCount = 0;
      
      if (NULL != _buckets)
      {
         SDB_OSS_DEL []_buckets;
         _buckets = NULL;
      }
      
      _newPagePool.clear();
      _diskStats.reset();
      _dfsm.close();
   }

   void freeSpaceMap::close()
   {
      ossScopedLock guard(&_latch, EXCLUSIVE);
      if (!isOpen())
      {
         goto done;
      }
      

   done:
      return;
   }

   INT32 freeSpaceMap::create(fsmFile *file,
                              CL_MB_ID mbID,
                              UINT32 logicalID,
                              UINT32 pageSize,
                              UINT32 minFreeSize,
                              BOOLEAN bucketMode,
                              STRIPING_ID min,
                              STRIPING_ID max)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(!isOpen(), "already open");
      UINT32 alignedMinSize = ossAlign4(minFreeSize);

      if (NULL == file || !file->isOpen() ||
          INVALID_CL_MB_ID == mbID ||
          DMS_INVALID_LOGICCLID == logicalID ||
          DMS_PAGE_SIZE64K != pageSize ||
          DMS_PAGE_SIZE32K != pageSize ||
          pageSize <= alignedMinSize)
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      else if (bucketMode)
      {
         if (INVALID_STRIPING_ID == min ||
             INVALID_STRIPING_ID == max ||
             max < min)
         {
            rc = SDB_INVALIDARG;
            goto error;
         }
      }

      _minStriping = min;
      _maxStriping = max;
      _pageSize = pageSize;
      _maxFreeSize = getMaxFreeSizeOfRdp(pageSize);
      if (_minFreeSize < alignedMinSize)
      {
         _minFreeSize = alignedMinSize;
      }
      _bucketCount = bucketMode ? FSM_STRIPING_BUCKET_COUNT : 1;
      _buckets = SDB_OSS_NEW fsmCandidateBucket[_bucketCount];
      if (NULL == _buckets)
      {
         PD_LOG(PDERROR, "failed to allcoate mem");
         rc = SDB_OOM;
         goto error;
      }

      rc = _dfsm.create(file, mbID, logicalID);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to open free space map on disk:%d", rc);
         goto error;
      }
   done:
      return rc;
   error:
      goto done;
   }

   INT32 freeSpaceMap::open(fsmFile *file,
                              CL_MB_ID mbID,
                              UINT32 logicalID,
                              UINT32 pageSize,
                              UINT32 minFreeSize,
                              BOOLEAN bucketMode,
                              STRIPING_ID min,
                              STRIPING_ID max)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(!isOpen(), "already open");
      UINT32 alignedMinSize = ossAlign4(minFreeSize);

      if (NULL == file || !file->isOpen() ||
          INVALID_CL_MB_ID == mbID ||
          DMS_INVALID_LOGICCLID == logicalID ||
          DMS_PAGE_SIZE64K != pageSize ||
          DMS_PAGE_SIZE32K != pageSize ||
          pageSize <= alignedMinSize)
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      else if (bucketMode)
      {
         if (INVALID_STRIPING_ID == min ||
             INVALID_STRIPING_ID == max ||
             max < min)
         {
            rc = SDB_INVALIDARG;
            goto error;
         }
      }

      _minStriping = min;
      _maxStriping = max;
      _pageSize = pageSize;
      _maxFreeSize = getMaxFreeSizeOfRdp(pageSize);
      if (_minFreeSize < alignedMinSize)
      {
         _minFreeSize = alignedMinSize;
      }
      _bucketCount = bucketMode ? FSM_STRIPING_BUCKET_COUNT : 1;
      _buckets = SDB_OSS_NEW fsmCandidateBucket[_bucketCount];
      if (NULL == _buckets)
      {
         PD_LOG(PDERROR, "failed to allcoate mem");
         rc = SDB_OOM;
         goto error;
      }

      rc = _dfsm.open(file, mbID, logicalID, _diskStats);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to open free space map on disk:%d", rc);
         goto error;
      }
   done:
      return rc;
   error:
      fini();
      goto done;
   }

   INT32 freeSpaceMap::fastFind(STRIPING_ID striping,
                                UINT32 originalRecordSize,
                                fsmCandidate &candidate)
   {
      INT32 rc = SDB_OK;
      fsmCandidateBucket *bucket = NULL;
      UINT32 bucketNo = 0;
      candidate.reset();
      UINT32 size = getMaxSizeOfRecordInRdp(originalRecordSize);

      if (!isOpen())
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      else if (0 == size)
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      else if (1 < _bucketCount && (INVALID_STRIPING_ID == striping))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      else if (_maxFreeSize < size)
      {
         rc = SDB_VESSEL_OUT_OF_RESOURCE;
         goto error;
      }      

      bucket = getBucket(striping, &bucketNo);
      SDB_ASSERT(NULL != bucket, "can not be null");

      if (!findFromBucket(bucketNo, bucket, size, candidate))
      {
         rc = SDB_VESSEL_FSM_NO_FREE_SPACE;
         goto error;
      }
      
   done:
      return rc;
   error:
      goto done;
   }

   INT32 freeSpaceMap::addNewPagesAndFind(CL_PAGE_SEQ firstSeq,
                                          PAGE_ID firstLpid,
                                          UINT32 count,
                                          STRIPING_ID striping,
                                          UINT32 originalRecordSize,
                                          fsmCandidate &candidate)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(8 == count, "must be eight");
      candidate.reset();
      UINT32 size = getMaxSizeOfRecordInRdp(originalRecordSize);
      ossScopedLock guard(&_latch, EXCLUSIVE);
      if (OSS_UNLIKELY(INVALID_CL_PAGE_SEQ == firstSeq ||
                       INVALID_PAGE_ID == firstLpid))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
   done:
      return rc;
   error:
      goto done;
   }

   INT32 freeSpaceMap::findInWholeMap(STRIPING_ID striping,
                                      UINT32 originalRecordSize,
                                      fsmCandidate &candidate)
   {
      INT32 rc = SDB_OK;
      fsmCandidateBucket *bucket = NULL;
      UINT32 bucketNo = 0;
      UINT32 size = getMaxSizeOfRecordInRdp(originalRecordSize);
      candidate.reset();
      ossScopedLock guard(&_latch, EXCLUSIVE);

      if (!isOpen())
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      else if (0 == size)
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      else if (1 < _bucketCount && (INVALID_STRIPING_ID == striping))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      else if (_maxFreeSize < size)
      {
         rc = SDB_VESSEL_OUT_OF_RESOURCE;
         goto error;
      }      

      bucket = getBucket(striping, &bucketNo);
      SDB_ASSERT(NULL != bucket, "can not be null");

      if (findFromBucket(bucketNo, bucket, size, candidate))
      {
         goto done;
      }

      //bucket->guaranteeNotFull(&_fastLatch);

      if (findPageFromPoolAndUpdateBucket(bucketNo, bucket, size, candidate))
      {
         goto done;
      }

      rc = findPageFromDiskMapAndUpdateBucket(bucketNo, bucket, size, candidate);
      if (SDB_OK != rc)
      {
         goto error;
      }
   done:
      return rc;
   error:
      goto done;
   }

   INT32 freeSpaceMap::findPageFromDiskMapAndUpdateBucket(UINT32 bucketNo,
                                                         fsmCandidateBucket *bucket,
                                                         UINT16 size,
                                                         fsmCandidate &candidate)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(NULL != bucket, "can not be null");
      SDB_ASSERT(bucketNo < _bucketCount, "impossible");
      SDB_ASSERT(DMS_PAGE_SIZE32K == _pageSize ||
                 DMS_PAGE_SIZE64K == _pageSize, "impossible");
      BOOLEAN found = FALSE;
      CL_PAGE_SEQ seq = INVALID_CL_PAGE_SEQ;
      UINT32 minFreeSize = 0;
      fsmSizeLvl candidateLvl;
      fsmSizeLvl lvl;
      lvl.init(_pageSize, size);
      if (1024 < size)
      {
         lvl.incDelta();
      }

      if (!isWorthToScanDisk(lvl.getLvl(), _diskStats.totalPageCount,
                            _diskStats.lvl1, _diskStats.lvl2,
                            _diskStats.lvl3, _diskStats.lvl4))
      {
         rc = SDB_VESSEL_FSM_NO_FREE_SPACE;
         goto error;
      }

      rc = _dfsm.find(lvl, found, seq, candidateLvl);
      if (SDB_OK != rc)
      {
         goto error;
      }

      if (!found)
      {
         rc = SDB_VESSEL_FSM_NO_FREE_SPACE;
         goto error;
      }

      minFreeSize = lvl.getMinFreeSize(_pageSize);
      SDB_ASSERT(size <= minFreeSize, "impossible");

      candidate = fsmCandidate(seq, INVALID_PAGE_ID, minFreeSize);
      if (_minFreeSize <= (minFreeSize - size))
      {
         fsmCandidate c = candidate;
         c.free -= size;
         bucket->upsert(&_fastLatch, c, FALSE);
      }
   done:
      return rc;
   error:
      goto done;
   }

   BOOLEAN freeSpaceMap::findPageFromPoolAndUpdateBucket(UINT32 bucketNo,
                                                         fsmCandidateBucket *bucket,
                                                         UINT16 size,
                                                         fsmCandidate &candidate)
   {
      SDB_ASSERT(NULL != bucket, "can not be null");
      SDB_ASSERT(bucketNo < _bucketCount, "impossible");
      SDB_ASSERT(size <= _maxFreeSize, "impossible");
      BOOLEAN r = FALSE;
      UINT32 maxCnt = 0;
      _pageSAndL page;

      if (_newPagePool.empty())
      {
         goto done;
      }

      maxCnt = estimateMaxUpdatingCount(bucket);
      SDB_ASSERT(0 < maxCnt, "can not be zero");

      page = _newPagePool.front();
      _newPagePool.pop_front();

      candidate.seq = page.seq;
      candidate.lpid = page.lpid;
      candidate.free = _maxFreeSize;

      if (_minFreeSize <= (_maxFreeSize - size))
      {
         fsmCandidate c(candidate);
         c.free -= size;
         bucket->upsert(&_fastLatch, c);
      }

      while (--maxCnt > 0 && !_newPagePool.empty())
      {
         page = _newPagePool.front();
         _newPagePool.pop_front();
         fsmCandidate c(page.seq, page.lpid, _maxFreeSize);
         bucket->upsert(&_fastLatch, c);
      }
      r = TRUE;
      
   done:
      return r;
   }

   BOOLEAN freeSpaceMap::findFromBucket(UINT32 bucketNo,
                                        fsmCandidateBucket *bucket,
                                        UINT32 size,
                                        fsmCandidate &candidate)
   {
      SDB_ASSERT(NULL != bucket, "can not be null");
      SDB_ASSERT(bucketNo < _bucketCount, "impossible");
      return bucket->findAndAutoRemoving(&_fastLatch,
                                         size,
                                         _minFreeSize,
                                         candidate);
   }

  

   UINT32 freeSpaceMap::estimateMaxUpdatingCount(fsmCandidateBucket *bucket)
   {
      SDB_ASSERT(1 == _bucketCount || 32 == _bucketCount, "impossible");
      SDB_ASSERT(NULL != bucket, "can not be null");
      UINT32 maxPageCount = bucket->getCapacity() - bucket->getSize(&_fastLatch);
      
      if (1 < _bucketCount && bucket->getReqCnt() <= 64)
      {
         maxPageCount = 1;
      }

      if (0 == maxPageCount)
      {
         maxPageCount = 1;
      }
      
   done:
      return maxPageCount;
   }

   fsmCandidateBucket *freeSpaceMap::getBucket(STRIPING_ID striping,
                                               UINT32 *bucketNo)
   {
      SDB_ASSERT(1 == _bucketCount || 32 == _bucketCount, "must be 1 or 32");
      UINT32 i = 0;
      if (1 == _bucketCount)
      {
         return _buckets;
      }
      else if (striping <= _minStriping)
      {
         return _buckets;
      }
      else if (_maxStriping <= striping)
      {
         return &(_buckets[_bucketCount -1]);
      }
      UINT16 range = (_maxStriping - _minStriping + 1) >> 5; /// divided by 32
      if (0 == range)
      {
         range = 1;
      }

      i = (striping - _minStriping)/ range;
      i &= 0x1f;
      if (NULL != bucketNo)
      {
         *bucketNo = i;
      }
      return &(_buckets[i]);
   }

   
}//namespace vessel
}//namespace engine