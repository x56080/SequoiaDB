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
   const UINT32 MAX_FAILURE_COUNT = 16;

   fsmCandidateBucket::fsmCandidateBucket():
   _reqCnt(0)
   {

   }

   fsmCandidateBucket::~fsmCandidateBucket()
   {

   }

   void fsmCandidateBucket::upsert(ossSpinLatch *latch,
                                   const fsmCandidate &candidate,
                                   BOOLEAN setFillBack,
                                   fsmCandidate *replaced)
   {
      SDB_ASSERT(candidate.isValid(), "can not be invalid");
      UINT32 toBeEvited = 0;
      UINT16 minFree = 0xFFFF;

      if (NULL != replaced)
      {
         replaced->reset();
      }
      
      ossSpinGuard guard(latch);

      for (UINT32 i = 0; i < FSM_CANDIDATE_BUCKET_CAPACITY; ++i)
      {
         fsmCandidate &c = _candidates[i];
         if (!c.isValid())
         {
            c = fsmCandidate(candidate.seq, candidate.lpid,
                             candidate.free);
            if (setFillBack)
            {
               c.setFillBackFlag();
            }
            if (NULL != replaced)
            {
               replaced->reset();
            }
            goto done;
         }
         else if (c.free < minFree)
         {
            toBeEvited = i;
            minFree = _candidates[i].free;
         }
      }

      if (NULL != replaced)
      {
         *replaced = _candidates[toBeEvited];
      }

      _candidates[toBeEvited] = fsmCandidate(candidate.seq,
                                             candidate.lpid,
                                             candidate.free);
      if (setFillBack)
      {
         _candidates[toBeEvited].setFillBackFlag();
      }
   done:
      return;
   }

   BOOLEAN fsmCandidateBucket::findAndAutoRemoving(ossSpinLatch *latch,
                                                   UINT16 needSize,
                                                   UINT16 minFreeSize,
                                                   fsmCandidate &candidate)
   {
      BOOLEAN r = FALSE;
      INT32 where = -1;
      ossSpinGuard guard(latch);
      UINT32 seed = _reqCnt++;

      for (UINT32 i = 0; i < FSM_CANDIDATE_BUCKET_CAPACITY; ++i)
      {
         UINT32 index = (seed + i) & 0x3;/// mod 4
         fsmCandidate &c = _candidates[i];
         if (c.isValid())
         {
            if (needSize <= c.free)
            {
               candidate = c;
               c.free -= needSize;
               if (c.free < minFreeSize)
               {
                  remove(index);
               }
               c.clearFillBackFlag();
               r = TRUE;
               break;
            }
            else if (c.incAndGetFaulureCnt() == MAX_FAILURE_COUNT)
            {
               remove(index);
            }
         }
      }

   done:
      return r;
   }

   void fsmCandidateBucket::guaranteeNotFull(ossSpinLatch *latch)
   {
      UINT32 toBeEvited = 0;
      UINT16 minFree = 0xFFFF; 
      ossSpinGuard guard(latch);
      for (UINT32 i = 0; i < FSM_CANDIDATE_BUCKET_CAPACITY; ++i)
      {
         fsmCandidate &c = _candidates[i];
         if (!c.isValid())
         {
            goto done;
         }
         else if (c.free < minFree)
         {
            toBeEvited = i;
            minFree = c.free;
         }
      }

      remove(toBeEvited);
   done:
      return;
   }

   UINT32 fsmCandidateBucket::getSize(ossSpinLatch *latch)const
   {
      UINT32 size = 0;
      ossSpinGuard guard(latch);
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