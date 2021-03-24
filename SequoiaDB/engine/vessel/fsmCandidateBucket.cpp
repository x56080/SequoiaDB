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
   const UINT32 MAX_FAILURE_COUNT = 8;

   fsmCandidateBucket::fsmCandidateBucket():
   _reqCnt(0)
   {

   }

   fsmCandidateBucket::~fsmCandidateBucket()
   {

   }

   BOOLEAN fsmCandidateBucket::upsert(const fsmCandidate &candidate,
                                      fsmCandidate *replaced)
   {
      SDB_ASSERT(candidate.isValid(), "can not be invalid");
      UINT16 minFree = candidate.free;
      BOOLEAN r = FALSE;
      _bucketCandidate *toBeEvited = NULL;

      for (UINT32 i = 0; i < FSM_CANDIDATE_BUCKET_CAPACITY; ++i)
      {
         _bucketCandidate &c = _candidates[i];
         if (!c.isValid())
         {
            c.reset(candidate.seq,
                    candidate.lpid,
                    candidate.free);
            r = TRUE;
            if (NULL != replaced)
            {
               replaced->reset();
            }
            goto done;
         }
         else if (c.free < minFree)
         {
            toBeEvited = &c;
            minFree = c.free;
         }
      }

      if (NULL == toBeEvited)
      {
         goto done;
      }

      if (NULL != replaced)
      {
         replaced->reset();
         replaced->seq = toBeEvited->seq;
         replaced->lpid = toBeEvited->lpid;
         replaced->free = toBeEvited->free;
      }

      toBeEvited->reset(candidate.seq,
                       candidate.lpid,
                       candidate.free);
      r = TRUE;
   done:
      return r;
   }

   BOOLEAN fsmCandidateBucket::findAndAutoRemoving(UINT16 size,
                                                   UINT16 minFreeSize,
                                                   fsmCandidate &candidate)
   {
      BOOLEAN r = FALSE;
      UINT32 seed = _reqCnt++;
      SDB_ASSERT(4 == FSM_CANDIDATE_BUCKET_CAPACITY, "impossible");

      for (UINT32 i = 0; i < FSM_CANDIDATE_BUCKET_CAPACITY; ++i)
      {
         UINT32 index = (seed + i) & 0x3;/// mod 4
         _bucketCandidate &c = _candidates[i];
         if (c.isValid())
         {
            if (size <= c.free)
            {
               candidate = fsmCandidate(c.seq, c.lpid, c.free);
               c.free -= size;
               if (c.free < minFreeSize)
               {
                  remove(index);
               }
               r = TRUE;
               break;
            }
            else if (c.incAndGetFaulureCnt() == MAX_FAILURE_COUNT)
            {
               remove(index);
            }
         }
      }

      return r;
   }

   BOOLEAN fsmCandidateBucket::tryToInc(CL_PAGE_SEQ seq,
                                        PAGE_ID lpid,
                                        UINT16 maxFreeSize,
                                        UINT16 newFreeSize,
                                        UINT16 delta)
   {
      BOOLEAN r = FALSE;
      for (UINT32 i = 0; i < FSM_CANDIDATE_BUCKET_CAPACITY; ++i)
      {
         _bucketCandidate &c = _candidates[i];
         if (!c.isValid() || seq != c.seq)
         {
            continue;
         }

         r = TRUE;

         if (maxFreeSize < (c.free + delta))
         {
            c.free = newFreeSize < maxFreeSize ? newFreeSize : maxFreeSize;
         }
         else
         {
            c.free += maxFreeSize;
         }

         if (INVALID_PAGE_ID == c.lpid &&
             INVALID_PAGE_ID != c.lpid)
         {
            c.lpid = lpid;
         }
      }
   done:
      return r;
   }

   BOOLEAN fsmCandidateBucket::tryToDec(CL_PAGE_SEQ seq,
                                        PAGE_ID lpid,
                                        UINT16 minFreeSize,
                                        UINT16 newFreeSize,
                                        UINT16 delta)
   {
      BOOLEAN r = FALSE;
      for (UINT32 i = 0; i < FSM_CANDIDATE_BUCKET_CAPACITY; ++i)
      {
         _bucketCandidate &c = _candidates[i];
         if (!c.isValid() || seq != c.seq)
         {
            continue;
         }

         r = TRUE;

         if (newFreeSize < minFreeSize)
         {
            remove(i);
         }
         else if (delta < c.free)
         {
            c.free -= delta;
            if (c.free < minFreeSize)
            {
               remove(i);
            }
            else if (INVALID_PAGE_ID == c.lpid &&
                     INVALID_PAGE_ID != c.lpid)
            {
               c.lpid = lpid;
            }
         }
         else
         {
            c.free = newFreeSize;
            if (INVALID_PAGE_ID == c.lpid &&
                     INVALID_PAGE_ID != c.lpid)
            {
               c.lpid = lpid;
            }
         }         
      }
   done:
      return r;
   }

   BOOLEAN fsmCandidateBucket::fillback(CL_PAGE_SEQ seq,
                                        PAGE_ID lpid,
                                        BOOLEAN failure)
   {
      BOOLEAN r = FALSE;
      for (UINT32 i = 0; i < FSM_CANDIDATE_BUCKET_CAPACITY; ++i)
      {
         _bucketCandidate &c = _candidates[i];
         if (!c.isValid() || seq != c.seq)
         {
            continue;
         }

         r = TRUE;

         if (failure)
         {
            if (MAX_FAILURE_COUNT <= c.incAndGetFaulureCnt())
            {
               remove(i);
               goto done;
            }
         }

         if (INVALID_PAGE_ID == c.lpid)
         {
            c.lpid = lpid;
         }
      }
   done:
      return r;
   }

   void fsmCandidateBucket::dump(fsmCandidate *candidates,
                                 UINT32 &count)const
   {
      UINT32 cnt = 0;
      for (UINT32 i = 0; i < FSM_CANDIDATE_BUCKET_CAPACITY; ++i)
      {
         const _bucketCandidate &c = _candidates[i];
         if (!c.isValid())
         {
            continue;
         }

         candidates[cnt++] = fsmCandidate(c.seq, c.lpid, c.free);
      }
      count = cnt;
      return;
   }

   UINT32 fsmCandidateBucket::getSize()const
   {
      UINT32 size = 0;
      for (UINT32 i = 0; i < FSM_CANDIDATE_BUCKET_CAPACITY; ++i)
      {
         if (_candidates[i].isValid())
         {
            ++size;
         }
      }
      return size;
   }
}//namespace vessel
}//namespace engine