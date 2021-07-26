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
#include "vessel/requestContext.h"

namespace engine
{
namespace vessel
{
   static const UINT32 SHARED_CL_BUCKET_COUNT = 16;
   static const UINT32 SHARED_CL_BUCKET_CAPACITY  = 2;
   static const UINT32 NONSHARED_CL_BUCKET_COUNT = 1;
   static const UINT32 NONSHARED_CL_BUCKET_CAPACITY = 8;

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

      _bucketCount = 0;
      _bucketLatchCount = 0;
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

   void freeSpaceMap::destroy()
   {
      if (!isOpen())
      {
         goto done;
      }

      _dfsm.destroy();
      fini();
   done:
      return;
   }

   INT32 freeSpaceMap::create(CL_MB_ID mbID,
                              UINT32 logicalID,
                              fsmFile *file,
                              STRIPING_ID min,
                              STRIPING_ID max)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(!isOpen(), "already open");
      UINT32 latchCount = 0;
      UINT32 bucketCount = 0;
      UINT32 bucketCapacity = 0;

      if (NULL == file || !file->isOpen() ||
          INVALID_CL_MB_ID == mbID ||
          DMS_INVALID_LOGICCLID == logicalID)
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      else if((INVALID_STRIPING_ID != min || INVALID_STRIPING_ID != max )
              &&
              ((INVALID_STRIPING_ID == min || INVALID_STRIPING_ID == max) ||
               max < min))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      _isOpen = TRUE;
      _minStriping = min;
      _maxStriping = max;

      if (isSharded())
      {
         latchCount = SHARED_CL_BUCKET_COUNT;
         bucketCount = SHARED_CL_BUCKET_COUNT;
         bucketCapacity = SHARED_CL_BUCKET_CAPACITY;
      }
      else
      {
         latchCount = NONSHARED_CL_BUCKET_COUNT;
         bucketCount = NONSHARED_CL_BUCKET_COUNT;
         bucketCapacity = NONSHARED_CL_BUCKET_CAPACITY;
      }

      rc = initBuckets(bucketCount,
                       bucketCapacity,
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
                            STRIPING_ID min,
                            STRIPING_ID max)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(!isOpen(), "already open");
      
      UINT32 latchCount = 0;
      UINT32 bucketCount = 0;
      UINT32 bucketCapacity = 0;

      if (NULL == file || !file->isOpen() ||
          INVALID_CL_MB_ID == mbID ||
          DMS_INVALID_LOGICCLID == logicalID)
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      else if((INVALID_STRIPING_ID != min || INVALID_STRIPING_ID != max )
              &&
              ((INVALID_STRIPING_ID == min || INVALID_STRIPING_ID == max) ||
               max < min))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      _isOpen = TRUE;
      _minStriping = min;
      _maxStriping = max;

      if (isSharded())
      {
         latchCount = SHARED_CL_BUCKET_COUNT;
         bucketCount = SHARED_CL_BUCKET_COUNT;
         bucketCapacity = SHARED_CL_BUCKET_CAPACITY;
      }
      else
      {
         latchCount = NONSHARED_CL_BUCKET_COUNT;
         bucketCount = NONSHARED_CL_BUCKET_COUNT;
         bucketCapacity = NONSHARED_CL_BUCKET_CAPACITY;
      }

      rc = initBuckets(bucketCount,
                       bucketCapacity,
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


   INT32 freeSpaceMap::find(requestContext *context,
                            INT32 lvl,
                            STRIPING_ID striping,
                            fsmCandidate &candidate)
   {
      INT32 rc = SDB_OK;
      UINT32 bucketNo = 0;
      candidate.reset();

      if (OSS_UNLIKELY(!isOpen()))
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }
      else if (OSS_UNLIKELY(NULL == context || !isValidFsmLvL(lvl)))
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
      else
      {
         bucketNo = (context->getSession()->getSessionID() & (_bucketCount - 1));
      }

      rc = _find(bucketNo, lvl, candidate);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to find candidate:%d", rc);
         goto error;
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

   INT32 freeSpaceMap::upgradePageSpaceLvl(UINT32 seq,
                                           INT32 lvl)
   {
      INT32 rc = SDB_OK;
      if (OSS_UNLIKELY(!isOpen()))
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }
      else if (OSS_UNLIKELY(INVALID_CL_PAGE_SEQ == seq ||
                            !isValidFsmLvL(lvl)))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      rc = _dfsm.upgradePageSpaceLvl(seq, lvl);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to upgrade space lvl in disk map:%d", rc);
         goto error;
      }
   done:
      return rc;
   error:
      goto done;
   }

   INT32 freeSpaceMap::downgradePgaeSpaceLvl(UINT32 seq,
                                             INT32 lvl)
   {
      INT32 rc = SDB_OK;
      if (OSS_UNLIKELY(!isOpen()))
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }
      else if (OSS_UNLIKELY(INVALID_CL_PAGE_SEQ == seq ||
                            !isValidFsmLvL(lvl)))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      rc = _dfsm.downgradePgaeSpaceLvl(seq, lvl);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to upgrade space lvl in disk map:%d", rc);
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


   INT32 freeSpaceMap::_find(UINT32 bucketNo,
                             INT32 lvl,
                             fsmCandidate &candidate)
   {
      SDB_ASSERT(bucketNo < _bucketCount, "impossible");
      SDB_ASSERT(isValidFsmLvL(lvl), "can not be invalid");
      SDB_ASSERT(!candidate.isValid(), "can not be valid");
      INT32 rc = SDB_OK;
      BOOLEAN upserted = FALSE;
      ossXLatchGuard guard(getBucketLatch(bucketNo));
      fsmCandidateBucket &bucket = _buckets[bucketNo];

      if (!bucket.isFull() && 0 < _newPagePool.getSizeWithNoLock())
      {
         rc = upsertIntoBucketFromNewPagePool(bucket, upserted);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to upsert bucket:%d", rc);
            goto error;
         }

         upserted = FALSE;
         /// Do not care about result.
      }
   
      rc = bucket.find(lvl, candidate);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to find candidate in bucket[%d], rc:%d",
               bucketNo, rc);
         goto error;
      }

      if (candidate.isValid())
      {
         goto done;
      }

      if (0 < _newPagePool.getSizeWithNoLock())
      {
         rc = findFromNewPagePool(candidate);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to find candidate from new page pool:%d", rc);
            goto error;
         }

         if (candidate.isValid())
         {
            rc = bucket.upsert(candidate.getSeq(), candidate.getInfoPtr());
            if (SDB_OK != rc)
            {
               PD_LOG(PDERROR, "failed to upsert into bucket:%d", rc);
               goto error;
            }
         }
         goto done;
      }

      rc = findFromDiskMap(lvl, candidate);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to find candidate from disk map:%d", rc);
         goto error;
      }

      if (candidate.isValid())
      {
         rc = bucket.upsert(candidate.getSeq(), candidate.getInfoPtr());
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to upsert into bucket:%d", rc);
            goto error;
         }
      }
   done:
      return rc;
   error:
      goto done;
   }

   INT32 freeSpaceMap::upsertIntoBucketFromNewPagePool(fsmCandidateBucket &bucket,
                                                       BOOLEAN &upserted)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(bucket.isOpen(), "can not be closed");
      
      freeSpaceTuple tuple;
      fsmCandidate::SHARED_INFO_PTR sptr;
      upserted = FALSE;

      if (_newPagePool.popForward(tuple))
      {
         rc = makeFsmCandidateSharedInfoPtr(tuple.getLpid(),
                                            tuple.getSpaceLvl(),
                                            sptr);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to make shared ptr:%d", rc);
            goto error;
         }

         rc = bucket.upsert(tuple.getSeq(), sptr);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to upsert bucket:%d", rc);
            goto error;
         }
         upserted = TRUE;
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

   INT32 freeSpaceMap::findFromNewPagePool(fsmCandidate &candidate)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(!candidate.isValid(), "can not be valid");

      freeSpaceTuple tuple;
      fsmCandidate::SHARED_INFO_PTR sptr;

      if (_newPagePool.popForward(tuple))
      {
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