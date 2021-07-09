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

   Source File Name = deltaLogRecordBuilder.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#include "vessel/deltaLogRecordBuilder.h"
#include "vessel/logicalPageSpaceCheckpoint.h"

namespace engine
{
namespace vessel
{
   void deltaLogRecordBuilder::reset()
   {
      ((deltaLogRecordHead *)_buffer)->_version = 0;
      ((deltaLogRecordHead *)_buffer)->_type = INVALID_DELTA_LOG_RECORD_TYPE;
      ((deltaLogRecordHead *)_buffer)->_size = 0;
      _w = 0;
      return;
   }

   deltaLogRecord deltaLogRecordBuilder::getDeltaLogRecord()const
   {
      deltaLogRecord dlr;
      dlr.reset(_buffer);
      return dlr;
   }

   INT32 deltaLogRecordBuilder::beginToBuild(DELTA_LOG_RECORD_TYPE type)
   {
      INT32 rc = SDB_OK;
      reset();
      if (OSS_UNLIKELY(INVALID_DELTA_LOG_RECORD_TYPE == type))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      ((deltaLogRecordHead *)_buffer)->_version = DELTA_LOG_RECORD_VERSION;
      ((deltaLogRecordHead *)_buffer)->_type = type;
      ((deltaLogRecordHead *)_buffer)->_size = 0;
      _w = DELTA_LOG_RECORD_HEAD_SIZE;
   done:
      return rc;
   error:
      goto done;
   }

   INT32 deltaLogRecordBuilder::append(UINT32 size, const void *data)
   {
      INT32 rc = SDB_OK;
      if (OSS_UNLIKELY(0 == size ||
                       NULL == data))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      else if (OSS_UNLIKELY(!isBuilding()))
      {
         rc = SDB_VESSEL_OPERATOION_NOT_PERMITTED;
         goto error;
      }
      else if (MAX_DELTA_LOG_RECORD_SIZE < (_w + size))
      {
         rc = SDB_VESSEL_OUT_OF_RESOURCE;
         goto error;
      }

      ossMemcpy((void *)(_buffer + _w), data, size);
      _w += size;

   done:
      return rc;
   error:
      goto done;
   }

   void deltaLogRecordBuilder::done()
   {
      if (OSS_LIKELY(isBuilding()))
      {
         ((deltaLogRecordHead *)_buffer)->_size = _w;
         _w = 0;
      }
      return;
   }

   INT32 deltaLogRecordBuilder::buildDummyLog(UINT32 recordSize)
   {
      INT32 rc = SDB_OK;

      if (OSS_UNLIKELY(recordSize < DELTA_LOG_RECORD_HEAD_SIZE ||
                       MAX_DELTA_LOG_RECORD_SIZE < recordSize))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      ((deltaLogRecordHead *)_buffer)->_version = DELTA_LOG_RECORD_VERSION;
      ((deltaLogRecordHead *)_buffer)->_type = DELTA_LOG_TYPE_DUMMY;
      ((deltaLogRecordHead *)_buffer)->_size = recordSize;
      ossMemset((void *)((ossValuePtr)_buffer + DELTA_LOG_RECORD_HEAD_SIZE),
                0, recordSize - DELTA_LOG_RECORD_HEAD_SIZE);
   done:
      return rc;
   error:
      goto done;
   }

   INT32 deltaLogRecordBuilder::buildCheckpointLog(const LPS_CHECKPOINT &checkpoint)
   {
      INT32 rc = SDB_OK;
      
      if (OSS_UNLIKELY(!checkpoint.isValid()))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      ((deltaLogRecordHead *)_buffer)->_version = DELTA_LOG_RECORD_VERSION;
      ((deltaLogRecordHead *)_buffer)->_type = DELTA_LOG_TYPE_CHECKPOINT;
      ((deltaLogRecordHead *)_buffer)->_size = DELTA_LOG_RECORD_HEAD_SIZE + LPS_CHECKPOINT_SIZE;
      SDB_ASSERT(((deltaLogRecordHead *)_buffer)->_size <= MAX_DELTA_LOG_RECORD_SIZE, "impossible");
      ossMemcpy((void *)((ossValuePtr)_buffer + DELTA_LOG_RECORD_HEAD_SIZE),
                &checkpoint, LPS_CHECKPOINT_SIZE);
   done:
      return rc;
   error:
      goto done;
   }

   INT32 deltaLogRecordBuilder::buildMappingLog(PAGE_SNAPSHOT_VERION psv,
                                                UINT8 count,
                                                const mappedLogicalPageId *mpids)
   {
      INT32 rc = SDB_OK;
      if (OSS_UNLIKELY(INVALID_PAGE_SNAPSHOT_VERSION == psv ||
                       0 == count ||
                       NULL == mpids))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      rc = beginToBuild(DELTA_LOG_TYPE_MAPPING);
      if (SDB_OK != rc)
      {
         goto error;
      }

      rc = append(sizeof(PAGE_SNAPSHOT_VERION), &psv);
      if (SDB_OK != rc)
      {
         goto error;
      }

      rc = append(sizeof(UINT8), &count);
      if (SDB_OK != rc)
      {
         goto error;
      }

      for (UINT32 i = 0; i < count; ++i)
      {
         UINT64 v = 0;
         const mappedLogicalPageId &mpid = mpids[i];
         if (OSS_UNLIKELY(!mpid.isValid()))
         {
            PD_LOG(PDERROR, "invalid id found");
            rc = SDB_INVALIDARG;
            goto error;
         }

         v = mpid.dumpAsUint64();

         rc = append(sizeof(UINT64), &v);
         if (SDB_OK != rc)
         {
            goto error;
         }
      }

      done();

   done:
      return rc;
   error:
      reset();
      goto done;
   }

   INT32 deltaLogRecordBuilder::buildRemappingLog(PAGE_SNAPSHOT_VERION psv,
                                                  UINT8 count,
                                                  const mappedLogicalPageId *mpids,
                                                  const PAGE_ID *oldPids,
                                                  BOOLEAN releaseOld)
   {
      INT32 rc = SDB_OK;
      UINT8 flags = 0;

      if (OSS_UNLIKELY(INVALID_PAGE_SNAPSHOT_VERSION == psv ||
                       0 == count ||
                       NULL == mpids ||
                       NULL == oldPids))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      rc = beginToBuild(DELTA_LOG_TYPE_REMAPPING);
      if (SDB_OK != rc)
      {
         goto error;
      }

      rc = append(sizeof(PAGE_SNAPSHOT_VERION), &psv);
      if (SDB_OK != rc)
      {
         goto error;
      }

      if (releaseOld)
      {
         OSS_BIT_SET(flags, DELTA_LOG_TYPE_REMAPPING_FLAG_RELEASE_PID);
      }
      rc = append(sizeof(UINT8), &flags);
      if (SDB_OK != rc)
      {
         goto error;
      }

      for (UINT8 i = 0; i < count; ++i)
      {
         UINT64 v = 0;
         const mappedLogicalPageId &mappedId = mpids[i];
         if (OSS_UNLIKELY(!mappedId.isValid()))
         {
            PD_LOG(PDERROR, "invalid id found");
            rc = SDB_INVALIDARG;
            goto error;
         }
         else if (OSS_UNLIKELY(INVALID_PAGE_ID == oldPids[i]))
         {
            PD_LOG(PDERROR, "invalid old pid found");
            rc = SDB_INVALIDARG;
            goto error;
         }

         v = mappedId.dumpAsUint64();
         rc = append(sizeof(UINT64), &v);
         if (SDB_OK != rc)
         {
            goto error;
         }

         rc = append(sizeof(UINT32), &(oldPids[i]));
         if (SDB_OK != rc)
         {
            goto error;
         }
      }

      done();
   done:
      return rc;
   error:
      reset();
      goto done;
   }


   INT32 deltaLogRecordBuilder::buildUnmappingLog(UINT8 count,
                                                  const mappedLogicalPageId *mpids,
                                                  BOOLEAN releaseOld)
   {
      INT32 rc = SDB_OK;
      UINT8 flags = 0;
      if (OSS_UNLIKELY(0 == count ||
                       NULL == mpids))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      rc = beginToBuild(DELTA_LOG_TYPE_UNMAPPING);
      if (SDB_OK != rc)
      {
         goto error;
      }

      if (releaseOld)
      {
         OSS_BIT_SET(flags, DELTA_LOG_TYPE_UNMAPPING_FLAG_RELEASE_PID);
      }

      rc = append(sizeof(UINT8), &flags);
      if (SDB_OK != rc)
      {
         goto error;
      }

      rc = append(sizeof(UINT8), &count);
      if (SDB_OK != rc)
      {
         goto error;
      }

      for (UINT8 i = 0; i < count; ++i)
      {
         UINT64 v = 0;
         const mappedLogicalPageId &mappedId = mpids[i];
         if (OSS_UNLIKELY(!mappedId.isValid()))
         {
            PD_LOG(PDERROR, "invalid lpid found");
            rc = SDB_INVALIDARG;
            goto error;
         }

         v = mappedId.dumpAsUint64();
         rc = append(sizeof(UINT64), &v);
         if (SDB_OK != rc)
         {
            goto error;
         }
      }

      done();
   done:
      return rc;
   error:
      goto done;
   }

   INT32 deltaLogRecordBuilder::buildReleasingLog(UINT8 count,
                                                  const PAGE_ID *pids)
   {
      INT32 rc = SDB_OK;
      if (OSS_UNLIKELY(0 == count ||
                       NULL == pids))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      
      rc = beginToBuild(DELTA_LOG_TYPE_RELEASING);
      if (OSS_UNLIKELY(SDB_OK != rc))
      {
         goto error;
      }

      rc = append(sizeof(UINT8), &count);
      if (OSS_UNLIKELY(SDB_OK != rc))
      {
         goto error;
      }

      for (UINT8 i = 0; i < count; ++i)
      {
         if (INVALID_PAGE_ID == pids[i])
         {
            PD_LOG(PDERROR, "invalid pid found");
            rc = SDB_INVALIDARG;
            goto error;
         }

         rc = append(sizeof(PAGE_ID), pids + i);
         if (SDB_OK != rc)
         {
            goto error;
         }
      }

      done();
   done:
      return rc;
   error:
      goto done;
   }

}//namespace vessel
}//namespace engine