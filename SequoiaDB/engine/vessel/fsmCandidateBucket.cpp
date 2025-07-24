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

   Source File Name = fsmCandidateBucket.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

*******************************************************************************/
#include "vessel/fsmCandidateBucket.h"
#include "vessel/freeSpaceMapDef.h"

namespace engine
{
namespace vessel
{
   static const UINT32 MAX_FAILURE_COUNT = 8;

///////////fsmCandidateBucket
   fsmCandidateBucket::fsmCandidateBucket()
   {

   }

   fsmCandidateBucket::~fsmCandidateBucket()
   {
      fini();
   }

   INT32 fsmCandidateBucket::init(UINT32 capacity)
   {
      INT32 rc = SDB_OK;
      fini();

      if (!ossIsPowerOf2(capacity))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      _capacity = capacity;
      _candidates = SDB_OSS_NEW _bucketCandidate[capacity];
      if (NULL == _candidates)
      {
         PD_LOG(PDERROR, "failed to allocate mem");
         rc = SDB_OOM;
         goto error;
      }
   done:
      return rc;
   error:
      fini();
      goto done;
   }

   void fsmCandidateBucket::fini()
   {
      _capacity = 0;
      _size = 0;
      if (NULL != _candidates)
      {
         SDB_OSS_DEL []_candidates;
         _candidates = NULL;
      }
      _reqCnt = 0;
      return;
   }

   void fsmCandidateBucket::clear()
   {
      SDB_ASSERT(NULL != _candidates, "can not be null");
      for (UINT32 i = 0; i < _size; ++i)
      {
         _candidates[i].reset();
      }
      _size = 0;
      return;
   }

   INT32 fsmCandidateBucket::upsert(UINT32 seq,
                                    const fsmCandidate::SHARED_INFO_PTR &sptr)
   {
      INT32 rc = SDB_OK;
      INT32 pos = -1;

      if (OSS_UNLIKELY(!isOpen()))
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }
      else if (OSS_UNLIKELY(NULL == sptr.get() ||
                            !isValidFsmLvL(sptr->_lvl)))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      for (UINT32 i = 0; i < _capacity; ++i)
      {
         _bucketCandidate &c = _candidates[i];
         if (!c.isValid() || !isValidFsmLvL(c.getInfoPtr()->_lvl))
         {
            pos = i;
            break;
         }
         else if (pos < 0)
         {
            pos = i;
         }
         else if (_candidates[pos].getFailureCnt() < c.getFailureCnt())
         {
            pos = i;
         }
      }

      if (!_candidates[pos].isValid())
      {
         ++_size;
      }
      _candidates[pos].reset(seq, sptr);
   done:
      return rc;
   error:
      goto done;
   }

   void fsmCandidateBucket::dumpTuplesWithValidLvl(ossPoolList<freeSpaceTuple> &tuples)const
   {
      freeSpaceTuple tuple;
      for (UINT32 i = 0; i < _capacity; ++i)
      {
         const _bucketCandidate &c = _candidates[i];
         if (!c.isValid() || !isValidFsmLvL(c.getInfoPtr()->_lvl))
         {
            continue;
         }

         tuple.reset(c.getSeq(),
                     c.getInfoPtr()->_lpid,
                     c.getInfoPtr()->_lvl);
         tuples.push_back(tuple);
      }
      return;
   }

   INT32 fsmCandidateBucket::find(INT32 lvl,
                                  fsmCandidate &candidate)
   {
      INT32 rc = SDB_OK;
      UINT32 seed = 0;
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

      seed = _reqCnt++;
      for (UINT32 i = 0; i < _capacity; ++i)
      {
         UINT32 pos = ((seed + i) & (_capacity - 1));
         _bucketCandidate &c = _candidates[pos];

         if (!c.isValid())
         {
            continue;
         }
         else if (!isValidFsmLvL(c.getInfoPtr()->_lvl))
         {
            c.reset();
            --_size;
            continue;
         }
         else if (lvl <= c.getInfoPtr()->_lvl)
         {
            candidate.reset(c.getSeq(), c.getInfoPtr());
            break;
         }
         else
         {
            c.incAndGetFaulureCnt();
         }
      }
   done:
      return rc;
   error:
      goto done;
   }

}//namespace vessel
}//namespace engin