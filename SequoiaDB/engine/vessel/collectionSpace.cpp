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

   Source File Name = collectionSpace.cpp

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

#include "vessel/collectionSpace.h"
#include "ossLikely.hpp"
#include "pdTrace.hpp"
#include "ossUtil.hpp"
#include "vessel/requestContext.h"
#include "vessel/extentSUContainer.h"
#include "vessel/instanceEnv.h"
#include "vessel/smpAccessor.h"
#include "vessel/spaceManagementPage.h"
#include "vessel/csgpAccessor.h"
#include "vessel/extentStorageUnit.h"
#include "dpsOp2Record.hpp"
#include "vessel/redoLogUtil.h"
#include "vessel/outerResource.h"
#include "dpsLogRecord.hpp"
#include "vessel/idMapPage.h"
#include "vessel/impAccessor.h"
#include "vessel/collection.h"
#include "vessel/crpAccessor.h"
#include "vessel/listCLCursor.h"

namespace engine
{
namespace vessel
{
   collectionSpace::collectionSpace():
   _su(NULL)
   {
      
   }

   collectionSpace::~collectionSpace()
   {
      teardown();
   }

   INT32 collectionSpace::setup(requestContext *context,
                                extentStorageUnit *su)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(NULL != context, "can not be null");
      SDB_ASSERT(NULL != su, "can not be null");
      BOOLEAN rollback = FALSE;

      if (OSS_UNLIKELY(NULL == context ||
                       NULL == su))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      else if (OSS_UNLIKELY(INVALID_SPACE_ID == su->getSpaceID()))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      rollback = TRUE;
      rc = initMetaRecordFromDisk(context, su);
      if (SDB_OK != rc)
      {
         goto error;
      }

      rc = initCollectionsFromDisk(context, su);
      if (SDB_OK != rc)
      {
         goto error;
      }

      _su = su;
   done:
      return rc;
   error:
      if (rollback)
      {
         teardown();
      }
      goto done;
   }

   INT32 collectionSpace::teardown()
   {
      _recordInMem.reset();
      _collectionMap.teardown();
      _su = NULL;
      return SDB_OK;
   }

   SPACE_ID collectionSpace::getSpaceID()const
   {
      if (OSS_LIKELY(NULL != _su))
      {
         return _su->getSpaceID();
      }
      return INVALID_SPACE_ID;
   }

   INT32 collectionSpace::createCL(requestContext *context,
                                   const strSlice &clName, 
                                   UINT32 clLogicalID,
                                   const createCLOptions &options)
   {
      INT32 rc = SDB_OK;
      CL_MB_ID mbID = INVALID_CL_MB_ID;
      collectionMap::collectionHolder *holder = NULL;
      BOOLEAN rollback = FALSE;
      BOOLEAN mblocked = FALSE;
      SDB_ASSERT(NULL != context && !context->mbLocked(), "impossible");

      if (OSS_UNLIKELY(NULL == context ||
                       clName.empty() ||
                       DMS_INVALID_LOGICCLID == clLogicalID))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      else if (!context->getSpaceIDLocked() ||
                getSpaceID() != context->getSpaceID())
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      if (_collectionMap.clExists(clName, clLogicalID))
      {
         rc = SDB_DMS_EXIST;
         LOG_ERR_AND_REPORT(context, rc, "duplicated collection name[%s] or id[%d]", clName.str(), clLogicalID);
         goto error;
      }

      rc = _collectionMap.allocateMBID(mbID, &holder);
      if (SDB_OK != rc)
      {
         goto error;
      }

      rc = context->lockMB(mbID, holder->getMutex(), EXCLUSIVE);
      if (SDB_OK != rc)
      {
         goto error;
      }
      mblocked = TRUE;

      SDB_ASSERT(holder->isFree(), "must be free");
   
      rc = _collectionMap.createCLObject(clName, clLogicalID, mbID, options, this, *holder);
      if (SDB_DMS_EXIST == rc)
      {
         LOG_ERR_AND_REPORT(context, rc, "duplicated collection name[%s] or id[%d]", clName.str(), clLogicalID);
         goto error;
      }
      else if (SDB_OK != rc)
      {
         goto error;
      }
      rollback = TRUE;

      rc = holder->getCollection()->saveOnDiskWhenCreating(context);
      if (SDB_OK != rc)
      {
         goto error;
      }
      
      context->unlockMB();
   done:
      return rc;
   error:
      if (rollback)
      {
         _collectionMap.destoryCLObject(*holder);
      }
      if (mblocked)
      {
         context->unlockMB();
      }
      if (INVALID_CL_MB_ID != mbID)
      {
         _collectionMap.releaseMBID(mbID);
      }
      goto done;
   }

   INT32 collectionSpace::listCollections(requestContext *context,
                                          listCLCursor *cursor)
   {
      INT32 rc = SDB_OK;
      UINT32 logicalID = DMS_INVALID_LOGICCLID;
      CL_MB_ID nextMBID = INVALID_CL_MB_ID;
      UINT32 nextLogicalID = DMS_INVALID_LOGICCLID;
      collectionMap::collectionHolder *holder = NULL;
      listCollectionsRecord record;
      BOOLEAN locked = FALSE;
      ISession *session = NULL;
      UINT32 loop = 0;
      
      if (OSS_UNLIKELY(NULL == context && NULL == cursor))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      else if (OSS_UNLIKELY(!cursor->isOpen()))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      session = context->getSession();

      do
      {
         if (loop++ == 16)
         {
            if (session->quit())
            {
               rc = SDB_APP_INTERRUPT;
               goto error;
            }
            loop = 0;
         }

         logicalID = cursor->getLastCLID();
         
         rc = _collectionMap.upperBound(logicalID, nextMBID, nextLogicalID, &holder);
         if (SDB_DMS_NOTEXIST == rc)
         {
            rc = SDB_OK;
            cursor->pushEnd();
            goto done;
         }
         else if (SDB_OK != rc)
         {
            goto error;
         }

         rc = context->lockMB(nextMBID, holder->getMutex(), SHARED);
         if (SDB_OK != rc)
         {
            goto error;
         }
         locked = TRUE;

         /// some one dropped cl before we locked.
         /// just continue.
         if (holder->isFree())
         {
            context->unlockMB();
            locked = FALSE;
            continue;
         }

         /// some one dropped cl and recreated before we locked.
         /// just continue;
         if (holder->getCollection()->getLogicalID() != nextLogicalID)
         {
            context->unlockMB();
            locked = FALSE;
            continue;
         }

         rc = holder->getCollection()->dump(context, record);
         if (SDB_OK != rc)
         {
            goto error;
         }

         context->unlockMB();
         locked = FALSE;
         holder = NULL;
         SDB_ASSERT(INVALID_CL_MB_ID != record.mbID, "impossible");

         rc = cursor->push(sizeof(listCollectionsRecord), (const CHAR *)(&record));
         if (SDB_VESSEL_CURSOR_NO_SPACE == rc)
         {
            rc = SDB_OK;
            goto done;
         }
         else if (SDB_OK != rc)
         {
            goto error;
         }
         else
         {
            cursor->setLastCLID(record.logicalID);
            continue;
         }
         
      } while (TRUE);
      
   done:
      return rc;
   error:
      if (locked)
      {
         context->unlockMB();
      }
      goto done;
   }

   INT32 collectionSpace::dump(requestContext *context,
                               listCollectionSpaceRecord &record)
   {
      INT32 rc = SDB_OK;
      UINT32 pageSize = 0;
      UINT32 pageCountPerSeg = 0;
      if (OSS_UNLIKELY(NULL == context ||
                       !context->getSpaceIDLocked() ||
                       context->getSpaceID() != getSpaceID()))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      rc = _su->getCoreArgs(SPACE_TYPE_RECORD_D, &pageSize, &pageCountPerSeg, NULL);
      if (SDB_OK != rc)
      {
         goto error;
      }

      record.version = getVersion();
      ossMemcpy(record.name, getCSName(), DMS_COLLECTION_SPACE_NAME_SZ+1);
      record.logicalID = getLogicalID();
      record.spaceID = getSpaceID();
      record.status = getStatus();
      record.flags = getFlags();
      record.dataPageSize = pageSize;
      record.dataPageCountPerSeg = pageCountPerSeg;
      rc = getSU()->getCoreArgs(SPACE_TYPE_IDX_D, &pageSize, &pageCountPerSeg, NULL);
      if (SDB_OK != rc)
      {
         goto error;
      }
      record.idxPageSize = pageSize;
      record.idxPageCountPerSeg = pageCountPerSeg;
   done:
      return rc;
   error:
      goto done;
   }

   INT32 collectionSpace::getCollectionByLogicalID(requestContext *context,
                                                   UINT32 logicalID,
                                                   OSS_LATCH_MODE mode,
                                                   collection **obj)
   {
      INT32 rc = SDB_OK;
      collectionMap::collectionHolder *holder = NULL;
      CL_MB_ID mbid = INVALID_CL_MB_ID;
      BOOLEAN locked = FALSE;

      if (OSS_UNLIKELY(NULL == context ||
                       !context->getSpaceIDLocked() ||
                       getSpaceID() != context->getSpaceID() ||
                       DMS_INVALID_LOGICCLID == logicalID ||
                       NULL == obj))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      rc = _collectionMap.getCollection(logicalID, mbid, &holder);
      if (SDB_OK != rc)
      {
         goto error;
      }

      rc = context->lockMB(mbid, holder->getMutex(), mode);
      if (SDB_OK != rc)
      {
         goto error;
      }
      locked = TRUE;

      if (logicalID != holder->getCollection()->getLogicalID())
      {
         rc = SDB_DMS_NOTEXIST;
         goto error;
      }

      *obj = holder->getCollection();
   done:
      return rc;
   error:
      if (locked)
      {
         context->unlockMB();
      }
      goto done;
   }

   INT32 collectionSpace::getCollectionByMBID(requestContext *context,
                                              CL_MB_ID mbID,
                                              UINT32 logicalID,
                                              OSS_LATCH_MODE mode,
                                              collection **obj)
   {
      INT32 rc = SDB_OK;
      collectionMap::collectionHolder *holder = NULL;
      BOOLEAN locked = FALSE;
      if (OSS_UNLIKELY(NULL == context ||
                       INVALID_CL_MB_ID == mbID ||
                       !context->getSpaceIDLocked() ||
                       getSpaceID() != context->getSpaceID() ||
                       NULL == obj))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      rc = _collectionMap.getCollection(mbID, &holder);
      if (SDB_OK != rc)
      {
         goto error;
      }

      rc = context->lockMB(mbID, holder->getMutex(), mode);
      if (SDB_OK != rc)
      {
         goto error;
      }
      locked = TRUE;

      if (DMS_INVALID_LOGICCLID != logicalID &&
          holder->getCollection()->getLogicalID() != logicalID)
      {
         rc = SDB_DMS_NOTEXIST;
         goto error;
      }

      *obj = holder->getCollection();
   done:
      return rc;
   error:
      if (locked)
      {
         context->unlockMB();
      }
      goto done;
   }

   INT32 collectionSpace::initMetaRecordFromDisk(requestContext *context,
                                                 extentStorageUnit *su)
   {
      INT32 rc = SDB_OK;
      csgpAccessor accessor;
      UINT32 flags = PAGE_ACCESSOR_FLAG_DIRECT;
      rc = accessor.setup(context,
                          SPACE_TYPE_RECORD_M,
                          CS_GLOBAL_META_PAGE_ID,
                          flags, su);
      if (SDB_OK != rc)
      {
         goto error;
      }

      rc = accessor.readMetaRecord(_recordInMem);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to read meta record:%d", rc);
         goto error;
      }

   done:
      accessor.teardown();
      return rc;
   error:
      goto done;
   }

   INT32 collectionSpace::initCollectionsFromDisk(requestContext *context,
                                                  extentStorageUnit *su)
   {
      INT32 rc = SDB_OK;
      UINT32 pageSize = 0;
      impAccessor imp;
      ossValuePtr ptr = 0;
      PAGE_ID lpid = INVALID_PAGE_ID;
      UINT32 capacity = 0;
      UINT32 free = 0;
      INT32 count = 0;

      rc = su->getCoreArgs(SPACE_TYPE_RECORD_M, &pageSize);
      if (SDB_OK != rc)
      {
         goto error;
      }

      rc = su->getPagePtr(SPACE_TYPE_RECORD_M, SYSTEM_MAP_PAGE_ID, ptr);
      if (SDB_OK != rc)
      {
         goto error;
      }

      rc = imp.setup(context, SPACE_TYPE_RECORD_M, SYSTEM_MAP_PAGE_ID,
                     pageSize, ptr);
      if (SDB_OK != rc)
      {
         goto error;
      }

      rc = imp.getHeadContent(lpid, capacity, free);
      if (SDB_OK != rc)
      {
         goto error;
      }

      if (free == capacity)
      {
         goto done;
      }

      count = capacity - free;
      SDB_ASSERT(0 < count, "impossible");
      for (UINT32 i = 0; i < capacity && 0 < count; ++i)
      {
         PAGE_ID pid = INVALID_PAGE_ID;

         rc = imp.getPid(lpid, &pid, NULL);
         if (SDB_OK != rc)
         {
            goto error;
         }
         else if (INVALID_PAGE_ID == pid)
         {
            continue;
         }
         
         --count;
         rc = initCollectionsFromOneDiskPage(context, pid, su);
         if (SDB_OK != rc)
         {
            goto error;
         }
      }
      
   done:
      imp.teardown();
      return rc;
   error:
      goto done;
   }

   INT32 collectionSpace::initCollectionsFromOneDiskPage(requestContext *context,
                                                         PAGE_ID pid,
                                                         extentStorageUnit *su)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(NULL != context, "can not be null");
      SDB_ASSERT(INVALID_PAGE_ID != pid, "can not be invalid");
      UINT32 pageSize = 0;
      ossValuePtr ptr = 0;
      crpAccessor crp;
      UINT32 capacity = 0;
      UINT64 bitmap = 0;
      collectionRecord record;

      rc = su->getCoreArgs(SPACE_TYPE_RECORD_D, &pageSize);
      if (SDB_OK != rc)
      {
         goto error;
      }

      rc = su->getPagePtr(SPACE_TYPE_RECORD_D, pid, ptr);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to get crp page:%d", rc);
         goto error;
      }

      rc = crp.setup(context, SPACE_TYPE_RECORD_D, pid,
                     pageSize, ptr);
      if (SDB_OK != rc)
      {
         goto error;
      }

      rc = crp.getHeadContent(capacity, bitmap);
      if (SDB_OK != rc)
      {
         goto error;
      }

      for (UINT32 i = 0; i < capacity; ++i)
      {
         UINT32 slot = (UINT64)1 << i;
         if (OSS_BIT_TEST(bitmap, slot))
         {
            /// bit 1 means free
            continue;
         }

         rc = crp.getClRecordBySlot(i, record);
         if (SDB_OK != rc)
         {
            goto error;
         }

         rc = _collectionMap.initObjWhenStartup(this, record);
         if (SDB_OK != rc)
         {
            goto error;
         }
      }
   done:
      crp.teardown();
      return rc;
   error:
      goto done;
   }

}//namespace vessel
}//namespace engine