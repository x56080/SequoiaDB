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

#include "vessel/vesselIdDef.h"
#include "pdTrace.hpp"
#include "ossLikely.hpp"
#include "vessel/pageDef.h"
#include "vessel/freeSpaceMapDef.h"
#include "vessel/fsmCandidateBucket.h"
#include "ossLatch.hpp"

namespace engine
{
namespace vessel
{
#pragma pack(4)
   class fsmCandidateBuckets : public SDBObject
   {
      public:
         fsmCandidateBuckets();
         ~fsmCandidateBuckets();

      public:
         OSS_INLINE UINT32 getBucketCount()const;

      public:
         INT32 init(UINT16 bucketCount, UINT16 bucketCapacity, UINT32 latchCount);
         void fini();

      public:
         BOOLEAN upsert(UINT32 bucketNo,
                        const fsmCandidate &candidate,
                        fsmCandidate *replaced=NULL);

         BOOLEAN findAndAutoRemoving(UINT32 bucketNo,
                                     UINT16 size,
                                     UINT16 minFreeSize,
                                     fsmCandidate &candidate);

         BOOLEAN tryToIncBucket(UINT32 seq,
                                PAGE_ID lpid,
                                UINT32 bucketBegin,
                                UINT16 maxFreeSize,
                                UINT16 newFreeSize,
                                UINT16 delta);

         BOOLEAN tryToDecBucket(UINT32 seq,
                                PAGE_ID lpid,
                                UINT32 bucketBegin,
                                UINT16 minFreeSize,
                                UINT16 newFreeSize,
                                UINT16 delta);

         BOOLEAN updateCandidate(UINT32 bucketNo,
                                 UINT32 seq,
                                 PAGE_ID lpid,
                                 UINT16 minFreeSize,
                                 UINT16 freeSizeFromBucket,
                                 UINT16 currentFreeSize,
                                 BOOLEAN failure);

         UINT32 getFreeSize(UINT32 bucketNo);
         UINT64 getReqCount(UINT32 bucketNo);

         /// should ensure candidates buffer size
         void dumpBucket(UINT32 i, fsmCandidate *candidates, UINT32 &count);
      private:
         OSS_INLINE ossXLatch *getLatch(UINT32 bucketNo);
      private:
         UINT32 _bucketCount = 0;
         UINT32 _bucketCapacity = 0;
         fsmCandidateBucket *_buckets = NULL;
         UINT32 _latchCount = 0;
         ossSpinXLatch *_latches = NULL;

   };//class fsmCandidateBuckets
#pragma pack()

   OSS_INLINE UINT32 fsmCandidateBuckets::getBucketCount()const
   {
      return _bucketCount;
   }

   OSS_INLINE ossXLatch *fsmCandidateBuckets::getLatch(UINT32 bucketNo)
   {
      return &(_latches[bucketNo & (_latchCount - 1)]);
   }
}//namespace vessel
}//namespace engine

#endif//SDB_VESSEL_FSM_CANDIDATE_BUCKETS_H_