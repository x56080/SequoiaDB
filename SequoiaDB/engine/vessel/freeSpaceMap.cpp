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
#include "ossLatchGuard.hpp"

namespace engine
{
namespace vessel
{
   static const UINT32 MAX_CANDIDATE_COUNT = 64;
   static const UINT32 SINGLE_BUCKET_MODE_CAPACITY = 8;

   freeSpaceMap::freeSpaceMap()
   {
      
   }

   freeSpaceMap::~freeSpaceMap()
   {
      fini();
   }

   void freeSpaceMap::fini()
   {
      _isOpen = FALSE;
      _minStriping = INVALID_STRIPING_ID;
      _maxStriping = INVALID_STRIPING_ID;
      finiBuckets();
      _newPagePool.clear();
      _dfsm.close();
      return;
   }

   void freeSpaceMap::close()
   {
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

   INT32 freeSpaceMap::create(CL_MB_ID mbID,
                              UINT32 logicalID,
                              fsmFile *file)
   {
      INT32 rc = SDB_OK;
      static const UINT32 _BUCKET_CAPACITY = 8;

      SDB_ASSERT(!isOpen(), "do not reinit");
      if (INVALID_CL_MB_ID == mbID ||
          DMS_INVALID_LOGICCLID == logicalID ||
          NULL == file || !file->isOpen())
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      _isOpen = TRUE;

      rc = initBuckets(1, SINGLE_BUCKET_MODE_CAPACITY, 1);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to init buckets:%d", rc);
         goto error;
      }

      rc = _dfsm.create(file, mbID, logicalID);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to create fsm on disk:%d", rc);
         goto error;
      }
   done:
      return rc;
   error:
      fini();
      goto done;
   }

   INT32 freeSpaceMap::create(CL_MB_ID mbID,
                              UINT32 logicalID,
                              fsmFile *file,
                              UINT32 bucketCount,
                              STRIPING_ID min,
                              STRIPING_ID max)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(!isOpen(), "already open");
      UINT32 latchCount = 0;

      if (NULL == file || !file->isOpen() ||
          INVALID_CL_MB_ID == mbID ||
          DMS_INVALID_LOGICCLID == logicalID ||
          !ossIsPowerOf2(bucketCount) ||
          MAX_CANDIDATE_COUNT < bucketCount ||
          INVALID_STRIPING_ID == min ||
          INVALID_STRIPING_ID == max ||
          max < min)
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      _isOpen = TRUE;
      _minStriping = min;
      _maxStriping = max;

      latchCount = bucketCount / 8;
      if (0 == latchCount)
      {
         latchCount = 1;
      }

      rc = initBuckets(bucketCount,
                       MAX_CANDIDATE_COUNT / bucketCount,
                       latchCount);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to init buckets:%d", rc);
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
      fini();
      goto done;
   }

   INT32 freeSpaceMap::open(CL_MB_ID mbID,
                            UINT32 logicalID,
                            UINT32 pageCount,
                            fsmFile *file,
                            UINT32 bucketCount,
                            STRIPING_ID min,
                            STRIPING_ID max)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(!isOpen(), "already open");
      
      UINT32 latchCount = 0;

      if (NULL == file || !file->isOpen() ||
          INVALID_CL_MB_ID == mbID ||
          DMS_INVALID_LOGICCLID == logicalID ||
          !ossIsPowerOf2(bucketCount) ||
          MAX_CANDIDATE_COUNT < bucketCount ||
          INVALID_STRIPING_ID == min ||
          INVALID_STRIPING_ID == max ||
          max < min)
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      _isOpen = TRUE;
      _minStriping = min;
      _maxStriping = max;

      latchCount = bucketCount / 8;
      if (0 == latchCount)
      {
         latchCount = 1;
      }

      rc = initBuckets(bucketCount,
                       MAX_CANDIDATE_COUNT / bucketCount,
                       latchCount);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to init buckets:%d", rc);
         goto error;
      }

      rc = _dfsm.open(file, mbID, logicalID, pageCount, TRUE);
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

   INT32 freeSpaceMap::open(CL_MB_ID mbID,
                            UINT32 logicalID,
                            UINT32 pageCount,
                            fsmFile *file)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(!isOpen(), "already open");
      
      UINT32 latchCount = 0;

      if (NULL == file || !file->isOpen() ||
          INVALID_CL_MB_ID == mbID ||
          DMS_INVALID_LOGICCLID == logicalID)
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      _isOpen = TRUE;

      rc = initBuckets(1, SINGLE_BUCKET_MODE_CAPACITY, 1);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to init buckets:%d", rc);
         goto error;
      }

      rc = _dfsm.open(file, mbID, logicalID, pageCount, TRUE);
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

   INT32 freeSpaceMap::fastFind(INT32 lvl,
                                STRIPING_ID striping,
                                fsmCandidate &candidate,
                                UINT16 *bucketTick)
   {
      INT32 rc = SDB_OK;
      UINT32 bucketNo = 0;
      candidate.reset();

      if (OSS_UNLIKELY(!isOpen()))
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }
      else if (OSS_UNLIKELY(!isValidFsmLvL(lvl)))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      else if (isSharded() && INVALID_STRIPING_ID == striping)
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      if (isSharded())
      {
         bucketNo = getBucketNo(striping);
      }

      rc = findFromBucket(bucketNo, lvl, candidate, bucketTick);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to find from bucket:%d", rc);
         goto error;
      }
   done:
      return rc;
   error:
      goto done;
   }

   INT32 freeSpaceMap::find(INT32 lvl,
                            STRIPING_ID striping,
                            fsmCandidate &candidate,
                            const UINT16 *bucketTick)
   {
      INT32 rc = SDB_OK;
      UINT32 bucketNo = 0;
      candidate.reset();

      if (OSS_UNLIKELY(!isOpen()))
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }
      else if (OSS_UNLIKELY(!isValidFsmLvL(lvl)))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      else if (isSharded() && INVALID_STRIPING_ID == striping)
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      if (isSharded())
      {
         bucketNo = getBucketNo(striping);
      }

      /// find from bucket
      if (NULL == bucketTick ||
          *bucketTick != _bucketTicks[bucketNo])
      {
         rc = findFromBucket(bucketNo, lvl, candidate, NULL);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to find from bucket:%d", rc);
            goto error;
         }

         if (candidate.isValid())
         {
            goto done;
         }
      }

      /// find from pool
      if (0 < _newPagePool.getSizeWithNoLock())
      {
         rc = findFromNewPagePool(candidate);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to find in pool:%d", rc);
            goto error;
         }
         else if (candidate.isValid())
         {
            rc = upsertIntoBucket(bucketNo,
                                  candidate.getSeq(),
                                  candidate.getInfoPtr());
            if (SDB_OK != rc)
            {
               PD_LOG(PDERROR, "failed to upsert page[%d] into bucket, rc:%d",
                      candidate.getSeq(), rc);
               rc = SDB_OK;
            }

            goto done;
         }
      }

      /// find from disk
      rc = findFromDiskMap(lvl, candidate);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to find from disk map:%d", rc);
         goto error;
      }
      else if (candidate.isValid())
      {
         rc = upsertIntoBucket(bucketNo,
                               candidate.getSeq(),
                               candidate.getInfoPtr());
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to upsert page[%d] into bucket, rc:%d",
                     candidate.getSeq(), rc);
            rc = SDB_OK;
         }

         goto done;
      }
   done:
      return rc;
   error:
      goto done;
   }

   INT32 freeSpaceMap::insertNewPages(UINT32 firstSeq,
                                      const PAGE_ID *lpids,
                                      UINT32 count)
   {
      INT32 rc = SDB_OK;

      if (OSS_UNLIKELY(!isOpen()))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      else if (OSS_UNLIKELY(INVALID_CL_PAGE_SEQ == firstSeq ||
                            0 == count ||
                            NULL == lpids))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      addNewPagesToPool(count, firstSeq, lpids);

      rc = _dfsm.incDataPageCount(count);
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

   INT32 freeSpaceMap::findFromDiskMap(INT32 lvl,
                                       fsmCandidate &candidate)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(isValidFsmLvL(lvl), "can not be invalid");
      SDB_ASSERT(!candidate.isValid(), "can not be valid");
      
      UINT32 seq = INVALID_CL_PAGE_SEQ;
      BOOLEAN found = FALSE;
      INT32 realLvl = FSM_INVALID_SPACE_LVL;

      rc = _dfsm.find(lvl, found, seq, realLvl);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to find free space from disk map:%d", rc);
         goto error;
      }
      
      if (found)
      {
         fsmCandidate::SHARED_INFO_PTR sptr;
         rc = makeFsmCandidateSharedInfoPtr(INVALID_PAGE_ID, realLvl, sptr);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to make shared ptr:%d", rc);
            goto error;
         }

         candidate.reset(seq, sptr);
      }
   done:
      return rc;
   error:
      if (INVALID_CL_PAGE_SEQ != seq)
      {
         INT32 trc = _dfsm.upgradePageSpaceLvl(seq, realLvl);
         if (SDB_OK != trc)
         {
            PD_LOG(PDERROR, "failed to give back page[%d], rc:%d", seq, rc);
         }
      }
      goto done;
   }

   INT32 freeSpaceMap::findFromNewPagePool(fsmCandidate &candidate)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(!candidate.isValid(), "can not be valid");
      freeSpaceTuple tuple;
      if (_newPagePool.popForward(tuple))
      {
         fsmCandidate::SHARED_INFO_PTR sptr;
         rc = makeFsmCandidateSharedInfoPtr(tuple.getLpid(),
                                            tuple.getSpaceLvl(),
                                            sptr);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to make shared ptr:%d", rc);
            goto error;
         }

         candidate.reset(tuple.getSeq(), sptr);
      }

      
   done:
      return rc;
   error:
      if (tuple.isValid())
      {
         if (SDB_OK != _newPagePool.pushForward(tuple))
         {
            PD_LOG(PDERROR, "failed to give back tuple[%d]", tuple.getSeq());
         }
      }
      goto done;
   }

   INT32 freeSpaceMap::findFromBucket(UINT32 bucketNo,
                                      INT32 lvl,
                                      fsmCandidate &candidate,
                                      UINT16 *tick)
   {
      SDB_ASSERT(bucketNo < _bucketCount, "impossible");
      SDB_ASSERT(isValidFsmLvL(lvl), "can not be invalid");
      INT32 rc = SDB_OK;
      ossXLatchGuard guard(getBucketLatch(bucketNo));
   
      rc = _buckets[bucketNo].find(lvl, _newPagePool, candidate);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to find candidate in bucket[%d], rc:%d",
                bucketNo, rc);
         goto error;
      }

      if (NULL != tick)
      {
         *tick = _bucketTicks[bucketNo];
      }
   done:
      return rc;
   error:
      goto done;
   }

   INT32 freeSpaceMap::upsertIntoBucket(UINT32 bucketNo,
                                        UINT32 seq,
                                        const fsmCandidate::SHARED_INFO_PTR sptr)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(bucketNo < _bucketCount, "impossible");
      SDB_ASSERT(INVALID_CL_PAGE_SEQ != seq, "can not be invalid");
      SDB_ASSERT(NULL != sptr.get(), "can not be invalid");
      ossXLatchGuard guard(getBucketLatch(bucketNo));

      rc = _buckets[bucketNo].upsert(seq, sptr);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to upsert into bucket:%d", rc);
         goto error;
      }

      ++_bucketTicks[bucketNo];
   done:
      return rc;
   error:
      goto done;
   }

   UINT32 freeSpaceMap::getBucketNo(STRIPING_ID striping)const
   {
      SDB_ASSERT(0 < _bucketCount, "can not be invalid");
      SDB_ASSERT(INVALID_STRIPING_ID != striping, "can not be invalid");
      SDB_ASSERT(_minStriping <= striping, "can not be invalid");
      SDB_ASSERT(striping <= _maxStriping, "can not be invalid");

      UINT32 bucketNo = 0;
      UINT32 totalStripingCount = 0;

      if (1 == _bucketCount)
      {
         return 0;
      }
      if (striping == _maxStriping)
      {
         return _bucketCount - 1;
      }

      totalStripingCount = _maxStriping - _minStriping + 1;
      if (totalStripingCount < _bucketCount)
      {
         bucketNo = (striping - _minStriping);
      }
      else
      {
         UINT16 range = totalStripingCount / _bucketCount;
         bucketNo = (striping - _minStriping) / range;
         SDB_ASSERT(bucketNo < _bucketCount, "impossible");
      }
      return bucketNo;
   }

   INT32 freeSpaceMap::initBuckets(UINT32 bucketCount,
                                   UINT32 bucketCapacity,
                                   UINT32 bucketLatchCount)
   {
      INT32 rc = SDB_OK;
      if (OSS_UNLIKELY(!ossIsPowerOf2(bucketCount) ||
                       !ossIsPowerOf2(bucketCapacity) ||
                       !ossIsPowerOf2(bucketLatchCount) ||
                       bucketCount < bucketLatchCount))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      _bucketCapacity = bucketCapacity;
      _bucketCount = bucketCount;
      _bucketLatchCount = bucketLatchCount;

      _buckets = SDB_OSS_NEW fsmCandidateBucket[_bucketCount];
      if (NULL == _buckets)
      {
         PD_LOG(PDERROR, "failed to allocate mem");
         rc = SDB_OOM;
         goto error;
      }

      for (UINT32 i = 0; i < _bucketCount; ++i)
      {
         rc = _buckets[i].init(bucketCapacity);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to init bucket[%d], rc:%d", i, rc);
            goto error;
         }
      }

      _bucketLatches = SDB_OSS_NEW ossSpinXLatch[_bucketLatchCount];
      if (NULL == _bucketLatches)
      {
         PD_LOG(PDERROR, "failed to allocate mem");
         rc = SDB_OOM;
         goto error;
      }

      _bucketTicks = new UINT16[_bucketCount];
      if (NULL == _bucketTicks)
      {
         PD_LOG(PDERROR, "failed to allocate mem");
         rc = SDB_OOM;
         goto error;
      }
      /// No need to zeroed _bucketTicks.
      /// We just want to diff old/current value.
   done:
      return rc;
   error:
      goto done;
   }

   void freeSpaceMap::finiBuckets()
   {
      if (NULL != _buckets)
      {
         SDB_OSS_DEL []_buckets;
         _buckets = NULL;
      }
      if (NULL != _bucketLatches)
      {
         SDB_OSS_DEL []_bucketLatches;
         _bucketLatches = NULL;
      }
      if (NULL != _bucketTicks)
      {
         delete []_bucketTicks;
         _bucketTicks = NULL;
      }
      _bucketCapacity = 0;
      _bucketCount = 0;
      _bucketLatchCount = 0;
      return;
   }

   void freeSpaceMap::addNewPagesToPool(UINT32 count,
                                        UINT32 firstSeq,
                                        const PAGE_ID *lpids)
   {
      SDB_ASSERT(0 < count, "impossible");
      SDB_ASSERT(INVALID_CL_PAGE_SEQ != firstSeq, "can not be invalid");
      SDB_ASSERT(NULL != lpids, "can not be null");

      for (UINT32 i = 0; i < count; ++i)
      {
         SDB_ASSERT(INVALID_PAGE_ID != lpids[i], "can not be invalid");
         INT32 rc = _newPagePool.pushForward(freeSpaceTuple(firstSeq + i,
                                                            lpids[i],
                                                            FSM_MAX_SPACE_LVL));
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to add new page[%d] to pool:%d",
                   firstSeq + i, rc);
         }
      }
      return;
   }

   void freeSpaceMap::savePagesInPool()
   {
      SDB_ASSERT(isOpen(), "must be open");
      freeSpaceTuple tuple;
      while (_newPagePool.popForward(tuple))
      {
         INT32 rc = _dfsm.upgradePageSpaceLvl(tuple.getSeq(),
                                              tuple.getSpaceLvl());
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to upgrade page[%d] on disk, rc:%d",
                   tuple.getSeq(), rc);
         }
      }
      return;
   }

   void freeSpaceMap::savePagesInBuckets()
   {
      SDB_ASSERT(isOpen(), "must be open");
      ossPoolList<freeSpaceTuple> tuples;
      
      for (UINT32 i = 0; i < _bucketCount; ++i)
      {
         _buckets[i].dumpTuplesWithValidLvl(tuples);
      }

      for (ossPoolList<freeSpaceTuple>::const_iterator itr = tuples.begin();
           itr != tuples.end(); ++itr)
      {
         INT32 rc = _dfsm.upgradePageSpaceLvl(itr->getSeq(),
                                              itr->getSpaceLvl());
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to upgrade page[%d] on disk, rc:%d",
                   itr->getSeq(), rc);
         }
      }
      return;
   }
}//namespace vessel
}//namespace engine