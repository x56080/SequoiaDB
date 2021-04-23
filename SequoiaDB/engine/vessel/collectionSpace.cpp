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
   static const UINT32 SYSTEM_PAGE_COUNT_IN_META_EXCEPT_SMP = 2;

   collectionSpace::collectionSpace():
   _status(CLOSED),
   _su(NULL),
   _currentPageCountInMeta(0)
   {
      
   }

   collectionSpace::~collectionSpace()
   {
      fini();
   }

   INT32 collectionSpace::create(requestContext *context,
                                 const strSlice &name,
                                 utilCSUniqueID uniqueID,
                                 storageUnit *su,
                                 const createCSOptions &options)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(isClosed(), "do not recreate");

      if (OSS_UNLIKELY(NULL == context ||
                       EXCLUSIVE != context->getSpaceIDLockedMode() ||
                       DMS_COLLECTION_SPACE_NAME_SZ < name.strLen() ||
                       name.empty() ||
                       NULL == su ||
                       !su->isOpen()))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      else if (!options.isValid())
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      else if (OSS_UNLIKELY(!isClosed()))
      {
         fini();
      }

      _recordInMem.version = CMR_VERSION_1;
      _recordInMem.status = CMR_STATUS_CREATING;
      _recordInMem.flags = 0;
      _recordInMem.maxCLLogicalID = DMS_INVALID_LOGICCSID;
      _recordInMem.uniqueID = uniqueID;
      ossMemcpy(_recordInMem.name, name.str(), name.strLen());
      _su = su;

      rc = cacheKeyParametersAboutStorage();
      if (SDB_OK != rc)
      {
         goto error;
      }

      rc = initNecessaryPagesWhenCreating(context);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to init necessary pages:%d", rc);
         goto error;
      }

      rc = initInMemBitMaps(context);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to init in-mem bitmap:%d", rc);
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
      _su->ensureCSNameFile(context, strSlice(_recordInMem.name));
      _status = OPEN;
   done:
      return rc;
   error:
      fini();
      goto done;
   }

   void collectionSpace::close()
   {
      fini();
      return;
   }

   INT32 collectionSpace::open(requestContext *context,
                               storageUnit *su)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(isClosed(), "do not recreate");
      if (OSS_UNLIKELY(NULL == context ||
                       NULL == su ||
                       !su->isOpen()))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      else if (OSS_UNLIKELY(!isClosed()))
      {
         fini();
      }

      _su = su;
      rc = cacheKeyParametersAboutStorage();
      if (SDB_OK != rc)
      {
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
         PD_LOG(PDERROR, "failed to load collections on disk:%d", rc);
         goto error;
      }

      rc = initInMemBitMaps(context);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to init in-mem bitmap:%d", rc);
         goto error;
      }

      _status = OPEN;
   done:
      return rc;
   error:
      fini();
      goto done;
   }

   void collectionSpace::fini()
   {
      _status = CLOSED;
      _su = NULL;
      _recordInMem.reset();
      _parameters.reset();
      _inMemDataSMP.fini();
      _inMemLpidPool.fini();
      _currentPageCountInMeta = 0;
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
      if (0 < _parameters.dataPageSize)
      {
         pageSize = _parameters.dataPageSize;
         return SDB_OK;
      }
      return SDB_VESSEL_RESOURCES_NOT_INIT;
   }

   INT32 collectionSpace::createCL(requestContext *context,
                                   const strSlice &clName, 
                                   utilCLInnerID clInnerId,
                                   const createCLOptions &options)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(isOpen(), "must be open");
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
      ossScopedLock lock(&_runtimeIndexLatch, SHARED);
      return _clNameIndex.size();
   }

   INT32 collectionSpace::listCollections(requestContext *context,
                                          listCLCursor *cursor)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(isOpen(), "must be open");
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
      if (NULL == _su || !_su->isOpen())
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }

      rc = _su->ensureFsmFile(context, file);
      if (SDB_OK != rc)
      {
         goto error;
      }
   done:
      return rc;
   error:
      goto done;
   }

   INT32 collectionSpace::dump(requestContext *context,
                               listCollectionSpaceRecord &record)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(isOpen(), "must be open");
      UINT32 pageSize = 0;
      UINT32 pageCountPerSeg = 0;
      if (OSS_UNLIKELY(NULL == context ||
                       !context->getSpaceIDLocked() ||
                       context->getSpaceID() != getSpaceID()))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      rc = _su->getCoreArgs(FILE_TYPE_DD, &pageSize, &pageCountPerSeg, NULL);
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
      rc = getSU()->getCoreArgs(FILE_TYPE_IDX_D, &pageSize, &pageCountPerSeg, NULL);
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
      UINT32 capacity = 0;
      SDB_ASSERT(0 < _parameters.dataPageSize, "can not be invalid");
      if (OSS_UNLIKELY(INVALID_CL_MB_ID == mbID))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      rc = getCapacityOfCLRecordPage(_parameters.dataPageSize, capacity);
      if (OSS_UNLIKELY(SDB_OK != rc))
      {
         goto error;
      }

      lpid = mbID / capacity;
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
      else if (!context->testLpidLocked(FILE_TYPE_DD, lpid))
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
                    FILE_TYPE_DM,
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
      else if (!context->testLpidLockMode(FILE_TYPE_DD, lpid, EXCLUSIVE))
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
                     FILE_TYPE_DM,
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
      SDB_ASSERT(isOpen(), "must be open");
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
      SDB_ASSERT(isOpen(), "must be open");
      if (OSS_UNLIKELY(NULL == context ||
                       0 == count ||
                       PAGE_COUNT_IN_EXTENT < count ||
                       NULL == lpids))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      do
      {
         UINT32 pageCount = _currentPageCountInMeta;
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
      SDB_ASSERT(isOpen(), "must be open");
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

   INT32 collectionSpace::allocateIdMapPageOnSMP(requestContext *context,
                                                 PAGE_ID pid)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(NULL != context, "can not be null");
      SDB_ASSERT(INVALID_PAGE_ID != pid, "can not be invalid");
      smpAccessor accessor;
      PAGE_ID smpPid = getMetaSMPPid(pid);
      if (INVALID_PAGE_ID == smpPid)
      {
         PD_LOG(PDERROR, "failed to get smp pid of pid[%d]", pid);
         rc = SDB_VESSEL_INTERNAL_ERR;
         goto error;
      }

      rc = accessor.init(context, FILE_TYPE_DM,
                         smpPid, PAGE_ACCESSOR_FLAG_NON_READONLY,
                         _su);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to init smp accessor of pid[%d], rc:%d",
                smpPid, rc);
         goto error;
      }

      rc = accessor.allocatePages(context, PAGE_TYPE_ID_MAP,
                                  1, NULL, &pid, slice());
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to allocate pid[%d] on smp:%d", pid, rc);
         goto error;
      }

      accessor.fini(context);
   done:
      return rc;
   error:
      accessor.fini(context);
      goto done;
   }

   INT32 collectionSpace::releaseIdMapPageOnSMP(requestContext *context,
                                                PAGE_ID pid)
   {
      SDB_ASSERT(FALSE, "todo");
      return SDB_OK;
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
      
      rc = smp.init(context, FILE_TYPE_DD,
                     smpPid, flags, _su);
      if (SDB_OK != rc)
      {
         goto error;
      }

      rc = smp.allocatePages(context, type, count, lpids, pids, args);
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

      rc = imp.init(context, FILE_TYPE_DM, impPid,
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

   PAGE_ID collectionSpace::getDataSMPPId(PAGE_ID pid)const
   {
      SDB_ASSERT(0 <_parameters.dataSMPCapacity, "can not be invalid");
      SDB_ASSERT(0 < _parameters.dataSMPCountPerFile, "can not be invalid");
      SDB_ASSERT(ossIsPowerOf2(_parameters.dataSMPCountPerFile), "must be power of 2");
      SDB_ASSERT(0 < _parameters.pageCountPerDataFile, "can not be invalid");
      SDB_ASSERT(ossIsPowerOf2(_parameters.pageCountPerDataFile), "must be power of 2");
      if (OSS_LIKELY(INVALID_PAGE_ID != pid))
      {
         PAGE_ID firstSMPOfThisFile = (pid & (~(_parameters.pageCountPerDataFile - 1)));
         UINT32 delta = ((pid / _parameters.dataSMPCapacity) &
                         (_parameters.dataSMPCountPerFile - 1));
         return firstSMPOfThisFile + delta;
      }

      return INVALID_PAGE_ID;
   }

   PAGE_ID collectionSpace::getMetaSMPPid(PAGE_ID pid)const
   {
      if (OSS_LIKELY(INVALID_PAGE_ID != pid))
      {
         return pid / _parameters.metaSMPCapcacity;
      }
      return INVALID_PAGE_ID;
   }

   UINT32 collectionSpace::getSystemPageCountInMetaFile()const
   {
      SDB_ASSERT(0 < _parameters.metaSMPCountPerFile, "can not be invalid");
      return _parameters.metaSMPCountPerFile + SYSTEM_PAGE_COUNT_IN_META_EXCEPT_SMP;
   }

   PAGE_ID collectionSpace::getCSGlobalMetaPid()const
   {
      SDB_ASSERT(0 < _parameters.metaSMPCountPerFile, "can not be invalid");
      return _parameters.metaSMPCountPerFile;
   }

   PAGE_ID collectionSpace::getSystemIdMapPid()const
   {
      return getCSGlobalMetaPid() + 1;
   }

   PAGE_ID collectionSpace::getDataIMPPid(PAGE_ID lpid)const
   {
      SDB_ASSERT(0 < _parameters.dataIDMapCapacity, "impossible");
      SDB_ASSERT(INVALID_PAGE_ID != lpid, "can not be invalid");
      if (OSS_LIKELY(INVALID_PAGE_ID != lpid))
      {
         return (lpid / _parameters.dataIDMapCapacity) + getSystemIdMapPid();
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
      SDB_ASSERT(0 < _parameters.dataSMPCountPerFile, "can not be invalid");
      SDB_ASSERT(0 < _parameters.pageCountPerDataFile, "can not be invalid");

      ossPoolVector<UINT32> occupied;
      occupied.reserve(_parameters.dataSMPCountPerFile);
      UINT32 sequence = 0;
      PAGE_ID pid = INVALID_PAGE_ID;
      ossScopedLock lock(&_extendingDataSpaceLatch);
      UINT32 count = _inMemDataSMP.getPageCount();
      BOOLEAN rollbackFile = FALSE;


      if (NULL != oldPageCount && *oldPageCount < count)
      {
         goto done;
      }

      rc = _su->createNewDataFile(context, &sequence);
      if (SDB_OK != rc)
      {
         goto error;
      }
      rollbackFile = TRUE;

      pid = sequence * _parameters.pageCountPerDataFile;

      rc = initDataSMP(context, pid, _parameters.dataSMPCountPerFile);
      if (SDB_OK != rc)
      {
         goto error;
      }

      occupied.push_back(_parameters.dataSMPCountPerFile);

      for (UINT32 i = 1; i < _parameters.dataSMPCountPerFile; ++i)
      {
         rc = initDataSMP(context, pid + i, 0);
         if (SDB_OK != rc)
         {
            goto error;
         }
         occupied.push_back(0);
      }

      rc = _su->fsync(FILE_TYPE_DD, pid, _parameters.dataSMPCountPerFile, TRUE);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to fsync smp:%d", rc);
         goto error;
      }

      rc = _inMemDataSMP.allocateBitPages(occupied);
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
      SDB_ASSERT(0 < _parameters.pageCountPerMetaFile, "can not be invalid");
      SDB_ASSERT(0 < _currentPageCountInMeta, "can not be zero");
      UINT32 pageCountInSeg = 0;
      PAGE_ID impPid = INVALID_PAGE_ID;
      BOOLEAN rollbackSMP = FALSE;
      ossScopedLock lock(&_extendingDataSpaceLatch);

      if (oldPageAllocated < _currentPageCountInMeta)
      {
         goto done;
      }
      if (_currentPageCountInMeta == _parameters.pageCountPerMetaFile)
      {
         rc = SDB_VESSEL_FS_UPPER_LIMIT;
         goto error;
      }

      rc = _su->getCoreArgs(FILE_TYPE_DM, NULL, &pageCountInSeg);
      if (OSS_UNLIKELY(SDB_OK != rc))
      {
         goto error;
      }

      SDB_ASSERT(64 == pageCountInSeg, "must be 64");
      /// all pages in current file were allocated.
      /// need to allocate new segment.
      if (0 == (_currentPageCountInMeta & (pageCountInSeg - 1)))
      {
         UINT32 segCount = (_currentPageCountInMeta >> 6) + 1;/// _currentPageCountInMeta / 64
         /// segment count must be specified that we do not need to
         /// rollback file size if get any error then.
         rc = _su->extendMetaFile(context, &segCount);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to extentd meta file:%d", rc);
            goto error;
         }
      }

      impPid = _currentPageCountInMeta;
      rc = initIMP(context, _parameters.metaPageSize, impPid);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to init new imp[%d], rc:%d", impPid, rc);
         goto error;
      }

      rc = allocateIdMapPageOnSMP(context, impPid);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to allocate imp[%d], rc:%d", impPid, rc);
         goto error;
      }
      rollbackSMP = TRUE;
      ++_currentPageCountInMeta;

      rc = _inMemLpidPool.allocateNewBitPage();
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to extend in-mem bitmap:%d", rc);
         goto error;
      }
   done:
      return rc;
   error:
      if (rollbackSMP)
      {
         INT32 tmpRC = releaseIdMapPageOnSMP(context, impPid);
         if (SDB_OK != tmpRC)
         {
            PD_LOG(PDSEVERE, "failed to rollback smp of meta, space id[%d], pid[%d], rc:%d",
                   getSpaceID(), impPid, tmpRC);
            _inMemLpidPool.incPageCount();
         }
      }
      goto done;
   }

   INT32 collectionSpace::cacheGlobalMetaData(requestContext *context)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(NULL != _su, "can not be null");
      csgpAccessor accessor;
      UINT32 flags = PAGE_ACCESSOR_FLAG_DIRECT;

      rc = accessor.init(context,
                         FILE_TYPE_DM,
                         getCSGlobalMetaPid(),
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
      impAccessor imp;
      ossValuePtr ptr = 0;
      PAGE_ID pid = INVALID_PAGE_ID;
      UINT32 clScanned = 0;
      PAGE_ID systemPid = getSystemIdMapPid();
      UINT32 capacity = 0;

      rc = getCapacityOfCLRecordPage(_parameters.dataPageSize, capacity);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to get capacity of cl record page:%d", rc);
         goto error;
      }

      rc = _su->getPagePtr(FILE_TYPE_DM, systemPid, ptr);
      if (SDB_OK != rc)
      {
         goto error;
      }

      rc = imp.initWithDirectMode(context, FILE_TYPE_DM, systemPid,
                                  _parameters.dataPageSize, ptr);
      if (SDB_OK != rc)
      {
         goto error;
      }

      for (UINT32 i = 0; i < _parameters.dataIDMapCapacity && clScanned < MAX_CL_MB_COUNT; ++i)
      {

         rc = imp.getPidByOffset(i, &pid, NULL);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to get next pid from imp:%d", rc);
            goto error;
         }

         clScanned += capacity;

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
      ossValuePtr ptr = 0;
      crpAccessor crp;
      UINT32 capacity = 0;
      collectionRecord record;

      rc = getCapacityOfCLRecordPage(_parameters.dataPageSize, capacity);
      if (SDB_OK != rc)
      {
         goto error;
      }

      rc = _su->getPagePtr(FILE_TYPE_DD, pid, ptr);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to get crp page:%d", rc);
         goto error;
      }

      rc = crp.initWithDirectMode(context, FILE_TYPE_DD, pid,
                                  _parameters.dataPageSize, ptr);
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
      rc = accessor.init(context, FILE_TYPE_DM,
                          getCSGlobalMetaPid(),
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
      rc = accessor.init(context, FILE_TYPE_DM,
                         getCSGlobalMetaPid(),
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
      SDB_ASSERT(0 < _parameters.metaPageSize, "can not be invalid");
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

      rc = initIMP(context, _parameters.metaPageSize, getSystemIdMapPid());
      if (SDB_OK != rc)
      {
         goto error;
      }

      rc = _su->fsync(FILE_TYPE_DM, 0, getSystemPageCountInMetaFile(), TRUE);
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
      SDB_ASSERT(NULL != context, "can not be null");
      SDB_ASSERT(NULL != _su, "can not be null");
      SDB_ASSERT(_su->isOpen(), "must be open");
      SDB_ASSERT(0 == _su->getMetaSegmentCount(), "must be zero");

      ossValuePtr ptr = 0;
      smpAccessor smp;
      UINT32 segCount = 1;

      rc = _su->extendMetaFile(context, &segCount);
      if (SDB_OK != rc)
      {
         goto error;
      }

      rc = _su->getPagePtr(FILE_TYPE_DM, SMP_PAGE_ID, ptr);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to get page ptr:%d", rc);
         goto error;
      }

      rc = smp.initWithDirectMode(context,
                                  FILE_TYPE_DM,
                                  SMP_PAGE_ID,
                                  _parameters.metaPageSize,
                                  ptr, FALSE, FALSE);
      if (SDB_OK != rc)
      {
         goto error;
      }

      /// first n pages are occupied by system pages.
      rc = smp.initSMP(context, getSystemPageCountInMetaFile());
      if (SDB_OK != rc)
      {
         goto error;
      }

      smp.fini(context);

      for (UINT32 i = 1; i < _parameters.metaSMPCountPerFile; ++i)
      {
         rc = _su->getPagePtr(FILE_TYPE_DM, SMP_PAGE_ID + i, ptr);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to get page ptr:%d", rc);
            goto error;
         }

         rc = smp.initWithDirectMode(context,
                                     FILE_TYPE_DM,
                                     SMP_PAGE_ID + i,
                                     _parameters.metaPageSize,
                                     ptr, FALSE, FALSE);
         if (SDB_OK != rc)
         {
            goto error;
         }

         rc = smp.initSMP(context, 0);
         if (SDB_OK != rc)
         {
            goto error;
         }

         smp.fini(context);
      }
   done:
      return rc;
   error:
      smp.fini(context);
      goto done;
   }

   INT32 collectionSpace::initGMP(requestContext *context,
                                  const csMetaRecord &record)
   {
      INT32 rc = SDB_OK;
      csgpAccessor csgp;
      ossValuePtr ptr = 0;
      PAGE_ID pid = getCSGlobalMetaPid();

      rc = _su->getPagePtr(FILE_TYPE_DM, pid, ptr);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to get cs global meta page:%d", rc);
         goto error;
      }

      rc = csgp.initWithDirectMode(context,
                                   FILE_TYPE_DM,
                                   pid, _parameters.metaPageSize,
                                   ptr, FALSE, FALSE);
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
                                  UINT32 pageSize,
                                  PAGE_ID pid)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(NULL != context, "can not be null");
      SDB_ASSERT(INVALID_PAGE_ID != pid, "can not be invalid");
      impAccessor imp;
      ossValuePtr ptr = 0;

      rc = _su->getPagePtr(FILE_TYPE_DM, pid, ptr);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to get cs global meta page:%d", rc);
         goto error;
      }

      rc = imp.initWithDirectMode(context,
                     FILE_TYPE_DM,
                     pid, pageSize,
                     ptr, FALSE, FALSE);
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
                                      PAGE_ID pid,
                                      UINT32 occupied)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(NULL != context, "can not be null");
      SDB_ASSERT(INVALID_PAGE_ID != pid, "can not be invalid");
      SDB_ASSERT(0 < _parameters.dataPageSize, "can not be invalid");
      ossValuePtr ptr = 0;
      smpAccessor smp;

      rc = _su->getPagePtr(FILE_TYPE_DD, pid, ptr);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to get page ptr, pid[%d], rc:%d", pid, rc);
         goto error;
      }

      rc = smp.initWithDirectMode(context, FILE_TYPE_DD,
                                  pid, _parameters.dataPageSize,
                                  ptr, FALSE, FALSE);
      if (SDB_OK != rc)
      {
         goto error;
      }

      rc = smp.initSMP(context, occupied);
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

   BOOLEAN collectionSpace::addToCreatingIndex(const strSlice &clName,
                                               utilCLInnerID innerID)
   {
      SDB_ASSERT(!clName.empty(), "can not be empty");
      BOOLEAN r = FALSE;
      ossScopedLock lock(&_runtimeIndexLatch, EXCLUSIVE);
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
      ossScopedLock lock(&_runtimeIndexLatch, EXCLUSIVE);
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
      ossScopedLock lock(&_runtimeIndexLatch, EXCLUSIVE);

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
      ossScopedLock lock(&_runtimeIndexLatch, EXCLUSIVE);

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
      ossScopedLock lock(&_runtimeIndexLatch, EXCLUSIVE);
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
      ossScopedLock lock(&_runtimeIndexLatch, SHARED);
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
      
      ossScopedLock lock(&_runtimeIndexLatch, SHARED);
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
      ossScopedLock lock(&_runtimeIndexLatch, SHARED);
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
      ossScopedLock lock(&_runtimeIndexLatch, SHARED);
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
      SDB_ASSERT(NULL != _su && _su->isOpen(), "can not be null");
      UINT32 maxPageCountPerSeg = 0;
      UINT32 maxSegCountPerFile = 0;

      rc = _su->getCoreArgs(FILE_TYPE_DM, &_parameters.metaPageSize,
                            &maxPageCountPerSeg, &maxSegCountPerFile);
      if (SDB_OK != rc)
      {
         goto error;
      }

      _parameters.pageCountPerMetaFile = maxPageCountPerSeg * maxSegCountPerFile;

      rc = get64AlignedCapacityOfIMP(_parameters.metaPageSize,
                                     _parameters.dataIDMapCapacity);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to get capacity of imp:%d", rc);
         goto error;
      }

      rc = _su->getCoreArgs(FILE_TYPE_DD, &_parameters.dataPageSize,
                            &maxPageCountPerSeg, &maxSegCountPerFile);
      if (SDB_OK != rc)
      {
         goto error;
      }

      _parameters.pageCountPerDataFile = maxPageCountPerSeg * maxSegCountPerFile;

      if (!getSMPCapacityAndCount(_parameters.metaPageSize,
                                  _parameters.metaSMPCapcacity,
                                  &_parameters.metaSMPCountPerFile))
      {
         PD_LOG(PDERROR, "failed to get data smp capacity");
         rc = SDB_VESSEL_INTERNAL_ERR;
         goto error;
      }

      if (!getSMPCapacityAndCount(_parameters.dataPageSize,
                                  _parameters.dataSMPCapacity,
                                  &_parameters.dataSMPCountPerFile))
      {
         PD_LOG(PDERROR, "failed to get data smp capacity");
         rc = SDB_VESSEL_INTERNAL_ERR;
         goto error;
      }
   done:
      return rc;
   error:
      _parameters.reset();
      goto done;
   }

   INT32 collectionSpace::initInMemLpidPool(requestContext *context)
   {
      SDB_ASSERT(NULL != context, "can not be null");
      SDB_ASSERT(NULL != _su, "can not be null");
      SDB_ASSERT(!_inMemLpidPool.isInitialized(), "do not reinit");
      SDB_ASSERT(0 < _parameters.metaPageSize, "can not be invalid");
      INT32 rc = SDB_OK;
      CHAR *buffer = NULL;
      smpAccessor smp;
      impAccessor imp;
      UINT32 sysPageCount = getSystemPageCountInMetaFile();
      UINT32 impBitsCount = _parameters.dataIDMapCapacity >> 6;

      buffer = context->allocateBuffer(_parameters.metaPageSize);
      if (NULL == buffer)
      {
         PD_LOG(PDERROR, "failed to allcoate mem");
         rc = SDB_OOM;
         goto error;
      }

      rc = _inMemLpidPool.init(_parameters.dataIDMapCapacity,
                               PAGE_COUNT_IN_EXTENT);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to init lpid bitmap:%d", rc);
         goto error;
      }
      /// skip system id map page.
      _inMemLpidPool.incPageCount();

      for (UINT32 i = 0; i < _parameters.metaSMPCountPerFile; ++i)
      {
         INT32 free = 0;
         UINT32 scanCount = 0;
         PAGE_ID minPid = i * _parameters.metaSMPCapcacity + sysPageCount;
         ossValuePtr ptr = 0;
         PAGE_ID smpPid = i + SMP_PAGE_ID;

         rc = _su->getPagePtr(FILE_TYPE_DM, smpPid, ptr);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to get smp pid[%d], rc:%d", smpPid, rc);
            goto error;
         }

         rc = smp.initWithDirectMode(context, FILE_TYPE_DM,
                                     i + SMP_PAGE_ID, _parameters.metaPageSize,
                                     ptr, TRUE, TRUE);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to init smp accessor:%d", rc);
            goto error;
         }

         rc = smp.getFreeCount(context, free);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to access smp[%d], rc:%d",
                   i + SMP_PAGE_ID, rc);
            goto error;
         }
         smp.fini(context);

         if (free < 0 ||
             _parameters.metaSMPCapcacity < ((UINT32)free + sysPageCount))
         {
            PD_LOG(PDERROR, "unexpected free count[%d] in page[%d]",
                   free, i + SMP_PAGE_ID);
            rc = SDB_VESSEL_INTERNAL_ERR;
            goto error;
         }
         else if ((INT32)_parameters.metaSMPCapcacity == free)
         {
            SDB_ASSERT(0 < i, "can not be the first smp");
            break;
         }

         _currentPageCountInMeta += (_parameters.metaSMPCapcacity - free);
         scanCount = _parameters.metaSMPCapcacity - free - sysPageCount;
         for (UINT32 j = 0; j < scanCount; ++j)
         {
            UINT32 freeInImp = 0;
            PAGE_ID impPid = minPid + j;
            ossValuePtr impPtr = 0;
            rc = _su->getPagePtr(FILE_TYPE_DM, impPid, impPtr);
            if (SDB_OK != rc)
            {
               PD_LOG(PDERROR, "failed to get imp page ptr[%d], rc:%d", impPid, impPtr);
               goto error;
            }

            rc = imp.initWithDirectMode(context, FILE_TYPE_DM,
                                        impPid, _parameters.metaPageSize,
                                        impPtr, TRUE, TRUE);
            if (SDB_OK != rc)
            {
               PD_LOG(PDERROR, "failed to init imp accessor at page[%d], rc:%d", impPid, rc);
               goto error;
            }

            rc = imp.dumpAsBitMap((UINT64*)buffer, freeInImp);
            if (SDB_OK != rc)
            {
               PD_LOG(PDERROR, "failed to dump imp[%d], rc:%d", impPid, rc);
               goto error;
            }

            imp.fini(context);
            if (0 == freeInImp)
            {
               _inMemLpidPool.incPageCount();
            }
            else
            {
               rc = _inMemLpidPool.mapNewBitPage(impBitsCount, (const UINT64 *)buffer);
               if (SDB_OK != rc)
               {
                  PD_LOG(PDERROR, "failed to map to bit map:%d", rc);
                  goto error;
               }
            }
         }

         if (0 != free)
         {
            break;
         }
         else
         {
            sysPageCount = 0;
         }
      }
   done:
      if (NULL != buffer)
      {
         context->releaseBuffer(buffer, _parameters.metaPageSize);
      }
      return rc;
   error:
      smp.fini(context);
      imp.fini(context);
      _inMemLpidPool.fini();
      goto done;
   }

   INT32 collectionSpace::initInMemBitMaps(requestContext *context)
   {
      INT32 rc = SDB_OK;
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
      SDB_ASSERT(0 < _parameters.dataSMPCapacity, "can not be invalid");
      SDB_ASSERT(0 < _parameters.dataPageSize, "can not be invalid");
      SDB_ASSERT(NULL != _su, "can not be null");
      SDB_ASSERT(!_inMemDataSMP.isInitialized(), "do not reinit");
      CHAR *buffer = NULL;
      const UINT32 buffSize = _parameters.dataPageSize;
      UINT32 fileCount = 0;
      UINT32 bitsCount = 0;
      smpAccessor accessor;

      buffer = context->allocateBuffer(buffSize);
      if (NULL == buffer)
      {
         PD_LOG(PDERROR, "failed to allcoate mem");
         rc = SDB_OOM;
         goto error;
      }

      rc = _inMemDataSMP.init(_parameters.dataSMPCapacity, PAGE_COUNT_IN_EXTENT);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to init inmem-bitmap:%d", rc);
         goto error;
      }

      fileCount = _su->getDataFileCount();
      bitsCount = _parameters.dataSMPCapacity >> 6;
      for (UINT32 i = 0; i < fileCount; ++i)
      {
         for (UINT32 j = 0; j < _parameters.dataSMPCountPerFile; ++j)
         {
            PAGE_ID pid = i * _parameters.pageCountPerDataFile + j;
            ossValuePtr ptr = 0;
            rc = _su->getPagePtr(FILE_TYPE_DD, pid, ptr);
            if (SDB_OK != rc)
            {
               PD_LOG(PDERROR, "failed to get data page[%d], rc:%d", pid, rc);
               goto error;
            }

            rc = accessor.initWithDirectMode(context, FILE_TYPE_DD,
                                             pid, _parameters.dataPageSize,
                                             ptr, TRUE, TRUE);
            if (SDB_OK != rc)
            {
               PD_LOG(PDERROR, "failed to init smp accessor:%d", rc);
               goto error;
            }
            rc = accessor.dumpSMP(context, buffSize, buffer);
            if (SDB_OK != rc)
            {
               PD_LOG(PDERROR, "failed to dump smp[%d], rc:%d", pid, rc);
               goto error;
            }
            accessor.fini(context);
            rc = _inMemDataSMP.mapNewBitPage(bitsCount, (const UINT64 *)(buffer));
            if (SDB_OK != rc)
            {
               PD_LOG(PDERROR, "failed to map smp to inmem bitmap, pid[%d], rc:%d", pid, rc);
               goto error;
            }
         }
      }
   done:
      if (NULL != buffer)
      {
         context->releaseBuffer(buffer, buffSize);
      }
      return rc;
   error:
      accessor.fini(context);
      _inMemDataSMP.fini();
      goto done;
   }
}//namespace vessel
}//namespace engine