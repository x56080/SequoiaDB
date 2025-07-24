/*******************************************************************************

   Copyright (C) 2011-Present SequoiaDB Ltd.

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

      http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.

   Source File Name = freeSpaceMap.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

*******************************************************************************/
#include "vessel/freeSpaceMap.h"
#include "ossLatchGuard.hpp"
#include "vessel/requestContext.h"

namespace engine
{
namespace vessel
{
   constexpr UINT32 CANDIDATE_BUCKET_COUNT = 16;
   constexpr UINT32 CANDIDATE_BUCKET_CAPACITY = 2;

   static UINT32 _getCanddiateBucketLatchCount()
   {
      UINT32 count = CANDIDATE_BUCKET_COUNT / 2;
      return 0 == count ? 1 : count;
   }

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
      _stripingRange.reset();
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

   void freeSpaceMap::truncate()
   {
      if (!isOpen())
      {
         goto done;
      }

      for (UINT32 i = 0; i < _bucketCount; ++i)
      {
         _buckets[i].clear();
      }
      _newPagePool.clear();
      _dfsm.truncate();
      
   done:
      return;
   }

   INT32 freeSpaceMap::create(CL_MB_ID mbID,
                              UINT32 logicalID,
                              fsmFile *file,
                              const dmsStripingRange &range)
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

      _isOpen = TRUE;
      _stripingRange = range;

      latchCount = _getCanddiateBucketLatchCount();
      bucketCount = CANDIDATE_BUCKET_COUNT;
      bucketCapacity = CANDIDATE_BUCKET_CAPACITY;

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
                            const dmsStripingRange &range)
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

      _isOpen = TRUE;
      _stripingRange = range;

      latchCount = _getCanddiateBucketLatchCount();
      bucketCount = CANDIDATE_BUCKET_COUNT;
      bucketCapacity = CANDIDATE_BUCKET_CAPACITY;

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
                            const dmsStripingId &striping,
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
      else if (hasStipingRange() && !striping.isValid())
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      if (hasStipingRange())
      {
         bucketNo = getBucketNoByStriping(striping);
      }
      else
      {
         bucketNo = getBucketNoByEid(context->getExecutor()->getID());
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

   INT32 freeSpaceMap::findAndKick(INT32 lvl,
                                   fsmCandidate &candidate)
   {
      INT32 rc = SDB_OK;
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

      rc = findFromNewPagePool(candidate);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to find candidate from new page pool:%d", rc);
         goto error;
      }

      if (candidate.isValid())
      {
         goto done;
      }
      
         
      rc = findFromDiskMap(lvl, candidate);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to find candidate from disk map:%d", rc);
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
      else if (OSS_UNLIKELY( 0 == count ||
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
      else if (OSS_UNLIKELY(!isValidFsmLvL(lvl)))
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
      else if (OSS_UNLIKELY(!isValidFsmLvL(lvl)))
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
      
      UINT32 seq = 0;
      BOOLEAN found = FALSE;
      INT32 realLvl = FSM_INVALID_SPACE_LVL;
      candidate.reset();

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
      if (FSM_INVALID_SPACE_LVL != realLvl)
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
      
      ossXLatchGuard guard(getBucketLatch(bucketNo));
      fsmCandidateBucket &bucket = _buckets[bucketNo];

      if (!bucket.isFull() && 0 < _newPagePool.peekSize())
      {
         BOOLEAN upserted = FALSE;
         rc = upsertIntoBucketFromNewPagePool(bucket, upserted);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to upsert bucket:%d", rc);
            goto error;
         }
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

      //if (0 < _newPagePool.peekSize())
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

            goto done;
         }
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
      if (FSM_INVALID_SPACE_LVL != tuple.getSpaceLvl())
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
      if (FSM_INVALID_SPACE_LVL != tuple.getSpaceLvl())
      {
         if (SDB_OK != _newPagePool.pushForward(tuple))
         {
            PD_LOG(PDERROR, "failed to give back tuple[%d]", tuple.getSeq());
         }
      }
      goto done;
   }

   UINT32 freeSpaceMap::getBucketNoByStriping(const dmsStripingId &striping)const
   {
      SDB_ASSERT(0 < _bucketCount, "can not be invalid");
      SDB_ASSERT(striping.isValid(), "can not be invalid");
      SDB_ASSERT(_stripingRange.isValid(), "can not be invalid");
      SDB_ASSERT(_stripingRange.contains(striping), "out of bound");

      UINT32 bucketNo = 0;
      UINT32 totalStripingCount = 0;

      if (1 == _bucketCount)
      {
         return 0;
      }
      if (striping == _stripingRange.getHigh())
      {
         return _bucketCount - 1;
      }

      totalStripingCount = _stripingRange.getStripingCount();
      if (totalStripingCount < _bucketCount)
      {
         bucketNo = (striping.getValue() - _stripingRange.getLow().getValue());
      }
      else
      {
         UINT16 range = ossAlignX(totalStripingCount, _bucketCount) / _bucketCount;
         bucketNo = (striping.getValue() - _stripingRange.getLow().getValue()) / range;
      }

      SDB_ASSERT(bucketNo < _bucketCount, "out of bound");
      return bucketNo;
   }

   UINT32 freeSpaceMap::getBucketNoByEid(EDUID eid)const
   {
      SDB_ASSERT(0 < _bucketCount, "can not be invalid");
      return eid % _bucketCount;
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
      SDB_ASSERT(NULL != lpids, "can not be null");

      /// pool is first in last out
      for (INT32 i = (INT32)count - 1; 0 <= i; --i)
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