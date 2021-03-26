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

   Source File Name = fsmCandidateBucket.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#ifndef SDB_VESSEL_FSM_CANDIDATE_BUCKET_H_
#define SDB_VESSEL_FSM_CANDIDATE_BUCKET_H_

#include "vessel/vesselDef.h"
#include "pdTrace.hpp"
#include "ossLikely.hpp"
#include "vessel/extentDef.h"
#include "ossSpinLatch.hpp"
#include "vessel/freeSpaceMapDef.h"

namespace engine
{
namespace vessel
{

#pragma pack(4)
   class fsmCandidateBucket : public SDBObject
   {
      public:
         fsmCandidateBucket();
         ~fsmCandidateBucket();

      public:
         OSS_INLINE UINT64 getReqCnt()const;

      public:
         INT32 init(UINT32 capacity);
         void fini();

         BOOLEAN upsert(UINT32 capacity,
                        const fsmCandidate &candidate,
                        fsmCandidate *replaced=NULL);

         BOOLEAN findAndAutoRemoving(UINT32 capacity,
                                     UINT16 size,
                                     UINT16 minFreeSize,
                                     fsmCandidate &candidate);

         UINT32 getSize(UINT32 capacity)const;

         BOOLEAN tryToInc(UINT32 capacity,
                          CL_PAGE_SEQ seq,
                          PAGE_ID lpid,
                          UINT16 maxFreeSize,
                          UINT16 newFreeSize,
                          UINT16 delta);

         BOOLEAN tryToDec(UINT32 capacity,
                          CL_PAGE_SEQ seq,
                          PAGE_ID lpid,
                          UINT16 minFreeSize,
                          UINT16 newFreeSize,
                          UINT16 delta);

         BOOLEAN updateCandidate(UINT32 capacity,
                                 CL_PAGE_SEQ seq,
                                 PAGE_ID lpid,
                                 UINT16 minFreeSize,
                                 UINT16 freeSizeFromBucket,
                                 UINT16 currentFreeSize,
                                 BOOLEAN failure);

         void dump(UINT32 capacity, fsmCandidate *candidates, UINT32 &count)const;

      private:
         OSS_INLINE void remove(UINT32 i);

      private:
         struct _bucketCandidate : public SDBObject
         {
            OSS_INLINE _bucketCandidate():
            seq(INVALID_CL_PAGE_SEQ),
            lpid(INVALID_PAGE_ID),
            free(0),
            failureCnt(0),
            flags(0){}
   
            OSS_INLINE ~_bucketCandidate(){}
            OSS_INLINE _bucketCandidate(const _bucketCandidate &o):
            seq(o.seq),
            lpid(o.lpid),
            free(o.free),
            failureCnt(o.failureCnt),
            flags(o.flags){}

            OSS_INLINE _bucketCandidate &operator=(const _bucketCandidate &o)
            {
               seq = o.seq;
               lpid = o.lpid;
               free = o.free;
               failureCnt = o.failureCnt;
               flags = o.flags;
               return *this;
            }

            OSS_INLINE void reset()
            {
               seq = INVALID_CL_PAGE_SEQ;
               lpid = INVALID_PAGE_ID;
               free = 0;
               failureCnt = 0;
               flags = 0;
               return;
            }

            OSS_INLINE void reset(CL_PAGE_SEQ s,
                                  PAGE_ID l,
                                  UINT16 f)
            {
               seq = s;
               lpid = l;
               free = f;
               failureCnt = 0;
               flags = 0;
               return;
            }

            OSS_INLINE UINT8 incAndGetFaulureCnt()
            {
               return ++failureCnt;
            }

            OSS_INLINE BOOLEAN isValid()const
            {
               return INVALID_CL_PAGE_SEQ != seq;
            }

            CL_PAGE_SEQ seq;
            PAGE_ID lpid;
            UINT16 free;
            UINT8 failureCnt;
            UINT8 flags;
         };//struct _bucketCandidate

      private:
         /// we should always keep searching done in one cpu cache line.
         _bucketCandidate *_candidates;
         UINT64 _reqCnt;
   };//class fsmCandidateBucket
#pragma pack()

   OSS_INLINE UINT64 fsmCandidateBucket::getReqCnt()const
   {
      return _reqCnt;
   }
   OSS_INLINE void fsmCandidateBucket::remove(UINT32 i)
   {
      _candidates[i].reset();
   }
}//namespace vessel
}//namespace engine

#endif//SDB_VESSEL_FSM_CANDIDATE_BUCKET_H_