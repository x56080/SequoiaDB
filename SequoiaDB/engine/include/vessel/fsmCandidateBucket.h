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

   Source File Name = fsmCandidateBucket.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef SDB_VESSEL_FSM_CANDIDATE_BUCKET_H_
#define SDB_VESSEL_FSM_CANDIDATE_BUCKET_H_

#include "vessel/vesselIdDef.h"
#include "pdTrace.hpp"
#include "ossLikely.hpp"
#include "vessel/pageDef.h"
#include "vessel/freeSpaceMapDef.h"
#include "vessel/fsmCandidate.h"
#include "vessel/freeSpaceTuple.h"
#include "ossMemPool.hpp"

namespace engine
{
namespace vessel
{
   class requestContext;
   
   class fsmCandidateBucket : public SDBObject
   {
      public:
         fsmCandidateBucket();
         ~fsmCandidateBucket();
         fsmCandidateBucket(const fsmCandidateBucket &) = delete;
         fsmCandidateBucket &operator=(const fsmCandidateBucket &) = delete;

      public:
         OSS_INLINE UINT64 getReqCnt()const;
         OSS_INLINE UINT32 getCapacity()const;
         OSS_INLINE UINT32 getSize()const;
         OSS_INLINE BOOLEAN isOpen()const;
         OSS_INLINE BOOLEAN isFull()const;

      public:
         INT32 init(UINT32 capacity);
         void fini();
         void clear();

         INT32 upsert(UINT32 seq,
                      const fsmCandidate::SHARED_INFO_PTR &sptr);

         INT32 find(INT32 lvl,
                    fsmCandidate &candidate);

         void dumpTuplesWithValidLvl(ossPoolList<freeSpaceTuple> &tuples)const;

      private:
         class _bucketCandidate : public SDBObject
         {
            public:
               _bucketCandidate(){}
               ~_bucketCandidate(){}
               _bucketCandidate(const _bucketCandidate &) = delete;
               _bucketCandidate &operator=(const _bucketCandidate &) = delete;

               OSS_INLINE void reset()
               {
                  _seq = 0;
                  _sptr.reset();
                  _failureCnt = 0;
                  return;
               }

               void reset(UINT32 seq,
                          const fsmCandidate::SHARED_INFO_PTR &sptr)
               {
                  _seq = seq;
                  _sptr = sptr;
                  _failureCnt = 0;
                  return;
               }

               OSS_INLINE UINT32 incAndGetFaulureCnt()
               {
                  return ++_failureCnt;
               }

               OSS_INLINE BOOLEAN isValid()const
               {
                  return nullptr != _sptr.get();
               }

               OSS_INLINE UINT32 getSeq()const
               {
                  return _seq;
               }
               OSS_INLINE const fsmCandidate::SHARED_INFO_PTR &getInfoPtr()const
               {
                  return _sptr;
               }
               OSS_INLINE UINT32 getFailureCnt()const
               {
                  return _failureCnt;
               }

            private:
               UINT32 _seq = 0;
               fsmCandidate::SHARED_INFO_PTR _sptr;
               UINT32 _failureCnt = 0;
         };//struct _bucketCandidate

      private:
         UINT32 _capacity = 0;
         UINT32 _size = 0;
         /// we should always keep searching done in few cpu cache lines.
         _bucketCandidate *_candidates = nullptr;
         UINT64 _reqCnt = 0;
   };//class fsmCandidateBucket

   OSS_INLINE BOOLEAN fsmCandidateBucket::isOpen()const
   {
      return NULL != _candidates;
   }
   OSS_INLINE UINT64 fsmCandidateBucket::getReqCnt()const
   {
      return _reqCnt;
   }
   OSS_INLINE UINT32 fsmCandidateBucket::getCapacity()const
   {
      return _capacity;
   }
   OSS_INLINE UINT32 fsmCandidateBucket::getSize()const
   {
      return _size;
   }
   OSS_INLINE BOOLEAN fsmCandidateBucket::isFull()const
   {
      return _capacity == _size;
   }
}//namespace vessel
}//namespace engine

#endif//SDB_VESSEL_FSM_CANDIDATE_BUCKET_H_