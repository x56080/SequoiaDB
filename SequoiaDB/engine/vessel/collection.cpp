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

   Source File Name = collection.cpp

   Descriptive Name =

   When/how to use: this program may be used on binary and text-formatted
   versions of PMD component. This file contains functions for agent processing.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#include "vessel/collection.h"
#include "pdTrace.hpp"
#include "ossUtil.hpp"
#include "ossLikely.hpp"
#include "vessel/requestContext.h"
#include "vessel/impAccessor.h"
#include "vessel/idMapPage.h"
#include "vessel/extentStorageUnit.h"
#include "dpsLogRecord.hpp"
#include "vessel/redoLogUtil.h"
#include "vessel/instanceEnv.h"
#include "vessel/crpAccessor.h"
#include "vessel/collectionSpace.h"
#include "dpsLogRecord.hpp"
#include "vessel/IRedoLogger.h"
#include "vessel/outerResource.h"
#include "vessel/lpidLockHelper.h"
#include "vessel/smpAccessor.h"

namespace engine
{
namespace vessel
{
   collection::collection():
   _collectionSpace(NULL),
   _su(NULL)
   {
      
   }

   collection::~collection()
   {

   }

   INT32 collection::setup(const strSlice &clName,
                           UINT32 logicalID,
                           CL_MB_ID mbID,
                           const createCLOptions &options,
                           collectionSpace *cs)
   {
      INT32 rc = SDB_OK;
      BOOLEAN rollback = FALSE;
      if (OSS_UNLIKELY(clName.empty() ||
                       DMS_INVALID_LOGICCLID == logicalID ||
                       INVALID_CL_MB_ID == mbID ||
                       NULL == cs))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      else if (OSS_UNLIKELY(INVALID_CL_MB_ID != _record.mbID))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      rollback = TRUE;
      _collectionSpace = cs;
      _su = _collectionSpace->getSU();
      _record.version = COLLECTION_RECORD_VERSION;
      _record.type = options.type;
      ossMemcpy(_record.name, clName.str(), clName.strLen() + 1);
      _record.logicalCLID = logicalID;
      _record.logicalCSID = _collectionSpace->getLogicalID();
      _record.mbID = mbID;
      _record.maxStripingGroup = options.maxStripingGroupCount;
      _record.compressionType = options.compressionType;
      if (CL_COMPRESSION_TYPE_NONE != options.compressionType)
      {
         _record.compressionAlgrithm = options.compressionAlgrithm;
      }
      rollback = FALSE;
   done:
      return rc;
   error:
      if (rollback)
      {
         _record.reset();
         _collectionSpace = NULL;
      }
      goto done;
   }

   INT32 collection::setup(const collectionRecord &record,
                           collectionSpace *cs)
   {
      INT32 rc = SDB_OK;
      if (OSS_UNLIKELY(NULL == cs))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      else if (OSS_UNLIKELY(COLLECTION_RECORD_VERSION != record.version))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      else if (OSS_UNLIKELY(INVALID_CL_MB_ID == record.mbID))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      
      _record = record;
      _collectionSpace = cs;
      _su = _collectionSpace->getSU();
   done:
      return rc;
   error:
      goto done;
   }

   INT32 collection::saveOnDiskWhenCreating(requestContext *context)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(NULL != context, "can not be null");
      SDB_ASSERT(context->getSpaceIDLocked(), "must be locked");
      SDB_ASSERT(context->getSpaceID() == _collectionSpace->getSpaceID(), "must be same");
      PAGE_ID lpid = INVALID_PAGE_ID;
      PAGE_ID pid = INVALID_PAGE_ID;
      
      lpidLockHelper lh;

      if (OSS_UNLIKELY(INVALID_CL_MB_ID == getMBID()))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      rc = _su->getLpidOfClRecord(getMBID(), lpid);
      if (OSS_UNLIKELY(SDB_OK != rc))
      {
         PD_LOG(PDERROR, "failed to get lpid");
         rc = SDB_VESSEL_INTERNAL_ERR;
         goto error;
      }

      rc = lh.lock(context, SPACE_TYPE_RECORD_D, lpid, EXCLUSIVE);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to lock lpid[%d], rc:%d", lpid, rc);
         goto error;
      }

      rc = ensureCLRecordPageAllocated(context, lpid, pid);
      if (SDB_OK != rc)
      {
         goto error;
      }

      rc = saveCLRecordWhenCreating(context, pid);
      if (SDB_OK != rc)
      {
         goto error;
      }

   done:
      lh.unlock();
      return rc;
   error:
      goto done;
   }

   INT32 collection::dump(requestContext *context,
                          listCollectionsRecord &record)
   {
      INT32 rc = SDB_OK;
      if (OSS_UNLIKELY(NULL == context))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      SDB_ASSERT(context->mbLocked(), "must locked");
      SDB_ASSERT(context->getMBID() == _record.mbID, "must be same");

      record.version = _record.version;
      record.logicalID = _record.logicalCLID;
      record.csLogicalID = _record.logicalCSID;
      record.spaceID = _collectionSpace->getSpaceID();
      record.mbID = _record.mbID;
      ossMemcpy(record.name, _record.name, ossStrlen(_record.name) + 1);
   done:
      return rc;
   error:
      goto done;
   }

   INT32 collection::ensureCLRecordPageAllocated(requestContext *context,
                                                 PAGE_ID lpid,
                                                 PAGE_ID &pid)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(NULL != context, "can not be null");
      SDB_ASSERT(INVALID_PAGE_ID != lpid, "can not be invalid");
      SDB_ASSERT(context->testLpidLockMode(SPACE_TYPE_RECORD_D, lpid, EXCLUSIVE), "must holding lock");
      SNAPSHOT_ID snap = INVALID_SNAPSHOT_ID;
      snapshotContainer &snapshot = context->getEnv()->snapContainer;

      rc = _su->getDataPhysicalPid(context, lpid, pid, snap);
      if (SDB_OK == rc)
      {
         if (snapshot.contains(snap, _su->getSpaceID()))
         {
            SDB_ASSERT(FALSE, "to do");
         }
      }
      else if (SDB_VESSEL_LOGICAL_PAGE_UNMAPPED == rc)
      {
         rc = allocatePageForCLRecord(context, lpid, pid);
         if (SDB_OK != rc)
         {
            goto error;
         }
      }
      else
      {
         goto error;
      }

   done:
      return rc;
   error:
      goto done;
   }

   INT32 collection::allocatePageForCLRecord(requestContext *context,
                                             PAGE_ID lpid,
                                             PAGE_ID &pid)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(NULL != context, "can not be null");
      SDB_ASSERT(INVALID_PAGE_ID != lpid, "can not be invalid");
      SDB_ASSERT(NULL != _su, "can not be null");
      DPS_LSN_OFFSET oplist = DPS_INVALID_LSN_OFFSET;

      rc = preallocateCLRecordPage(context, lpid, pid);
      if (SDB_OK != rc)
      {
         goto error;
      }

      rc = allocateCLRecordPageOnSMP(context, lpid, pid, oplist);
      if (SDB_OK != rc)
      {
         goto error;
      }

      SDB_ASSERT(DPS_INVALID_LSN_OFFSET != oplist, "can not be invalid");

      rc = _su->remapLpid(context, lpid, pid, &oplist);
      if (SDB_OK != rc)
      {
         /// we do not need to rollback lpid mapping.
         /// just leave it there for next ddl request.
         releaseCLRecordPageOnSMP(context, pid, &oplist);
         goto error;
      }
   done:
      return rc;
   error:
      if (INVALID_PAGE_ID != pid)
      {
         _su->releaseDataPagesPreallocated(context, 1, &pid);
         pid = INVALID_PAGE_ID;
      }
      
      goto done;
   }

   INT32 collection::allocateCLRecordPageOnSMP(requestContext *context,
                                               PAGE_ID lpid,
                                               PAGE_ID pid,
                                               DPS_LSN_OFFSET &oplist)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(NULL != context, "can not be null");
      SDB_ASSERT(INVALID_PAGE_ID != lpid, "can not be invalid");
      SDB_ASSERT(INVALID_PAGE_ID != pid, "can not be invalid");
      UINT32 flags = PAGE_ACCESSOR_FLAG_NON_READONLY;
      DPS_LSN_OFFSET lsn = DPS_INVALID_LSN_OFFSET;
      PAGE_ID smpPid = INVALID_PAGE_ID;
      smpAccessor smp;

      rc = _su->getDataSMPPId(pid, smpPid);
      if (SDB_OK != rc)
      {
         goto error;
      }

      rc = smp.setup(context, SPACE_TYPE_RECORD_D,
                     smpPid, flags, _su);
      if (SDB_OK != rc)
      {
         goto error;
      }

      rc = smp.allocateCLRecordPage(lpid, pid, &lsn);
      if (SDB_OK != rc)
      {
         goto error;
      }

      oplist = lsn;
   done:
      smp.teardown();
      return rc;
   error:
      goto done;
   }

   INT32 collection::releaseCLRecordPageOnSMP(requestContext *context,
                                              PAGE_ID pid,
                                              const DPS_LSN_OFFSET *oplist)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(FALSE, "todo");
   done:
      return rc;
   error:
      goto done;
   }

   INT32 collection::preallocateCLRecordPage(requestContext *context,
                                             PAGE_ID lpid,
                                             PAGE_ID &pid)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(NULL != context, "can not be null");
      SDB_ASSERT(INVALID_PAGE_ID != lpid, "can not be invalid");
      SDB_ASSERT(NULL != _su, "can not be null");

      UINT32 pageSize = 0;
      ossValuePtr pagePtr = 0;
      crpAccessor crp;
      
      pid = INVALID_PAGE_ID;
   
      rc = _su->getCoreArgs(SPACE_TYPE_RECORD_D, &pageSize);
      if (SDB_OK != rc)
      {
         goto error;
      }

      rc = _su->preallocateDataPages(context, 1, &pid);
      if (SDB_OK != rc)
      {
         goto error;
      }

      SDB_ASSERT(INVALID_PAGE_ID != pid, "can not be invalid");

      rc = _su->getPagePtr(SPACE_TYPE_RECORD_D, pid, pagePtr);
      if (SDB_OK != rc)
      {
         goto error;
      }

      rc = crp.setup(context, SPACE_TYPE_RECORD_D,
                     pid, pageSize, pagePtr, FALSE, FALSE);
      if (SDB_OK != rc)
      {
         goto error;
      }

      rc = crp.initPage(lpid);
      if (SDB_OK != rc)
      {
         goto error;
      }

      crp.teardown();

      rc = _su->fsync(SPACE_TYPE_RECORD_D, pid, 1, TRUE);
      if (SDB_OK != rc)
      {
         goto error;
      }
   done:
      return rc;
   error:
      crp.teardown();
      if (INVALID_PAGE_ID != pid)
      {
         _su->releaseDataPagesPreallocated(context, 1, &pid);
         pid = INVALID_PAGE_ID;
      }
      goto done;
   }

   INT32 collection::saveCLRecordWhenCreating(requestContext *context, PAGE_ID pid)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(NULL != context, "can not be null");
      SDB_ASSERT(INVALID_PAGE_ID != pid, "can not be invalid");
      crpAccessor accessor;
      UINT32 flags = PAGE_ACCESSOR_FLAG_NON_READONLY;
      rc = accessor.setup(context,
                          SPACE_TYPE_RECORD_D,
                          pid, flags, _su);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to setup crp accessor:%d", rc);
         goto error;
      }

      rc = accessor.createCL(_collectionSpace->getCSName(), _record);
      if (SDB_OK != rc)
      {
         goto error;
      }
   done:
      accessor.teardown();
      return rc;
   error:
      goto done;
   }
}//namespace vessel
}//namespace engine