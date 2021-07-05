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

   Source File Name = indexSpace.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#include "vessel/indexSpace.h"
#include "vessel/storageUnit.h"
#include "vessel/copyOnWriteSpaceEnv.h"
#include "vessel/spaceManagementPage.h"
#include "vessel/idxDataFile.h"
#include "vessel/requestContext.h"
#include "vessel/instanceEnv.h"

namespace engine
{
namespace vessel
{
   indexSpace::~indexSpace()
   {}

   INT32 indexSpace::mapNewLpids(requestContext *context,
                                 UINT32 count,
                                 const PAGE_ID *lpids,
                                 const PAGE_ID *pids)
   {
      INT32 rc = SDB_OK;
      if (OSS_UNLIKELY(NULL == context ||
                       0 == count ||
                       NULL == lpids ||
                       NULL == pids))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      else if (OSS_UNLIKELY(!isOpen()))
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }

      rc = _env->mapNewLpids(context, FILE_TYPE_IDX_D, count, lpids, pids);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to map new lpids:%d", rc);
         goto error;
      }
   done:
      return rc;
   error:
      goto done;
   }

   INT32 indexSpace::allocateIdMapPagesOnDisk(requestContext *context,
                                              PAGE_ID first,
                                              UINT32 count)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(isOpen(), "must be open");
      SDB_ASSERT(INVALID_PAGE_ID != first, "can not be invalid");
      SDB_ASSERT(0 < count, "can not be zero");
      UINT32 maxPageCount = 0;
      UINT32 maxSegCount = 0;

      if (OSS_UNLIKELY(NULL == context ||
                       INVALID_PAGE_ID == first ||
                       0 == count ||
                       count < PAGE_COUNT_IN_EXTENT))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      else if (OSS_UNLIKELY(!isOpen()))
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }

      rc = getSU()->getCoreArgs(FILE_TYPE_IDX_M, NULL,
                           &maxPageCount, &maxSegCount);
      if (SDB_OK != rc)
      {
         goto error;
      }

      if ((maxPageCount * maxSegCount) <= (first + count))
      {
         PD_LOG(PDERROR, "imp pids[%d,%d] out of range", first, count);
         rc = SDB_VESSEL_FS_UPPER_LIMIT;
         goto error;
      }

      rc = _env->allocateFromSmp(context, FILE_TYPE_IDX_M, first, count);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to allocate idx imp:%d", rc);
         goto error;
      }

   done:
      return rc;
   error:
      goto done;
   }

   INT32 indexSpace::createDataFile(requestContext *context,
                                    UINT64 sequence)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(isOpen(), "must be open");

      storageCoreArgs args;
      UINT32 maxCount = 0;
      UINT32 smpCount = 0;
      CHAR *buffer = NULL;
      UINT32 bufferSize = 0;
      PAGE_ID lpid = INVALID_PAGE_ID;
      PAGE_ID pid = INVALID_PAGE_ID;

      rc = getMaxDataFileCount(maxCount);
      if (OSS_UNLIKELY(SDB_OK != rc))
      {
         goto error;
      }

      if (OSS_UNLIKELY((UINT64)maxCount <= sequence))
      {
         rc = SDB_VESSEL_FS_UPPER_LIMIT;
         goto error;
      }

      rc = getSU()->getCoreArgs(FILE_TYPE_IDX_D, args);
      if (OSS_UNLIKELY(SDB_OK != rc))
      {
         goto error;
      }

      if (OSS_UNLIKELY(getSMPCapacityOrCount(args.pageSize, NULL, &smpCount)))
      {
         rc = SDB_VESSEL_INTERNAL_ERR;
         goto error;
      }

      rc = ensureDataFile(context, sequence, NULL);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to ensure data file:%d", rc);
         goto error;
      }

      /// bufferSize = smpCount * 4 (bytes) * 2 (pid and lpid);
      bufferSize = smpCount << 3;
      buffer = context->allocateBuffer(bufferSize);
      if (NULL == buffer)
      {
         PD_LOG(PDERROR, "failed to allocate mem");
         rc = SDB_OOM;
         goto error;
      }

      pid = sequence * args.getMaxPageCountInFile();
      lpid = sequence * smpCount;
      for (UINT32 i = 0; i < smpCount; ++i)
      {
         ((PAGE_ID *)buffer)[i] = lpid + i;
         ((PAGE_ID *)buffer)[i + smpCount] = pid + i;
      }

      rc = this->mapNewLpids(context,
                             smpCount,
                             (const PAGE_ID *)(buffer),
                             (const PAGE_ID *)(buffer + (bufferSize >> 1)));
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to map new smps to id map:%d", rc);
         goto error;
      }
   done:
      if (NULL != buffer)
      {
         context->releaseBuffer(buffer, bufferSize);
      }
      return rc;
   error:
      /// No need to rollback file coz we always use "ensure* functions to 
      /// create and extend file.
      goto done;
   }

   UINT32 indexSpace::getDataFileCount()
   {
      SDB_ASSERT(NULL != getSU(), "can not be null");
      return getSU()->getIdxDfileCount();
   }

   INT32 indexSpace::getDataSMPOfFile(UINT32 sequence, UINT32 i, PAGE_ID &pid)
   {
      INT32 rc = SDB_OK;
   done:
      return rc;
   error:
      goto done;
   }

   INT32 indexSpace::getMaxDataFileCount(UINT32 &cnt)const
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(isOpen(), "must be open");
      UINT32 pageSize = 0;
      UINT32 capacity = 0;

      rc = getSU()->getCoreArgs(FILE_TYPE_IDX_M, &pageSize);
      if (OSS_UNLIKELY(SDB_OK != rc))
      {
         goto error;
      }

      if (OSS_UNLIKELY(!get64AlignedIMPCapacity(pageSize, capacity)))
      {
         rc = SDB_VESSEL_INTERNAL_ERR;
         goto error;
      }

      cnt = getReservedImpCount() * capacity;
   done:
      return rc;
   error:
      goto done;
   }

   INT32 indexSpace::ensureDataFile(requestContext *context,
                                    UINT64 sequence,
                                    idxDataFile **out)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(isOpen(), "must be open");
      SDB_ASSERT(NULL != context, "can not be null");

      UINT32 pageSize = 0;
      UINT32 smpCount = 0;
      UINT32 smpCapacity = 0;
      idxDataFile *file = NULL;
      ossValuePtr ptr = 0;
      PAGE_ID lpid = INVALID_PAGE_ID;
      storageCoreArgs args;
      BOOLEAN sparse = context->getEnv()->options.extendFileWithSparse;

      rc = getSU()->getCoreArgs(FILE_TYPE_IDX_D, &pageSize);
      if (OSS_UNLIKELY(SDB_OK != rc))
      {
         goto error;
      }

      if (!getSMPCapacityOrCount(pageSize, &smpCapacity, &smpCount))
      {
         rc = SDB_VESSEL_INTERNAL_ERR;
         goto error;
      }

      rc = getSU()->ensureIdxDataFile(context, sequence, &file);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to ensure idx data file[%lld], rc:%d", sequence, rc);
         goto error;
      }

      rc = file->ensureSegmentCount(1, sparse);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to ensure segment:%d", rc);
         goto error;
      }

      rc = file->getPagePtr(0, ptr);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to get page ptr:%d", rc);
         goto error;
      }

      lpid = sequence * smpCount;
      /// idx data smp's pid is logical page id.
      /// it will be init as the offset in system imp slot.
      if (!initSmp(pageSize, lpid, smpCapacity, smpCount, (CHAR *)ptr))
      {
         PD_LOG(PDERROR, "failed to init smp");
         rc = SDB_VESSEL_INTERNAL_ERR;
         goto error;
      }

      for (UINT32 i = 1; i < smpCount; ++i)
      {
         rc = file->getPagePtr(i, ptr);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to get page ptr:%d", rc);
            goto error;
         }

         if (!initSmp(pageSize, lpid + i, smpCapacity, 0, (CHAR *)ptr))
         {
            PD_LOG(PDERROR, "failed to init smp");
            rc = SDB_VESSEL_INTERNAL_ERR;
            goto error;
         }
      }

      if (NULL != out)
      {
         *out = file;
      }

   done:
      return rc;
   error:
      goto done;
   }
}//namespace vessel
}//namespace engine