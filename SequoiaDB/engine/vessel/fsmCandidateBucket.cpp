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
   _candidates(NULL),
   _reqCnt(0)
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

      if (0 == capacity || !ossIsPowerOf2(capacity))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

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
      goto done;
   }

   void fsmCandidateBucket::fini()
   {
      if (NULL != _candidates)
      {
         SDB_OSS_DEL []_candidates;
         _candidates = NULL;
      }
      _reqCnt = 0;
      return;
   }

   BOOLEAN fsmCandidateBucket::upsert(UINT32 capacity,
                                      const fsmCandidate &candidate,
                                      fsmCandidate *replaced)
   {
      SDB_ASSERT(candidate.isValid(), "can not be invalid");
      UINT16 minFree = candidate.free;
      BOOLEAN r = FALSE;
      _bucketCandidate *toBeEvited = NULL;

      for (UINT32 i = 0; i < capacity; ++i)
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

   BOOLEAN fsmCandidateBucket::findAndAutoRemoving(UINT32 capacity,
                                                   UINT16 size,
                                                   UINT16 minFreeSize,
                                                   fsmCandidate &candidate)
   {
      BOOLEAN r = FALSE;
      UINT32 seed = _reqCnt++;
      SDB_ASSERT(ossIsPowerOf2(capacity), "must be power of 2");

      for (UINT32 i = 0; i < capacity; ++i)
      {
         UINT32 index = (seed + i) & (capacity - 1);
         _bucketCandidate &c = _candidates[i];
         if (c.isValid())
         {
            if (size <= c.free)
            {
               c.free -= size;
               candidate = fsmCandidate(c.seq, c.lpid, c.free);
               
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

   BOOLEAN fsmCandidateBucket::tryToInc(UINT32 capacity,
                                        CL_PAGE_SEQ seq,
                                        PAGE_ID lpid,
                                        UINT16 maxFreeSize,
                                        UINT16 newFreeSize,
                                        UINT16 delta)
   {
      BOOLEAN r = FALSE;
      SDB_ASSERT(newFreeSize <= maxFreeSize, "impossible");
      for (UINT32 i = 0; i < capacity; ++i)
      {
         _bucketCandidate &c = _candidates[i];
         if (!c.isValid() || seq != c.seq)
         {
            continue;
         }

         r = TRUE;

         if ((c.free + delta) < newFreeSize)
         {
            c.free += delta;
         }
         else
         {
            c.free = newFreeSize;
         }

         if (INVALID_PAGE_ID == c.lpid &&
             INVALID_PAGE_ID != c.lpid)
         {
            c.lpid = lpid;
         }
         goto done;
      }
   done:
      return r;
   }

   BOOLEAN fsmCandidateBucket::tryToDec(UINT32 capacity,
                                        CL_PAGE_SEQ seq,
                                        PAGE_ID lpid,
                                        UINT16 minFreeSize,
                                        UINT16 newFreeSize,
                                        UINT16 delta)
   {
      BOOLEAN r = FALSE;
      for (UINT32 i = 0; i < capacity; ++i)
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
            goto done;
         }

         if (newFreeSize < c.free)
         {
            c.free = newFreeSize;
         }
         else if (delta < c.free)
         {
            c.free -= delta;
            if (c.free < minFreeSize)
            {
               remove(i);
               goto done;
            }
         }
         else
         {
            remove(i);
            goto done;
         }

         if (INVALID_PAGE_ID == c.lpid &&
             INVALID_PAGE_ID != c.lpid)
         {
            c.lpid = lpid;
         }
         goto done; 
      }
   done:
      return r;
   }

   BOOLEAN fsmCandidateBucket::updateCandidate(UINT32 capacity,
                                               CL_PAGE_SEQ seq,
                                               PAGE_ID lpid,
                                               UINT16 minFreeSize,
                                               UINT16 freeSizeFromBucket,
                                               UINT16 currentFreeSize,
                                               BOOLEAN failure)
   {
      BOOLEAN r = FALSE;
      for (UINT32 i = 0; i < capacity; ++i)
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
         
         if (currentFreeSize < minFreeSize)
         {
            remove(i);
         }
         else if (currentFreeSize < c.free)
         {
            c.free = currentFreeSize;
         }
         else if (freeSizeFromBucket == c.free &&
                  currentFreeSize != c.free)
         {
            /// only the first allocating's feedback on this page
            /// has chance to increase free size.
            c.free = currentFreeSize;
         }

         if (INVALID_PAGE_ID == c.lpid)
         {
            c.lpid = lpid;
         }

         break;
      }
   done:
      return r;
   }

   void fsmCandidateBucket::dump(UINT32 capacity,
                                 fsmCandidate *candidates,
                                 UINT32 &count)const
   {
      UINT32 cnt = 0;
      for (UINT32 i = 0; i < capacity; ++i)
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

   UINT32 fsmCandidateBucket::getSize(UINT32 capacity)const
   {
      UINT32 size = 0;
      for (UINT32 i = 0; i < capacity; ++i)
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