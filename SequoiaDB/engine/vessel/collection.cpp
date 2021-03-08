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
#include "vessel/storageUnit.h"
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
   _collectionSpace(NULL)
   {
      
   }

   collection::~collection()
   {

   }

   INT32 collection::initWhenOpen(const collectionRecord &record,
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
   done:
      return rc;
   error:
      goto done;
   }

   INT32 collection::create(requestContext *context,
                            const strSlice &clName,
                            utilCLInnerID innerID,
                            UINT32 logicalID,
                            collectionSpace *cs,
                            const createCLOptions &options)
   {
      INT32 rc = SDB_OK;
      BOOLEAN rollback = FALSE;
      if (OSS_UNLIKELY(NULL == context ||
                       !context->mbLocked() ||
                       EXCLUSIVE != context->getMBLockMode() ||
                       INVALID_CL_MB_ID == context->getMBID() ||
                       clName.empty() ||
                       DMS_INVALID_LOGICCLID == logicalID ||
                       NULL == cs ||
                       !options.isValid()))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      else if (NULL != _collectionSpace)
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      rollback = TRUE;
      _record.version = COLLECTION_RECORD_VERSION;
      _record.mbID = context->getMBID();
      _record.innerID = innerID;
      _record.type = options.type;
      _record.logicalCLID = logicalID;
      ossStrncpy(_record.name, clName.str(), clName.strLen());
      _record.maxSGCount = options.maxStripingGroupCount;
      _record.compressionType = options.compressionType;
      if (CL_COMPRESSION_TYPE_NONE != options.compressionType)
      {
         _record.compressionAlgrithm = options.compressionAlgrithm;
      }
      _collectionSpace = cs;

      rc = saveOnDiskWhenCreating(context);
      if (SDB_OK != rc)
      {
         goto error;
      }
   done:
      return rc;
   error:
      if (rollback)
      {
         fini();
      }
      goto done;
   }

   void collection::fini()
   {
      _record.reset();
      _collectionSpace = NULL;
      return;
   }

   INT32 collection::saveOnDiskWhenCreating(requestContext *context)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(NULL != context, "can not be null");
      SDB_ASSERT(context->getSpaceIDLocked(), "must be locked");
      SDB_ASSERT(context->getSpaceID() == _collectionSpace->getSpaceID(), "must be same");
      SDB_ASSERT(INVALID_CL_MB_ID != getMBID(), "can not be invlalid");
      PAGE_ID lpid = INVALID_PAGE_ID;
      PAGE_ID pid = INVALID_PAGE_ID;
      lpidLockHelper lh;

      rc = _collectionSpace->getLpidOfClRecord(getMBID(), lpid);
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
      ossMemcpy(record.name, _record.name, ossStrlen(_record.name) + 1);
      record.csUniqueID = _collectionSpace->getUniqueID();
      record.clInnerID = _record.innerID;
      record.clLogicalID = _record.logicalCLID;
      record.spaceID = _collectionSpace->getSpaceID();
      record.mbID = _record.mbID;
      record.maxSGCount = _record.maxSGCount;
      
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
      SDB_ASSERT(NULL != _collectionSpace, "can not be null");
      SDB_ASSERT(NULL != context, "can not be null");
      SDB_ASSERT(INVALID_PAGE_ID != lpid, "can not be invalid");
      SDB_ASSERT(context->testLpidLockMode(SPACE_TYPE_RECORD_D, lpid, EXCLUSIVE), "must holding lock");
      PAGE_ID cow = INVALID_PAGE_ID;

      rc = _collectionSpace->getPhyPidInDFile(context, lpid, pid, &cow);
      if (SDB_OK == rc)
      {
         goto done;
      }
      else if (SDB_VESSEL_LOGICAL_PAGE_UNMAPPED == rc)
      {
         rc = allocatePageForCLRecord(context, lpid, pid);
         if (SDB_OK != rc)
         {
            goto error;
         }
      }
      else if (SDB_VESSEL_PAGE_IN_SNAPSHOT == rc)
      {
         rc = _collectionSpace->copyOnWritePageInDFile(context, lpid, cow, pid);
         if (SDB_OK != rc)
         {
            goto error;
         }
      }
      else
      {
         PD_LOG(PDERROR, "failed to get phy pid of lpid[%d], rc:%d", lpid, rc);
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
      SDB_ASSERT(NULL != _collectionSpace, "can not be null");
      SDB_ASSERT(context->testLpidLockMode(SPACE_TYPE_RECORD_D, lpid, EXCLUSIVE), "must holding lock");

      rc = preallocateCLRecordPage(context, lpid, pid);
      if (SDB_OK != rc)
      {
         goto error;
      }

      rc = _collectionSpace->allocateDataPages(context, PAGE_TYPE_COLLECTION_RECORD,
                                               1, &lpid, &pid, slice());
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to allocate data pages:%d", rc);
         goto error;
      }
   done:
      return rc;
   error:
      if (INVALID_PAGE_ID != pid)
      {
         _collectionSpace->releaseDataPagesPreallocated(context, 1, &pid);
         pid = INVALID_PAGE_ID;
      }
      goto done;
   }

   INT32 collection::preallocateCLRecordPage(requestContext *context,
                                             PAGE_ID lpid,
                                             PAGE_ID &pid)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(NULL != context, "can not be null");
      SDB_ASSERT(INVALID_PAGE_ID != lpid, "can not be invalid");

      UINT32 pageSize = 0;
      ossValuePtr pagePtr = 0;
      crpAccessor crp;
      storageUnit *su = _collectionSpace->getSU();
      
      pid = INVALID_PAGE_ID;
   
      rc = su->getCoreArgs(SPACE_TYPE_RECORD_D, &pageSize);
      if (SDB_OK != rc)
      {
         goto error;
      }

      rc = _collectionSpace->preallocatePhyPagesInDFile(context, 1, &pid);
      if (SDB_OK != rc)
      {
         goto error;
      }

      SDB_ASSERT(INVALID_PAGE_ID != pid, "can not be invalid");

      rc = su->getPagePtr(SPACE_TYPE_RECORD_D, pid, pagePtr);
      if (SDB_OK != rc)
      {
         goto error;
      }

      rc = crp.initWithDirectMode(context, SPACE_TYPE_RECORD_D,
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

      crp.fini();

      rc = su->fsync(SPACE_TYPE_RECORD_D, pid, 1, TRUE);
      if (SDB_OK != rc)
      {
         goto error;
      }
   done:
      return rc;
   error:
      crp.fini();
      if (INVALID_PAGE_ID != pid)
      {
         _collectionSpace->releaseDataPagesPreallocated(context, 1, &pid);
         pid = INVALID_PAGE_ID;
      }
      goto done;
   }

   INT32 collection::saveCLRecordWhenCreating(requestContext *context, PAGE_ID pid)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(NULL != context, "can not be null");
      SDB_ASSERT(INVALID_PAGE_ID != pid, "can not be invalid");
      SDB_ASSERT(NULL != _collectionSpace, "can not be null");
      crpAccessor accessor;
      UINT32 flags = PAGE_ACCESSOR_FLAG_NON_READONLY;
      rc = accessor.init(context,
                          SPACE_TYPE_RECORD_D,
                          pid, flags, _collectionSpace->getSU());
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
      accessor.fini();
      return rc;
   error:
      goto done;
   }
}//namespace vessel
}//namespace engine