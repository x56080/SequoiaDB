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

   Source File Name = fsmFile.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

*******************************************************************************/
#include "vessel/fsmFile.h"
#include "pdTrace.hpp"
#include "ossLikely.hpp"
#include "vessel/bitmapUtils.h"
#include "utilMemListPool.hpp"
#include "vessel/requestContext.h"

namespace engine
{
namespace vessel
{
   fsmFile::fsmFile()
   {
      SDB_ASSERT(FSM_BITMAP_OWNER_PAGE_SIZE == FSM_FILE_PAGE_SIZE, "must be same");
      SDB_ASSERT(FSM_BITMAP_PAGE_SIZE == FSM_FILE_PAGE_SIZE, "must be same");
   }

   INT32 fsmFile::_open(BOOLEAN isCreating)
   {
      if( isCreating )
      {
         ossValuePtr ptr = _getSmePtr();
         ossMemset( (void*)ptr, 0xFF, FSM_FILE_SME_USED_SIZE );
         ossMemset( (void*)(ptr + FSM_FILE_SME_USED_SIZE), 0x00, FSM_FILE_SME_ALIGNED_SIZE - FSM_FILE_SME_USED_SIZE );
         ptr = _getEntryArrayPtr();
         ossMemset( (void*)ptr, 0xFF, FSM_FILE_ENTRY_ARRAY_SIZE );
      }
      return SDB_OK;
   }

   INT32 fsmFile::initToWork()
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(storageFile::isOpen(), "must be open");
      ossValuePtr ptr = _getSmePtr();
      _smeScanner.load((UINT64 *)ptr, FSM_FILE_SME_CAPACITY );
      return rc;

   }

   INT32 fsmFile::allocateNewPage(PAGE_ID &pid)
   {
      INT32 rc = SDB_OK;
      std::unique_lock<std::mutex> lock(_latch);
      if (OSS_UNLIKELY(!isOpen()))
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }

      rc = _findFreePageFromSme(pid);
      if (SDB_OK != rc)
      {
         goto error;
      }

      SDB_ASSERT(INVALID_PAGE_ID != pid, "can not be invalid");

      rc = _ensureSpace(pid);
      if (SDB_OK != rc)
      {
         goto error;
      }
      
      rc = _allocateFreePageFromSme(pid);
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
      std::unique_lock<std::mutex> lock(_latch);
      if (OSS_UNLIKELY(!isOpen()))
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }
      else if (0 == count || NULL == pids)
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      rc = _releasePagesFromSme(count, pids);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to release pids on Sme:%d", rc);
         goto error;
      }
   done:
      return rc;
   error:
      goto done;
   }

   INT32 fsmFile::_findFreePageFromSme(PAGE_ID &pid)
   {
      INT32 rc = SDB_OK;
      pid = INVALID_PAGE_ID;
      INT32 nextFreePid = -1;
      if( !_smeScanner.moveToNextUnzeroPos(nextFreePid) )
      {
         rc = SDB_VESSEL_NOT_ENOUGH_FREE_RESOURCE;
         goto error;
      }
      pid = nextFreePid;
   done:
      return rc;
   error:
      goto done;
   }

   INT32 fsmFile::_allocateFreePageFromSme(PAGE_ID pid)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(INVALID_PAGE_ID != pid, "can not be invalid");
      const storageFileHead &head = getCommonHeadInMem();
      SDB_ASSERT(FSM_FILE_PAGE_SIZE == head.pageSize, "must be same");

      _smeScanner.clearBit(pid);

      rc = _fsyncSme();
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to fsync Sme:%d", rc);
         goto error;
      }

   done:
      return rc;
   error:
      goto done;
   }

   INT32 fsmFile::_releasePagesFromSme(UINT32 count, const PAGE_ID *pids)
   {
      
      INT32 rc = SDB_OK;
      SDB_ASSERT(0 < count && NULL != pids, "can not be invalid");
      const storageFileHead &head = getCommonHeadInMem();
      SDB_ASSERT(FSM_FILE_PAGE_SIZE == head.pageSize, "must be same");
      
      for (UINT32 i = 0; i < count; ++i)
      {
         PAGE_ID pid = pids[i];
         if (INVALID_PAGE_ID == pid || pid < 0 || pid >= FSM_FILE_SME_CAPACITY)
         {
            PD_LOG(PDERROR, "invalid pid to released");
            continue;
         }
         if(_smeScanner.testBit(pid)){
            PD_LOG(PDERROR, "pid[%d] is free in smp", pid);
            continue;
         }
         else _smeScanner.setBit(pid);
      }
      rc = _fsyncSme();
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to fsync Sme:%d", rc);
         goto error;
      }

   done:
      return rc;
   error:
      goto done;
   }

   INT32 fsmFile::_ensureSpace(PAGE_ID pid)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(INVALID_PAGE_ID != pid, "can not be invalid");

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

   ossValuePtr fsmFile::_getSmePtr()
   {
      ossValuePtr ptr = getReservedAreaPtr();
      SDB_ASSERT(ptr, "failed to get sme ptr, do not create reserved area");
      return ptr;
   }

   ossValuePtr fsmFile::_getEntryArrayPtr(){
      return _getSmePtr() + FSM_FILE_SME_ALIGNED_SIZE;
   }

   fsmCLEntry* fsmFile::getEntrySlotPtr(CL_MB_ID mbID)
   {

      return  reinterpret_cast<fsmCLEntry*>(_getEntryArrayPtr() + static_cast<UINT32>(mbID) * FSM_CL_ENTRY_SIZE);
   }

   UINT32 fsmFile::_getReservedAreaSize() const
   {
      return FSM_FILE_RESERVED_AREA_SIZE;
   }

   INT32 fsmFile::_fsyncSme()
   {
      INT32 rc = SDB_OK;
      INT32 mmapSegmentID=getReservedAreaMmapSegmentID();
      SDB_ASSERT( 0 < mmapSegmentID, "failed to get reserved area segment id when fsync sme");
      rc = flushBlock(mmapSegmentID, 0, FSM_FILE_SME_USED_SIZE, TRUE);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to fsync sme:%d", rc);
         goto error;
      }
   done:
      return rc;
   error:
      goto done;
   }

   INT32 fsmFile::fsyncEntry(CL_MB_ID mbID)
   {
      INT32 rc = SDB_OK;
      INT32 mmapSegmentID = getReservedAreaMmapSegmentID();
      SDB_ASSERT( 0 < mmapSegmentID, "failed to get reserved area segment id when fsync entry");
      UINT32 blockId = (UINT32)mbID / FSM_FILE_ENTRY_BLOCK_CAPACITY;
      UINT32 offset = FSM_FILE_SME_ALIGNED_SIZE + (blockId * FSM_FILE_ENTRY_BLOCK_SIZE);
      rc = flushBlock(mmapSegmentID, offset, FSM_FILE_ENTRY_BLOCK_SIZE, TRUE);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to fsync entry:%d", rc);
         goto error;
      }
   done:
      return rc;
   error:
      goto done;
   }

}//namespace vessel
}//namespace engine