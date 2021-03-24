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

   Source File Name = fsmCandidateBucketss.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#ifndef SDB_VESSEL_FSM_CANDIDATE_BUCKETS_H_
#define SDB_VESSEL_FSM_CANDIDATE_BUCKETS_H_

#include "vessel/vesselDef.h"
#include "pdTrace.hpp"
#include "ossLikely.hpp"
#include "vessel/extentDef.h"
#include "ossSpinLatch.hpp"
#include "vessel/freeSpaceMapDef.h"
#include "vessel/fsmCandidateBucket.h"

namespace engine
{
namespace vessel
{
   class fsmCandidateBuckets : public SDBObject
   {
      public:
         fsmCandidateBuckets();
         ~fsmCandidateBuckets();

      public:
         OSS_INLINE UINT32 getBucketCount()const;

      public:
         INT32 init(UINT32 bucketCount, UINT32 latchCount);
         void fini();

      public:
         BOOLEAN upsert(UINT32 bucketNo,
                        const fsmCandidate &candidate,
                        fsmCandidate *replaced=NULL);

         BOOLEAN findAndAutoRemoving(UINT32 bucketNo,
                                     UINT16 size,
                                     UINT16 minFreeSize,
                                     fsmCandidate &candidate);

         BOOLEAN tryToIncBucket(CL_PAGE_SEQ seq,
                                PAGE_ID lpid,
                                UINT32 bucketBegin,
                                UINT16 maxFreeSize,
                                UINT16 newFreeSize,
                                UINT16 delta);

         BOOLEAN tryToDecBucket(CL_PAGE_SEQ seq,
                                PAGE_ID lpid,
                                UINT32 bucketBegin,
                                UINT16 minFreeSize,
                                UINT16 newFreeSize,
                                UINT16 delta);

         BOOLEAN fillback(UINT32 bucketNo,
                          CL_PAGE_SEQ seq,
                          PAGE_ID lpid,
                          BOOLEAN failure);

         UINT32 getFreeSize(UINT32 bucketNo);
         UINT64 getReqCount(UINT32 bucketNo);

         /// candidates buffer size should be FSM_CANDIDATE_BUCKET_CAPACITY
         void dumpBucket(UINT32 i, fsmCandidate *candidates, UINT32 &count);
      private:
         OSS_INLINE ossSpinLatch *getLatch(UINT32 bucketNo);

      private:
         UINT32 _bucketCount;
         fsmCandidateBucket *_buckets;
         UINT32 _latchCount;
         ossSpinLatch *_latches;

   };//class fsmCandidateBuckets

   OSS_INLINE UINT32 fsmCandidateBuckets::getBucketCount()const
   {
      return _bucketCount;
   }

   OSS_INLINE ossSpinLatch *fsmCandidateBuckets::getLatch(UINT32 bucketNo)
   {
      return &(_latches[bucketNo & (_latchCount - 1)]);
   }
}//namespace vessel
}//namespace engine

#endif//SDB_VESSEL_FSM_CANDIDATE_BUCKETS_H_