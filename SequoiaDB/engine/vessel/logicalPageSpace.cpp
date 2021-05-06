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

   Source File Name = logicalPageSpace.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#include "vessel/logicalPageSpace.h"
#include "vessel/storageUnit.h"
#include "vessel/idMapPage.h"
#include "vessel/spaceManagementPage.h"
#include "vessel/smpAccessor.h"
#include "vessel/impAccessor.h"
#include "ossLikely.hpp"

namespace engine
{
namespace vessel
{
   logicalPageSpace::~logicalPageSpace()
   {
      _lpidPool.fini();
      _ppidPool.fini();
   }

   void logicalPageSpace::close()
   {
      _su = NULL;
      _lpidPool.fini();
      _pageCountInMeta = 0;
      _ppidPool.fini();
      _dataFileCount = 0;
      return;
   }

   INT32 logicalPageSpace::open(requestContext *context,
                                storageUnit *su)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(!isOpen(), "do not reinit");
      if (OSS_UNLIKELY(NULL == context ||
                       NULL == su ||
                       !su->isOpen()))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      _su = su;
      rc = initInMemLpidPoolFromDisk(context);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to init in-mem lpid pool:%d", rc);
         goto error;
      }

      rc = initInMemPpidPoolFromDisk(context);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to init in-mem pid pool:%d", rc);
         goto error;
      }
   done:
      return rc;
   error:
      close();
      goto done;
   }

   INT32 logicalPageSpace::getDataPageSize(UINT32 &pageSize)const
   {
      INT32 rc = SDB_OK;
      UINT32 size = 0;
      if (OSS_LIKELY(NULL != _su && _su->isOpen()))
      {
         rc = _su->getCoreArgs(getTypeOfDataFile(), &size);
         if (SDB_OK != rc)
         {
            goto error;
         }
         pageSize = size;
      }
      else
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }
   done:
      return rc;
   error:
      goto done;
   }

   INT32 logicalPageSpace::getMetaPageSize(UINT32 &pageSize)const
   {
      INT32 rc = SDB_OK;
      UINT32 size = 0;
      if (OSS_LIKELY(NULL != _su && _su->isOpen()))
      {
         rc = _su->getCoreArgs(getTypeOfMetaFile(), &size);
         if (SDB_OK != rc)
         {
            goto error;
         }
         pageSize = size;
      }
      else
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }
   done:
      return rc;
   error:
      goto done;
   }

   SPACE_ID logicalPageSpace::getSpaceID()const
   {
      return NULL == _su ? INVALID_SPACE_ID : _su->getSpaceID();
   }

   INT32 logicalPageSpace::preallocatePages(requestContext *context,
                                            UINT32 count,
                                            PAGE_ID *lpids,
                                            PAGE_ID *pids)
   {
      INT32 rc = SDB_OK;
      BOOLEAN rollbcak = FALSE;
      if (OSS_UNLIKELY(NULL == context ||
                       0 == count ||
                       NULL == lpids ||
                       NULL == pids))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      rc = preallocateLogicalPids(context, count, lpids);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to preallocate lpids:%d", rc);
         goto error;
      }

      rollbcak = TRUE;

      rc = preallocatePhysicalPids(context, count, pids);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to preallocate pids:%d", rc);
         goto error;
      }

   done:
      return rc;
   error:
      if (rollbcak)
      {
         releaseLogicalPidsPreallocated(context, count, lpids);
      }
      goto done;
   }

   void logicalPageSpace::releasePagesPreallocated(requestContext *context,
                                                   UINT32 count,
                                                   PAGE_ID *lpids,
                                                   PAGE_ID *pids)
   {
      if (OSS_UNLIKELY(NULL == context ||
                       0 == count ||
                       NULL == lpids ||
                       NULL == pids))
      {
         SDB_ASSERT(FALSE, "impossible");
         goto done;
      }

      releasePhysicalPidsPreallocated(context, count, pids);
      releaseLogicalPidsPreallocated(context, count, lpids);
   done:
      return;
   }

   INT32 logicalPageSpace::preallocateLogicalPids(requestContext *context,
                                                  UINT32 count,
                                                  PAGE_ID *lpids)
   {
      INT32 rc = SDB_OK;
      
      if (OSS_UNLIKELY(NULL == context ||
                       0 == count ||
                       NULL == lpids))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      else if (OSS_UNLIKELY(!_lpidPool.isInitialized()))
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }

      do
      {
         UINT32 pageCount = _lpidPool.getPageCount();
         rc = _lpidPool.allocateBits(count, lpids);
         if (SDB_OK == rc)
         {
            break;
         }
         else if (SDB_VESSEL_NOT_ENOUGH_FREE_RESOURCE == rc)
         {
            rc = extendLogicalPidSpace(context, &pageCount);
            if (SDB_OK != rc)
            {
               PD_LOG(PDERROR, "failed to extend lpid pool:%d", rc);
               goto error;
            }
         }
         else
         {
            PD_LOG(PDERROR, "failed to allocate lpid from bitmap:%d", rc);
            goto error;
         }
      } while(TRUE);

   done:
      return rc;
   error:
      goto done;
   }

   void logicalPageSpace::releaseLogicalPidsPreallocated(requestContext *context,
                                                         UINT32 count,
                                                         const PAGE_ID *lpids)
   {
      if (OSS_UNLIKELY(NULL == context ||
                       0 == count ||
                       NULL == lpids))
      {
         goto done;
      }
      else if (OSS_UNLIKELY(!_lpidPool.isInitialized()))
      {
          goto done;
      }

      _lpidPool.releaseBits(count, lpids);
   done:
      return;
   }

   INT32 logicalPageSpace::preallocatePhysicalPids(requestContext *context,
                                                   UINT32 count,
                                                   PAGE_ID *pids)
   {
      INT32 rc = SDB_OK;
      if (OSS_UNLIKELY(NULL == context ||
                       0 == count ||
                       NULL == pids))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      else if (OSS_UNLIKELY(!_ppidPool.isInitialized()))
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }

      do
      {
         UINT32 pageCount = _ppidPool.getPageCount();
         rc = _ppidPool.allocateBits(count, pids);
         if (SDB_OK == rc)
         {
            break;
         }
         else if (SDB_VESSEL_NOT_ENOUGH_FREE_RESOURCE == rc)
         {
            rc = extendPhysicalPidSpace(context, &pageCount);
            if (SDB_OK != rc)
            {
               PD_LOG(PDERROR, "failed to extend lpid pool:%d", rc);
               goto error;
            }
         }
         else
         {
            PD_LOG(PDERROR, "failed to allocate lpid from bitmap:%d", rc);
            goto error;
         }
      }while (TRUE);
   done:
      return rc;
   error:
      goto done;
   }

   void logicalPageSpace::releasePhysicalPidsPreallocated(requestContext *context,
                                                           UINT32 count,
                                                           const PAGE_ID *pids)
   {
      if (OSS_UNLIKELY(NULL == context ||
                       0 == count ||
                       NULL == pids))
      {
         goto done;
      }
      else if (OSS_UNLIKELY(!_ppidPool.isInitialized()))
      {
          goto done;
      }

      _ppidPool.releaseBits(count, pids);

   done:
      return;
   }

   BOOLEAN logicalPageSpace::isOpen()const
   {
      return NULL != _su;
   }

   PAGE_ID logicalPageSpace::getSMPPIdOfImp(PAGE_ID pid)const
   {
      PAGE_ID smp = INVALID_PAGE_ID;
      INT32 rc = SDB_OK;
      UINT32 capacity = 0;
      UINT32 count = 0;
      UINT32 pageSize = 0;
      UINT32 n = 0;

      if (OSS_UNLIKELY(!isOpen() || INVALID_PAGE_ID == pid))
      {
         SDB_ASSERT(FALSE, "impossible");
         goto done;
      }

      rc = getSU()->getCoreArgs(getTypeOfMetaFile(), &pageSize);
      if (OSS_UNLIKELY(SDB_OK != rc))
      {
         goto done;
      }

      if (OSS_UNLIKELY(!getSMPCapacityOrCount(pageSize, &capacity, &count)))
      {
         goto done;
      }

      n = pid / capacity;
      if (OSS_UNLIKELY(count <= n))
      {
         goto done;
      }
      
      smp = n;
   done:
      return smp;
   }

   PAGE_ID logicalPageSpace::getImpPidOfLpid(PAGE_ID lpid)const
   {
      SDB_ASSERT(isOpen(), "impossible");
      SDB_ASSERT(INVALID_PAGE_ID != lpid, "can not be invalid");
      PAGE_ID pid = INVALID_PAGE_ID;
      UINT32 pageSize = 0;
      UINT32 impCapacity = 0;
      UINT32 smpCount = 0;
      INT32 rc = SDB_OK;

      if (OSS_UNLIKELY(!isOpen()))
      {
         goto done;
      }

      rc = _su->getCoreArgs(getTypeOfMetaFile(), &pageSize);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to get page size of %d, rc:%d",
                getTypeOfMetaFile(), rc);
         goto done;
      }

      if (!get64AlignedIMPCapacity(pageSize, impCapacity))
      {
         PD_LOG(PDERROR, "failed to get imp capacity");
         goto done;
      }

      if (!getSMPCapacityOrCount(pageSize, NULL, &smpCount))
      {
         PD_LOG(PDERROR, "failed to get smp count");
         goto done;
      }

      pid = (lpid / impCapacity) + smpCount + getSystemPageCount();
   done:
      return pid;
   }

   INT32 logicalPageSpace::extendLogicalPidSpace(requestContext *context,
                                                 const UINT32 *pageInPoolBeforeExtending)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(NULL != context, "can not be null");
      SDB_ASSERT(NULL != _su, "can not be null");
      SDB_ASSERT(_lpidPool.isInitialized(), "can not be invalid");
      static const UINT32 EXTENDING_META_PAGE_COUNT = 1;
      UINT32 currentImpCount = 0;
      UINT32 currentTotalPageCount = 0;
      UINT32 smpCount = 0;
      UINT32 pageSize = 0;
      FILE_TYPE type = getTypeOfMetaFile();

      rc = _su->getCoreArgs(type, &pageSize);
      if (OSS_UNLIKELY(SDB_OK != rc))
      {
         PD_LOG(PDERROR, "failed to get page size:%d", rc);
         goto error;
      }

      if (OSS_UNLIKELY(!getSMPCapacityOrCount(pageSize, NULL, &smpCount)))
      {
         PD_LOG(PDERROR, "failed to get smp count");
         rc = SDB_VESSEL_INTERNAL_ERR;
         goto error;
      }

      {
      ossScopedLock guard(&_extendingLatch);
      
      currentImpCount = _lpidPool.getPageCount();
      if (NULL != pageInPoolBeforeExtending &&
          (*pageInPoolBeforeExtending < currentImpCount))
      {
         goto done;
      }

      currentTotalPageCount = currentImpCount +
                              smpCount +
                              getSystemPageCount();

      /// The total count should always as same as _pageCountInMeta.
      /// Unless failed to allocate new page in _lpidPool.(eg: Error OOM)
      if (currentTotalPageCount == _pageCountInMeta)
      {
         rc = allocateIdMapPagesOnDisk(context, _pageCountInMeta, EXTENDING_META_PAGE_COUNT);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to allocate new imps:%d", rc);
            goto error;
         }
         _pageCountInMeta += EXTENDING_META_PAGE_COUNT;
      }
      else if (OSS_UNLIKELY((currentTotalPageCount + EXTENDING_META_PAGE_COUNT) !=
                             _pageCountInMeta))
      {
         SDB_ASSERT(FALSE, "_pageCountInMeta in chaos");
         /// The delta should always be EXTENDING_META_PAGE_COUNT.
         PD_LOG(PDSEVERE, "page count[%d] covered by pool or _pageCountInMeta[%d] is in chaos",
                currentTotalPageCount, _pageCountInMeta);
         rc = SDB_VESSEL_INTERNAL_ERR;
         goto error;
      }

      /// _pageCountInMeta has been update.
      /// if failed to update pool, just waiting for next extending.
      rc = _lpidPool.allocateBitPages(EXTENDING_META_PAGE_COUNT, 0);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to extend lpid bitmap:%d", rc);
         goto error;
      }
      }
   done:
      return rc;
   error:
      goto done;
   }

   INT32 logicalPageSpace::extendPhysicalPidSpace(requestContext *context,
                                                  const UINT32 *pageInPoolBeforeExtending)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(NULL != context, "can not be null");
      SDB_ASSERT(NULL != _su, "can not be null");
      SDB_ASSERT(_ppidPool.isInitialized(), "can not be invalid");
      FILE_TYPE type = getTypeOfDataFile();
      UINT32 smpCount = 0;
      UINT32 smpCapacity = 0;
      UINT32 pageSize = 0;
      UINT32 pageCountInPool = 0;
      UINT32 fileCountInPool = 0;
      ossPoolVector<UINT32> occupied;

      rc = _su->getCoreArgs(type, &pageSize);
      if (OSS_UNLIKELY(SDB_OK != rc))
      {
         PD_LOG(PDERROR, "failed to get data page size of type[%d], rc:%d", type, rc);
         goto error;
      }

      if (OSS_UNLIKELY(!getSMPCapacityOrCount(pageSize, &smpCapacity, &smpCount)))
      {
         PD_LOG(PDERROR, "failed to get smp capacity");
         rc = SDB_VESSEL_INTERNAL_ERR;
         goto error;
      }

      {
      ossScopedLock guard(&_extendingLatch);
      pageCountInPool = _ppidPool.getPageCount();
      if (NULL != pageInPoolBeforeExtending &&
          (*pageInPoolBeforeExtending < pageCountInPool))
      {
         goto done;
      }

      fileCountInPool = pageCountInPool / smpCount;
      if (fileCountInPool == _dataFileCount)
      {
         UINT64 sequence = _dataFileCount;
         rc = createDataFile(context, sequence);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "faield to create data file with sequence[%lld], rc:%d", sequence, rc);
            goto error;
         }

         ++_dataFileCount;
      }
      else if ((fileCountInPool + 1) != _dataFileCount)
      {
         SDB_ASSERT(FALSE, "_dataFileCount in chaos");
         /// The delta should always be smp count.
         PD_LOG(PDSEVERE, "file count[%d] covered by pool or _dataFileCount[%d] is in chaos",
                fileCountInPool, _dataFileCount);
         rc = SDB_VESSEL_INTERNAL_ERR;
         goto error;
      }

      occupied.reserve(smpCount);
      occupied.push_back(smpCount);
      for (UINT32 i = 1; i < smpCount; ++i)
      {
         occupied.push_back(0);
      }
      rc = _ppidPool.allocateBitPages(occupied);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to add new file to ppid pool:%d", rc);
         goto error;
      }
      }
   done:
      return rc;
   error:
      goto done;
   }

   INT32 logicalPageSpace::initInMemLpidPoolFromDisk(requestContext *context)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(NULL != context, "can not be null");
      SDB_ASSERT(NULL != _su && _su->isOpen(), "can not be invalid");
      SDB_ASSERT(!_lpidPool.isInitialized(), "do not reinit");
      UINT32 pageSize = 0;
      UINT32 impCapacity = 0;
      UINT32 smpCapacity = 0;
      UINT32 smpCount = 0;
      FILE_TYPE type = getTypeOfMetaFile();
      UINT32 pageSkippedWhenScan = 0;
      smpAccessor accessor;
      pageAccessor::options o;

      SDB_ASSERT(INVALID_FILE_TYPE != type, "can not be invalid");

      rc = _su->getCoreArgs(type, &pageSize);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to get page size of type:%d, rc:%d", type, rc);
         goto error;
      }

      if (!get64AlignedIMPCapacity(pageSize, impCapacity))
      {
         PD_LOG(PDERROR, "failed to get capacity of imp");
         rc = SDB_VESSEL_INTERNAL_ERR;
         goto error;
      }

      if (!getSMPCapacityOrCount(pageSize, &smpCapacity, &smpCount))
      {
         PD_LOG(PDERROR, "failed to get smp capacity");
         rc = SDB_VESSEL_INTERNAL_ERR;
         goto error;
      }

      _pageCountInMeta = smpCount + getSystemPageCount();
      pageSkippedWhenScan = smpCount +
                            getSystemPageCount() + 
                            getReservedImpCount();

      rc = _lpidPool.init(impCapacity, getFreeBoundOfLpidPool());
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to init lpid pool:%d", rc);
         goto error;
      }

      /// imp pages reserved will not managed by lpid pool. 
      _lpidPool.incPageCount(getReservedImpCount());

      for (UINT32 i = 0; i < smpCount; ++i)
      {
         PAGE_ID firstImpToScan = i * smpCapacity + pageSkippedWhenScan;
         UINT32 impCountToScan = 0;
         UINT32 freeCount = 0;

         rc = accessor.initWithOptions(context, type, SMP_PAGE_ID + i,
                                       o, _su);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to init accessor of page[%d], rc:%d",
                   SMP_PAGE_ID + i, rc);
            goto error;
         }

         rc = accessor.getFreeCount(context, freeCount);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to get free count of smp:%d", rc);
            goto error;
         }

         accessor.fini(context);

         if (smpCapacity < (freeCount + pageSkippedWhenScan))
         {
            PD_LOG(PDERROR, "unexpected free count[%d] in page[%d]",
                   freeCount, SMP_PAGE_ID + i);
            rc = SDB_VESSEL_INTERNAL_ERR;
            goto error;
         }

         /// We always modify smp of idmap file serially.
         impCountToScan = smpCapacity - freeCount - pageSkippedWhenScan;
         for (UINT32 j = 0; j < impCountToScan; ++j)
         {
            rc = mapImpPageToBitmap(context, pageSize,
                                     impCapacity, firstImpToScan + j,
                                     type, _su, _lpidPool);
            if (SDB_OK != rc)
            {
               PD_LOG(PDERROR, "failed to add page to pool:%d", rc);
               goto error;
            }
         }

         /// Always allocate new pages serially according to the page id.
         /// If free count is not zero, pages must be all free int the last ones.
         if (0 < freeCount)
         {
            break;
         }
         else
         {
            pageSkippedWhenScan = 0;
         }
      }

      _pageCountInMeta += _lpidPool.getPageCount();

   done:
      return rc;
   error:
      accessor.fini(context);
      _lpidPool.fini();
      _pageCountInMeta = 0;
      goto done;
   }

   INT32 logicalPageSpace::initInMemPpidPoolFromDisk(requestContext *context)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(NULL != context, "can not be null");
      SDB_ASSERT(NULL != _su && _su->isOpen(), "can not be invalid");
      SDB_ASSERT(!_ppidPool.isInitialized(), "do not reinit");
      FILE_TYPE type = getTypeOfDataFile();
      UINT32 fileCount = 0;
      UINT32 pageSize = 0;
      UINT32 smpCount = 0;
      UINT32 smpCapacity = 0;

      rc = _su->getCoreArgs(type, &pageSize);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to get page size:%d", rc);
         goto error;
      }

      if (!getSMPCapacityOrCount(pageSize, &smpCapacity, &smpCount))
      {
         PD_LOG(PDERROR, "failed to get smp capacity");
         rc = SDB_VESSEL_INTERNAL_ERR;
         goto error;
      }

      rc = _ppidPool.init(smpCapacity, getFreeBoundOfPpidPool());
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to init ppid pool:%d", rc);
         goto error;
      }

      fileCount = getDataFileCount();
      for (UINT32 i = 0; i < fileCount; ++i)
      {
         for (UINT32 j = 0; j < smpCount; ++j)
         {
            PAGE_ID pid = INVALID_PAGE_ID;
            rc = getDataSMPOfFile(i, j, pid);
            if (SDB_OK != rc)
            {
               PD_LOG(PDERROR, "failed to get smp pid of [%d:%d], rc:%d",
                      i, j, rc);
               goto error;
            }

            rc = mapSMPPageToBitmap(context, pageSize, smpCapacity,
                                    pid, type, _su, _ppidPool);
            if (SDB_OK != rc)
            {
               PD_LOG(PDERROR, "failed to map smp[%d] to bitmap:%d", pid, rc);
               goto error;
            }
         }
      }
      _dataFileCount = fileCount;
   done:
      return rc;
   error:
      _ppidPool.fini();
      _dataFileCount = 0;
      goto done;
   }

   INT32 logicalPageSpace::mapImpPageToBitmap(requestContext *context,
                                               UINT32 impPageSize,
                                               UINT32 impCapacity,
                                               PAGE_ID impPid,
                                               FILE_TYPE type,
                                               storageUnit *su,
                                               inMemBitMap &bitmap)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(NULL != context, "can not be null");
      SDB_ASSERT(0 < impPageSize, "can not be invalid");
      SDB_ASSERT(0 < impCapacity && 0 == (impCapacity & 63), "must be 64 aligned");
      SDB_ASSERT(INVALID_FILE_TYPE != type, "can not be invalid");
      SDB_ASSERT(INVALID_PAGE_ID != impPid, "can not be invalid");
      SDB_ASSERT(bitmap.isInitialized(), "can not be invalid");
      SDB_ASSERT(NULL != su, "can not be null");
      impAccessor accessor;
      pageAccessor::options o;
      UINT32 freeCount = 0;
      CHAR *buffer = NULL;
      UINT32 bitsCount = impCapacity >> 6;

      rc = accessor.initWithOptions(context, type, impPid, o, su);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to init imp accessor:%d", rc);
         goto error;
      }

      rc = accessor.getFreeCount(context, freeCount);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to get free count of imp:%d", rc);
         goto error;
      }

      if (0 == freeCount)
      {
         bitmap.incPageCount();
      }
      else if (impCapacity == freeCount)
      {
         rc = bitmap.allocateNewBitPage();
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to allocate new bitmap page:%d", rc);
            goto error;
         }
      }
      else
      {
         buffer = context->allocateBuffer(impPageSize);
         if (NULL == buffer)
         {
            PD_LOG(PDERROR, "failed to allocate mem");
            rc = SDB_OOM;
            goto error;
         }

         rc = accessor.dumpAsBitMap((UINT64 *)buffer, freeCount);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to dump imp:%d", rc);
            goto error;
         }

         rc = bitmap.mapNewBitPage(bitsCount, (const UINT64 *)buffer);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to map imp[%d] to bitmap:%d", impPid, rc);
            goto error;
         }
      }

      accessor.fini(context);
      
   done:
      if (NULL != buffer)
      {
         context->releaseBuffer(buffer, impPageSize);
      }
      return rc;
   error:
      accessor.fini(context);
      goto done;
   }

   INT32 logicalPageSpace::mapSMPPageToBitmap(requestContext *context,
                                             UINT32 pageSize,
                                             UINT32 capacity,
                                             PAGE_ID pid,
                                             FILE_TYPE type,
                                             storageUnit *su,
                                             inMemBitMap &bitmap)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(NULL != context, "can not be null");
      SDB_ASSERT(0 < pageSize, "can not be invalid");
      SDB_ASSERT(0 < capacity && 0 == (capacity & 63), "must be 64 aligned");
      SDB_ASSERT(INVALID_FILE_TYPE != type, "can not be invalid");
      SDB_ASSERT(INVALID_PAGE_ID != pid, "can not be invalid");
      SDB_ASSERT(bitmap.isInitialized(), "can not be invalid");
      SDB_ASSERT(NULL != su, "can not be null");
      smpAccessor accessor;
      pageAccessor::options o;
      UINT32 freeCount = 0;
      CHAR *buffer = NULL;
      UINT32 bitsCount = capacity >> 6;

      rc = accessor.initWithOptions(context, type, pid, o, su);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to init imp accessor:%d", rc);
         goto error;
      }

      rc = accessor.getFreeCount(context, freeCount);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to get free count of imp:%d", rc);
         goto error;
      }

      if (0 == freeCount)
      {
         bitmap.incPageCount();
      }
      else if (capacity == freeCount)
      {
         rc = bitmap.allocateNewBitPage();
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to allocate new bitmap page:%d", rc);
            goto error;
         }
      }
      else
      {
         buffer = context->allocateBuffer(pageSize);
         if (NULL == buffer)
         {
            PD_LOG(PDERROR, "failed to allocate mem");
            rc = SDB_OOM;
            goto error;
         }

         rc = accessor.dumpSMP(context, pageSize, buffer);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to dump smp:%d", rc);
            goto error;
         }

         rc = bitmap.mapNewBitPage(bitsCount, (const UINT64 *)buffer);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to map smp[%d] to bitmap:%d", pid, rc);
            goto error;
         }
      }

      accessor.fini(context);

   done:
      if (NULL != buffer)
      {
         context->releaseBuffer(buffer, pageSize);
      }
      return rc;
   error:
      accessor.fini(context);
      goto done;
   }
}//namespace vessel
}//namespace engine