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

   Source File Name = fsmFile.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#include "vessel/fsmFile.h"
#include "pdTrace.hpp"
#include "ossLikely.hpp"
#include "vessel/bitMapUtils.h"
#include "vessel/freeSpaceMapDef.h"
#include "utilMemListPool.hpp"
#include "vessel/requestContext.h"

namespace engine
{
namespace vessel
{

   fsmFile::fsmFile():
   _firstFree(-1)
   {

   }

   fsmFile::~fsmFile()
   {
      
   }

   INT32 fsmFile::initAfterCreation()
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(isOpen(), "can not be closed");
      const storageFileHead &head = getCommonHeadInMem();
      SDB_ASSERT(FSM_PAGE_SIZE == head.pageSize, "must be same");
      ossValuePtr ptr = 0;
      UINT32 offset = 0;
      UINT32 totalBitsCount = FSM_PAGE_SIZE >> 3; /// divided by 8

      rc = ensureSegmentCount(1);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to allocate new segment:%d", rc);
         goto error;
      }

      rc = getPagePtr(FSM_SMP_PID, ptr);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to get page ptr:%d", rc);
         goto error;
      }

      ossMemset((CHAR *)ptr, 0xFF, FSM_PAGE_SIZE);

      /// first page is smp, can not be allocated.
      if (!findAndClearFirstFreeBitFromBit64(totalBitsCount, -1, (UINT64*)ptr, offset))
      {
         PD_LOG(PDERROR, "failed to allocate space for smp");
         rc = SDB_VESSEL_INTERNAL_ERR;
         goto error;
      }

      /// entry page can not be allocated.
      for (UINT32 i = 0; i < FSM_ENTRY_PAGE_COUNT; ++i)
      {
         ossValuePtr tmpPtr = 0;
         if (!findAndClearFirstFreeBitFromBit64(totalBitsCount, -1, (UINT64*)ptr, offset))
         {
            PD_LOG(PDERROR, "failed to allocate space for entry page");
            rc = SDB_VESSEL_INTERNAL_ERR;
            goto error;
         }

         SDB_ASSERT(offset == i + 1, "impossible");
         rc = getPagePtr(offset, tmpPtr);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to get ptr of page[%d], rc:%d", offset, rc);
            goto error;
         }

         ossMemset((CHAR *)tmpPtr, 0xFF, FSM_PAGE_SIZE);
      }

      fsync(FSM_SMP_PID, 1 + FSM_ENTRY_PAGE_COUNT, TRUE);
      _firstFree = 0;
   done:
      return rc;
   error:
      goto done;
   }

   INT32 fsmFile::initAfterOpen()
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(isOpen(), "can not be closed");
      ossValuePtr ptr = 0;
      UINT32 offset = 0;
      UINT32 totalBitsCount = FSM_PAGE_SIZE >> 3; /// divided by 8
      UINT32 minFreePid = FSM_ENTRY_PAGE_COUNT + 1; /// 1 for smp
      UINT32 nextFreePid = INVALID_PAGE_ID;
      UINT32 currentSegCount = 0;

      currentSegCount = getSegmentCount();

      if (0 == currentSegCount)
      {
         PD_LOG(PDERROR, "fsm file has no segment, reinit fsm");
         /// reinit file
         rc = initAfterCreation();
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to reinit fsm file:%d", rc);
            goto error;
         }
         else
         {
            goto done;
         }
      }

      rc = getPagePtr(FSM_SMP_PID, ptr);
      if (OSS_UNLIKELY(SDB_OK != rc))
      {
         PD_LOG(PDERROR, "failed to get page ptr:%d", rc);
         goto error;
      }

      if (findFirstFreeBitFromBit64(totalBitsCount, -1, (const UINT64 *)ptr, nextFreePid))
      {
         if (nextFreePid < minFreePid)
         {
            PD_LOG(PDSEVERE, "invalid first free pid was found, smp may crashed");
            rc = SDB_VESSEL_PAGE_CRASHED;
            goto error;
         }
         else
         {
            _firstFree = nextFreePid >> 6; /// divided by 64
         }
      }
   done:
      return rc;
   error:
      goto done;
   }

   INT32 fsmFile::allocateNewPage(PAGE_ID &pid)
   {
      INT32 rc = SDB_OK;
      ossScopedLock lock(&_latch);
      if (OSS_UNLIKELY(!isOpen()))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      rc = findFreePageFromSmp(pid);
      if (SDB_OK != rc)
      {
         goto error;
      }

      SDB_ASSERT(INVALID_PAGE_ID != pid, "can not be invalid");

      rc = ensureSpace(pid);
      if (SDB_OK != rc)
      {
         goto error;
      }

      rc = allocateFreePageFromSmp(pid);
      if (SDB_OK != rc)
      {
         goto error;
      }

   done:
      return rc;
   error:
      goto done;
   }

   INT32 fsmFile::releasePages(UINT32 count, const PAGE_ID *pids)
   {
      INT32 rc = SDB_OK;
      ossScopedLock lock(&_latch);
      if (OSS_UNLIKELY(!isOpen()))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      else if (0 == count || NULL == pids)
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
   done:
      return rc;
   error:
      goto done;
   }

   INT32 fsmFile::findFreePageFromSmp(PAGE_ID &pid)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(isOpen(), "can not be closed");
      const storageFileHead &head = getCommonHeadInMem();
      UINT32 totalCount = FSM_PAGE_SIZE >> 3;// dividec by 8
      PAGE_ID minFreePid = FSM_ENTRY_PAGE_COUNT + 1;/// 1 smp + all entry pages.

      pid = INVALID_PAGE_ID;
      ossValuePtr ptr = 0;
      UINT32 offset = 0;
      if (_firstFree < 0)
      {
         rc = SDB_VESSEL_SMP_NO_FREE;
         goto error;
      }

      rc = getPagePtr(FSM_SMP_PID, ptr);
      if (OSS_UNLIKELY(SDB_OK != rc))
      {
         PD_LOG(PDERROR, "failed to get smp ptr:%d", rc);
         goto error;
      }

      if (!findFirstFreeBitFromBit64(totalCount, _firstFree, (const UINT64 *)ptr, offset))
      {
         PD_LOG(PDERROR, "failed to find free page from fsm");
         rc = SDB_VESSEL_INTERNAL_ERR;
         goto error;
      }

      if (OSS_UNLIKELY(offset < minFreePid))
      {
         PD_LOG(PDERROR, "smp pid or entry pids should not be found in fsm");
         rc = SDB_VESSEL_INTERNAL_ERR;
         goto error;
      }
      
      pid = offset;
   done:
      return rc;
   error:
      goto done;
   }

   INT32 fsmFile::allocateFreePageFromSmp(PAGE_ID pid)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(isOpen(), "can not be closed");
      SDB_ASSERT(INVALID_PAGE_ID != pid, "can not be invalid");
      const storageFileHead &head = getCommonHeadInMem();
      SDB_ASSERT(FSM_PAGE_SIZE == head.pageSize, "must be same");
      UINT32 totalCount = FSM_PAGE_SIZE >> 3; /// divide by 8
      ossValuePtr ptr = 0;
      UINT32 nextFree = -1;

      rc = getPagePtr(FSM_SMP_PID, ptr);
      if (OSS_UNLIKELY(SDB_OK != rc))
      {
         PD_LOG(PDERROR, "failed to get smp ptr:%d", rc);
         goto error;
      }

      if (!setNotFreeIfFree64(totalCount, (UINT64 *)ptr, pid))
      {
         PD_LOG(PDERROR, "failed to update smp when clear pid[%d]", pid);
         rc = SDB_VESSEL_INTERNAL_ERR;
         goto error;
      }

      fsync(FSM_SMP_PID, 1, TRUE);

      if (findFirstFreeBitFromBit64(totalCount, _firstFree, (const UINT64 *)ptr, nextFree))
      {
         _firstFree = nextFree >> 6; /// divided by 64
      }
      else
      {
         _firstFree = -1;
      }
   done:
      return rc;
   error:
      goto done;
   }

   INT32 fsmFile::releasePagesFromSmp(UINT32 count, const PAGE_ID *pids)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(isOpen(), "can not be closed");
      SDB_ASSERT(0 < count && NULL != pids, "can not be invalid");
      const storageFileHead &head = getCommonHeadInMem();
      SDB_ASSERT(FSM_PAGE_SIZE == head.pageSize, "must be same");
      UINT32 totalCount = FSM_PAGE_SIZE >> 3; /// divide by 8
      ossValuePtr ptr = 0;
      PAGE_ID minPid = INVALID_PAGE_ID;
      INT32 minFree = -1;

      rc = getPagePtr(FSM_SMP_PID, ptr);
      if (OSS_UNLIKELY(SDB_OK != rc))
      {
         PD_LOG(PDERROR, "failed to get smp ptr:%d", rc);
         goto error;
      }

      for (UINT32 i = 0; i < count; ++i)
      {
         PAGE_ID pid = pids[i];
         if (INVALID_PAGE_ID == pid)
         {
            PD_LOG(PDERROR, "invalid pid to released");
            continue;
         }

         if (!setFreeIfNotFree64(totalCount, (UINT64 *)ptr, pid))
         {
            PD_LOG(PDERROR, "pid[%d] is not free in smp", pid);
            continue;
         }

         if (INVALID_PAGE_ID == minPid)
         {
            minPid = pid;
         }
         else if (pid < minPid)
         {
            minPid = pid;
         }
      }

      if (INVALID_PAGE_ID == minPid)
      {
         PD_LOG(PDERROR, "no page was released");
         rc = SDB_VESSEL_INTERNAL_ERR;
         goto error;
      }

      minFree = minPid >> 6; /// divided by 64
      if (-1 == _firstFree)
      {
         _firstFree = minFree;
      }
      else if (0 <= minFree && minFree < _firstFree)
      {
         _firstFree = minFree;
      }
      fsync(FSM_SMP_PID, 1, TRUE);
   done:
      return rc;
   error:
      goto done;
   }

   INT32 fsmFile::ensureSpace(PAGE_ID pid)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(INVALID_PAGE_ID != pid, "can not be invalid");
      SDB_ASSERT(isOpen(), "can not be closed");

      const storageFileHead &head = getCommonHeadInMem();
      UINT32 segCount = pid / head.maxPageCountPerSeg + 1;
      rc = ensureSegmentCount(segCount);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to sure data segment count[%d], rc:%d", segCount, rc);
         goto error;
      } 
   done:
      return rc;
   error:
      goto done;
   }
}//namespace vessel
}//namespace engine