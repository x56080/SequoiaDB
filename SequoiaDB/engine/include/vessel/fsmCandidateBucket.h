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
   static const UINT32 FSM_CANDIDATE_BUCKET_CAPACITY = 4;

   class fsmCandidateBucket : public SDBObject
   {
      public:
         fsmCandidateBucket();
         ~fsmCandidateBucket();

      public:
         OSS_INLINE UINT32 getCapacity()const;

         /// no-latch allowed.
         OSS_INLINE UINT64 getReqCnt()const;

         OSS_INLINE BOOLEAN isFull(ossSpinLatch *latch)const;
         OSS_INLINE BOOLEAN isEmpty(ossSpinLatch *latch)const;
      public:
         /// we can save 8Bytes in one bucket if 
         /// not store latch's pointer as member.
         void upsert(ossSpinLatch *latch,
                     const fsmCandidate &candidate,
                     BOOLEAN setFillBack=TRUE,
                     fsmCandidate *replaced=NULL);

         BOOLEAN findAndAutoRemoving(ossSpinLatch *latch,
                                     UINT16 needSize,
                                     UINT16 minFreeSize,
                                     fsmCandidate &candidate);

         void guaranteeNotFull(ossSpinLatch *latch);

         UINT32 getSize(ossSpinLatch *latch)const;

      private:
         OSS_INLINE void remove(UINT32 i);

      private:
         /// we should always keep searching done in one cpu cache line.
         fsmCandidate _candidates[FSM_CANDIDATE_BUCKET_CAPACITY];
         UINT64 _reqCnt;
   };//class fsmCandidateBucket

   OSS_INLINE BOOLEAN fsmCandidateBucket::isFull(ossSpinLatch *latch)const
   {
      return FSM_CANDIDATE_BUCKET_CAPACITY == getSize(latch);
   }

   OSS_INLINE BOOLEAN fsmCandidateBucket::isEmpty(ossSpinLatch *latch)const
   {
      return 0 == getSize(latch);
   }

   OSS_INLINE UINT32 fsmCandidateBucket::getCapacity()const
   {
      return FSM_CANDIDATE_BUCKET_CAPACITY;
   }

   OSS_INLINE UINT64 fsmCandidateBucket::getReqCnt()const
   {
      return _reqCnt;
   }

   OSS_INLINE void fsmCandidateBucket::remove(UINT32 i)
   {
      if (OSS_LIKELY(i < FSM_CANDIDATE_BUCKET_CAPACITY))
      {
         _candidates[i].reset();
      }
   done:
      return;
   }
}//namespace vessel
}//namespace engine

#endif//SDB_VESSEL_FSM_CANDIDATE_BUCKET_H_