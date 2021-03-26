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
#include "vessel/fsmSizeLvl.h"

namespace engine
{
namespace vessel
{
   const UINT32 freeSpaceMap::FIND_BUCKET = 0x01;
   const UINT32 freeSpaceMap::FIND_NEW_POOL = 0x02;
   const UINT32 freeSpaceMap::FIND_DISK_MAP = 0x04;
   const UINT32 freeSpaceMap::FIND_ALL = 0x07;

   freeSpaceMap::freeSpaceMap():
   _minStriping(INVALID_STRIPING_ID),
   _maxStriping(INVALID_STRIPING_ID),
   _pageSize(0),
   _maxFreeSize(0),
   _minFreeSize(getMinSizeOfRecordInRdp()),
   _bucketCount(0),
   _bucketCapacity(0)
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
      _bucketCapacity = 0;
      
      _buckets.fini();
      _newPagePool.clear();
      _dfsm.close();
   }

   void freeSpaceMap::close()
   {
      ossScopedLock guard(&_latch, EXCLUSIVE);
      if (!isOpen())
      {
         goto done;
      }
      
      savePagesInPool();
      savePagesInBuckets();
      fini();
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
      UINT32 latchCount = 0;

      if (NULL == file || !file->isOpen() ||
          INVALID_CL_MB_ID == mbID ||
          DMS_INVALID_LOGICCLID == logicalID ||
          (DMS_PAGE_SIZE64K != pageSize && 
           DMS_PAGE_SIZE32K != pageSize) ||
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

      if (bucketMode)
      {
         _bucketCount = FSM_STRIPING_BUCKET_COUNT;
         _bucketCapacity = 2;
         latchCount = 4;
      }
      else
      {
         _bucketCount = 1;
         _bucketCapacity = 8;
         latchCount = 1;
      }

      rc = _buckets.init(_bucketCount, _bucketCapacity, latchCount);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to init fsm bucket:%d", rc);
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
      UINT32 latchCount = 0;

      if (NULL == file || !file->isOpen() ||
          INVALID_CL_MB_ID == mbID ||
          DMS_INVALID_LOGICCLID == logicalID ||
          (DMS_PAGE_SIZE64K != pageSize && 
           DMS_PAGE_SIZE32K != pageSize) ||
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

      if (bucketMode)
      {
         _bucketCount = FSM_STRIPING_BUCKET_COUNT;
         _bucketCapacity = 2;
         latchCount = 4;
      }
      else
      {
         _bucketCount = 1;
         _bucketCapacity = 8;
         latchCount = 1;
      }

      rc = _buckets.init(_bucketCount, _bucketCapacity, latchCount);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to init fsm bucket:%d", rc);
         goto error;
      }

      rc = _dfsm.open(file, mbID, logicalID);
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

      bucketNo = getBucketNo(striping);

      if (!findFromBucket(bucketNo, size, candidate))
      {
         rc = SDB_VESSEL_FSM_NO_FREE_SPACE;
         goto error;
      }
      
   done:
      return rc;
   error:
      goto done;
   }

   INT32 freeSpaceMap::addNewPages(CL_PAGE_SEQ firstSeq,
                                   const PAGE_ID *lpids,
                                   UINT32 count)
   {
      INT32 rc = SDB_OK;
      ossScopedLock guard(&_latch, EXCLUSIVE);
      if (OSS_UNLIKELY(INVALID_CL_PAGE_SEQ == firstSeq ||
                       PAGE_COUNT_IN_EXTENT != count ||
                       NULL == lpids))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      else if (OSS_UNLIKELY(!isOpen()))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      addNewPagesToPool(count, firstSeq, lpids);

      rc = _dfsm.addNewPages(firstSeq, count);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to add new pages to disk map:%d", rc);
         goto error;
      }
   done:
      return rc;
   error:
      goto done;
   }

   INT32 freeSpaceMap::findInWholeMap(STRIPING_ID striping,
                                      UINT32 originalRecordSize,
                                      UINT32 flags,
                                      fsmCandidate &candidate)
   {
      INT32 rc = SDB_OK;
      UINT32 bucketNo = 0;
      UINT32 size = getMaxSizeOfRecordInRdp(originalRecordSize);
      candidate.reset();
      UINT32 findingFlags = 0 == flags ? freeSpaceMap::FIND_ALL : flags;

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

      bucketNo = getBucketNo(striping);

      {
      ossScopedLock guard(&_latch, EXCLUSIVE);
      if (OSS_BIT_TEST(findingFlags, freeSpaceMap::FIND_BUCKET) &&
          findFromBucket(bucketNo, size, candidate))
      {
         goto done;
      }

      if (OSS_BIT_TEST(findingFlags, freeSpaceMap::FIND_NEW_POOL) &&
          findPageFromPoolAndUpdateBucket(bucketNo, size, candidate))
      {
         goto done;
      }

      if (OSS_BIT_TEST(findingFlags, freeSpaceMap::FIND_DISK_MAP))
      {
         rc = findPageFromDiskMapAndUpdateBucket(bucketNo, size, candidate);
         if (SDB_OK != rc)
         {
            goto error;
         }
      }
      }
   done:
      return rc;
   error:
      goto done;
   }

   INT32 freeSpaceMap::updateBucket(CL_PAGE_SEQ seq,
                                    PAGE_ID lpid,
                                    UINT32 bucketNo,
                                    UINT16 freeSizeFromBucket,
                                    UINT16 currentFreeSize,
                                    BOOLEAN failure)
   {
      INT32 rc = SDB_OK;
      if (OSS_UNLIKELY(INVALID_CL_PAGE_SEQ == seq ||
                       INVALID_PAGE_ID == lpid ||
                       _maxFreeSize < freeSizeFromBucket ||
                       _maxFreeSize < currentFreeSize ||
                       _bucketCount <= bucketNo))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      _buckets.updateCandidate(bucketNo, seq, lpid,
                              _minFreeSize, freeSizeFromBucket,
                              currentFreeSize, failure);
   done:
      return rc;
   error:
      goto done;
   }

   INT32 freeSpaceMap::findPageFromDiskMapAndUpdateBucket(UINT32 bucketNo,
                                                          UINT16 size,
                                                          fsmCandidate &candidate)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(bucketNo < _bucketCount, "impossible");
      SDB_ASSERT(DMS_PAGE_SIZE32K == _pageSize ||
                 DMS_PAGE_SIZE64K == _pageSize, "impossible");
      CL_PAGE_SEQ seq = INVALID_CL_PAGE_SEQ;
      UINT32 minFreeSize = 0;
      fsmSizeLvl candidateLvl;
      fsmSizeLvl lvl;
      lvl.init(_pageSize, size);
      if (_minFreeSize <= size)
      {
         lvl.incDelta();
      }

      rc = _dfsm.find(lvl, seq, candidateLvl);
      if (SDB_VESSEL_FSM_NO_FREE_SPACE == rc)
      {
         goto error;
      }
      else if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to find free space from disk map:%d", rc);
         goto error;
      }

      minFreeSize = candidateLvl.getMinFreeSize(_pageSize);
      SDB_ASSERT(size <= minFreeSize, "impossible");

      candidate = fsmCandidate(seq, INVALID_PAGE_ID, minFreeSize - size);
      if (_minFreeSize <= (minFreeSize - size))
      {
         fsmCandidate c(candidate);
         if (_buckets.upsert(bucketNo, candidate))
         {
            candidate.setBucketNo(bucketNo);
            candidate.setFeedback();
         }
      }
   done:
      return rc;
   error:
      goto done;
   }

   BOOLEAN freeSpaceMap::findPageFromPoolAndUpdateBucket(UINT32 bucketNo,
                                                         UINT16 size,
                                                         fsmCandidate &candidate)
   {
      SDB_ASSERT(bucketNo < _bucketCount, "impossible");
      SDB_ASSERT(size <= _maxFreeSize, "impossible");

      BOOLEAN r = FALSE;
      UINT32 maxCnt = 0;
      _pageSAndL page;

      if (_newPagePool.empty())
      {
         goto done;
      }

      maxCnt = estimateMaxUpdatingCount(bucketNo);
      SDB_ASSERT(0 < maxCnt, "can not be zero");

      page = _newPagePool.front();
      _newPagePool.pop_front();

      candidate = fsmCandidate(page.seq, page.lpid, _maxFreeSize - size);

      if (_minFreeSize <= (_maxFreeSize - size))
      {
         fsmCandidate c(candidate);
         if (_buckets.upsert(bucketNo, c))
         {
            candidate.setBucketNo(bucketNo);
         }
      }

      while (--maxCnt > 0 && !_newPagePool.empty())
      {
         page = _newPagePool.front();
         _newPagePool.pop_front();
         fsmCandidate c(page.seq, page.lpid, _maxFreeSize);
         _buckets.upsert(bucketNo, c);
      }
      r = TRUE;
      
   done:
      return r;
   }

   INT32 freeSpaceMap::incPageFreeSize(CL_PAGE_SEQ sequence,
                                       PAGE_ID lpid,
                                       STRIPING_ID minStriping,
                                       UINT16 newFreeSize,
                                       UINT16 delta)
   {
      INT32 rc = SDB_OK;
      UINT32 bucketNo = 0;
      fsmSizeLvl oldLvl;
      fsmSizeLvl newLvl;

      if (OSS_UNLIKELY(INVALID_CL_PAGE_SEQ == sequence ||
                        INVALID_PAGE_ID == lpid ||
                        newFreeSize < delta ||
                        _maxFreeSize < newFreeSize))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      else if (OSS_UNLIKELY(!isOpen()))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      else if (newFreeSize < _minFreeSize)
      {
         goto done;
      }

      oldLvl.init(_pageSize, newFreeSize - delta);
      newLvl.init(_pageSize, newFreeSize);

      if (oldLvl.getLvl() == newLvl.getLvl())
      {
         goto done;
      }

      bucketNo = getBucketNo(minStriping);

      {
      ossScopedLock lock(&_latch, SHARED);
      if (_buckets.tryToIncBucket(sequence, lpid, bucketNo,
                                  _maxFreeSize, newFreeSize, delta))
      {
         goto done;
      }

      rc = _dfsm.updatePageFreeSizeLvL(sequence, newLvl);
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

   INT32 freeSpaceMap::decPageFreeSize(CL_PAGE_SEQ sequence,
                                       PAGE_ID lpid,
                                       STRIPING_ID minStriping,
                                       UINT16 newFreeSize,
                                       UINT16 delta)
   {
      INT32 rc = SDB_OK;
      UINT32 bucketNo = 0;
      fsmSizeLvl oldLvl;
      fsmSizeLvl newLvl;
      
      if (OSS_UNLIKELY(INVALID_CL_PAGE_SEQ == sequence ||
                        INVALID_PAGE_ID == lpid ||
                        newFreeSize < delta ||
                        _maxFreeSize < newFreeSize))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      else if (OSS_UNLIKELY(!isOpen()))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      oldLvl.init(_pageSize, newFreeSize + delta);
      newLvl.init(_pageSize, newFreeSize);
      if (oldLvl.getLvl() == newLvl.getLvl())
      {
         goto done;
      }

      bucketNo = getBucketNo(minStriping);

      {
      ossScopedLock lock(&_latch, SHARED);
      if (_buckets.tryToDecBucket(sequence, lpid, bucketNo,
                                  _minFreeSize, newFreeSize, delta))
      {
         goto done;
      }

      rc = _dfsm.updatePageFreeSizeLvL(sequence, newLvl);
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

   BOOLEAN freeSpaceMap::findFromBucket(UINT32 bucketNo,
                                        UINT32 size,
                                        fsmCandidate &candidate)
   {
      SDB_ASSERT(bucketNo < _bucketCount, "impossible");
      return _buckets.findAndAutoRemoving(bucketNo, size, _minFreeSize, candidate);
   }

  

   UINT32 freeSpaceMap::estimateMaxUpdatingCount(UINT32 bucketNo)
   {
      UINT32 maxPageCount = _buckets.getFreeSize(bucketNo);
      if (0 == maxPageCount)
      {
         maxPageCount = 1;
      }
      
   done:
      return maxPageCount;
   }

   UINT32 freeSpaceMap::getBucketNo(STRIPING_ID striping)
   {
      SDB_ASSERT(1 == _bucketCount || 32 == _bucketCount, "must be 1 or 32");
      UINT32 bucketNo = 0;
      if (1 == _bucketCount || INVALID_STRIPING_ID == striping)
      {
         return 0;
      }
      else if (striping <= _minStriping)
      {
         return 0;
      }
      else if (_maxStriping <= striping)
      {
         return _bucketCount - 1;
      }
      UINT16 range = (_maxStriping - _minStriping + 1) >> 5; /// divided by 32
      if (0 == range)
      {
         range = 1;
      }

      bucketNo = (striping - _minStriping) / range;
      bucketNo &= 0x1f;
      return bucketNo;
   }

   void freeSpaceMap::addNewPagesToPool(UINT32 count,
                                        CL_PAGE_SEQ seq,
                                        const PAGE_ID *lpids)
   {
      SDB_ASSERT(0 < count, "impossible");
      SDB_ASSERT(INVALID_CL_PAGE_SEQ != seq, "can not be invalid");
      SDB_ASSERT(NULL != lpids, "can not be null");
      for (UINT32 i = 0; i < count; ++i)
      {
         SDB_ASSERT(INVALID_PAGE_ID != lpids[i], "can not be invalid");
         if (INVALID_PAGE_ID != lpids[i])
         {
            _newPagePool.push_back(_pageSAndL(seq + i, lpids[i]));
         }
      }
      return;
   }

   void freeSpaceMap::savePagesInPool()
   {
      SDB_ASSERT(isOpen(), "must be open");
      fsmSizeLvl lvl;
      lvl.init(_pageSize, _maxFreeSize);
      _NEW_PAGE_POOL::const_iterator itr = _newPagePool.begin();
      for (; itr != _newPagePool.end(); ++itr)
      {
         _dfsm.updatePageFreeSizeLvL(itr->seq, lvl);
      }
      return;
   }

   void freeSpaceMap::savePagesInBuckets()
   {
      SDB_ASSERT(isOpen(), "must be open");
      UINT32 size = _buckets.getBucketCount();
      fsmCandidate *candidates = SDB_OSS_NEW fsmCandidate[_bucketCapacity];
      if (NULL == candidates)
      {
         PD_LOG(PDERROR, "failed to allocate mem");
         goto done;
      }
      
      for (UINT32 i = 0; i < size; ++i)
      {
         UINT32 cnt = 0;
         _buckets.dumpBucket(i, candidates, cnt);
         for (UINT32 j = 0; j < cnt; ++j)
         {
            fsmSizeLvl lvl;
            lvl.init(_pageSize, candidates[j].free);
            _dfsm.updatePageFreeSizeLvL(candidates[j].seq, lvl);
         }
      }
   done:
      if (NULL != candidates)
      {
         SDB_OSS_DEL []candidates;
      }
      return;
   }
}//namespace vessel
}//namespace engine