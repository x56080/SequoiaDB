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

   Source File Name = fsmCandidateBucket.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

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
      if (NULL != _candidates)
      {
         SDB_OSS_DEL []_candidates;
         _candidates = NULL;
      }
      _reqCnt = 0;
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
      else if (OSS_UNLIKELY(INVALID_CL_PAGE_SEQ == seq ||
                            NULL == sptr.get() ||
                            !isValidFsmLvL(sptr->_lvl)))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      for (UINT32 i = 0; i < _capacity; ++i)
      {
         _bucketCandidate &c = _candidates[i];
         if (!c.isValid() ||
             !isValidFsmLvL(c.getInfoPtr()->_lvl))
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

   INT32 fsmCandidateBucket::resetWithNewCandidate(UINT32 pos,
                                                   FREE_SPACE_TUPLE_POOL &newPagePool,
                                                   BOOLEAN &inserted)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(pos < _capacity, "can not be out of bound");
      freeSpaceTuple tuple;
      inserted = FALSE;
      
      if (newPagePool.popForward(tuple))
      {
         SDB_ASSERT(tuple.isValid(), "impossible");
         SDB_ASSERT(isValidFsmLvL(tuple.getSpaceLvl()), "impossible");
         fsmCandidate::SHARED_INFO_PTR sptr;
         rc = makeFsmCandidateSharedInfoPtr(tuple.getLpid(),
                                            tuple.getSpaceLvl(),
                                            sptr);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to make shared ptr:%d", rc);
            goto error;
         }

         _candidates[pos].reset(tuple.getSeq(), sptr);
         inserted = TRUE;
      }

   done:
      return rc;
   error:
      if (tuple.isValid())
      {
         if (SDB_OK != newPagePool.pushForward(tuple))
         {
            PD_LOG(PDERROR, "failed to give back tuple[%d]", tuple.getSeq());
         }
      }
      goto done;
   }

   INT32 fsmCandidateBucket::find(INT32 lvl,
                                  FREE_SPACE_TUPLE_POOL &newPagePool,
                                  fsmCandidate &candidate)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(isValidFsmLvL(lvl), "can not be invalid");
      SDB_ASSERT(!candidate.isValid(), "can not be valid");
      INT32 toBeReset = -1;
      INT32 toBeEvicted = -1;
      BOOLEAN found = FALSE;
      UINT32 seed = 0;

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

         if (!c.isValid() || !isValidFsmLvL(c.getInfoPtr()->_lvl))
         {
            if (toBeReset < 0)
            {
               toBeReset = pos;
            }
         }
         else if (lvl <= c.getInfoPtr()->_lvl)
         {
            found = TRUE;
            candidate.reset(c.getSeq(), c.getInfoPtr());
            break;
         }
         else
         {
            UINT32 failureCount = c.incAndGetFaulureCnt();
            if (toBeEvicted < 0)
            {
               toBeEvicted = pos;
            }
            else if (_candidates[toBeEvicted].getFailureCnt() < failureCount)
            {
               toBeEvicted = pos;
            }
         }
      }

      if (0 == newPagePool.getSizeWithNoLock())
      {
         goto done;
      }

      if (found)
      {
         /// Always fill bucket with new page no matter if we found candidate.
         if (0 <= toBeReset)
         {
            BOOLEAN inserted = FALSE;
            resetWithNewCandidate(toBeReset, newPagePool, inserted);
         } 
      }
      else
      {
         INT32 resetPos = (toBeReset < 0) ? toBeEvicted : toBeReset;
         SDB_ASSERT(0 <= resetPos, "impossible");
         BOOLEAN inserted = FALSE;
         rc = resetWithNewCandidate((UINT32)resetPos, newPagePool, inserted);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to reset with new candidate:%d", rc);
            goto error;
         }
         else if (!inserted)
         {
            goto done;
         }

         candidate.reset(_candidates[resetPos].getSeq(),
                         _candidates[resetPos].getInfoPtr());
      }
   done:
      return rc;
   error:
      goto done;
   }

}//namespace vessel
}//namespace engin