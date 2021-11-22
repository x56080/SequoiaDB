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

   Source File Name = deltaLogRecordReader.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#include "vessel/deltaLogRecordReader.h"
#include "ossLikely.hpp"
#include "pdTrace.hpp"

namespace engine
{
namespace vessel
{
   ////////////dlrMappingReader
   INT32 dlrMappingReader::read(const deltaLogRecord &dlr,
                                PAGE_SNAPSHOT_VERION &psv,
                                UINT8 &count)
   {
      INT32 rc = SDB_OK;
      UINT32 offset = 0;

      if (OSS_UNLIKELY(!dlr.isValid()))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      else if (OSS_UNLIKELY(dlr.getLogHead()->_type !=
                            DELTA_LOG_TYPE_MAPPING))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      {
      const PAGE_SNAPSHOT_VERION *ptr = dlr.getRecordBodyPtr<PAGE_SNAPSHOT_VERION>(offset);
      if (NULL == ptr)
      {
         PD_LOG(PDERROR, "failed to get psv ptr");
         rc = SDB_VESSEL_INTERNAL_ERR;
         goto error;
      }

      if (INVALID_PAGE_SNAPSHOT_VERSION == *ptr)
      {
         PD_LOG(PDERROR, "invalid psv found");
         rc = SDB_VESSEL_INTERNAL_ERR;
         goto error;
      }
      psv = *ptr;
      offset += sizeof(PAGE_SNAPSHOT_VERION);
      }

      {
      const UINT8 *ptr = dlr.getRecordBodyPtr<UINT8>(offset);
      if (NULL == ptr)
      {
         PD_LOG(PDERROR, "failed to get count ptr");
         rc = SDB_VESSEL_INTERNAL_ERR;
         goto error;
      }

      if (0 == *ptr)
      {
         PD_LOG(PDERROR, "invalid count found");
         rc = SDB_VESSEL_INTERNAL_ERR;
         goto error;
      }
      count = *ptr;
      }
      
   done:
      return rc;
   error:
      goto done;
   }

   INT32 dlrMappingReader::getItem(const deltaLogRecord &dlr,
                                   UINT8 i,
                                   mappedLogicalPageId &mappedId)
   {
      INT32 rc = SDB_OK;
      const UINT64 *ptr = NULL;
      UINT32 offset = sizeof(PAGE_SNAPSHOT_VERION) + sizeof(UINT8) +
                      ((UINT32)i << 3);

      PAGE_SNAPSHOT_VERION psv = INVALID_PAGE_SNAPSHOT_VERSION;
      UINT8 count = 0;;

      rc = read(dlr, psv, count);
      if (SDB_OK != rc)
      {
         goto error;
      }

      if (count <= i)
      {
         rc = SDB_OUT_OF_BOUND;
         goto error;
      }

      ptr = (const UINT64 *)(dlr.getRecordBodyPtr(offset, sizeof(UINT64)));
      if (NULL == ptr)
      {
         PD_LOG(PDERROR, "failed to get pair ptr of i[%d]", i);
         rc = SDB_VESSEL_INTERNAL_ERR;
         goto error;
      }

      mappedId = *ptr;
      if (!mappedId.isValid())
      {
         PD_LOG(PDERROR, "invalid pid pair found at i[%d]", i);
         rc = SDB_VESSEL_INTERNAL_ERR;
         goto error;
      }
   done:
      return rc;
   error:
      mappedId = mappedLogicalPageId();
      goto done;
   }

   ////////////dlrRemappingReader
   INT32 dlrRemappingReader::read(const deltaLogRecord &dlr,
                                  PAGE_SNAPSHOT_VERION &psv,
                                  UINT8 &flags,
                                  UINT8 &count)
   {
      INT32 rc = SDB_OK;
      UINT32 offset = 0;

      if (OSS_UNLIKELY(!dlr.isValid()))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      else if (OSS_UNLIKELY(DELTA_LOG_TYPE_REMAPPING != dlr.getLogHead()->_type))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      {
      const PAGE_SNAPSHOT_VERION *ptr = dlr.getRecordBodyPtr<PAGE_SNAPSHOT_VERION>(offset);
      if (NULL == ptr)
      {
         rc = SDB_VESSEL_INTERNAL_ERR;
         goto error;
      }
      if (INVALID_PAGE_SNAPSHOT_VERSION == *ptr)
      {
         PD_LOG(PDERROR, "invalid snapshot version");
         rc = SDB_VESSEL_INTERNAL_ERR;
         goto error;
      }
      psv = *ptr;
      offset += sizeof(PAGE_SNAPSHOT_VERION);
      }

      {
      const UINT8 *ptr = dlr.getRecordBodyPtr<UINT8>(offset);
      if (NULL == ptr)
      {
         rc = SDB_VESSEL_INTERNAL_ERR;
         goto error;
      }
      flags = *ptr;
      offset += sizeof(UINT8);
      }

      {
      const UINT8 *ptr = dlr.getRecordBodyPtr<UINT8>(offset);
      if (NULL == ptr)
      {
         rc = SDB_VESSEL_INTERNAL_ERR;
         goto error;
      }
      count = *ptr;
      if (0 == count)
      {
         PD_LOG(PDERROR, "count is zero");
         rc = SDB_VESSEL_INTERNAL_ERR;
         goto error;
      }
      }

   done:
      return rc;
   error:
      goto done;
   }

   INT32 dlrRemappingReader::getItem(const deltaLogRecord &dlr,
                                     UINT8 i,
                                     mappedLogicalPageId &mappedId,
                                     PAGE_ID &oldPid)
   {
      INT32 rc = SDB_OK;
      PAGE_SNAPSHOT_VERION psv = INVALID_PAGE_SNAPSHOT_VERSION;
      UINT8 flags = 0;
      UINT8 count = 0;
      UINT32 offset = sizeof(PAGE_SNAPSHOT_VERION) + sizeof(UINT8)
                      + sizeof(UINT8) + (i * (sizeof(UINT64) + sizeof(UINT32)));

      rc = read(dlr, psv, flags, count);
      if (SDB_OK != rc)
      {
         goto error;
      }
      
      if (count <= i)
      {
         rc = SDB_OUT_OF_BOUND;
         goto error;
      }

      {
      const UINT64 *ptr = dlr.getRecordBodyPtr<UINT64>(offset);
      if (NULL == ptr)
      {
         PD_LOG(PDERROR, "failed to get mapped id ptr");
         rc = SDB_VESSEL_INTERNAL_ERR;
         goto error;
      }

      mappedId = *ptr;
      if (!mappedId.isValid())
      {
         PD_LOG(PDERROR, "invalid mapped id found");
         rc = SDB_VESSEL_INTERNAL_ERR;
         goto error;
      }
      }

      {
      offset += sizeof(UINT64);
      const UINT32 *ptr = dlr.getRecordBodyPtr<UINT32>(offset);
      if (NULL == ptr)
      {
         PD_LOG(PDERROR, "failed to get old pid ptr");
         rc = SDB_VESSEL_INTERNAL_ERR;
         goto error;
      }

      oldPid = *ptr;
      if (INVALID_PAGE_ID == oldPid)
      {
         PD_LOG(PDERROR, "invalid old pid found");
         rc = SDB_VESSEL_INTERNAL_ERR;
         goto error;
      }
      }
   done:
      return rc;
   error:
      goto done;
   }

   ////////////dlrUnmapingReader
   INT32 dlrUnmapingReader::read(const deltaLogRecord &dlr,
                                 UINT8 &flags,
                                 UINT8 &count)
   {
      INT32 rc = SDB_OK;
      if (OSS_UNLIKELY(!dlr.isValid() ||
                       DELTA_LOG_TYPE_UNMAPPING != dlr.getLogHead()->_type))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      {
      const UINT8 *ptr = dlr.getRecordBodyPtr<UINT8>(0);
      if (NULL == ptr)
      {
         PD_LOG(PDERROR, "failed to get flag ptr");
         rc = SDB_VESSEL_INTERNAL_ERR;
         goto error;
      }
      flags = *ptr;
      }

      {
      const UINT8 *ptr = dlr.getRecordBodyPtr<UINT8>(sizeof(UINT8));
      if (NULL == ptr)
      {
         PD_LOG(PDERROR, "failed to get count ptr");
         rc = SDB_VESSEL_INTERNAL_ERR;
         goto error;
      }
      if (0 == *ptr)
      {
         PD_LOG(PDERROR, "invalid count found");
         rc = SDB_VESSEL_INTERNAL_ERR;
         goto error;
      }
      count = *ptr;
      }
   done:
      return rc;
   error:
      goto done;
   }

   INT32 dlrUnmapingReader::getItem(const deltaLogRecord &dlr,
                                    UINT8 i,
                                    mappedLogicalPageId &mappedId)
   {
      INT32 rc = SDB_OK;
      UINT32 offset = 0;
      UINT8 count = 0;
      UINT8 flags = 0;
      const UINT64 *ptr = NULL;

      rc = dlrUnmapingReader::read(dlr, flags, count);
      if (SDB_OK != rc)
      {
         goto error;
      }

      if (count <= i)
      {
         rc = SDB_OUT_OF_BOUND;
         goto error;
      }

      offset = sizeof(UINT8) + sizeof(UINT8);
      offset += ((UINT32)i << 3);
      ptr = dlr.getRecordBodyPtr<UINT64>(offset);
      if (NULL == ptr)
      {
         PD_LOG(PDERROR, "failed to mapped id ptr");
         rc = SDB_VESSEL_INTERNAL_ERR;
         goto error;
      }
      mappedId = *ptr;
      if (!mappedId.isValid())
      {
         PD_LOG(PDERROR, "invalid mapped id found");
         rc = SDB_VESSEL_INTERNAL_ERR;
         goto error;
      }
   done:
      return rc;
   error:
      goto done;
   }

   ////////////dlrReleasingReader
/*
   INT32 dlrReleasingReader::read(const deltaLogRecord &dlr, UINT8 &count)
   {
      INT32 rc = SDB_OK;
      const UINT8 *ptr = NULL;
      if (OSS_UNLIKELY(!dlr.isValid() ||
                       DELTA_LOG_TYPE_RELEASING != dlr.getLogHead()->_type))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      ptr = dlr.getRecordBodyPtr<UINT8>(0);
      if (NULL == ptr)
      {
         PD_LOG(PDERROR, "failed to get count ptr");
         rc = SDB_VESSEL_INTERNAL_ERR;
         goto error;
      }

      if (0 == *ptr)
      {
         PD_LOG(PDERROR, "invalid count found");
         rc = SDB_INVALIDARG;
         goto error;
      }

      count = *ptr;
   done:
      return rc;
   error:
      goto done;
   }

   INT32 dlrReleasingReader::getItem(const deltaLogRecord &dlr,
                                     UINT8 i,
                                     PAGE_ID &pid)
   {
      INT32 rc = SDB_OK;
      UINT8 count = 0;
      UINT32 offset = sizeof(UINT8) + ((UINT32)i << 2);
      const PAGE_ID *pidPtr = NULL;

      rc = dlrReleasingReader::read(dlr, count);
      if (SDB_OK != rc)
      {
         goto error;
      }
      else if (count <= i)
      {
         rc = SDB_OUT_OF_BOUND;
         goto error;
      }

      pidPtr = dlr.getRecordBodyPtr<PAGE_ID>(offset);
      if (NULL == pidPtr)
      {
         PD_LOG(PDERROR, "failed to get pid ptr");
         rc = SDB_VESSEL_INTERNAL_ERR;
         goto error;
      }
      else if (INVALID_PAGE_ID == *pidPtr)
      {
         PD_LOG(PDERROR, "invalid pid found");
         rc = SDB_VESSEL_INTERNAL_ERR;
         goto error;
      }

      pid = *pidPtr;

   done:
      return rc;
   error:
      goto done;
   }

   */
}//namespace vessel
}//namespace engine
