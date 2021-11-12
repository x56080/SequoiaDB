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

   Source File Name = dataPageCluster.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#include "vessel/dataPageCluster.h"
#include "vessel/storageFileCreater.h"
#include "pdTrace.hpp"
#include "ossLatchGuard.hpp"

namespace engine
{
namespace vessel
{
   dataPageCluster::dataPageCluster()
   {}

   dataPageCluster::~dataPageCluster()
   {
      _close();
   }

   INT32 dataPageCluster::open(const storageCoreArgs &args,
                               const storageFileCreater *creater,
                               const storageFileLoader *loader,
                               UINT32 freeBound)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(!isOpen(), "do not reopen");
      inMemBitmap::options o;
      o.bitmapBeginPage = 0;

      if (OSS_UNLIKELY(!args.isValid() ||
                       NULL == creater ||
                       !creater->isValid() ||
                       args.maxPageCountPerSeg < freeBound))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      _args = args;
      _creater = creater;

      rc = _allocator.init(_args.maxPageCountPerSeg, o);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to init page allocator:%d", rc);
         goto error;
      }

      rc = openFiles(loader);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to open files:%d", rc);
         goto error;
      }

      _segmentCountOnDisk = getTotalSegmentCountAllocated();
      if (0 < _segmentCountOnDisk)
      {
         rc = _allocator.allocateNewBitmapPages(_segmentCountOnDisk);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to allocate bitmap pages:%d", rc);
            goto error;
         }
      }
   done:
      return rc;
   error:
      close();
      goto done;
   }

   void dataPageCluster::close()
   {
      closeFiles();
      _close();
      return;
   }

   void dataPageCluster::destroy()
   {
      destroyFiles();
      _close();
      return;
   }

   void dataPageCluster::_close()
   {
      _args.reset();
      _creater = NULL;
      _allocator.fini();
      _segmentCountOnDisk = 0;
      return;
   }

   INT32 dataPageCluster::allocatePage(PAGE_ID &pid)
   {
      return allocatePages(1, &pid);
   }

   INT32 dataPageCluster::allocatePages(UINT32 count,
                                        PAGE_ID *pids)
   {
      INT32 rc = SDB_OK;
      BOOLEAN rollback = FALSE;

      if (OSS_UNLIKELY(!isOpen()))
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }
      else if (OSS_UNLIKELY(0 == count ||
                            NULL == pids))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      do
      {
         UINT32 oldCount = _allocator.getPageCount();
         rc = _allocator.allocateBits(count, pids);
         if (SDB_OK == rc)
         {
            break;
         }
         else if (SDB_VESSEL_NOT_ENOUGH_FREE_RESOURCE != rc)
         {
            PD_LOG(PDERROR, "failed to allocate from bitmap:%d", rc);
            goto error;
         }
         else
         {
            rc = extendPageSpace(1, &oldCount);
            if (SDB_OK != rc)
            {
               PD_LOG(PDERROR, "failed to extend page space:%d", rc);
               goto error;
            }
         }
      } while (TRUE);

      rollback = TRUE;

      if (mayBeSparse() && hasSparseFile())
      {
         /// Files may be sparse.
         rc = ensureAllPagesNotSparse(count, pids);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to ensure all pages not sparse:%d", rc);
            goto error;
         }
      }
      
      rollback = FALSE;
      
   done:
      return rc;
   error:
      if (rollback)
      {
         releasePages(count, pids);
      }
      goto done;
   }

   INT32 dataPageCluster::occupyPage(PAGE_ID pid)
   {
      return occupyPages(1, &pid);
   }

   INT32 dataPageCluster::occupyPages(UINT32 count, const PAGE_ID *pids)
   {
      INT32 rc = SDB_OK;
      BOOLEAN rollback = FALSE;

      if (OSS_UNLIKELY(!isOpen()))
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }
      else if (OSS_UNLIKELY(0 == count || NULL == pids))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      rc = _allocator.occupy(count, pids);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to occupy pages:%d", rc);
         goto error;
      }

      if (mayBeSparse() && hasSparseFile())
      {
         rc = ensureAllPagesNotSparse(count, pids);
         if (SDB_OK != rc)
         {
            rollback = TRUE;
            PD_LOG(PDERROR, "failed to ensure all pages not sparse:%d", rc);
            goto error;
         }
      }
   done:
      return rc;
   error:
      if (rollback)
      {
         releasePages(count, pids);
      }
      goto done;
   }

   void dataPageCluster::releasePages(UINT32 count, const PAGE_ID *pids)
   {
      if (OSS_UNLIKELY(!isOpen()))
      {
         SDB_ASSERT(FALSE, "must be open");
         goto done;
      }
      else if (OSS_UNLIKELY(0 == count ||
                            NULL == pids))
      {
         SDB_ASSERT(FALSE, "invalid args");
         goto done;
      }

      _allocator.releaseBits(count, pids);

   done:
      return;
   }

   void dataPageCluster::releasePage(PAGE_ID pid)
   {
      return releasePages(1, &pid);
   }

   INT32 dataPageCluster::ensureAllPagesNotSparse(UINT32 count,
                                                  const PAGE_ID *pids)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(isOpen(), "can not be closed");
      SDB_ASSERT(0 < count, "can not be zero");
      SDB_ASSERT(NULL != pids, "can not be null");
      INT32 preSegmentId = -1;

      for (UINT32 i = 0; i < count; ++i)
      {
         BOOLEAN isSparse = FALSE;
         PAGE_ID pid = pids[i];
         SDB_ASSERT(INVALID_PAGE_ID != pid, "can not be invalid");
         
         /// If segment is sparse, all pages in segment must be free.
         /// But we can not suppose the first page of segment always
         /// be allocated first in allocating which pids are specified.
         UINT32 segmentId = pid / _args.maxPageCountPerSeg;
         if ((INT32)segmentId == preSegmentId)
         {
            continue;
         }

         preSegmentId = (INT32)segmentId;

         if (_segmentCountOnDisk == (segmentId + 1))
         {
            /// The last segment can not be sparse.
            continue;
         }

         rc = isSparseSegment(segmentId, isSparse);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to check if segment[%d] is sparse, rc:%d",
                   segmentId, rc);
            goto error;
         }

         if (!isSparse)
         {
            continue;
         }

         {
         ossScopedLock guard(&_extendingLatch);
         rc = ensureSegmentNotSparse(segmentId);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to ensure segment not sparse:%d", rc);
            goto error;
         }
         }
      }
   done:
      return rc;
   error:
      goto done;
   }

   INT32 dataPageCluster::ensurePidSpace(PAGE_ID pid)
   {
      INT32 rc = SDB_OK;
      UINT32 minSegCount = 0;
      ossXLatchGuard guard(&_extendingLatch, FALSE);

      if (OSS_UNLIKELY(!isOpen()))
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }
      else if (OSS_UNLIKELY(INVALID_PAGE_ID == pid))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      minSegCount = pid / _args.maxPageCountPerSeg + 1;
      if (minSegCount <= _allocator.getPageCount())
      {
         goto done;
      }

      guard.lock();
      if (minSegCount <= _allocator.getPageCount())
      {
         goto done;
      }

      do
      {
         if (_allocator.getPageCount() == _segmentCountOnDisk)
         {
            rc = allocateNewSegment();
            if (SDB_OK != rc)
            {
               PD_LOG(PDERROR, "failed to allocate new segment on disk:%d", rc);
               goto error;
            }
            ++_segmentCountOnDisk;
         }
         
         rc = _allocator.allocateNewBitmapPages(1);
         if (SDB_OK != rc)
         {
            /// No need to do anything to rollback file.
            /// Just wait for the next allocating.
            PD_LOG(PDERROR, "failed to allocate new page in bitmap:%d", rc);
            goto error;
         }
      } while (_allocator.getPageCount() < minSegCount);
      
   done:
      return rc;
   error:
      goto done;
   }

   INT32 dataPageCluster::extendPageSpace(UINT32 segmentCount,
                                          const UINT32 *oldSegmentCount)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(isOpen(), "can not be closed");

      ossScopedLock guard(&_extendingLatch);
      UINT32 count = _allocator.getPageCount();
      UINT32 targetCount = NULL == oldSegmentCount ?
                           count : *oldSegmentCount;
      targetCount += segmentCount;
      
      while (_segmentCountOnDisk < targetCount)
      {
         rc = allocateNewSegment();
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to allocate new segment on disk:%d", rc);
            goto error;
         }
         ++_segmentCountOnDisk;
      }

      if (count < _segmentCountOnDisk)
      {
         rc = _allocator.allocateNewBitmapPages(_segmentCountOnDisk - count);
         if (SDB_OK != rc)
         {
            /// No need to do anything to rollback file.
            /// Just wait for the next extending.
            PD_LOG(PDERROR, "failed to allocate new page in bitmap:%d", rc);
            goto error;
         }
      }
      
   done:
      return rc;
   error:
      goto done;
   }

   INT32 dataPageCluster::getPagePtr(FILE_TYPE type,
                                     PAGE_ID pid,
                                     mmapPagePointer &ptr)const
   {
      INT32 rc = SDB_OK;
      if (OSS_UNLIKELY(!isOpen()))
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }
      else if (OSS_UNLIKELY(INVALID_FILE_TYPE == type ||
                            INVALID_PAGE_ID == pid))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      else if (type != getDataFileType())
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      rc = getDataPagePtr(pid, ptr);
      if (SDB_OK != rc)
      {
         goto error;
      }
   done:
      return rc;
   error:
      goto done;
   }
}//namespace vessel
}//namespace engine