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

   Source File Name = fclusterSpaceManager.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

*******************************************************************************/
#include "vessel/fclusterSpaceManager.h"
#include "vessel/storageFileCluster.h"
#include "vessel/baseMetaDataFile.h"
#include "pdTrace.hpp"
#include "vessel/strictBuffer.h"

#include <array>

namespace engine
{
namespace vessel
{
   INT32 fclusterSpaceManager::init(PAGE_ID smeEntryPid,
                                    baseMetaDataFile *mfile,
                                    storageFileCluster *fcluster,
                                    UINT32 minFreePcntReused)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(!isReady(), "do not reinit");
      variableExtentAllocator::options o;

      if (OSS_UNLIKELY(INVALID_PAGE_ID == smeEntryPid ||
                       nullptr == mfile ||
                       !mfile->isOpen() ||
                       nullptr == fcluster ||
                       !fcluster->isOpen()))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      _entryPid = smeEntryPid;
      _mfile = mfile;
      _fcluster = fcluster;
      o.maxPageCountPerSegment = fcluster->getCoreArgs().maxPageCountPerSeg;
      o.maxSegmentCountPerFile = fcluster->getCoreArgs().maxSegmentCountPerFile;
      o.minSegFreeCntReused = minFreePcntReused;
      SDB_ASSERT(o.minSegFreeCntReused <= o.maxPageCountPerSegment, "out of size");
      _allocator.init(o);
      rc = _loadFclusterSme();
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to load smes from file:%d", rc);
         goto error;
      }
   done:
      return rc;
   error:
      fini();
      goto done;
   }

   void fclusterSpaceManager::fini()
   {
      _allocator.reset();
      _entryPid = INVALID_PAGE_ID;
      _mfile = nullptr;
      _fcluster = nullptr;
      return;
   }

   INT32 fclusterSpaceManager::reserve(PAGE_ID &pid)
   {
      return reserveExtent(1, pid);
   }

   INT32 fclusterSpaceManager::reserveExtent(UINT32 pcnt, PAGE_ID &pid)
   {
      INT32 rc = SDB_OK;
      pid = INVALID_PAGE_ID;
      
      if (OSS_UNLIKELY(!isReady()))
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }

      do
      {
         UINT32 currentSegCount = 0;
         std::unique_lock<std::mutex> guard(_mutex, std::defer_lock);
         rc = _allocator.reserveExtent(pcnt, pid, &currentSegCount);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to reserve extent from allocator:%d", rc);
            goto error;
         }
         else if (INVALID_PAGE_ID != pid)
         {
            break;
         }
         else
         {
            guard.lock();
            if (currentSegCount == _allocator.peekSegmentCount())
            {
               rc = _extendNewDataSegment();
               if (SDB_OK != rc)
               {
                  PD_LOG(PDERROR, "failed to extend new data segment:%d", rc);
                  goto error;
               }
            }
         }
      } while (TRUE);
      
   done:
      return rc;
   error:
      goto done;
   }

   void fclusterSpaceManager::release(PAGE_ID pid)
   {
      return releaseExtent(pid, 1);
   }

   void fclusterSpaceManager::releaseExtent(PAGE_ID pid, UINT32 pcnt)
   {
      if (isReady())
      {
         _allocator.freeExtent(pid, pcnt);
      }

      return;
   }

   void fclusterSpaceManager::releaseBatch(UINT32 size, const PAGE_ID *pids)
   {
      SDB_ASSERT(nullptr != pids, "can not be invalid");

      if (isReady())
      {
         _allocator.freePids(size, pids);
      }

      return;
   }

   void fclusterSpaceManager::releaseBatch(const sparseBitmap32 &bm)
   {
      if (isReady())
      {
         _allocator.freePids(bm);
      }
      return;
   }

   INT32 fclusterSpaceManager::_extendNewDataSegment()
   {
      INT32 rc = SDB_OK;
      strictBuffer smeBuffer;
      UINT32 currentSegments = _allocator.peekSegmentCount();

      /// always extend sme first
      rc = _ensureSegmentSmeBuffer(currentSegments, smeBuffer);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to ensure sme buffer of segment[%d]:%d",
                currentSegments, rc);
         goto error;
      }

      SDB_ASSERT(smeBuffer.isWritable(), "impossible");
      smeBuffer.setBuffer(0xFF);

      if (_fcluster->getTotalSegmentCount() == currentSegments)
      {
         rc = _fcluster->allocateNewSegment(1);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to extend new fcluster segment:%d", rc);
            goto error;
         }
      }

      ///WARNING: we will init smgr by data file segment count when start.
      /// sme must be rebuild if engine crashed.
      rc = _allocator.depositWithSme((UINT64 *)smeBuffer.getWPtr());
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to deposit into allocator:%d", rc);
         goto error;
      }

      SDB_ASSERT((currentSegments + 1) == _allocator.peekSegmentCount(), "must be same");
   done:
      return rc;
   error:
      goto done;
   }

   INT32 fclusterSpaceManager::_loadFclusterSme()
   {
      INT32 rc = SDB_OK;
      UINT32 totalSegments = _fcluster->getTotalSegmentCount();

      for (UINT32 i = 0; i < totalSegments; ++i)
      {
         strictBuffer buffer;
         rc = _getSegmentSmeBuffer(i, buffer);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to get sme ptr of segment[%d], rc:%d", i, rc);
            goto error;
         }

         rc = _allocator.depositWithSme((UINT64 *)buffer.getWPtr());
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to init allocator:%d", rc);
            goto error;
         }
      }

      SDB_ASSERT(_allocator.peekSegmentCount() == totalSegments, "must be same");
   done:
      return rc;
   error:
      goto done;
   }

   INT32 fclusterSpaceManager::_getSegmentSmeBuffer(UINT32 segmentId,
                                                    strictBuffer &buffer)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(INVALID_PAGE_ID != _entryPid, "can not be invalid");
      buffer.reset();

      UINT32 smeBufferSize = _fcluster->getCoreArgs().maxPageCountPerSeg >> 3;
      UINT32 smeBufferOffset = (segmentId * smeBufferSize) % _mfile->getCommonHeadInMem().pageSize;
      UINT32 smePageCapacity = _mfile->getCommonHeadInMem().pageSize / smeBufferSize;
      UINT32 pageNo = segmentId / smePageCapacity;
      UINT32 entryPageCapacity = _mfile->getCommonHeadInMem().pageSize / sizeof(PAGE_ID);
      SDB_ASSERT(pageNo < entryPageCapacity, "out of bound");

      const PAGE_ID *smePid = nullptr;
      strictBuffer entryBuffer, smePageBuffer;
      rc = _mfile->makeReadableBuffer(_entryPid, entryBuffer);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to get entry buffer[%d], rc:%d",_entryPid, rc);
         goto error;
      }

      smePid = entryBuffer.getReadableObjPtr<PAGE_ID>(pageNo << 2);
      if (OSS_UNLIKELY(nullptr == smePid))
      {
         PD_LOG(PDERROR, "failed to get ptr of page no[%d]", pageNo);
         rc = SDB_VESSEL_INTERNAL_ERR;
         goto error;
      }
      else if (INVALID_PAGE_ID == *smePid)
      {
         PD_LOG(PDERROR, "sme page of pageno[%d] not inited", pageNo);
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }

      rc = _mfile->makeWritableBuffer(*smePid, smePageBuffer);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to get writable buffer of pid[%d], rc:%d", *smePid, rc);
         goto error;
      }

      buffer = smePageBuffer.getWritableBuffer(smeBufferSize, smeBufferOffset);
      if (!buffer.isWritable())
      {
         PD_LOG(PDERROR, "failed to get buffer[%d, %d]", smeBufferSize, smeBufferOffset);
         rc = SDB_VESSEL_INTERNAL_ERR;
         goto error;
      }
   done:
      return rc;
   error:
      goto done;
   }

   INT32 fclusterSpaceManager::_ensureSegmentSmeBuffer(UINT32 segmentId,
                                                       strictBuffer &buffer)
   {
      INT32 rc = SDB_OK;
      buffer.reset();

      UINT32 smeBufferSize = _fcluster->getCoreArgs().maxPageCountPerSeg >> 3;
      UINT32 smePageCapacity = _mfile->getCommonHeadInMem().pageSize / smeBufferSize;
      UINT32 pageNo = segmentId / smePageCapacity;
      UINT32 smeBufferOffset = (segmentId % smePageCapacity) * smeBufferSize;

      strictBuffer smePageBuffer;

      PAGE_ID smePid = INVALID_PAGE_ID;
      rc = _ensureSmePage(pageNo, smePid);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to ensure sme page:%d", rc);
         goto error;
      }
      
      rc = _mfile->makeWritableBuffer(smePid, smePageBuffer);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to get sme page buffer[%d], rc:%d", smePid, rc);
         goto error;
      }

      buffer = smePageBuffer.getWritableBuffer(smeBufferSize, smeBufferOffset);
      if (OSS_UNLIKELY(!buffer.isWritable()))
      {
         PD_LOG(PDERROR, "failed to get sme buffer of segment[%d]", segmentId);
         rc = SDB_VESSEL_INTERNAL_ERR;
         goto error;
      }
   done:
      return rc;
   error:
      goto done;
   }

   INT32 fclusterSpaceManager::_ensureSmePage(UINT32 pageNo, PAGE_ID &pid)
   {
      INT32 rc = SDB_OK;
      strictBuffer entryBuffer;
      PAGE_ID *smePid = nullptr;
      PAGE_ID newPid = INVALID_PAGE_ID;

      pid = INVALID_PAGE_ID;

      rc = _mfile->makeWritableBuffer(_entryPid, entryBuffer);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to get entry buffer[%d], rc:%d", _entryPid, rc);
         goto error;
      }

      static_assert(4 == sizeof(PAGE_ID), "must be 4");
      smePid = entryBuffer.getWritableObjPtr<PAGE_ID>(pageNo << 2);
      if (OSS_UNLIKELY(nullptr == smePid))
      {
         PD_LOG(PDERROR, "failed to ptr of page no[%d]", pageNo);
         rc = SDB_VESSEL_INTERNAL_ERR;
         goto error;
      }

      if (INVALID_PAGE_ID == *smePid)
      {
         rc = _mfile->reservePid(newPid);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to reserve pid:%d", rc);
            goto error;
         }

         *smePid = newPid;
      }

      pid = *smePid;

   done:
      return rc;
   error:
      goto done;
   }

   BOOLEAN fclusterSpaceManager::test(PAGE_ID pid)
   {
      return _allocator.test(pid);
   }

} // namespace vessel

} // namespace engine
