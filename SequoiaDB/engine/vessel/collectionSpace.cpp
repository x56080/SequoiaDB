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
#include "vessel/instanceEnv.h"
#include "vessel/smpAccessor.h"
#include "vessel/spaceManagementPage.h"
#include "vessel/csgpAccessor.h"
#include "vessel/storageUnit.h"
#include "dpsOp2Record.hpp"
#include "vessel/redoLogUtil.h"
#include "vessel/outerResource.h"
#include "dpsLogRecord.hpp"
#include "vessel/idMapPage.h"
#include "vessel/impAccessor.h"
#include "vessel/collection.h"
#include "vessel/crpAccessor.h"
#include "vessel/listCLCursor.h"
#include "vessel/IRedoLogger.h"
#include "vessel/fsmFile.h"
#include "vessel/bitMapUtils.h"
#include "vessel/fsmFile.h"

namespace engine
{
namespace vessel
{
   collectionSpace::collectionSpace():
   _status(CLOSED),
   _su(NULL),
   _capacityOfCLRecordPage(0),
   _idMapCapacity(0),
   _maxPageCountPerDataFile(0),
   _maxPageCountPerMetaFile(0),
   _pageAllocatedInMetaSMP(0)
   {
      
   }

   collectionSpace::~collectionSpace()
   {
      fini();
   }

   INT32 collectionSpace::create(requestContext *context,
                                 const strSlice &name,
                                 utilCSUniqueID uniqueID,
                                 UINT32 logicalID,
                                 const createCSOptions &options)
   {
      INT32 rc = SDB_OK;
      createSUOptions suOptions;
      static const UINT64 maxFileSize = 0x04ull * 1024 * 1024 * 1024;
      SDB_ASSERT(isClosed(), "do not recreate");
      BOOLEAN rollback = FALSE;

      if (OSS_UNLIKELY(NULL == context ||
                       !context->getSpaceIDLocked() ||
                       name.empty() ||
                       DMS_INVALID_LOGICCSID == logicalID))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      suOptions.csName = name;
      suOptions.sid = context->getSpaceID();
      suOptions.logicalID = logicalID;
      suOptions.metaArgs.pageSize = DMS_PAGE_SIZE32K;
      suOptions.metaArgs.maxPageCountPerSeg = 64;
      suOptions.metaArgs.maxSegmentCountPerFile = 2048;
      suOptions.idxMetaArgs.pageSize = DMS_PAGE_SIZE32K;
      suOptions.idxMetaArgs.maxPageCountPerSeg = 64;
      suOptions.idxMetaArgs.maxSegmentCountPerFile = 2048;

      suOptions.dataArgs.pageSize = options.dataPageSize;
      suOptions.dataArgs.maxPageCountPerSeg = options.dataPageCountPerSegment;
      suOptions.dataArgs.maxSegmentCountPerFile = maxFileSize / options.dataPageSize /options.dataPageCountPerSegment;
      suOptions.idxArgs.pageSize = options.idxPageSize;
      suOptions.idxArgs.maxPageCountPerSeg = options.idxPageCountPerSegment;
      suOptions.idxArgs.maxSegmentCountPerFile = maxFileSize / options.idxPageSize / options.idxPageCountPerSegment;


      _su = SDB_OSS_NEW storageUnit();
      if (NULL == _su)
      {
         PD_LOG(PDERROR, "failed to allocate mem");
         rc = SDB_OOM;
         goto error;
      }

      rc = _su->create(context, suOptions);
      if (SDB_OK != rc)
      {
         goto error;
      }
      _status = SU_LOADED;
      rollback = TRUE;
      
      _recordInMem.version = CMR_VERSION_1;
      _recordInMem.status = CMR_STATUS_CREATING;
      _recordInMem.flags = options.flags;
      _recordInMem.maxCLLogicalID = DMS_INVALID_LOGICCSID;
      _recordInMem.uniqueID = uniqueID;
      _recordInMem.csLogicalID = logicalID;
      ossMemcpy(_recordInMem.name, name.str(), name.strLen());

      rc = initNecessaryPagesWhenCreating(context);
      if (SDB_OK != rc)
      {
         goto error;
      }

      rc = cacheKeyParametersAboutStorage();
      if (SDB_OK != rc)
      {
         goto error;
      }

      rc = updateStatusToOnlineWhenCreating(context);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to set cs status to online:%d", rc);
         goto error;
      }

      /// The name file is only for easier viewing. Even if failed
      /// to write file, it does not affect the result.
      _su->ensureSUNameFile(context, strSlice(_recordInMem.name));
   done:
      return rc;
   error:
      if (rollback)
      {
         destroy(context);
      }
      goto done;
   }

   INT32 collectionSpace::destroy(requestContext *context)
   {
      INT32 rc = SDB_OK;
      if (!suIsLoaded())
      {
         goto done;
      }

      if (NULL != _su)
      {
         rc = _su->destroy(context);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to destroy collection spacep[%s], rc:%d", _recordInMem.name, rc);
            /// do not goto error.
         }
      }

      fini();
   done:
      return rc;
   error:
      goto done;
   }

   void collectionSpace::close(requestContext *context)
   {
      if (!isClosed())
      {
         if (NULL != _su)
         {
            _su->close(context);
         }
         fini();
      }
      return;
   }

   INT32 collectionSpace::openAfterSULoaded(requestContext *context)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(SU_LOADED == _status, "must be su_loaded");
      if (OSS_UNLIKELY(NULL == context))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      else if (OSS_UNLIKELY(SU_LOADED != _status))
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }

      rc = cacheGlobalMetaData(context);
      if (SDB_OK != rc)
      {
         goto error;
      }

      rc = initCollectionsFromDisk(context);
      if (SDB_OK != rc)
      {
         goto error;
      }

      _status = OPEN;
   done:
      return rc;
   error:
      _recordInMem.reset();
      goto done;
   }

   INT32 collectionSpace::openSU(requestContext *context,
                                 const strSlice &dirName)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(isClosed(), "must be closed");
      if (OSS_UNLIKELY(NULL == context ||
                       dirName.empty() ||
                       MAX_SU_DIR_LEN < dirName.strLen()))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      else if (OSS_UNLIKELY(!isClosed()))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      _su = SDB_OSS_NEW storageUnit();
      if (NULL == _su)
      {
         PD_LOG(PDERROR, "failed to allocate mem");
         rc = SDB_OOM;
         goto error;
      }

      rc = _su->open(context, dirName);
      if (SDB_OK != rc)
      {
         goto error;
      }

      rc = cacheKeyParametersAboutStorage();
      if (SDB_OK != rc)
      {
         goto error;
      }
      _status = SU_LOADED;
   done:
      return rc;
   error:
      close(context);
      goto done;
   }

   void collectionSpace::fini()
   {
      _status = CLOSED;
      SAFE_OSS_DELETE(_su);
      _recordInMem.reset();
      _capacityOfCLRecordPage = 0;
      _idMapCapacity = 0;
      _maxPageCountPerDataFile = 0;
      _maxPageCountPerMetaFile = 0;
      _pageAllocatedInMetaSMP = 0;
      
      _inMemDataSMP.fini();
      _inMemLpidPool.fini();
      _clNameIndex.clear();
      _clIdIndex.clear();
      _creatingNameIndex.clear();
      _creatingIdIndex.clear();
      _collectionAllocator.fini();

      return;
   }

   SPACE_ID collectionSpace::getSpaceID()const
   {
      if (OSS_LIKELY(NULL != _su))
      {
         return _su->getSpaceID();
      }
      return INVALID_SPACE_ID;
   }

   INT32 collectionSpace::getDataPageSize(UINT32 &pageSize)
   {
      INT32 rc = SDB_OK;
      if (OSS_UNLIKELY(NULL == _su))
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }

      rc = _su->getCoreArgs(SPACE_TYPE_RECORD_D, &pageSize);
      if (OSS_UNLIKELY(SDB_OK != rc))
      {
         goto error;
      }
   done:
      return rc;
   error:
      goto done;
   }

   INT32 collectionSpace::createCL(requestContext *context,
                                   const strSlice &clName, 
                                   utilCLInnerID clInnerId,
                                   const createCLOptions &options)
   {
      INT32 rc = SDB_OK;
      CL_MB_ID mbID = INVALID_CL_MB_ID;
      UINT32 logicalID = DMS_INVALID_LOGICCLID;
      collectionAllocator::collectionHolder *holder = NULL;

      BOOLEAN rollbackIndex = FALSE;
      SDB_ASSERT(NULL != context, "can not be null");
      SDB_ASSERT(context->getSpaceIDLocked(), "must be locked");
      SDB_ASSERT(getSpaceID() == context->getSpaceID(), "must be same");
      SDB_ASSERT(!context->mbLocked(), "can not be locked");

      if (OSS_UNLIKELY(NULL == context ||
                       clName.empty()))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      else if (!options.isValid())
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      if (!addToCreatingIndex(clName, clInnerId))
      {
         rc = SDB_DMS_EXIST;
         goto error;
      }
      rollbackIndex = TRUE;

      rc = _collectionAllocator.allocateNewMB(mbID, &holder);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to allocate new mb:%d", rc);
         goto error;
      }

      rc = context->lockMB(mbID, holder->getMutex(), EXCLUSIVE);
      if (SDB_OK != rc)
      {
         goto error;
      }

      SDB_ASSERT(!holder->isFree(), "can not be free");

      rc = allocateCLLogicalID(context, logicalID);
      if (SDB_OK != rc)
      {
         goto error;
      }

      rc = holder->getCollection()->create(context, clName, clInnerId,
                                           logicalID, this, options);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to create cl[%s] on disk:%d", clName.str(), rc);
         goto error;
      }

      moveToFormalIndex(holder);
      context->unlockMB();
   done:
      return rc;
   error:
      context->unlockMB();
      if (INVALID_CL_MB_ID != mbID)
      {
         _collectionAllocator.releaseMB(mbID);
      }
      if (rollbackIndex)
      {
         rollbackCreatingIndex(clName, clInnerId);
      }
      goto done;
   }

   UINT32 collectionSpace::getCollectionCount()
   {
      ossScopedLock lock(&_indexLatch, SHARED);
      return _clNameIndex.size();
   }

   INT32 collectionSpace::listCollections(requestContext *context,
                                          listCLCursor *cursor)
   {
      INT32 rc = SDB_OK;
      static const UINT32 NAME_BUFF_SIZE = DMS_COLLECTION_NAME_SZ + 1;
      CHAR clName[NAME_BUFF_SIZE] = {0};
      collectionAllocator::collectionHolder *holder = NULL;
      listCollectionsRecord record;
      BOOLEAN locked = FALSE;
      ISession *session = NULL;
      UINT32 loop = 0;
      static const UINT32 quitCheck = 16;
      
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
         UINT32 lid = DMS_INVALID_LOGICCLID;
         strSlice nameSlice(cursor->getCLName());
         if (loop++ == quitCheck)
         {
            if (session->quit())
            {
               rc = SDB_APP_INTERRUPT;
               goto error;
            }
            loop = 0;
         }

         if (!upperBoundCLName(nameSlice, NAME_BUFF_SIZE, clName, lid, &holder))
         {
            cursor->pushEnd();
            goto done;
         }

         rc = context->lockMB(holder->getMBId(), holder->getMutex(), SHARED);
         if (OSS_UNLIKELY(SDB_OK != rc))
         {
            rc = SDB_VESSEL_INTERNAL_ERR;
            PD_LOG(PDERROR, "failed to get mb lock:%d", rc);
            goto error;
         }
         locked = TRUE;

         /// some one dropped cl before we locked.
         /// just continue.
         if (holder->isFree())
         {
            context->unlockMB();
            locked = FALSE;
            cursor->setCLName(clName);
            continue;
         }

         /// some one dropped cl and mb was reallocated
         /// just continue;
         if (lid != holder->getCollection()->getLogicalID())
         {
            context->unlockMB();
            locked = FALSE;
            cursor->setCLName(clName);
            continue;
         }

         /// some one renamed cl.
         /// just continue;
         if (cursor->isPushed(holder->getCollection()->getLogicalID()))
         {
            context->unlockMB();
            locked = FALSE;
            cursor->setCLName(clName);
            continue;
         }

         holder->getCollection()->dump(context, record);
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
            cursor->setCLName(clName);
            cursor->markLIdPushed(lid);
            if (cursor->hasNoSpaceToPush(sizeof(listCollectionsRecord)))
            {
               break;
            }
            continue;
         }
         
      } while (TRUE);
      
   done:
      if (locked)
      {
         context->unlockMB();
      }
      return rc;
   error:
      
      goto done;
   }

   INT32 collectionSpace::ensureFsmFile(requestContext *context,
                                        fsmFile **file)
   {
      INT32 rc = SDB_OK;
      BOOLEAN sparse = FALSE;
      if (OSS_UNLIKELY(NULL == file))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      else if (OSS_UNLIKELY(isClosed()))
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }

      if (NULL == _su->getFsmFilePtr())
      {
         ossScopedLock guard(&_dataAndMetaSpaceLatch);
         if (NULL == _su->getFsmFilePtr())
         {
            rc = _su->createFsmFile(context);
            if (SDB_OK != rc)
            {
               PD_LOG(PDERROR, "failed to create fsm file:%d", rc);
               goto error;
            }
         }
      }

      if (!_su->getFsmFilePtr()->isReadyToWork())
      {
         sparse = context->getEnv()->options.extendFileWithSparse;
         rc = _su->getFsmFilePtr()->initToWork(sparse);
         if (SDB_OK != rc)
         {
            goto error;
         }
      }

      *file = _su->getFsmFilePtr();
   done:
      return rc;
   error:
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
      record.uniqueID = getUniqueID();
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

   INT32 collectionSpace::getCollectionByName(requestContext *context,
                                              const strSlice &clName, 
                                              OSS_LATCH_MODE mode,
                                              collection **obj)
   {
      INT32 rc = SDB_OK;
      collectionAllocator::collectionHolder *holder = NULL;
      UINT32 logicalID = DMS_INVALID_LOGICCLID;
      BOOLEAN locked = FALSE;

      if (OSS_UNLIKELY(NULL == context ||
                       !context->getSpaceIDLocked() ||
                       getSpaceID() != context->getSpaceID() ||
                       clName.empty() ||
                       NULL == obj))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      if (!findCollection(clName, logicalID, &holder))
      {
         rc = SDB_DMS_NOTEXIST;
         goto error;
      }

      SDB_ASSERT(DMS_INVALID_LOGICCLID != logicalID, "can not be invalid");

      rc = context->lockMB(holder->getMBId(), holder->getMutex(), mode);
      if (SDB_OK != rc)
      {
         goto error;
      }
      locked = TRUE;

      /// some one dropped cl
      if (holder->isFree())
      {
         rc = SDB_DMS_NOTEXIST;
         goto error;
      }

      /// some one dropped cl and mb was reallocated.
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
      collectionAllocator::collectionHolder *holder = NULL;
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

      rc = _collectionAllocator.getHolder(mbID, holder);
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

      if (holder->isFree())
      {
         rc = SDB_DMS_NOTEXIST;
         goto error;
      }

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

   INT32 collectionSpace::getLpidOfClRecord(CL_MB_ID mbID, PAGE_ID &lpid)const
   {
      INT32 rc = SDB_OK;
      if (OSS_UNLIKELY(INVALID_CL_MB_ID == mbID))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      else if (OSS_UNLIKELY(0 == _capacityOfCLRecordPage))
      {
         rc = SDB_VESSEL_INTERNAL_ERR;
         goto error;
      }

      SDB_ASSERT(0 < _capacityOfCLRecordPage, "can not be zero");
      lpid = mbID / _capacityOfCLRecordPage;
   done:
      return rc;
   error:
      goto done;
   }

   INT32 collectionSpace::getDataPhyPidInIdMapToRead(requestContext *context,
                                                     PAGE_ID lpid,
                                                     PAGE_ID &ppid)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(NULL != _su, "can not be null");
      PAGE_ID pid = INVALID_PAGE_ID;
      impAccessor imp;

      if (OSS_UNLIKELY(NULL == context ||
                       INVALID_PAGE_ID == lpid))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      else if (!context->testLpidLocked(SPACE_TYPE_RECORD_D, lpid))
      {
         rc = SDB_VESSEL_FORBIDDEN_OP_WLT;
         goto error;
      }

      pid = getDataIMPPid(lpid);
      if (INVALID_PAGE_ID == pid)
      {
         PD_LOG(PDERROR, "failed to get pid of imp");
         rc = SDB_VESSEL_INTERNAL_ERR;
         goto error;
      }
      
      rc = imp.init(context,
                    SPACE_TYPE_RECORD_M,
                    pid,
                    PAGE_ACCESSOR_FLAG_NONE,
                    _su);
      if (SDB_OK != rc)
      {
         goto error;
      }

      rc = imp.getPidByLpid(lpid, &ppid, NULL);
      if (SDB_OK != rc)
      {
         goto error;
      }

      if (INVALID_PAGE_ID == ppid)
      {
         rc = SDB_VESSEL_LOGICAL_PAGE_UNMAPPED;
         goto error;
      }
   done:
      imp.fini(context);
      return rc;
   error:
      ppid = INVALID_PAGE_ID;
      goto done;
   }

   INT32 collectionSpace::getDataPhyPidInIdMapToWrite(requestContext *context,
                                                      PAGE_ID lpid,
                                                      PAGE_ID &pid)
   {
      INT32 rc = SDB_OK;
      BOOLEAN needCow = FALSE;
      PAGE_ID newPid = INVALID_PAGE_ID;

      rc = getDataPhyPidInIdMapToWrite(context, lpid, pid, needCow);
      if (SDB_OK != rc)
      {
         goto error;
      }
      else if (!needCow)
      {
         goto done;
      }

      rc = copyOnWritePageInDFile(context, lpid, pid, newPid);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to copy lpid[%d] to new physical page:%d", lpid, rc);
         goto error;
      }

      pid = newPid;
   done:
      return rc;
   error:
      goto done;
   }

   INT32 collectionSpace::getDataPhyPidInIdMapToWrite(requestContext *context,
                                                      PAGE_ID lpid,
                                                      PAGE_ID &ppid,
                                                      BOOLEAN &mustBeCow)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(NULL != _su, "can not be null");
      PAGE_ID pid = INVALID_PAGE_ID;
      SNAPSHOT_ID snap = INVALID_SNAPSHOT_ID;
      snapshotContainer *snapshot = NULL;
      impAccessor imp;
      mustBeCow = FALSE;

      if (OSS_UNLIKELY(NULL == context ||
                       INVALID_PAGE_ID == lpid))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      else if (!context->testLpidLockMode(SPACE_TYPE_RECORD_D, lpid, EXCLUSIVE))
      {
         rc = SDB_VESSEL_FORBIDDEN_OP_WLT;
         goto error;
      }

      snapshot = &(context->getEnv()->snapContainer);

      pid = getDataIMPPid(lpid);
      if (INVALID_PAGE_ID == pid)
      {
         PD_LOG(PDERROR, "failed to get pid of imp");
         rc = SDB_VESSEL_INTERNAL_ERR;
         goto error;
      }
      
      rc = imp.init(context,
                     SPACE_TYPE_RECORD_M,
                     pid,
                     PAGE_ACCESSOR_FLAG_NONE,
                     _su);
      if (SDB_OK != rc)
      {
         goto error;
      }

      rc = imp.getPidByLpid(lpid, &ppid, &snap);
      if (SDB_OK != rc)
      {
         goto error;
      }

      if (INVALID_PAGE_ID == ppid)
      {
         rc = SDB_VESSEL_LOGICAL_PAGE_UNMAPPED;
         goto error;
      }

      SDB_ASSERT(INVALID_SNAPSHOT_ID != snap, "can not be invalid");
      if (snapshot->contains(snap, getSpaceID()))
      {
         mustBeCow = TRUE;
         goto error;
      }

   done:
      imp.fini(context);
      return rc;
   error:
      ppid = INVALID_PAGE_ID;
      goto done;
   }

   INT32 collectionSpace::copyOnWritePageInDFile(requestContext *context,
                                                 PAGE_ID lpid,
                                                 PAGE_ID toBeCow,
                                                 PAGE_ID &pid)
   {
      SDB_ASSERT(FALSE, "todo");
      return -1;
   }

   INT32 collectionSpace::preallocatePhyPagesInDFile(requestContext *context,
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

      if (!isInMemBitMapsReady())
      {
         rc = initInMemBitMaps(context);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to init in memory bit map:%d", rc);
            goto error;
         }
      }

      do
      {
         UINT32 pageCount = _inMemDataSMP.getPageCount();
         rc = _inMemDataSMP.allocateBits(count, pids);
         if (SDB_OK == rc)
         {
            /// pids already sorted.
            PAGE_ID pid = pids[count - 1];
            rc = _su->ensureDataFileSpace(context, pid);
            if (SDB_OK != rc)
            {
               _inMemDataSMP.releaseBits(count, pids);
               goto error;
            }
            goto done;
         }
         else if (SDB_VESSEL_NOT_ENOUGH_FREE_RESOURCE != rc)
         {
            PD_LOG(PDERROR, "failed to allocate page from bitmap:%d", rc);
            goto error;
         }
         else
         {
            rc = createNewDataFileAndExtendBitMap(context, &pageCount);
            if (SDB_OK != rc)
            {
               PD_LOG(PDERROR, "failed to create new data file:%d", rc);
               goto error;
            }
            continue;
         }
         
      } while (TRUE);
      
   done:
      return rc;
   error:
      goto done;
   }

   void collectionSpace::releasePhyPagesPreallocated(requestContext *context,
                                                      UINT32 count,
                                                      const PAGE_ID *pids)
   {
      _inMemDataSMP.releaseBits(count, pids);
   }

   void collectionSpace::releaseDataPagesPreallocated(requestContext *context,
                                                      UINT32 count,
                                                      const PAGE_ID *lpids,
                                                      const PAGE_ID *pids)
   {
      SDB_ASSERT(0 < count, "can not be zero");
      SDB_ASSERT(NULL != lpids && NULL != pids, "can not be null");
      _inMemLpidPool.releaseBits(count, lpids);
      _inMemDataSMP.releaseBits(count, pids);
      return;
   }

   INT32 collectionSpace::preallocateDataPages(requestContext *context,
                                               UINT32 count,
                                               PAGE_ID *lpids,
                                               PAGE_ID *pids)
   {
      INT32 rc = SDB_OK;
      BOOLEAN rollback = FALSE;
      if (OSS_UNLIKELY(NULL == context ||
                       0 == count ||
                       NULL == lpids ||
                       NULL == pids))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      rc = preallocateLpids(context, count, lpids);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to preallocate lpid:%d", rc);
         goto error;
      }

      rollback = TRUE;

      rc = preallocatePhyPagesInDFile(context, count, pids);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to preallocate page:%d", rc);
         goto error;
      }
   done:
      return rc;
   error:
      if (rollback)
      {
         releaseLpidsPreallocated(context, count, lpids);
      }
      goto done;
   }

   INT32 collectionSpace::preallocateLpids(requestContext *context,
                                           UINT32 count,
                                           PAGE_ID *lpids)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(0 != _idMapCapacity, "can not be invalid");

      if (OSS_UNLIKELY(NULL == context ||
                       0 == count ||
                       PAGE_COUNT_IN_EXTENT < count ||
                       NULL == lpids))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      if (OSS_UNLIKELY(!isInMemBitMapsReady()))
      {
         rc = initInMemBitMaps(context);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to init in memory bit map:%d", rc);
            goto error;
         }
      }

      do
      {
         UINT32 pageCount = _pageAllocatedInMetaSMP;
         rc = _inMemLpidPool.allocateBits(count, lpids);
         if (SDB_OK == rc)
         {
            goto done;
         }
         else if (SDB_VESSEL_NOT_ENOUGH_FREE_RESOURCE != rc)
         {
            PD_LOG(PDERROR, "failed to allocate lpid from bitmap:%d", rc);
            goto error;
         }
         else
         {
            rc = ensureNewIMPAndExtendPool(context, pageCount);
            if (SDB_OK != rc)
            {
               PD_LOG(PDERROR, "failed to ensure new imp:%d", rc);
               goto error;
            }
            continue;
         }
      } while (TRUE);
      
   done:
      return rc;
   error:
      goto done;
   }

   void collectionSpace::releaseLpidsPreallocated(requestContext *context,
                                                  UINT32 count,
                                                  PAGE_ID *lpids)
   {
      SDB_ASSERT(NULL != context, "can not be null");
      SDB_ASSERT(0 < count, "can not be zero");
      SDB_ASSERT(NULL != lpids, "can not be null");
      _inMemLpidPool.releaseBits(count, lpids);
   }

   INT32 collectionSpace::allocateDataPages(requestContext *context,
                                             PAGE_TYPE type,
                                             UINT32 count,
                                             const PAGE_ID *lpids,
                                             const PAGE_ID *pids,
                                             const slice &args,
                                             DPS_LSN_OFFSET *oplist)
   {
      INT32 rc = SDB_OK;
      DPS_LSN_OFFSET oplistLsn = DPS_INVALID_LSN_OFFSET;
      BOOLEAN oplistTail = NULL == oplist;
      PAGE_ID smpPid = INVALID_PAGE_ID;
      PAGE_ID impPid = INVALID_PAGE_ID;

      if (OSS_UNLIKELY(NULL == context ||
                       INVALID_PAGE_TYPE == type ||
                       0 == count ||
                       NULL == lpids ||
                       NULL == pids))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      smpPid = getDataSMPPId(pids[0]);
      if (INVALID_PAGE_ID == smpPid)
      {
         PD_LOG(PDERROR, "failed to pid of smp, data pid[%d]", pids[0]);
         rc = SDB_VESSEL_INTERNAL_ERR;
         goto error;
      }

      for (UINT32 i = 1; i < count; ++i)
      {
         PAGE_ID tmpSmp = getDataSMPPId(pids[i]);
         if (INVALID_PAGE_ID == tmpSmp)
         {
            PD_LOG(PDERROR, "failed to pid of smp, data pid[%d]", pids[i]);
            rc = SDB_VESSEL_INTERNAL_ERR;
            goto error;
         }
         if (tmpSmp != smpPid)
         {
            PD_LOG(PDERROR, "multi allocating must from only one smp");
            rc = SDB_INVALIDARG;
            goto error;
         }
      }

      impPid = getDataIMPPid(lpids[0]);
      if (INVALID_PAGE_ID == impPid)
      {
         PD_LOG(PDERROR, "failed to pid of imp, lpid[%d]", lpids[0]);
         rc = SDB_VESSEL_INTERNAL_ERR;
         goto error;
      }

      for (UINT32 i = 1; i < count; ++i)
      {
         PAGE_ID tmpImp = getDataIMPPid(lpids[i]);
         if (INVALID_PAGE_ID == tmpImp)
         {
            PD_LOG(PDERROR, "failed to pid of imp, lpid[%d]", lpids[0]);
            rc = SDB_VESSEL_INTERNAL_ERR;
            goto error;
         }
         if (tmpImp != impPid)
         {
            PD_LOG(PDERROR, "multi allocating must from only one imp");
            rc = SDB_INVALIDARG;
            goto error;
         }
      }

      rc = allocateDataPagesOnSMP(context, type, count, lpids,
                                  pids, args, &oplistLsn);
      if (SDB_OK != rc)
      {
         goto error;
      }

      rc = mapNewLpids(context, count, lpids, pids, oplistLsn, oplistTail);
      if (SDB_OK != rc)
      {
         PD_LOG(PDWARNING, "begin to rollback allocating on smp");
         if (SDB_OK != releaseDataPagesOnSMP(context, count, pids, &oplistLsn))
         {
            PD_LOG(PDSEVERE, "failed to rollback allocating:%d", rc);
            IRedoLogger *logger = context->getOuterResource()->logger;
            logger->abortOplist(context->getSession(), oplistLsn);
         }

         goto error;
      }

      if (NULL != oplist)
      {
         *oplist = oplistLsn;
      }
   done:
      return rc;
   error:
      goto done;
   }

   INT32 collectionSpace::releaseDataPages(requestContext *context,
                                UINT32 count,
                                const PAGE_ID *lpids,
                                DPS_LSN_OFFSET oplist)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(FALSE, "todo");
   done:
      return rc;
   error:
      goto done;
   }

   INT32 collectionSpace::allocateDataPagesOnSMP(requestContext *context,
                                                 PAGE_TYPE type,
                                                 UINT32 count,
                                                 const PAGE_ID *lpids,
                                                 const PAGE_ID *pids,
                                                 const slice &args,
                                                 DPS_LSN_OFFSET *oplist)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(NULL != context, "can not be null");
      SDB_ASSERT(0 != count, "can not be zero");
      SDB_ASSERT(NULL != lpids, "can not be null");
      SDB_ASSERT(NULL != pids, "can not be null");
      SDB_ASSERT(INVALID_PAGE_TYPE != type, "can not be invalid");
      
      PAGE_ID smpPid = INVALID_PAGE_ID;
      UINT32 flags = PAGE_ACCESSOR_FLAG_NON_READONLY;
      if (NULL != oplist)
      {
         flags |= PAGE_ACCESSOR_FLAG_OPLIST_HEAD;
      }
      smpAccessor smp;
      
      smpPid = getDataSMPPId(pids[0]);
      if (INVALID_PAGE_ID == smpPid)
      {
         PD_LOG(PDERROR, "failed to pid of smp, data pid[%d]", pids[0]);
         rc = SDB_VESSEL_INTERNAL_ERR;
         goto error;
      }
      
      rc = smp.init(context, SPACE_TYPE_RECORD_D,
                     smpPid, flags, _su);
      if (SDB_OK != rc)
      {
         goto error;
      }

      rc = smp.allocatePages(context, _maxPageCountPerDataFile,
                             type, count, lpids, pids, args);
      if (SDB_OK != rc)
      {
         goto error;
      }

      if (NULL != oplist)
      {
         *oplist = smp.getOplist();
      }
   done:
      smp.fini(context);
      return rc;
   error:
      goto done;
   }

   INT32 collectionSpace::releaseDataPagesOnSMP(requestContext *context,
                                                UINT32 count,
                                                const PAGE_ID *pids,
                                                DPS_LSN_OFFSET *oplist)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(FALSE, "todo");
   done:
      return rc;
   error:
      goto done;
   }

   INT32 collectionSpace::mapNewLpids(requestContext *context,
                                      UINT32 count,
                                      const PAGE_ID *lpids,
                                      const PAGE_ID *pids,
                                      DPS_LSN_OFFSET oplist,
                                      BOOLEAN oplistTail)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(NULL != context, "can not be null");
      SDB_ASSERT(NULL != _su, "can not be null");
      SDB_ASSERT(0 != count, "can not be zero");
      SDB_ASSERT(NULL != lpids && NULL != pids, "can not be null");
      SDB_ASSERT(DPS_INVALID_LSN_OFFSET != oplist, "can not be invalid");

      DPS_LSN_OFFSET lsn = DPS_INVALID_LSN_OFFSET;
      PAGE_ID impPid = INVALID_PAGE_ID;
      snapshotContainer &container = context->getEnv()->snapContainer;
      impAccessor imp;
      UINT32 flags = PAGE_ACCESSOR_FLAG_NON_READONLY;

      if (DPS_INVALID_LSN_OFFSET != oplist)
      {
         lsn = oplist;
         if (oplistTail)
         {
            flags |= PAGE_ACCESSOR_FLAG_OPLIST_TAIL;
         }
      }

      impPid = getDataIMPPid(lpids[0]);
      if (OSS_UNLIKELY(INVALID_PAGE_ID == impPid))
      {
         PD_LOG(PDERROR, "failed to get data imp pid");
         rc = SDB_VESSEL_INTERNAL_ERR;
         goto error;
      }

      rc = imp.init(context, SPACE_TYPE_RECORD_M, impPid,
                     flags, _su, lsn);
      if (SDB_OK != rc)
      {
         goto error;
      }

      rc = imp.map(context, count, lpids, pids, container.getOnlineID());
      if (SDB_OK != rc)
      {
         goto error;
      }

   done:
      imp.fini(context);
      return rc;
   error:
      goto done;
   }

   PAGE_ID collectionSpace::getDataSMPPId(PAGE_ID pid)
   {
      SDB_ASSERT(0 < _maxPageCountPerDataFile, "can not be invalid");
      if (OSS_LIKELY(INVALID_PAGE_ID != pid && 0 < _maxPageCountPerDataFile))
      {
         return pid / _maxPageCountPerDataFile;
      }

      return INVALID_PAGE_ID;
   }

   PAGE_ID collectionSpace::getDataIMPPid(PAGE_ID lpid)const
   {
      SDB_ASSERT(0 < _idMapCapacity, "impossible");
      SDB_ASSERT(INVALID_PAGE_ID != lpid, "can not be invalid");
      if (OSS_LIKELY(INVALID_PAGE_ID != lpid && 0 < _idMapCapacity))
      {
         return (lpid / _idMapCapacity) + SYSTEM_MAP_PAGE_ID;
      }
      return INVALID_PAGE_ID;
   }

   INT32 collectionSpace::createNewDataFileAndExtendBitMap(requestContext *context,
                                                           const UINT32 *oldPageCount)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(NULL != context, "can not be null");
      SDB_ASSERT(NULL != _su, "can not be null");
      SDB_ASSERT(_su->isOpen(), "must be open");
      UINT32 sequence = 0;
      PAGE_ID pid = INVALID_PAGE_ID;
      UINT32 maxPageCountPerSeg = 0;
      UINT32 pageCount = 0;
      ossScopedLock lock(&_dataAndMetaSpaceLatch);
      UINT32 count = _inMemDataSMP.getPageCount();
      BOOLEAN rollbackFile = FALSE;

      if (NULL != oldPageCount && *oldPageCount < count)
      {
         goto done;
      }

      rc = _su->getCoreArgs(SPACE_TYPE_RECORD_D, &maxPageCountPerSeg, &pageCount);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to get core args:%d", rc);
         goto error;
      }

      rc = _su->createDataFile(context, &sequence);
      if (SDB_OK != rc)
      {
         goto error;
      }
      rollbackFile = TRUE;

      pid = sequence * maxPageCountPerSeg;

      rc = initDataSMP(context, pid);
      if (SDB_OK != rc)
      {
         goto error;
      }

      /// 1 page occupied for smp.
      rc = _inMemDataSMP.allocateNewBitPage(1);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to extend in-mem bitmap:%d", rc);
         goto error;
      }

   done:
      return rc;
   error:
      if (rollbackFile)
      {
         _su->removeLastDataFile(context);
      }
      goto done;
   }

   INT32 collectionSpace::ensureNewIMPAndExtendPool(requestContext *context,
                                                    UINT32 oldPageAllocated)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(NULL != context, "can not be null");
      SDB_ASSERT(NULL != _su, "can not be null");
      SDB_ASSERT(0 <_maxPageCountPerMetaFile, "can not be zero");
      SDB_ASSERT(0 <_pageAllocatedInMetaSMP, "can not be zero");
      UINT32 pageCountInSeg = 0;
      PAGE_ID impPid = INVALID_PAGE_ID;
      ossScopedLock lock(&_dataAndMetaSpaceLatch);

      if (oldPageAllocated < _pageAllocatedInMetaSMP)
      {
         goto done;
      }

      rc = _su->getCoreArgs(SPACE_TYPE_RECORD_M, NULL, &pageCountInSeg);
      if (OSS_UNLIKELY(SDB_OK != rc))
      {
         goto error;
      }

      SDB_ASSERT(64 == pageCountInSeg, "must be 64");
      /// all pages in current file were allocated.
      if (0 == (_pageAllocatedInMetaSMP & (pageCountInSeg - 1)))
      {
         UINT32 segCount = (_pageAllocatedInMetaSMP >> 6) + 1;/// _pageAllocatedInMetaSMP / 64
         /// segment count must be specified that we do not need to
         /// rollback file size if get any error then.
         rc = _su->extendMetaFile(context, &segCount);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to extentd meta file:%d", rc);
            goto error;
         }
      }

      impPid = _pageAllocatedInMetaSMP;
      rc = initIMP(context, impPid);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to init new imp[%d], rc:%d", impPid, rc);
         goto error;
      }

      rc = _su->fsync(SPACE_TYPE_RECORD_M, impPid, TRUE);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to fsync pid[%d], rc:%d", impPid, rc);
         goto error;
      }

      rc = _inMemLpidPool.allocateNewBitPage();
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to extend in-mem bitmap:%d", rc);
         goto error;
      }

      ++_pageAllocatedInMetaSMP;
   done:
      return rc;
   error:
      goto done;
   }

   INT32 collectionSpace::cacheGlobalMetaData(requestContext *context)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(NULL != _su, "can not be null");
      csgpAccessor accessor;
      UINT32 flags = PAGE_ACCESSOR_FLAG_DIRECT;

      rc = accessor.init(context,
                         SPACE_TYPE_RECORD_M,
                         CS_GLOBAL_META_PAGE_ID,
                         flags, _su);
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
      accessor.fini(context);
      return rc;
   error:
      _recordInMem.reset();
      goto done;
   }

   INT32 collectionSpace::initCollectionsFromDisk(requestContext *context)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(0 < _idMapCapacity, "can not be zero");
      SDB_ASSERT(0 < _capacityOfCLRecordPage, "can not be zero");
      UINT32 pageSize = 0;
      impAccessor imp;
      ossValuePtr ptr = 0;
      PAGE_ID pid = INVALID_PAGE_ID;
      UINT32 clScanned = 0;

      rc = _su->getCoreArgs(SPACE_TYPE_RECORD_M, &pageSize);
      if (SDB_OK != rc)
      {
         goto error;
      }

      rc = _su->getPagePtr(SPACE_TYPE_RECORD_M, SYSTEM_MAP_PAGE_ID, ptr);
      if (SDB_OK != rc)
      {
         goto error;
      }

      rc = imp.initWithDirectMode(context, SPACE_TYPE_RECORD_M, SYSTEM_MAP_PAGE_ID,
                                  pageSize, ptr);
      if (SDB_OK != rc)
      {
         goto error;
      }

      for (UINT32 i = 0; i < _idMapCapacity && clScanned < MAX_CL_MB_COUNT; ++i)
      {

         rc = imp.getPidByOffset(i, &pid, NULL);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to get next pid from imp:%d", rc);
            goto error;
         }

         clScanned += _capacityOfCLRecordPage;

         /// we cannot guarantee that no holes in page.
         if (INVALID_PAGE_ID == pid)
         {
            continue;
         }

         rc = initCollectionsFromOneDiskPage(context, pid);
         if (SDB_OK != rc)
         {
            goto error;
         }
      }
   
   done:
      imp.fini(context);
      return rc;
   error:
      _clNameIndex.clear();
      _clIdIndex.clear();
      _collectionAllocator.fini();
      goto done;
   }

   INT32 collectionSpace::initCollectionsFromOneDiskPage(requestContext *context,
                                                         PAGE_ID pid)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(NULL != context, "can not be null");
      SDB_ASSERT(INVALID_PAGE_ID != pid, "can not be invalid");
      UINT32 pageSize = 0;
      ossValuePtr ptr = 0;
      crpAccessor crp;
      UINT32 capacity = 0;
      collectionRecord record;
      
      rc = _su->getCoreArgs(SPACE_TYPE_RECORD_D, &pageSize);
      if (SDB_OK != rc)
      {
         goto error;
      }

      rc = getCapacityOfCLRecordPage(pageSize, capacity);
      if (SDB_OK != rc)
      {
         goto error;
      }

      rc = _su->getPagePtr(SPACE_TYPE_RECORD_D, pid, ptr);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to get crp page:%d", rc);
         goto error;
      }

      rc = crp.initWithDirectMode(context, SPACE_TYPE_RECORD_D, pid,
                                  pageSize, ptr);
      if (SDB_OK != rc)
      {
         goto error;
      }

      for (UINT32 i = 0; i < capacity; ++i)
      {
         collectionAllocator::collectionHolder *holder = NULL;
         strSlice nameSlice;

         rc = crp.getClRecordBySlot(i, record);
         if (SDB_DMS_NOTEXIST == rc)
         {
            rc = SDB_OK;
            continue;
         }
         else if (SDB_OK != rc)
         {
            goto error;
         }

         if (INVALID_CL_MB_ID == record.mbID)
         {
            PD_LOG(PDERROR, "bitmap may crashed on cl record page[%d]", pid);
            rc = SDB_VESSEL_INTERNAL_ERR;
            goto error;
         }

         nameSlice.reset(record.name);
         if (existsInFormalIndex(nameSlice, record.innerID))
         {
            PD_LOG(PDERROR, "duplicate cl name[%s] or inner id[%d] when loading",
                   record.name, record.innerID);
            rc = SDB_VESSEL_INTERNAL_ERR;
            goto error;
         }

         rc = _collectionAllocator.occupyMB(record.mbID, &holder);
         if (SDB_OK != rc)
         {
            goto error;
         }

         rc = holder->getCollection()->initWhenOpen(context, record, this);
         if (SDB_OK != rc)
         {
            goto error;
         }

         insertIntoFormalIndex(holder);
      }
   done:
      crp.fini(context);
      return rc;
   error:
      goto done;
   }

   INT32 collectionSpace::allocateCLLogicalID(requestContext *context,
                                              UINT32 &lid)
   {
      INT32 rc = SDB_OK;
      csgpAccessor accessor;
      rc = accessor.init(context, SPACE_TYPE_RECORD_M,
                          CS_GLOBAL_META_PAGE_ID,
                          PAGE_ACCESSOR_FLAG_NON_READONLY);
      if (SDB_OK != rc)
      {
         goto error;
      }

      rc = accessor.allocateCLLogicalID(context, lid);
      if (SDB_OK != rc)
      {
         goto error;
      }
   done:
      accessor.fini(context);
      return rc;
   error:
      goto done;
   }

   INT32 collectionSpace::updateStatusToOnlineWhenCreating(requestContext *context)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(NULL != _su, "can not be null");
      SDB_ASSERT(!_recordInMem.isOnline(), "can not be online");
      dataIDMapFileHead h;
      csgpAccessor accessor;

      _su->dumpIDMapFileHead(h);
      rc = accessor.init(context, SPACE_TYPE_RECORD_M,
                         CS_GLOBAL_META_PAGE_ID,
                         PAGE_ACCESSOR_FLAG_NON_READONLY,
                         getSU());
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "fialed to init accessor:%d", rc);
         goto error;
      }

      rc = accessor.setOnlineWhenCreating(context, h);
      if (SDB_OK != rc)
      {
         goto error;
      }

      _recordInMem.status = CMR_STATUS_ONLINE;
   done:
      accessor.fini(context);
      return rc;
   error:
      goto done;
   }

   INT32 collectionSpace::initNecessaryPagesWhenCreating(requestContext *context)
   {
      INT32 rc = SDB_OK;
      rc = firstExtendMetaFile(context);
      if (SDB_OK != rc)
      {
         goto error;
      }

      rc = initGMP(context, _recordInMem);
      if (SDB_OK != rc)
      {
         goto error;
      }

      rc = initIMP(context, SYSTEM_MAP_PAGE_ID);
      if (SDB_OK != rc)
      {
         goto error;
      }

      rc = _su->fsync(SPACE_TYPE_RECORD_M, SMP_PAGE_ID, 3, TRUE);
      if (SDB_OK != rc)
      {
         goto error;
      }
   done:
      return rc;
   error:
      goto done;
   }

   INT32 collectionSpace::firstExtendMetaFile(requestContext *context)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(NULL != _su, "can not be null");
      SDB_ASSERT(_su->isOpen(), "must be open");
      SDB_ASSERT(0 == _su->getMetaSegmentCount(), "must be zero");
      ossValuePtr ptr = 0;
      smpAccessor smp;
      UINT32 maxSegmentCount = 0;
      UINT32 maxPageCount = 0;
      UINT32 pageSize = 0;

      rc = _su->getCoreArgs(SPACE_TYPE_RECORD_M,
                            &pageSize,
                            &maxPageCount,
                            &maxSegmentCount);
      if (OSS_UNLIKELY(SDB_OK != rc))
      {
         PD_LOG(PDERROR, "failed to get core args of su:%d", rc);
         goto error;
      }

      rc = _su->extendMetaFile(context);
      if (SDB_OK != rc)
      {
         goto error;
      }

      rc = _su->getPagePtr(SPACE_TYPE_RECORD_M, SMP_PAGE_ID, ptr);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to get page ptr:%d", rc);
         goto error;
      }

      rc = smp.initWithDirectMode(context,
                                  SPACE_TYPE_RECORD_M,
                                  SMP_PAGE_ID, pageSize, ptr,
                                  FALSE, FALSE);
      if (SDB_OK != rc)
      {
         goto error;
      }

      /// first 3 pages are occupied by system pages.
      rc = smp.initSMP(context, maxSegmentCount, maxPageCount, 3);
      if (SDB_OK != rc)
      {
         goto error;
      }
   done:
      smp.fini(context);
      return rc;
   error:
      goto done;
   }

   INT32 collectionSpace::initGMP(requestContext *context,
                                  const csMetaRecord &record)
   {
      INT32 rc = SDB_OK;
      csgpAccessor csgp;
      ossValuePtr ptr = 0;
      UINT32 pageSize = 0;

      rc = _su->getCoreArgs(SPACE_TYPE_RECORD_M, &pageSize, NULL, NULL);
      if (OSS_UNLIKELY(SDB_OK != rc))
      {
         PD_LOG(PDERROR, "failed to get core args:%d", rc);
         goto error;
      }

      rc = _su->getPagePtr(SPACE_TYPE_RECORD_M, CS_GLOBAL_META_PAGE_ID, ptr);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to get cs global meta page:%d", rc);
         goto error;
      }

      rc = csgp.initWithDirectMode(context,
                                    SPACE_TYPE_RECORD_M,
                                    CS_GLOBAL_META_PAGE_ID,
                                    pageSize, ptr, FALSE, FALSE);
      if (SDB_OK != rc)
      {
         goto error;
      }

      rc = csgp.initPage(context, record);
      if (SDB_OK != rc)
      {
         goto error;
      }
   done:
      csgp.fini(context);
      return rc;
   error:
      goto done;
   }

   INT32 collectionSpace::initIMP(requestContext *context,
                                  PAGE_ID pid)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(NULL != context, "can not be null");
      SDB_ASSERT(INVALID_PAGE_ID != pid, "can not be invalid");
      UINT32 pageSize = 0;
      impAccessor imp;
      ossValuePtr ptr = 0;

      rc = _su->getCoreArgs(SPACE_TYPE_RECORD_M, &pageSize, NULL, NULL);
      if (OSS_UNLIKELY(SDB_OK != rc))
      {
         PD_LOG(PDERROR, "failed to get core args:%d", rc);
         goto error;
      }

      rc = _su->getPagePtr(SPACE_TYPE_RECORD_M, pid, ptr);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to get cs global meta page:%d", rc);
         goto error;
      }

      rc = imp.initWithDirectMode(context,
                     SPACE_TYPE_RECORD_M,
                     pid,
                     pageSize, ptr, FALSE, FALSE);
      if (SDB_OK != rc)
      {
         goto error;
      }

      rc = imp.initPage(context);
      if (SDB_OK != rc)
      {
         goto error;
      }
   done:
      imp.fini(context);
      return rc;
   error:
      goto done;
   }

   INT32 collectionSpace::initDataSMP(requestContext *context,
                                      PAGE_ID pid)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(NULL != context, "can not be null");
      SDB_ASSERT(INVALID_PAGE_ID != pid, "can not be invalid");
      UINT32 maxSegmentCount = 0;
      UINT32 maxPageCount = 0;
      UINT32 pageSize = 0;
      ossValuePtr ptr = 0;
      smpAccessor smp;

      rc = _su->getCoreArgs(SPACE_TYPE_RECORD_D, &pageSize, &maxPageCount, &maxSegmentCount);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to get core args:%d", rc);
         goto error;
      }

      rc = _su->getPagePtr(SPACE_TYPE_RECORD_D, pid, ptr);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to get page ptr, pid[%d], rc:%d", pid, rc);
         goto error;
      }

      rc = smp.initWithDirectMode(context, SPACE_TYPE_RECORD_D,
                                  pid, pageSize, ptr, FALSE, FALSE);
      if (SDB_OK != rc)
      {
         goto error;
      }

      rc = smp.initSMP(context, maxSegmentCount, maxPageCount, 1);
      if (SDB_OK != rc)
      {
         goto error;
      }

      rc = _su->fsync(SPACE_TYPE_RECORD_D, pid, 1, TRUE);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to fsync page:%d, rc:%d", pid, rc);
         goto error;
      }
   done:
      smp.fini(context);
      return rc;
   error:
      goto done;
   }

   BOOLEAN collectionSpace::addToCreatingIndex(const strSlice &clName,
                                               utilCLInnerID innerID)
   {
      SDB_ASSERT(!clName.empty(), "can not be empty");
      BOOLEAN r = FALSE;
      ossScopedLock lock(&_indexLatch, EXCLUSIVE);
      if (0 < _creatingNameIndex.count(clName.str()))
      {
         goto done;
      }
      else if (_clNameIndex.count(clName.str()))
      {
         goto done;
      }
      else if (UTIL_IS_VALID_CL_INNERID(innerID))
      {
         if (0 < _creatingIdIndex.count(innerID))
         {
            goto done;
         }
         else if (0 < _clIdIndex.count(innerID))
         {
            goto done;
         }
      }
      
      _creatingNameIndex.insert(clName.str());
      if (UTIL_IS_VALID_CL_INNERID(innerID))
      {
         _creatingIdIndex.insert(innerID);
      }
      r = TRUE;
   done:
      return r;
   }

   void collectionSpace::rollbackCreatingIndex(const strSlice &clName,
                                                utilCLInnerID innerID)
   {
      SDB_ASSERT(!clName.empty(), "can not be empty");
      ossScopedLock lock(&_indexLatch, EXCLUSIVE);
      _creatingNameIndex.erase(clName.str());
      if (UTIL_IS_VALID_CL_INNERID(innerID))
      {
         _creatingIdIndex.erase(innerID);
      }
   }

   void collectionSpace::moveToFormalIndex(collectionAllocator::collectionHolder *holder)
   {
      SDB_ASSERT(NULL != holder && !holder->isFree(), "can not be invalid");
      const CHAR *clName = holder->getCollection()->getName();
      utilCLInnerID innerID = holder->getCollection()->getInnerID();
      UINT32 logicalID = holder->getCollection()->getLogicalID();
      SDB_ASSERT(DMS_INVALID_LOGICCLID != logicalID, "can not be invalid");
      _UID_HOLDER_PAIR p(holder, logicalID);
      ossScopedLock lock(&_indexLatch, EXCLUSIVE);

      _creatingNameIndex.erase(clName);
      _clNameIndex[clName] = p;
      if (UTIL_IS_VALID_CL_INNERID(innerID))
      {
         _creatingIdIndex.erase(innerID);
         _clIdIndex[innerID] = p;
      }
      return;
   }

   BOOLEAN collectionSpace::insertIntoFormalIndex(collectionAllocator::collectionHolder *holder)
   {
      SDB_ASSERT(NULL != holder && !holder->isFree(), "can not be empty");
      collection *cl = holder->getCollection();
      const CHAR *name = cl->getName();
      utilCLInnerID innerID = cl->getInnerID();
      UINT32 logicalID = cl->getLogicalID();
      _UID_HOLDER_PAIR p(holder, logicalID);
      BOOLEAN r = FALSE;
      ossScopedLock lock(&_indexLatch, EXCLUSIVE);

      if (!_clNameIndex.insert(std::make_pair(name, p)).second)
      {
         goto done;
      }
      else if (UTIL_IS_VALID_CL_INNERID(innerID))
      {
         if (!_clIdIndex.insert(std::make_pair(innerID, p)).second)
         {
            _clNameIndex.erase(name);
            goto done;
         }
      }

      r = TRUE;
   done:
      return r;
   }

   void collectionSpace::eraseFromIndex(const strSlice &clName,
                                        utilCLInnerID innerID)
   {
      SDB_ASSERT(!clName.empty(), "can not be empty");
      ossScopedLock lock(&_indexLatch, EXCLUSIVE);
      _clNameIndex.erase(clName.str());
      if (UTIL_IS_VALID_CL_INNERID(innerID))
      {
         _clIdIndex.erase(innerID);
      }

      return;
   }

   BOOLEAN collectionSpace::existsInFormalIndex(const strSlice &clName, utilCLInnerID innerID)
   {
      SDB_ASSERT(!clName.empty(), "can not be empty");
      ossScopedLock lock(&_indexLatch, SHARED);
      return (0 < _clNameIndex.count(clName.str()) ||
             (UTIL_IS_VALID_CL_INNERID(innerID) &&
              0 < _clIdIndex.count(innerID)));
             
   }


   INT32 collectionSpace::upperBoundCLName(const strSlice &clName,
                                           UINT32 bufferSize,
                                           CHAR *nameBuffer,
                                           UINT32 &logicalID,
                                           collectionAllocator::collectionHolder **holder)
   {
      BOOLEAN r = FALSE;
      SDB_ASSERT((DMS_COLLECTION_NAME_SZ + 1) <= bufferSize, "buffer size not enough");
      SDB_ASSERT(NULL != nameBuffer, "can not be null");
      SDB_ASSERT(NULL != holder, "can not be null");
      
      ossScopedLock lock(&_indexLatch, SHARED);
      NAME_INDEX::const_iterator itr = _clNameIndex.begin();
      if (!clName.empty())
      {
         itr = _clNameIndex.upper_bound(clName.str());
      }

      if (_clNameIndex.end() != itr)
      {
         ossStrcpy(nameBuffer, itr->first);
         *holder = itr->second.holder;
         logicalID = itr->second.logicalID;
         r = TRUE;
      }
      
   done:
      return r;
   }

   BOOLEAN collectionSpace::findCollection(const strSlice &clName,
                                           UINT32 &logicalID,
                                           collectionAllocator::collectionHolder **holder)
   {
      SDB_ASSERT(!clName.empty(), "can not be empty");
      SDB_ASSERT(NULL != holder, "can not be null");
      ossScopedLock lock(&_indexLatch, SHARED);
      NAME_INDEX::const_iterator itr = _clNameIndex.find(clName.str());
      if (_clNameIndex.end() != itr)
      {
         SDB_ASSERT(itr->second.isValid(), "must be valid");
         logicalID = itr->second.logicalID;
         *holder = itr->second.holder;
         return TRUE;
      }
      return FALSE;
   }

   BOOLEAN collectionSpace::findCollection(utilCLInnerID innerID,
                                           UINT32 &logicalID,
                                           collectionAllocator::collectionHolder **holder)
   {
      SDB_ASSERT(UTIL_IS_VALID_CL_INNERID(innerID), "can not be invalid");
      SDB_ASSERT(NULL != holder, "can not be null");
      ossScopedLock lock(&_indexLatch, SHARED);
      ID_INDEX::const_iterator itr = _clIdIndex.find(innerID);
      if (_clIdIndex.end() != itr)
      {
         SDB_ASSERT(itr->second.isValid(), "must be valid");
         logicalID = itr->second.logicalID;
         *holder = itr->second.holder;
         return TRUE;
      }
      return FALSE;
   }

   INT32 collectionSpace::cacheKeyParametersAboutStorage()
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(NULL != _su, "can not be null");
      UINT32 pageSize = 0;
      UINT32 maxSegPerFile = 0;
      UINT32 maxPagePerSeg = 0;
      UINT32 capacityOfSMP = 0;

      rc = _su->getCoreArgs(SPACE_TYPE_RECORD_M, &pageSize);
      if (SDB_OK != rc)
      {
         goto error;
      }

      rc = getCapacityOfIMP(pageSize, _idMapCapacity);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to get capacity of imp:%d", rc);
         goto error;
      }

      rc = _su->getCoreArgs(SPACE_TYPE_RECORD_D, &pageSize, &maxPagePerSeg, &maxSegPerFile);
      if (SDB_OK != rc)
      {
         goto error;
      }

      rc = getCapacityOfCLRecordPage(pageSize, _capacityOfCLRecordPage);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to get capacity of cl record page:%d", rc);
         goto error;
      }

      _maxPageCountPerDataFile = maxPagePerSeg * maxSegPerFile;

      rc = getSMPCapacity8BytesAligned(pageSize, maxSegPerFile, maxPagePerSeg, capacityOfSMP);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to get smp capacity:%d", rc);
         goto error;
      }

      if (_maxPageCountPerDataFile != capacityOfSMP)
      {
         rc = SDB_VESSEL_INTERNAL_ERR;
         PD_LOG(PDERROR, "_maxPageCountPerDataFile and capacityOfSMP should be same");
         goto error;
      }

      rc = _su->getCoreArgs(SPACE_TYPE_RECORD_M, &pageSize, &maxPagePerSeg, &maxSegPerFile);
      if (SDB_OK != rc)
      {
         goto error;
      }
      _maxPageCountPerMetaFile = maxPagePerSeg * maxSegPerFile;
      rc = getSMPCapacity8BytesAligned(pageSize, maxSegPerFile, maxPagePerSeg, capacityOfSMP);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to get smp capacity:%d", rc);
         goto error;
      }
      if (_maxPageCountPerMetaFile != capacityOfSMP)
      {
         rc = SDB_VESSEL_INTERNAL_ERR;
         PD_LOG(PDERROR, "_maxPageCountPerMetaFile and capacityOfSMP should be same");
         goto error;
      }
   done:
      return rc;
   error:
      goto done;
   }

   INT32 collectionSpace::initInMemLpidPool(requestContext *context)
   {
      SDB_ASSERT(0 < _idMapCapacity, "can not be zero");
      SDB_ASSERT(NULL != context, "can not be null");
      SDB_ASSERT(NULL != _su, "can not be null");
      SDB_ASSERT(0 < _maxPageCountPerMetaFile, "can not be zero");
      INT32 rc = SDB_OK;
      smpAccessor accessor;
      impAccessor imp;
      UINT32 pageSize = 0;
      CHAR *buffer = NULL;
      const UINT64 *bits = NULL;
      PAGE_ID maxPid = _maxPageCountPerMetaFile;
      UINT32 bitsCount = _maxPageCountPerMetaFile >> 6;
      UINT32 idMapBitsCount = 0;
      UINT32 firstFree = 0;

      rc = _su->getCoreArgs(SPACE_TYPE_RECORD_M, &pageSize);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to get core args:%d", rc);
         goto error;
      }

      buffer = context->allocateBuffer(pageSize);
      if (NULL == buffer)
      {
         PD_LOG(PDERROR, "failed to allcoate mem");
         rc = SDB_OOM;
         goto error;
      }

      rc = accessor.init(context, SPACE_TYPE_RECORD_M, SMP_PAGE_ID, 0, _su);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to init smp accessor:%d", rc);
         goto error;
      }

      rc = accessor.dumpSMP(pageSize, buffer);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to dump smp[%d], rc:%d", SMP_PAGE_ID, rc);
         goto error;
      }

      accessor.fini(context);

      bits = (const UINT64 *)(buffer + SMP_HEAD_LEN);
      if (findFirstFreeBitFromBit64(bitsCount, -1, bits, firstFree))
      {
         if (firstFree <= SYSTEM_MAP_PAGE_ID)
         {
            PD_LOG(PDERROR, "the first free offset should not be system reserved page");
            rc = SDB_VESSEL_INTERNAL_ERR;
            goto error;
         }
         maxPid = firstFree;
         _pageAllocatedInMetaSMP = firstFree;
      }
      else
      {
         _pageAllocatedInMetaSMP = _maxPageCountPerMetaFile;
      }

      rc = _inMemLpidPool.init(_idMapCapacity, PAGE_COUNT_IN_EXTENT);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to init lpid bitmap:%d", rc);
         goto error;
      }

      idMapBitsCount = ossAlign64(_idMapCapacity) >> 6;
      /// skip system id map page.
      _inMemLpidPool.incPageCount();
      for (PAGE_ID i = SYSTEM_MAP_PAGE_ID + 1; i < maxPid; ++i)
      {
         UINT32 freeCount = 0;
         rc = imp.init(context, SPACE_TYPE_RECORD_M, i, 0, _su);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to init crp accessor at page[%d], rc:%d", i, rc);
            goto error;
         }

         rc = imp.dumpAsBitMap((UINT64*)buffer, freeCount);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to dump imp[%d], rc:%d", i, rc);
            goto error;
         }

         imp.fini(context);

         if (0 == freeCount)
         {
            _inMemLpidPool.incPageCount();
         }
         else
         { 
            rc = _inMemLpidPool.mapNewBitPage(idMapBitsCount, (const UINT64 *)buffer);
            if (SDB_OK != rc)
            {
               PD_LOG(PDERROR, "failed to map new bitmap page:%d", rc);
               goto error;
            }
         }
      }
   done:
      if (NULL != buffer)
      {
         context->releaseBuffer(buffer, pageSize);
      }
      return rc;
   error:
      accessor.fini(context);
      imp.fini(context);
      _inMemLpidPool.fini();
      goto done;
   }

   BOOLEAN collectionSpace::isInMemBitMapsReady()const
   {
      return _inMemDataSMP.isInitialized();
   }

   INT32 collectionSpace::initInMemBitMaps(requestContext *context)
   {
      INT32 rc = SDB_OK;
      ossScopedLock lock(&_dataAndMetaSpaceLatch);
      if (isInMemBitMapsReady())
      {
         goto done;
      }

      rc = initInMemDataSMPBitMap(context);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to init bitmap of data smp:%d", rc);
         goto error;
      }
      
      rc = initInMemLpidPool(context);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to init bitmap of lpid pool:%d", rc);
         goto error;
      }
   done:
      return rc;
   error:
      goto done;
   }


   INT32 collectionSpace::initInMemDataSMPBitMap(requestContext *context)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(NULL != context, "can not be null");
      SDB_ASSERT(NULL != _su, "can not be null");
      SDB_ASSERT(0 < _maxPageCountPerDataFile, "can not be zero");
      UINT32 smpCount = 0;
      UINT32 pageSize = 0;
      CHAR *buffer = NULL;
      PAGE_ID pid = INVALID_PAGE_ID;
      UINT32 maxSegmentCount = 0;
      UINT32 pageCountPerSeg = 0;
      UINT32 capacity = 0;
      UINT32 bitsCount = 0;

      rc = _su->getCoreArgs(SPACE_TYPE_RECORD_D, &pageSize,
                            &pageCountPerSeg, &maxSegmentCount);
      if (OSS_UNLIKELY(SDB_OK != rc))
      {
         PD_LOG(PDERROR, "failed to get core args:%d", rc);
         goto error;
      }

      buffer = context->allocateBuffer(pageSize);
      if (NULL == buffer)
      {
         PD_LOG(PDERROR, "failed to allcoate mem");
         rc = SDB_OOM;
         goto error;
      }

      rc = getSMPCapacity8BytesAligned(pageSize, maxSegmentCount,
                                       pageCountPerSeg, capacity);
      if (SDB_OK != rc)
      {
         goto error;
      }

      rc = _inMemDataSMP.init(capacity, PAGE_COUNT_IN_EXTENT);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to init inmem-bitmap:%d", rc);
         goto error;
      }

      smpCount = _su->getDataFileCount();
      pid = SMP_PAGE_ID;
      bitsCount = capacity >> 6;
      for (UINT32 i = 0; i < smpCount; ++i)
      {
         smpAccessor accessor;
         rc = accessor.init(context, SPACE_TYPE_RECORD_D, pid, 0, _su);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to init smp accessor:%d", rc);
            goto error;
         }

         rc = accessor.dumpSMP(pageSize, buffer);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to dump smp[%d], rc:%d", pid, rc);
            goto error;
         }

         accessor.fini(context);

         rc = _inMemDataSMP.mapNewBitPage(bitsCount, (const UINT64 *)(buffer + SMP_HEAD_LEN));
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to map smp to inmem bitmap, pid[%d], rc:%d", pid, rc);
            goto error;
         }

         pid += _maxPageCountPerDataFile;
      }
   done:
      if (NULL != buffer)
      {
         context->releaseBuffer(buffer, pageSize);
      }
      return rc;
   error:
      _inMemDataSMP.fini();
      goto done;
   }
}//namespace vessel
}//namespace engine